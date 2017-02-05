#include "editor.h"
#include "eye.h"
#include "map.h"

#include <algorithm>
#include <glm/gtx/norm.hpp>


namespace {
	bool point_in_rect(const glm::vec2& p, glm::vec2 r1, glm::vec2 r2) {
		if (r1.x > r2.x) std::swap(r1.x, r2.x);
		if (r1.y > r2.y) std::swap(r1.y, r2.y);
		return p.x >= r1.x && p.x <= r2.x && p.y >= r1.y && p.y <= r2.y;
	}


	float point_line_segment_distance(const glm::vec2& p, glm::vec2 l1, glm::vec2 l2) {
		glm::vec2 pl = p - l1;
		glm::vec2 ll = l2 - l1;
		float t = std::max(0.0f, std::min(1.0f, glm::dot(pl, ll) / glm::length2(ll)));
		return glm::distance(p, l1 + t * ll);
	}
}



void Editor::keyboard(const SDL_KeyboardEvent& key) {
	if (key.type != SDL_KEYDOWN) return;

	// toggle grid
	if (key.keysym.sym == SDLK_HASH) {
		m_grid_enabled = !m_grid_enabled;
		return;
	}

	// snap to grid
	if (key.keysym.sym == SDLK_v) {
		snap_to_grid();
		return;
	}

	// delete
	if (key.keysym.sym == SDLK_x) {
		// NOTE: inverse direction is important
		for (int i = m_selection.size() - 1;  i >= 0; --i) {
			WallRef& ref = m_selection[i];
			Sector& s = map.sectors[ref.sector_nr];
			s.walls.erase(s.walls.begin() + ref.wall_nr);
		}
		m_selection.clear();
		for (auto it = map.sectors.begin(); it != map.sectors.end();) {
			if (it->walls.size() <= 2) it = map.sectors.erase(it);
			else ++it;
		}
		map.setup_portals();
		return;
	}

	// split along edge
	if (key.keysym.sym == SDLK_b) {
		if (m_selection.size() != 2) return;
		if (m_selection[0].sector_nr != m_selection[1].sector_nr) return;
		Sector& s = map.sectors[m_selection[0].sector_nr];
		int n1 = m_selection[0].wall_nr;
		int n2 = m_selection[1].wall_nr;
		if (n1 == n2 || (n1 == 0 && n2 == (int) s.walls.size() - 1)) return;
		m_selection.clear();

		//TODO
		Sector new_sector;
		new_sector.floor_height = s.floor_height;
		new_sector.ceil_height = s.ceil_height;
		new_sector.walls = std::vector<Wall>(s.walls.begin() + n1, s.walls.begin() + n2 + 1);
		s.walls.erase(s.walls.begin() + n1 + 1, s.walls.begin() + n2);

		map.sectors.emplace_back(std::move(new_sector));
		map.setup_portals();

		return;
	}

}


void Editor::mouse_button(const SDL_MouseButtonEvent& button) {
	const uint8_t* ks = SDL_GetKeyboardState(nullptr);



	if (button.button == SDL_BUTTON_LEFT) {
		if (button.type == SDL_MOUSEBUTTONDOWN) {

			// add vertex
			if (ks[SDL_SCANCODE_C]) {
				float dist = 10;
				WallRef ref { -1 };
				for (int i = 0; i < (int) map.sectors.size(); ++i) {
					Sector& sector = map.sectors[i];
					for (int j = 0; j < (int) sector.walls.size(); ++j) {
						Wall& w1 = sector.walls[j];
						Wall& w2 = sector.walls[(j + 1) % sector.walls.size()];
						float d = point_line_segment_distance(m_cursor, w1.pos, w2.pos);
						if (d < dist) {
							dist = d;
							ref.sector_nr = i;
							ref.wall_nr = j;
						}
					}
				}
				if (ref.sector_nr != -1) {
					m_selection.clear();

					Sector& sector = map.sectors[ref.sector_nr];
					sector.walls.insert(sector.walls.begin() + ref.wall_nr + 1, { m_cursor });
					m_selection.push_back({ ref.sector_nr, ref.wall_nr + 1 });

					WallRef& next = sector.walls[ref.wall_nr].next;
					if (next.sector_nr != -1) {
						Sector& next_sector = map.sectors[next.sector_nr];
						next_sector.walls.insert(next_sector.walls.begin() + next.wall_nr + 1, { m_cursor });
						m_selection.push_back({ next.sector_nr, next.wall_nr + 1 });
					}
					map.setup_portals();
				}
			}


		}

		// auto snapping
		if (button.type == SDL_MOUSEBUTTONUP) {
			snap_to_grid();
		}
	}

	// select vertices
	if (button.button == SDL_BUTTON_RIGHT) {
		if (button.type == SDL_MOUSEBUTTONDOWN) {
			m_selecting = true;
			m_select_pos = m_cursor;
		}
		else {
			m_selecting = false;

			// add to selection
			bool keep_old = ks[SDL_SCANCODE_LCTRL] || ks[SDL_SCANCODE_RCTRL];
			if (!keep_old) m_selection.clear();

			glm::vec2 b1 = m_select_pos;
			glm::vec2 b2 = m_cursor;
			bool just_one = b1 == b2;
			if (just_one) {
				b1 -= glm::vec2(m_zoom * 7);
				b2 += glm::vec2(m_zoom * 7);
			}
			for (int i = 0; i < (int) map.sectors.size(); ++i) {
				Sector& sector = map.sectors[i];
				for (int j = 0; j < (int) sector.walls.size(); ++j) {
					Wall& wall = sector.walls[j];
					if (point_in_rect(wall.pos, b1, b2)) {
						m_selection.push_back({ i, j });
						if (just_one) {
							i = map.sectors.size();
							break;
						}
					}
				}
			}

			if (keep_old) {
				// keep selection sorted
				std::sort(m_selection.begin(), m_selection.end());
				// remove duplicate
				for (int i = 0; i < (int) m_selection.size() - 1;) {
					if (m_selection[i] == m_selection[i + 1]) {
						m_selection.erase(m_selection.begin() + i + 1);
					}
					else ++i;
				}
			}
		}
	}
}


void Editor::mouse_wheel(const SDL_MouseWheelEvent& wheel) {
	// zoom
	m_zoom *= powf(0.9, wheel.y);
}

void Editor::snap_to_grid() {
	for (WallRef& ref : m_selection) {
		Wall& wall = map.sectors[ref.sector_nr].walls[ref.wall_nr];
		wall.pos.x = std::floor(wall.pos.x + 0.5);
		wall.pos.y = std::floor(wall.pos.y + 0.5);
	}
	map.setup_portals();
}

void Editor::draw() {

	int x, y;
	uint32_t buttons = SDL_GetMouseState(&x, &y);
	glm::vec2 old_cursor = m_cursor;
	m_cursor = glm::vec2(
		x - rmw::context.get_width() / 2,
		y - rmw::context.get_height() / 2);
	m_cursor *= m_zoom;
	m_cursor -= m_scroll;

	// scrolling
	glm::vec2 mov = m_cursor - old_cursor;
	if (glm::length2(mov) > 0) {
		if (buttons & SDL_BUTTON_MMASK) {
			m_scroll += mov;
			m_cursor -= mov;
		}

		// move selection
		if (buttons & SDL_BUTTON_LMASK) {
			for (WallRef& ref : m_selection) {
				Wall& wall = map.sectors[ref.sector_nr].walls[ref.wall_nr];
				wall.pos += mov;
			}
			map.setup_portals();
		}
	}




	// start drawing stuff
	renderer2D.origin();
	renderer2D.translate(rmw::context.get_width() / 2, rmw::context.get_height() / 2);
	renderer2D.scale(1 / m_zoom);
	renderer2D.translate(m_scroll);

/*
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/


	// grid
	if (m_grid_enabled) {
		renderer2D.set_color(100, 100, 100);
		int inc = 1;
		int w = rmw::context.get_width() / 2;
		int h = rmw::context.get_height() / 2;
		float x1 = std::floor((-m_scroll.x - m_zoom * w) / inc) * inc;
		float x2 = std::floor((-m_scroll.x + m_zoom * w) / inc + 1) * inc;
		float y1 = std::floor((-m_scroll.y - m_zoom * h) / inc) * inc;
		float y2 = std::floor((-m_scroll.y + m_zoom * h) / inc + 1) * inc;

		for (int x = x1; x < x2; x += inc) renderer2D.line(x, y1, x, y2);
		for (int y = y1; y < y2; y += inc) renderer2D.line(x1, y, x2, y);


	}




	// map
	renderer2D.set_point_size(7);
	for (int i = 0; i < (int) map.sectors.size(); ++i) {
		const Sector& sector = map.sectors[i];

		for (int j = 0; j < (int) sector.walls.size(); ++j) {
			auto& w1 = sector.walls[j];
			auto& w2 = sector.walls[(j + 1) % sector.walls.size()];

			// full wall
			if (w1.next.sector_nr == -1) {
//				if (i == eye.get_location().sector) renderer2D.set_color(100, 100, 255);
//				else
					renderer2D.set_color(200, 200, 200);
			}
			else {
				renderer2D.set_color(200, 0, 0);
			}

			renderer2D.line(w1.pos, w2.pos);
			renderer2D.point(w1.pos);

		}
	}


	// player
	{
		glm::vec2 p(eye.get_location().pos.x, eye.get_location().pos.z);
		float co = cosf(eye.get_ang_y());
		float si = sinf(eye.get_ang_y());
		glm::mat2x2 m = { {  co,  si }, { -si,  co }, };
		glm::vec2 p1 = m * glm::vec2( 0.0, -0.6) + p;
		glm::vec2 p2 = m * glm::vec2( 0.3,  0.2) + p;
		glm::vec2 p3 = m * glm::vec2(-0.3,  0.2) + p;
		renderer2D.set_color(0, 255, 255);
		renderer2D.line(p1, p2);
		renderer2D.line(p2, p3);
		renderer2D.line(p3, p1);
	}


	// selection
	renderer2D.set_color(255, 255, 0);
	renderer2D.set_point_size(7);
	for (const WallRef& ref : m_selection) {
		Wall& w = map.sectors[ref.sector_nr].walls[ref.wall_nr];
//		renderer2D.rect(w.pos + glm::vec2(m_zoom * 3), w.pos - glm::vec2(m_zoom * 3));
		renderer2D.point(w.pos);
	}


	// selection box
	if (m_selecting) {
		renderer2D.set_color(255, 255, 0);
		renderer2D.rect(m_cursor, m_select_pos);
	}



	// cursor
//	renderer2D.set_color(255, 255, 255);
//	renderer2D.line(
//		m_cursor - glm::vec2(3 * m_zoom, 0),
//		m_cursor + glm::vec2(3 * m_zoom, 0));
//	renderer2D.line(
//		m_cursor - glm::vec2(0, 3 * m_zoom),
//		m_cursor + glm::vec2(0, 3 * m_zoom));



	renderer2D.flush();
}

#include "editor.h"
#include "eye.h"
#include "map.h"


namespace {

	bool point_in_rect(const glm::vec2& p, glm::vec2 r1, glm::vec2 r2) {
		if (r1.x > r2.x) std::swap(r1.x, r2.x);
		if (r1.y > r2.y) std::swap(r1.y, r2.y);
		return p.x >= r1.x && p.x <= r2.x && p.y >= r1.y && p.y <= r2.y;
	}

}



void Editor::update_cursor(int x, int y) {
	m_cursor = glm::vec2(
		x - rmw::context.get_width() / 2,
		y - rmw::context.get_height() / 2);
	m_cursor *= m_zoom;
	m_cursor -= m_scroll;
}


void Editor::mouse_motion(const SDL_MouseMotionEvent& motion) {
}


void Editor::mouse_button(const SDL_MouseButtonEvent& button) {
	if (button.button == SDL_BUTTON_LEFT) {
		if (button.state == SDL_PRESSED) {
			m_selecting = true;
			m_select_pos = m_cursor;
		}
		else {
			m_selecting = false;

			const uint8_t* ks = SDL_GetKeyboardState(nullptr);
			if (!ks[SDL_SCANCODE_LCTRL] && !ks[SDL_SCANCODE_RCTRL]) m_selection.clear();

			// select vertices
			for (int i = 0; i < (int) map.sectors.size(); ++i) {
				Sector& sector = map.sectors[i];
				for (int j = 0; j < (int) sector.walls.size(); ++j) {
					Wall& wall = sector.walls[j];
					if (point_in_rect(wall.pos, m_select_pos, m_cursor)) {
						m_selection.push_back({ i, j });
					}
				}
			}
		}
	}
}


void Editor::mouse_wheel(const SDL_MouseWheelEvent& wheel) {
	// zoom
	m_zoom *= powf(0.9, wheel.y);
}


void Editor::draw() {

	// scrolling
	int x, y;
	uint32_t buttons = SDL_GetMouseState(&x, &y);
	glm::vec2 old_cursor = m_cursor;
	update_cursor(x, y);
	glm::vec2 mov = m_cursor - old_cursor;
	if (buttons & SDL_BUTTON_RMASK) {
		m_scroll += mov;
		m_cursor -= mov;
	}

	const uint8_t* ks = SDL_GetKeyboardState(nullptr);
	// move selection
	if (ks[SDL_SCANCODE_G]) {
		for (WallRef& ref : m_selection) {
			Wall& wall = map.sectors[ref.sector_nr].walls[ref.wall_nr];
			wall.pos += mov;
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


	// map
	for (int i = 0; i < (int) map.sectors.size(); ++i) {
		const Sector& sector = map.sectors[i];

		for (int j = 0; j < (int) sector.walls.size(); ++j) {
			auto& w1 = sector.walls[j];
			auto& w2 = sector.walls[(j + 1) % sector.walls.size()];

			// full wall
			if (w1.next.sector_nr == -1) {
				if (i == eye.get_location().sector) renderer2D.set_color(100, 100, 255);
				else renderer2D.set_color(200, 200, 200);
			}
			else {
				renderer2D.set_color(200, 0, 0);
			}

			renderer2D.line(w1.pos, w2.pos);

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
		renderer2D.set_color(0, 0, 255);
		renderer2D.line(p1, p2);
		renderer2D.line(p2, p3);
		renderer2D.line(p3, p1);
	}


	// selection
	renderer2D.set_color(255, 255, 0);
	for (const WallRef& ref : m_selection) {
		Wall& w = map.sectors[ref.sector_nr].walls[ref.wall_nr];
		renderer2D.rect(w.pos + glm::vec2(m_zoom * 3), w.pos - glm::vec2(m_zoom * 3));
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

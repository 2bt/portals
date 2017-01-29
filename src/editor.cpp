#include "editor.h"
#include "eye.h"
#include "map.h"



void Editor::init() {
	m_renderer.init();
}
void Editor::mouse_motion(const SDL_MouseMotionEvent& motion) {
	if (motion.state & SDL_BUTTON_LMASK) {
		m_scroll += glm::vec2(motion.xrel, motion.yrel);

	}
}
void Editor::mouse_up(const SDL_MouseButtonEvent& button) {
}
void Editor::mouse_down(const SDL_MouseButtonEvent& button) {
}
void Editor::mouse_wheel(const SDL_MouseWheelEvent& wheel) {
	m_zoom *= powf(1.1, wheel.y);
}


void Editor::draw() {

	int x, y;
	SDL_GetMouseState(&x, &y);

	m_renderer.origin();
	m_renderer.translate(rmw::context.get_width() / 2, rmw::context.get_height() / 2);
	m_renderer.scale(m_zoom);
	m_renderer.translate(m_scroll);




	// draw player
	{
		glm::vec2 p(eye.get_pos().x, eye.get_pos().z);
		float co = cosf(eye.get_ang_y());
		float si = sinf(eye.get_ang_y());
		glm::mat2x2 m = { {  co,  si }, { -si,  co }, };
		glm::vec2 p1 = m * glm::vec2( 0.0, -0.6) + p;
		glm::vec2 p2 = m * glm::vec2( 0.3,  0.2) + p;
		glm::vec2 p3 = m * glm::vec2(-0.3,  0.2) + p;
		m_renderer.set_color(255, 255, 0);
		m_renderer.line(p1, p2);
		m_renderer.line(p2, p3);
		m_renderer.line(p3, p1);
	}


/*
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/


	for (auto& s : map.sectors) {
		for (int i = 0; i < s.wall_count; i++) {
			auto& w1 = map.walls[s.wall_index + i];
			auto& w2 = map.walls[w1.other_point];

			// full wall
			if (w1.next_sector == -1) m_renderer.set_color(200, 200, 200);
			else m_renderer.set_color(200, 0, 0);
			m_renderer.line(w1.pos, w2.pos);

		}
	}


	m_renderer.flush();
}

#include "editor.h"
#include "eye.h"
#include "map.h"



void Editor::init() {
	m_renderer.init();
}


void Editor::draw() {

	static int i = 0;
	if (!i++) init();


	float scale = 10;
	auto eye_pos = eye.get_pos();
	auto offset = glm::vec2(eye_pos.x, eye_pos.z) * scale;


	float co = cosf(eye.get_ang_y());
	float si = sinf(eye.get_ang_y());




/*
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/

	// draw camera
	m_renderer.set_color(255, 255, 0);
	m_renderer.line(0, 0, -si * scale, -co * scale);


	for (auto& s : map.sectors) {


		for (int i = 0; i < s.wall_count; i++) {

			auto& w1 = map.walls[s.wall_index + i];
			auto& w2 = map.walls[w1.other_point];
			auto p1 = w1.pos * -scale + offset;
			auto p2 = w2.pos * -scale + offset;

			// full wall
			if (w1.next_sector == -1) m_renderer.set_color(200, 200, 200);
			else m_renderer.set_color(200, 0, 0);
			m_renderer.line(p1.x, -p1.y, p2.x, -p2.y);

		}
	}


	m_renderer.flush();
}

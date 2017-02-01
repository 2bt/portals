#pragma once

#include "renderer2d.h"

class Editor {
public:
	void draw();
	void mouse_motion(const SDL_MouseMotionEvent& motion);
	void mouse_button(const SDL_MouseButtonEvent& button);
	void mouse_wheel(const SDL_MouseWheelEvent& wheel);

private:

	void update_cursor(int x, int y);

	glm::vec2				m_scroll;
	float					m_zoom = 0.1;
	glm::vec2				m_cursor;



	bool					m_selecting = false;
	glm::vec2				m_select_pos;
	std::vector<int>		m_selection;

};


extern Editor editor;

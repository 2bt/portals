#pragma once

#include "renderer2d.h"
#include "map.h"

class Editor {
public:
	void draw();
	void mouse_button(const SDL_MouseButtonEvent& button);
	void mouse_wheel(const SDL_MouseWheelEvent& wheel);
	void keyboard(const SDL_KeyboardEvent& key);

private:

	glm::vec2				m_scroll;
	float					m_zoom = 0.1;
	glm::vec2				m_cursor;



	bool					m_selecting = false;
	glm::vec2				m_select_pos;
	std::vector<WallRef>	m_selection;


	bool					m_grid_enabled = false;
};


extern Editor editor;

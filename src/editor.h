#pragma once

#include "map.h"

#include <SDL2/SDL.h>


class Editor {
public:
	void draw();
	void mouse_button(const SDL_MouseButtonEvent& button);
	void mouse_wheel(const SDL_MouseWheelEvent& wheel);
	void keyboard(const SDL_KeyboardEvent& key);

private:

	void snap_to_grid();
	void split_sector();
	void merge_sectors();


	glm::vec2				m_scroll;
	float					m_zoom = 0.1;
	glm::vec2				m_cursor;


	bool					m_selecting = false;
	glm::vec2				m_select_pos;
	std::vector<WallRef>	m_selection;

	bool					m_grid_enabled = false;

	bool					m_edit_enabled = false;
};


extern Editor editor;

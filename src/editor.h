#pragma once

#include "renderer2d.h"

class Editor {
public:
	void draw();
	void mouse_motion(const SDL_MouseMotionEvent& motion);
	void mouse_up(const SDL_MouseButtonEvent& button);
	void mouse_down(const SDL_MouseButtonEvent& button);
	void mouse_wheel(const SDL_MouseWheelEvent& wheel);

private:

	glm::vec2				m_scroll;
	float					m_zoom = 10;
	glm::vec2				m_curor;
};


extern Editor editor;

#pragma once

#include <glm/glm.hpp>


class Editor {
public:
	void draw();

private:


	glm::vec2	scroll;
	float		zoom = 10;
};


extern Editor editor;

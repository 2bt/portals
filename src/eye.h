#pragma once


#include <glm/gtx/transform.hpp>


class Eye {
public:
	void update();
	glm::mat4x4 get_view_mtx() const;
	glm::vec3	get_pos() const { return pos; }
	float		get_ang_y() const { return ang_y; }
private:
	glm::vec3	pos { 1.47, 2.246044, -8.26 };
	float		ang_x = 0.18;
	float		ang_y = -3.28;
};


extern Eye eye;

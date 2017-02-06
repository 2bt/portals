#pragma once


//#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include "map.h"


class Eye {
	friend class Map;
public:
	Eye();
	void				update();
	glm::mat4x4			get_view_mtx() const;
	const Location&		get_location() const { return loc; }
	float				get_ang_y() const { return ang_y; }
private:
	Location	loc;
	float		ang_x;
	float		ang_y;
};


extern Eye eye;

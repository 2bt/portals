#pragma once


//#include <glm/gtx/transform.hpp>
#include <glm/glm.hpp>
#include "editor.h"


class Eye {
	friend class Editor;
public:
	void                init();
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

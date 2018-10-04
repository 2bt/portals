#include "eye.h"


#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <SDL2/SDL.h>


Eye::Eye() {
	loc.pos = { 0, 0, 0 };
	loc.sector_nr = map.pick_sector(glm::vec2(loc.pos.x, loc.pos.z));
	ang_x = 0;
	ang_y = 0;
}


void Eye::update() {

	auto ks = SDL_GetKeyboardState(nullptr);

	ang_y += (ks[SDL_SCANCODE_RIGHT]	- ks[SDL_SCANCODE_LEFT]		) * 0.03;
	ang_x += (ks[SDL_SCANCODE_DOWN]		- ks[SDL_SCANCODE_UP]		) * 0.03;
	ang_x = std::max<float>(-M_PI * 0.5, std::min<float>(M_PI * 0.5, ang_x));

	float x = (ks[SDL_SCANCODE_D]		- ks[SDL_SCANCODE_A]		) * 0.3;
	float z = (ks[SDL_SCANCODE_S]		- ks[SDL_SCANCODE_W]		) * 0.3;
	float y = (ks[SDL_SCANCODE_SPACE]	- ks[SDL_SCANCODE_LSHIFT]	) * 0.3;

	float cy = cosf(ang_y);
	float sy = sinf(ang_y);
	float cx = cosf(ang_x);
	float sx = sinf(ang_x);

	glm::vec3 mov = {
		x * cy - z * sy * cx + sy * sx * y,
		y * cx + z * sx,
		x * sy + z * cy * cx - cy * sx * y,
	};


	map.clip_move(loc, mov);
}


glm::mat4x4 Eye::get_view_mtx() const {
	return	glm::rotate<float>(ang_x, glm::vec3(1, 0, 0)) *
			glm::rotate<float>(ang_y, glm::vec3(0, 1, 0)) *
			glm::translate(-loc.pos);
}


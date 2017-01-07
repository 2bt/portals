#include "eye.h"


#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>




void Eye::update() {

	auto ks = SDL_GetKeyboardState(nullptr);

	ang_y += (ks[SDL_SCANCODE_RIGHT]	- ks[SDL_SCANCODE_LEFT]		) * 0.02;
	ang_x += (ks[SDL_SCANCODE_DOWN]		- ks[SDL_SCANCODE_UP]		) * 0.02;
	ang_x = std::max<float>(-M_PI * 0.5, std::min<float>(M_PI * 0.5, ang_x));

	float x = (ks[SDL_SCANCODE_D]		- ks[SDL_SCANCODE_A]		) * 0.1;
	float z = (ks[SDL_SCANCODE_S]		- ks[SDL_SCANCODE_W]		) * 0.1;
	float y = (ks[SDL_SCANCODE_SPACE]	- ks[SDL_SCANCODE_LSHIFT]	) * 0.1;

	float cy = cosf(ang_y);
	float sy = sinf(ang_y);
	float cx = cosf(ang_x);
	float sx = sinf(ang_x);

	pos.y += y * cx + z * sx;
	pos.x += x * cy - z * sy * cx + sy * sx * y;
	pos.z += x * sy + z * cy * cx - cy * sx * y;


//	printf("%f %f %f %f %f\n", pos.x, pos.y, pos.z, ang_x, ang_y);
}


glm::mat4x4 Eye::get_view_mtx() const {
	return	glm::rotate<float>(ang_x, glm::vec3(1, 0, 0)) *
			glm::rotate<float>(ang_y, glm::vec3(0, 1, 0)) *
			glm::translate(-pos);
}


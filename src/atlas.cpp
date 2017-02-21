#include "atlas.h"
#include <assert.h>
#include <SDL2/SDL_image.h>
#include <glm/glm.hpp>


Atlas::~Atlas() {
	for (SDL_Surface* s : m_surfaces) SDL_FreeSurface(s);
}


void Atlas::init() {
	for (SDL_Surface* s : m_surfaces) SDL_FreeSurface(s);
	m_surfaces.clear();
}


void Atlas::add_surface() {
	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, SURFACE_SIZE, SURFACE_SIZE, 32, SDL_PIXELFORMAT_RGBA32);
	m_surfaces.push_back(s);
	for (int& i : m_columns) i = 0;
}


AtlasRegion Atlas::allocate_region(int w, int h) {
	assert(w <= SURFACE_SIZE || h <= SURFACE_SIZE);

	if (m_surfaces.empty()) add_surface();

	bool found = false;

	int xx;
	int yy = SURFACE_SIZE - h;

	for (int x = 0; x < SURFACE_SIZE - w;) {
		int y = 0;
		for (int i = 0; i < w; ++i) {
			y = std::max(y, m_columns[x + i]);
			if (y >= yy) break;
		}
		if (y < yy) {
			yy = y;
			xx = x;
			found = true;
		}
		++x;
		while (x < SURFACE_SIZE - w && m_columns[x - 1] == m_columns[x]) ++x;
	}

	assert(found);
	for (int x = 0; x < w; ++x) m_columns[xx + x] = yy + h;

	AtlasRegion r;
	r.surface_nr = m_surfaces.size() - 1;
	r.x = xx;
	r.y = yy;
	r.w = w;
	r.h = h;

	glm::u8vec4 color;
	color.r = rand() % 256;
	color.g = rand() % 256;
	color.b = rand() % 256;
	color.a = 255;
	SDL_Surface* s = m_surfaces.back();

	for (int x = r.x; x < r.x + r.w; ++x) {
		for (int y = r.y; y < r.y + r.h; ++y) {
			glm::u8vec4 * p = (glm::u8vec4 *) ((uint8_t * ) s->pixels + y * s->pitch + x * sizeof(glm::u8vec4));
			*p = color;
		}
	}


	return r;
}


void Atlas::save() {
	for (int i = 0; i < (int) m_surfaces.size(); ++i) {

		IMG_SavePNG(m_surfaces[i], "sm.png");
	}

}

#include "atlas.h"
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glm/glm.hpp>


Atlas::~Atlas() {
	for (SDL_Surface* s : m_surfaces) SDL_FreeSurface(s);
}


void Atlas::init() {
	for (SDL_Surface* s : m_surfaces) SDL_FreeSurface(s);
	m_surfaces.clear();
	m_surface_loaded = false;
}

bool Atlas::load_surface(const char* name) {
	SDL_Surface* s = IMG_Load(name);
	if (!s) return false;
	SDL_Surface* t = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGB24, 0);
	SDL_FreeSurface(s);
	m_surfaces.push_back(t);
	m_surface_loaded = true;
	return true;
}

void Atlas::add_surface() {
	SDL_Surface* s = SDL_CreateRGBSurface(0, SURFACE_SIZE, SURFACE_SIZE, 24, 0, 0, 0, 0);
	m_surfaces.push_back(s);
	for (int& i : m_columns) i = 0;
}


AtlasRegion Atlas::allocate_region(int w, int h) {
	assert(w <= SURFACE_SIZE || h <= SURFACE_SIZE);

	if (m_surfaces.empty()) add_surface();

	bool found = false;

	int xx = 0;
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


	if (!m_surface_loaded) {
		// fill region with random color
		glm::u8vec3 color;
		color.r = rand() % 256;
		color.g = rand() % 256;
		color.b = rand() % 256;
		SDL_Surface* s = m_surfaces.back();
		for (int x = r.x; x < r.x + r.w; ++x)
		for (int y = r.y; y < r.y + r.h; ++y) {
			glm::u8vec3& p = *(glm::u8vec3 *) ((uint8_t * ) s->pixels + y * s->pitch + x * sizeof(glm::u8vec3));
			if (x == r.x || x == r.x + r.w - 1 || y == r.y || y == r.y + r.h - 1) {
				p = color;
			}
			else if ((x ^ y) & 1) {
				p.r = color.r / 2;
				p.g = color.g / 2;
				p.b = color.b / 2;
			}
		}
	}

	return r;
}


void Atlas::save() {
	for (int i = 0; i < (int) m_surfaces.size(); ++i) {

		IMG_SavePNG(m_surfaces[i], "sm.png");
	}

}

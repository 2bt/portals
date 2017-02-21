#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <array>



struct AtlasRegion {
	int surface_nr;
	int x;
	int y;
	int w;
	int h;
};


class Atlas {
public:
	enum { SURFACE_SIZE = 2048 };

	~Atlas();
	void			init();
	AtlasRegion		allocate_region(int w, int h);
	void			save();

//private:

	std::vector<SDL_Surface*>			m_surfaces;
	std::array<int, SURFACE_SIZE>		m_columns;


	void add_surface();
};

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
	enum { SURFACE_SIZE = 512 };

	~Atlas();
	void			init();
	AtlasRegion		allocate_region(int w, int h);
	bool			load_surface(const char* name);
	void			save();

//private:

	std::vector<SDL_Surface*>			m_surfaces;
	std::array<int, SURFACE_SIZE>		m_columns;
	bool								m_surface_loaded = false;

	void add_surface();
};

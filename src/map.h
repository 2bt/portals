#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "atlas.h"


struct MapVertex {
	glm::vec3		pos;
	glm::vec2		uv;
	glm::vec2		uv2;
	MapVertex(const glm::vec3& pos, const glm::vec2& uv, const glm::vec2& uv2=glm::vec2(0, 0))
		: pos(pos), uv(uv), uv2(uv2)
	{}
};


struct MapFace {
	glm::mat4				mat;
	glm::mat4				inv_mat;
	int						tex_nr;
	AtlasRegion				shadow;
	std::vector<MapVertex>	verts;
};


struct Location {
	glm::vec3	pos;
	int			sector_nr;
};


struct WallRef {
	int sector_nr;
	int wall_nr;
	bool operator<(const WallRef& rhs) const {
		return sector_nr < rhs.sector_nr ||
			(sector_nr == rhs.sector_nr && wall_nr < rhs.wall_nr);
	}
	bool operator==(const WallRef& rhs) const {
		return sector_nr == rhs.sector_nr && wall_nr == rhs.wall_nr;
	}
};


struct Wall {
	glm::vec2 pos;
	std::vector<WallRef> refs;
};


struct Sector {
	std::vector<Wall>		walls;
	float					floor_height;
	float					ceil_height;
	std::vector<MapFace>	faces;
};


class Map {
public:
	Map();
	int		pick_sector(const glm::vec2& p) const;
	void	clip_move(Location& loc, const glm::vec3& mov) const;
	void	setup_portals();
	bool	load(const char* name);
	bool	save(const char* name) const;
	float	ray_intersect(	const Location& loc, const glm::vec3& dir,
							WallRef& ref, glm::vec3& normal,
							float max_factor=std::numeric_limits<float>::infinity()) const;

//private:

	std::vector<Sector>	sectors = {
		{	{
				{{ -10,	10	}},
				{{ 10,	10	}},
				{{ 10,	-10	}},
				{{ -10,	-10	}},
			},
			0, 10
		},
	};

	void	setup_sector_faces(Sector& s);
	void	bake();
	Atlas	shadow_atlas;

};


extern Map map;

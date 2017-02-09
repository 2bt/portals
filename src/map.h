#pragma once

#include <vector>
#include <glm/glm.hpp>


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
	WallRef	next;
};


struct Sector {
	std::vector<Wall> walls;
	float floor_height;
	float ceil_height;
};


struct Vall {
	glm::vec2 pos;
	std::vector<WallRef> refs;
};
struct Zector {
	std::vector<Vall> walls;
	float floor_height;
	float ceil_height;
};



class Map {
public:
	Map();
	int pick_sector(const glm::vec2& p) const;
	void clip_move(Location& loc, const glm::vec3& mov) const;
	void setup_portals();
	bool load(const char* name);
	bool save(const char* name) const;


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

	std::vector<Zector> zectors;

};


extern Map map;

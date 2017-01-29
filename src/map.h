#pragma once


#include <vector>
#include <glm/glm.hpp>


struct Wall {
	glm::vec2 pos;
	int other_point;
	int	next_wall;
	int next_sector;
};


struct Sector {
	int wall_index;
	int wall_count;
	float floor_height;
	float ceil_height;
};


struct Location {
	glm::vec3	pos;
	int			sector;
};


class Map {
public:
	Map();
	int find_sector(const glm::vec3& pos) const;
	void clip_move(Location& loc, const glm::vec3& mov) const;

//private:

	std::vector<Wall>	walls;
	std::vector<Sector>	sectors;

};


extern Map map;

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


struct Map {

	int find_sector(glm::vec3 pos) const;

	void clip_move(glm::vec3& pos, int& sector, glm::vec3 mov) const;


	Map() {
//		walls = {
//			{{-10, 10	}, 1, -1, -1 },
//			{{ 10, 5	}, 2, -1, -1 },
//			{{ 10,-10	}, 3, -1, -1 },
//			{{-10,-10	}, 0, -1, -1 },
//
//		};
//
//		sectors = {
//			{ 0, 4, 	-2, 3 },
//		};

		walls = {
			{{ 0,	10	}, 1, -1, -1 },
			{{ 10,	10	}, 2, -1, -1 },
			{{ 10,	0	}, 3, -1, -1 },
			{{ 5,	-2	}, 4, 5, 1 },
			{{ 0,	0	}, 0, -1, -1 },


			{{ 0,	0	}, 6, 3, 0 },
			{{ 5,	-2	}, 7, -1, -1 },
			{{ 5,	-10	}, 8, -1, -1 },
			{{ -5,	-10	}, 9, -1, -1 },
			{{ -5,	0	}, 5, -1, -1 },
		};

		sectors = {
			{ 0, 5, 	-2, 3 },
			{ 5, 5,     -1, 7 }
		};
	}

	std::vector<Wall>	walls;
	std::vector<Sector>	sectors;

};


extern Map map;

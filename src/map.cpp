#include "map.h"

#include <cstdio>
#include <glm/gtx/norm.hpp>

Map::Map() {

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


int Map::find_sector(const glm::vec3& pos) const {


	return 0;
}

void Map::clip_move(Location& loc, const glm::vec3& mov) const {

	float radius = 1.0;
	float floor_dist = 2.0;
	float ceil_dist = 0.5;


	loc.pos += mov;

	const Sector& s = map.sectors[loc.sector];
	glm::vec2 pos(loc.pos.x, loc.pos.z);


	if (loc.pos.y - floor_dist < s.floor_height)	loc.pos.y = s.floor_height + floor_dist;
	if (loc.pos.y + ceil_dist > s.ceil_height)		loc.pos.y = s.ceil_height - ceil_dist;


	for (int j = 0; j < s.wall_count; ++j) {
		const Wall& w1 = walls[s.wall_index + j];
		const Wall& w2 = walls[w1.other_point];

		glm::vec2 ww = w2.pos - w1.pos;
		glm::vec2 norm = glm::normalize(glm::vec2(-ww.y, ww.x));
		glm::vec2 pw = w1.pos - pos;
		float dst = glm::dot(norm, pw);
		if (dst < radius) {

			loc.pos.x -= norm.x * (radius - dst);
			loc.pos.z -= norm.y * (radius - dst);
		}

	}

}



Map map;

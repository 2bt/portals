#include "map.h"

// for debuging
#include "renderer2d.h"

#include <cstdio>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <queue>


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
	// TODO


	return 0;
}


void Map::clip_move(Location& loc, const glm::vec3& mov) const {

	float radius = 0.8;
	float floor_dist = 1.5;
	float ceil_dist = 0.5;


   // ignore height movement
	loc.pos.x += mov.x;
	loc.pos.z += mov.z;

	std::vector<int> visited;
	std::queue<int> todo({ loc.sector });
	glm::vec2 pos(loc.pos.x, loc.pos.z);
	while (!todo.empty()) {
		int nr = todo.front();
		todo.pop();
		visited.push_back(nr);
		const Sector& sector = map.sectors[nr];

		for (int j = 0; j < sector.wall_count; ++j) {
			const Wall& wall = walls[sector.wall_index + j];
			glm::vec2 ww = walls[wall.other_point].pos - wall.pos;
			glm::vec2 pw = pos - wall.pos;

			float u = glm::dot(pw, ww) / glm::length2(ww);
			u = std::max(0.0f, std::min(1.0f, u));
			glm::vec2 p = wall.pos + ww * u;
			glm::vec2 normal = pos - p;
			float dst = glm::length(normal);
			if (dst < radius) {

            // DEBUG
				renderer2D.line(p, pos);

				normal *= radius / dst - 1;

				if (wall.next_sector == -1) {
					// wall
					pos += normal;
				}
				else {
					// portal
					const Sector& s = sectors[wall.next_sector];

					if (loc.pos.y - floor_dist < s.floor_height
					||  loc.pos.y + ceil_dist > s.ceil_height) {
						pos += normal;
					}
					else {
						if (std::find(visited.begin(), visited.end(), wall.next_sector) == visited.end()) {
							todo.push(wall.next_sector);

                     // have we passed through the portal?
							float cross = glm::dot(glm::vec2(ww.y, -ww.x), pw);
							if (cross < 0) loc.sector = wall.next_sector;
						}
					}
				}
			}
		}
	}

   // handle height
   loc.pos.y += mov.y;
   for (int nr : visited) {
		const Sector& sector = map.sectors[nr];
		// clamp height
		if (loc.pos.y - floor_dist < sector.floor_height)	loc.pos.y = sector.floor_height + floor_dist;
		if (loc.pos.y + ceil_dist > sector.ceil_height)		loc.pos.y = sector.ceil_height - ceil_dist;
   }


	loc.pos.x = pos.x;
	loc.pos.z = pos.y;
}



Map map;

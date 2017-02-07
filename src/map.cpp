#include "map.h"
#include "eye.h"

#include <cstdio>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>

namespace std {
	template <>
	class hash<std::pair<glm::vec2, glm::vec2>> {
	public:
		size_t operator()(const std::pair<glm::vec2, glm::vec2>& edge) const {
			return (hash<glm::vec2>()(edge.first) << 7) ^ hash<glm::vec2>()(edge.second);
		}
	};
}


Map::Map() {

	sectors = {
		{	{
				{{ 0,	10	}},
				{{ 10,	10	}},
				{{ 10,	0	}},
				{{ 5,	-2	}},
				{{ 0,	0	}},
			},
			-2, 4
		},
		{	{
				{{ 0,	0	}},
				{{ 5,	-2	}},
				{{ 5,	-10	}},
				{{ -5,	-10	}},
				{{ -5,	0	}},
			},
			-1, 7
		},
	};


	setup_portals();

}


void Map::setup_portals() {
	std::unordered_map<std::pair<glm::vec2, glm::vec2>, WallRef> wall_map;
	for (int i = 0; i < (int) sectors.size(); ++i) {
		Sector& sector = sectors[i];

		for (int j = 0; j < (int) sector.walls.size(); ++j) {
			Wall& w1 = sector.walls[j];
			Wall& w2 = sector.walls[(j + 1) % sector.walls.size()];

			auto it = wall_map.find(std::make_pair(w2.pos, w1.pos));
			if (it == wall_map.end()) {
				wall_map[std::make_pair(w1.pos, w2.pos)] = { i, j };
				w1.next.sector_nr = -1;
			}
			else {
				// init portal
				WallRef& ref = it->second;
				w1.next.sector_nr = ref.sector_nr;
				w1.next.wall_nr = ref.wall_nr;
				Wall& next_wall = sectors[ref.sector_nr].walls[ref.wall_nr];
				next_wall.next.sector_nr = i;
				next_wall.next.wall_nr = j;
				wall_map.erase(it);
			}
		}
	}

	// XXX
	eye.loc.sector_nr = pick_sector(glm::vec2(eye.loc.pos.x, eye.loc.pos.z));
}


int Map::pick_sector(const glm::vec2& p) const {
	for (int i = 0; i < (int) sectors.size(); ++i) {
		const Sector& s = sectors[i];
		bool success = false;
		for (int j = 0; j < (int) s.walls.size(); ++j) {
			const glm::vec2& w1 = s.walls[j].pos;
			const glm::vec2& w2 = s.walls[(j + 1) % s.walls.size()].pos;
			glm::vec2 ww = w2 - w1;
			glm::vec2 pw = p - w1;
			if ((w1.y <= p.y) == (w2.y > p.y) && (pw.x < ww.x * pw.y / ww.y)) {
				success = !success;
			}
		}
		if (success) return i;
	}
	return -1;
}


void Map::clip_move(Location& loc, const glm::vec3& mov) const {

	float radius = 0.8;
	float floor_dist = 1.5;
	float ceil_dist = 0.5;


	// ignore height movement
	std::vector<int> visited;
	std::queue<int> todo({ loc.sector_nr });
	loc.pos.x += mov.x;
	loc.pos.z += mov.z;
	glm::vec2 pos(loc.pos.x, loc.pos.z);
	while (!todo.empty()) {
		int nr = todo.front();
		todo.pop();
		visited.push_back(nr);
		const Sector& sector = map.sectors[nr];

		for (int j = 0; j < (int) sector.walls.size(); ++j) {
			const Wall& wall = sector.walls[j];
			const Wall& wall2 = sector.walls[(j + 1) % sector.walls.size()];
			glm::vec2 ww = wall2.pos - wall.pos;
			glm::vec2 pw = pos - wall.pos;

			float u = glm::dot(pw, ww) / glm::length2(ww);
			u = std::max(0.0f, std::min(1.0f, u));
			glm::vec2 p = wall.pos + ww * u;
			glm::vec2 normal = pos - p;
			float dst = glm::length(normal);
			if (dst < radius) {

				normal *= radius / dst - 1;

				if (wall.next.sector_nr == -1) {
					// wall
					pos += normal;
				}
				else {
					// portal
					const Sector& s = sectors[wall.next.sector_nr];

					if (loc.pos.y - floor_dist < s.floor_height
					||  loc.pos.y + ceil_dist > s.ceil_height) {
						pos += normal;
					}
					else {
						if (std::find(visited.begin(), visited.end(), wall.next.sector_nr) == visited.end()) {
							todo.push(wall.next.sector_nr);

							// have we passed through the portal?
							float cross = glm::dot(glm::vec2(ww.y, -ww.x), pw);
							if (cross < 0) loc.sector_nr = wall.next.sector_nr;
						}
					}
				}
			}
		}
	}
	loc.pos.x = pos.x;
	loc.pos.z = pos.y;

	// handle height
	loc.pos.y += mov.y;
	for (int nr : visited) {
		const Sector& sector = map.sectors[nr];
		// clamp height
		if (loc.pos.y - floor_dist < sector.floor_height)	loc.pos.y = sector.floor_height + floor_dist;
		if (loc.pos.y + ceil_dist > sector.ceil_height)		loc.pos.y = sector.ceil_height - ceil_dist;
	}


}


Map map;

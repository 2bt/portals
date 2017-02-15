#include "map.h"
#include "eye.h"
#include "math.h"

#include <cstdio>
#include <algorithm>
#include <limits>
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
	setup_portals();
}


bool Map::load(const char* name) {
	FILE* f = fopen(name, "r");
	if (!f) return false;
	sectors.clear();
	char line[4096];
	while (fgets(line, sizeof(line), f)) {
		sectors.emplace_back();
		Sector& s = sectors.back();
		float x, y;
		int n;
		char* p = line;
		while (sscanf(p, "%f,%f%n", &x, &y, &n) == 2) {
			p += n;
			s.walls.push_back({ glm::vec2(x, y) });
		}
		fscanf(f, "%f,%f\n", &s.floor_height, &s.ceil_height);
	}
	fclose(f);
	setup_portals();

	return true;
}


bool Map::save(const char* name) const {
	FILE* f = fopen(name, "w");
	if (!f) return false;
	for (const Sector& s : sectors) {
		for (const Wall& w : s.walls) fprintf(f, " %.f,%.f", w.pos.x, w.pos.y);
		fprintf(f, "\n %.f,%.f\n", s.floor_height, s.ceil_height);
	}
	fclose(f);
	return true;
}


void Map::setup_portals() {
	for (Sector& sector : sectors) {
		for (Wall& wall : sector.walls) wall.refs.clear();

		for (int j = 0; j < (int) sector.walls.size();) {
			Wall& w1 = sector.walls[j];
			Wall& w2 = sector.walls[(j + 1) % sector.walls.size()];
			if (w1.pos == w2.pos) sector.walls.erase(sector.walls.begin() + j);
			else ++j;
		}
	}

	std::unordered_map<std::pair<glm::vec2, glm::vec2>, std::vector<WallRef>> wall_map;
	for (int i = 0; i < (int) sectors.size(); ++i) {
		Sector& sector = sectors[i];

		for (int j = 0; j < (int) sector.walls.size(); ++j) {
			Wall& w1 = sector.walls[j];
			Wall& w2 = sector.walls[(j + 1) % sector.walls.size()];
			wall_map[std::make_pair(w1.pos, w2.pos)].push_back({ i, j });

			auto it = wall_map.find(std::make_pair(w2.pos, w1.pos));
			if (it == wall_map.end()) continue;

			// init portal
			std::vector<WallRef>& refs = it->second;
			for (WallRef& ref : refs) {
				Sector& s = sectors[ref.sector_nr];
				if (s.floor_height >= sector.ceil_height || s.ceil_height <= sector.floor_height) continue;

				Wall& w = s.walls[ref.wall_nr];
				w.refs.push_back({ i, j });
				w1.refs.push_back(ref);

				// sort refs top to bottom
				auto cmp = [this](const WallRef& r1, const WallRef& r2){
					Sector s1 = sectors[r1.sector_nr];
					Sector s2 = sectors[r2.sector_nr];
					return s1.floor_height > s2.floor_height;
				};
				std::sort(w.refs.begin(), w.refs.end(), cmp);
				std::sort(w1.refs.begin(), w1.refs.end(), cmp);
			}

		}
	}
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
//	loc.pos += mov;

	float radius = 1.6;
	float floor_dist = 5;
	float ceil_dist = 1;

	glm::vec3 new_pos = loc.pos;

	// ignore height movement
	std::vector<int> visited;
	std::queue<int> todo({ loc.sector_nr });
	new_pos.x += mov.x;
	new_pos.z += mov.z;
	glm::vec2 pos(new_pos.x, new_pos.z);
	while (!todo.empty()) {
		int nr = todo.front();
		todo.pop();
		if (nr == -1) continue;
		visited.push_back(nr);
		const Sector& s = map.sectors[nr];

		for (int j = 0; j < (int) s.walls.size(); ++j) {
			const Wall& w = s.walls[j];
			const Wall& w2 = s.walls[(j + 1) % s.walls.size()];
			glm::vec2 ww = w2.pos - w.pos;
			glm::vec2 pw = pos - w.pos;

			float u = glm::dot(pw, ww) / glm::length2(ww);
			u = std::max(0.0f, std::min(1.0f, u));
			glm::vec2 p = w.pos + ww * u;
			glm::vec2 normal = pos - p;
			float dst = glm::length(normal);
			if (dst < radius) {


				bool pass = false;
				for (const WallRef& r : w.refs) {
					const Sector& s2 = sectors[r.sector_nr];
					if (new_pos.y + ceil_dist <= s2.ceil_height
					&&  new_pos.y - floor_dist >= s2.floor_height) {
						pass = true;

						if (std::find(visited.begin(), visited.end(), r.sector_nr) == visited.end()) {
							todo.push(r.sector_nr);
						}
						break;
					}
				}

				if (!pass) {
					normal *= radius / dst - 1;
					pos += normal;
				}
			}
		}
	}

	new_pos.x = pos.x;
	new_pos.z = pos.y;

	// handle height
	new_pos.y += mov.y;
	for (int nr : visited) {
		const Sector& sector = map.sectors[nr];
		// clamp height
		if (new_pos.y - floor_dist < sector.floor_height)	new_pos.y = sector.floor_height + floor_dist;
		if (new_pos.y + ceil_dist > sector.ceil_height)		new_pos.y = sector.ceil_height - ceil_dist;
	}


	WallRef ref;
	glm::vec3 normal;

	ray_intersect(loc, new_pos - loc.pos, ref, normal, 1);
	loc.sector_nr = ref.sector_nr;
	loc.pos = new_pos;

}


float Map::ray_intersect(	const Location& loc, const glm::vec3& dir,
							WallRef& ref, glm::vec3& normal, float max_factor) const {

	ref.sector_nr = loc.sector_nr;
	float min_factor = 0;
	glm::vec2 p(loc.pos.x, loc.pos.z);
	glm::vec2 d(dir.x, dir.z);

	for (;;) {
		const Sector& s = sectors[ref.sector_nr];
		float factor = max_factor;
		for (int i = 0; i < (int) s.walls.size(); ++i) {
			const Wall& w1 = s.walls[i];
			const Wall& w2 = s.walls[(i + 1) % s.walls.size()];
			glm::vec2 ww = w2.pos - w1.pos;
			glm::vec2 pw = p - w1.pos;
			float c = cross(ww, d);
			if (c <= 0) continue;
			float t = cross(pw, d) / c;
			float u = cross(pw, ww) / c;
			if (u > min_factor && u < factor && t >= 0 && t <= 1) {
				factor = u;
				ref.wall_nr = i;
				normal = glm::vec3(ww.y, 0, -ww.x);
			}
		}

		if (factor == max_factor) return max_factor;

		float y = loc.pos.y + dir.y * factor;

		// ceiling
		if (y > s.ceil_height) {
			factor *=  (s.ceil_height - loc.pos.y) / (y - loc.pos.y);
			normal = glm::vec3(0, -1, 0);
			ref.wall_nr = -1;
			return factor;
		}
		// floor
		if (y < s.floor_height) {
			factor *=  (s.floor_height - loc.pos.y) / (y - loc.pos.y);
			normal = glm::vec3(0, 1, 0);
			ref.wall_nr = -2;
			return factor;
		}

		const Wall& w = s.walls[ref.wall_nr];
		bool portal = false;
		for (const WallRef& r : w.refs) {
			const Sector& s2 = sectors[r.sector_nr];
			if (y < s2.ceil_height && y > s2.floor_height) {
				ref = r;
				portal = true;
				min_factor = factor;
				break;
			}
		}
		if (!portal) {
			normal = glm::normalize(normal);
			return factor;
		}
	}
	return 0;
}


Map map;

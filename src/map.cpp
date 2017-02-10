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
	}

	std::unordered_map<std::pair<glm::vec2, glm::vec2>, std::vector<WallRef>> wall_map;
	for (int i = 0; i < (int) sectors.size(); ++i) {
		Sector& sector = sectors[i];

		for (int j = 0; j < (int) sector.walls.size(); ++j) {
			Wall& w1 = sector.walls[j];
			Wall& w2 = sector.walls[(j + 1) % sector.walls.size()];

			auto it = wall_map.find(std::make_pair(w2.pos, w1.pos));
			if (it == wall_map.end()) {
				wall_map[std::make_pair(w1.pos, w2.pos)].push_back({ i, j });
				continue;
			}

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
	loc.pos += mov;
/*
	float radius = 1.6;
	float floor_dist = 5;
	float ceil_dist = 1;


	// ignore height movement
	std::vector<int> visited;
	std::queue<int> todo({ loc.sector_nr });
	loc.pos.x += mov.x;
	loc.pos.z += mov.z;
	glm::vec2 pos(loc.pos.x, loc.pos.z);
	while (!todo.empty()) {
		int nr = todo.front();
		todo.pop();
		if (nr == -1) continue;
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

*/
}


Map map;

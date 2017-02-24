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
	load("map.txt");

//	shadow_atlas.load_surface("sm.png");
	for (Sector& s : sectors) setup_sector_faces(s);
	bake();
	shadow_atlas.save();
}


static float rand_float() { return rand() / (float) RAND_MAX; }


void Map::bake() {
	SDL_Surface* surf = shadow_atlas.m_surfaces[0];

	for (int i = 0; i < (int) sectors.size(); ++i) {

		printf("baking sector %d\n", i);

		const Sector& s = sectors[i];

		Location loc;
		loc.sector_nr = i;

		for (const MapFace& f : s.faces)
		for (int y = f.shadow.y; y < f.shadow.y + f.shadow.h; ++y)
		for (int x = f.shadow.x; x < f.shadow.x + f.shadow.w; ++x) {

			loc.pos = glm::vec3(f.mat * glm::vec4(x, y, 02, 1));
			srand(loc.pos.x * 100 + loc.pos.y * 171 + loc.pos.z * 31);

			float a = 0;
			int N = 2000;
			for (int k = 0; k < N; ++k) {
				glm::vec3 normal;
				WallRef ref;
				glm::vec3 dir;
				for (int j = 0; j < 5; ++j) {
					dir.x = 2 * rand_float() - 1;
					dir.y = 2 * rand_float() - 1;
					dir.z = rand_float();
					if (glm::length2(dir) <= 1) break;
				}

				dir = glm::vec3(f.mat * glm::vec4(dir, 0));

				auto l = loc;
				l.pos += dir * -0.1f;
				float f = ray_intersect(l, dir, ref, normal, 30);
//				float f = ray_intersect(loc, dir, ref, normal, 30);
				a += f / 30 / N;
			}

			glm::u8vec3& pixel = *(glm::u8vec3 *) ((uint8_t * ) surf->pixels + y * surf->pitch + x * sizeof(glm::u8vec3));
			pixel.r = pixel.g = pixel.b = 255 * powf(a, 0.8);


		}
	}
}


#define SHADOW_DETAIL 0.25f


void Map::setup_sector_faces(Sector& s) {

	// walls

	s.faces.clear();
	auto generate_wall_face = [this, &s](const glm::vec2& p1, float y1, const glm::vec2& p2, float y2) {
		float u1, u2;
		if (abs(p2.x - p1.x) > abs(p2.y - p1.y)) {
			u1 = p1.x;
			u2 = p2.x;
		}
		else {
			u1 = p1.y;
			u2 = p2.y;
		}

		s.faces.emplace_back();
		MapFace& face = s.faces.back();

		face.tex_nr = 0;
		glm::vec2 pp = p2 - p1;
		float len = glm::length(pp);
		int w = std::ceil(len * SHADOW_DETAIL) + 1;
		int h = std::ceil((y1 - y2) * SHADOW_DETAIL) + 1;
		face.shadow = shadow_atlas.allocate_region(w, h);

		pp /= len * SHADOW_DETAIL;
		face.mat[0] = glm::vec4(pp.x, 0, pp.y, 0);
		face.mat[1] = glm::vec4(0, -1.0f / SHADOW_DETAIL, 0, 0);
		face.mat[2] = glm::vec4(pp.y, 0, -pp.x, 0);
		face.mat[3] = glm::vec4(
				p1.x - face.shadow.x * pp.x,
				y1 + face.shadow.y / SHADOW_DETAIL,
				p1.y - face.shadow.x * pp.y,
				1);
		face.inv_mat = glm::inverse(face.mat);


		glm::vec3 v[4] = {
			glm::vec3(p1.x, y1, p1.y),
			glm::vec3(p1.x, y2, p1.y),
			glm::vec3(p2.x, y1, p2.y),
			glm::vec3(p2.x, y2, p2.y),
		};
		glm::vec2 q[4];
		for (int i = 0; i < 4; ++i) {
			glm::vec2 t = glm::vec2(face.inv_mat * glm::vec4(v[i], 1));
			q[i] = (t + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
		}

		face.verts.emplace_back(v[0], glm::vec2(u1, y1), q[0]);
		face.verts.emplace_back(v[3], glm::vec2(u2, y2), q[3]);
		face.verts.emplace_back(v[1], glm::vec2(u1, y2), q[1]);
		face.verts.emplace_back(v[0], glm::vec2(u1, y1), q[0]);
		face.verts.emplace_back(v[2], glm::vec2(u2, y1), q[2]);
		face.verts.emplace_back(v[3], glm::vec2(u2, y2), q[3]);

	};
	for (int j = 0; j < (int) s.walls.size(); ++j) {
		auto& w = s.walls[j];
		auto& p1 = w.pos;
		auto& p2 = s.walls[(j + 1) % s.walls.size()].pos;

		float h = s.ceil_height;
		for (const WallRef& ref : w.refs) {
			const Sector& s2 = map.sectors[ref.sector_nr];
			if (s2.ceil_height < h) {
				generate_wall_face(p1, h, p2, s2.ceil_height);
			}
			h = s2.floor_height;
		}
		if (h > s.floor_height) {
			generate_wall_face(p1, h, p2, s.floor_height);
		}
	}


	// floor and ceiling
	std::vector<glm::vec2> poly;
	glm::vec2 max;
	glm::vec2 min;
	for (int j = 0; j < (int) s.walls.size(); ++j) {
		const Wall& w = s.walls[j];
		poly.emplace_back(w.pos);
		if (j == 0) max = min = w.pos;
		else {
			max.x = std::max(max.x, w.pos.x);
			max.y = std::max(max.y, w.pos.y);
			min.x = std::min(min.x, w.pos.x);
			min.y = std::min(min.y, w.pos.y);
		}
	}
	min = glm::floor(min * SHADOW_DETAIL);
	max = glm::ceil(max * SHADOW_DETAIL);
	glm::ivec2 size = max - min + glm::vec2(1);
	s.faces.resize(s.faces.size() + 2);
	MapFace& floor_face = s.faces[s.faces.size() - 2];
	MapFace& ceil_face = s.faces[s.faces.size() - 1];
	floor_face.tex_nr = 1;
	ceil_face.tex_nr = 2;
	floor_face.shadow = shadow_atlas.allocate_region(size.x, size.y);
	ceil_face.shadow = shadow_atlas.allocate_region(size.x, size.y);

	floor_face.mat = {
		1.0f / SHADOW_DETAIL, 0, 0, 0,
		0, 0, 1.0f / SHADOW_DETAIL, 0,
		0, 1.0f / SHADOW_DETAIL, 0, 0,
		(min.x - floor_face.shadow.x) / SHADOW_DETAIL, s.floor_height,
		(min.y - floor_face.shadow.y) / SHADOW_DETAIL, 1
	};

	ceil_face.mat = {
		1.0f / SHADOW_DETAIL, 0, 0, 0,
		0, 0, 1.0f / SHADOW_DETAIL, 0,
		0, -1.0f / SHADOW_DETAIL, 0, 0,
		(min.x - ceil_face.shadow.x) / SHADOW_DETAIL, s.ceil_height,
		(min.y - ceil_face.shadow.y) / SHADOW_DETAIL, 1
	};

	floor_face.inv_mat = glm::inverse(floor_face.mat);
	ceil_face.inv_mat = glm::inverse(ceil_face.mat);

	triangulate(poly, [&s, &floor_face, &ceil_face]
	(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
	{
		{
			glm::vec2 t1 = glm::vec2(floor_face.inv_mat * glm::vec4(p1.x, 0, p1.y, 1));
			glm::vec2 t2 = glm::vec2(floor_face.inv_mat * glm::vec4(p2.x, 0, p2.y, 1));
			glm::vec2 t3 = glm::vec2(floor_face.inv_mat * glm::vec4(p3.x, 0, p3.y, 1));
			glm::vec2 q1 = (t1 + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
			glm::vec2 q2 = (t2 + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
			glm::vec2 q3 = (t3 + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
			floor_face.verts.emplace_back(glm::vec3(p1.x, s.floor_height, p1.y), p1, q1);
			floor_face.verts.emplace_back(glm::vec3(p2.x, s.floor_height, p2.y), p2, q2);
			floor_face.verts.emplace_back(glm::vec3(p3.x, s.floor_height, p3.y), p3, q3);
		}

		{
			glm::vec2 t1 = glm::vec2(ceil_face.inv_mat * glm::vec4(p1.x, 0, p1.y, 1));
			glm::vec2 t2 = glm::vec2(ceil_face.inv_mat * glm::vec4(p2.x, 0, p2.y, 1));
			glm::vec2 t3 = glm::vec2(ceil_face.inv_mat * glm::vec4(p3.x, 0, p3.y, 1));
			glm::vec2 q1 = (t1 + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
			glm::vec2 q2 = (t2 + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
			glm::vec2 q3 = (t3 + glm::vec2(0.5)) / (float) Atlas::SURFACE_SIZE;
			ceil_face.verts.emplace_back(glm::vec3(p1.x, s.ceil_height, p1.y), p1, q1);
			ceil_face.verts.emplace_back(glm::vec3(p3.x, s.ceil_height, p3.y), p3, q3);
			ceil_face.verts.emplace_back(glm::vec3(p2.x, s.ceil_height, p2.y), p2, q2);
		}

	});

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

	// TODO: have this be input
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

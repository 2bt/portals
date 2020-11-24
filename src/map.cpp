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
    load("media/map.txt");

    bool loaded = shadow_atlas.load_surface("sm.png");
    for (Sector& s : sectors) setup_sector_faces(s);
    if (!loaded) {
        bake();
        shadow_atlas.save();
    }
}


static float rand_float() { return rand() / (float) RAND_MAX; }



bool Map::fix_sector(Location& loc) const {

    glm::vec2 p(loc.pos.x, loc.pos.z);

    std::vector<int> visited;
    std::queue<int> todo({ loc.sector_nr });
    while (!todo.empty() && visited.size() < 15) {
        int nr = todo.front();
        todo.pop();
        visited.push_back(nr);

        const Sector& s = sectors[nr];

        if (loc.pos.y > s.ceil_height || loc.pos.y < s.floor_height) {
            continue;
        }

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

        if (success) {
            loc.sector_nr = nr;
            return true;
        }

        for (int j = 0; j < (int) s.walls.size(); ++j) {
            const Wall& w = s.walls[j];
            glm::vec2 ww = s.walls[(j + 1) % s.walls.size()].pos - w.pos;
            if (cross(w.pos - p, ww) > 0)
            for (const WallRef& r : w.refs) {
                if (std::find(visited.begin(), visited.end(), r.sector_nr) == visited.end()) {
                    todo.push(r.sector_nr);
                }
            }
        }
    }
    return false;
}


void Map::bake() {
    printf("baking shadow maps (this may take a minute)...\n");

    SDL_Surface* surf = shadow_atlas.m_surfaces[0];


    for (int i = 0; i < (int) sectors.size(); ++i) {

        printf("baking sector %d\n", i);

        const Sector& s = sectors[i];

        Location loc;

        for (const MapFace& f : s.faces) {

            auto pix = [
                data = (uint8_t *) surf->pixels,
                 xx = f.shadow.x,
                 yy = f.shadow.y,
                 pitch = surf->pitch
            ](int x, int y) -> glm::u8vec3& {
                return *(glm::u8vec3 *) (data + (y + yy) * pitch + (x + xx) * sizeof(glm::u8vec3));
            };

            for (int y = 0; y < f.shadow.h; ++y)
            for (int x = 0; x < f.shadow.w; ++x) {

                glm::u8vec3& pixel = pix(x, y);

                loc.sector_nr = i;
                loc.pos = glm::vec3(f.mat * glm::vec4(x + 0.01, y + 0.01, 0, 1)) + f.normal * 0.01f;
                if (!fix_sector(loc)) {
                    pixel = { 255, 0, 0 };
                    continue;
                }


                float a = 0;
                int N = 10000;
                for (int k = 0; k < N; ++k) {
                    glm::vec3 normal;
                    WallRef ref;
                    glm::vec3 dir;
                    for (int j = 0; j < 5; ++j) {
                        dir.x = 2 * rand_float() - 1;
                        dir.y = 2 * rand_float() - 1;
                        dir.z = 2 * rand_float() - 1;
                        if (glm::length2(dir) <= 1) break;
                    }

                    if (glm::dot(dir, f.normal) < 0) dir = -dir;

                    float f = ray_intersect(loc, dir, ref, normal, 60);
                    a += f / 60 / N;
                }

                pixel.r = pixel.g = pixel.b = 255 * powf(a, 1.3);

            }


            for (int y = 0; y < f.shadow.h; ++y)
            for (int x = 0; x < f.shadow.w; ++x) {
                glm::u8vec3& p = pix(x, y);
                if (p == glm::u8vec3(255, 0, 0)) {
                    p = { 255, 255, 255 };

                    if (x > 0 && pix(x - 1, y) != glm::u8vec3(255, 0, 0)) p = glm::min(p, pix(x - 1, y));
                    if (x < f.shadow.w - 1 && pix(x + 1, y) != glm::u8vec3(255, 0, 0)) p = glm::min(p, pix(x + 1, y));
                    if (y > 0 && pix(x, y - 1) != glm::u8vec3(255, 0, 0)) p = glm::min(p, pix(x, y - 1));
                    if (y < f.shadow.h - 1 && pix(x, y + 1) != glm::u8vec3(255, 0, 0)) p = glm::min(p, pix(x, y + 1));


                }
            }


        }
    }
}


#define SHADOW_DETAIL 0.5f


void Map::setup_sector_faces(Sector& s) {

    // walls
    // TODO: fix T junctions
    s.faces.clear();
    auto generate_wall_face = [this, &s](const glm::vec2& p1, float y1, const glm::vec2& p2, float y2) {
        float u1, u2;
        glm::vec2 pp = glm::normalize(p2 - p1);
        if (abs(pp.x) > abs(pp.y)) {
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
        face.normal = glm::vec3(pp.y, 0, -pp.x);

        glm::vec3 d[4] = {
            glm::vec3(p1.x, y1, p1.y),
            glm::vec3(p1.x, y2, p1.y),
            glm::vec3(p2.x, y1, p2.y),
            glm::vec3(p2.x, y2, p2.y),
        };
        face.verts.emplace_back(d[0], glm::vec2(u1, y1));
        face.verts.emplace_back(d[3], glm::vec2(u2, y2));
        face.verts.emplace_back(d[1], glm::vec2(u1, y2));
        face.verts.emplace_back(d[0], glm::vec2(u1, y1));
        face.verts.emplace_back(d[2], glm::vec2(u2, y1));
        face.verts.emplace_back(d[3], glm::vec2(u2, y2));

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
    for (int j = 0; j < (int) s.walls.size(); ++j) {
        const glm::vec2& p = s.walls[j].pos;
        poly.emplace_back(p);
    }

    s.faces.resize(s.faces.size() + 2);
    MapFace& floor_face = s.faces[s.faces.size() - 2];
    MapFace& ceil_face = s.faces[s.faces.size() - 1];

    floor_face.tex_nr = 1;
    floor_face.normal = glm::vec3(0, 1, 0);

    ceil_face.tex_nr = 2;
    ceil_face.normal = glm::vec3(0, -1, 0);

    triangulate(poly, [&s, &floor_face, &ceil_face]
    (const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
    {
        floor_face.verts.emplace_back(glm::vec3(p1.x, s.floor_height, p1.y), p1);
        floor_face.verts.emplace_back(glm::vec3(p2.x, s.floor_height, p2.y), p2);
        floor_face.verts.emplace_back(glm::vec3(p3.x, s.floor_height, p3.y), p3);
        ceil_face.verts.emplace_back(glm::vec3(p1.x, s.ceil_height, p1.y), p1);
        ceil_face.verts.emplace_back(glm::vec3(p3.x, s.ceil_height, p3.y), p3);
        ceil_face.verts.emplace_back(glm::vec3(p2.x, s.ceil_height, p2.y), p2);
    });


    // set shadow texture coordinates
    for (MapFace& f : s.faces) {
        glm::vec3 min = f.verts.front().pos;
        glm::vec3 max = f.verts.front().pos;
        for (const MapVertex& v : f.verts) {
            min = glm::min(min, v.pos);
            max = glm::max(max, v.pos);
        }
        min = glm::floor(min * SHADOW_DETAIL);
        max = glm::ceil(max * SHADOW_DETAIL);
        glm::ivec3 size = max - min + glm::vec3(1);
        min /= SHADOW_DETAIL;
        max /= SHADOW_DETAIL;


        auto nn = f.normal;
        auto n = f.normal / SHADOW_DETAIL;
        auto abs = glm::abs(n);
        float o = 1 / SHADOW_DETAIL;

        auto pp = f.verts.front().pos - min;
        auto t = glm::dot(pp, nn);

        if (abs.x > abs.y && abs.x > abs.z) {
            f.shadow = shadow_atlas.allocate_region(size.y, size.z);
            f.mat[0] = glm::vec4(-n.y / nn.x, o, 0, 0);
            f.mat[1] = glm::vec4(-n.z / nn.x, 0, o, 0);
            f.mat[2] = glm::vec4(f.normal, 0);
            f.mat[3] = glm::vec4(min.x + t / nn.x, min.y, min.z, 1);
        }
        else if (abs.y > abs.x && abs.y > abs.z) {
            f.shadow = shadow_atlas.allocate_region(size.x, size.z);
            f.mat[0] = glm::vec4(o, -n.x / nn.y, 0, 0);
            f.mat[1] = glm::vec4(0, -n.z / nn.y, o, 0);
            f.mat[2] = glm::vec4(f.normal, 0);
            f.mat[3] = glm::vec4(min.x, min.y + t / nn.y, min.z, 1);
        }
        else {
            f.shadow = shadow_atlas.allocate_region(size.x, size.y);
            f.mat[0] = glm::vec4(o, 0, -n.x / nn.z, 0);
            f.mat[1] = glm::vec4(0, o, -n.y / nn.z, 0);
            f.mat[2] = glm::vec4(f.normal, 0);
            f.mat[3] = glm::vec4(min.x, min.y, min.z + t / nn.z, 1);
        }


        f.inv_mat = glm::inverse(f.mat);

        for (MapVertex& v : f.verts) {

            if (abs.x > abs.y && abs.x > abs.z) {
                v.uv2.x = v.pos.y - min.y;
                v.uv2.y = v.pos.z - min.z;
            }
            else if (abs.y > abs.x && abs.y > abs.z) {
                v.uv2.x = v.pos.x - min.x;
                v.uv2.y = v.pos.z - min.z;
            }
            else {
                v.uv2.x = v.pos.x - min.x;
                v.uv2.y = v.pos.y - min.y;
            }
            v.uv2 *= SHADOW_DETAIL;
            v.uv2 += glm::vec2(f.shadow.x, f.shadow.y) + glm::vec2(0.5);
            v.uv2 /= Atlas::SURFACE_SIZE;
        }

    }



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
        assert(fscanf(f, "%f,%f\n", &s.floor_height, &s.ceil_height) == 2);
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
        if (new_pos.y - floor_dist < sector.floor_height)    new_pos.y = sector.floor_height + floor_dist;
        if (new_pos.y + ceil_dist > sector.ceil_height)        new_pos.y = sector.ceil_height - ceil_dist;
    }


    WallRef ref;
    glm::vec3 normal;

    ray_intersect(loc, new_pos - loc.pos, ref, normal, 1);
    loc.sector_nr = ref.sector_nr;
    loc.pos = new_pos;

}


float Map::ray_intersect(const Location& loc, const glm::vec3& dir,
                         WallRef& ref, glm::vec3& normal, float max_factor) const
{

    ref.sector_nr = loc.sector_nr;
    ref.wall_nr = 0;
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

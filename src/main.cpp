#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "renderer2d.h"
#include "renderer3d.h"

#include "rmw.h"
#include "math.h"
#include "map.h"
#include "eye.h"
#include "editor.h"


Renderer2D	renderer2D;
Renderer3D	renderer3D;


Eye		eye;
Editor	editor;


#include <limits>

float ray_intersect(const Location& loc, const glm::vec3& dir, WallRef& ref, glm::vec3& normal) {

	ref.sector_nr = loc.sector_nr;
	float factor = std::numeric_limits<float>::infinity();

	Sector& s = map.sectors[ref.sector_nr];
	glm::vec2 p(loc.pos.x, loc.pos.z);
	glm::vec2 d(dir.x, dir.z);
	for (int i = 0; i < (int) s.walls.size(); ++i) {
		const Wall& w1 = s.walls[i];
		const Wall& w2 = s.walls[(i + 1) % s.walls.size()];
		glm::vec2 ww = w2.pos - w1.pos;
		glm::vec2 pw = p - w1.pos;
		float c = cross(ww, d);
		if (c <= 0) continue;
		float t = cross(pw, d) / c;
		float u = cross(pw, ww) / c;
		if (u > 0 && u < factor && t >= 0 && t <= 1) {
			factor = u;
			ref.wall_nr = i;
			normal = glm::vec3(ww.y, 0, -ww.x);
		}
	}

	float y = loc.pos.y + dir.y * factor;

	// check ceiling
	if (y > s.ceil_height) {
		factor *=  (s.ceil_height - loc.pos.y) / (y - loc.pos.y);
		normal = glm::vec3(0, -1, 0);
		ref.wall_nr = -1;
		return factor;
	}
	else if (y < s.floor_height) {
		factor *=  (s.floor_height - loc.pos.y) / (y - loc.pos.y);
		normal = glm::vec3(0, 1, 0);
		ref.wall_nr = -2;
		return factor;
	}
	else {
		const Wall& w = s.walls[ref.wall_nr];
		if (w.refs.empty()) {
			normal = glm::normalize(normal);
			return factor;
		}

		// TODO: check w.refs


	}

	printf("portal\n");
	return factor;
}




class MapRenderer {
public:

	struct Vert {
		glm::vec3		pos;
		glm::vec2		uv;
		glm::u8vec4		col;

		Vert(	const glm::vec3& pos,
				const glm::vec2& uv,
				const glm::u8vec4& col = glm::u8vec4(255, 255, 255, 255))
			: pos(pos), uv(uv), col(col)
		{}
	};


	void init() {

		shader = rmw::context.create_shader(
			R"(#version 330
				layout(location = 0) in vec3 in_pos;
				layout(location = 1) in vec2 in_uv;
				layout(location = 2) in vec4 in_color;
				uniform mat4 mvp;
				out vec2 ex_uv;
				out vec4 ex_color;
				out float ex_depth;
				void main() {
					gl_Position = mvp * vec4(in_pos, 1.0);
					ex_uv = in_uv;
					ex_color = in_color;
					ex_depth = gl_Position.z;
				})",
			R"(#version 330
				in vec2 ex_uv;
				in vec4 ex_color;
				in float ex_depth;
				uniform sampler2D tex;
				out vec4 out_color;
				void main() {
					vec4 c = texture(tex, ex_uv) * ex_color;
					out_color = vec4(c.rgb * pow(0.98, ex_depth), c.a);
				})");

		vertex_buffer = rmw::context.create_vertex_buffer(rmw::BufferHint::StreamDraw);
		vertex_array = rmw::context.create_vertex_array();
		vertex_array->set_primitive_type(rmw::PrimitiveType::Triangles);
		vertex_array->set_attribute(0, vertex_buffer, rmw::ComponentType::Float, 3, false, 0, sizeof(Vert));
		vertex_array->set_attribute(1, vertex_buffer, rmw::ComponentType::Float, 2, false, 12, sizeof(Vert));
		vertex_array->set_attribute(2, vertex_buffer, rmw::ComponentType::Uint8, 4, true, 20, sizeof(Vert));

		tex_wall  = rmw::context.create_texture_2D("media/wall.png");
		tex_floor = rmw::context.create_texture_2D("media/floor.png");
		tex_ceil  = rmw::context.create_texture_2D("media/ceil.png");
	}

	void draw() {

		wall_verts.clear();
		floor_verts.clear();
		ceil_verts.clear();

		for (int i = 0; i < (int) map.sectors.size(); ++i) {
			Sector& sector = map.sectors[i];

			// walls
			for (int j = 0; j < (int) sector.walls.size(); ++j) {
				auto& w1 = sector.walls[j];
				auto& w2 = sector.walls[(j + 1) % sector.walls.size()];
				auto& p1 = w1.pos;
				auto& p2 = w2.pos;

				float h = sector.ceil_height;
				for (const WallRef& ref : w1.refs) {
					const Sector& s = map.sectors[ref.sector_nr];
					if (s.ceil_height < h) {
						generate_wall(p1, h, p2, s.ceil_height);
					}
					h = s.floor_height;
				}
				if (h > sector.floor_height) {
					generate_wall(p1, h, p2, sector.floor_height);
				}
			}


			// trangulate floor and ceiling
			std::vector<glm::vec2> poly;
			for (const Wall& w : sector.walls) poly.emplace_back(w.pos);
			triangulate(poly, [this, &sector](
				const glm::vec2& p1,
				const glm::vec2& p2,
				const glm::vec2& p3)
			{
				floor_verts.emplace_back(glm::vec3(p1.x, sector.floor_height, p1.y), p1);
				floor_verts.emplace_back(glm::vec3(p2.x, sector.floor_height, p2.y), p2);
				floor_verts.emplace_back(glm::vec3(p3.x, sector.floor_height, p3.y), p3);
				ceil_verts.emplace_back(glm::vec3(p1.x, sector.ceil_height, p1.y), p1);
				ceil_verts.emplace_back(glm::vec3(p2.x, sector.ceil_height, p2.y), p2);
				ceil_verts.emplace_back(glm::vec3(p3.x, sector.ceil_height, p3.y), p3);
			});
		}


		glm::mat4 mat_perspective = glm::perspective(
			glm::radians(60.0f),
			rmw::context.get_aspect_ratio(),
			0.1f, 500.0f);

		glm::mat4 mat_view = eye.get_view_mtx();
		shader->set_uniform("mvp", mat_perspective * mat_view);


		rmw::RenderState rs;
		rs.depth_test_enabled = true;


		vertex_buffer->init_data(wall_verts);
		vertex_array->set_count(wall_verts.size());
		shader->set_uniform("tex", tex_wall);
		rmw::context.draw(rs, shader, vertex_array);

		vertex_buffer->init_data(ceil_verts);
		vertex_array->set_count(ceil_verts.size());
		shader->set_uniform("tex", tex_ceil);
		rmw::context.draw(rs, shader, vertex_array);

		vertex_buffer->init_data(floor_verts);
		vertex_array->set_count(floor_verts.size());
		shader->set_uniform("tex", tex_floor);
		rmw::context.draw(rs, shader, vertex_array);


		//if (0)
		{
			// XXX:
			renderer3D.set_transformation(mat_perspective * mat_view);
			renderer3D.set_line_width(3);
			renderer3D.set_point_size(10);


			static glm::vec3 orig;
			static glm::vec3 dir;
			static glm::vec3 mark;
			static glm::vec3 mark_normal;

			int x, y;
			int b = SDL_GetMouseState(&x, &y);
			if (b) {
				orig = eye.get_location().pos;
				glm::vec4 c = glm::vec4(x / (float) rmw::context.get_width() * 2 - 1,
										y / (float) rmw::context.get_height() * -2 + 1, -1, 1);
				glm::vec4 v = glm::inverse(mat_perspective * mat_view) * c;
				dir = glm::normalize(glm::vec3(v) / v.w - orig);

				WallRef ref;
				float f = ray_intersect(eye.get_location(), dir, ref, mark_normal);
				mark = eye.get_location().pos + dir * f;
			}



			renderer3D.set_color(0, 255, 0);
			renderer3D.point(mark);
			renderer3D.set_color(0, 100, 0);
			renderer3D.line(mark, mark + mark_normal * 3.0f);

			renderer3D.set_color(255, 0, 255);
			renderer3D.point(orig);
			renderer3D.set_color(0, 0, 255);
			renderer3D.line(orig, orig + dir * 50.0f);

			// wire frame
			renderer3D.set_color(255, 0, 0);
/*
			for (int i = 0; i < (int) map.sectors.size(); ++i) {
				Sector& sector = map.sectors[i];
				for (int j = 0; j < (int) sector.walls.size(); ++j) {
					auto& w1 = sector.walls[j];
					auto& w2 = sector.walls[(j + 1) % sector.walls.size()];
					auto& p1 = w1.pos;
					auto& p2 = w2.pos;

					float h = sector.ceil_height;
					for (const WallRef& ref : w1.refs) {
						const Sector& s = map.sectors[ref.sector_nr];
						if (s.ceil_height < h) {
							renderer3D.line(
								glm::vec3(p1.x, h, p1.y),
								glm::vec3(p2.x, h, p2.y));
							renderer3D.line(
								glm::vec3(p1.x, h, p1.y),
								glm::vec3(p1.x, s.ceil_height, p1.y));
							renderer3D.line(
								glm::vec3(p1.x, s.ceil_height, p1.y),
								glm::vec3(p2.x, s.ceil_height, p2.y));
						}
						h = s.floor_height;
					}
					if (h > sector.floor_height) {
						renderer3D.line(
							glm::vec3(p1.x, h, p1.y),
							glm::vec3(p2.x, h, p2.y));
						renderer3D.line(
							glm::vec3(p1.x, h, p1.y),
							glm::vec3(p1.x, sector.floor_height, p1.y));
						renderer3D.line(
							glm::vec3(p1.x, sector.floor_height, p1.y),
							glm::vec3(p2.x, sector.floor_height, p2.y));
					}
				}
			}
*/
			renderer3D.flush();
		}

	}



private:

	void generate_wall(const glm::vec2& p1, float h1, const glm::vec2& p2, float h2) {
		float u1, u2;
		if (abs(p2.x - p1.x) > abs(p2.y - p1.y)) {
			u1 = p1.x;
			u2 = p2.x;
		}
		else {
			u1 = p1.y;
			u2 = p2.y;
		}
		wall_verts.emplace_back(glm::vec3(p1.x, h1, p1.y), glm::vec2(u1, h1));
		wall_verts.emplace_back(glm::vec3(p1.x, h2, p1.y), glm::vec2(u1, h2));
		wall_verts.emplace_back(glm::vec3(p2.x, h2, p2.y), glm::vec2(u2, h2));
		wall_verts.emplace_back(glm::vec3(p1.x, h1, p1.y), glm::vec2(u1, h1));
		wall_verts.emplace_back(glm::vec3(p2.x, h2, p2.y), glm::vec2(u2, h2));
		wall_verts.emplace_back(glm::vec3(p2.x, h1, p2.y), glm::vec2(u2, h1));
	}

	std::vector<Vert> wall_verts;
	std::vector<Vert> ceil_verts;
	std::vector<Vert> floor_verts;


	rmw::Shader::Ptr		shader;
	rmw::VertexBuffer::Ptr	vertex_buffer;
	rmw::VertexArray::Ptr	vertex_array;

	rmw::Texture2D::Ptr		tex_wall;
	rmw::Texture2D::Ptr		tex_ceil;
	rmw::Texture2D::Ptr		tex_floor;

} renderer;



int main(int argc, char** argv) {
	rmw::context.init(800, 600, "portal");

	map.load("map.txt");

	renderer2D.init();
	renderer3D.init();


	renderer.init();


	bool running = true;
	while (running) {
		SDL_Event e;
		while (rmw::context.poll_event(e)) {
			switch (e.type) {

			case SDL_QUIT:
				running = false;
				break;

			case SDL_KEYDOWN:
				if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = false;
				editor.keyboard(e.key);
				break;
			case SDL_KEYUP:
				editor.keyboard(e.key);
				break;

			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				editor.mouse_button(e.button);
				break;

			case SDL_MOUSEWHEEL:
				editor.mouse_wheel(e.wheel);
				break;

			default: break;
			}
		}



		rmw::context.clear(rmw::ClearState { { 0, 0, 0, 1 } });

		// move player
		eye.update();

		renderer.draw();

		editor.draw();

		rmw::context.flip_buffers();
	}

}

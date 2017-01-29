#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include "rmw.h"
#include "map.h"
#include "eye.h"
#include "editor.h"


Eye		eye;
Editor	editor;

Renderer2D	renderer2D;


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
					out_color = vec4(c.rgb * pow(0.9, ex_depth), c.a);
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


			glm::u8vec4 col(255, 255, 255, 255);
			if (i == eye.get_location().sector) {
				col.g = 100;
				col.b = 100;
			}

			// walls
			for (int j = 0; j < sector.wall_count; ++j) {

				auto& w1 = map.walls[sector.wall_index + j];
				auto& w2 = map.walls[w1.other_point];
				auto p1 = w1.pos;
				auto p2 = w2.pos;


				// full wall
				if (w1.next_sector == -1) {
					wall(p1.x, sector.floor_height, p1.y, p2.x, sector.ceil_height, p2.y);
					continue;
				}

				auto& s2 = map.sectors[w1.next_sector];

				// floor wall
				if (sector.floor_height < s2.floor_height) {
					wall(p1.x, sector.floor_height, p1.y, p2.x, s2.floor_height, p2.y);
				}

				// ceil wall
				if (sector.ceil_height > s2.ceil_height) {
					wall(p1.x, s2.ceil_height, p1.y, p2.x, sector.ceil_height, p2.y);
				}


			}


			// floor and ceiling
			auto p1 = map.walls[sector.wall_index].pos;

			for (int i = 2; i < sector.wall_count; ++i) {
				auto p2 = map.walls[sector.wall_index + i - 1].pos;
				auto p3 = map.walls[sector.wall_index + i].pos;

				floor_verts.emplace_back(glm::vec3(p1.x, sector.floor_height, p1.y), p1, col);
				floor_verts.emplace_back(glm::vec3(p2.x, sector.floor_height, p2.y), p2, col);
				floor_verts.emplace_back(glm::vec3(p3.x, sector.floor_height, p3.y), p3, col);

				ceil_verts.emplace_back(glm::vec3(p2.x, sector.ceil_height, p2.y), p2);
				ceil_verts.emplace_back(glm::vec3(p1.x, sector.ceil_height, p1.y), p1);
				ceil_verts.emplace_back(glm::vec3(p3.x, sector.ceil_height, p3.y), p3);
			}
		}


		glm::mat4 mat_perspective = glm::perspective(
			glm::radians(60.0f),
			rmw::context.get_aspect_ratio(),
			0.1f, 100.0f);

		shader->set_uniform("mvp", mat_perspective * eye.get_view_mtx());


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
	}




private:

	void wall(float x1, float y1, float z1, float x2, float y2, float z2) {

		float u1, u2;
		if (abs(x2 - x1) > abs(z2 - z1)) {
			u1 = x1;
			u2 = x2;
		}
		else {
			u1 = z1;
			u2 = z2;
		}

		wall_verts.emplace_back(glm::vec3(x1, y1, z1), glm::vec2(u1, y1));
		wall_verts.emplace_back(glm::vec3(x1, y2, z1), glm::vec2(u1, y2));
		wall_verts.emplace_back(glm::vec3(x2, y2, z2), glm::vec2(u2, y2));

		wall_verts.emplace_back(glm::vec3(x1, y1, z1), glm::vec2(u1, y1));
		wall_verts.emplace_back(glm::vec3(x2, y2, z2), glm::vec2(u2, y2));
		wall_verts.emplace_back(glm::vec3(x2, y1, z2), glm::vec2(u2, y1));

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


	renderer2D.init();


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
				break;

			case SDL_MOUSEMOTION:
				editor.mouse_motion(e.motion);
				break;

			case SDL_MOUSEBUTTONUP:
				editor.mouse_up(e.button);
				break;

			case SDL_MOUSEBUTTONDOWN:
				editor.mouse_down(e.button);
				break;

			case SDL_MOUSEWHEEL:
				editor.mouse_wheel(e.wheel);
				break;

			default: break;
			}
		}



		eye.update();
		rmw::context.clear(rmw::ClearState { { 0.1, 0.1, 0.1, 1 } });

		renderer.draw();

		editor.draw();

		rmw::context.flip_buffers();
	}

}

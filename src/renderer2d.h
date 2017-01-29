#pragma once

#include "rmw.h"

class Renderer2D {
public:

	void init() {
		m_shader = rmw::context.create_shader(
			R"(#version 330
				layout(location = 0) in vec2 in_pos;
				layout(location = 1) in vec4 in_color;
				uniform vec2 resolution;
				out vec4 ex_color;
				void main() {
					vec2 p = in_pos / resolution * 2.0 - 1;
					gl_Position = vec4(p.x, -p.y, 0.0, 1.0);
					ex_color = in_color;
				})",
			R"(#version 330
				in vec4 ex_color;
				out vec4 out_color;
				void main() {
					out_color = ex_color;
				})");
		m_rs.line_width = 2;
		m_rs.depth_test_enabled = false;
		m_vb = rmw::context.create_vertex_buffer(rmw::BufferHint::StreamDraw);
		m_va = rmw::context.create_vertex_array();
		m_va->set_primitive_type(rmw::PrimitiveType::Lines);
		m_va->set_attribute(0, m_vb, rmw::ComponentType::Float, 2, false, 0, sizeof(Vert));
		m_va->set_attribute(1, m_vb, rmw::ComponentType::Uint8, 4, true,  8, sizeof(Vert));

	}


	void push() {
		assert(m_transform_index < m_transforms.size() - 2);
		++m_transform_index;
		transform() = m_transforms[m_transform_index - 1];
	}
	void pop() {
		assert(m_transform_index > 0);
		--m_transform_index;
	}
	void origin() {
		transform() = glm::mat3x2();
	}
	void scale(const glm::vec2& v) { scale(v.x, v.y); }
	void scale(float x) { scale(x, x); }
	void scale(float x, float y) {
		transform()[0][0] *= x;
		transform()[1][1] *= y;
	}
	void translate(const glm::vec2& v) { translate(v.x, v.y); }
	void translate(float x, float y) {
		transform()[2][0] += x;
		transform()[2][1] += y;
	}

	void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255) {
		m_color[0] = r;
		m_color[1] = g;
		m_color[2] = b;
		m_color[3] = a;
	}

	void line(float x1, float y1, float x2, float y2) {
		line(glm::vec2(x1, y1), glm::vec2(x2, y2));
	}
	void line(const glm::vec2& p1, const glm::vec2& p2) {
		m_vertices.emplace_back(transform() * glm::vec3(p1, 1), m_color);
		m_vertices.emplace_back(transform() * glm::vec3(p2, 1), m_color);
	}

	void flush() {
		m_vb->init_data(m_vertices);
		m_va->set_count(m_vertices.size());
		m_vertices.clear();

		m_shader->set_uniform("resolution", glm::vec2(rmw::context.get_width(), rmw::context.get_height()));

		rmw::context.draw(m_rs, m_shader, m_va);
	}

private:

	glm::mat3x2& transform() { return m_transforms[m_transform_index]; }

	struct Vert {
		glm::vec2				pos;
		std::array<uint8_t, 4>	color;
		Vert(const glm::vec2& p, const std::array<uint8_t, 4>& c) : pos(p), color(c) {}
	};

	std::vector<Vert>			m_vertices;
	std::array<uint8_t, 4> 		m_color;

	rmw::RenderState			m_rs;
	rmw::VertexArray::Ptr		m_va;
	rmw::VertexBuffer::Ptr		m_vb;
	rmw::Shader::Ptr			m_shader;

	std::array<glm::mat3x2, 16>	m_transforms;
	size_t						m_transform_index = 0;
};

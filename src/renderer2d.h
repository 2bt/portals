#pragma once

#include "rmw.h"

class Renderer2D {
public:

	void init() {
		m_shader = rmw::context.create_shader(
			R"(#version 330
				layout(location = 0) in vec2 in_pos;
				layout(location = 1) in vec4 in_color;
				layout(location = 2) in float in_point_size;
				uniform vec2 resolution;
				out vec4 ex_color;
				void main() {
					vec2 p = in_pos / resolution * 2.0 - 1;
					gl_Position = vec4(p.x, -p.y, 0.0, 1.0);
					ex_color = in_color;
					gl_PointSize = in_point_size;
				})",
			R"(#version 330
				in vec4 ex_color;
				out vec4 out_color;
				void main() {
					out_color = ex_color;
				})");

		m_rs.line_width = 1;
		m_rs.depth_test_enabled = false;

		m_rs.blend_enabled = true;
		m_rs.blend_func_src_rgb		= rmw::BlendFunc::SrcAlpha;
		m_rs.blend_func_src_alpha	= rmw::BlendFunc::SrcAlpha;
		m_rs.blend_func_dst_rgb		= rmw::BlendFunc::OneMinusSrcAlpha;
		m_rs.blend_func_dst_alpha	= rmw::BlendFunc::OneMinusSrcAlpha;

		m_vb = rmw::context.create_vertex_buffer(rmw::BufferHint::StreamDraw);
		m_va = rmw::context.create_vertex_array();
		m_va->set_attribute(0, m_vb, rmw::ComponentType::Float, 2, false,  0, sizeof(Vert));
		m_va->set_attribute(1, m_vb, rmw::ComponentType::Uint8, 4, true,   8, sizeof(Vert));
		m_va->set_attribute(2, m_vb, rmw::ComponentType::Float, 1, false, 12, sizeof(Vert));
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
		transform()[2][0] += x * transform()[0][0];
		transform()[2][1] += y * transform()[1][1];
	}

	void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255) {
		m_color[0] = r;
		m_color[1] = g;
		m_color[2] = b;
		m_color[3] = a;
	}
	void set_point_size(float s) {
		m_point_size = s;
	}
	void set_line_width(float w) {
		if (w != m_rs.line_width &&
			m_va->get_primitive_type() == rmw::PrimitiveType::Lines) flush();
		m_rs.line_width = w;
	}

	void point(float x, float y) {
		point(glm::vec2(x, y));
	}
	void point(const glm::vec2& p) {
		if (m_va->get_primitive_type() != rmw::PrimitiveType::Points) {
			flush();
			m_va->set_primitive_type(rmw::PrimitiveType::Points);
		}
		m_verts.emplace_back(transform() * glm::vec3(p, 1), m_color, m_point_size);
	}
	void line(float x1, float y1, float x2, float y2) {
		line(glm::vec2(x1, y1), glm::vec2(x2, y2));
	}
	void line(const glm::vec2& p1, const glm::vec2& p2) {
		if (m_va->get_primitive_type() != rmw::PrimitiveType::Lines) {
			flush();
			m_va->set_primitive_type(rmw::PrimitiveType::Lines);
		}
		m_verts.emplace_back(transform() * glm::vec3(p1, 1), m_color);
		m_verts.emplace_back(transform() * glm::vec3(p2, 1), m_color);
	}
	void rect(const glm::vec2& p1, const glm::vec2& p2) {
		line(p1.x, p1.y, p1.x, p2.y);
		line(p1.x, p2.y, p2.x, p2.y);
		line(p2.x, p2.y, p2.x, p1.y);
		line(p2.x, p1.y, p1.x, p1.y);
	}

	void triangle(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3) {
		if (m_va->get_primitive_type() != rmw::PrimitiveType::Triangles) {
			flush();
			m_va->set_primitive_type(rmw::PrimitiveType::Triangles);
		}
		m_verts.emplace_back(transform() * glm::vec3(p1, 1), m_color);
		m_verts.emplace_back(transform() * glm::vec3(p2, 1), m_color);
		m_verts.emplace_back(transform() * glm::vec3(p3, 1), m_color);
	}

	void flush() {
		m_shader->set_uniform("resolution", glm::vec2(rmw::context.get_width(), rmw::context.get_height()));

		m_vb->init_data(m_verts);
		m_va->set_count(m_verts.size());
		m_verts.clear();
		rmw::context.draw(m_rs, m_shader, m_va);
	}

private:

	glm::mat3x2& transform() { return m_transforms[m_transform_index]; }

	struct Vert {
		glm::vec2				pos;
		std::array<uint8_t, 4>	color;
		float size;
		Vert(const glm::vec2& p, const std::array<uint8_t, 4>& c, float s=0)
			: pos(p), color(c), size(s) {}
	};

	std::array<uint8_t, 4> 		m_color;
	float						m_point_size = 1;

	std::vector<Vert>			m_verts;

	rmw::RenderState			m_rs;
	rmw::Shader::Ptr			m_shader;
	rmw::VertexArray::Ptr		m_va;
	rmw::VertexBuffer::Ptr		m_vb;

	std::array<glm::mat3x2, 16>	m_transforms;
	size_t						m_transform_index = 0;
};


extern Renderer2D	renderer2D;

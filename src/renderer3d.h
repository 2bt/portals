#pragma once

#include "rmw.h"


class Renderer3D {
public:

	void init() {
		m_shader = rmw::context.create_shader(
			R"(#version 330
				layout(location = 0) in vec3 in_pos;
				layout(location = 1) in vec4 in_color;
				layout(location = 2) in float in_point_size;
				uniform mat4 mvp;
				out vec4 ex_color;
				void main() {
					gl_Position = mvp * vec4(in_pos, 1.0);
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
		m_rs.depth_test_enabled = true;

		m_rs.blend_enabled = true;
		m_rs.blend_func_src_rgb		= rmw::BlendFunc::SrcAlpha;
		m_rs.blend_func_src_alpha	= rmw::BlendFunc::SrcAlpha;
		m_rs.blend_func_dst_rgb		= rmw::BlendFunc::OneMinusSrcAlpha;
		m_rs.blend_func_dst_alpha	= rmw::BlendFunc::OneMinusSrcAlpha;

		m_vb = rmw::context.create_vertex_buffer(rmw::BufferHint::StreamDraw);
		m_va = rmw::context.create_vertex_array();
		m_va->set_attribute(0, m_vb, rmw::ComponentType::Float, 3, false,  0, sizeof(Vert));
		m_va->set_attribute(1, m_vb, rmw::ComponentType::Uint8, 4, true,  12, sizeof(Vert));
		m_va->set_attribute(2, m_vb, rmw::ComponentType::Float, 1, false, 16, sizeof(Vert));
	}


	void set_transformation(const glm::mat4 mat) {
		flush();
		m_shader->set_uniform("mvp", mat);
	}
	void set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255) {
		m_color.r = r;
		m_color.g = g;
		m_color.b = b;
		m_color.a = a;
	}
	void set_point_size(float s) {
		m_point_size = s;
	}
	void set_line_width(float w) {
		if (w != m_rs.line_width &&
			m_va->get_primitive_type() == rmw::PrimitiveType::Lines) flush();
		m_rs.line_width = w;
	}

	void point(const glm::vec3& p) {
		if (m_va->get_primitive_type() != rmw::PrimitiveType::Points) {
			flush();
			m_va->set_primitive_type(rmw::PrimitiveType::Points);
		}
		m_verts.emplace_back(p, m_color, m_point_size);
	}
	void line(const glm::vec3& p1, const glm::vec3& p2) {
		if (m_va->get_primitive_type() != rmw::PrimitiveType::Lines) {
			flush();
			m_va->set_primitive_type(rmw::PrimitiveType::Lines);
		}
		m_verts.emplace_back(p1, m_color);
		m_verts.emplace_back(p2, m_color);
	}
	void triangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
		if (m_va->get_primitive_type() != rmw::PrimitiveType::Triangles) {
			flush();
			m_va->set_primitive_type(rmw::PrimitiveType::Triangles);
		}
		m_verts.emplace_back(p1, m_color);
		m_verts.emplace_back(p2, m_color);
		m_verts.emplace_back(p3, m_color);
	}

	void flush() {
		m_vb->init_data(m_verts);
		m_va->set_count(m_verts.size());
		m_verts.clear();
		rmw::context.draw(m_rs, m_shader, m_va);
	}

private:

	struct Vert {
		glm::vec3		pos;
		glm::u8vec4		color;
		float			size;
		Vert(const glm::vec3& p, const glm::u8vec4& c, float s=0)
			: pos(p), color(c), size(s) {}
	};

	glm::u8vec4					m_color;
	float						m_point_size = 1;

	std::vector<Vert>			m_verts;

	rmw::RenderState			m_rs;
	rmw::Shader::Ptr			m_shader;
	rmw::VertexArray::Ptr		m_va;
	rmw::VertexBuffer::Ptr		m_vb;
};


extern Renderer3D	renderer3D;

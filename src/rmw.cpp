#include "rmw.h"
#include <glm/glm.hpp>
#include <GL/glew.h>


namespace rmw {


constexpr uint32_t map_to_gl(ComponentType t) {
	const uint32_t lut[] = {
		GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT,
		GL_INT, GL_UNSIGNED_INT, GL_FLOAT, GL_HALF_FLOAT,
	};
	return lut[static_cast<int>(t)];
}
constexpr uint32_t map_to_gl(BufferHint h) {
	const uint32_t lut[] = { GL_STREAM_DRAW, GL_DYNAMIC_DRAW, GL_STATIC_DRAW };
	return lut[static_cast<int>(h)];
}
constexpr uint32_t map_to_gl(DepthTestFunc dtf) {
	const uint32_t lut[] = { GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, };
	return lut[static_cast<int>(dtf)];
}
constexpr uint32_t map_to_gl(PrimitiveType pt) {
	const uint32_t lut[] = {
		GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES,
		GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES
	};
	return lut[static_cast<int>(pt)];
}
constexpr uint32_t map_to_gl(BlendFunc bf) {
	const uint32_t lut[] = {
		GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA,
		GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR,
		GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA,
		GL_SRC_ALPHA_SATURATE,
	};
	return lut[static_cast<int>(bf)];
}
constexpr uint32_t map_to_gl(BlendEquation be) {
	const uint32_t lut[] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT };
	return lut[static_cast<int>(be)];
}




VertexArray::VertexArray() {
	m_first = 0;
	m_count = 0;
	m_indexed = false;
	m_primitive_type = PrimitiveType::Triangles;
	glGenVertexArrays(1, &m_va);
}
VertexArray::~VertexArray() {
	glDeleteVertexArrays(1, &m_va);
}


void VertexArray::set_attribute(int i, const VertexBuffer::Ptr& vb, ComponentType component_type,
								int component_count, bool normalized, int offset, int stride) {
	glBindVertexArray(m_va);
	vb->bind();
	glEnableVertexAttribArray(i);
	glVertexAttribPointer(i, component_count, map_to_gl(component_type), normalized, stride, reinterpret_cast<void*>(offset));
}
void VertexArray::set_attribute(int i, float f) {
	glBindVertexArray(m_va);
	glDisableVertexAttribArray(i);
	glVertexAttrib1f(i, f);
}
void VertexArray::set_attribute(int i, const glm::vec2& v) {
	glBindVertexArray(m_va);
	glDisableVertexAttribArray(i);
	glVertexAttrib2fv(i, &v.x);
}
void VertexArray::set_attribute(int i, const glm::vec3& v) {
	glBindVertexArray(m_va);
	glDisableVertexAttribArray(i);
	glVertexAttrib3fv(i, &v.x);
}
void VertexArray::set_attribute(int i, const glm::vec4& v) {
	glBindVertexArray(m_va);
	glDisableVertexAttribArray(i);
	glVertexAttrib4fv(i, &v.x);
}

void VertexArray::set_index_buffer(const IndexBuffer& ib) {
	m_indexed = true;
	glBindVertexArray(m_va);
	ib.bind();
}


GpuBuffer::GpuBuffer(uint32_t target, BufferHint hint) : m_target(target), m_hint(hint), m_size(0) {
	glGenBuffers(1, &m_b);
}
GpuBuffer::~GpuBuffer() {
	glDeleteBuffers(1, &m_b);
}
void GpuBuffer::bind() const {
	glBindBuffer(m_target, m_b);
}
void GpuBuffer::init_data(const void* data, int size) {
	m_size = size;
	glBindVertexArray(0);
	bind();
	glBufferData(m_target, m_size, data, map_to_gl(m_hint));
}

VertexBuffer::VertexBuffer(BufferHint hint) : GpuBuffer(GL_ARRAY_BUFFER, hint) {}
IndexBuffer::IndexBuffer(BufferHint hint) : GpuBuffer(GL_ELEMENT_ARRAY_BUFFER, hint) {}










// shader


static GLuint compile_shader(uint32_t type, const char* src) {
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, nullptr);
	glCompileShader(s);
	GLint e = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &e);
	if (e) return s;
	int len = 0;
	glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
	char log[len];
	glGetShaderInfoLog(s, len, &len, log);
	fprintf(stderr, "Error: can't compile shader\n%s\n%s\n", src, log);
	glDeleteShader(s);
	return 0;
}


bool Shader::init(const char* vs, const char* fs) {
	GLint v = compile_shader(GL_VERTEX_SHADER, vs);
	if (v == 0) return false;
	GLint f = compile_shader(GL_FRAGMENT_SHADER, fs);
	if (f == 0) {
		glDeleteShader(v);
		return false;
	}
	m_program = glCreateProgram();

	glAttachShader(m_program, v);
	glAttachShader(m_program, f);
	glDeleteShader(v);
	glDeleteShader(f);
	glLinkProgram(m_program);


	// attributes
	int count;
	glGetProgramiv(m_program, GL_ACTIVE_ATTRIBUTES, &count);
	for (int i = 0; i < count; ++i) {
		char name[128];
		int size;
		uint32_t type;
		glGetActiveAttrib(m_program, i, sizeof(name), nullptr, &size, &type, name);
		m_attributes.push_back({ name, type, glGetAttribLocation(m_program, name) });
	}

	// uniforms
	glGetProgramiv(m_program, GL_ACTIVE_UNIFORMS, &count);
	for (int i = 0; i < count; ++i) {
		char name[128];
		int size; // > 1 for arrays
		uint32_t type;
		glGetActiveUniform(m_program, i, sizeof(name), nullptr, &size, &type, name);
		int location = glGetUniformLocation(m_program, name);
		Uniform::Ptr u;
		switch (type) {
		case GL_FLOAT:		u = std::make_unique<UniformExtend<float>>(name, type, location); break;
		case GL_FLOAT_VEC2: u = std::make_unique<UniformExtend<glm::vec2>>(name, type, location); break;
		case GL_FLOAT_VEC3: u = std::make_unique<UniformExtend<glm::vec3>>(name, type, location); break;
		case GL_FLOAT_VEC4: u = std::make_unique<UniformExtend<glm::vec4>>(name, type, location); break;
		case GL_FLOAT_MAT4: u = std::make_unique<UniformExtend<glm::mat4>>(name, type, location); break;
		case GL_SAMPLER_2D: u = std::make_unique<UniformTexture2D>(name, type, location); break;

		default:
			fprintf(stderr, "Error: uniform '%s' has unknown type\n", name);
			assert(false);
		}
		m_uniforms.push_back(std::move(u));
	}

	return true;
}


Shader::~Shader() {
	glDeleteProgram(m_program);
}


void gl_uniform(int l, float v) { glUniform1f(l, v); }
void gl_uniform(int l, const glm::vec2& v) { glUniform2fv(l, 1, &v.x); }
void gl_uniform(int l, const glm::vec3& v) { glUniform3fv(l, 1, &v.x); }
void gl_uniform(int l, const glm::vec4& v) { glUniform4fv(l, 1, &v.x); }
void gl_uniform(int l, const glm::mat4& v) { glUniformMatrix4fv(l, 1, false, &v[0].x); }



template <class T>
void Shader::UniformExtend<T>::update() const {
	if (!dirty) return;
	dirty = false;
	gl_uniform(location, value);
}

void Shader::UniformTexture2D::update() const {
	// NOTE: this is hacky
	// but what's the best way to do this?
	int unit = location;
	// TODO: set this via Context
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, handle);
	glUniform1i(location, unit);
}


/*
	// TODO:
	// cache bound textures to minimize calls to glBindTexture()
	// cache this variable in Context
	int max_texture_units;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_texture_units);
	std::vector<GLuint> bound_textures(max_texture_units);
*/
Texture2D::Texture2D(SDL_Surface* img) {

	glGenTextures(1, &m_handle);
	glBindTexture(GL_TEXTURE_2D, m_handle); // TODO: caching via Context

//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->w, img->h, 0, GL_RGB, GL_UNSIGNED_BYTE, img->pixels);

	glGenerateMipmap(GL_TEXTURE_2D);

//	glBindTexture(GL_TEXTURE_2D, 0);
}




// Context


bool Context::init(int width, int height, const char* title)
{
	m_viewport.w = width;
	m_viewport.h = height;

	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);


	m_window = SDL_CreateWindow(title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			m_viewport.w, m_viewport.h,
			SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE*/);

	SDL_GL_SetSwapInterval(-1); // v-sync
	m_gl_context = SDL_GL_CreateContext(m_window);

	glewExperimental = true;
	glewInit();


	glEnable(GL_PROGRAM_POINT_SIZE);


	// initialize the reder state according to opengl's initial state
	m_render_state.depth_test_enabled = 0;
	m_render_state.depth_test_func = DepthTestFunc::Less;

	return true;
}

Context::~Context()
{
	SDL_GL_DeleteContext(m_gl_context);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
	IMG_Quit();
}


bool Context::poll_event(SDL_Event& e) {
	if (!SDL_PollEvent(&e)) return false;
	if (e.type == SDL_WINDOWEVENT
	&&  e.window.event == SDL_WINDOWEVENT_RESIZED) {
		m_viewport.w = e.window.data1;
		m_viewport.h = e.window.data2;
	}

	return true;
}


void Context::clear(const ClearState& cs) {

	if (m_clear_state.color != cs.color) {
		m_clear_state.color = cs.color;
		glClearColor(m_clear_state.color.x, m_clear_state.color.y, m_clear_state.color.z, m_clear_state.color.w);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}



void Context::draw(const RenderState& rs, const Shader::Ptr& shader, const VertexArray::Ptr& va) {
	if (va->m_count == 0) return;

	// sync render state
	if (m_render_state.depth_test_enabled != rs.depth_test_enabled) {
		m_render_state.depth_test_enabled = rs.depth_test_enabled;
		if (m_render_state.depth_test_enabled) {
			glEnable(GL_DEPTH_TEST);
			if (m_render_state.depth_test_func != rs.depth_test_func) {
				m_render_state.depth_test_func = rs.depth_test_func;
				glDepthFunc(map_to_gl(m_render_state.depth_test_func));
			}
		}
		else glDisable(GL_DEPTH_TEST);
	}
	if (m_render_state.blend_enabled != rs.blend_enabled) {
		m_render_state.blend_enabled = rs.blend_enabled;
		if (m_render_state.blend_enabled) {
			glEnable(GL_BLEND);
			if (m_render_state.blend_func_src_rgb != rs.blend_func_src_rgb
			|| m_render_state.blend_func_src_alpha != rs.blend_func_src_alpha
			|| m_render_state.blend_func_dst_rgb != rs.blend_func_dst_rgb
			|| m_render_state.blend_func_dst_alpha != rs.blend_func_dst_alpha) {
				m_render_state.blend_func_src_rgb	= rs.blend_func_src_rgb;
				m_render_state.blend_func_src_alpha	= rs.blend_func_src_alpha;
				m_render_state.blend_func_dst_rgb	= rs.blend_func_dst_rgb;
				m_render_state.blend_func_dst_alpha	= rs.blend_func_dst_alpha;
				glBlendFuncSeparate(
						map_to_gl(m_render_state.blend_func_src_rgb),
						map_to_gl(m_render_state.blend_func_dst_rgb),
						map_to_gl(m_render_state.blend_func_src_alpha),
						map_to_gl(m_render_state.blend_func_dst_alpha));
			}
			if (m_render_state.blend_equation_rgb != rs.blend_equation_rgb
			|| m_render_state.blend_equation_alpha != rs.blend_equation_alpha) {
				m_render_state.blend_equation_rgb = rs.blend_equation_rgb;
				m_render_state.blend_equation_alpha = rs.blend_equation_alpha;
				glBlendEquationSeparate(
						map_to_gl(m_render_state.blend_equation_rgb),
						map_to_gl(m_render_state.blend_equation_alpha));
			}
			if (m_render_state.blend_color != rs.blend_color) {
				m_render_state.blend_color = rs.blend_color;
				glBlendColor(
						m_render_state.blend_color.r,
						m_render_state.blend_color.g,
						m_render_state.blend_color.b,
						m_render_state.blend_color.a);
			}
		}
		else glDisable(GL_BLEND);
	}



	// only consider RenderState::viewport if it's valid
	const Viewport& vp = rs.viewport.w == 0 ? m_viewport : rs.viewport;
	if (memcmp(&m_render_state.viewport, &vp, sizeof(Viewport)) != 0) {
		m_render_state.viewport = vp;
		glViewport( m_render_state.viewport.x,
					m_render_state.viewport.y,
					m_render_state.viewport.w,
					m_render_state.viewport.h);
	}
	if (m_render_state.line_width != rs.line_width) {
		m_render_state.line_width = rs.line_width;
		glLineWidth(m_render_state.line_width);
	}


	// sync shader
	if (m_shader != shader.get()) {
		m_shader = shader.get();
		glUseProgram(m_shader->m_program);
	}
	m_shader->update_uniforms();


	glBindVertexArray(va->m_va);

	if (va->m_indexed) {
		glDrawElements(map_to_gl(va->m_primitive_type), va->m_count, GL_UNSIGNED_INT,
				reinterpret_cast<void*>(va->m_first));
	}
	else {
		glDrawArrays(map_to_gl(va->m_primitive_type), va->m_first, va->m_count);
	}

}


void Context::flip_buffers() const {
	SDL_GL_SwapWindow(m_window);
}


Texture2D::Ptr Context::create_texture_2D(SDL_Surface* img) const {
	return Texture2D::Ptr(new Texture2D(img));
}


Texture2D::Ptr Context::create_texture_2D(const char* file) const {
	SDL_Surface* img = IMG_Load(file);
	if (!img) return nullptr;
	Texture2D::Ptr tex(new Texture2D(img));
	SDL_FreeSurface(img);
	return tex;
}


// the singleton
rmw::Context context;


}

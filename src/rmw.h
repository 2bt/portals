#pragma once


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <vector>


namespace rmw {


// states

struct ClearState {
	glm::vec4		color;
};


struct Viewport {
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
};

enum class DepthTestFunc { Never, Less, Equal, LEqual };

enum class BlendFunc {
	Zero, One, SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor,
	SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha,
	ConstantColor, OneMinusConstantColor,
	ConstantAlpha, OneMinusConstantAlph,
	SrcAlphaSaturate,
};
enum class BlendEquation { Add, Subtract, ReverseSubtract };

struct RenderState {
	Viewport		viewport;

	// depth
	bool			depth_test_enabled		= false;
	DepthTestFunc	depth_test_func			= DepthTestFunc::LEqual;

	// blend
	bool			blend_enabled			= false;
	BlendFunc		blend_func_src_rgb		= BlendFunc::One;
	BlendFunc		blend_func_src_alpha	= BlendFunc::Zero;
	BlendFunc		blend_func_dst_rgb		= BlendFunc::One;
	BlendFunc		blend_func_dst_alpha	= BlendFunc::Zero;
	BlendEquation	blend_equation_rgb		= BlendEquation::Add;
	BlendEquation	blend_equation_alpha	= BlendEquation::Add;
	glm::vec4		blend_color;

	float			line_width				= 1;
};


// buffers

enum class BufferHint { StaticDraw, StreamDraw, DynamicDraw };


class GpuBuffer {
public:
	virtual ~GpuBuffer();

	void init_data(const void* data, int size);

	int size() const { return _size; }

protected:
	GpuBuffer(const GpuBuffer&) = delete;
	GpuBuffer& operator=(const GpuBuffer&) = delete;
	GpuBuffer(uint32_t target, BufferHint hint);

	void bind() const;

	uint32_t	_target;
	BufferHint	_hint;
	int			_size;
	uint32_t	_b;
};


class VertexBuffer : public GpuBuffer {
	friend class Context;
	friend class VertexArray;
public:
	typedef std::unique_ptr<VertexBuffer> Ptr;

	using GpuBuffer::init_data;

	template<class T>
	void init_data(const std::vector<T>& data) {
		init_data(static_cast<const void*>(data.data()), data.size() * sizeof(T));
	}

private:

	VertexBuffer(BufferHint hint);
};


class IndexBuffer : public GpuBuffer {
	friend class Context;
	friend class VertexArray;
public:
	typedef std::unique_ptr<IndexBuffer> Ptr;

	using GpuBuffer::init_data;

	void init_data(const std::vector<int>& data) {
		init_data(static_cast<const void*>(data.data()), data.size() * sizeof(int));
	}

private:
	IndexBuffer(BufferHint hint);
};


enum class PrimitiveType { Points, LineStrip, LineLoop, Lines, TriangleStrip, TriangleFan, Triangles };
enum class ComponentType { Int8, Uint8, Int16, Uint16, Int32, Uint32, Float, HalfFloat };


class VertexArray {
	friend class Context;
public:
	typedef std::unique_ptr<VertexArray> Ptr;

	~VertexArray();

	void set_first(int i) { _first = i; };
	void set_count(int i) { _count = i; };
	void set_primitive_type(PrimitiveType t) { _primitive_type = t; };
	PrimitiveType get_primitive_type() const { return _primitive_type; };

	void set_attribute(int i, const VertexBuffer::Ptr& vb, ComponentType component_type,
						int component_count, bool normalized, int offset, int stride);
	void set_attribute(int i, float f);
	void set_attribute(int i, const glm::vec2& v);
	void set_attribute(int i, const glm::vec3& v);
	void set_attribute(int i, const glm::vec4& v);
	// ...

	void set_index_buffer(const IndexBuffer& ib);

	enum { MAX_NUM_ATTRIBUTES = 5 };

private:
	VertexArray(const VertexArray&) = delete;
	VertexArray& operator=(const VertexArray&) = delete;
	VertexArray();

	int						_first;
	int						_count;
	bool					_indexed;
	PrimitiveType			_primitive_type;
	uint32_t				_va;
};


// texture

enum class WrapMode { Clamp, Repeat, ClampZero, MirrowedRepeat };
enum class FilterMode { Linear, Nearest }; // TODO: mipmap


class Texture2D {
	friend class Context;
	friend class Shader;
public:
	typedef std::unique_ptr<Texture2D> Ptr;



	// sampler stuff
	void set_wrap(WrapMode horiz, WrapMode vert);
	void set_filter(FilterMode min, FilterMode mag);


private:
	Texture2D(SDL_Surface* img);

	uint32_t		_handle;
};


// shader

class Shader {
	friend class Context;
public:
	typedef std::unique_ptr<Shader> Ptr;

//	enum class AttributeType { Float, Vec2, Vec3, Vec4, Mat2, Mat3, Mat4 };

	template<class T>
	void set_uniform(const std::string& name, const T& value) {
		for (auto& u : _uniforms) {
			if (u->name == name) {
				auto* ue = dynamic_cast<UniformExtend<T>*>(u.get());
				assert(ue);
				if (ue->value != value) {
					ue->value = value;
					ue->dirty = true;
				}
				return;
			}
		}
		assert(false);
	}
	void set_uniform(const std::string& name, const Texture2D::Ptr& texture) {
		for (auto& u : _uniforms) {
			if (u->name == name) {
				auto* ue = dynamic_cast<UniformTexture2D*>(u.get());
				assert(ue);
				ue->set(texture);
				return;
			}
		}
		assert(false);
	}


	~Shader();
private:
	Shader() {}
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	bool init(const char* vs, const char* fs);

	struct Attribute {
		std::string		name;
		uint32_t		type;
		int				location;
	};

	struct Uniform {
		typedef std::unique_ptr<Uniform> Ptr;
		std::string		name;
		uint32_t		type;
		int				location;
		Uniform(const std::string& name, uint32_t type, int location) : name(name), type(type), location(location) {}
		virtual ~Uniform() {}
		virtual void update() const = 0;
	};

	struct UniformTexture2D : Uniform {
		UniformTexture2D(const std::string& name, uint32_t type, int location) : Uniform(name, type, location) {}
		void update() const override;
//		void update() const override {
//			if (!dirty) return;
//			dirty = false;
////			gl_uniform(location, value);
//		}
		void set(const Texture2D::Ptr& texture) {
			handle = texture->_handle;
		}
		uint32_t		handle;
	};

	template <class T>
	struct UniformExtend : Uniform {
		UniformExtend(const std::string& name, uint32_t type, int location) : Uniform(name, type, location) {}
		void update() const override;
		mutable bool	dirty { true };
		T				value { 0 };
	};

	void update_uniforms() const {
		for (auto& u : _uniforms) u->update();
	}



	uint32_t					_program = 0;
	std::vector<Attribute>		_attributes;
	std::vector<Uniform::Ptr>	_uniforms;
};



class Context {
public:

	bool init(int width, int height, const char* title);
	~Context();

	bool poll_event(SDL_Event& e);

	int get_width()  const { return _viewport.w; }
	int get_height() const { return _viewport.h; }
	float get_aspect_ratio() const { return _viewport.w / (float) _viewport.h; }


	void clear(const ClearState& cs);
	void draw(const RenderState& rs, const Shader::Ptr& shader, const VertexArray::Ptr& va);
	void flip_buffers() const;


	// NOTE: don't use std::make_unique because of private constructors

	Shader::Ptr create_shader(const char* vs, const char* fs) const {
		Shader::Ptr s(new Shader());
		if (!s->init(vs, fs)) return nullptr;
		return s;
	}

	VertexBuffer::Ptr create_vertex_buffer(BufferHint hint) const {
		return VertexBuffer::Ptr(new VertexBuffer(hint));
	}

	IndexBuffer::Ptr create_index_buffer(BufferHint hint) const {
		return IndexBuffer::Ptr(new IndexBuffer(hint));
	}

	VertexArray::Ptr create_vertex_array() const {
		return VertexArray::Ptr(new VertexArray());
	}

	Texture2D::Ptr create_texture_2D(const char* file) const;


private:

	SDL_Window*		_window;
	Viewport		_viewport;
	SDL_GLContext	_gl_context;

	RenderState		_render_state;
	ClearState		_clear_state;
	const Shader*	_shader;
};


extern rmw::Context context;


}

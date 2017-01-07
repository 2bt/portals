#pragma once


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <glm/glm.hpp>
#include <memory>
#include <array>
#include <vector>


namespace rmw {


struct ClearState {
	glm::vec4		color;
};


enum class DepthTestFunc { Never, Less, Equal, LEqual };
enum class PrimitiveType { Points, LineStrip, LineLoop, Lines, TriangleStrip, TriangleFan, Triangles };
struct Viewport { int x=0, y=0, w=0, h=0; };

struct RenderState {
	Viewport		viewport;
	bool			depth_test_enabled	= true;
	DepthTestFunc	depth_test_func		= DepthTestFunc::LEqual;

	float			line_width			= 1;

	// TODO:
	// blending
	// ...
};



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


enum class ComponentType { Int8, Uint8, Int16, Uint16, Int32, Uint32, Float, HalfFloat };



class VertexArray {
	friend class Context;
public:
	typedef std::unique_ptr<VertexArray> Ptr;

	~VertexArray();

	void set_first(int i) { _first = i; };
	void set_count(int i) { _count = i; };
	void set_primitive_type(PrimitiveType t) { _primitive_type = t; };

	void set_attribute(int i, const VertexBuffer::Ptr& vb, ComponentType ct, int cc, bool normalized, int offset, int stride);
	void set_attribute(int i, const glm::vec3& v);
	void set_attribute(int i, const glm::vec4& v);
	// ...

	void set_index_buffer(const IndexBuffer& ib);

	enum {
		MAX_NUM_ATTRIBUTES = 5
	};

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




class Shader {
	friend class Context;
public:
	typedef std::unique_ptr<Shader> Ptr;

	enum class AttributeType { Float, Vec2, Vec3, Vec4, Mat2, Mat3, Mat4 };

	template<class T>
	void set_uniform(const std::string& name, const T& value) {
		for (auto& u : _uniforms) {
			if (u->name == name) {
				auto& ue = dynamic_cast<UniformExtend<T>&>(*u);
				if (ue.value != value) {
					ue.value = value;
					ue.dirty = true;
				}
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

	void clear(const ClearState& cs);
	void draw(const RenderState& rs, const Shader::Ptr& shader, const VertexArray::Ptr& va);
	void flip_buffers() const;

	Shader::Ptr create_shader(const char* vs, const char* fs) {
		Shader::Ptr s(new Shader());
		if (!s->init(vs, fs)) return nullptr;
		return s;
	}

	VertexBuffer::Ptr create_vertex_buffer(BufferHint hint) {
		return VertexBuffer::Ptr(new VertexBuffer(hint));
	}
	IndexBuffer::Ptr create_index_buffer(BufferHint hint) {
		return IndexBuffer::Ptr(new IndexBuffer(hint));
	}
	VertexArray::Ptr create_vertex_array() {
		return VertexArray::Ptr(new VertexArray());
	}



private:

	SDL_Window*		_window;
	int				_width;
	int				_height;
	SDL_GLContext	_gl_context;

	RenderState		_render_state;
	ClearState		_clear_state;
	const Shader*	_shader;
};


}

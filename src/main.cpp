#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "renderer2d.h"
#include "renderer3d.h"

#include "rmw.h"
#include "math.h"
#include "map.h"
#include "eye.h"
#include "editor.h"


Renderer2D renderer2D;
Renderer3D renderer3D;

Eye    eye;
Editor editor;


class MapRenderer {
public:

    void init() {

        shader = rmw::context.create_shader(
            R"(#version 100
                attribute vec3 in_pos;
                attribute vec2 in_uv;
                attribute vec2 in_uv2;
                uniform mat4 mvp;
                varying vec2 ex_uv;
                varying vec2 ex_uv2;
                varying float ex_depth;
                void main() {
                    gl_Position = mvp * vec4(in_pos, 1.0);
                    ex_uv = in_uv;
                    ex_uv2 = in_uv2;
                    ex_depth = gl_Position.z;
                })",
            R"(#version 100
                precision mediump float;
                varying vec2 ex_uv;
                varying vec2 ex_uv2;
                varying float ex_depth;
                uniform sampler2D tex;
                uniform sampler2D shadow;
                void main() {
                    vec4 c = texture2D(tex, ex_uv) * texture2D(shadow, ex_uv2);
                    gl_FragColor = vec4(c.rgb * pow(0.99, ex_depth), c.a);
                })");

        vertex_buffer = rmw::context.create_vertex_buffer(rmw::BufferHint::StreamDraw);
        vertex_array = rmw::context.create_vertex_array();
        vertex_array->set_primitive_type(rmw::PrimitiveType::Triangles);
        vertex_array->set_attribute(0, vertex_buffer, rmw::ComponentType::Float, 3, false, 0, sizeof(MapVertex));
        vertex_array->set_attribute(1, vertex_buffer, rmw::ComponentType::Float, 2, false, 12, sizeof(MapVertex));
        vertex_array->set_attribute(2, vertex_buffer, rmw::ComponentType::Float, 2, false, 20, sizeof(MapVertex));

        textures[0] = rmw::context.create_texture_2D("media/wall.png");
        textures[1] = rmw::context.create_texture_2D("media/floor.png");
        textures[2] = rmw::context.create_texture_2D("media/ceil.png");
        shadow_map = rmw::context.create_texture_2D(map.shadow_atlas.m_surfaces[0], rmw::FilterMode::Linear);
    }


    void draw(const rmw::RenderState& rs, const rmw::Framebuffer::Ptr& fb) {

        for (Mesh& m : meshes) m.clear();

        for (int i = 0; i < (int) map.sectors.size(); ++i) {
            const Sector& s = map.sectors[i];

            for (const MapFace& f : s.faces) {
                Mesh& m = meshes[f.tex_nr];
                m.insert(m.end(), f.verts.begin(), f.verts.end());
            }
        }


        glm::mat4 mat_perspective = glm::perspective(
            glm::radians(60.0f),
            rmw::context.get_aspect_ratio(),
            0.1f, 500.0f);

        glm::mat4 mat_view = eye.get_view_mtx();
        shader->set_uniform("mvp", mat_perspective * mat_view);
        shader->set_uniform("shadow", shadow_map);


        for (int i = 0; i < (int) meshes.size(); ++i) {
            vertex_buffer->init_data(meshes[i]);
            vertex_array->set_count(meshes[i].size());
            shader->set_uniform("tex", textures[i]);
            rmw::context.draw(rs, shader, vertex_array, fb);
        }


//        if (0)
//        {
//            renderer3D.set_transformation(mat_perspective * mat_view);
//            renderer3D.set_line_width(3);
//            renderer3D.set_point_size(5);
//
//            static glm::vec3 orig;
//            static glm::vec3 dir;
//            static glm::vec3 mark;
//            static glm::vec3 mark_normal;
//
//            int x, y;
//            int b = SDL_GetMouseState(&x, &y);
//            if (b) {
//                orig = eye.get_location().pos;
//                glm::vec4 c = glm::vec4(x / (float) rmw::context.get_width() * 2 - 1,
//                                        y / (float) rmw::context.get_height() * -2 + 1, -1, 1);
//                glm::vec4 v = glm::inverse(mat_perspective * mat_view) * c;
//                dir = glm::normalize(glm::vec3(v) / v.w - orig);
//
//                WallRef ref;
//                float f = map.ray_intersect(eye.get_location(), dir, ref, mark_normal);
//                mark = eye.get_location().pos + dir * f;
//
//                if (ref.wall_nr == -2) {
//                    auto& fs = map.sectors[ref.sector_nr].faces;
//                    auto& f = fs[fs.size() - 2];
//                    auto t = glm::ivec2(glm::floor(glm::vec2(f.inv_mat * glm::vec4(mark, 1)) + glm::vec2(0.5)));
//                    auto s = map.shadow_atlas.m_surfaces[0];
//                    auto p = (glm::u8vec3 *) ((uint8_t * ) s->pixels + (t.y + f.shadow.y) * s->pitch + (t.x + f.shadow.x) * sizeof(glm::u8vec3));
//                    p->r = 255;
//                    p->g = 0;
//                    p->b = 0;
//                    shadow_map = rmw::context.create_texture_2D(s);
//                }
//            }
//
//            renderer3D.set_color(0, 255, 0);
//            renderer3D.point(mark);
//            renderer3D.set_color(0, 100, 0);
//            renderer3D.line(mark, mark + mark_normal * 3.0f);
//            renderer3D.flush();
//        }
    }



private:
    typedef std::vector<MapVertex> Mesh;


    rmw::Shader::Ptr                   shader;
    rmw::VertexBuffer::Ptr             vertex_buffer;
    rmw::VertexArray::Ptr              vertex_array;

    std::array<Mesh, 3>                meshes;
    std::array<rmw::Texture2D::Ptr, 3> textures;
    rmw::Texture2D::Ptr                shadow_map;

} renderer;


bool running = true;

void loop(void* args) {
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
      case SDL_WINDOWEVENT:
          if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
//            offscreen_color->init(rmw::TextureFormat::RGB, rmw::context.get_width() / 4, rmw::context.get_height() / 4);
//            offscreen_depth->init(rmw::TextureFormat::Depth, rmw::context.get_width() / 4, rmw::context.get_height() / 4);
//            // NOTE: this is only necessary to update fb size
//            // so we don't need to set the viewport size
//            fb->attach_color(offscreen_color);
//            fb->attach_depth(offscreen_depth);
            }
            break;
        default: break;
        }
    }


    // update
    eye.update();

    // render
    rmw::context.clear(rmw::ClearState { { 0, 0, 1, 1 } });
    rmw::RenderState rs;
    rs.depth_test_enabled = true;
    renderer.draw(rs, rmw::context.get_default_framebuffer());

//    rmw::context.clear(rmw::ClearState { { 0, 0, 1, 1 } }, fb);
//    renderer.draw(rs, fb);
//    // draw quad
//    shader->set_uniform("tex", offscreen_color);
//    rmw::context.draw(rs, shader, va);

    editor.draw();
    rmw::context.flip_buffers();
}


int main(int argc, char** argv) {
    rmw::context.init(800, 600, "portal");

    renderer2D.init();
    renderer3D.init();

    renderer.init();

    eye.init();

//    auto vb = rmw::context.create_vertex_buffer(rmw::BufferHint::StreamDraw);
//    std::vector<int8_t> data = { 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, };
//    vb->init_data(data);
//    auto va = rmw::context.create_vertex_array();
//    va->set_primitive_type(rmw::PrimitiveType::Triangles);
//    va->set_count(6);
//    va->set_attribute(0, vb, rmw::ComponentType::Int8, 2, false, 0, 2);
//    auto shader = rmw::context.create_shader(
//        R"(#version 330
//            layout(location = 0) in vec2 in_pos;
//            uniform mat4 mvp;
//            out vec2 ex_uv;
//            void main() {
//                gl_Position = vec4(in_pos * 2.0 - vec2(1.0), 0, 1.0);
//                ex_uv = in_pos;
//            })",
//        R"(#version 330
//            in vec2 ex_uv;
//            uniform sampler2D tex;
//            out vec4 out_color;
//            void main() {
//                out_color = vec4(texture(tex, ex_uv).rgb, 1);
//            })");
//    rmw::Framebuffer::Ptr fb = rmw::context.create_framebuffer();
//    rmw::Texture2D::Ptr offscreen_color = rmw::context.create_texture_2D(rmw::TextureFormat::RGB, rmw::context.get_width() / 4, rmw::context.get_height() / 4);
//    rmw::Texture2D::Ptr offscreen_depth = rmw::context.create_texture_2D(rmw::TextureFormat::Depth, rmw::context.get_width() / 4, rmw::context.get_height() / 4);
//    fb->attach_color(offscreen_color);
//    fb->attach_depth(offscreen_depth);


#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(loop, nullptr, -1, true);
#else
    while (running) loop(nullptr);
#endif
}

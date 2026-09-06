#include "Renderer.hpp"
#include <gl/GL.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace engine {
namespace {
constexpr GLenum arrayBuffer = 0x8892, staticDraw = 0x88E4;
constexpr GLenum vertexShaderType = 0x8B31, fragmentShaderType = 0x8B30;
constexpr GLenum compileStatus = 0x8B81, linkStatus = 0x8B82, infoLogLength = 0x8B84;

PROC extension(const char* name) {
    const PROC address = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::intptr_t>(address);
    return (!address || value == 1 || value == 2 || value == 3 || value == -1) ? nullptr : address;
}
// Windows exports GL 1.1 directly; load the required core functions from the current driver.
struct GLApi {
#define GL_FUNCTIONS(X) \
    X(void, GenVertexArrays, (GLsizei, GLuint*)) \
    X(void, DeleteVertexArrays, (GLsizei, const GLuint*)) \
    X(void, BindVertexArray, (GLuint)) \
    X(void, GenBuffers, (GLsizei, GLuint*)) \
    X(void, DeleteBuffers, (GLsizei, const GLuint*)) \
    X(void, BindBuffer, (GLenum, GLuint)) \
    X(void, BufferData, (GLenum, std::ptrdiff_t, const void*, GLenum)) \
    X(void, EnableVertexAttribArray, (GLuint)) \
    X(void, VertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
    X(GLuint, CreateShader, (GLenum)) \
    X(void, ShaderSource, (GLuint, GLsizei, const char* const*, const GLint*)) \
    X(void, CompileShader, (GLuint)) \
    X(void, GetShaderiv, (GLuint, GLenum, GLint*)) \
    X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei*, char*)) \
    X(void, DeleteShader, (GLuint)) \
    X(GLuint, CreateProgram, ()) \
    X(void, AttachShader, (GLuint, GLuint)) \
    X(void, LinkProgram, (GLuint)) \
    X(void, GetProgramiv, (GLuint, GLenum, GLint*)) \
    X(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei*, char*)) \
    X(void, DeleteProgram, (GLuint)) \
    X(void, UseProgram, (GLuint)) \
    X(GLint, GetUniformLocation, (GLuint, const char*)) \
    X(void, UniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat*)) \
    X(void, Uniform3f, (GLint, GLfloat, GLfloat, GLfloat))
#define DECLARE(returnType, name, args) using name##Type = returnType (APIENTRY*) args; name##Type name = nullptr;
    GL_FUNCTIONS(DECLARE)
#undef DECLARE
    void load() {
#define LOAD(returnType, name, args) name = reinterpret_cast<name##Type>(extension("gl" #name)); if (!name) throw std::runtime_error("OpenGL function unavailable: gl" #name);
        GL_FUNCTIONS(LOAD)
#undef LOAD
    }
#undef GL_FUNCTIONS
};
void checkGL(const char* operation) {
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) { throw std::runtime_error(std::string(operation) + ": OpenGL error " + std::to_string(error)); }
}
struct Vertex { Vec3 position, normal, color; };
struct Mesh { GLuint vao = 0, vbo = 0; GLsizei count = 0; };
constexpr char vertexSource[] = R"glsl(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform vec3 uTint;
out vec3 normal;
out vec3 color;
void main() {
    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);
    normal = vec3(0.0);
    if (dot(aNormal, aNormal) > 0.0)
        normal = normalize(transpose(inverse(mat3(uModel))) * aNormal);
    color = aColor * uTint;
}
)glsl";
constexpr char fragmentSource[] = R"glsl(#version 330 core
in vec3 normal;
in vec3 color;
out vec4 fragmentColor;
void main() {
    float light = 1.0;
    if (dot(normal, normal) > 0.5)
        light = 0.25 + 0.75 * max(dot(normalize(normal), normalize(vec3(-0.5, 1.0, -0.7))), 0.0);
    fragmentColor = vec4(color * light, 1.0);
}
)glsl";
}

struct Renderer::Impl {
    HWND window = nullptr;
    HDC dc = nullptr;
    HGLRC context = nullptr;
    GLApi gl;
    GLuint program = 0, vertexShader = 0, fragmentShader = 0;
    GLint modelLocation = -1, viewProjectionLocation = -1, tintLocation = -1;
    Mesh cube, grid;
    unsigned int width = 0, height = 0;
    bool vsync = false;

    ~Impl() {
        if (context) {
            if (wglMakeCurrent(dc, context)) {
                if (cube.vbo) gl.DeleteBuffers(1, &cube.vbo);
                if (grid.vbo) gl.DeleteBuffers(1, &grid.vbo);
                if (cube.vao) gl.DeleteVertexArrays(1, &cube.vao);
                if (grid.vao) gl.DeleteVertexArrays(1, &grid.vao);
                if (program) gl.DeleteProgram(program);
                if (vertexShader) gl.DeleteShader(vertexShader);
                if (fragmentShader) gl.DeleteShader(fragmentShader);
                wglMakeCurrent(nullptr, nullptr);
            }
            wglDeleteContext(context);
        }
        if (dc) ReleaseDC(window, dc);
    }

    void initialize(HWND handle) {
        window = handle;
        dc = GetDC(window);
        if (!dc) throw std::runtime_error("Could not acquire the window device context.");
        PIXELFORMATDESCRIPTOR format{};
        format.nSize = sizeof(format);
        format.nVersion = 1;
        format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        format.iPixelType = PFD_TYPE_RGBA;
        format.cColorBits = 24;
        format.cDepthBits = 24;
        format.cStencilBits = 8;
        const int index = ChoosePixelFormat(dc, &format);
        if (!index || !SetPixelFormat(dc, index, &format)) throw std::runtime_error("No suitable OpenGL pixel format.");
        // Bootstrap solely to obtain WGL context creation; rendering uses a 3.3 core context.
        context = wglCreateContext(dc);
        if (!context || !wglMakeCurrent(dc, context)) throw std::runtime_error("Could not create the bootstrap OpenGL context.");
        using CreateContext = HGLRC (WINAPI*)(HDC, HGLRC, const int*);
        const auto create = reinterpret_cast<CreateContext>(extension("wglCreateContextAttribsARB"));
        if (!create) throw std::runtime_error("OpenGL 3.3 Core requires a graphics driver with WGL_ARB_create_context.");
        constexpr int attributes[] = {0x2091, 3, 0x2092, 3, 0x9126, 0x00000001, 0};
        const HGLRC modern = create(dc, nullptr, attributes);
        if (!modern) throw std::runtime_error("The graphics driver could not create an OpenGL 3.3 Core context.");
        if (!wglMakeCurrent(dc, modern)) {
            wglDeleteContext(modern);
            throw std::runtime_error("Could not activate the OpenGL 3.3 Core context.");
        }
        wglDeleteContext(context);
        context = modern;
        gl.load();
        GLint major = 0, minor = 0, profile = 0;
        glGetIntegerv(0x821B, &major); glGetIntegerv(0x821C, &minor); glGetIntegerv(0x9126, &profile);
        if (major < 3 || (major == 3 && minor < 3) || !(profile & 1))
            throw std::runtime_error("An OpenGL 3.3 or newer Core context is required.");
        OutputDebugStringA(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        using SwapInterval = BOOL (WINAPI*)(int);
        if (const auto swapInterval = reinterpret_cast<SwapInterval>(extension("wglSwapIntervalEXT")))
            vsync = swapInterval(1) != FALSE;

        const auto compile = [&](GLenum type, const char* source, GLuint& id) {
            id = gl.CreateShader(type);
            gl.ShaderSource(id, 1, &source, nullptr);
            gl.CompileShader(id);
            GLint success = 0;
            gl.GetShaderiv(id, compileStatus, &success);
            if (!success) {
                GLint length = 0; gl.GetShaderiv(id, infoLogLength, &length);
                std::string log(static_cast<size_t>(length > 0 ? length : 1), '\0');
                gl.GetShaderInfoLog(id, static_cast<GLsizei>(log.size()), nullptr, log.data());
                throw std::runtime_error("GLSL shader compilation failed: " + log);
            }
        };
        compile(vertexShaderType, vertexSource, vertexShader);
        compile(fragmentShaderType, fragmentSource, fragmentShader);
        program = gl.CreateProgram();
        gl.AttachShader(program, vertexShader); gl.AttachShader(program, fragmentShader);
        gl.LinkProgram(program);
        GLint linked = 0; gl.GetProgramiv(program, linkStatus, &linked);
        if (!linked) {
            GLint length = 0; gl.GetProgramiv(program, infoLogLength, &length);
            std::string log(static_cast<size_t>(length > 0 ? length : 1), '\0');
            gl.GetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
            throw std::runtime_error("GLSL program link failed: " + log);
        }
        gl.DeleteShader(vertexShader); vertexShader = 0;
        gl.DeleteShader(fragmentShader); fragmentShader = 0;
        modelLocation = gl.GetUniformLocation(program, "uModel");
        viewProjectionLocation = gl.GetUniformLocation(program, "uViewProjection");
        tintLocation = gl.GetUniformLocation(program, "uTint");
        if (modelLocation < 0 || viewProjectionLocation < 0 || tintLocation < 0)
            throw std::runtime_error("Required GLSL uniforms are missing.");

        std::vector<Vertex> vertices;
        const auto face = [&](Vec3 normal, std::array<Vec3, 4> corners) {
            // Counter-clockwise outward-facing triangles, as expected by GL_BACK culling.
            for (const int i : {0, 1, 2, 0, 2, 3}) vertices.push_back({corners[i], normal, {1,1,1}});
        };
        face({0,0,-1}, {{{-1,-1,-1}, {-1,1,-1}, {1,1,-1}, {1,-1,-1}}});
        face({0,0,1}, {{{1,-1,1}, {1,1,1}, {-1,1,1}, {-1,-1,1}}});
        face({-1,0,0}, {{{-1,-1,1}, {-1,1,1}, {-1,1,-1}, {-1,-1,-1}}});
        face({1,0,0}, {{{1,-1,-1}, {1,1,-1}, {1,1,1}, {1,-1,1}}});
        face({0,1,0}, {{{-1,1,-1}, {-1,1,1}, {1,1,1}, {1,1,-1}}});
        face({0,-1,0}, {{{-1,-1,1}, {-1,-1,-1}, {1,-1,-1}, {1,-1,1}}});
        upload(cube, vertices);
        vertices.clear();
        for (int line=-10; line<=10; ++line) {
            const float value = static_cast<float>(line);
            const Vec3 neutral{0.12f, 0.17f, 0.23f};
            const Vec3 xColor = line == 0 ? Vec3{0.50f,0.20f,0.20f} : neutral;
            const Vec3 zColor = line == 0 ? Vec3{0.20f,0.32f,0.55f} : neutral;
            vertices.push_back({{-10,-1.01f,value}, {}, xColor});
            vertices.push_back({{10,-1.01f,value}, {}, xColor});
            vertices.push_back({{value,-1.01f,-10}, {}, zColor});
            vertices.push_back({{value,-1.01f,10}, {}, zColor});
        }
        upload(grid, vertices);
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);
        glClearColor(0.025f, 0.040f, 0.065f, 1.0f);
        checkGL("OpenGL initialization");
    }

    void upload(Mesh& mesh, const std::vector<Vertex>& vertices) {
        mesh.count = static_cast<GLsizei>(vertices.size());
        gl.GenVertexArrays(1, &mesh.vao); gl.GenBuffers(1, &mesh.vbo);
        gl.BindVertexArray(mesh.vao); gl.BindBuffer(arrayBuffer, mesh.vbo);
        gl.BufferData(arrayBuffer, static_cast<std::ptrdiff_t>(vertices.size()*sizeof(Vertex)), vertices.data(), staticDraw);
        const std::array<size_t, 3> offsets{offsetof(Vertex, position), offsetof(Vertex, normal), offsetof(Vertex, color)};
        for (GLuint attribute=0; attribute<3; ++attribute) {
            gl.EnableVertexAttribArray(attribute);
            gl.VertexAttribPointer(attribute, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsets[attribute]));
        }
        gl.BindVertexArray(0);
    }
    void draw(const Mesh& mesh, GLenum primitive, const Mat4& world, Vec3 color) {
        gl.UniformMatrix4fv(modelLocation, 1, GL_FALSE, world.values.data());
        gl.Uniform3f(tintLocation, color.x, color.y, color.z);
        gl.BindVertexArray(mesh.vao);
        glDrawArrays(primitive, 0, mesh.count);
    }
};

Renderer::Renderer(HWND window, unsigned int width, unsigned int height) : impl_(std::make_unique<Impl>()) {
    impl_->initialize(window);
    resize(width, height);
}
Renderer::~Renderer() = default;
void Renderer::resize(unsigned int width, unsigned int height) {
    if (!width || !height) return;
    impl_->width = width; impl_->height = height;
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}
void Renderer::drawScene(const Camera& camera, const Scene& scene) {
    auto& renderer = *impl_;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer.gl.UseProgram(renderer.program);
    const Mat4 vp = camera.viewProjection(static_cast<float>(renderer.width)/static_cast<float>(renderer.height));
    renderer.gl.UniformMatrix4fv(renderer.viewProjectionLocation, 1, GL_FALSE, vp.values.data());
    renderer.draw(renderer.grid, GL_LINES, Mat4::identity(), {1,1,1});
    for (const auto& object : scene.cubes)
        renderer.draw(renderer.cube, GL_TRIANGLES, object.worldMatrix(), object.color);
    renderer.draw(renderer.cube, GL_TRIANGLES, scene.player.object.worldMatrix(), scene.player.object.color);
    renderer.gl.BindVertexArray(0);
    checkGL("Frame rendering");
}
void Renderer::present() {
    auto& renderer = *impl_;
    if (!SwapBuffers(renderer.dc)) throw std::runtime_error("OpenGL buffer swap failed.");
    if (!renderer.vsync) Sleep(1);
}
}

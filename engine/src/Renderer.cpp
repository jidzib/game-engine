#include "Renderer.hpp"
#include <gl/GL.h>
#include <array>
#include <algorithm>
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
constexpr GLenum framebuffer = 0x8D40, readFramebuffer = 0x8CA8, drawFramebuffer = 0x8CA9;
constexpr GLenum renderbuffer = 0x8D41, colorAttachment = 0x8CE0, depthAttachment = 0x8D00;
constexpr GLenum framebufferComplete = 0x8CD5, rgba8 = 0x8058, depth24 = 0x81A6;
constexpr GLenum maxRenderbufferSize = 0x84E8, clampToEdge = 0x812F, framebufferSrgb = 0x8DB9;

PROC extension(const char* name) {
    const PROC address = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::intptr_t>(address);
    return (!address || value == 1 || value == 2 || value == 3 || value == -1) ? nullptr : address;
}
// Windows exports GL 1.1 directly; load the required core functions from the current driver.
struct GLApi {
#define GL_FUNCTIONS(X) \
    X(void, GenFramebuffers, (GLsizei, GLuint*)) \
    X(void, DeleteFramebuffers, (GLsizei, const GLuint*)) \
    X(void, BindFramebuffer, (GLenum, GLuint)) \
    X(void, FramebufferTexture2D, (GLenum, GLenum, GLenum, GLuint, GLint)) \
    X(GLenum, CheckFramebufferStatus, (GLenum)) \
    X(void, GenRenderbuffers, (GLsizei, GLuint*)) \
    X(void, DeleteRenderbuffers, (GLsizei, const GLuint*)) \
    X(void, BindRenderbuffer, (GLenum, GLuint)) \
    X(void, RenderbufferStorage, (GLenum, GLenum, GLsizei, GLsizei)) \
    X(void, FramebufferRenderbuffer, (GLenum, GLenum, GLenum, GLuint)) \
    X(void, BlitFramebuffer, (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)) \
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
    struct Target {
        GLuint fbo = 0, color = 0, depth = 0;
        unsigned int width = 0, height = 0;
    } target;
    unsigned int maxWidth = 0, maxHeight = 0;
    unsigned int width = 0, height = 0;
    bool vsync = false;

    ~Impl() {
        if (context) {
            if (wglMakeCurrent(dc, context)) {
                release(target);
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
        GLint textureLimit = 0, depthLimit = 0, viewportLimits[2]{};
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &textureLimit);
        glGetIntegerv(maxRenderbufferSize, &depthLimit);
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportLimits);
        checkGL("Query scene target size limits");
        if (textureLimit <= 0 || depthLimit <= 0 || viewportLimits[0] <= 0 || viewportLimits[1] <= 0)
            throw std::runtime_error("OpenGL returned invalid scene target size limits.");
        maxWidth = static_cast<unsigned int>(std::min({textureLimit, depthLimit, viewportLimits[0]}));
        maxHeight = static_cast<unsigned int>(std::min({textureLimit, depthLimit, viewportLimits[1]}));
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

    void release(Target& owned) noexcept {
        if (owned.fbo) gl.DeleteFramebuffers(1, &owned.fbo);
        if (owned.depth) gl.DeleteRenderbuffers(1, &owned.depth);
        if (owned.color) glDeleteTextures(1, &owned.color);
        owned = {};
    }

    void resizeTarget(unsigned int newWidth, unsigned int newHeight) {
        if (!newWidth || !newHeight) {
            width = newWidth; height = newHeight;
            return; // Keep storage for restore, but hide it from sceneTexture().
        }
        if (newWidth > maxWidth || newHeight > maxHeight)
            throw std::runtime_error("Scene target " + std::to_string(newWidth) + "x" + std::to_string(newHeight)
                + " exceeds OpenGL limits " + std::to_string(maxWidth) + "x" + std::to_string(maxHeight));
        if (target.width != newWidth || target.height != newHeight) {
            // Commit only a complete target. A failed resize keeps the old allocation valid.
            Target next;
            try {
                checkGL("Before scene target allocation");
                gl.GenFramebuffers(1, &next.fbo);
                glGenTextures(1, &next.color);
                gl.GenRenderbuffers(1, &next.depth);
                checkGL("Create scene target objects");
                if (!next.fbo || !next.color || !next.depth)
                    throw std::runtime_error("OpenGL returned an empty scene target object.");
                glBindTexture(GL_TEXTURE_2D, next.color);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);
                glTexImage2D(GL_TEXTURE_2D, 0, rgba8, static_cast<GLsizei>(newWidth),
                    static_cast<GLsizei>(newHeight), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                checkGL("Allocate scene color texture (RGBA8)");
                gl.BindRenderbuffer(renderbuffer, next.depth);
                gl.RenderbufferStorage(renderbuffer, depth24, static_cast<GLsizei>(newWidth), static_cast<GLsizei>(newHeight));
                checkGL("Allocate scene depth renderbuffer (DEPTH_COMPONENT24)");
                gl.BindFramebuffer(framebuffer, next.fbo);
                gl.FramebufferTexture2D(framebuffer, colorAttachment, GL_TEXTURE_2D, next.color, 0);
                gl.FramebufferRenderbuffer(framebuffer, depthAttachment, renderbuffer, next.depth);
                glDrawBuffer(colorAttachment);
                glReadBuffer(colorAttachment);
                const GLenum status = gl.CheckFramebufferStatus(framebuffer);
                checkGL("Attach scene framebuffer storage");
                if (status != framebufferComplete)
                    throw std::runtime_error("Scene framebuffer incomplete; OpenGL status " + std::to_string(status));
                next.width = newWidth; next.height = newHeight;
            } catch (const std::exception& error) {
                gl.BindFramebuffer(framebuffer, 0);
                gl.BindRenderbuffer(renderbuffer, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
                release(next);
                throw std::runtime_error("Scene target allocation " + std::to_string(newWidth) + "x"
                    + std::to_string(newHeight) + " failed: " + error.what());
            }
            gl.BindFramebuffer(framebuffer, 0);
            gl.BindRenderbuffer(renderbuffer, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            release(target);
            target = next;
        }
        width = newWidth; height = newHeight;
    }

    void windowState() {
        gl.BindFramebuffer(framebuffer, 0);
        glDrawBuffer(GL_BACK); glReadBuffer(GL_BACK);
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        gl.UseProgram(0); gl.BindVertexArray(0);
        glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST); glDisable(GL_STENCIL_TEST); glDisable(framebufferSrgb);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); glDepthMask(GL_TRUE);
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
    impl_->resizeTarget(width, height);
}
void Renderer::drawScene(const Camera& camera, const Scene& scene) {
    auto& renderer = *impl_;
    if (!renderer.width || !renderer.height) return;
    renderer.gl.BindFramebuffer(framebuffer, renderer.target.fbo);
    glDrawBuffer(colorAttachment);
    glViewport(0, 0, static_cast<GLsizei>(renderer.target.width), static_cast<GLsizei>(renderer.target.height));
    glDisable(GL_SCISSOR_TEST); glDisable(GL_BLEND); glDisable(GL_STENCIL_TEST); glDisable(framebufferSrgb);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glClearDepth(1.0); glDepthRange(0.0, 1.0);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.025f, 0.040f, 0.065f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderer.gl.UseProgram(renderer.program);
    const Mat4 vp = camera.viewProjection(static_cast<float>(renderer.target.width)/static_cast<float>(renderer.target.height));
    renderer.gl.UniformMatrix4fv(renderer.viewProjectionLocation, 1, GL_FALSE, vp.values.data());
    renderer.draw(renderer.grid, GL_LINES, Mat4::identity(), {1,1,1});
    for (const auto& object : scene.cubes)
        renderer.draw(renderer.cube, GL_TRIANGLES, object.worldMatrix(), object.color);
    renderer.draw(renderer.cube, GL_TRIANGLES, scene.player.object.worldMatrix(), scene.player.object.color);
    renderer.windowState();
    checkGL("Frame rendering");
}
SceneTexture Renderer::sceneTexture() const noexcept {
    const auto& renderer = *impl_;
    if (!renderer.width || !renderer.height) return {};
    return {renderer.target.color, renderer.target.width, renderer.target.height};
}
void Renderer::blitSceneToWindow() {
    auto& renderer = *impl_;
    if (!renderer.width || !renderer.height) return;
    renderer.windowState();
    renderer.gl.BindFramebuffer(readFramebuffer, renderer.target.fbo);
    glReadBuffer(colorAttachment);
    renderer.gl.BindFramebuffer(drawFramebuffer, 0);
    renderer.gl.BlitFramebuffer(0, 0, static_cast<GLint>(renderer.target.width), static_cast<GLint>(renderer.target.height),
        0, 0, static_cast<GLint>(renderer.width), static_cast<GLint>(renderer.height), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    renderer.windowState();
    checkGL("Scene preview blit");
}
void Renderer::present() {
    auto& renderer = *impl_;
    if (!renderer.width || !renderer.height) return;
    if (!SwapBuffers(renderer.dc)) throw std::runtime_error("OpenGL buffer swap failed.");
    if (!renderer.vsync) Sleep(1);
}
}

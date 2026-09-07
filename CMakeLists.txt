cmake_minimum_required(VERSION 3.25)
project(GameEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(NOT WIN32)
    message(FATAL_ERROR "The current window/context layer uses Windows WGL. The renderer requires OpenGL 3.3 Core.")
endif()

add_library(engine STATIC
    engine/src/Collision.cpp
    engine/src/Engine.cpp
    engine/src/Camera.cpp
    engine/src/Window.cpp
    engine/src/Renderer.cpp
)
add_library(GameEngine::Engine ALIAS engine)
target_include_directories(engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/engine/include")
target_compile_features(engine PUBLIC cxx_std_20)
target_compile_definitions(engine PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
find_package(OpenGL REQUIRED)
target_link_libraries(engine PRIVATE OpenGL::GL user32 gdi32)

add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC third_party/imgui third_party/imgui/backends)
target_compile_features(imgui PUBLIC cxx_std_20)
target_compile_definitions(imgui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(imgui PRIVATE OpenGL::GL user32 gdi32 dwmapi)
add_library(editor STATIC editor/Editor.cpp)
target_include_directories(editor PUBLIC editor PRIVATE engine/include engine/src)
target_compile_features(editor PUBLIC cxx_std_20)
target_compile_definitions(editor PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(editor PRIVATE imgui OpenGL::GL)
target_link_libraries(engine PRIVATE editor)

add_executable(sandbox WIN32 sandbox/main.cpp)
target_link_libraries(sandbox PRIVATE GameEngine::Engine)

foreach(target IN ITEMS engine editor sandbox)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT sandbox)

include(CTest)
if(BUILD_TESTING)
    add_executable(scene_tests tests/SceneTests.cpp)
    target_link_libraries(scene_tests PRIVATE GameEngine::Engine)
    add_test(NAME scene_behavior COMMAND scene_tests)
    add_executable(collision_tests tests/CollisionTests.cpp)
    target_link_libraries(collision_tests PRIVATE GameEngine::Engine)
    add_test(NAME collision_behavior COMMAND collision_tests)
    add_test(NAME render_smoke COMMAND sandbox --smoke-test)
    set_tests_properties(render_smoke PROPERTIES TIMEOUT 20)
    add_executable(renderer_tests tests/RendererTests.cpp)
    target_include_directories(renderer_tests PRIVATE engine/src)
    target_link_libraries(renderer_tests PRIVATE GameEngine::Engine OpenGL::GL)
    add_test(NAME renderer_target COMMAND renderer_tests)
    set_tests_properties(renderer_target PROPERTIES TIMEOUT 20)
    add_executable(navigation_tests tests/NavigationTests.cpp)
    target_include_directories(navigation_tests PRIVATE editor)
    target_link_libraries(navigation_tests PRIVATE GameEngine::Engine)
    add_test(NAME editor_navigation COMMAND navigation_tests)
    add_executable(editor_tests tests/EditorTests.cpp)
    target_include_directories(editor_tests PRIVATE engine/src)
    target_link_libraries(editor_tests PRIVATE GameEngine::Engine editor imgui OpenGL::GL)
    add_test(NAME editor_interface COMMAND editor_tests)
    set_tests_properties(editor_interface PROPERTIES TIMEOUT 20)
endif()

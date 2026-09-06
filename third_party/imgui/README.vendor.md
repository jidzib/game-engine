# Dear ImGui

Vendored unmodified from ocornut/imgui v1.91.9b, immutable commit
`f5befd2d29e66809cd1110a152e375a7f1981f06`.

Source archive:
https://codeload.github.com/ocornut/imgui/zip/f5befd2d29e66809cd1110a152e375a7f1981f06

Archive SHA-256:
`e17c195c7b98a5edf5be1d034114a3920beb3c182e49384ed04ef7fab35fa702`

Acquisition: download that archive, verify SHA-256, extract, copy root *.h,
*.cpp and LICENSE.txt, plus backends/imgui_impl_win32.{h,cpp},
backends/imgui_impl_opengl3.{h,cpp} and imgui_impl_opengl3_loader.h.
The upstream demo source is retained but is not compiled.
MIT license is in LICENSE.txt. No network or package manager is needed to build.
The root CMakeLists.txt builds four core sources and the two backends as the
private imgui dependency of editor, using the bundled OpenGL loader.

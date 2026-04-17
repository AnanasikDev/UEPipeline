add_executable(editor
    src/main.cpp
    src/app.cpp
    include/app.h)

target_link_options(editor PRIVATE "/SUBSYSTEM:WINDOWS")
target_include_directories(editor PRIVATE include)
set_target_properties(editor PROPERTIES FOLDER "Apps")

# GLAD (compiled)
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/external/glad/src/gl.c)

target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/external/glad/include
)

# stb_image (header-only)
add_library(stb_image INTERFACE)

target_include_directories(stb_image INTERFACE
    ${CMAKE_SOURCE_DIR}/external/stb_image
)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# ImGui 
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# ImGui wrapper
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC glfw)

target_link_libraries(editor
    PRIVATE
        glfw
        glad
        imgui_lib
        stb_image
)
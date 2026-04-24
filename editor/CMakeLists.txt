file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
)

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    include/*.h
)

add_executable(editor
    ${SRC_FILES}
    ${HEADER_FILES}
 "include/paths.h" "include/config.h")
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

# json (header-only)
add_library(json INTERFACE)

target_include_directories(json INTERFACE
    ${CMAKE_SOURCE_DIR}/external/json
)

include(FetchContent)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    URL      https://github.com/glfw/glfw/archive/refs/tags/3.3.8.tar.gz
)
FetchContent_MakeAvailable(glfw)

# ImGui 
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/heads/docking.zip
)
FetchContent_MakeAvailable(imgui)
file(REMOVE_RECURSE
    ${imgui_SOURCE_DIR}/examples
    ${imgui_SOURCE_DIR}/docs
    ${imgui_SOURCE_DIR}/misc
    ${imgui_SOURCE_DIR}/.github
    ${imgui_SOURCE_DIR}/.editorconfig
)

# Remove all backends except the two we use
file(GLOB _all_backends ${imgui_SOURCE_DIR}/backends/*)
set(_keep
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.h
)
list(REMOVE_ITEM _all_backends ${_keep})
file(REMOVE ${_all_backends})

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
        json
        imgui_lib
        stb_image
)

add_custom_command(TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/out/ship
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:editor> ${CMAKE_SOURCE_DIR}/out/ship/editor.exe
    COMMENT "Shipping: copied editor.exe"
)
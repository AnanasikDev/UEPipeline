cmake_minimum_required(VERSION 3.31)
project(UIPipeline LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This makes the sub-projects appear as folders in VS
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CTest) # Enables the 'ctest' command and VS Test Explorer integration

if (MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

add_subdirectory(editor)
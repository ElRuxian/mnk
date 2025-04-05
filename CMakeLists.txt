cmake_minimum_required(VERSION 3.14)
project(ExtendedMNKGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
    add_compile_options(-Wno-changes-meaning)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)

include(FetchContent)

add_definitions(-DASIO_STANDALONE)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/asio-src/asio/include")
include_directories(${ASIO_INCLUDE_DIR})

set(BUILD_SHARED_LIBS OFF)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -pthread")

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG master
)
FetchContent_MakeAvailable(sfml)
set(SFML_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/_deps/sfml-src/include)
include_directories(${SFML_INCLUDE_DIRS})
add_definitions(-DSFML_STATIC)
set(SFML_DIR "${CMAKE_BINARY_DIR}/_deps/sfml-src/lib/cmake/SFML")


FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR "${CMAKE_BINARY_DIR}/_deps/imgui-src")
include_directories(${IMGUI_DIR})
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
)

FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/eliasdaler/imgui-sfml.git
    GIT_TAG master
)
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
FetchContent_MakeAvailable(imgui-sfml)
include_directories(${CMAKE_BINARY_DIR}/_deps/imgui-sfml-src)

add_library(imgui-sfml STATIC 
    ${CMAKE_BINARY_DIR}/_deps/imgui-sfml-src/imgui-SFML.cpp
)
include_directories(src/mnkg)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(mnkg STATIC ${SOURCES})

target_link_libraries(mnkg PRIVATE sfml-graphics imgui imgui-sfml OpenGL::GL)

foreach(source_file ${SOURCES})
    file(READ ${source_file} CONTENTS)
    if (CONTENTS MATCHES "main\\s*\\(")
        get_filename_component(name ${source_file} NAME_WE)
        add_executable(${name} ${source_file})
        target_link_libraries(${name} PRIVATE mnkg sfml-graphics imgui imgui-sfml)
    endif()
endforeach()

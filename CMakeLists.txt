cmake_minimum_required(VERSION 3.14)
project(ExtendedMNKGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-changes-meaning)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.1.2
)
FetchContent_MakeAvailable(crow)
include_directories(${crow_SOURCE_DIR}/include)

include_directories(src/mnkg)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(mnkg STATIC ${SOURCES})

foreach(source_file ${SOURCES})
    file(READ ${source_file} CONTENTS)
    if (CONTENTS MATCHES "main\\s*\\(")
        get_filename_component(name ${source_file} NAME_WE)
        add_executable(${name} ${source_file})
        target_link_libraries(${name} PRIVATE mnkg)
    endif()
endforeach()

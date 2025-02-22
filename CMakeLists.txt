cmake_minimum_required(VERSION 3.10)
project(ExtendedMNKGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-changes-meaning)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(src)

file(GLOB_RECURSE SOURCES src/*.cpp)

set(EXECUTABLES "")
foreach(source_file ${SOURCES})
    file(READ ${source_file} CONTENTS)
    if (CONTENTS MATCHES "main\\s*\\(")
        get_filename_component(name ${source_file} NAME_WE)
        add_executable(${name} ${source_file})
        list(APPEND EXECUTABLES ${name})
    endif()
endforeach()

message("Generated sources: ${SOURCES}")
message("Generated executables: ${EXECUTABLES}")

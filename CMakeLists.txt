cmake_minimum_required(VERSION 3.10)
project(ExtendedMNKGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wno-changes-meaning)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 2.5 COMPONENTS system window graphics network audio REQUIRED)

include_directories(src/mnkg)

file(GLOB_RECURSE SOURCES src/*.cpp)

set(EXECUTABLES "")
foreach(source_file ${SOURCES})
    file(READ ${source_file} CONTENTS)
    if (CONTENTS MATCHES "main\\s*\\(")
        get_filename_component(name ${source_file} NAME_WE)
        add_executable(${name} ${source_file})
        if (CONTENTS MATCHES "#include <SFML/")
            target_link_libraries(${name} PRIVATE sfml-system sfml-window sfml-graphics sfml-audio sfml-network)
        endif()
        list(APPEND EXECUTABLES ${name})
    endif()
endforeach()

message("Generated sources: ${SOURCES}")
message("Generated executables: ${EXECUTABLES}")

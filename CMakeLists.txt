cmake_minimum_required(VERSION 3.10)
project(ExtendedMnkGame)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(.)

file(GLOB_RECURSE SOURCES *.cpp)

foreach(source_file ${SOURCES})
    get_filename_component(name ${source_file} NAME_WE)
    add_executable(${name} ${source_file})
endforeach()


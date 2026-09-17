cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Tsubaki
VERSION 0.0.2
DESCRIPTION
"A checksum utility for file integrity, comparison, and duplicate detection."
LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(tsubaki main.cc)

# add_subdirectory(plugin_core)
# target_link_libraries(tsubaki PRIVATE plugin_core)

add_subdirectory(plugins)

add_subdirectory(core)

target_link_libraries(tsubaki PRIVATE core)

target_include_directories(tsubaki PRIVATE
"${CMAKE_SOURCE_DIR}/core"
"${CMAKE_SOURCE_DIR}/plugins")



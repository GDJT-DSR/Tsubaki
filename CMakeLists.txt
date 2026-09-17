cmake_minimum_required(VERSION 3.16)

project(Tsubaki
VERSION 0.0.3
DESCRIPTION
"A checksum utility for file integrity, comparison, and duplicate detection."
LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(TSUBAKI_SANITIZE "Build with AddressSanitizer/UBSan" OFF)
if(TSUBAKI_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(plugins)

add_subdirectory(core)

add_executable(tsubaki main.cc)

target_link_libraries(tsubaki PRIVATE core)

target_include_directories(tsubaki PRIVATE
"${CMAKE_SOURCE_DIR}/core"
"${CMAKE_SOURCE_DIR}/plugins")

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

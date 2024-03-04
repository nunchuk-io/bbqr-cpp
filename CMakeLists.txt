cmake_minimum_required(VERSION 3.16)

project(bbqr-cpp LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BBQR_BUILD_EXAMPLES "Build examples" ON)

set(${PROJECT_NAME}_HEADERS include/bbqr/bbqr.hpp)
set(${PROJECT_NAME}_SOURCES src/bbqr.cpp src/utils.cpp src/strencoding.hpp)

set(ZLIB_BUILD_EXAMPLES OFF)
add_subdirectory(contrib/zlib)

add_library(${PROJECT_NAME} ${${PROJECT_NAME}_HEADERS} ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)

if (BBQR_BUILD_EXAMPLES)
    add_executable(join examples/join.cpp)
    target_link_libraries(join ${PROJECT_NAME})

    add_executable(split examples/split.cpp)
    target_link_libraries(split ${PROJECT_NAME})
endif ()

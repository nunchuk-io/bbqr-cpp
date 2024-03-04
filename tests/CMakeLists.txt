cmake_minimum_required(VERSION 3.25)

project(bbqr-cpp-test LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
add_library(test_main OBJECT unit.cpp)

set(BBQR_BUILD_EXAMPLES OFF)
add_subdirectory(.. bbqr-cpp)

target_include_directories(test_main PUBLIC ${PROJECT_SOURCE_DIR}/contrib/doctest)
set(files test_encoding.cpp test_decoding.cpp test_loopback.cpp)

file(COPY test_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
foreach (file ${files})
    get_filename_component(testcase ${file} NAME_WE)
    add_executable(${testcase} ${file} $<TARGET_OBJECTS:test_main>)
    target_link_libraries(${testcase} bbqr-cpp)
    target_include_directories(${testcase} PUBLIC "${PROJECT_SOURCE_DIR}/contrib/doctest")

    add_test(NAME ${testcase} COMMAND ${testcase})
endforeach ()


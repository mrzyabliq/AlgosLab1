cmake_minimum_required(VERSION 3.16)
project(MalyshevLab1 LANGUAGES CXX)

add_library(shortest_paths INTERFACE)
target_include_directories(shortest_paths INTERFACE headers)
target_compile_features(shortest_paths INTERFACE cxx_std_17)
if(MSVC)
    target_compile_options(shortest_paths INTERFACE /W4 /utf-8)
else()
    target_compile_options(shortest_paths INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(dijkstra src/main.cpp)
target_link_libraries(dijkstra PRIVATE shortest_paths)

option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_executable(dijkstra_tests tests/DijkstraTests.cpp)
    target_link_libraries(dijkstra_tests PRIVATE shortest_paths)
    add_test(NAME dijkstra_tests COMMAND dijkstra_tests)
    add_test(NAME dijkstra_graph_input_test COMMAND ${CMAKE_COMMAND}
        "-DPROGRAM=$<TARGET_FILE:dijkstra>"
        "-DEXAMPLES_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}/examples"
        "-DTEST_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/graph_input_test"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/GraphInputTest.cmake")
    add_test(NAME dijkstra_benchmark_test COMMAND ${CMAKE_COMMAND}
        "-DPROGRAM=$<TARGET_FILE:dijkstra>"
        "-DTEST_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/benchmark_test"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/BenchmarkTest.cmake")
endif()

cmake_minimum_required(VERSION 3.7.1)
project(seastar-starter VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(seastar)

add_executable(main main.cc)
target_link_libraries(main Seastar::seastar)

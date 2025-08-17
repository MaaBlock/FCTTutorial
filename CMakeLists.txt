cmake_minimum_required(VERSION 3.22.1)
include(config/config_vcpkg.cmake)
project(fct_tutorial)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(FCT)
add_subdirectory(tutorial)
cmake_minimum_required(VERSION 3.9)
project(pizza_example_problem)

set(CMAKE_CXX_STANDARD 17)

add_executable(pizza_example_problem main.cpp pcg_random.hpp pcg_extras.hpp pcg_uint128.hpp)
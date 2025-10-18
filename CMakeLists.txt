cmake_minimum_required(VERSION 4.0)
project(ecs_net C)

set(CMAKE_C_STANDARD 11)

add_library(ecs_net STATIC flecs/flecs.c flecs-net.c)
target_include_directories(ecs_net PUBLIC ./)

enable_testing()
add_subdirectory(test)
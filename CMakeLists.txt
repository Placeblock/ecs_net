include(FetchContent)

cmake_minimum_required(VERSION 4.0)
project(ecs_net CXX)

set(CMAKE_CXX_STANDARD 20)

set(SKIP_PERFORMANCE_COMPARISON ON)
FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal
        GIT_TAG v1.3.2
)
FetchContent_MakeAvailable(cereal)


add_subdirectory(lib/ecs_history)

add_library(ecs_net
        include/ecs_net/change_serialization.hpp
        include/ecs_net/component_serialization.hpp
        include/ecs_net/serialization.hpp
        include/ecs_net/entity_version.hpp
        include/ecs_net/commit.hpp
        include/ecs_net/registry.hpp
        src/entity_version.cpp
)
target_link_libraries(ecs_net cereal ecs_history)
target_include_directories(ecs_net PUBLIC include)

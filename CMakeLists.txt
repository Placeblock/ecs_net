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
set(CPPZMQ_BUILD_TESTS OFF)
set(CPPZMQ_BUILD_EXAMPLES OFF)
option(CPPZMQ_BUILD_TESTS OFF)
FetchContent_Declare(
        cppzmq
        GIT_REPOSITORY https://github.com/zeromq/cppzmq
        GIT_TAG v4.11.0
)
FetchContent_MakeAvailable(cppzmq cereal)


add_subdirectory(lib/ecs_history)

add_executable(ecs_net ecs-net.cpp
        include/ecs_net/change_serialization.hpp
        include/ecs_net/component_serialization.hpp
        include/ecs_net/serialization.hpp
        include/ecs_net/entity_version.hpp
        src/entity_version.cpp
        include/ecs_net/registry.hpp
)
target_link_libraries(ecs_net cppzmq cereal ecs-history)
target_include_directories(ecs_net PUBLIC include)

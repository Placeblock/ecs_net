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
FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt
        GIT_TAG v3.15.0
)
set(CPPZMQ_BUILD_TESTS OFF)
set(CPPZMQ_BUILD_EXAMPLES OFF)
FetchContent_Declare(
        cppzmq
        GIT_REPOSITORY https://github.com/zeromq/cppzmq
        GIT_TAG v4.11.0
)
FetchContent_MakeAvailable(EnTT cppzmq cereal)

add_subdirectory(lib/ecs_history)

add_executable(ecs_net ecs-net.cpp
        include/change_serialization.hpp
        include/component_serializer.hpp
        include/component_traits.hpp)
target_link_libraries(ecs_net EnTT::EnTT cppzmq cereal ecs-history)
target_include_directories(ecs_net PUBLIC include)

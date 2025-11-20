include(FetchContent)

cmake_minimum_required(VERSION 4.0)
project(ecs_net CXX)

set(CMAKE_CXX_STANDARD  17)

FetchContent_Declare(
        Bitsery
        GIT_REPOSITORY      https://github.com/fraillt/bitsery
        GIT_TAG             v5.2.5
)
FetchContent_Declare(
        EnTT
        GIT_REPOSITORY      https://github.com/skypjack/entt
        GIT_TAG             v3.15.0
)
set(CPPZMQ_BUILD_TESTS OFF)
set(CPPZMQ_BUILD_EXAMPLES OFF)
FetchContent_Declare(
        cppzmq
        GIT_REPOSITORY      https://github.com/zeromq/cppzmq
        GIT_TAG             v4.11.0
)
FetchContent_MakeAvailable(EnTT cppzmq Bitsery)


add_executable(ecs_net ecs-net.cpp)
target_link_libraries(ecs_net EnTT::EnTT cppzmq bitsery)
target_include_directories(ecs_net PUBLIC include)

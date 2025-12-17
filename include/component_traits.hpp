//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_COMPONENT_TRAITS_HPP
#define ECS_NET_COMPONENT_TRAITS_HPP
#include <cstdint>

namespace ecsnet {
    enum class traits_t : uint16_t {
        NO = 0x00,
        TRIVIAL = 0x01,

        _entt_enum_as_bitmask
    };
}

#endif //ECS_NET_COMPONENT_TRAITS_HPP

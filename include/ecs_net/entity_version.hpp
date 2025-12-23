//
// Created by felix on 12/20/25.
//

#ifndef ECS_NET_ENTITY_VERSION_HPP
#define ECS_NET_ENTITY_VERSION_HPP

#include <cstdint>
#include <unordered_map>

#include "ecs_history/static_entity.hpp"

#ifndef ENTITY_VERSION_TYPE
#define ENTITY_VERSION_TYPE uint16_t
#endif

namespace ecs_net {
    using entity_version_t = ENTITY_VERSION_TYPE;

    class entity_version_handler_t {
        std::unordered_map<ecs_history::static_entity_t, entity_version_t> versions;

    public:
        [[nodiscard]] entity_version_t get_version(ecs_history::static_entity_t entity);
        void set_version(ecs_history::static_entity_t entity, entity_version_t version);
        entity_version_t increment_version(ecs_history::static_entity_t entity);
        void remove_entity(ecs_history::static_entity_t entity);
        void add_entity(ecs_history::static_entity_t entity, entity_version_t version);
    };
}


#endif //ECS_NET_ENTITY_VERSION_HPP
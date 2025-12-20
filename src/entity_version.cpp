//
// Created by felix on 12/20/25.
//

#include "ecs_net/entity_version.hpp"

ecs_net::entity_version_t ecs_net::entity_version_handler_t::get_version(const ecs_history::static_entity_t entity) const {
    return this->versions.at(entity);
}

/**
 * @param entity The entity for which the version should get incremented
 * @return The version of the entity BEFORE incrementing.
 */
ecs_net::entity_version_t ecs_net::entity_version_handler_t::increase_version(const ecs_history::static_entity_t entity) {
    return this->versions.at(entity)++;
}

void ecs_net::entity_version_handler_t::remove_entity(const ecs_history::static_entity_t entity) {
    this->versions.erase(entity);
}

//
// Created by felix on 12/20/25.
//

#include "ecs_net/entity_version.hpp"

ecs_net::entity_version_t ecs_net::entity_version_handler_t::get_version(const ecs_history::static_entity_t entity) {
    return this->versions[entity];
}

void ecs_net::entity_version_handler_t::set_version(const ecs_history::static_entity_t entity,
    const entity_version_t version) {
    this->versions[entity] = version;
}
ecs_net::entity_version_t ecs_net::entity_version_handler_t::increment_version(const ecs_history::static_entity_t entity) {
    if (!this->versions.contains(entity)) {
        throw std::runtime_error("entity does not exist in version handler");
    }
    return this->versions.at(entity)++;
}

void ecs_net::entity_version_handler_t::remove_entity(const ecs_history::static_entity_t entity) {
    if (!this->versions.contains(entity)) {
        throw std::runtime_error("entity does not exist in version handler");
    }
    this->versions.erase(entity);
}

void ecs_net::entity_version_handler_t::
add_entity(const ecs_history::static_entity_t entity, const entity_version_t version) {
    this->versions[entity] = version;
}

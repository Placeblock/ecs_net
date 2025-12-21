//
// Created by felix on 12/20/25.
//

#ifndef ECS_HISTORY_COMMIT_HPP
#define ECS_HISTORY_COMMIT_HPP
#include <vector>

#include "ecs_history/change_set.hpp"
#include "ecs_net/entity_version.hpp"

struct commit_t {
    std::unordered_map<ecs_history::static_entity_t, ecs_net::entity_version_t> entity_versions;
    std::vector<ecs_history::static_entity_t> created_entities;
    std::vector<std::unique_ptr<ecs_history::base_component_change_set_t>> component_change_sets;
    std::vector<ecs_history::static_entity_t> deleted_entities;
};

#endif //ECS_HISTORY_COMMIT_HPP
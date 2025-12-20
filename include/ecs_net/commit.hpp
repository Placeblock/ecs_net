//
// Created by felix on 12/20/25.
//

#ifndef ECS_HISTORY_COMMIT_HPP
#define ECS_HISTORY_COMMIT_HPP
#include <vector>

#include "../../lib/ecs_history/include/ecs_history/change_set.hpp"
#include "ecs_net/entity_version.hpp"

struct commit_t {
    std::vector<std::unique_ptr<ecs_history::base_component_change_set_t>> component_change_sets;
    std::unordered_map<ecs_history::static_entity_t, ecs_net::entity_version_t> entity_versions;
};

#endif //ECS_HISTORY_COMMIT_HPP
//
// Created by felix on 12/20/25.
//

#ifndef ECS_HISTORY_COMMIT_HPP
#define ECS_HISTORY_COMMIT_HPP
#include <utility>
#include <vector>

#include "ecs_history/change_set.hpp"
#include "ecs_net/entity_version.hpp"

namespace ecs_net {
    struct commit_t {
        bool undo = false;
        std::unordered_map<ecs_history::static_entity_t, entity_version_t> entity_versions;
        std::vector<ecs_history::static_entity_t> created_entities;
        std::vector<std::unique_ptr<ecs_history::base_change_set_t>> change_sets;
        std::vector<ecs_history::static_entity_t> destroyed_entities;

        commit_t() = default;
        commit_t(std::unordered_map<ecs_history::static_entity_t, entity_version_t> entity_versions,
            std::vector<ecs_history::static_entity_t> created_entities,
            std::vector<std::unique_ptr<ecs_history::base_change_set_t>> change_sets,
            std::vector<ecs_history::static_entity_t> destroyed_entities)
                : entity_versions(std::move(entity_versions)),
                created_entities(std::move(created_entities)),
                change_sets(std::move(change_sets)),
                destroyed_entities(std::move(destroyed_entities)) {
        }

        commit_t(commit_t &commit) = delete;
        commit_t &operator=(commit_t &commit) = delete;
    };


    inline std::unique_ptr<commit_t> commit_changes(entt::registry &reg) {
        const auto& static_entities = reg.ctx().get<ecs_history::static_entities_t>();
        auto& version_handler = reg.ctx().get<entity_version_handler_t>();

        auto commit = std::make_unique<commit_t>();
        const auto &monitors = reg.ctx().get<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >();
        for (auto &monitor: monitors) {
            commit->change_sets.emplace_back(monitor->commit());
            monitor->clear();
        }

        auto& created_entities = reg.storage<entt::reactive>("entity_created_storage"_hs);
        commit->created_entities.reserve(created_entities.size());
        for (const auto& created_entity : created_entities) {
            ecs_history::static_entity_t static_entity = static_entities.get_static_entity(created_entity);
            commit->created_entities.emplace_back(static_entity);
        }
        created_entities.clear();
        auto& destroyed_entities = reg.storage<entt::reactive>("entity_destroyed_storage"_hs);
        commit->destroyed_entities.reserve(created_entities.size());
        for (const auto& destroyed_entity : destroyed_entities) {
            ecs_history::static_entity_t static_entity = static_entities.get_static_entity(destroyed_entity);
            commit->destroyed_entities.emplace_back(static_entity);
        }
        destroyed_entities.clear();


        std::unordered_set<ecs_history::static_entity_t> commit_entities;
        for (const auto& change_set : commit->change_sets) {
            change_set->for_entity([&commit_entities](const ecs_history::static_entity_t& static_entity) {
                commit_entities.emplace(static_entity);
            });
        }
        for (const ecs_history::static_entity_t& static_entity : commit->created_entities) {
            commit_entities.emplace(static_entity);
        }
        for (const ecs_history::static_entity_t& static_entity : commit->destroyed_entities) {
            commit_entities.emplace(static_entity);
        }
        for (const ecs_history::static_entity_t& static_entity : commit_entities) {
            commit->entity_versions[static_entity] = version_handler.increase_version(static_entity);
        }

        return std::move(commit);
    }

    inline bool can_apply(entt::registry &reg, const commit_t &commit) {
        auto& version_handler = reg.ctx().get<entity_version_handler_t>();

        return std::ranges::all_of(commit.entity_versions, [&](const auto &pair) {
            return pair.second == version_handler.get_version(pair.first);
        });
    }

    inline void apply_commit(entt::registry &reg, const commit_t &commit) {
        auto& static_entities = reg.ctx().get<ecs_history::static_entities_t>();
        auto& version_handler = reg.ctx().get<entity_version_handler_t>();

        for (const ecs_history::static_entity_t& created_entity : commit.created_entities) {
            const entt::entity entt = reg.create();
            static_entities.create(entt, created_entity);
        }
        ecs_history::any_change_applier_t applier{reg, static_entities};
        for (const auto& change : commit.change_sets) {
            change->supply(applier);
        }
        for (const ecs_history::static_entity_t& removed_entity : commit.destroyed_entities) {
            const auto entt = static_entities.remove(removed_entity);
            reg.destroy(entt);
        }
        for (const auto &static_entity: commit.entity_versions | std::views::keys) {
            commit.undo ? version_handler.decrease_version(static_entity) : version_handler.increase_version(static_entity);
        }
    }
}

#endif //ECS_HISTORY_COMMIT_HPP
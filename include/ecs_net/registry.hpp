//
// Created by felix on 12/23/25.
//

#ifndef ECS_NET_REGISTRY_HPP
#define ECS_NET_REGISTRY_HPP
#include "commit.hpp"
#include "ecs_history/change_applier.hpp"
#include "ecs_history/gather_strategy/registry.hpp"

namespace ecs_net {
    class registry_t : public ecs_history::registry_t {
        entity_version_handler_t &version_handler;
    public:
        explicit registry_t(entt::registry& registry)
            : ecs_history::registry_t(registry), version_handler(this->handle.ctx().emplace<entity_version_handler_t>()) {
        }

        entt::entity create() const {
            const auto entity = this->handle.create();
            const ecs_history::static_entity_t static_entity = this->static_entities.create(entity);
            this->version_handler.add_entity(static_entity, 0);
            return entity;
        }

        std::unique_ptr<commit_t> commit_changes() const {
            const auto& static_entities = this->handle.ctx().get<ecs_history::static_entities_t>();

            auto commit = std::make_unique<commit_t>();
            const auto &monitors = this->handle.ctx().get<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >();
            for (auto &monitor: monitors) {
                commit->change_sets.emplace_back(monitor->commit());
                monitor->clear();
            }

            auto& created_entities = this->handle.storage<entt::reactive>("entity_created_storage"_hs);
            commit->created_entities.reserve(created_entities.size());
            for (const auto& created_entity : created_entities) {
                ecs_history::static_entity_t static_entity = static_entities.get_static_entity(created_entity);
                commit->created_entities.emplace_back(static_entity);
            }
            created_entities.clear();
            auto& destroyed_entities = this->handle.storage<entt::reactive>("entity_destroyed_storage"_hs);
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
                commit->entity_versions[static_entity] = this->version_handler.increase_version(static_entity);
            }

            return std::move(commit);
        }

        bool can_apply(const commit_t &commit) {
            return std::ranges::all_of(commit.entity_versions, [&](const auto &pair) {
                return pair.second == this->version_handler.get_version(pair.first);
            });
        }

        void apply_commit(const commit_t &commit) const {
            if (this->handle.ctx().contains<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >()) {
                const auto &monitors = this->handle.ctx().get<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >();
                for (auto &monitor: monitors) {
                    monitor->disable();
                }
            }

            for (const ecs_history::static_entity_t& created_entity : commit.created_entities) {
                const entt::entity entt = this->handle.create();
                this->static_entities.create(entt, created_entity);
            }
            ecs_history::any_change_applier_t applier{this->handle, this->static_entities};
            for (const auto& change : commit.change_sets) {
                change->supply(applier);
            }
            for (const ecs_history::static_entity_t& removed_entity : commit.destroyed_entities) {
                const auto entt = this->static_entities.remove(removed_entity);
                this->handle.destroy(entt);
            }
            for (const auto &static_entity: commit.entity_versions | std::views::keys) {
                !commit.undo ? this->version_handler.decrease_version(static_entity) : this->version_handler.increase_version(static_entity);
            }

            if (this->handle.ctx().contains<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >()) {
                const auto &monitors = this->handle.ctx().get<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >();
                for (auto &monitor: monitors) {
                    monitor->enable();
                }
            }
        }
    };
}

#endif //ECS_NET_REGISTRY_HPP

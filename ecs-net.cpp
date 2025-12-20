#define ENTT_ID_TYPE std::uint64_t

#include <fstream>
#include <entt/entt.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "cereal/types/string.hpp"

#include <ecs_history/gather_strategy/monitor.hpp>
#include <ecs_history/gather_strategy/change_mixin.hpp>
#include <ecs_history/gather_strategy/registry.hpp>
#include <chrono>
#include "ecs_net/serialization.hpp"


entt::registry reg;
auto &version_handler = reg.ctx().emplace<ecs_net::entity_version_handler_t>();
auto &static_entities = reg.ctx().emplace<ecs_history::static_entities_t>();

std::unique_ptr<commit_t> commit_changes() {
    auto commit = std::make_unique<commit_t>();
    const auto &monitors = reg.ctx().get<std::vector<std::shared_ptr<ecs_history::base_component_monitor_t> > >();
    for (auto &monitor: monitors) {
        commit->component_change_sets.emplace_back(monitor->commit());
        monitor->clear();
    }

    std::unordered_set<ecs_history::static_entity_t> commit_entities;
    for (const auto& change_set : commit->component_change_sets) {
        change_set->for_entity([&commit_entities](const ecs_history::static_entity_t& static_entity) {
            commit_entities.emplace(static_entity);
        });
    }
    for (const ecs_history::static_entity_t& static_entity : commit_entities) {
        commit->entity_versions[static_entity] = version_handler.increase_version(static_entity);
    }

    return std::move(commit);
}

struct rectangle_t {
    uint32_t x, y;
};
template<>
struct entt::storage_type<rectangle_t> {
    /*! @brief Type-to-storage conversion result. */
    using type = change_storage_t<rectangle_t>;
};

int main() {
    ecs_net::serialization::initialize_component_meta_types();
    entt::meta_factory<rectangle_t>{}
            .data<&rectangle_t::x>("x"_hs)
            .data<&rectangle_t::y>("y"_hs);
    ecs_history::record_changes<rectangle_t>(reg);


    return 0;
}

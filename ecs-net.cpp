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

entt::registry reg2;
auto &version_handler2 = reg2.ctx().emplace<ecs_net::entity_version_handler_t>();
auto &static_entities2 = reg2.ctx().emplace<ecs_history::static_entities_t>();

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

template<typename T>
void emplace(entt::registry &registry, entt::entity entt, T &value) {
    registry.emplace<T>(entt, value);
}

int main() {
    ecs_net::serialization::initialize_component_meta_types();
    entt::meta_factory<rectangle_t>{}
            .func<ecs_net::serialization::deserialize_change_set<cereal::PortableBinaryInputArchive, rectangle_t>>("deserialize_change_set"_hs)
            .func<emplace<rectangle_t>>("emplace"_hs)
            .data<&rectangle_t::x>("x"_hs)
            .data<&rectangle_t::y>("y"_hs);
    ecs_history::record_changes<entt::entity>(reg);
    ecs_history::record_changes<rectangle_t>(reg);

    const auto entity = reg.create();
    reg.emplace<rectangle_t>(entity, 1, 2);
    const ecs_history::static_entity_t static_entity = static_entities.create(entity);
    version_handler.add_entity(static_entity, 0);

    const auto commit = commit_changes();

    std::ofstream wf("data.buildit", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    ecs_net::serialization::serialize_commit(archive, *commit);
    wf.close();

    std::ifstream rf("data.buildit", std::ios::in | std::ios::binary);
    cereal::PortableBinaryInputArchive iarchive(rf);
    auto versions = ecs_net::serialization::deserialize_commit_entity_versions(iarchive);
    auto created_entities = ecs_net::serialization::deserialize_entity_list(iarchive);
    auto changes = ecs_net::serialization::deserialize_commit_changes(iarchive);
    auto removed_entities = ecs_net::serialization::deserialize_entity_list(iarchive);

    for (ecs_history::static_entity_t created_entity : created_entities) {
        entt::entity entt = reg2.create();
        static_entities2.create(entt, created_entity);
    }
    ecs_history::any_component_change_applier_t applier{reg2, static_entities2};
    for (const auto& change : changes) {
        change->supply(applier);
    }
    for (ecs_history::static_entity_t removed_entity : removed_entities) {
        auto entt = static_entities2.remove(removed_entity);
        reg2.destroy(entt);
    }

    auto rectangle = reg2.get<rectangle_t>(entity);

    return 0;
}

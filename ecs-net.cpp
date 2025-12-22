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


struct rectangle_t {
    uint32_t x, y;
};
template<>
struct entt::storage_type<rectangle_t> {
    /*! @brief Type-to-storage conversion result. */
    using type = change_storage_t<rectangle_t>;
};

template<typename T>
void emplace(entt::registry &registry, const entt::entity entt, const T &value) {
    registry.emplace<T>(entt, value);
}

int main() {
    ecs_net::serialization::initialize_component_meta_types();
    ecs_history::record_changes<entt::entity>(reg);
    entt::meta_factory<rectangle_t>{}
        .func<ecs_net::serialization::deserialize_change_set<cereal::PortableBinaryInputArchive, rectangle_t>>("deserialize_change_set"_hs)
        .func<emplace<rectangle_t>>("emplace"_hs)
        .data<&rectangle_t::x, entt::as_ref_t>("x"_hs)
        .data<&rectangle_t::y, entt::as_ref_t>("y"_hs);
    ecs_history::record_changes<rectangle_t>(reg);

    const auto entity = reg.create();
    reg.emplace<rectangle_t>(entity, 1, 2);
    const ecs_history::static_entity_t static_entity = static_entities.create(entity);
    version_handler.add_entity(static_entity, 0);

    const auto commit = ecs_net::commit_changes(reg);

    std::ofstream wf("data.buildit", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    ecs_net::serialization::serialize_commit(archive, *commit);
    wf.close();

    std::ifstream rf("data.buildit", std::ios::in | std::ios::binary);
    cereal::PortableBinaryInputArchive iarchive(rf);
    ecs_net::commit_t commit2 = ecs_net::serialization::deserialize_commit(iarchive);
    if (!ecs_net::can_apply(reg2, commit2)) {
        throw std::runtime_error("cannot apply commit");
    }
    ecs_net::apply_commit(reg2, commit2);
    rf.close();

    auto rectangle = reg2.get<rectangle_t>(entity);

    return 0;
}

#define ENTT_ID_TYPE std::uint64_t

#include <fstream>
#include <entt/entt.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

#include <ecs_history/gather_strategy/monitor.hpp>
#include <ecs_history/gather_strategy/change_mixin.hpp>
#include <ecs_history/gather_strategy/registry.hpp>
#include <chrono>

#include "ecs_net/registry.hpp"
#include "ecs_net/serialization.hpp"

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
    entt::meta_factory<rectangle_t>{}
        .func<ecs_net::serialization::deserialize_change_set<cereal::PortableBinaryInputArchive, rectangle_t>>("deserialize_change_set"_hs)
        .func<emplace<rectangle_t>>("emplace"_hs)
        .data<&rectangle_t::x, entt::as_ref_t>("x"_hs)
        .data<&rectangle_t::y, entt::as_ref_t>("y"_hs);

    entt::registry reg;
    ecs_net::registry_t net_reg{reg};
    net_reg.record_changes<entt::entity>();
    net_reg.record_changes<rectangle_t>();

    entt::registry reg2;
    ecs_net::registry_t net_reg2{reg2};


    const auto entity = net_reg.create();
    reg.emplace<rectangle_t>(entity, static_cast<uint32_t>(1), static_cast<uint32_t>(2));

    const auto commit = net_reg.commit_changes();

    std::ofstream wf("data.buildit", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    ecs_net::serialization::serialize_commit(archive, *commit);
    wf.close();

    std::ifstream rf("data.buildit", std::ios::in | std::ios::binary);
    cereal::PortableBinaryInputArchive iarchive(rf);
    ecs_net::commit_t commit2 = ecs_net::serialization::deserialize_commit(iarchive);
    if (!net_reg2.can_apply(commit2)) {
        throw std::runtime_error("cannot apply commit");
    }
    net_reg2.apply_commit(commit2);
    rf.close();

    auto rectangle = reg2.get<rectangle_t>(entity);

    ecs_net::commit_t inverted_commit = !commit2;
    net_reg2.apply_commit(inverted_commit);

    return 0;
}

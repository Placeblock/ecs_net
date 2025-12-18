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

struct plugin_component_t {
    char data;
};

template<>
struct entt::storage_type<plugin_component_t> {
    /*! @brief Type-to-storage conversion result. */
    using type = change_storage_t<plugin_component_t>;
};

int main() {
    ecs_net::serialization::initialize_component_meta_types();

    entt::meta_factory<plugin_component_t>{}
            .traits<ecs_net::serialization::traits_t>(ecs_net::serialization::traits_t::TRIVIAL)
            .data<&plugin_component_t::data>("data"_hs);

    entt::registry registry;
    auto &static_entities = registry.ctx().emplace<ecs_history::static_entities_t>();
    ecs_history::record_changes<plugin_component_t>(registry);

    const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        const entt::entity entt = registry.create();
        static_entities.create(entt);
        registry.emplace<plugin_component_t>(entt, 'a');
    }

    std::ofstream wf("data.buildit", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    ecs_net::serialization::serialize_registry(archive, registry);
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    wf.close();

    return 0;
}

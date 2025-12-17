#define ENTT_ID_TYPE std::uint64_t

#include <fstream>
#include <entt/entt.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "cereal/types/string.hpp"

#include <monitor.hpp>
#include <change_mixin.hpp>
#include <chrono>

#include "change_serialization.hpp"
#include "registry.hpp"

using namespace entt::literals;

enum class traits_t : uint16_t {
    NO = 0x00,
    TRIVIAL = 0x01,

    _entt_enum_as_bitmask
};

struct plugin_component_t {
    char data;
};

struct entity_version_t {
    uint32_t version;
};


template<typename Archive, traits_t traits = traits_t::NO>
void serialize_registry(Archive &archive, entt::registry &reg) {
    const auto &static_entities = reg.ctx().get<static_entities_t>();

    if constexpr (traits != traits_t::NO) {
        const uint16_t numSets = std::ranges::count_if(reg.storage(), [](const auto &p) {
            const entt::meta_type type = entt::resolve(p.first);
            const auto type_traits = type.traits<traits_t>();
            return !!(type_traits & traits);
        });
        archive(numSets);
    } else {
        uint16_t storages = std::distance(reg.storage().begin(), reg.storage().end());
        archive(storages);
    }

    for (auto [id, set]: reg.storage()) {
        const auto meta = entt::resolve(id);
        if (!meta) {
            continue;
        }
        if constexpr (traits != traits_t::NO) {
            if (!(meta.traits<traits_t>() & traits)) {
                continue;
            }
        }
        archive(static_cast<uint64_t>(id));
        archive(static_cast<uint32_t>(set.size()));
        for (const auto &entt: set) {
            static_entity_t static_entity = static_entities.get_static_entity(entt);
            archive(static_entity);
            archive(static_entities.get_version(static_entity));
            void *value = set.value(entt);
            const entt::meta_any any = meta.from_void(value);
            ecsnet::serialization::serialize_component(archive, any);
        }
    }
}

template<typename Archive>
void serialize_changes(Archive &archive, entt::registry &reg) {
    ecsnet::serialization::change_serializer<Archive> serializer{archive};
    const auto &monitors = reg.ctx().get<std::vector<std::shared_ptr<base_component_monitor_t> > >();
    for (auto &monitor: monitors) {
        auto commit = monitor->commit();
        commit->supply(serializer);
    }
}

template<>
struct entt::storage_type<plugin_component_t> {
    /*! @brief Type-to-storage conversion result. */
    using type = change_storage_t<plugin_component_t>;
};

int main() {
    ecsnet::serialization::initialize_component_meta_types();

    entt::meta_factory<plugin_component_t>{}
            .traits<traits_t>(traits_t::TRIVIAL)
            .data<&plugin_component_t::data>("data"_hs);

    entt::registry registry;
    auto &static_entities = registry.ctx().emplace<static_entities_t>();
    ecs_history::record_changes<plugin_component_t>(registry);

    for (int i = 0; i < 1000000; ++i) {
        const entt::entity entt = registry.create();
        static_entities.create(entt);
        registry.emplace<plugin_component_t>(entt, 0);
    }

    std::ofstream wf("data.buildit", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    serialize_registry(archive, registry);
    wf.close();

    return 0;
}

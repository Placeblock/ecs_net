#define ENTT_ID_TYPE std::uint64_t

#include <fstream>
#include <entt/entt.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
#include "cereal/types/string.hpp"

using namespace entt::literals;

enum class traits_t : uint16_t {
    NO = 0x00,
    TRIVIAL = 0x01,

    _entt_enum_as_bitmask
};

struct plugin_component_t {
    std::string name;
};

struct entity_version_t {
    uint32_t version;
};


template<typename Archive, typename Type>
void serialize_simple(Archive &archive, const Type &value) {
    archive(value);
}

template<typename Archive, traits_t traits = traits_t::NO>
void serialize_registry(Archive &archive, entt::registry &reg) {
    const uint16_t numSets = std::ranges::count_if(reg.storage(), [](const auto &p) {
        const entt::meta_type type = entt::resolve(p.first);
        const auto type_traits = type.traits<traits_t>();
        return !!(type_traits & traits);
    });
    archive(numSets);

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
            archive(entt);
            archive(reg.get<entity_version_t>(entt).version);
            void *value = set.value(entt);
            const entt::meta_any any = meta.from_void(value);
            serialize_component(archive, any);
        }
    }
}

template<typename Archive>
void serialize_changes(Archive &archive, entt::registry &reg) {
    reg.ctx().



}


#define SERIALIZE_SIMPLE(T) \
    entt::meta_factory<T>() \
    .func<serialize_simple<cereal::PortableBinaryOutputArchive, T>>("serialize"_hs)


int main() {
    SERIALIZE_SIMPLE(uint64_t);
    SERIALIZE_SIMPLE(uint32_t);
    SERIALIZE_SIMPLE(uint16_t);
    SERIALIZE_SIMPLE(uint8_t);
    SERIALIZE_SIMPLE(int64_t);
    SERIALIZE_SIMPLE(int32_t);
    SERIALIZE_SIMPLE(int16_t);
    SERIALIZE_SIMPLE(int8_t);
    SERIALIZE_SIMPLE(char);
    SERIALIZE_SIMPLE(std::string);

    entt::registry registry;

    entt::meta_factory<plugin_component_t>{}
            .data<&plugin_component_t::name>("name"_hs);

    const entt::entity entt = registry.create();
    registry.emplace<plugin_component_t>(entt, "t");

    std::ofstream wf("data.buildit", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    serialize_registry(archive, registry);
    wf.close();

    return 0;
}

#define ENTT_ID_TYPE std::uint64_t

#include <fstream>
#include <entt/entt.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace entt::literals;

enum class traits_t : uint16_t {
    TRANSIENT = 0x01,
    HISTORY = 0x02,
    SYNCHRONIZED = 0x04,
    SAVED = 0x08,

    _entt_enum_as_bitmask
};

struct plugin_component_t {
    void *data;
};

template<typename Archive>
void serializeComponent(Archive &archive, const entt::entity entt,
                        const entt::meta_any &value) {
    const entt::meta_type type = value.type();

    const auto traits = type.traits<traits_t>();
    if (!!(traits & traits_t::TRANSIENT)) {
        archive(entt);
        const void *data = value.base().data();
        auto binary = cereal::binary_data(data, type.size_of());
        archive(binary);
        return;
    }

    throw std::runtime_error("Cannot serialize component");
}

template<typename Archive>
void serializeRegistry(Archive &archive, entt::registry &reg, traits_t traits = traits_t::HISTORY) {
    const size_t numSets = std::ranges::count_if(reg.storage(), [traits](const auto &p) {
        const entt::meta_type type = entt::resolve(p.first);
        const auto type_traits = type.traits<traits_t>();
        return !!(type_traits & traits);
    });
    archive(numSets);

    for (auto [id, set]: reg.storage()) {
        const auto meta = entt::resolve(id);
        if (!meta || !(meta.traits<traits_t>() & traits)) {
            continue;
        }
        archive(static_cast<uint64_t>(id));
        archive(static_cast<uint32_t>(set.size()));
        for (const auto &entt: set) {
            void *value = set.value(entt);
            const entt::meta_any any = meta.from_void(value);
            serializeComponent(archive, entt, any);
        }
    }
}

int main() {
    entt::registry registry;

    entt::meta_factory<plugin_component_t>{}
            .traits(traits_t::TRANSIENT)
            .data<&plugin_component_t::data>("data"_hs);

    int test = 0;
    const entt::entity entt = registry.create();
    registry.emplace<plugin_component_t>(entt, &test);

    std::ofstream wf("student.dat", std::ios::out | std::ios::binary);
    cereal::PortableBinaryOutputArchive archive(wf);
    serializeRegistry(archive, registry);
    wf.close();

    return 0;
}

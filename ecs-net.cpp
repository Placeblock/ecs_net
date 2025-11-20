#define ENTT_ID_TYPE std::uint64_t

#include <entt/entt.hpp>
#include <iostream>

typedef uint64_t network_id_t;

template<typename T>
class change_storage_t {
    using storage_t = typename entt::storage_for<T>::type;
    using reactive_storage = entt::basic_reactive_mixin<entt::basic_storage<entt::reactive>, entt::basic_registry<> >;

public:
    explicit change_storage_t(entt::registry &registry, const bool on_destroy = false,
                              const entt::id_type id = entt::type_hash<T>::value()) : on_destroy(on_destroy) {
        this->change_storage.bind(registry);
        if (on_destroy) {
            this->change_storage.template on_destroy<T>(id);
        } else {
            this->change_storage.template on_construct<T>(id).template on_update<T>(id);
        }
    }

    reactive_storage change_storage;
    bool on_destroy;
};

template<typename T>
class storage_archive_t {
public:
    std::vector<char> buffer;

    storage_archive_t(const entt::storage<network_id_t> &network_ids,
                      std::function<void(storage_archive_t &, const T &)> serialize) : network_ids(network_ids),
        serialize(std::move(std::move(serialize))) {
    }

    void write(const void *data, const size_t size) {
        buffer.insert(buffer.end(), static_cast<const char *>(data), static_cast<const char *>(data) + size);
    }

    void operator()(const entt::entity entity) {
        this->write(static_cast<const void *>(&this->network_ids.get(entity)), sizeof(network_id_t));
    }

    void operator()(const T &data) {
        this->serialize(*this, data);
    }

    void operator()(std::underlying_type_t<entt::entity> _) const {
    }

private:
    const entt::storage<network_id_t> &network_ids;
    std::function<void(storage_archive_t &, const T &)> serialize;
};

struct plugin_component_t {
    char data;
};

int main() {
    entt::registry registry;

    entt::storage<network_id_t> network_ids;

    using namespace entt::literals;
    auto &entity_storage = registry.storage<entt::reactive>("entity_observer"_hs);
    entity_storage.on_construct<entt::entity>();
    auto &entity_destroy_storage = registry.storage<entt::reactive>("entity_destroy_observer"_hs);
    entity_destroy_storage.on_destroy<entt::entity>();

    const change_storage_t<plugin_component_t> component_storage(registry);
    const change_storage_t<plugin_component_t> destroy_component_storage(registry, true);

    // #############################################
    const entt::entity entity = registry.create();
    registry.emplace<plugin_component_t>(entity, 0);
    network_ids.emplace(entity, 0);
    const entt::entity entity2 = registry.create();
    registry.destroy(entity2);
    // #############################################

    storage_archive_t<plugin_component_t> archive{
        network_ids,
        [](storage_archive_t<plugin_component_t> &arch, const plugin_component_t &comp) {
            arch.write(&comp.data, sizeof(comp.data));
        }
    };
    storage_archive_t<plugin_component_t> destroy_archive{
        network_ids,
        [](storage_archive_t<plugin_component_t> &arch, const plugin_component_t &comp) {
            arch.write(&comp.data, sizeof(comp.data));
        }
    };
    storage_archive_t<plugin_component_t> entity_archive{network_ids, nullptr};
    storage_archive_t<plugin_component_t> entity_destroy_archive{network_ids, nullptr};

    entt::snapshot{registry}.get<plugin_component_t>(archive, component_storage.change_storage.begin(),
                                                     component_storage.change_storage.end());
    destroy_archive.write(destroy_component_storage.change_storage.data(),
                          destroy_component_storage.change_storage.size());
    entity_archive.write(entity_storage.data(), entity_storage.size() * sizeof(entt::entity));
    entity_destroy_archive.write(entity_destroy_storage.data(), entity_destroy_storage.size() * sizeof(entt::entity));

    printf("Sending %zu bytes for new components\n", archive.buffer.size());
    printf("Sending %zu bytes for removed components\n", destroy_archive.buffer.size());
    printf("Sending %zu bytes for new entities\n", entity_archive.buffer.size());
    printf("Sending %zu bytes for removed entities\n", entity_destroy_archive.buffer.size());

    return 0;
}

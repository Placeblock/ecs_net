#define ENTT_ID_TYPE std::uint64_t

#include <entt/entt.hpp>
#include <zmq.hpp>
#include <iostream>

enum message_type_t : uint8_t {
    COMPONENT_UPDATES,
    COMPONENT_DESTROYED,
    ENTITIES,
    ENTITIES_DESTROYED,
};

typedef uint64_t network_id_t;
using namespace entt::literals;


struct component_type_t {
    std::string name;
    entt::id_type id;
};

struct buffer_t {
    std::vector<char> data;

    buffer_t() = default;

    buffer_t(const buffer_t &other) = delete;

    buffer_t &operator=(const buffer_t &other) = delete;

    buffer_t(buffer_t &&other) noexcept : data(std::move(other.data)) {
    }

    buffer_t &operator=(buffer_t &&other) noexcept {
        this->data = std::move(other.data);
        return *this;
    }

    void write(const void *data, const size_t size) {
        this->data.insert(this->data.end(), static_cast<const char *>(data), static_cast<const char *>(data) + size);
    }
};

struct registry_component_type_t {
    using reactive_storage = entt::basic_reactive_mixin<entt::basic_storage<entt::reactive>, entt::basic_registry<> >;

    entt::registry &registry;
    const entt::storage<network_id_t> &network_ids;
    component_type_t component_type;
    reactive_storage updated_storage;
    reactive_storage destroyed_storage;

    registry_component_type_t(entt::registry &registry, const entt::storage<network_id_t> &network_ids,
                              const component_type_t &component_type)
        : registry(registry), network_ids(network_ids), component_type(component_type) {
        this->updated_storage.bind(registry);
        this->destroyed_storage.bind(registry);
    }

    virtual void serialize_updated(buffer_t &buffer) = 0;

    virtual void serialize_destroyed(buffer_t &buffer) = 0;

    virtual void serialize_all(buffer_t &buffer) = 0;

    virtual ~registry_component_type_t() = default;
};

template<typename T>
class storage_archive_t;

template<typename T>
using serialize_fn_t = std::function<void(const storage_archive_t<T> &, const T &)>;


template<typename T>
class storage_archive_t {
public:
    storage_archive_t(const entt::storage<network_id_t> &network_ids,
                      const serialize_fn_t<T> &serialize_fn,
                      buffer_t &buffer) : buffer(buffer), network_ids(network_ids),
                                          serialize_fn(serialize_fn) {
    }

    void write(const void *data, const size_t size) const {
        buffer.write(data, size);
    }

    void operator()(const entt::entity entity) const {
        this->write(static_cast<const void *>(&this->network_ids.get(entity)), sizeof(network_id_t));
    }

    void operator()(const T &data) const {
        this->serialize_fn(*this, data);
    }

    void operator()(std::underlying_type_t<entt::entity> _) const {
    }

private:
    buffer_t &buffer;
    const entt::storage<network_id_t> &network_ids;
    const serialize_fn_t<T> &serialize_fn;
};

template<typename T>
struct typed_registry_component_type_t final : registry_component_type_t {
    const serialize_fn_t<T> serialize_fn;

    typed_registry_component_type_t(entt::registry &registry, const entt::storage<network_id_t> &network_ids,
                                    const component_type_t &component_type,
                                    const serialize_fn_t<T> &serialize_fn)
        : registry_component_type_t(registry, network_ids, component_type), serialize_fn(serialize_fn) {
        this->updated_storage.template on_construct<T>(component_type.id).template on_update<T>(component_type.id);
        this->destroyed_storage.template on_destroy<T>(component_type.id);
    }

    void serialize_updated(buffer_t &buffer) override {
        storage_archive_t<T> archive{network_ids, serialize_fn, buffer};
        entt::snapshot{registry}.get<T>(archive, updated_storage.begin(),
                                        updated_storage.end());
    }

    void serialize_destroyed(buffer_t &buffer) override {
        storage_archive_t<T> destroy_archive{network_ids, nullptr, buffer};
        for (const auto &entt: destroyed_storage) {
            destroy_archive.write(&network_ids.get(entt), sizeof(network_id_t));
        }
    }

    void serialize_all(buffer_t &buffer) override {
        storage_archive_t<T> archive{network_ids, serialize_fn, buffer};
        entt::snapshot{registry}.get<T>(archive, component_type.id);
    }
};

struct message_writer_t {
    std::vector<zmq::const_buffer> parts;
};

struct registry_components_t {
    std::vector<std::unique_ptr<registry_component_type_t> > registry_components;

    static buffer_t create_envelope(const message_type_t message_type, const std::string &component_type_name) {
        buffer_t envelope;
        envelope.write(&message_type, 1);
        envelope.write(component_type_name.c_str(), component_type_name.size());
        return envelope;
    }

    void serialize_changes(std::vector<buffer_t> &parts) const {
        for (const auto &comp: registry_components) {
            printf("Serializing changes for component %s\n", comp->component_type.name.c_str());
            if (!comp->updated_storage.empty()) {
                printf("Serializing updated\n");
                parts.emplace_back(create_envelope(COMPONENT_UPDATES, comp->component_type.name));
                buffer_t data;
                comp->serialize_updated(data);
                parts.emplace_back(std::move(data));
            }
            if (!comp->destroyed_storage.empty()) {
                printf("Serializing destroyed\n");
                parts.emplace_back(create_envelope(COMPONENT_DESTROYED, comp->component_type.name));
                buffer_t data;
                comp->serialize_destroyed(data);
                parts.emplace_back(std::move(data));
            }
        }
    }
};

struct plugin_component_t {
    char data;
};

int main() {
    entt::registry registry;

    entt::storage<network_id_t> network_ids;

    auto &entity_storage = registry.storage<entt::reactive>("entity_observer"_hs);
    entity_storage.on_construct<entt::entity>();
    auto &entity_destroy_storage = registry.storage<entt::reactive>("entity_destroy_observer"_hs);
    entity_destroy_storage.on_destroy<entt::entity>();

    registry_components_t components;
    component_type_t test_component_type{"test_component", entt::type_hash<plugin_component_t>::value()};
    std::unique_ptr<registry_component_type_t> test_registry_component_type =
            std::make_unique<typed_registry_component_type_t<plugin_component_t> >(
                registry, network_ids, test_component_type,
                [](const storage_archive_t<plugin_component_t> &arch, const plugin_component_t &comp) {
                    arch.write(&comp.data, sizeof(comp.data));
                });
    components.registry_components.emplace_back(std::move(test_registry_component_type));

    // #############################################
    const entt::entity entity = registry.create();
    registry.emplace<plugin_component_t>(entity, 0);
    network_ids.emplace(entity, 0);
    // #############################################

    buffer_t entities_buffer;
    buffer_t entities_destroyed_buffer;
    for (const auto &entt: entity_storage) {
        entities_buffer.write(&network_ids.get(entt), sizeof(network_id_t));
    }
    for (const auto &entt: entity_destroy_storage) {
        entities_destroyed_buffer.write(&network_ids.get(entt), sizeof(network_id_t));
    }

    std::vector<buffer_t> parts;
    components.serialize_changes(parts);

    printf("Sending %lu parts for component changes\n", parts.size());
    printf("Sending %zu bytes for new entities\n", entities_buffer.data.size());
    printf("Sending %zu bytes for removed entities\n", entities_destroyed_buffer.data.size());

    return 0;
}

#define ENTT_ID_TYPE std::uint64_t

#include <entt/entt.hpp>
#include <zmq.hpp>
#include <iostream>
#include <csignal>

typedef uint64_t network_id_t;

entt::registry registry;

template<typename T>
class change_observer {
    using storage_t = typename entt::storage_for<T>::type;
    using reactive_storage = entt::basic_reactive_mixin<entt::basic_storage<entt::reactive>, entt::basic_registry<> >;

public:
    change_observer(entt::registry &registry, storage_t &storage, const entt::id_type id)
        : registry(registry), storage(storage) {
        printf("Observer uses storage id: %lu\n", id);
        this->change_storage.on_construct<T>(id).template on_update<T>(id).template on_destroy<T>(id);
    }

    explicit change_observer(entt::registry &registry, const entt::id_type id = entt::type_hash<T>::value())
        : change_observer(registry, registry.storage<T>(id), id) {
    }

    reactive_storage change_storage;

private:
    const entt::registry &registry;
    const storage_t &storage;
};

template<typename T>
class storage_archive {
    void write(const void *data, const size_t size) {
        buffer.insert(buffer.end(), static_cast<const char *>(data), static_cast<const char *>(data) + size);
    }

public:
    std::vector<char> buffer;

    storage_archive(const entt::storage<network_id_t> &network_ids,
                    std::function<void(storage_archive &, T &)> serialize) : network_ids(network_ids),
                                                                             serialize(serialize) {
    }

    void operator()(const entt::entity entity) {
        this->write(static_cast<const void *>(&network_ids.get(entity)), sizeof(network_id_t));
    }

    void operator()(const T &data) {
        this->serialize(*this, data);
    }

    void operator()(std::underlying_type_t<entt::entity> _) const {
    }

private:
    const entt::storage<network_id_t> &network_ids;
    std::function<void(storage_archive &, T &)> serialize;
};

struct plugin_component {
    char data;
};

char buffer[10000000];

int main() {
    zmq::context_t ctx;
    zmq::socket_t socket(ctx, zmq::socket_type::pub);
    socket.bind("tcp://*:5555");

    entt::storage<network_id_t> network_ids;

    std::string storage_type = "test";
    storage_type += "_component";
    const entt::hashed_string storage_name(storage_type.c_str(), storage_type.size());
    printf("Plugin Component has storage id: %lu\n", static_cast<entt::id_type>(storage_name));
    auto &storage = registry.storage<plugin_component>(storage_name);
    const change_observer<plugin_component> observer(registry, storage_name);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (int i = 0; i < sizeof(buffer); ++i) {
        const entt::entity entity = registry.create();
        storage.emplace(entity, plugin_component{&buffer[i]});
        network_ids.emplace(entity, i);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Creating entities: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
            "[µs]" << std::endl;

    begin = std::chrono::steady_clock::now();
    buffer_sink sink;
    int i = 0;
    for (const auto entity: observer.change_storage) {
        sink.write(&registry.get<network_id>(entity), 8);
        test_serialize(buffer_sink::DataWriter, &sink, &storage.get(entity).data);
        i++;
    }
    end = std::chrono::steady_clock::now();
    std::cout << "Serialization took: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
            "[µs]" << std::endl;
    printf("Sending %zu bytes from %d entities\n", sink.buffer.size(), i);

    begin = std::chrono::steady_clock::now();
    const zmq::const_buffer data(sink.buffer.data(), sink.buffer.size());
    socket.send(data, zmq::send_flags::dontwait);
    end = std::chrono::steady_clock::now();
    std::cout << "Sending took: " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<
            "[µs]" << std::endl;

    socket.close();
    ctx.close();

    return 0;
}

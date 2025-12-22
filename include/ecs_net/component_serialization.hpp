//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_COMPONENT_SERIALIZER_HPP
#define ECS_NET_COMPONENT_SERIALIZER_HPP

#include <entt/entt.hpp>

using namespace entt::literals;

namespace ecs_net::serialization {
    enum class traits_t : uint16_t {
        NO = 0x00,
        TRIVIAL = 0x01,

        _entt_enum_as_bitmask
    };

    /**
     *
     * @tparam Archive The type of Archive to write to / read from
     * @tparam Serialize Whether to serialize or deserialize
     * @param archive The archive
     * @param value The value
     */
    template<typename Archive, bool Serialize>
    void serialize_component(Archive &archive, entt::meta_any value) {
        const entt::meta_type type = value.type();

        constexpr auto funcId = Serialize ? "serialize"_hs : "deserialize"_hs;
        if (auto func = type.func(funcId); func) {
            if (!func.invoke({}, entt::forward_as_meta(archive), value.as_ref())) {
                throw std::runtime_error("Could not (de)serialize component with func");
            }
            return;
        }

        if (type.is_sequence_container()) {
            auto sequence = value.as_sequence_container();
            uint32_t size = sequence.size();
            archive(size);
            if constexpr (!Serialize) {
                sequence.resize(size);
            }
            for (auto element: sequence) {
                serialize_component<Archive, Serialize>(archive, element.as_ref());
            }
            return;
        }
        if (type.is_associative_container()) {
            auto assoc = value.as_associative_container();
            uint32_t size = assoc.size();
            archive(size);

            if constexpr (Serialize) {
                for (auto [key, val]: assoc) {
                    serialize_component<Archive, Serialize>(archive, key.as_ref());
                    serialize_component<Archive, Serialize>(archive, val.as_ref());
                }
            } else {
                for (uint32_t i = 0; i < size; ++i) {
                    auto key = assoc.key_type().construct();
                    auto val = assoc.mapped_type().construct();
                    serialize_component<Archive, Serialize>(archive, key.as_ref());
                    serialize_component<Archive, Serialize>(archive, val.as_ref());
                    if (!assoc.insert(key, val)) {
                        throw std::runtime_error("Could not insert pair into associative container while deserializing.");
                    }
                }
            }
            return;
        }

        for (const auto& [id, data]: type.data()) {
            if (!data) {
                throw std::runtime_error("Component attribute is false");
            }
            if constexpr (Serialize) {
                serialize_component<Archive, Serialize>(archive, data.get(value).as_ref());
            } else {
                entt::meta_any data_any = data.get(value).as_ref();
                serialize_component<Archive, Serialize>(archive, data_any.as_ref());
                value.set(id, data_any);
            }
        }
    }

    template<typename Archive, typename Type>
    void serialize_simple(Archive &archive, Type &value) {
        archive(value);
    }

#define SERIALIZE_SIMPLE(T) \
        entt::meta_factory<T>() \
        .func<serialize_simple<cereal::PortableBinaryOutputArchive, const T>>("serialize"_hs) \
        .func<serialize_simple<cereal::PortableBinaryInputArchive, T>>("deserialize"_hs)

    inline void initialize_component_meta_types() {
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
    }
};

#endif //ECS_NET_COMPONENT_SERIALIZER_HPP

//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_COMPONENT_SERIALIZER_HPP
#define ECS_NET_COMPONENT_SERIALIZER_HPP

#include <entt/entt.hpp>
#include <cereal/cereal.hpp>

using namespace entt::literals;

namespace ecs_net::serialization {
    enum class traits_t : uint16_t {
        NO = 0x00,
        TRIVIAL = 0x01,

        _entt_enum_as_bitmask
    };

    template<typename Archive>
    void serialize_component(Archive &archive, const entt::meta_any &value) {
        const entt::meta_type type = value.type();
        if (auto func = type.func("serialize"_hs); func) {
            func.invoke(value, entt::forward_as_meta(archive), value.as_ref());
            return;
        }
        if (!!(type.traits<traits_t>() & traits_t::TRIVIAL)) {
            const void *data = value.base().data();
            auto binary = cereal::binary_data(data, type.size_of());
            archive(binary);
            return;
        }
        if (type.is_sequence_container()) {
            auto sequence = value.as_sequence_container();
            std::size_t size = sequence.size();
            archive(size);
            if (sequence.size() > 0
                && !!(sequence.value_type().traits<traits_t>() & traits_t::TRIVIAL)) {
                const void *data = sequence.begin().base().data();
                auto binary = cereal::binary_data(data, sequence.value_type().size_of() * size);
                archive(binary);
            } else {
                for (const auto &element: sequence) {
                    serialize_component(archive, element);
                }
            }
            return;
        }
        if (type.is_associative_container()) {
            auto assoc = value.as_associative_container();
            std::size_t size = assoc.size();
            archive(size);
            for (const auto &[key, val]: assoc) {
                serialize_component(archive, key);
                serialize_component(archive, val);
            }
            return;
        }

        for (auto [id, data]: type.data()) {
            if (!data) {
                throw std::runtime_error("Component attribute is false");
            }
            if (!!(data.traits<traits_t>() & traits_t::TRIVIAL)) {
                const void *bin_data = data.get(value).base().data();
                auto binary = cereal::binary_data(bin_data, data.type().size_of());
                archive(binary);
            } else {
                serialize_component(archive, data.get(value).as_ref());
            }
        }
    }


    template<typename Archive, typename Type>
    void serialize_simple(Archive &archive, const Type &value) {
        archive(value);
    }

#define SERIALIZE_SIMPLE(T) \
        entt::meta_factory<T>() \
        .func<serialize_simple<cereal::PortableBinaryOutputArchive, T>>("serialize"_hs)

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

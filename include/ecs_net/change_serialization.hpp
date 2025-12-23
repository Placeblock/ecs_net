
//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_CHANGE_SERIALIZATION_HPP
#define ECS_NET_CHANGE_SERIALIZATION_HPP

#include "component_serialization.hpp"

namespace ecs_net::serialization {
enum class change_type_t : uint8_t {
    CONSTRUCT = 0,
    UPDATE = 1,
    UPDATE_ONLY_NEW = 2,
    DESTRUCT = 3,
    DESTRUCT_ONLY_NEW = 4
};

template<typename Archive, bool OnlyNew = true>
class change_serializer final : public ecs_history::any_change_supplier_t {
public:
    explicit change_serializer(Archive &archive)
        : archive(archive) {
    }

    void apply_construct(ecs_history::static_entity_t static_entity,
                         entt::meta_any &value) override {
        archive(static_entity);
        archive(change_type_t::CONSTRUCT);
        serialize_component<Archive, true>(this->archive, value);
    }

    void apply_update(ecs_history::static_entity_t static_entity,
                      entt::meta_any &old_value,
                      entt::meta_any &new_value) override {
        archive(static_entity);
        if constexpr (!OnlyNew) {
            archive(change_type_t::UPDATE);
            serialize_component<Archive, true>(this->archive, old_value);
            serialize_component<Archive, true>(this->archive, new_value);
        } else {
            archive(change_type_t::UPDATE_ONLY_NEW);
            serialize_component<Archive, true>(this->archive, new_value);
        }
    }

    void apply_destruct(ecs_history::static_entity_t static_entity,
                        entt::meta_any &old_value) override {
        archive(static_entity);
        if constexpr (!OnlyNew) {
            archive(change_type_t::DESTRUCT);
            serialize_component<Archive, true>(this->archive, old_value);
        } else {
            archive(change_type_t::DESTRUCT_ONLY_NEW);
        }
    }

private:
    Archive &archive;
};

template<typename Archive, typename Type>
std::unique_ptr<ecs_history::component_change_t<Type> > deserialize_change(Archive &archive) {
    const entt::meta_type &type = entt::resolve<Type>();
    ecs_history::static_entity_t static_entity;
    archive(static_entity);
    change_type_t change_type;
    archive(change_type);
    switch (change_type) {
    case change_type_t::CONSTRUCT: {
        entt::meta_any value = type.construct();
        if (!value) {
            throw std::runtime_error("construction of component failed");
        }
        serialize_component<Archive, false>(archive, value.as_ref());
        return std::make_unique<ecs_history::construct_change_t<Type> >(
            static_entity,
            value.cast<Type>());
    }
    case change_type_t::UPDATE: {
        entt::meta_any old_value = type.construct();
        if (!old_value) {
            throw std::runtime_error("construction of component failed");
        }
        serialize_component<Archive, false>(archive, old_value);
        entt::meta_any new_value = type.construct();
        if (!new_value) {
            throw std::runtime_error("construction of component failed");
        }
        serialize_component<Archive, false>(archive, new_value);
        return std::make_unique<ecs_history::update_change_t<Type> >(
            static_entity,
            old_value.cast<Type>(),
            new_value.cast<Type>());
    }
    case change_type_t::UPDATE_ONLY_NEW: {
        entt::meta_any new_value = type.construct();
        if (!new_value) {
            throw std::runtime_error("construction of component failed");
        }
        serialize_component<Archive, false>(archive, new_value);
        return std::make_unique<ecs_history::update_change_t<Type> >(
            static_entity,
            Type{},
            new_value.cast<Type>());
    }
    case change_type_t::DESTRUCT: {
        entt::meta_any old_value = type.construct();
        if (!old_value) {
            throw std::runtime_error("construction of component failed");
        }
        serialize_component<Archive, false>(archive, old_value);
        return std::make_unique<ecs_history::destruct_change_t<Type> >(
            static_entity,
            old_value.cast<Type>());
    }
    case change_type_t::DESTRUCT_ONLY_NEW: {
        return std::make_unique<ecs_history::destruct_change_t<Type> >(static_entity, Type{});
    }
    default:
        return nullptr;
    }
}
}

#endif //ECS_NET_CHANGE_SERIALIZATION_HPP

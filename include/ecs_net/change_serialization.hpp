
//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_CHANGE_SERIALIZATION_HPP
#define ECS_NET_CHANGE_SERIALIZATION_HPP

#include <ecs_history/change_applier.hpp>
#include <utility>
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
    class change_serializer final : public ecs_history::any_component_change_supplier_t {
    public:
        explicit change_serializer(Archive &archive)
            : archive(archive) {
        }

        void apply_construct(ecs_history::static_entity_t static_entity, entt::meta_any &value) override {
            archive(static_entity);
            archive(change_type_t::CONSTRUCT);
            serialize_component<Archive, true>(this->archive, value);
        }

        void apply_update(ecs_history::static_entity_t static_entity, entt::meta_any &old_value, entt::meta_any &new_value) override {
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

        void apply_destruct(ecs_history::static_entity_t static_entity, entt::meta_any &old_value) override {
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

    struct deserialized_change {
        ecs_history::static_entity_t static_entity;

        explicit deserialized_change(const ecs_history::static_entity_t static_entity)
            : static_entity(static_entity) {}

        virtual void apply(entt::registry &reg, const ecs_history::static_entities_t& static_entities) const = 0;

        virtual ~deserialized_change() = default;

        static void apply(entt::registry &reg, entt::entity entt, const entt::meta_any &value) {
            const auto emplaceFunc = value.type().func("emplace"_hs);
            if (!emplaceFunc) {
                const std::string type_name{value.base().type().name()};
                throw std::runtime_error("cannot find emplace function for deserialized change " + type_name);
            }
            emplaceFunc.invoke({}, entt::forward_as_meta(reg), entt, value.as_ref());
        }
    };
    struct construct_change final : deserialized_change {
        entt::meta_any value;
        explicit construct_change(const ecs_history::static_entity_t static_entity, entt::meta_any value)
            : deserialized_change(static_entity), value(std::move(value)) {
        }
        void apply(entt::registry &reg, const ecs_history::static_entities_t& static_entities) const override {
            const auto entt = static_entities.get_entity(this->static_entity);
            deserialized_change::apply(reg, entt, this->value);
        }
    };
    struct update_change final : deserialized_change {
        entt::meta_any old_value;
        entt::meta_any new_value;
        explicit update_change(const ecs_history::static_entity_t static_entity,
                entt::meta_any old_value, entt::meta_any new_value)
            : deserialized_change(static_entity), old_value(std::move(old_value)), new_value(std::move(new_value)) {
        }
        void apply(entt::registry &reg, const ecs_history::static_entities_t &static_entities) const override {
            const auto entt = static_entities.get_entity(this->static_entity);
            deserialized_change::apply(reg, entt, this->new_value);
        }
    };
    struct destruct_change final : deserialized_change {
        entt::meta_any old_value;
        destruct_change(const ecs_history::static_entity_t static_entity, entt::meta_any old_value)
            : deserialized_change(static_entity),
              old_value(std::move(old_value)) {
        }
        void apply(entt::registry &reg, const ecs_history::static_entities_t &static_entities) const override {
            const auto entt = static_entities.get_entity(this->static_entity);
            const auto storage = reg.storage(old_value.type().id());
            storage->remove(entt);
        }
    };

    template<typename Archive>
    std::unique_ptr<deserialized_change> deserialize_change(Archive &archive, entt::meta_type &type) {
        ecs_history::static_entity_t static_entity;
        archive(static_entity);
        change_type_t change_type;
        archive(change_type);
        switch (change_type) {
            case change_type_t::CONSTRUCT: {
                entt::meta_any value = type.construct();
                serialize_component<Archive, false>(archive, value);
                return std::make_unique<construct_change>(static_entity, value);
            }
            case change_type_t::UPDATE: {
                entt::meta_any old_value = type.construct();
                serialize_component<Archive, false>(archive, old_value);
                entt::meta_any new_value = type.construct();
                serialize_component<Archive, false>(archive, new_value);
                return std::make_unique<update_change>(static_entity, old_value, new_value);
            }
            case change_type_t::UPDATE_ONLY_NEW: {
                entt::meta_any new_value = type.construct();
                serialize_component<Archive, false>(archive, new_value);
                return std::make_unique<update_change>(static_entity, entt::meta_any{}, new_value);
            }
            case change_type_t::DESTRUCT: {
                entt::meta_any old_value = type.construct();
                serialize_component<Archive, false>(archive, old_value);
                return std::make_unique<destruct_change>(static_entity, old_value);
            }
            case change_type_t::DESTRUCT_ONLY_NEW: {
                return std::make_unique<destruct_change>(static_entity, entt::meta_any{});
            }
            default:
                return nullptr;
        }
    }
}

#endif //ECS_NET_CHANGE_SERIALIZATION_HPP

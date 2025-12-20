
//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_CHANGE_SERIALIZATION_HPP
#define ECS_NET_CHANGE_SERIALIZATION_HPP

#include <ecs_history/change_applier.hpp>
#include "component_serialization.hpp"

namespace ecs_net::serialization {
    enum class change_type_t : uint8_t {
        CONSTRUCT = 0,
        UPDATE = 1,
        UPDATE_ONLY_NEW = 2,
        DESTRUCT = 3,
        DESTRUCT_ONLY_NEW = 4,

        _entt_enum_as_bitmask
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
            serialize_component(this->archive, value);
        }

        void apply_update(ecs_history::static_entity_t static_entity, entt::meta_any &old_value, entt::meta_any &new_value) override {
            archive(static_entity);
            if constexpr (!OnlyNew) {
                archive(change_type_t::UPDATE);
                serialize_component(this->archive, old_value);
                serialize_component(this->archive, new_value);
            } else {
                archive(change_type_t::UPDATE_ONLY_NEW);
                serialize_component(this->archive, new_value);
            }
        }

        void apply_destruct(ecs_history::static_entity_t static_entity, entt::meta_any &old_value) override {
            archive(static_entity);
            if constexpr (!OnlyNew) {
                archive(change_type_t::DESTRUCT);
                serialize_component(this->archive, old_value);
            } else {
                archive(change_type_t::DESTRUCT_ONLY_NEW);
            }
        }

    private:
        Archive &archive;
    };
}

#endif //ECS_NET_CHANGE_SERIALIZATION_HPP

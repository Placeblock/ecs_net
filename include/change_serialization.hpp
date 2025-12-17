
//
// Created by felix on 17.12.25.
//

#ifndef ECS_NET_CHANGE_SERIALIZATION_HPP
#define ECS_NET_CHANGE_SERIALIZATION_HPP

#include <change_applier.hpp>
#include "component_serializer.hpp"

namespace ecsnet::serialization {
    template<typename Archive, typename Type, bool OnlyNew = true>
    class change_serializer final : public component_change_supplier_t<Type> {
    public:
        explicit change_serializer(Archive &archive)
            : archive(archive), type(entt::resolve<Type>()) {
        }

        void apply(const construct_change_t<Type> &c) override {
            const entt::meta_any value{c.value};
            serialize_component(archive, value);
        }

        void apply(const update_change_t<Type> &c) override {
            if constexpr (!OnlyNew) {
                const entt::meta_any old_value{c.value};
                serialize_component(archive, old_value);
            }
            const entt::meta_any new_value{c.value};
            serialize_component(archive, new_value);
        }

        void apply(const destruct_change_t<Type> &c) override {
            if constexpr (!OnlyNew) {
                const entt::meta_any old_value{c.value};
                serialize_component(archive, old_value);
            }
        }

    private:
        Archive &archive;
        entt::meta_type type;
    };
}

#endif //ECS_NET_CHANGE_SERIALIZATION_HPP

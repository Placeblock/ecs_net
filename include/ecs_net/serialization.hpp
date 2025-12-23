//
// Created by felix on 12/18/25.
//

#ifndef ECS_NET_SERIALIZATION_H
#define ECS_NET_SERIALIZATION_H

#include <ecs_history/static_entity.hpp>

#include "component_serialization.hpp"
#include "change_serialization.hpp"
#include <entt/entt.hpp>

#include "commit.hpp"
#include "entity_version.hpp"

namespace ecs_net::serialization {

template<typename Archive>
void serialize_storage(Archive &archive,
                       const entt::basic_sparse_set<> &storage,
                       const ecs_history::static_entities_t &static_entities) {
    const auto meta = entt::resolve(storage.info().hash());
    archive(static_cast<uint64_t>(storage.info().hash()));
    archive(static_cast<uint32_t>(storage.size()));
    for (const auto &entt : storage) {
        ecs_history::static_entity_t static_entity = static_entities.get_static_entity(entt);
        archive(static_entity);
        const void *value = storage.value(entt);
        const entt::meta_any any = meta.from_void(value);
        serialize_component<Archive, true>(archive, any);
    }
}

template<typename Archive, traits_t traits = traits_t::NO>
void serialize_registry(Archive &archive, entt::registry &reg) {
    const auto &static_entities = reg.ctx().get<ecs_history::static_entities_t>();
    auto &version_handler = reg.ctx().get<entity_version_handler_t>();

    if constexpr (traits != traits_t::NO) {
        const uint16_t numSets = std::ranges::count_if(reg.storage(),
                                                       [](const auto &p) {
                                                           const entt::meta_type type =
                                                               entt::resolve(p.first);
                                                           const auto type_traits = type.traits<
                                                               traits_t>();
                                                           return !!(type_traits & traits);
                                                       });
        archive(numSets);
    } else {
        uint16_t storages = std::distance(reg.storage().begin(), reg.storage().end());
        archive(storages);
    }

    for (auto [id, storage] : reg.storage()) {
        const auto meta = entt::resolve(id);
        if (!meta) {
            continue;
        }
        if constexpr (traits != traits_t::NO) {
            if (!(meta.traits<traits_t>() & traits)) {
                continue;
            }
        }
        serialize_storage(archive, storage, static_entities, version_handler);
    }

    const uint32_t entities = reg.storage<entt::entity>().size();
    archive(entities);
    for (const auto &entt : reg.storage<entt::entity>()) {
        const ecs_history::static_entity_t static_entity = static_entities.get_static_entity(entt);
        const entity_version_t version = version_handler.get_version(static_entity);
        archive(static_entity);
        archive(version);
    }
}

template<typename Archive>
void serialize_commit(Archive &archive, commit_t &commit) {
    uint32_t entity_version_count = commit.entity_versions.size();
    archive(entity_version_count);
    for (const auto &[static_entity, version] : commit.entity_versions) {
        archive(static_entity);
        archive(version);
    }
    uint32_t created_entity_count = commit.created_entities.size();
    archive(created_entity_count);
    for (const ecs_history::static_entity_t &created_entity : commit.created_entities) {
        archive(created_entity);
    }
    uint16_t change_sets = commit.change_sets.size();
    archive(change_sets);
    change_serializer<Archive> serializer{archive};
    for (const auto &change_set : commit.change_sets) {
        archive(change_set->id);
        uint32_t count = change_set->count();
        archive(count);
        change_set->supply(serializer);
    }
    uint32_t deleted_entity_count = commit.destroyed_entities.size();
    archive(deleted_entity_count);
    for (const ecs_history::static_entity_t &deleted_entity : commit.destroyed_entities) {
        archive(deleted_entity);
    }
}

template<typename Archive>
std::unordered_map<ecs_history::static_entity_t, entity_version_t>
deserialize_commit_entity_versions(Archive &archive) {
    std::unordered_map<ecs_history::static_entity_t, entity_version_t> entity_versions;
    uint32_t entity_version_count;
    archive(entity_version_count);
    for (int i = 0; i < entity_version_count; ++i) {
        ecs_history::static_entity_t static_entity;
        archive(static_entity);
        entity_version_t version;
        archive(version);
        entity_versions[static_entity] = version;
    }
    return entity_versions;
}

template<typename Archive>
std::vector<ecs_history::static_entity_t> deserialize_entity_list(Archive &archive) {
    uint32_t entity_count;
    archive(entity_count);
    auto static_entities = std::vector<ecs_history::static_entity_t>(entity_count);
    for (int i = 0; i < entity_count; ++i) {
        archive(static_entities[i]);
    }
    return static_entities;
}

template<typename Archive, typename Type>
std::unique_ptr<ecs_history::base_change_set_t> deserialize_change_set(Archive &archive) {
    auto change_set = std::make_unique<ecs_history::change_set_t<Type> >();
    uint32_t count;
    archive(count);
    for (uint32_t i = 0; i < count; ++i) {
        std::unique_ptr<ecs_history::component_change_t<Type> > change = deserialize_change<
            Archive, Type>(archive);
        change_set->add_change(std::move(change));
    }
    return std::move(change_set);
}

template<typename Archive>
std::vector<std::unique_ptr<ecs_history::base_change_set_t> > deserialize_commit_changes(
    Archive &archive) {
    std::vector<std::unique_ptr<ecs_history::base_change_set_t> > change_sets;
    uint16_t change_set_count;
    archive(change_set_count);
    for (uint16_t i = 0; i < change_set_count; ++i) {
        entt::id_type id;
        archive(id);
        entt::meta_type type = entt::resolve(id);
        auto deserialize_changes_func = type.func("deserialize_change_set"_hs);
        if (!deserialize_changes_func) {
            const auto type_name = std::string{type.info().name()};
            throw std::runtime_error(
                "could not find deserialize change set function for type " + type_name);
        }
        auto change_set = deserialize_changes_func.invoke({}, entt::forward_as_meta(archive));
        if (!change_set) {
            throw std::runtime_error("failed to deserialize change set");
        }
        std::unique_ptr<ecs_history::base_change_set_t> &changes = change_set.template cast<
            std::unique_ptr<ecs_history::base_change_set_t> &>();
        change_sets.push_back(std::move(changes));
    }
    return change_sets;
}

template<typename Archive>
commit_t deserialize_commit(Archive &archive) {
    return commit_t{
        serialization::deserialize_commit_entity_versions(archive),
        serialization::deserialize_entity_list(archive),
        serialization::deserialize_commit_changes(archive),
        serialization::deserialize_entity_list(archive)};
}
}

#endif //ECS_NET_SERIALIZATION_H
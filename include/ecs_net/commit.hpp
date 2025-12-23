//
// Created by felix on 12/20/25.
//

#ifndef ECS_HISTORY_COMMIT_HPP
#define ECS_HISTORY_COMMIT_HPP

#include <utility>
#include <vector>
#include <random>

#include "ecs_history/change_set.hpp"
#include "ecs_net/entity_version.hpp"

namespace ecs_net {

struct commit_id {
    uint64_t part1;
    uint64_t part2;

    commit_id(const uint64_t part1, const uint64_t part2) : part1{part1}, part2{part2} {
    }

    bool operator==(const commit_id &other) const {
        return part1 == other.part1 && part2 == other.part2;
    }

    bool operator!=(const commit_id &other) const {
        return !(*this == other);
    }

    template<class Archive>
    void serialize(Archive &ar) {
        ar(part1, part2);
    }
};

class commit_id_generator_t {
    std::random_device rd;

    std::mt19937_64 gen;

public:
    commit_id_generator_t() : gen{rd()} {
    }

    commit_id next() {
        return commit_id{gen(), gen()};
    }
};

struct commit_t {
    bool undo = false;
    std::unordered_map<ecs_history::static_entity_t, entity_version_t> entity_versions;
    std::vector<ecs_history::static_entity_t> created_entities;
    std::vector<std::unique_ptr<ecs_history::base_change_set_t> > change_sets;
    std::vector<ecs_history::static_entity_t> destroyed_entities;

    commit_t() = default;

    commit_t(std::unordered_map<ecs_history::static_entity_t, entity_version_t> entity_versions,
             std::vector<ecs_history::static_entity_t> created_entities,
             std::vector<std::unique_ptr<ecs_history::base_change_set_t> > change_sets,
             std::vector<ecs_history::static_entity_t> destroyed_entities)
        : entity_versions(std::move(entity_versions)),
          created_entities(std::move(created_entities)),
          change_sets(std::move(change_sets)),
          destroyed_entities(std::move(destroyed_entities)) {
    }

    commit_t(commit_t &commit) = delete;

    commit_t &operator=(commit_t &commit) = delete;

    commit_t(commit_t &&commit) = default;

    commit_t invert() {
        commit_t inverted_commit{};
        inverted_commit.created_entities = this->destroyed_entities;
        inverted_commit.destroyed_entities = this->created_entities;
        for (const auto &base_change_set : this->change_sets) {
            inverted_commit.change_sets.push_back(base_change_set->invert());
        }
        inverted_commit.undo = !this->undo;
        if (this->undo) {
            for (auto [entity, version] : this->entity_versions) {
                inverted_commit.entity_versions[entity] = --version;
            }
        } else {
            for (auto [entity, version] : this->entity_versions) {
                inverted_commit.entity_versions[entity] = ++version;
            }
        }
        return inverted_commit;
    }
};
}

#endif //ECS_HISTORY_COMMIT_HPP
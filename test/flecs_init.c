//
// Created by felix on 18.10.25.
//

#include <flecs-net.h>
#include <stdio.h>

typedef struct {
    int x, y;
} TestComponent;


void lol(ecs_iter_t *it) {
    printf("t");
}

int flecs_init() {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, TestComponent);
    ecs_entity_t Updated = ecs_new(world);

    const ecs_entity_t *entities = ecs_bulk_new_w_id(world, FLECS_IDTestComponentID_, 10);

    ECS_SYSTEM(world, lol, EcsOnUpdate, TestComponent);

    printf("test");

    return 1;
}
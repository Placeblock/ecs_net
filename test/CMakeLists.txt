set (TestList
    flecs_init.c
)

create_test_sourcelist(Tests FlecsNetTests.c ${TestList})
add_executable(FlecsNetTests ${Tests})
target_link_libraries(FlecsNetTests PRIVATE ecs_net)

foreach (test ${TestList})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${TName} COMMAND FlecsNetTests ${TName})
endforeach ()
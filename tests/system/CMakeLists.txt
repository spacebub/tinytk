ttk_test(system_text Text.cpp)
ttk_test(system_env Env.cpp Paths.cpp)
ttk_test(system_process Process.cpp)
ttk_test(system_util Format.cpp Desktop.cpp)

if (TTK_JSON)
    ttk_test(system_json Json.cpp)
    target_link_libraries(ttk_test_system_json PRIVATE ttk::json)
endif ()

if (TTK_HTTP)
    ttk_test(system_http Http.cpp)
    target_link_libraries(ttk_test_system_http PRIVATE ttk::http)
endif ()

if (TTK_NOTIFY)
    ttk_test(system_notify Notify.cpp)
    target_link_libraries(ttk_test_system_notify PRIVATE ttk::notify)
endif ()

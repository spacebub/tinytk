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
    # Windows and macOS build the stub in place of the D-Bus client.
    if (WIN32 OR APPLE)
        ttk_test(system_notify Notify_Stub.cpp)
    else ()
        ttk_test(system_notify Notify.cpp)
    endif ()

    target_link_libraries(ttk_test_system_notify PRIVATE ttk::notify)
endif ()

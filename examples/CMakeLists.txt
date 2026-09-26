# A windowed build on Windows expects WinMain. Nothing defines one, so the entry
# point is named instead.
function(ttk_example target source name)
    add_executable(${target}
            ${source}
            ${TTK_SANITIZER_SOURCES})

    target_link_libraries(${target} PRIVATE ttk::ui)

    target_compile_options(${target} PRIVATE ${TTK_COMPILE_FLAGS})

    set_target_properties(${target} PROPERTIES
            OUTPUT_NAME ${name}
            WIN32_EXECUTABLE ON)

    if (MSVC)
        target_link_options(${target} PRIVATE /ENTRY:mainCRTStartup)
    endif ()

    if (TTK_SANITIZE)
        target_compile_definitions(${target} PRIVATE
                TTK_SANITIZER_LOG="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}")
    endif ()
endfunction()

if (TTK_EXAMPLES)
    ttk_example(ttk_hello hello.cpp hello)
    ttk_example(ttk_gallery gallery.cpp gallery)
endif ()

cmake_minimum_required(VERSION 3.25)
project(tinytk
        VERSION 1.0.0
        DESCRIPTION "A small system and interface toolkit"
        HOMEPAGE_URL "https://github.com/spacebub/tinytk"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

option(TTK_UI "Build ttk::ui, which fetches SDL3 and Blend2D" ON)
option(TTK_JSON "Build ttk::json, which fetches yyjson" ON)
option(TTK_HTTP "Build ttk::http, which needs libcurl outside Windows" ON)
option(TTK_NOTIFY "Build ttk::notify, desktop notifications over the session bus" ON)
option(TTK_EXAMPLES "Build the example programs" ${PROJECT_IS_TOP_LEVEL})
option(TTK_GALLERY "Build the gallery, which shows every widget" OFF)
option(TTK_BENCHMARKS "Build the benchmark suite in bench/" OFF)
option(TTK_SANITIZE "Build with the address and undefined sanitizers" OFF)

if (MSVC)
    set(TTK_COMPILE_FLAGS
            "/W4"
            "/WX"
            "/MP"
            "/GS"
            "$<$<CONFIG:RELEASE>:/O1;/GL;/Gw;/Gy>")
else ()
    set(TTK_COMPILE_FLAGS
            "-Wall"
            "-Wextra"
            "-Werror"
            "-Wpedantic"
            "-fstack-protector-strong"
            "$<$<CONFIG:DEBUG>:-Og>"
            "$<$<CONFIG:RELEASE>:-Os;-ffunction-sections;-fdata-sections;-fvisibility=hidden>")

    if (NOT APPLE)
        list(APPEND TTK_COMPILE_FLAGS "$<$<CONFIG:RELEASE>:-fno-plt>")
    endif ()
endif ()

# The binaries are the consumer's, so what strips them is only set up when this is one.
if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

    if (MSVC)
        add_link_options("$<$<CONFIG:RELEASE>:/LTCG;/OPT:REF;/OPT:ICF;/INCREMENTAL:NO;/DEBUG:NONE>")
    elseif (APPLE)
        add_link_options("$<$<CONFIG:RELEASE>:-Wl,-dead_strip>")
    else ()
        add_link_options("$<$<CONFIG:RELEASE>:-Wl,--gc-sections;-Wl,--as-needed;-Wl,-O1;-s>")
    endif ()
endif ()

add_library(ttk_sanitizers INTERFACE)

if (TTK_SANITIZE)
    set(SANITIZER_FLAGS -fsanitize=address -fsanitize=undefined)

    # -Og folds frames, which misattributes sanitizer stacks.
    target_compile_options(ttk_sanitizers INTERFACE
            ${SANITIZER_FLAGS}
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
            -g)

    target_link_options(ttk_sanitizers INTERFACE ${SANITIZER_FLAGS})

    # Runtime options and suppressions are compiled in. ASAN_OPTIONS still overrides.
    # It defines symbols, so it is a source of the binary rather than part of the interface target.
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/lsan.supp SANITIZER_SUPPRESSIONS)
    configure_file(cmake/Sanitizers.cpp.in ${PROJECT_BINARY_DIR}/Sanitizers.cpp @ONLY)

    set(TTK_SANITIZER_SOURCES ${PROJECT_BINARY_DIR}/Sanitizers.cpp CACHE INTERNAL "")
else ()
    set(TTK_SANITIZER_SOURCES "" CACHE INTERNAL "")
endif ()

# One self-contained binary. This keeps every fetched library static.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(cmake/DownloadCache.cmake)
include(cmake/Embed.cmake)

add_subdirectory(src)

if ((TTK_EXAMPLES OR TTK_GALLERY) AND TTK_UI)
    add_subdirectory(examples)
endif ()

if (TTK_BENCHMARKS AND TTK_UI)
    add_subdirectory(bench)
endif ()

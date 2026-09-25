include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "" FORCE)

set(TTK_BENCHMARK_VERSION v1.9.5)

ttk_cache_source(benchmark https://github.com/google/benchmark.git ${TTK_BENCHMARK_VERSION})

FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG ${TTK_BENCHMARK_VERSION}
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
        SYSTEM)

FetchContent_MakeAvailable(benchmark)

add_executable(ttk_bench
        Main.cpp

        support/Canvas.cpp
        support/Canvas.h
        support/Fixtures.cpp
        support/Fixtures.h
        support/Sandbox.cpp
        support/Sandbox.h

        system/Text_bench.cpp

        ui/Anim_bench.cpp
        ui/Controls_bench.cpp
        ui/Glyphs_bench.cpp
        ui/Layout_bench.cpp
        ui/Painter_bench.cpp
        ui/ReorderGrid_bench.cpp
        ui/TextEdit_bench.cpp
        ui/TextView_bench.cpp
        ui/Typeface_bench.cpp

        ${TTK_SANITIZER_SOURCES})

target_include_directories(ttk_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (TTK_JSON)
    target_sources(ttk_bench PRIVATE system/Json_bench.cpp)
    target_link_libraries(ttk_bench PRIVATE ttk::json)
endif ()

target_link_libraries(ttk_bench PRIVATE
        ttk::ui
        benchmark::benchmark)

if (TTK_SANITIZE)
    target_compile_definitions(ttk_bench PRIVATE
            TTK_SANITIZER_LOG="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ttk_bench")
endif ()

target_compile_options(ttk_bench PRIVATE ${TTK_COMPILE_FLAGS})

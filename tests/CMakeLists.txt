include(FetchContent)
include(GoogleTest)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

set(TTK_GOOGLETEST_VERSION v1.17.0)

ttk_cache_source(googletest https://github.com/google/googletest.git ${TTK_GOOGLETEST_VERSION})

FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG ${TTK_GOOGLETEST_VERSION}
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
        SYSTEM)

FetchContent_MakeAvailable(googletest)

add_library(ttk_test_support STATIC
        support/Canvas.cpp
        support/Canvas.h
        support/Main.cpp
        support/Sandbox.cpp
        support/Sandbox.h)
target_include_directories(ttk_test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ttk_test_support PUBLIC ttk::ui GTest::gtest)
target_compile_options(ttk_test_support PRIVATE ${TTK_COMPILE_FLAGS})

# One program per area. Each TEST in it is registered with ctest on its own, so `ctest -j` runs them in parallel.
function(ttk_test name)
    add_executable(ttk_test_${name} ${ARGN} ${TTK_SANITIZER_SOURCES})
    target_link_libraries(ttk_test_${name} PRIVATE ttk_test_support)
    target_compile_options(ttk_test_${name} PRIVATE ${TTK_COMPILE_FLAGS})
    gtest_discover_tests(ttk_test_${name} TEST_PREFIX "${name}." DISCOVERY_MODE PRE_TEST)
endfunction()

ttk_test(paint Paint.cpp)

add_subdirectory(draw)
add_subdirectory(toolkit)
add_subdirectory(layout)
add_subdirectory(controls)
add_subdirectory(widgets)
add_subdirectory(overlays)
add_subdirectory(system)

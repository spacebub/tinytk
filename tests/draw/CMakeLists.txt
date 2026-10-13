ttk_test(draw_damage Damage.cpp)
ttk_test(draw_surface Surface.cpp Pixels.h)
ttk_test(draw_typeface Typeface.cpp Pixels.h)
ttk_test(draw_glyphs Glyphs.cpp Pixels.h)
ttk_test(draw_paint Paint.cpp Pixels.h)
ttk_test(draw_svg Svg.cpp Pixels.h)
ttk_test(draw_anim Anim.cpp)
ttk_test(draw_theme Theme.cpp)
ttk_test(draw_cache Cache.cpp)
ttk_test(draw_platform Platform.cpp)

# The ring is driven against a compositor of our own, made of libwayland-server.
if (TTK_WLSHM)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_TEST REQUIRED IMPORTED_TARGET wayland-server wayland-client)

    ttk_test(draw_wlshm WlShm.cpp Compositor.cpp Compositor.h)
    target_link_libraries(ttk_test_draw_wlshm PRIVATE PkgConfig::WAYLAND_TEST)
endif ()

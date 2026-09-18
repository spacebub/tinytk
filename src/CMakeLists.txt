include(FetchContent)

# Includes are written relative to src/.
set(TTK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(TTK_YYJSON_VERSION 0.13.0)

set(YYJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_MISC OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_FUZZER OFF CACHE BOOL "" FORCE)
set(YYJSON_INSTALL OFF CACHE BOOL "" FORCE)

ttk_cache_source(yyjson https://github.com/ibireme/yyjson.git ${TTK_YYJSON_VERSION})

FetchContent_Declare(yyjson
        GIT_REPOSITORY https://github.com/ibireme/yyjson.git
        GIT_TAG ${TTK_YYJSON_VERSION}
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
        SYSTEM)

FetchContent_MakeAvailable(yyjson)

add_library(ttk_system STATIC
        ttk/system/Env.cpp
        ttk/system/Env.h
        ttk/system/Http.cpp
        ttk/system/Http.h
        ttk/system/Json.cpp
        ttk/system/Json.h
        ttk/system/Paths.cpp
        ttk/system/Paths.h
        ttk/system/Process.cpp
        ttk/system/Process.h
        ttk/system/Text.cpp
        ttk/system/Text.h)

add_library(ttk::system ALIAS ttk_system)

target_include_directories(ttk_system PUBLIC ${TTK_INCLUDE_DIR})

target_link_libraries(ttk_system
        PUBLIC
        yyjson
        ttk_sanitizers)

if (WIN32)
    target_link_libraries(ttk_system PRIVATE winhttp)
else ()
    find_package(CURL REQUIRED)

    target_link_libraries(ttk_system PRIVATE CURL::libcurl)
endif ()

target_compile_options(ttk_system PRIVATE ${TTK_COMPILE_FLAGS})

if (NOT TTK_UI)
    return()
endif ()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

foreach (part AUDIO CAMERA JOYSTICK HAPTIC HIDAPI SENSOR POWER GPU DIALOG)
    set(SDL_${part} OFF CACHE BOOL "" FORCE)
endforeach ()

set(TTK_SDL_VERSION release-3.2.24)

ttk_cache_source(SDL3 https://github.com/libsdl-org/SDL.git ${TTK_SDL_VERSION})

# SDL_RENDER and OpenGL stay on: only the Windows and X11 video drivers implement a
# framebuffer of their own, and everywhere else SDL_GetWindowSurface falls back to
# SDL_CreateWindowTexture, which is the 2D render API over a GPU texture.
FetchContent_Declare(SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG ${TTK_SDL_VERSION}
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
        SYSTEM)

FetchContent_MakeAvailable(SDL3)

# Blend2D bundles asmjit rather than linking one, so this populates the source and
# stops. A SOURCE_SUBDIR holding no CMakeLists.txt is the documented way to say so.
set(TTK_ASMJIT_VERSION e24347521858616d2d070375ad0d52512e6e44ba)

ttk_cache_source(asmjit https://github.com/asmjit/asmjit.git ${TTK_ASMJIT_VERSION})

FetchContent_Declare(asmjit
        GIT_REPOSITORY https://github.com/asmjit/asmjit.git
        GIT_TAG ${TTK_ASMJIT_VERSION}
        SOURCE_SUBDIR populate-only)

FetchContent_MakeAvailable(asmjit)

set(ASMJIT_DIR "${asmjit_SOURCE_DIR}" CACHE PATH "" FORCE)
set(BLEND2D_STATIC ON CACHE BOOL "" FORCE)
set(BLEND2D_TEST OFF CACHE BOOL "" FORCE)
set(BLEND2D_NO_INSTALL ON CACHE BOOL "" FORCE)
set(BLEND2D_NO_JIT_LOGGING ON CACHE BOOL "" FORCE)

set(TTK_BLEND2D_VERSION 58ca9460b4138af6e793184682e9324e0a8053b2)  # v0.21.3

ttk_cache_source(blend2d https://github.com/blend2d/blend2d.git ${TTK_BLEND2D_VERSION})

FetchContent_Declare(blend2d
        GIT_REPOSITORY https://github.com/blend2d/blend2d.git
        GIT_TAG ${TTK_BLEND2D_VERSION}
        EXCLUDE_FROM_ALL
        SYSTEM)

FetchContent_MakeAvailable(blend2d)

add_library(ttk_ui STATIC
        # draw: the rasteriser, and what is drawn with it
        ttk/draw/Anim.cpp
        ttk/draw/Anim.h
        ttk/draw/Chrome.h
        ttk/draw/Damage.cpp
        ttk/draw/Damage.h
        ttk/draw/Glyphs.cpp
        ttk/draw/Glyphs.h
        ttk/draw/Paint.cpp
        ttk/draw/Paint.h
        ttk/draw/Surface.cpp
        ttk/draw/Surface.h
        ttk/draw/Svg.cpp
        ttk/draw/Svg.h
        ttk/draw/Theme.cpp
        ttk/draw/Theme.h
        ttk/draw/Typeface.cpp
        ttk/draw/Typeface.h

        # toolkit: the widget layer
        ttk/toolkit/Compose.cpp
        ttk/toolkit/Compose.h
        ttk/toolkit/Event.h
        ttk/toolkit/Painter.cpp
        ttk/toolkit/Painter.h
        ttk/toolkit/Root.cpp
        ttk/toolkit/Root.h
        ttk/toolkit/Widget.cpp
        ttk/toolkit/Widget.h

        # toolkit/layout: what places other widgets
        ttk/toolkit/layout/Box.cpp
        ttk/toolkit/layout/Box.h
        ttk/toolkit/layout/CardGrid.cpp
        ttk/toolkit/layout/CardGrid.h
        ttk/toolkit/layout/Collapsible.h
        ttk/toolkit/layout/Pair.cpp
        ttk/toolkit/layout/Pair.h
        ttk/toolkit/layout/Panel.cpp
        ttk/toolkit/layout/Panel.h
        ttk/toolkit/layout/Picture.cpp
        ttk/toolkit/layout/Picture.h
        ttk/toolkit/layout/ReorderGrid.h
        ttk/toolkit/layout/Rule.h
        ttk/toolkit/layout/Scroll.cpp
        ttk/toolkit/layout/Scroll.h
        ttk/toolkit/layout/Spacer.h
        ttk/toolkit/layout/Wrap.cpp
        ttk/toolkit/layout/Wrap.h

        # toolkit/controls: what a page is built from
        ttk/toolkit/controls/Button.cpp
        ttk/toolkit/controls/Button.h
        ttk/toolkit/controls/Card.cpp
        ttk/toolkit/controls/Card.h
        ttk/toolkit/controls/Check.cpp
        ttk/toolkit/controls/Check.h
        ttk/toolkit/controls/Chip.cpp
        ttk/toolkit/controls/Chip.h
        ttk/toolkit/controls/Fact.cpp
        ttk/toolkit/controls/Fact.h
        ttk/toolkit/controls/Field.cpp
        ttk/toolkit/controls/Field.h
        ttk/toolkit/controls/GlyphButton.cpp
        ttk/toolkit/controls/GlyphButton.h
        ttk/toolkit/controls/Label.cpp
        ttk/toolkit/controls/Label.h
        ttk/toolkit/controls/MultistateSwitch.cpp
        ttk/toolkit/controls/MultistateSwitch.h
        ttk/toolkit/controls/Pill.cpp
        ttk/toolkit/controls/Pill.h
        ttk/toolkit/controls/Select.cpp
        ttk/toolkit/controls/Select.h
        ttk/toolkit/controls/StatusIndicator.cpp
        ttk/toolkit/controls/StatusIndicator.h
        ttk/toolkit/controls/Stepper.cpp
        ttk/toolkit/controls/Stepper.h
        ttk/toolkit/controls/TabStrip.cpp
        ttk/toolkit/controls/TabStrip.h
        ttk/toolkit/controls/TextBox.cpp
        ttk/toolkit/controls/TextBox.h
        ttk/toolkit/controls/TextView.cpp
        ttk/toolkit/controls/TextView.h
        ttk/toolkit/controls/Toggle.cpp
        ttk/toolkit/controls/Toggle.h

        # toolkit/overlays: what comes up over a page
        ttk/toolkit/overlays/Dialog.cpp
        ttk/toolkit/overlays/Dialog.h
        ttk/toolkit/overlays/Menu.cpp
        ttk/toolkit/overlays/Menu.h
        ttk/toolkit/overlays/Tips.cpp
        ttk/toolkit/overlays/Tips.h

        # util: leaf helpers, with nothing above them
        ttk/util/Cache.h
        ttk/util/Clipboard.cpp
        ttk/util/Clipboard.h
        ttk/util/Clock.h
        ttk/util/Desktop.cpp
        ttk/util/Desktop.h
        ttk/util/Format.cpp
        ttk/util/Format.h
        ttk/util/Window.h

        # shell: the window and the frame loop
        ttk/shell/Shell.cpp
        ttk/shell/Shell.h

        # dialogs: the modal ones every application asks
        ttk/dialogs/ConfirmDialog.cpp
        ttk/dialogs/ConfirmDialog.h
        ttk/dialogs/DialogLayer.cpp
        ttk/dialogs/DialogLayer.h
        ttk/dialogs/FilePicker.cpp
        ttk/dialogs/FilePicker.h
        ttk/dialogs/FilePickerDialog.cpp
        ttk/dialogs/FilePickerDialog.h
        ttk/dialogs/PromptDialog.cpp
        ttk/dialogs/PromptDialog.h

        # notices: the toasts and what feeds them
        ttk/notices/Notice.h
        ttk/notices/Notifier.cpp
        ttk/notices/Notifier.h
        ttk/notices/Toasts.cpp
        ttk/notices/Toasts.h)

add_library(ttk::ui ALIAS ttk_ui)

if (WIN32)
    target_sources(ttk_ui PRIVATE ttk/draw/Chrome_Win32.cpp)
    target_link_libraries(ttk_ui PUBLIC dwmapi shell32 user32)
else ()
    target_sources(ttk_ui PRIVATE ttk/draw/Chrome_Stub.cpp)
endif ()

target_link_libraries(ttk_ui
        PUBLIC
        ttk_system
        blend2d::blend2d
        SDL3::SDL3-static)

target_compile_options(ttk_ui PRIVATE ${TTK_COMPILE_FLAGS})

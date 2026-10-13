add_executable(ttk_paint_test Paint.cpp ${TTK_SANITIZER_SOURCES})
target_link_libraries(ttk_paint_test PRIVATE ttk::ui)
target_compile_options(ttk_paint_test PRIVATE ${TTK_COMPILE_FLAGS})
add_test(NAME paint_correctness COMMAND ttk_paint_test)

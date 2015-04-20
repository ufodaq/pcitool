include_directories(
    ${CMAKE_SOURCE_DIR}
)

link_directories(${UFODECODE_LIBRARY_DIRS})

add_executable(xilinx xilinx.c)
target_link_libraries(xilinx pcilib rt)

add_executable(lorenzo_ipedma_test lorenzo_ipedma_test.c)
target_link_libraries(lorenzo_ipedma_test pcilib rt)
set_target_properties(lorenzo_ipedma_test PROPERTIES COMPILE_FLAGS -Wno-array-bounds)

add_executable(pio_test pio_test.c)
target_link_libraries(pio_test pcilib rt)

add_executable(compare_to_value compare_to_value.c)

add_executable(heb_strip_bad_values heb_strip_bad_values.c)
add_executable(check_counter check_counter.c)

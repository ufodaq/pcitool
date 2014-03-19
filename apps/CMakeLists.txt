include_directories(
    ${CMAKE_SOURCE_DIR}
)

link_directories(${UFODECODE_LIBRARY_DIRS})

add_executable(xilinx xilinx.c)
target_link_libraries(xilinx pcilib rt)

add_executable(pio_test pio_test.c)
target_link_libraries(pio_test pcilib rt)

add_executable(compare_to_value compare_to_value.c)

add_executable(heb_strip_bad_values heb_strip_bad_values.c)

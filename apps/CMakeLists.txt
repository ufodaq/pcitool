include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
)

link_directories(
    ${CMAKE_BINARY_DIR}/pcilib
)

add_executable(xilinx xilinx.c)
target_link_libraries(xilinx pcilib rt)

add_executable(pio_test pio_test.c)
target_link_libraries(pio_test pcilib rt)

add_executable(compare_to_value compare_to_value.c)

add_executable(heb_strip_bad_values heb_strip_bad_values.c)
add_executable(check_counter check_counter.c)

add_executable(test_multithread test_multithread.c)
target_link_libraries (test_multithread pcilib ${CMAKE_THREAD_LIBS_INIT})

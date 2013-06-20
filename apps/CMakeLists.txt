include_directories(
    ${CMAKE_SOURCE_DIR}
)

link_directories(${UFODECODE_LIBRARY_DIRS})

add_executable(xilinx xilinx.c)
target_link_libraries(xilinx pcilib rt)

add_executable(xilinx2 xilinx2.c)
target_link_libraries(xilinx2 pcilib rt)

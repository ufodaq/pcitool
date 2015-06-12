include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
)

set(HEADERS ${HEADERS} default.h software_registers.h)

add_library(protocols STATIC default.c software_registers.c)

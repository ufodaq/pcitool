include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
)

set(HEADERS ${HEADERS} default.h)

add_library(protocols STATIC default.c)

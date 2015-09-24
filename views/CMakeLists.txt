include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
)

set(HEADERS ${HEADERS} enum.h transform.h)

add_library(views STATIC enum.c transform.c)

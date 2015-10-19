include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIR}
    ${UTHASH_INCLUDE_DIRS}
)

set(HEADERS ${HEADERS} default.h software.h property.h)

add_library(protocols STATIC default.c software.c property.c)

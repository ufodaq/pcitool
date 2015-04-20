include_directories(
    ${CMAKE_SOURCE_DIR}
)


set(HEADERS ${HEADERS} default.h)

add_library(protocols STATIC default.c)

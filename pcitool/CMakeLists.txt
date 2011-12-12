include_directories(
    ${CMAKE_SOURCE_DIR}
)

set(HEADERS ${HEADERS} sysinfo.h)

add_library(pcitool STATIC sysinfo.c)


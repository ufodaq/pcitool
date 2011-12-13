include_directories(
    ${CMAKE_SOURCE_DIR}
)

set(HEADERS ${HEADERS} sysinfo.h formaters.h)

add_library(pcitool STATIC sysinfo.c formaters.c)


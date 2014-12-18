include_directories(
    ${CMAKE_SOURCE_DIR}
    ${UFODECODE_INCLUDE_DIRS}
)

set(HEADERS ${HEADERS} model.h)

add_library(kapture STATIC kapture.c)


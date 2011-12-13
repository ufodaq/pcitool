include_directories(
    ${CMAKE_SOURCE_DIR}
)

set(HEADERS ${HEADERS} ipecamera.h model.h reader.h events.h data.h public.h private.h)

add_library(ipecamera STATIC ipecamera.c model.c reader.c events.c data.c)

install(FILES ipecamera.h
    DESTINATION include
)

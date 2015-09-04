include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
)

set(HEADERS ${HEADERS} nwl.h nwl_private.h nwl_engine.h nwl_irq.h nwl_loopback.h ipe.h ipe_private.h)

add_library(dma STATIC nwl.c nwl_engine.c nwl_irq.c nwl_loopback.c ipe.c ipe_benchmark.c)

#set(HEADERS ${HEADERS} ipe.h ipe_private.h)
#add_library(dma STATIC  ipe.c)

include_directories(
    ${CMAKE_SOURCE_DIR}
)


set(HEADERS ${HEADERS} nwl.h nwl_dma.h nwl_engine.h nwl_irq.h nwl_loopback.h nwl_register.h)

add_library(dma STATIC nwl.c nwl_engine.c nwl_irq.c nwl_loopback.c nwl_register.c)


include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
)

set(HEADERS pcilib.h pci.h export.h model.h bank.h register.h kmem.h irq.h dma.h event.h plugin.h tools.h error.h debug.h config.h)
add_library(pcilib SHARED pci.c export.c model.c bank.c register.c kmem.c irq.c dma.c event.c plugin.c tools.c error.c debug.c)
target_link_libraries(pcilib dma protocols ${CMAKE_THREAD_LIBS_INIT} ${UFODECODE_LIBRARIES} ${CMAKE_DL_LIBS})
add_dependencies(pcilib dma protocols)

install(TARGETS pcilib
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(FILES pcilib.h
    DESTINATION include
)

install(FILES bank.h register.h dma.h event.h model.h error.h debug.h tools.h export.h
    DESTINATION include/pcilib
)

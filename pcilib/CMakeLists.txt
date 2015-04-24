include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
)

set(HEADERS pcilib.h pci.h config.h model.h bank.h register.h kmem.h irq.h dma.h event.h tools.h error.h)
add_library(pcilib SHARED pci.c config.c model.c bank.c register.c kmem.c irq.c dma.c event.c tools.c error.c)
target_link_libraries(pcilib dma protocols ${CMAKE_THREAD_LIBS_INIT} ${UFODECODE_LIBRARIES} )
add_dependencies(pcilib dma protocols)

install(TARGETS pcilib
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(FILES pcilib.h
    DESTINATION include
)

install(FILES bank.h register.h dma.h event.h model.h error.h tools.h config.h
    DESTINATION include/pcilib
)

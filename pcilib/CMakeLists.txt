include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
)

set(HEADERS pcilib.h pci.h export.h bar.h fifo.h model.h bank.h register.h
views.h xml.h kmem.h irq.h locking.h lock.h dma.h event.h plugin.h tools.h error.h
debug.h env.h version.h config.h unit.h)
add_library(pcilib SHARED pci.c export.c bar.c fifo.c model.c bank.c
register.c views.c xml.c kmem.c irq.c locking.c lock.c dma.c event.c plugin.c tools.c error.c debug.c env.c unit.c)
target_link_libraries(pcilib dma protocols ${CMAKE_THREAD_LIBS_INIT}
${UFODECODE_LIBRARIES} ${CMAKE_DL_LIBS} ${EXTRA_SYSTEM_LIBS}
${LIBXML2_LIBRARIES} ${PYTHON_LIBRARIES})
add_dependencies(pcilib dma protocols)

install(TARGETS pcilib
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(FILES pcilib.h
    DESTINATION include
)

install(FILES bar.h kmem.h locking.h lock.h bank.h register.h views.h xml.h dma.h event.h model.h error.h debug.h env.h tools.h export.h version.h
    DESTINATION include/pcilib
)

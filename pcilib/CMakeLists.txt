include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIR}
    ${UTHASH_INCLUDE_DIRS}
)

set(HEADERS pcilib.h pci.h datacpy.h memcpy.h pagecpy.h cpu.h timing.h export.h value.h bar.h fifo.h model.h bank.h register.h view.h property.h unit.h xml.h py.h kmem.h irq.h locking.h lock.h dma.h event.h plugin.h tools.h error.h debug.h env.h config.h version.h build.h)
add_library(pcilib SHARED pci.c datacpy.c memcpy.c pagecpy.c cpu.c timing.c export.c value.c bar.c fifo.c model.c bank.c register.c view.c unit.c property.c xml.c py.c kmem.c irq.c locking.c lock.c dma.c event.c plugin.c tools.c error.c debug.c env.c)
target_link_libraries(pcilib dma protocols views ${CMAKE_THREAD_LIBS_INIT} ${UFODECODE_LIBRARIES} ${CMAKE_DL_LIBS} ${EXTRA_SYSTEM_LIBS} ${LIBXML2_LIBRARIES} ${PYTHON_LIBRARIES})
add_dependencies(pcilib build dma protocols views)

install(TARGETS pcilib
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

install(FILES pcilib.h
    DESTINATION include
)

install(FILES bar.h kmem.h locking.h lock.h bank.h register.h xml.h dma.h event.h model.h error.h debug.h env.h tools.h timing.h cpu.h datacpy.h pagecpy.h memcpy.h export.h view.h unit.h
    DESTINATION include/pcilib
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
    DESTINATION include/pcilib
)

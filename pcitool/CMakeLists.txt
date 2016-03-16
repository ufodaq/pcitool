include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${FASTWRITER_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
)

link_directories(
    ${FASTWRITER_LIBRARY_DIRS}
    ${CMAKE_SOURCE_DIR}/pcilib
)

if (NOT DISABLE_PCITOOL)
    set(HEADERS ${HEADERS} sysinfo.h formaters.h buildinfo.h)
    add_executable(pci cli.c sysinfo.c formaters.c buildinfo.c)
    add_dependencies(pci pcilib)
    target_link_libraries(pci pcilib ${FASTWRITER_LIBRARIES})
    set_target_properties(pci PROPERTIES
	LINK_FLAGS "${CMAKE_THREAD_LIBS_INIT} ${EXTRA_SYSTEM_LIBS}"
    )

#set_target_properties(pci PROPERTIES
#    LINK_FLAGS "-Wl,pcitool/libpcitool.a"
#)

    install(TARGETS pci
	DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif (NOT DISABLE_PCITOOL)

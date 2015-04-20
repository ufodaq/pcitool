project(pcitool)

set(PCILIB_VERSION "0.1.0")
set(PCILIB_ABI_VERSION "1")

cmake_minimum_required(VERSION 2.6)

set(DISABLE_PCITOOL FALSE CACHE BOOL "Build only the library") 


find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

#Check in sibling directory
if (NOT DISABLE_PCITOOL)
    pkg_check_modules(FASTWRITER fastwriter REQUIRED)
endif (NOT DISABLE_PCITOOL)

set(HEADERS pcilib.h pci.h config.h model.h bank.h register.h kmem.h irq.h dma.h event.h tools.h error.h)
add_definitions("-fPIC --std=c99 -Wall -O2 -gdwarf-2 -g3")
#add_definitions("-fPIC --std=c99 -Wall -O2")

add_subdirectory(dma)
add_subdirectory(protocols)
add_subdirectory(pcitool)
add_subdirectory(apps)

include_directories(
    .
    ${FASTWRITER_INCLUDE_DIRS}
)

link_directories(
    ${FASTWRITER_LIBRARY_DIRS}
    ${UFODECODE_LIBRARY_DIRS}
)

add_library(pcilib SHARED pci.c config.c model.c bank.c register.c kmem.c irq.c dma.c event.c tools.c error.c) 
target_link_libraries(pcilib dma protocols ${CMAKE_THREAD_LIBS_INIT} ${UFODECODE_LIBRARIES} )
add_dependencies(pcilib dma protocols pcitool)

set_target_properties(pcilib PROPERTIES
    VERSION ${PCILIB_VERSION}
    SOVERSION ${PCILIB_ABI_VERSION}
)

if (NOT DISABLE_PCITOOL)
    add_executable(pci cli.c)
    add_dependencies(pci pcitool)
    target_link_libraries(pci pcilib pcitool ${FASTWRITER_LIBRARIES})
    set_target_properties(pci PROPERTIES
	LINK_FLAGS ${CMAKE_THREAD_LIBS_INIT}
    )
endif (NOT DISABLE_PCITOOL)

#set_target_properties(pci PROPERTIES
#    LINK_FLAGS "-Wl,pcitool/libpcitool.a"
#)

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif(NOT DEFINED BIN_INSTALL_DIR)


install(TARGETS pcilib
    LIBRARY DESTINATION lib${LIB_SUFFIX}
)

if (NOT DISABLE_PCITOOL)
    install(TARGETS pci
	DESTINATION ${BIN_INSTALL_DIR}
    )
endif (NOT DISABLE_PCITOOL)

install(FILES pcilib.h
    DESTINATION include
)

install(FILES bank.h register.h dma.h event.h model.h error.h tools.h
    DESTINATION include/pcilib
)

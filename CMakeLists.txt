project(pcitool)

set(PCILIB_VERSION "0.0.1")
set(PCILIB_ABI_VERSION "0")

cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig REQUIRED)

#Check in sibling directory
pkg_check_modules(UFODECODE ufodecode REQUIRED)

set(HEADERS pcilib.h pci.h register.h kmem.h irq.h dma.h event.h default.h tools.h error.h)
add_definitions("-fPIC --std=c99 -Wall -O2 -pthread")

add_subdirectory(dma)
add_subdirectory(ipecamera)
add_subdirectory(pcitool)

add_library(pcilib SHARED pci.c register.c kmem.c irq.c dma.c event.c default.c tools.c error.c) 
target_link_libraries(pcilib ufodecode dma ipecamera)
add_dependencies(pcilib dma ipecamera)

set_target_properties(pcilib PROPERTIES
    VERSION ${PCILIB_VERSION}
    SOVERSION ${PCILIB_ABI_VERSION}
    LINK_FLAGS "-pthread"
#    LINK_FLAGS "-pthread -Wl,--whole-archive,dma/libdma.a,ipecamera/libipecamera.a,--no-whole-archive"
)

add_executable(pci cli.c)
add_dependencies(pci pcitool)
target_link_libraries(pci pcilib pcitool)

#set_target_properties(pci PROPERTIES
#    LINK_FLAGS "-Wl,pcitool/libpcitool.a"
#)

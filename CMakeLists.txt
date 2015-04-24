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

add_definitions("-fPIC --std=c99 -Wall -O2 -gdwarf-2 -g3")
#add_definitions("-fPIC --std=c99 -Wall -O2")

add_subdirectory(dma)
add_subdirectory(protocols)
add_subdirectory(pcilib)
add_subdirectory(pcitool)
add_subdirectory(apps)

set_target_properties(pcilib PROPERTIES
    VERSION ${PCILIB_VERSION}
    SOVERSION ${PCILIB_ABI_VERSION}
)





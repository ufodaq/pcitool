project(pcitool)

set(PCILIB_VERSION "0.2.0")
set(PCILIB_ABI_VERSION "2")

cmake_minimum_required(VERSION 2.6)

set(DISABLE_PCITOOL FALSE CACHE BOOL "Build only the library") 

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

#Check in sibling directory
if (NOT DISABLE_PCITOOL)
    pkg_check_modules(FASTWRITER fastwriter REQUIRED)
endif (NOT DISABLE_PCITOOL)

add_definitions("-fPIC --std=c99 -Wall -O2 -gdwarf-2 -g3")
#add_definitions("-fPIC --std=c99 -Wall -O2")

include(cmake/version.cmake)
VERSION_TO_VARS(${PCILIB_VERSION} PCILIB_VERSION_MAJOR PCILIB_VERSION_MINOR PCILIB_VERSION_MICRO)


add_subdirectory(dma)
add_subdirectory(protocols)
add_subdirectory(pcilib)
add_subdirectory(pcitool)
add_subdirectory(apps)

set_target_properties(pcilib PROPERTIES
    VERSION ${PCILIB_VERSION}
    SOVERSION ${PCILIB_ABI_VERSION}
)

set(TARNAME "pcitool")
set(PACKAGE_VERSION ${PCILIB_VERSION})
set(PACKAGE_NAME "${TARNAME}")
set(PACKAGE_TARNAME "${TARNAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://ufo.kit.edu/ufo/newticket")

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif(NOT DEFINED BIN_INSTALL_DIR)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif(NOT DEFINED LIB_INSTALL_DIR)

if(NOT DEFINED INCLUDE_INSTALL_DIR)
    set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
endif(NOT DEFINED INCLUDE_INSTALL_DIR)

if(NOT DEFINED DATA_INSTALL_DIR)
    set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share")
endif(NOT DEFINED DATA_INSTALL_DIR)

if(NOT DEFINED LOCALE_INSTALL_DIR)
    set(LOCALE_INSTALL_DIR "${DATA_INSTALL_DIR}/locale")
endif(NOT DEFINED LOCALE_INSTALL_DIR)

set(PCILIB_PLUGIN_DIR "${LIB_INSTALL_DIR}/pcilib" CACHE PATH "Directory to install plugins")
set(PCILIB_DATA_DIR "${DATA_INSTALL_DIR}/pcilib" CACHE PATH "Directory to install data files")
set(PCILIB_MODEL_DIR "${PCILIB_DATA_DIR}/models" CACHE PATH "Directory to install XML models")
set(PCILIB_DEBUG_DIR "." CACHE PATH "Directory to write debug information")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcitool.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pcitool.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcilib/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/pcilib/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcilib/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/pcilib/version.h)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/pcitool.pc 
    DESTINATION ${LIB_INSTALL_DIR}/pkgconfig
)

message("-- Configured pcitool ${PCILIB_VERSION_MAJOR}.${PCILIB_VERSION_MINOR}.${PCILIB_VERSION_MICRO} with public ABI ${PCILIB_ABI_VERSION}")

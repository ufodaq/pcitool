project(pcitool)

set(PCILIB_VERSION "0.2.6")
set(PCILIB_ABI_VERSION "2")

cmake_minimum_required(VERSION 2.8)
#set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH true)
#set(CMAKE_PREFIX_PATH ${CMAKE_SYSTEM_PREFIX_PATH})

set(DISABLE_PCITOOL FALSE CACHE BOOL "Build only the library")
set(DISABLE_PYTHON FALSE CACHE BOOL "Disable python scripting support") 

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

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

SET(ENV{PKG_CONFIG_PATH} "${LIB_INSTALL_DIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

if (NOT DISABLE_PYTHON)
    find_package(PythonLibs 2.7 REQUIRED)
    find_package(SWIG REQUIRED)
    set(HAVE_PYTHON TRUE)
endif (NOT DISABLE_PYTHON)

set(EXTRA_SYSTEM_LIBS -lrt)

check_include_files(stdatomic.h HAVE_STDATOMIC_H)

find_path(UTHASH_INCLUDE_DIRS uthash.h PATH_SUFFIXES uthash)
if (UTHASH_INCLUDE_DIRS)
    message ("uthash is found at ${UTHASH_INCLUDE_DIRS}")
else (UTHASH_INCLUDE_DIRS)
    message (FATAL_ERROR "uthash is not found...")
endif (UTHASH_INCLUDE_DIRS)

pkg_check_modules(LIBXML2 libxml-2.0 REQUIRED)

#Check in sibling directory
if (NOT DISABLE_PCITOOL)
    pkg_check_modules(FASTWRITER fastwriter REQUIRED)
endif (NOT DISABLE_PCITOOL)

add_definitions("-fPIC --std=c99 -Wall -O2 -gdwarf-2 -g3 -fno-omit-frame-pointer")
#add_definitions("-fPIC --std=c99 -Wall -O2")

include(cmake/version.cmake)

VERSION_TO_VARS(${PCILIB_VERSION} PCILIB_VERSION_MAJOR PCILIB_VERSION_MINOR PCILIB_VERSION_MICRO)

add_custom_target(build)
add_custom_command(TARGET build
    COMMAND ${CMAKE_COMMAND} -DPCILIB_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DPCILIB_BINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/build.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_source_files_properties(${CMAKE_BINARY_DIR}/pcilib/build.h PROPERTIES GENERATED TRUE)


set(TARNAME "pcitool")
set(PACKAGE_VERSION ${PCILIB_VERSION})
set(PACKAGE_NAME "${TARNAME}")
set(PACKAGE_TARNAME "${TARNAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "http://ufo.kit.edu/ufo/newticket")

set(PCILIB_PLUGIN_DIR "${LIB_INSTALL_DIR}/pcilib" CACHE PATH "Directory to install plugins")
set(PCILIB_DATA_DIR "${DATA_INSTALL_DIR}/pcilib" CACHE PATH "Directory to install data files")
set(PCILIB_MODEL_DIR "${PCILIB_DATA_DIR}/models" CACHE PATH "Directory to install XML models")
set(PCILIB_DOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs/" CACHE PATH "Directory to install documentation")
set(PCILIB_DEBUG_DIR "." CACHE PATH "Directory to write debug information")

add_subdirectory(dma)
add_subdirectory(protocols)
add_subdirectory(views)
add_subdirectory(pcilib)
add_subdirectory(pcitool)
add_subdirectory(apps)
add_subdirectory(xml)

if (HAVE_PYTHON)
    add_subdirectory(pywrap)
endif (HAVE_PYTHON)

set_target_properties(pcilib PROPERTIES
    VERSION ${PCILIB_VERSION}
    SOVERSION ${PCILIB_ABI_VERSION}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/pcitool.pc.in ${CMAKE_CURRENT_BINARY_DIR}/misc/pcitool.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcilib/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/pcilib/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcilib/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/pcilib/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile)

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    file(COPY ${CMAKE_SOURCE_DIR}/xml DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/pci
	 DESTINATION ${CMAKE_BINARY_DIR}
	 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    file(COPY ${CMAKE_SOURCE_DIR}/run
	 DESTINATION ${CMAKE_BINARY_DIR}
	 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/xml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
   file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pywrap/templates DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/pywrap)
   file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pywrap/static DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/pywrap)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pywrap/html_server.py ${CMAKE_CURRENT_BINARY_DIR}/pywrap/html_server.py)
endif(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/misc/pcitool.pc
    DESTINATION ${LIB_INSTALL_DIR}/pkgconfig
)

message("-- Configured pcitool ${PCILIB_VERSION_MAJOR}.${PCILIB_VERSION_MINOR}.${PCILIB_VERSION_MICRO} with public ABI ${PCILIB_ABI_VERSION}")

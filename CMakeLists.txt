project(pcitool C)

set(RELEASE "0")
set(PCILIB_VERSION "0.2.7")
set(PCILIB_ABI_VERSION "2")

cmake_minimum_required(VERSION 2.8)
#set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH true)
#set(CMAKE_PREFIX_PATH ${CMAKE_SYSTEM_PREFIX_PATH})

set(DISABLE_PCITOOL FALSE CACHE BOOL "Build only the library")
set(DISABLE_PYTHON FALSE CACHE BOOL "Disable python scripting support") 
set(DISABLE_SERVERS FALSE CACHE BOOL "Disable Web API support") 

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(NOT DEFINED LIB_SUFFIX)
 if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIB_SUFFIX "64")
 else (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIB_SUFFIX "")
 endif (CMAKE_SIZEOF_VOID_P EQUAL 8)
endif(NOT DEFINED LIB_SUFFIX)

if(NOT DEFINED BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif(NOT DEFINED BIN_INSTALL_DIR)

if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")
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

if (NOT DEFINED PCILIB_PLUGIN_DIR)
    set(PCILIB_PLUGIN_DIR "${LIB_INSTALL_DIR}/pcilib" CACHE PATH "Directory to install plugins")
endif (NOT DEFINED PCILIB_PLUGIN_DIR)

if (NOT DEFINED PCILIB_DATA_DIR)
    set(PCILIB_DATA_DIR "${DATA_INSTALL_DIR}/pcilib" CACHE PATH "Directory to install data files")
endif (NOT DEFINED PCILIB_DATA_DIR)

if (NOT DEFINED PCILIB_MODEL_DIR)
    set(PCILIB_MODEL_DIR "${PCILIB_DATA_DIR}/models" CACHE PATH "Directory to install XML models")
endif (NOT DEFINED PCILIB_MODEL_DIR)

if (NOT DEFINED PCILIB_SERVER_DIR)
    set(PCILIB_SERVER_DIR "${PCILIB_DATA_DIR}/pyserver" CACHE PATH "Directory to install python server")
endif (NOT DEFINED PCILIB_SERVER_DIR)

if (NOT DEFINED PCILIB_DOC_DIR)
    set(PCILIB_DOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs/" CACHE PATH "Directory to install documentation")
endif (NOT DEFINED PCILIB_DOC_DIR)
    
if (NOT DEFINED PCILIB_DEBUG_DIR)
    set(PCILIB_DEBUG_DIR "." CACHE PATH "Directory to write debug information")
endif (NOT DEFINED PCILIB_DEBUG_DIR)

SET(ENV{PKG_CONFIG_PATH} "${LIB_INSTALL_DIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Doxygen)

if (NOT DISABLE_PYTHON)
    set(PYTHON_VERSION 2.7 CACHE STRING "python version")

    find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
    find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT REQUIRED)

    find_package(SWIG REQUIRED)
    if (NOT PYTHON_VERSION_STRING VERSION_EQUAL PYTHONLIBS_VERSION_STRING)
	message (FATAL_ERROR "Version mismatch between python interpreter and libraries")
    endif (NOT PYTHON_VERSION_STRING VERSION_EQUAL PYTHONLIBS_VERSION_STRING)

#    execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "from sysconfig import get_path; print get_path('platlib')" OUTPUT_VARIABLE PYTHON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    
    if (NOT DEFINED PYTHON_INSTALL_DIR)
	execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.PREFIXES[-1])" OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
	file (TO_CMAKE_PATH "${PYTHON_PREFIX}" PYTHON_PREFIX)
	execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0])" OUTPUT_VARIABLE PYTHON_SITE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	file (TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR)

	string (REGEX REPLACE "^${PYTHON_PREFIX}/" "${CMAKE_INSTALL_PREFIX}/" PYTHON_SITE_DIR "${PYTHON_SITE_DIR}")
	set(PYTHON_INSTALL_DIR ${PYTHON_SITE_DIR} CACHE PATH "path to install python module")
    endif (NOT DEFINED PYTHON_INSTALL_DIR)

    set(HAVE_PYTHON TRUE)
endif (NOT DISABLE_PYTHON)

if (NOT HAVE_PYTHON)
    set(DISABLE_SERVERS TRUE)
endif (NOT HAVE_PYTHON)

set(EXTRA_SYSTEM_LIBS -lrt)

include(CheckIncludeFiles)
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

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_PACKAGE_CONTACT "Suren A. Chilingaryan <csa@suren.me>")
if (${RELEASE} GREATER 0)
    set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}.${RELEASE}")
else (${RELEASE} GREATER 0)
    set(CPACK_PACKAGE_VERSION "${PACKAGE_VERSION}")
endif (${RELEASE} GREATER 0)
set(CPACK_SOURCE_IGNORE_FILES "/.bzr/;CMakeFiles;_CPack_Packages;cmake_install.cmake;CPack.*.cmake;CMakeCache.txt;install_manifest.txt;config.h$;.pc$;Makefile;.tar.bz2$;~$;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")
include(CPack)

add_custom_target(dist_clean COMMAND ${CMAKE_MAKE_PROGRAM} clean WORKING_DIRECTORY ${CMAKE_CURRENT_DIR})
add_custom_target(dist DEPENDS dist_clean COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


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

add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
    COMMENT "Generating API documentation with Doxygen" VERBATIM
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "docs/html")


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcitool.spec.in ${CMAKE_CURRENT_BINARY_DIR}/pcitool.spec)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/dkms.conf.in ${CMAKE_CURRENT_BINARY_DIR}/misc/dkms.conf)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/pcitool.pc.in ${CMAKE_CURRENT_BINARY_DIR}/misc/pcitool.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcilib/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/pcilib/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pcilib/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/pcilib/version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile)
if (NOT DISABLE_SERVERS)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/pcilib_api.service.in ${CMAKE_CURRENT_BINARY_DIR}/misc/pcilib_api.service)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/pcilib_html.service.in ${CMAKE_CURRENT_BINARY_DIR}/misc/pcilib_html.service)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/misc/pcilib.sysconfig.in ${CMAKE_CURRENT_BINARY_DIR}/misc/pcilib.sysconfig)
endif (NOT DISABLE_SERVERS)

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    file(COPY ${CMAKE_SOURCE_DIR}/xml DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/pyserver DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/pci
	 DESTINATION ${CMAKE_BINARY_DIR}
	 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    file(COPY ${CMAKE_SOURCE_DIR}/run
	 DESTINATION ${CMAKE_BINARY_DIR}
	 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/misc/pcitool.pc
    DESTINATION ${LIB_INSTALL_DIR}/pkgconfig
)

if (NOT DISABLE_SERVERS)
   #install Python servers
   file(GLOB DEPLOY_FILES_AND_DIRS "${CMAKE_SOURCE_DIR}/pyserver/*")
   foreach(ITEM ${DEPLOY_FILES_AND_DIRS})
	if( IS_DIRECTORY "${ITEM}" )
	    list(APPEND DIRS_TO_DEPLOY "${ITEM}")
        else()
    	    list(APPEND FILES_TO_DEPLOY "${ITEM}")
        endif()
    endforeach()
    install(FILES ${FILES_TO_DEPLOY} DESTINATION ${PCILIB_SERVER_DIR})
    install(DIRECTORY ${DIRS_TO_DEPLOY} DESTINATION ${PCILIB_SERVER_DIR})
endif (NOT DISABLE_SERVERS)

message("-- Configured pcitool ${PCILIB_VERSION_MAJOR}.${PCILIB_VERSION_MINOR}.${PCILIB_VERSION_MICRO} with public ABI ${PCILIB_ABI_VERSION}")

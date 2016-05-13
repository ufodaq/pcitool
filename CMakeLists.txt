project(pcitool C)

set(RELEASE "3")
set(PCILIB_VERSION "0.2.8")
set(PCILIB_ABI_VERSION "0")

cmake_minimum_required(VERSION 2.8)
#set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH true)
#set(CMAKE_PREFIX_PATH ${CMAKE_SYSTEM_PREFIX_PATH})

set(DISABLE_PCITOOL FALSE CACHE BOOL "Build only the library")
set(DISABLE_PYTHON FALSE CACHE BOOL "Disable python scripting support") 
set(DISABLE_SERVERS FALSE CACHE BOOL "Disable Web API support") 

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")


include(GNUInstallDirs)
include(CheckIncludeFiles)

include(cmake/tools.cmake)
include(cmake/version.cmake)
VERSION_TO_VARS(${PCILIB_VERSION} PCILIB_VERSION_MAJOR PCILIB_VERSION_MINOR PCILIB_VERSION_MICRO)

set(EXTRA_SYSTEM_LIBS -lrt)
add_definitions("-fPIC --std=c99 -Wall -O2 -gdwarf-2 -g3 -fno-omit-frame-pointer")
#add_definitions("-fPIC --std=c99 -Wall -O2")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Doxygen)

if (NOT DEFINED PCILIB_PLUGIN_DIR)
    set(PCILIB_PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/pcilib" CACHE PATH "Directory to install plugins")
endif (NOT DEFINED PCILIB_PLUGIN_DIR)

if (NOT DEFINED PCILIB_DATA_DIR)
    set(PCILIB_DATA_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/pcilib" CACHE PATH "Directory to install data files")
endif (NOT DEFINED PCILIB_DATA_DIR)

if (NOT DEFINED PCILIB_MODEL_DIR)
    set(PCILIB_MODEL_DIR "${PCILIB_DATA_DIR}/models" CACHE PATH "Directory to install XML models")
endif (NOT DEFINED PCILIB_MODEL_DIR)

if (NOT DEFINED PCILIB_SERVER_DIR)
    set(PCILIB_SERVER_DIR "${PCILIB_DATA_DIR}/pyserver" CACHE PATH "Directory to install python server")
endif (NOT DEFINED PCILIB_SERVER_DIR)

if (NOT DEFINED PCILIB_DOC_DIR)
    set(PCILIB_DOC_DIR "${CMAKE_BINARY_DIR}/docs/" CACHE PATH "Directory to install documentation")
endif (NOT DEFINED PCILIB_DOC_DIR)
    
if (NOT DEFINED PCILIB_DEBUG_DIR)
    set(PCILIB_DEBUG_DIR "." CACHE PATH "Directory to write debug information")
endif (NOT DEFINED PCILIB_DEBUG_DIR)

if (NOT DEFINED PCILIB_DYNAMIC_HEADERS_INC)
    set(PCILIB_DYNAMIC_HEADERS_INC "-I${CMAKE_BINARY_DIR}")
endif (NOT DEFINED PCILIB_DYNAMIC_HEADERS_INC)

SET(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_LIBDIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}")

if (NOT DISABLE_PYTHON)
    set(PYTHON_VERSION 2.7 CACHE STRING "python version")

    find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
    find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT REQUIRED)

    find_package(SWIG REQUIRED)
    if (NOT PYTHON_VERSION_STRING VERSION_EQUAL PYTHONLIBS_VERSION_STRING)
	message (FATAL_ERROR "Version mismatch between python interpreter and libraries")
    endif (NOT PYTHON_VERSION_STRING VERSION_EQUAL PYTHONLIBS_VERSION_STRING)

#    execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "from sysconfig import get_path; print get_path('platlib')" OUTPUT_VARIABLE PCILIB_PYTHON_SITEDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    
    if (NOT DEFINED PCILIB_PYTHON_SITEDIR)
	execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.PREFIXES[-1])" OUTPUT_VARIABLE PYTHON_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
	file (TO_CMAKE_PATH "${PYTHON_PREFIX}" PYTHON_PREFIX)

#	execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "import site; print (site.getsitepackages()[0])" OUTPUT_VARIABLE PYTHON_SITE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
#	file (TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR)
#	string (REGEX REPLACE "^${PYTHON_PREFIX}/" "${CMAKE_INSTALL_PREFIX}/" PYTHON_SITE_DIR "${PYTHON_SITE_DIR}")

	execute_process (COMMAND ${PYTHON_EXECUTABLE} -c "import site; list = [dir for dir in site.getsitepackages() if dir.startswith('${CMAKE_INSTALL_PREFIX}')]; liblist = [dir for dir in list if dir.startswith('${CMAKE_INSTALL_PREFIX}/lib')]; print liblist[0] if liblist else list[0] if list else site.getsitepackages()[0]" OUTPUT_VARIABLE PYTHON_SITE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(${PYTHON_SITE_DIR})
	file (TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR)

	set(PCILIB_PYTHON_SITEDIR ${PYTHON_SITE_DIR} CACHE PATH "path to install python module")
    endif (NOT DEFINED PCILIB_PYTHON_SITEDIR)

    set(HAVE_PYTHON TRUE)
endif (NOT DISABLE_PYTHON)

if (NOT HAVE_PYTHON)
    set(DISABLE_SERVERS TRUE)
endif (NOT HAVE_PYTHON)

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

set_target_properties(pcilib PROPERTIES VERSION ${PCILIB_VERSION} SOVERSION ${PCILIB_ABI_VERSION})
add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/Doxyfile 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs
    COMMENT "Generating API documentation with Doxygen" VERBATIM
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "docs/html")

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/misc/pcitool.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
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
#set(CPACK_SOURCE_IGNORE_FILES "([^d]|d[^r])[^/]*/Makefile;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/.bzr/;CMakeFiles;_CPack_Packages;cmake_install.cmake;CPack.*.cmake;CMakeCache.txt;install_manifest.txt;common.mk;pcilib/config.h$;.pc$;Makefile$;__pycache__;.pyc$;.tar.bz2$;~$;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION}")
include(CPack)

# In releases, we just keep the pre-generated build.h 
if(EXISTS ${CMAKE_SOURCE_DIR}/.bzr/ OR NOT EXISTS ${CMAKE_SOURCE_DIR}/pcilib/build.h)
    add_custom_target(build
	COMMAND ${CMAKE_COMMAND} -DPCILIB_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DPCILIB_BINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/build.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_dependencies(pcilib build)
    add_dependencies(pci build)

	# We need to generate build.h for source releases
    add_custom_target(dist_clean make clean -C driver
	COMMAND ${CMAKE_MAKE_PROGRAM} clean 
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_target(dist_prepare DEPENDS dist_clean
	COMMAND ${CMAKE_COMMAND} -DPCILIB_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DPCILIB_BINARY_DIR=${CMAKE_SOURCE_DIR} -DPCILIB_RELEASE=${CPACK_PACKAGE_VERSION} -P ${CMAKE_SOURCE_DIR}/cmake/build.cmake
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_target(dist DEPENDS dist_prepare COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

    set_source_files_properties(${CMAKE_BINARY_DIR}/pcilib/build.h PROPERTIES GENERATED TRUE)
    set_source_files_properties(${CMAKE_SOURCE_DIR}/pcilib/build.h PROPERTIES GENERATED TRUE)
endif(EXISTS ${CMAKE_SOURCE_DIR}/.bzr/ OR NOT EXISTS ${CMAKE_SOURCE_DIR}/pcilib/build.h)

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

configure_file(${CMAKE_SOURCE_DIR}/driver/Makefile.in ${CMAKE_SOURCE_DIR}/driver/Makefile)
configure_file(${CMAKE_SOURCE_DIR}/pcitool.spec.in ${CMAKE_BINARY_DIR}/pcitool.spec)
configure_file(${CMAKE_SOURCE_DIR}/misc/dkms.conf.in ${CMAKE_BINARY_DIR}/misc/dkms.conf)
configure_file(${CMAKE_SOURCE_DIR}/misc/pcitool.pc.in ${CMAKE_BINARY_DIR}/misc/pcitool.pc)
configure_file(${CMAKE_SOURCE_DIR}/pcilib/config.h.in ${CMAKE_BINARY_DIR}/pcilib/config.h)
configure_file(${CMAKE_SOURCE_DIR}/pcilib/version.h.in ${CMAKE_BINARY_DIR}/pcilib/version.h)
configure_file(${CMAKE_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_BINARY_DIR}/docs/Doxyfile)
if (NOT DISABLE_SERVERS)
    configure_file(${CMAKE_SOURCE_DIR}/misc/pcilib_api.service.in ${CMAKE_BINARY_DIR}/misc/pcilib_api.service)
    configure_file(${CMAKE_SOURCE_DIR}/misc/pcilib_html.service.in ${CMAKE_BINARY_DIR}/misc/pcilib_html.service)
    configure_file(${CMAKE_SOURCE_DIR}/misc/pcilib.sysconfig.in ${CMAKE_BINARY_DIR}/misc/pcilib.sysconfig)
endif (NOT DISABLE_SERVERS)

PYTHON_CLEAN_LIST("${CMAKE_BINARY_DIR}/xml" XML_PY_FILES)
PYTHON_CLEAN_LIST("${CMAKE_BINARY_DIR}/pyserver" SERVER_PY_FILES)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${XML_PY_FILES};${SERVER_PY_FILES}")

message("-- Configured pcitool ${PCILIB_VERSION_MAJOR}.${PCILIB_VERSION_MINOR}.${PCILIB_VERSION_MICRO} with public ABI ${PCILIB_ABI_VERSION}")

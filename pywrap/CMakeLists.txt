include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_SOURCE_DIR}/pywrap
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIR}
    ${UTHASH_INCLUDE_DIRS}
)

include(${SWIG_USE_FILE})

set(HEADERS pcipywrap.h)
set(CMAKE_SWIG_FLAGS "")

swig_add_module(pcipywrap python pcipywrap.i pcipywrap.c)
swig_link_libraries(pcipywrap ${PYTHON_LIBRARIES} pcilib)


#install pcilib python wrapper into Python site packages folder
execute_process ( COMMAND python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

install(TARGETS ${SWIG_MODULE_pcipywrap_REAL_NAME} DESTINATION ${PYTHON_SITE_PACKAGES})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pcipywrap.py DESTINATION ${PYTHON_SITE_PACKAGES})

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/api_server.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_pcipywrap.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

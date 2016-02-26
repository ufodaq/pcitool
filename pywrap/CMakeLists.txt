include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIR}
    ${UTHASH_INCLUDE_DIRS}
)

set(HEADERS pcipywrap.h)

#Creating python wrapping
include(${SWIG_USE_FILE})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_SWIG_FLAGS "")

swig_add_module(pcipywrap python pcipywrap.i pcipywrap.c)
swig_link_libraries(pcipywrap ${PYTHON_LIBRARIES} pcilib)

#install pcilib python wrapper into Python site packages folder
execute_process ( COMMAND python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
install(TARGETS ${SWIG_MODULE_pcipywrap_REAL_NAME} DESTINATION ${PYTHON_SITE_PACKAGES})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pcipywrap.py DESTINATION ${PYTHON_SITE_PACKAGES})

configure_file(server.py server.py)
configure_file(test_pcipywrap.py test_pcipywrap.py)

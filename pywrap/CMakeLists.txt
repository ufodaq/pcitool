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

install(TARGETS ${SWIG_MODULE_pcipywrap_REAL_NAME} DESTINATION ${PCILIB_PYTHON_SITEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pcipywrap.py DESTINATION ${PCILIB_PYTHON_SITEDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pcilib.py DESTINATION ${PCILIB_PYTHON_SITEDIR})

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
   file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_pcilib.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "__pycache__;pcilib.pyc;pcipywrap.pyc")



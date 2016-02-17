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
INCLUDE(${SWIG_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
SET(CMAKE_SWIG_FLAGS "")

SWIG_ADD_MODULE(pcipywrap python pcipywrap.i pcipywrap.c)
SWIG_LINK_LIBRARIES(pcipywrap ${PYTHON_LIBRARIES} pcilib)

configure_file(server.py server.py)

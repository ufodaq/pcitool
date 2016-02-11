include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/pcilib
    ${CMAKE_BINARY_DIR}/pcilib
    ${LIBXML2_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIR}
    ${UTHASH_INCLUDE_DIRS}
)

#Creating python wrapping
INCLUDE(${SWIG_USE_FILE})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(CMAKE_SWIG_FLAGS "")

SET_SOURCE_FILES_PROPERTIES(pcipywrap.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_MODULE(pcipywrap python pcipywrap.i pcipywrap.c)
SWIG_LINK_LIBRARIES(pcipywrap ${PYTHON_LIBRARIES} pcilib)

configure_file(server.py server.py)

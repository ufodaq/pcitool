MACRO(PYTHON_CLEAN_LIST PYPATH RESULT)
    file(GLOB_RECURSE PYTHON_FILES "${PYPATH}/*.py")

    set(CLEAN_LIST "")
    set(PYTHON_DIRS "")
    foreach(ITEM ${PYTHON_FILES})
	get_filename_component(DIR ${ITEM} PATH)		# Later version may require to use DIRECTORY instead of PATH
	list(APPEND PYTHON_DIRS "${DIR}/__pycache__")
	list(APPEND CLEAN_LIST "${ITEM}c")
    endforeach(ITEM ${PYTHON_FILES})
    list(REMOVE_DUPLICATES PYTHON_DIRS)
    list(APPEND CLEAN_LIST ${PYTHON_DIRS})

    set(${RESULT} ${CLEAN_LIST})
ENDMACRO(PYTHON_CLEAN_LIST PYPATH CLEAN_LIST)

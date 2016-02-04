cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

find_package(BAZAAR QUIET)

set(PCILIB_BUILD_DATE "")
set(PCILIB_LAST_MODIFICATION "")
set(PCILIB_REVISION "0")
set(PCILIB_REVISION_BRANCH "")
set(PCILIB_REVISION_AUTHOR "")
set(PCILIB_REVISION_MODIFICATIONS "")

execute_process(
    COMMAND date "+%Y/%m/%d %H:%M:%S"
    RESULT_VARIABLE _retcode
    OUTPUT_VARIABLE _output
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (${_retcode} EQUAL 0)
    set(PCILIB_BUILD_DATE ${_output})
endif (${_retcode} EQUAL 0)

execute_process(
    COMMAND find ${CMAKE_SOURCE_DIR} -type f -name "*.[ch]" -printf "%TY/%Tm/%Td %TH:%TM:%TS  %p\n"
    COMMAND sort -n
    COMMAND grep -E -v "build.h|config.h|CMakeFiles|./apps"
    COMMAND tail -n 1
    COMMAND cut -d " " -f 1-2
    COMMAND cut -d "." -f 1
    RESULT_VARIABLE _retcode
    OUTPUT_VARIABLE _output
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (${_retcode} EQUAL 0)
    set(PCILIB_LAST_MODIFICATION ${_output})
endif (${_retcode} EQUAL 0)

if (BAZAAR_FOUND)
    execute_process(
	COMMAND ${BAZAAR_EXECUTABLE} revno --tree ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE _retcode
	OUTPUT_VARIABLE _output
	OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (${_retcode} EQUAL 0)
	set(PCILIB_REVISION ${_output})

	execute_process(
	    COMMAND ${BAZAAR_EXECUTABLE} log -r${PCILIB_REVISION} ${CMAKE_SOURCE_DIR}
	    RESULT_VARIABLE _retcode
	    OUTPUT_VARIABLE _output
	    OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if (${_retcode} EQUAL 0)
	    string(REGEX REPLACE "^(.*\n)?committer: ([^\n]+).*"
                    "\\2" PCILIB_REVISION_AUTHOR "${_output}" )
	    string(REGEX REPLACE "^(.*\n)?branch nick: ([^\n]+).*"
                    "\\2" PCILIB_REVISION_BRANCH "${_output}" )
	endif (${_retcode} EQUAL 0)
    endif (${_retcode} EQUAL 0)

    execute_process(
	COMMAND ${BAZAAR_EXECUTABLE} status -SV
	COMMAND cut -c 5-
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	RESULT_VARIABLE _retcode
	OUTPUT_VARIABLE _output
	OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (${_retcode} EQUAL 0)
	string(REGEX REPLACE "\n+" ";" PCILIB_REVISION_MODIFICATIONS ${_output})
#	set(PCILIB_REVISION_MODIFICATIONS ${_output})
    endif (${_retcode} EQUAL 0)
endif(BAZAAR_FOUND)

configure_file(${CMAKE_SOURCE_DIR}/pcilib/build.h.in ${CMAKE_BINARY_DIR}/pcilib/build.h)

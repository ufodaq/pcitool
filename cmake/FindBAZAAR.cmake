#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
#  Copyright (C) 2010 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
# This CMake script finds the BAZAAR version control system executable and
# and fetches the veresion string to valid that BAZAAR was found and executes
# properly.
#
# Usage:
#  find_package( BAZAAR )
#
# User definable.
#    BAZAAR_EXECUTABLE      Set this to use a version of BAZAAR that is not in
#                           current path.  Defaults to bzr.
#
# Defines:
#    BAZAAR_FOUND           Set to TRUE if BAZAAR command line client is found
#                           and the bzr --version command executes properly.
#    BAZAAR_VERSION         Result of the bzr --version command.
#

set( BAZAAR_FOUND FALSE )

find_program( BAZAAR_EXECUTABLE bzr
              DOC "BAZAAR version control system command line client" )
mark_as_advanced( BAZAAR_EXECUTABLE )

if( BAZAAR_EXECUTABLE )

    # BAZAAR commands should be executed with the C locale, otherwise
    # the message (which are parsed) may be translated causing the regular
    # expressions to fail.
    set( _BAZAAR_SAVED_LC_ALL "$ENV{LC_ALL}" )
    set( ENV{LC_ALL} C )

    # Fetch the BAZAAR executable version.
    execute_process( COMMAND ${BAZAAR_EXECUTABLE} --version
                     OUTPUT_VARIABLE _bzr_version_output
                     ERROR_VARIABLE _bzr_version_error
                     RESULT_VARIABLE _bzr_version_result
                     OUTPUT_STRIP_TRAILING_WHITESPACE )

    if( ${_bzr_version_result} EQUAL 0 )
        set( BAZAAR_FOUND TRUE )
        string( REGEX REPLACE "^[\n]*Bazaar \\(bzr\\) ([0-9.a-z]+).*"
                "\\1" BAZAAR_VERSION "${_bzr_version_output}" )
	if( NOT BAZAAR_FIND_QUIETLY )
	    message( STATUS "BAZAAR version control system version ${BAZAAR_VERSION} found." )
	endif()
    endif()

    # restore the previous LC_ALL
    set( ENV{LC_ALL} ${_BAZAAR_SAVED_LC_ALL} )
endif()

if( NOT BAZAAR_FOUND )
    if( NOT BAZAAR_FIND_QUIETLY )
        message( STATUS "BAZAAR version control command line client was not found." )
    else()
        if( BAZAAR_FIND_REQUIRED )
            message( FATAL_ERROR "BAZAAR version control command line client was not found." )
        endif()
    endif()
endif()

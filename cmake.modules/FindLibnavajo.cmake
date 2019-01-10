# - Find libnavajo
#   Find the libnavajo includes and client library
# This module defines
#  LIBNAVAJO_INCLUDE_DIRS
#  LIBNAVAJO_LIBRARIES
#  LIBNAVAJO_PRECOMPILER_EXEC
#  LIBNAVAJO_FOUND

include (FindPackageHandleStandardArgs)

find_path (LIBNAVAJO_INCLUDE_DIRS libnavajo/libnavajo.hh
    NAME
        libnavajo.hh
    PATHS
        /usr/include
        /include
        /usr/local/include
    DOC
        "Directory for libnavajo headers"
)

find_program(LIBNAVAJO_PRECOMPILER_EXEC navajoPrecompiler
  NAME
      navajoPrecompiler
  PATH
      /usr/bin
      /bin
      /usr/local/bin
)

find_library (LIBNAVAJO_LIBRARIES
    NAMES
        navajo
    PATHS
        /usr/lib/libnavajo
        /usr/lib64/libnavajo
        /lib/libnavajo
        /usr/local/lib/libnavajo
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS("libnavajo"
    "libnavajo couldn't be found"
    LIBNAVAJO_LIBRARIES
    LIBNAVAJO_INCLUDE_DIRS
)

mark_as_advanced (LIBNAVAJO_INCLUDE_DIR LIBNAVAJO_LIBRARY)


MACRO (HTML_PRECOMPIL_REPO HTML_REPO)

	SET(LIBNAVAJO_precompiledFile "PrecompiledRepository.cc" )
	    
  ADD_CUSTOM_COMMAND(
      OUTPUT ${LIBNAVAJO_precompiledFile}
      COMMAND rm -f ${LIBNAVAJO_precompiledFile}
      COMMAND ${LIBNAVAJO_PRECOMPILER_EXEC} ${HTML_REPO} ${HTML_REPOSITORY_OPTIONS} >> ${LIBNAVAJO_precompiledFile}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      MAIN_DEPENDENCY ${HTML_REPO}
  )

ENDMACRO ()



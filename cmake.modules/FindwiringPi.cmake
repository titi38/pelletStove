# - Find wiringPi
#   Find the wiringPi includes and client library
# This module defines
#  WIRINGPI_INCLUDE_DIRS
#  WIRINGPI_LIBRARIES
#  WIRINGPI_PRECOMPILER_EXEC
#  WIRINGPI_FOUND

include (FindPackageHandleStandardArgs)

find_path (WIRINGPI_INCLUDE_DIRS wiringPi/wiringPi.h
    NAME
        wiringPi.h
    PATHS
        /usr/include
        /usr/local/include
    DOC
        "Directory for wiringPi headers"
)

find_library (WIRINGPI_LIBRARIES
    NAMES
        libwiringPi.so
    PATHS
        /usr/lib/libwiringPi
        /usr/lib64/libwiringPi
        /lib/libwiringPi
        /usr/local/lib/libwiringPi
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS("wiringPi"
    "wiringPi couldn't be found"
    WIRINGPI_LIBRARIES
    WIRINGPI_INCLUDE_DIRS
)

mark_as_advanced (WIRINGPI_INCLUDE_DIR WIRINGPI_LIBRARY)





# - Try to find the mbed tls Library
# The module will set the following variables
#
#  MBED_TLS_FOUND - System has popt
#  MBED_TLS_INCLUDE_DIR - The popt include directory
#  MBED_TLS_LIBRARIES - The libraries needed to use popt

# Find the include directories
FIND_PATH(MBED_TLS_INCLUDE_DIR
    NAMES mbedtls/ssl.h
    HINTS ${MBED_TLS_ROOT}
    PATH_SUFFIXES include
    DOC "Path containing the mbedtls/ssl.h include file"
    )

FIND_LIBRARY(MBED_TLS_LIBRARIES
    NAMES mbedtls
    HINTS ${MBED_TLS_ROOT}
    PATH_SUFFIXES library
    DOC "mbedtls library path"
    )

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(MBED_TLS
    REQUIRED_VARS MBED_TLS_INCLUDE_DIR MBED_TLS_LIBRARIES
  )

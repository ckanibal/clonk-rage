# - Try to find the sdl2 Library
# The module will set the following variables
#
#  SDL2_FOUND - System has popt
#  SDL2_INCLUDE_DIR - The popt include directory
#  SDL2_LIBRARIES - The libraries needed to use popt

# Find the include directories
FIND_PATH(SDL2_INCLUDE_DIR
    NAMES SDL.h
    HINTS ${SDL2_ROOT}
    PATH_SUFFIXES include
    DOC "Path containing the SDL.h file"
    )

FIND_LIBRARY(SDL2_LIBRARIES
    NAMES SDL2
    HINTS ${SDL2_ROOT}
    PATH_SUFFIXES build
    DOC "sdl2 library path"
    )

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2
    REQUIRED_VARS SDL2_INCLUDE_DIR SDL2_LIBRARIES
  )

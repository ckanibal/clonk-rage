cmake_minimum_required (VERSION 2.8)
project (clonk-rage)

# module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/lib")
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/lib")

if(WIN32)
elseif(UNIX OR APPLE)
  add_definitions(-DHAVE_STDINT_H)
  add_definitions(-DHAVE_UNISTD_H)
  add_definitions(-DHAVE_PTHREAD)
  add_definitions(-DUSE_GL)
endif()

if(WIN32)
  MESSAGE( STATUS "Building for WIN32 ..." )
  add_definitions(-DWIN32)
elseif(APPLE)
  MESSAGE( STATUS "Building for APPLE ..." )
  add_definitions(-D__APPLE__)
  add_definitions(-DUSE_SDL_MAINLOOP)
  add_definitions(-DHAVE_SDL)

  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
elseif(UNIX)
  MESSAGE( STATUS "Building for UNIX ..." )
endif()

# include common libraries
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

set(MBED_TLS_ROOT "${LIBRARY_DIR}/mbedtls/")
find_package(MBED_TLS REQUIRED)

# include projects
subdirs(engine standard)

# set target properties
set_property(GLOBAL PROPERTY CXX_STANDARD 11)

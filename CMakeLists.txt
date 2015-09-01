cmake_minimum_required (VERSION 2.8)
project (clonk-rage)

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

  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
elseif(UNIX)
  MESSAGE( STATUS "Building for UNIX ..." )
endif()


# include projects
subdirs(engine standard)

# set target properties
set_property(GLOBAL PROPERTY CXX_STANDARD 11)

cmake_minimum_required (VERSION 2.8)
project (clonk-rage)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

OPTION(WITH_GL "Use OpenGL for rendering" ON)
OPTION(WITH_NETWORK "Enable network" ON)
OPTION(WITH_SDL_MAINLOOP "Enable SDL mainloop" OFF)

if(WIN32)
elseif(UNIX OR APPLE)
  add_definitions(-DHAVE_STDINT_H)
  add_definitions(-DHAVE_UNISTD_H)
  add_definitions(-DHAVE_PTHREAD)

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

if(WIN32)
  MESSAGE( STATUS "Building for WIN32 ..." )
  add_definitions(-DWIN32)
elseif(APPLE)
  MESSAGE( STATUS "Building for APPLE ..." )
  set(USE_SDL_MAINLOOP ON)
  add_definitions(-D__APPLE__)
  add_definitions(-DUSE_SDL_MAINLOOP)
elseif(UNIX)
  MESSAGE( STATUS "Building for UNIX ..." )
  set(USE_X11 ON)
  add_definitions(-DUSE_X11)
endif()

if(WITH_GL)
  add_definitions(-DUSE_GL)
  find_package(OpenGL REQUIRED)
  link_libraries(${OPENGL_LIBRARIES})

  find_package(GLEW REQUIRED)
  if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
  endif()
endif()

if(WITH_NETWORK)
  add_definitions(-DNETWORK)
else()
  add_definitions(-DNONETWORK)
endif()

# include projects
subdirs(engine standard)

# set target properties
set_property(GLOBAL PROPERTY CXX_STANDARD 14)

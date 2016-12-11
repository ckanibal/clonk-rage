cmake_minimum_required (VERSION 2.8)
project (clonk-rage)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

OPTION(USE_GL "Use OpenGL for rendering" ON)
OPTION(WITH_NETWORK "Enable network" ON)

if(WIN32)
elseif(UNIX OR APPLE)
  add_definitions(-DHAVE_STDINT_H)
  add_definitions(-DHAVE_UNISTD_H)
  add_definitions(-DHAVE_PTHREAD)
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
endif()

if(USE_GL)
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
  # TODO: active NONETWORK
endif()

## include common libraries ##
# openssl
# set(OPENSSL_LOCAL_DIR 3rdparty/openssl/)
find_package(OpenSSL REQUIRED)
list(APPEND LIBS
	${OPENSSL_LIBRARIES}
)

include_directories(${PNG_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})


# include projects
subdirs(engine standard)

# set target properties
set_property(GLOBAL PROPERTY CXX_STANDARD 11)

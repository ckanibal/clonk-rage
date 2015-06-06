cmake_minimum_required (VERSION 2.8.7)
project (clonk_rage CXX C)

set(STANDARD_SOURCES
  src/Bitmap256.cpp
  src/CStdFile.cpp
  src/d3dutil.cpp
  src/DInputX.cpp
  src/DSoundX.cpp
  src/dxutil.cpp
  src/Fixed.cpp
  src/Midi.cpp
  src/OpenURL.cpp
  src/PathFinder.cpp
  src/Standard.cpp
  src/StdBase64.cpp
  src/StdBitmap.cpp
  src/StdBuf.cpp
  src/StdCompiler.cpp
  src/StdConfig.cpp
  src/StdD3D.cpp
  src/StdDDraw2.cpp
  src/StdFile.cpp
  src/StdFont.cpp
  src/StdGL.cpp
  src/StdGLCtx.cpp
  src/StdMarkup.cpp
  src/StdNoGfx.cpp
  src/StdPNG.cpp
  src/StdRegistry.cpp
  src/StdResStr2.cpp
  src/StdScheduler.cpp
  src/StdSDLApp.cpp
  src/StdSDLWindow.cpp
  src/StdSurface2.cpp
  src/StdSurface8.cpp
  src/StdTApp.cpp
  src/StdVideo.cpp
  src/StdXApp.cpp
  src/StdXWindow.cpp
  src/MacUtility.mm
)

set(STANDARD_HEADERS
  inc/Bitmap256.h
  inc/CStdFile.h
  inc/d3dutil.h
  inc/DInputX.h
  inc/DSoundX.h
  inc/dxutil.h
  inc/Fixed.h
  inc/Midi.h
  inc/PathFinder.h
  inc/Standard.h
  inc/StdAdaptors.h
  inc/StdBase64.h
  inc/StdBitmap.h
  inc/StdBuf.h
  inc/StdColors.h
  inc/StdCompiler.h
  inc/StdConfig.h
  inc/StdD3D.h
  inc/StdDDraw2.h
  inc/StdFacet.h
  inc/StdFile.h
  inc/StdFont.h
  inc/StdGL.h
  inc/StdMarkup.h
  inc/StdNoGfx.h
  inc/StdPNG.h
  inc/StdRandom.h
  inc/StdRegistry.h
  inc/StdResStr.h
  inc/StdResStr2.h
  inc/StdScheduler.h
  inc/StdSurface2.h
  inc/StdSurface8.h
  inc/StdSync.h
  inc/StdVideo.h
)

set(ZLIB_SOURCES
  zlib/adler32.c
  zlib/compress.c
  zlib/crc32.c
  zlib/deflate.c
  zlib/gzio.c
  zlib/infback.c
  zlib/inffast.c
  zlib/inflate.c
  zlib/inftrees.c
  zlib/trees.c
  zlib/uncompr.c
  zlib/zutil.c
)

set(PNG_SOURCES
  lpng121/png.c
  lpng121/pngerror.c
  lpng121/pnggccrd.c
  lpng121/pngget.c
  lpng121/pngmem.c
  lpng121/pngpread.c
  lpng121/pngread.c
  lpng121/pngrio.c
  lpng121/pngrtran.c
  lpng121/pngrutil.c
  lpng121/pngset.c
  lpng121/pngtest.c
  lpng121/pngtrans.c
  lpng121/pngvcrd.c
  lpng121/pngwio.c
  lpng121/pngwrite.c
  lpng121/pngwtran.c
  lpng121/pngwutil.c
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/zlib
)

if(NOT WIN32)
  add_definitions(-DHAVE_STDINT_H)
  add_definitions(-DHAVE_PTHREAD)
  add_definitions(-DHAVE_UNISTD_H)
  add_definitions(-DUSE_GL)
endif()

## osx only
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_definitions(-DUSE_SDL_MAINLOOP)
  add_definitions(-D__APPLE__)
  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
endif()

## libpng
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

## opengl & friends
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)

add_library(
  standard
  STATIC
  ${STANDARD_SOURCES}
  ${STANDARD_HEADERS}
  ${ZLIB_SOURCES}
)

target_link_libraries(
  standard
  ${SDL_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${GLUT_LIBRARY}
  ${GLEW_LIBRARIES}
  ${PNG_LIBRARIES}
)

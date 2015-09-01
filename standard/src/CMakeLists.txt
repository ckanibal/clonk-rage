include_directories(${clonk-rage_SOURCE_DIR}/standard/inc)
set(STANDARD_SOURCES
  Bitmap256.cpp
  CStdFile.cpp
  Fixed.cpp
  Midi.cpp
  OpenURL.cpp
  PathFinder.cpp
  Standard.cpp
  StdBase64.cpp
  StdBitmap.cpp
  StdBuf.cpp
  StdCompiler.cpp
  StdConfig.cpp
  StdD3D.cpp
  StdDDraw2.cpp
  StdFile.cpp
  StdFont.cpp
  StdGL.cpp
  StdGLCtx.cpp
  StdMarkup.cpp
  StdNoGfx.cpp
  StdPNG.cpp
  StdRegistry.cpp
  StdResStr2.cpp
  StdSDLApp.cpp
  StdSDLWindow.cpp
  StdScheduler.cpp
  StdSurface2.cpp
  StdSurface8.cpp
  StdTApp.cpp
  StdVideo.cpp
  StdWindow.cpp
  StdXApp.cpp
  StdXWindow.cpp
  dxutil.cpp
)
if(WIN32)
  list(APPEND STANDARD_SOURCES
    StdJoystick.cpp
  )
elseif(APPLE)

elseif(UNIX)
  list(APPEND STANDARD_SOURCES
    StdGtkWindow.cpp
  )
endif()

add_library(standard ${STANDARD_SOURCES})

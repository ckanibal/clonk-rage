cmake_minimum_required (VERSION 2.6)
project ("standard library" CXX)

include(${CMAKE_SOURCE_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

option(INTERNAL_FREETYPE "Build using internal freetype library" OFF)

set(STANDARD_SRC
		src/Bitmap256.cpp
		src/CStdFile.cpp
		src/Fixed.cpp
		src/OpenURL.cpp
		src/PathFinder.cpp
		src/Standard.cpp
		src/StdBase64.cpp
		src/StdBitmap.cpp
		src/StdBuf.cpp
		src/StdCompiler.cpp
		src/StdConfig.cpp
		src/StdDDraw2.cpp
		src/StdFile.cpp
		src/StdFont.cpp
		src/StdMarkup.cpp
		src/StdNoGfx.cpp
		src/StdPNG.cpp
		src/StdResStr2.cpp
		src/StdScheduler.cpp
		src/StdSurface2.cpp
		src/StdSurface8.cpp
		src/StdVideo.cpp
		src/zlib/gzio.c
		src/zlib/gzio.h
		src/zlib/zutil.h
)

if(WIN32)
elseif(APPLE)
	list(APPEND STANDARD_SRC
	  src/MacUtility.mm
	)
elseif(UNIX)
endif()

if(WITH_GLIB)
	set(STANDARD_SRC ${STANDARD_SRC} src/StdGtkWindow.cpp)
endif()

if(USE_X11)
	set(STANDARD_SRC ${STANDARD_SRC} src/StdXApp.cpp src/StdXWindow.cpp)
endif()

if(USE_GL)
	set(STANDARD_SRC ${STANDARD_SRC} src/StdGL.cpp src/StdGLCtx.cpp)
endif()

if(USE_SDL_MAINLOOP)
	set(STANDARD_SRC ${STANDARD_SRC} src/StdSDLApp.cpp src/StdSDLWindow.cpp)
	find_package(SDL)
	include_directories(${SDL_INCLUDE_DIR})
endif()

if(USE_DIRECTX)
	set(STANDARD_SRC ${STANDARD_SRC} src/d3dutil.cpp src/DInputX.cpp src/DSoundX.cpp src/dxutil.cpp src/StdD3D.cpp src/StdJoystick.cpp)
endif()

if(USE_CONSOLE)
	set(STANDARD_SRC ${STANDARD_SRC} src/StdTApp.cpp)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	 set(STANDARD_SRC ${STANDARD_SRC} src/StdXWindow.cpp)
 elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	 set(STANDARD_SRC ${STANDARD_SRC} src/StdWindow.cpp)
 endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(STANDARD_SRC ${STANDARD_SRC} src/StdWindow.cpp)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(STANDARD_SRC ${STANDARD_SRC} src/StdRegistry.cpp)
elseif(APPLE)
	set(STANDARD_SRC ${STANDARD_SRC} src/MacUtility.mm ../xcode/SDLMain.h)
endif()

include_directories("${PROJECT_SOURCE_DIR}/inc/")
include_directories("${PROJECT_SOURCE_DIR}/config/")
include_directories("${PROJECT_SOURCE_DIR}")

if(WITH_GLIB)
	find_package(GTK2)
	include_directories(${GTK2_INCLUDE_DIRS})
endif()

include_directories(${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR})

add_definitions(-DC4ENGINE)
set(CMAKE_CXX_FLAGS "-fwrapv")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_library(standard STATIC ${STANDARD_SRC})
set_property(TARGET standard PROPERTY COMPILE_DEFINITIONS HAVE_CONFIG_H)
set_property(TARGET standard PROPERTY COMPILE_DEFINITIONS HAVE_ICONV)
set_property(TARGET standard PROPERTY COMPILE_DEFINITIONS HAVE_FREETYPE)
target_link_libraries(standard ${CONAN_LIBS} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${SDL_LIBRARIES} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES} ${BZIP2_LIBRARIES})

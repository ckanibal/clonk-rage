cmake_minimum_required(VERSION 2.6)

# include(${CMAKE_SOURCE_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()

# add source directory
add_subdirectory(src/)
add_subdirectory(sec/)
add_subdirectory(inc/)

# defines
add_definitions(-DC4ENGINE)

# add targets
add_executable(engine ${ENGINE_SRC} ${ENGINE_H} ${SEC_SRC})

if(WITH_SDL_MAINLOOP)
  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
  list(APPEND LIBS
    ${SDL_LIBRARY}
  )
endif()

# libraries
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# include directories
include_directories(inc/ sec/ ../standard/inc/)

# properties
set_property(TARGET engine PROPERTY CXX_STANDARD 14)

# link libraries
MESSAGE( STATUS "PNG_LIBRARIES: " ${PNG_LIBRARIES} )
MESSAGE( STATUS "CONAN_LIBS: " ${CONAN_LIBS} )
MESSAGE( STATUS "LIBS: " ${LIBS} )
target_link_libraries(engine ${LIBS} ${OPENSSL_LIBRARIES} ${PNG_LIBRARIES} ${CONAN_LIBS} Threads::Threads standard)

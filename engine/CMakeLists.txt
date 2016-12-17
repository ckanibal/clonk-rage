cmake_minimum_required(VERSION 2.6)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# add source directory
add_subdirectory(src/)
add_subdirectory(sec/)
add_subdirectory(inc/)

# defines
add_definitions(-DC4ENGINE)

# add targets
# MESSAGE( STATUS "ENGINE_SRC:         " ${ENGINE_SRC})
# MESSAGE( STATUS "SEC_SRC:         " ${SEC_SRC})
add_executable(engine ${ENGINE_SRC} ${ENGINE_H} ${SEC_SRC})

if(WITH_SDL_MAINLOOP)
  find_package(SDL REQUIRED)
  include_directories(${SDL_INCLUDE_DIR})
  list(APPEND LIBS
    ${SDL_LIBRARY}
  )
endif()

# include directories
include_directories(inc/ sec/ ../standard/inc/)

# link libraries
target_link_libraries(engine ${LIBS} ${CONAN_LIBS} standard)

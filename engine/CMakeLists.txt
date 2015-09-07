# defines
add_definitions(-DC4ENGINE)

# add libraries
include_directories(${MBED_TLS_INCLUDE_DIR})
list(APPEND LIBS ${SDL_LIBRARY})

# add source directory
subdirs(src sec)

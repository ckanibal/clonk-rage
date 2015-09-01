find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

# add source directory
subdirs(src)

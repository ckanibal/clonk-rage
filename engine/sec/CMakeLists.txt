include_directories(sec/)

set(SEC_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/C4ConfigShareware.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/C4SecurityCertificates.cpp
)

set(SEC_SRC ${SEC_SRC} PARENT_SCOPE)

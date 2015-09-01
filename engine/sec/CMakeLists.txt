include_directories(${clonk-rage_SOURCE_DIR}/engine/inc)
include_directories(${clonk-rage_SOURCE_DIR}/standard/inc)
link_directories(${clonk-rage_SOURCE_DIR}/standard/src)

set(SEC_SOURCES
  C4ConfigShareware.cpp
  C4SecurityCertificates.cpp
)

list(APPEND ENGINE_SOURCES ${SEC_SOURCES})

cmake_minimum_required (VERSION 2.8.7)
project (zlib CXX C)

set(ZLIB_SOURCES
  adler32.c
  compress.c
  crc32.c
  crc32.h
  deflate.c
  deflate.h
  gzio.c
  infback.c
  inffast.c
  inffast.h
  inffixed.h
  inflate.c
  inflate.h
  inftrees.c
  inftrees.h
  trees.c
  trees.h
  uncompr.c
  zconf.h
  zlib.h
  zutil.c
  zutil.h
)

add_library(zlib ${ZLIB_SOURCES})

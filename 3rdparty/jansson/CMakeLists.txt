
# For jansson we don't want to build docs or tests.
# We need to use CACHE INTERNAL to prevent those settings from being overwritten

set(JANSSON_BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(JANSSON_BUILD_DOCS OFF CACHE INTERNAL "")
set(JANSSON_WITHOUT_TESTS ON CACHE INTERNAL "")
set(BUILD_DOCS 0 CACHE INTERNAL "")
set(WITHOUT_TESTS 1 CACHE INTERNAL "")

# To make builds significantly faster on Windows:
if(WIN32)
  set(HAVE_ENDIAN_H 0)
  set(HAVE_FCNTL_H 1)
  set(HAVE_SCHED_H 0)
  set(HAVE_UNISTD_H 0)
  set(HAVE_SYS_PARAM_H 0)
  set(HAVE_SYS_STAT_H 1)
  set(HAVE_SYS_TIME_H 0)

  set(HAVE_CLOSE 1)
  set(HAVE_GETPID 1)
  set(HAVE_GETTIMEOFDAY 0)
  set(HAVE_OPEN 1)
  set(HAVE_READ 1)
  set(HAVE_SCHED_YIELD 0)

  set(HAVE_SYS_TYPES_H 1)
  set(HAVE_INTTYPES_H 0)
  set(HAVE_STDDEF_H 1)
  set(HAVE_STDINT_H 1)
  set(HAVE_SSIZE_T FALSE)
  set(HAVE_UPPERCASE_SSIZE_T FALSE)
  set(HAVE_STRTOLL 0)
  set(HAVE_STRTOQ 0)
  set(HAVE__STRTOI64 1)
  set(HAVE_LOCALE_H 1)
  set(HAVE_LOCALECONV 1)
  set(HAVE_SETLOCALE 1)
  set(HAVE_INLINE 0)
  set(HAVE___INLINE 1)
  set(HAVE___INLINE__ 0)
  set(HAVE_SNPRINTF 0)
  set(HAVE__SNPRINTF 1)

  set(HAVE___INT64 TRUE)
  set(HAVE_INT64_T TRUE)
  set(HAVE_LONG_LONG_INT TRUE)
  set(HAVE_INT32_T TRUE)
  set(HAVE___INT32 TRUE)
  set(HAVE_UINT32_T TRUE)
  set(HAVE___UINT32 FALSE)
  set(HAVE_LONG_INT TRUE)
  set(HAVE_UNSIGNED_LONG_INT TRUE)
  set(HAVE_INT TRUE)
  set(HAVE_UNSIGNED_INT TRUE)

  set(__INT64 8)
  set(INT64_T 8)
  set(LONG_LONG_INT 8)
  set(INT32_T 4)
  set(__INT32 4)
  set(LONG_INT 4)
  set(INT 4)

  set(UINT32_T 4)
  set(__UINT32 0)
  set(UNSIGNED_LONG_INT 4)
  set(UNSIGNED_INT 4)

  set(HAVE_SYNC_BUILTINS 0)
  set(HAVE_ATOMIC_BUILTINS 0)
endif()

add_subdirectory(jansson)

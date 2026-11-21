cmake_minimum_required(VERSION 3.15)
project(bulkprint LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)

# Custom installation paths
set(DEPS_ROOT "" CACHE PATH "dependencies installation directory")
set(LIBHARU_ROOT "" CACHE PATH "libharu installation directory")
set(ZLIB_ROOT "" CACHE PATH "zlib installation directory")

# --------------------------------
# Find libHaru
# --------------------------------

find_path(LIBHARU_INCLUDE_DIR
    NAMES hpdf.h
    HINTS ${LIBHARU_ROOT} ${DEPS_ROOT}
    PATH_SUFFIXES include include/libharu
)

find_library(LIBHARU_LIBRARY
    NAMES hpdf libhpdf
    HINTS ${LIBHARU_ROOT} ${DEPS_ROOT}
    PATH_SUFFIXES lib lib64
)

if(NOT LIBHARU_INCLUDE_DIR OR NOT LIBHARU_LIBRARY)
    message(FATAL_ERROR "libHaru not found!")
endif()

# --------------------------------
# Find ZLIB
# --------------------------------

find_path(ZLIB_INCLUDE_DIR
    NAMES zlib.h
    HINTS ${ZLIB_ROOT} ${DEPS_ROOT}
    PATH_SUFFIXES include
)

find_library(ZLIB_LIBRARY
    NAMES z zlib
    HINTS ${ZLIB_ROOT} ${DEPS_ROOT}
    PATH_SUFFIXES lib lib64
)

if(NOT ZLIB_INCLUDE_DIR OR NOT ZLIB_LIBRARY)
    message(FATAL_ERROR "ZLIB not found!")
endif()

# --------------------------------
# Display detected libraries
# --------------------------------

message(STATUS "libHaru include: ${LIBHARU_INCLUDE_DIR}")
message(STATUS "libHaru library: ${LIBHARU_LIBRARY}")

message(STATUS "ZLIB include: ${ZLIB_INCLUDE_DIR}")
message(STATUS "ZLIB library: ${ZLIB_LIBRARY}")

# --------------------------------
# Build application
# --------------------------------

add_executable(bulkprint
    src/main.cpp
)

target_include_directories(bulkprint PRIVATE
    ${LIBHARU_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

target_link_libraries(bulkprint PRIVATE
    ${LIBHARU_LIBRARY}
    ${ZLIB_LIBRARY}
)


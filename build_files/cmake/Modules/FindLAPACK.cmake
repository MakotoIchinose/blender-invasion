# - Find LAPACK library
# Find the native LAPACK includes and library
# This module defines
#  LAPACK_INCLUDE_DIRS, where to find lapack.h, Set when
#                            LAPACK_INCLUDE_DIR is found.
#  LAPACK_LIBRARIES, libraries to link against to use LAPACK.
#  LAPACK_ROOT_DIR, The base directory to search for LAPACK.
#                        This can also be an environment variable.
#  LAPACK_FOUND, If false, do not try to use LAPACK.
#
# also defined, but not for general use are
#  LAPACK_LIBRARY, where to find the LAPACK library.

#=============================================================================
# Copyright 2015 Blender Foundation.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

# If LAPACK_ROOT_DIR was defined in the environment, use it.
IF(NOT LAPACK_ROOT_DIR AND NOT $ENV{LAPACK_ROOT_DIR} STREQUAL "")
  SET(LAPACK_ROOT_DIR $ENV{LAPACK_ROOT_DIR})
ENDIF()

SET(_lapack_SEARCH_DIRS
  ${LAPACK_ROOT_DIR}
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/lapack
  /opt/lib/lapack
)

#FIND_PATH(LAPACK_INCLUDE_DIR
#  NAMES
#    lapack/AABB.h #there is no lapack.h, but a lot of different headers, just pick the first here
#  HINTS
#    ${_lapack_SEARCH_DIRS}
#  PATH_SUFFIXES
#    include
#)

FIND_LIBRARY(LAPACK_LIBRARY
  NAMES
    lapack
  HINTS
    ${_lapack_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib
)

FIND_LIBRARY(BLAS_LIBRARY
  NAMES
    blas
  HINTS
    ${_lapack_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib
)


# handle the QUIETLY and REQUIRED arguments and set LAPACK_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LAPACK DEFAULT_MSG
    LAPACK_LIBRARY BLAS_LIBRARY)

IF(LAPACK_FOUND)
  SET(LAPACK_LIBRARIES ${LAPACK_LIBRARY} ${BLAS_LIBRARY})
  #SET(LAPACK_INCLUDE_DIRS ${LAPACK_INCLUDE_DIR})
ENDIF(LAPACK_FOUND)

MARK_AS_ADVANCED(
  #LAPACK_INCLUDE_DIR
  LAPACK_LIBRARY
)

UNSET(_lapack_SEARCH_DIRS)

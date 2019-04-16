# - Find IGL library
# Find the native IGL includes and library
# This module defines
#  IGL_INCLUDE_DIRS, where to find igl.h, Set when
#                            IGL_INCLUDE_DIR is found.
#  IGL_LIBRARIES, libraries to link against to use IGL.
#  IGL_ROOT_DIR, The base directory to search for IGL.
#                        This can also be an environment variable.
#  IGL_FOUND, If false, do not try to use IGL.
#
# also defined, but not for general use are
#  IGL_LIBRARY, where to find the IGL library.

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

# If IGL_ROOT_DIR was defined in the environment, use it.
IF(NOT IGL_ROOT_DIR AND NOT $ENV{IGL_ROOT_DIR} STREQUAL "")
  SET(IGL_ROOT_DIR $ENV{IGL_ROOT_DIR})
ENDIF()

SET(_igl_SEARCH_DIRS
  ${IGL_ROOT_DIR}
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/igl
  /opt/lib/igl
)

FIND_PATH(IGL_INCLUDE_DIR
  NAMES
    igl/AABB.h #there is no igl.h, but a lot of different headers, just pick the first here
  HINTS
    ${_igl_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

FIND_LIBRARY(IGL_LIBRARY
  NAMES
    igl
  HINTS
    ${_igl_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib
)

FIND_LIBRARY(IGL_COMISO_LIBRARY
  NAMES
    igl_comiso
  HINTS
    ${_igl_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib
)

FIND_LIBRARY(COMISO_LIBRARY
  NAMES
    CoMISo
  HINTS
    ${_igl_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set IGL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IGL DEFAULT_MSG
    IGL_LIBRARY IGL_COMISO_LIBRARY COMISO_LIBRARY IGL_INCLUDE_DIR)

IF(IGL_FOUND)
  SET(IGL_LIBRARIES ${IGL_LIBRARY}
                    ${IGL_COMISO_LIBRARY}
                    ${COMISO_LIBRARY})
  SET(IGL_INCLUDE_DIRS ${IGL_INCLUDE_DIR})
ENDIF(IGL_FOUND)

MARK_AS_ADVANCED(
  IGL_INCLUDE_DIR
  IGL_LIBRARY
  IGL_COMISO_LIBRARY
  COMISO_LIBRARY
)

UNSET(_igl_SEARCH_DIRS)


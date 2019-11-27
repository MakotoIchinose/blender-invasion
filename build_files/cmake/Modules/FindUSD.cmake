# - Find Universal Scene Description (USD) library
# Find the native USD includes and libraries
# This module defines
#  USD_INCLUDE_DIRS, where to find USD headers, Set when
#                        USD_INCLUDE_DIR is found.
#  USD_LIBRARIES, libraries to link against to use USD.
#  USD_ROOT_DIR, The base directory to search for USD.
#                    This can also be an environment variable.
#  USD_FOUND, If false, do not try to use USD.
#

#=============================================================================
# Copyright 2019 Blender Foundation.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

# If USD_ROOT_DIR was defined in the environment, use it.
IF(NOT USD_ROOT_DIR AND NOT $ENV{USD_ROOT_DIR} STREQUAL "")
  SET(USD_ROOT_DIR $ENV{USD_ROOT_DIR})
ENDIF()

SET(_usd_SEARCH_DIRS
  ${USD_ROOT_DIR}
  /usr/local
  /opt/lib/usd
  /opt/usd
)

FIND_PATH(USD_ROOT_DIR
  NAMES
    pxrConfig.cmake
  HINTS
    ${_usd_SEARCH_DIRS}
)

FIND_PATH(USD_INCLUDE_DIR
  NAMES
    pxr/usd/usd/api.h
  HINTS
    ${_usd_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

FIND_LIBRARY(_usd_LIBRARY
  NAMES
  usd
  HINTS
    ${_usd_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib lib/static
)

# handle the QUIETLY and REQUIRED arguments and set USD_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(USD DEFAULT_MSG USD_ROOT_DIR _usd_LIBRARY USD_INCLUDE_DIR)

IF(USD_FOUND)
  get_filename_component(USD_LIBRARY_DIR ${_usd_LIBRARY} DIRECTORY)

  SET(USD_LIBRARIES)
  # List copied from pxrTargets.cmake, which is created when building USD.
  foreach(_usd_lib arch tf gf js trace work plug vt ar kind sdf ndr sdr pcp usd usdGeom usdVol usdLux usdShade usdRender usdHydra usdRi usdSkel usdUI usdUtils)
    list(APPEND USD_LIBRARIES "${USD_LIBRARY_DIR}/lib${_usd_lib}.a")
  endforeach()
  list(REVERSE USD_LIBRARIES)

  SET(USD_INCLUDE_DIRS ${USD_INCLUDE_DIR})
ENDIF(USD_FOUND)

MARK_AS_ADVANCED(
  USD_INCLUDE_DIR
  USD_LIBRARY_DIR
  USD_ROOT_DIR
)

UNSET(_usd_LIBRARY)
UNSET(_usd_SEARCH_DIRS)

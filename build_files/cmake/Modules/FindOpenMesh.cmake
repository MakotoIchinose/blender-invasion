# - Find OPENMESH library
# Find the native OPENMESH includes and library
# This module defines
#  OPENMESH_INCLUDE_DIRS, where to find openmesh.h, Set when
#                            OPENMESH_INCLUDE_DIR is found.
#  OPENMESH_LIBRARIES, libraries to link against to use OPENMESH.
#  OPENMESH_ROOT_DIR, The base directory to search for OPENMESH.
#                        This can also be an environment variable.
#  OPENMESH_FOUND, If false, do not try to use OPENMESH.
#
# also defined, but not for general use are
#  OPENMESH_LIBRARY, where to find the OPENMESH library.

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

# If OPENMESH_ROOT_DIR was defined in the environment, use it.
IF(NOT OPENMESH_ROOT_DIR AND NOT $ENV{OPENMESH_ROOT_DIR} STREQUAL "")
  SET(OPENMESH_ROOT_DIR $ENV{OPENMESH_ROOT_DIR})
ENDIF()

#SET(OPENMESH_INCLUDE_DIR ${OPENMESH_ROOT_DIR}/include)

SET(_openmesh_SEARCH_DIRS
  ${OPENMESH_ROOT_DIR}
  #${OPENMESH_ROOT_DIR}/include/OpenMesh/Core/Geometry
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/openmesh
  /opt/lib/openmesh
)

FIND_PATH(OPENMESH_INCLUDE_DIR
  NAMES
   OpenMesh
  HINTS
    ${_openmesh_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

FIND_LIBRARY(OPENMESH_CORE_LIBRARY
  NAMES
    OpenMeshCore
  HINTS
    ${_openmesh_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib/OpenMesh
)

FIND_LIBRARY(OPENMESH_TOOLS_LIBRARY
  NAMES
    OpenMeshTools
  HINTS
    ${_openmesh_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib/OpenMesh
)


# handle the QUIETLY and REQUIRED arguments and set OPENMESH_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OPENMESH DEFAULT_MSG
    OPENMESH_CORE_LIBRARY OPENMESH_TOOLS_LIBRARY OPENMESH_INCLUDE_DIR)

IF(OPENMESH_FOUND)
  SET(OPENMESH_LIBRARIES ${OPENMESH_CORE_LIBRARY} ${OPENMESH_TOOLS_LIBRARY})
  SET(OPENMESH_INCLUDE_DIRS ${OPENMESH_INCLUDE_DIR})
ENDIF(OPENMESH_FOUND)

MARK_AS_ADVANCED(
  OPENMESH_INCLUDE_DIR
  OPENMESH_CORE_LIBRARY
  OPENMESH_TOOLS_LIBRARY
)

UNSET(_openmesh_SEARCH_DIRS)

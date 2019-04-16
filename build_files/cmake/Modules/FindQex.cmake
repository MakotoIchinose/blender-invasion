# - Find QEX library
# Find the native QEX includes and library
# This module defines
#  QEX_INCLUDE_DIRS, where to find qex.h, Set when
#                            QEX_INCLUDE_DIR is found.
#  QEX_LIBRARIES, libraries to link against to use QEX.
#  QEX_ROOT_DIR, The base directory to search for QEX.
#                        This can also be an environment variable.
#  QEX_FOUND, If false, do not try to use QEX.
#
# also defined, but not for general use are
#  QEX_LIBRARY, where to find the QEX library.

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

# If QEX_ROOT_DIR was defined in the environment, use it.
IF(NOT QEX_ROOT_DIR AND NOT $ENV{QEX_ROOT_DIR} STREQUAL "")
  SET(QEX_ROOT_DIR $ENV{QEX_ROOT_DIR})
ENDIF()

SET(_qex_SEARCH_DIRS
  ${QEX_ROOT_DIR}
  /usr/local
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/qex
  /opt/lib/qex
)

FIND_PATH(QEX_INCLUDE_DIR
  NAMES
    qex.h
  HINTS
    ${_qex_SEARCH_DIRS}
  PATH_SUFFIXES
    include
)

FIND_LIBRARY(QEX_LIBRARY
  NAMES
    QExStatic
  HINTS
    ${_qex_SEARCH_DIRS}
  PATH_SUFFIXES
    lib64 lib
)

# handle the QUIETLY and REQUIRED arguments and set QEX_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QEX DEFAULT_MSG
    QEX_LIBRARY QEX_INCLUDE_DIR)

IF(QEX_FOUND)
  SET(QEX_LIBRARIES ${QEX_LIBRARY})
  SET(QEX_INCLUDE_DIRS ${QEX_INCLUDE_DIR})
ENDIF(QEX_FOUND)

MARK_AS_ADVANCED(
  QEX_INCLUDE_DIR
  QEX_LIBRARY
)

UNSET(_qex_SEARCH_DIRS)


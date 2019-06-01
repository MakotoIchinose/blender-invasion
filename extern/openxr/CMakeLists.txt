# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# All rights reserved.
# ***** END GPL LICENSE BLOCK *****

set(MAJOR 0)
set(MINOR 90)

# Several files use these compile-time OS switches
if(WIN32)
  add_definitions(-DXR_OS_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_definitions(-DXR_OS_LINUX)
endif()

include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(secure_getenv HAVE_SECURE_GETENV)
CHECK_FUNCTION_EXISTS(__secure_getenv HAVE___SECURE_GETENV)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/common_cmake_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/common_cmake_config.h)

set(OPENXR_SDK_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")

add_custom_target(
  generate_openxr_header
  DEPENDS
    ${OPENXR_SDK_INCLUDES}/openxr/openxr_platform_defines.h
    ${OPENXR_SDK_INCLUDES}/openxr/openxr.h
    ${OPENXR_SDK_INCLUDES}/openxr/openxr_platform.h
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(
  xr_global_generated_files
  DEPENDS
    xr_generated_dispatch_table.h
    xr_generated_dispatch_table.c
    xr_generated_utilities.h
    xr_generated_utilities.c
)

add_subdirectory(loader)

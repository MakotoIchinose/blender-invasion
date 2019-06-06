# Copyright (c) 2017 The Khronos Group Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Author:
#

#set (CMAKE_VERBOSE_MAKEFILE 1)

# Use this feature for Windows to automatically generate an exports file for the DLL.
# See https://blog.kitware.com/create-dlls-on-windows-without-declspec-using-new-cmake-export-all-feature/
include(GenerateExportHeader)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS false)

# TODO loader name is needed for lib linking in source/, so better define this higher up the source tree.
set(LOADER_NAME openxr_loader)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FALLBACK_CONFIG_DIRS "/etc/xdg" CACHE STRING
        "Search path to use when XDG_CONFIG_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant.")
    set(FALLBACK_DATA_DIRS "/usr/local/share:/usr/share" CACHE STRING
        "Search path to use when XDG_DATA_DIRS is unset or empty or the current process is SUID/SGID. Default is freedesktop compliant.")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(openxr_loader_RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/loader.rc)
    set(LOADER_NAME ${LOADER_NAME}-${MAJOR}_${MINOR})
endif()

set(INC
  ..
  ../common
  ../include
  ../../jsoncpp/include
  # For generated/configured common_cmake_config.h
  ${CMAKE_CURRENT_BINARY_DIR}/..
)

# List of all files externally generated outside of the loader that the loader
# needs to build with.
SET(LOADER_EXTERNAL_GEN_FILES
    #../../../jsoncpp/dist/jsoncpp.cpp
    ../xr_generated_dispatch_table.c
    ../xr_generated_utilities.c
)

set(SRC
  api_layer_interface.cpp
  loader_core.cpp
  loader_instance.cpp
  loader_logger.cpp
  manifest_file.cpp
  runtime_interface.cpp
  ../common/filesystem_utils.cpp
  ${LOADER_EXTERNAL_GEN_FILES}
)

set(LIB
  jsoncpp_lib
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND LIB
    -lstdc++fs -ldl -lpthread -lm
  )
endif()

blender_add_lib(${LOADER_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(
    ${LOADER_NAME}
      PRIVATE FALLBACK_CONFIG_DIRS="${FALLBACK_CONFIG_DIRS}"
      PRIVATE FALLBACK_DATA_DIRS="${FALLBACK_DATA_DIRS}"
      PRIVATE SYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}"
  )
endif()


add_custom_target(
  loader_gen_files
  DEPENDS
    xr_generated_loader.hpp
    xr_generated_loader.cpp
)

add_dependencies(
  ${LOADER_NAME}
    generate_openxr_header
    xr_global_generated_files
    loader_gen_files
#   jsoncppAmalgamatedFiles
)

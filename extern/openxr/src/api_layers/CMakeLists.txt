# Copyright (c) 2017-2019 The Khronos Group Inc.
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

# Force all compilers to output to binary folder without additional output (like Windows adds "Debug" and "Release" folders)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_CURRENT_BINARY_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

# Copy the api_layer_platform_defines.h file and place it in the binary (build) directory.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/api_layer_platform_defines.h ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

# Basics for core_validation API Layer

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/XrApiLayer_core_validation.json ${CMAKE_CURRENT_BINARY_DIR}/XrApiLayer_core_validation.json COPYONLY)

add_library(XrApiLayer_core_validation SHARED
    core_validation.cpp
    ${OPENXR_ROOT_DIR}/src/common/hex_and_handles.cpp
    ${OPENXR_ROOT_DIR}/src/common/hex_and_handles.h
    ${OPENXR_ROOT_DIR}/src/xr_generated_dispatch_table.c
    ${OPENXR_ROOT_DIR}/src/api_layers/xr_generated_core_validation.cpp
    ${OPENXR_ROOT_DIR}/src/api_layers/xr_generated_core_validation.hpp
    # Included in this list to force generation
    ${CMAKE_CURRENT_BINARY_DIR}/XrApiLayer_core_validation.json
)
set_target_properties(XrApiLayer_core_validation PROPERTIES FOLDER ${API_LAYERS_FOLDER})

add_dependencies(XrApiLayer_core_validation
    generate_openxr_header
    xr_global_generated_files
)
target_include_directories(XrApiLayer_core_validation
    PRIVATE ${OPENXR_ROOT_DIR}/src/common
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_BINARY_DIR}/include
    PRIVATE ${CMAKE_BINARY_DIR}/src
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)
if(VulkanHeaders_FOUND)
    target_include_directories(XrApiLayer_core_validation
        PRIVATE ${Vulkan_INCLUDE_DIRS}
    )
endif()

# Flag generated files
set_source_files_properties(
    ${CMAKE_BINARY_DIR}/src/xr_generated_dispatch_table.c
    PROPERTIES GENERATED TRUE
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows core_validation-specific information
    target_compile_definitions(XrApiLayer_core_validation PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Turn off transitional "changed behavior" warning message for Visual Studio versions prior to 2015.
    # The changed behavior is that constructor initializers are now fixed to clear the struct members.
    target_compile_options(XrApiLayer_core_validation PRIVATE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,19>>:/wd4351>")

    FILE(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/XrApiLayer_core_validation.def DEF_FILE)
    add_custom_target(copy-core_validation-def-file ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEF_FILE} ${CMAKE_CURRENT_BINARY_DIR}/XrApiLayer_core_validation.def
        VERBATIM
    )
	set_target_properties(copy-core_validation-def-file PROPERTIES FOLDER ${HELPER_FOLDER})
elseif(APPLE)
    # Apple core_validation-specific information
    target_compile_options(XrApiLayer_core_validation PRIVATE -Wpointer-arith -Wno-unused-function -Wno-sign-compare)
    set_target_properties(XrApiLayer_core_validation PROPERTIES LINK_FLAGS "-Wl")

else()
    # Linux core_validation-specific information
    target_compile_options(XrApiLayer_core_validation PRIVATE -Wpointer-arith -Wno-unused-function -Wno-sign-compare)
    set_target_properties(XrApiLayer_core_validation PROPERTIES LINK_FLAGS "-Wl,-Bsymbolic,--exclude-libs,ALL")
endif()

# Install explicit layers on Linux
set(TARGET_NAMES
    XrApiLayer_core_validation)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    foreach(TARGET_NAME ${TARGET_NAMES})
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.json DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/openxr/${MAJOR}/api_layers/explicit.d)
        install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endforeach()
endif()

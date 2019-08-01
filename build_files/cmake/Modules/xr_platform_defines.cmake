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


if(WIN32)
  add_definitions(-DXR_OS_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_definitions(-DXR_OS_LINUX)
endif()

# Determine the presentation backend for Linux systems.
# Use an include because the code is pretty big.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(presentation)
endif()

# Several files use these compile-time platform switches
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions( -DXR_USE_PLATFORM_WIN32 )
elseif( PRESENTATION_BACKEND MATCHES "xlib" )
    add_definitions( -DXR_USE_PLATFORM_XLIB )
elseif( PRESENTATION_BACKEND MATCHES "xcb" )
    add_definitions( -DXR_USE_PLATFORM_XCB )
elseif( PRESENTATION_BACKEND MATCHES "wayland" )
    add_definitions( -DXR_USE_PLATFORM_WAYLAND )
endif()

add_definitions(-DXR_USE_GRAPHICS_API_OPENGL)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_definitions(-DXR_USE_GRAPHICS_API_D3D)
    add_definitions(-DXR_USE_GRAPHICS_API_D3D10)
    add_definitions(-DXR_USE_GRAPHICS_API_D3D11)
    add_definitions(-DXR_USE_GRAPHICS_API_D3D12)
endif()


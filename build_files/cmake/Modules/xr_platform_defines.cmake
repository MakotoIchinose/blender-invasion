#=============================================================================
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#
# Inspired on the Testing.cmake from Libmv
#
#=============================================================================


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


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
# ***** END GPL LICENSE BLOCK *****

set(INC
  .
)

set(INC_SYS
  ${OPENXR_SDK_INCLUDE_DIR}
)

set(SRC
  intern/VAMR.cc
  intern/VAMR_Context.cc
  intern/VAMR_Event.cc
  intern/VAMR_GraphicsBinding.cc
  intern/VAMR_Session.cc

  VAMR_capi.h
  VAMR_IContext.h
  VAMR_Types.h
  intern/VAMR_Context.h
  intern/VAMR_Exception.h
  intern/VAMR_intern.h
  intern/VAMR_IGraphicsBinding.h
  intern/VAMR_openxr_includes.h
  intern/VAMR_Session.h
)

if(WIN32)
  list(APPEND LIB
    shlwapi
  )
elseif(WITH_X11)
  add_definitions(-DWITH_X11)
endif()


add_definitions(${GL_DEFINITIONS})

include(xr_platform_defines)

blender_add_lib(bf_intern_vamr "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

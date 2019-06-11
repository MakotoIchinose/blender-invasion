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
# The Original Code is Copyright (C) 2006, Blender Foundation
# All rights reserved.
# ***** END GPL LICENSE BLOCK *****

include(/opt/usd/pxrConfig.cmake)

set(INC
  .
  ../blenkernel
  ../blenlib
  ../blenloader
  ../bmesh
  ../depsgraph
  ../editors/include
  ../makesdna
  ../makesrna
  ../windowmanager
  ../../../intern/guardedalloc
  ../../../intern/utfconv
)

set(INC_SYS
  ${PXR_INCLUDE_DIRS}
  ${BOOST_INCLUDE_DIR}
)

set(SRC
  intern/usd_capi.cc
  intern/usd_exporter.cc

  usd.h
  intern/usd_exporter.h
)

set(LIB
  bf_blenkernel
  bf_blenlib
  ${PXR_LIBRARIES}
)

blender_add_lib(bf_usd "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

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
# The Original Code is Copyright (C) 2019, Blender Foundation
# All rights reserved.
# ***** END GPL LICENSE BLOCK *****

# This suppresses the warning "This file includes at least one deprecated or antiquated header which
# may be removed without further notice at a future date", which is caused by the USD library
# including <ext/hash_set>. Nothing we can do about that until they change what
# they include, so this just suppresses it.
add_definitions(-D_GLIBCXX_PERMIT_BACKWARD_HASH)

set(INC
  .
  ..
  ../../../source/blender/blenlib
  ../../../source/blender/blenkernel
  ../../../source/blender/usd
  ../../../source/blender/makesdna
  ../../../source/blender/depsgraph
  ${USD_INCLUDE_DIRS}
  ${BOOST_INCLUDE_DIR}
  ${TBB_INCLUDE_DIR}
)

set(LIB
  bf_blenloader_test
  bf_blenloader

  # Should not be needed but gives windows linker errors if the ocio libs are linked before this:
  bf_intern_opencolorio
  bf_gpu

  bf_usd
)

include_directories(${INC})

setup_libdirs()
get_property(BLENDER_SORTED_LIBS GLOBAL PROPERTY BLENDER_SORTED_LIBS_PROP)

set(SRC
  abstract_hierarchy_iterator_test.cc
  hierarchy_context_order_test.cc
  usd_stage_creation_test.cc
)

if(WITH_BUILDINFO)
  list(APPEND SRC "$<TARGET_OBJECTS:buildinfoobj>")
endif()

BLENDER_SRC_GTEST_EX(
  NAME usd
  SRC "${SRC}"
  EXTRA_LIBS "${LIB}"
  COMMAND_ARGS --test-assets-dir "${CMAKE_SOURCE_DIR}/../lib/tests")

setup_liblinks(usd_test)

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

set(OPENMESH_EXTRA_ARGS
  -DBUILD_APPS=OFF
  -DDISABLE_QMAKE_BUILD=ON
)

ExternalProject_Add(external_openmesh
  URL ${OPENMESH_URI}
  DOWNLOAD_DIR ${DOWNLOAD_DIR}
  URL_HASH SHA256=${OPENMESH_HASH}
  PREFIX ${BUILD_DIR}/openmesh
  PATCH_COMMAND ${PATCH_CMD} -p 0 -d ${BUILD_DIR}/openmesh/src/external_openmesh < ${PATCH_DIR}/openmesh.diff
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/openmesh -Wno-dev ${DEFAULT_CMAKE_FLAGS} ${OPENMESH_EXTRA_ARGS}
  INSTALL_DIR ${LIBDIR}/openmesh
)

if (BUILD_MODE STREQUAL Debug)
ExternalProject_Add_Step(external_openmesh after_install
        COMMAND ${CMAKE_COMMAND} -E rename ${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshCored.a
                                           ${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshCore.a

        COMMAND ${CMAKE_COMMAND} -E rename ${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshToolsd.a
                                           ${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshTools.a

        DEPENDEES install
)
endif()


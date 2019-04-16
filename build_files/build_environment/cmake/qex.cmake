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

set(QEX_EXTRA_ARGS
        -DOPENMESH_CORE_LIBRARY=${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshCore.a
        -DOPENMESH_INCLUDE_DIR=${LIBDIR}/openmesh/include
        -DOPENMESH_LIBRARY_DIR=${LIBDIR}/openmesh/lib/OpenMesh
        -DOPENMESH_TOOLS_LIBRARY=${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshTools.a
)

ExternalProject_Add(external_qex
	URL ${QEX_URI}
	DOWNLOAD_DIR ${DOWNLOAD_DIR}
        URL_HASH SHA256=${QEX_HASH}
	PREFIX ${BUILD_DIR}/qex
        PATCH_COMMAND ${PATCH_CMD} -p 0 -d ${BUILD_DIR}/qex/src/external_qex < ${PATCH_DIR}/qex.diff
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/qex -Wno-dev ${DEFAULT_CMAKE_FLAGS} ${QEX_EXTRA_ARGS}
        INSTALL_DIR ${LIBDIR}/qex
)

#grr, INSTALL(TARGETS has no exclude pattern, so removing unnecessary empty dirs here
ExternalProject_Add_Step(external_qex after_install
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LIBDIR}/qex/lib/src
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LIBDIR}/qex/lib/interfaces
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LIBDIR}/qex/lib/demo
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LIBDIR}/qex/include/src
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LIBDIR}/qex/include/interfaces
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${LIBDIR}/qex/include/demo
        DEPENDEES install
)

add_dependencies(
        external_qex
        external_igl
        external_openmesh
)


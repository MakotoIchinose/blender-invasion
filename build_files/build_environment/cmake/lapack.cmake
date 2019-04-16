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

set(LAPACK_EXTRA_ARGS
        #-DOPENMESH_CORE_LIBRARY=${LIBDIR}/openmesh/lib/OpenMesh/libOpenMeshCore.a
)

ExternalProject_Add(external_lapack
        URL ${LAPACK_URI}
        DOWNLOAD_DIR ${DOWNLOAD_DIR}
        URL_HASH SHA256=${LAPACK_HASH}
        PREFIX ${BUILD_DIR}/lapack
       # PATCH_COMMAND ${PATCH_CMD} -p 0 -d ${BUILD_DIR}/qex/src/external_qex < ${PATCH_DIR}/qex.diff
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/lapack -Wno-dev ${DEFAULT_CMAKE_FLAGS} ${LAPACK_EXTRA_ARGS}
        INSTALL_DIR ${LIBDIR}/lapack
)

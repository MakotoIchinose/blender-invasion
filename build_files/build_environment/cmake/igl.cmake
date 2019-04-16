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

set(IGL_EXTRA_ARGS
	-DLIBIGL_BUILD_PYTHON=OFF
	-DLIBIGL_BUILD_TESTS=OFF
	-DLIBIGL_BUILD_TUTORIALS=OFF
	-DLIBIGL_USE_STATIC_LIBRARY=ON
	-DLIBIGL_WITHOUT_COPYLEFT=OFF
	-DLIBIGL_WITH_CGAL=OFF
	-DLIBIGL_WITH_COMISO=ON
	-DLIBIGL_WITH_CORK=OFF
	-DLIBIGL_WITH_EMBREE=OFF
	-DLIBIGL_WITH_MATLAB=OFF
	-DLIBIGL_WITH_MOSEK=OFF
	-DLIBIGL_WITH_OPENGL=OFF
	-DLIBIGL_WITH_OPENGL_GLFW=OFF
	-DLIBIGL_WITH_OPENGL_GLFW_IMGUI=OFF
	-DLIBIGL_WITH_PNG=OFF
	-DLIBIGL_WITH_PYTHON=OFF
	-DLIBIGL_WITH_TETGEN=OFF
	-DLIBIGL_WITH_TRIANGLE=OFF
	-DLIBIGL_WITH_XML=OFF
	-DBLAS_LIBRARIES=${LIBDIR}/lapack/lib/libblas.a
)

ExternalProject_Add(external_igl
	URL ${IGL_URI}
	DOWNLOAD_DIR ${DOWNLOAD_DIR}
	URL_HASH SHA256=${IGL_HASH}
	PREFIX ${BUILD_DIR}/igl
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/igl -Wno-dev ${DEFAULT_CMAKE_FLAGS} ${IGL_EXTRA_ARGS}
	INSTALL_DIR ${LIBDIR}/igl
)

ExternalProject_Add_Step(external_igl after_install
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${BUILD_DIR}/igl/src/external_igl/include/igl/copyleft/comiso
                                                   ${LIBDIR}/igl/include/igl/copyleft/comiso

        COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_DIR}/igl/src/external_igl-build/libigl_comiso.a
                                              ${LIBDIR}/igl/lib/libigl_comiso.a

        COMMAND ${CMAKE_COMMAND} -E copy ${BUILD_DIR}/igl/src/external_igl-build/libCoMISo.a
                                            ${LIBDIR}/igl/lib/libCoMISo.a

        COMMAND ${CMAKE_COMMAND} -E copy_directory ${BUILD_DIR}/igl/src/external_igl/external/eigen/Eigen
                                                    ${LIBDIR}/igl/include/eigen/Eigen

        DEPENDEES install
)

add_dependencies(
        external_igl
        external_lapack
)

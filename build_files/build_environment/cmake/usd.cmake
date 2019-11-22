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

set(USD_EXTRA_ARGS
  -DBoost_COMPILER:STRING=${BOOST_COMPILER_STRING}
  -DBoost_USE_MULTITHREADED=ON
  -DBoost_USE_STATIC_LIBS=ON
  -DBoost_USE_STATIC_RUNTIME=OFF
  -DBOOST_ROOT=${LIBDIR}/boost
  -DTBB_INCLUDE_DIRS=${LIBDIR}/tbb/include
  -DTBB_LIBRARIES=${LIBDIR}/tbb/lib/${LIBPREFIX}tbb_static${LIBEXT}
  -DTbb_TBB_LIBRARY=${LIBDIR}/tbb/lib/${LIBPREFIX}tbb_static${LIBEXT}
  -DPXR_ENABLE_PYTHON_SUPPORT=OFF
  -DPXR_BUILD_IMAGING=OFF
  -DPXR_BUILD_TESTS=OFF
  -DBUILD_SHARED_LIBS=OFF
  -DPYTHON_EXECUTABLE=${PYTHON_BINARY}
  -DCMAKE_DEBUG_POSTFIX=_d
)

ExternalProject_Add(external_usd
  URL ${USD_URI}
  DOWNLOAD_DIR ${DOWNLOAD_DIR}
  URL_HASH MD5=${USD_HASH}
  PREFIX ${BUILD_DIR}/usd
  PATCH_COMMAND ${PATCH_CMD} -p 1 -d ${BUILD_DIR}/usd/src/external_usd < ${PATCH_DIR}/usd.diff
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${LIBDIR}/usd -Wno-dev ${DEFAULT_CMAKE_FLAGS} ${USD_EXTRA_ARGS}
  INSTALL_DIR ${LIBDIR}/usd
)

add_dependencies(
  external_usd
  external_tbb
  external_boost
)

if(WIN32)
  if(BUILD_MODE STREQUAL Release)
    ExternalProject_Add_Step(external_usd after_install
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/usd/ ${HARVEST_TARGET}/usd
      DEPENDEES install
    )
  endif()
  if(BUILD_MODE STREQUAL Debug)
    ExternalProject_Add_Step(external_usd after_install
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBDIR}/usd/lib ${HARVEST_TARGET}/usd/lib
      DEPENDEES install
    )
  endif()

endif()


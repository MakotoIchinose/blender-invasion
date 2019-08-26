cmake_minimum_required(VERSION 3.1)
project(QuadriFlow LANGUAGES CXX)

if(DEFINED QUADRIFLOW_CMAKE_CFG)
	include(${QUADRIFLOW_CMAKE_CFG})
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEMON_3RD_PATH 3rd/lemon-1.3.1)

set(LEMON_SOURCE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${LEMON_3RD_PATH})

set(LEMON_SRC ${LEMON_SOURCE_ROOT_DIR}/lemon)
set(LEMON_INCLUDE_DIRS ${LEMON_SOURCE_ROOT_DIR})

CONFIGURE_FILE(
  ${LEMON_SRC}/config.h.in
  ${LEMON_SRC}/config.h
  )
set(LEMON_SOURCES
  ${LEMON_SRC}/arg_parser.cc
  ${LEMON_SRC}/base.cc
  ${LEMON_SRC}/color.cc
  ${LEMON_SRC}/lp_base.cc
  ${LEMON_SRC}/lp_skeleton.cc
  ${LEMON_SRC}/random.cc
  ${LEMON_SRC}/bits/windows.cc
  )

include_directories(src)
include_directories(3rd/pcg32)
include_directories(3rd/pss)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${EIGEN_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${GLUT_INCLUDE_DIRS})
include_directories(${LEMON_INCLUDE_DIRS})
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${GUROBI_INCLUDE_DIRS})

if (BUILD_PERFORMANCE_TEST)
    add_definitions(-DPERFORMANCE_TEST)
endif(BUILD_PERFORMANCE_TEST)

if (BUILD_OPENMP)
    add_definitions(-DWITH_OMP)
endif(BUILD_OPENMP)

if (BUILD_LOG)
    add_definitions(-DLOG_OUTPUT)
endif(BUILD_LOG)

if (BUILD_GUROBI)
    add_definitions(-DWITH_GUROBI)
endif(BUILD_GUROBI)

if (BUILD_TBB)
    add_definitions(-DWITH_TBB)
endif(BUILD_TBB)

if (BUILD_FREE_LICENSE)
    add_definitions(-DEIGEN_MPL2_ONLY)
endif(BUILD_FREE_LICENSE)

if(BUILD_STATIC_LIB)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

set(
    quadriflow_SRC
    src/adjacent-matrix.cpp
    src/adjacent-matrix.hpp
    src/compare-key.hpp
    src/config.hpp
    src/dedge.cpp
    src/dedge.hpp
    src/disajoint-tree.hpp
    src/dset.hpp
    src/field-math.hpp
    src/flow.hpp
    src/hierarchy.cpp
    src/hierarchy.hpp
    src/loader.cpp
    src/loader.hpp
    src/localsat.cpp
    src/localsat.hpp
    src/merge-vertex.cpp
    src/merge-vertex.hpp
    src/optimizer.cpp
    src/optimizer.hpp
    src/parametrizer.cpp
    src/parametrizer-flip.cpp
    src/parametrizer-int.cpp
    src/parametrizer-mesh.cpp
    src/parametrizer-scale.cpp
    src/parametrizer-sing.cpp
    src/parametrizer.hpp
    src/serialize.hpp
    src/subdivide.cpp
    src/subdivide.hpp
)

add_library( qflow ${LIB_TYPE}
    ${quadriflow_SRC}
    ${LEMON_SRC}
)
target_link_libraries(
    qflow
    ${TBB_LIBRARIES}
    ${LEMON_LIBRARIES}
    ${GUROBI_LIBRARIES}
)

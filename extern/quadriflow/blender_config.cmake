# Do NOT use CACHE behavior here, as this has the same effect as defining an option,
# it exposes the setting in main CMake config (which we do not want here).

set(QUADRIFLOW_STANDALONE FALSE)
set(BUILD_PERFORMANCE_TEST FALSE)
set(BUILD_LOG FALSE)
set(BUILD_GUROBI FALSE)
set(BUILD_OPENMP ${WITH_OPENMP})
set(BUILD_TBB FALSE)
set(BUILD_FREE_LICENSE FALSE) #If TRUE, remove LGPL code
set(BUILD_STATIC_LIB TRUE)

set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
set(EIGEN_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIRS})

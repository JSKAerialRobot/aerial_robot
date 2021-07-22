cmake_minimum_required(VERSION 2.8.3)
project(hydrus)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  aerial_robot_msgs
  aerial_robot_control
  aerial_robot_model
  spinal
  eigen_conversions
  tf_conversions
  std_srvs
  dynamic_reconfigure
  )

find_package(Eigen3 REQUIRED)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

generate_dynamic_reconfigure_options(
  cfg/LQI.cfg)

catkin_package(
  INCLUDE_DIRS include test
  LIBRARIES hydrus_robot_model hydrus_controller_pluginlib hydrus_numerical_jacobians
  CATKIN_DEPENDS roscpp aerial_robot_control aerial_robot_model aerial_robot_msgs spinal tf_conversions eigen_conversions
  DEPENDS EIGEN3
)

# Eigen requires optimization to get good performance
# http://eigen.tuxfamily.org/index.php?title=FAQ#Optimization
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")


###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
# include_directories(include)
include_directories(
  include
  test
  ${Boost_INCLUDE_DIR}
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
)

macro(get_urdf_to_collada _urdf_to_collada_exe)
  find_package(collada_urdf_jsk_patch QUIET)
  find_package(collada_urdf REQUIRED)
  if (collada_urdf_jsk_patch_FOUND)
    set(${_urdf_to_collada_exe} ${collada_urdf_jsk_patch_PREFIX}/lib/collada_urdf_jsk_patch/urdf_to_collada)
  elseif (collada_urdf_FOUND)
    set(${_urdf_to_collada_exe} ${collada_urdf_PREFIX}/lib/collada_urdf/urdf_to_collada)
  endif (collada_urdf_jsk_patch_FOUND)
  if(NOT EXISTS "${${_urdf_to_collada_exe}}")
    message(FATAL_ERROR "could not find ${${_urdf_to_collada_exe}}")
  endif()
endmacro(get_urdf_to_collada _urdf_to_collada_exe)

get_urdf_to_collada(_urdf_to_collada_exe)
message(STATUS "Found ${_urdf_to_collada_exe}")

# hydrus euslisp model generation
add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.l
  COMMAND rosrun euscollada collada2eus hydrus.dae hydrus.yaml hydrus.l
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907
  DEPENDS ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.dae
          ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.yaml)
add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.dae
  COMMAND ${_urdf_to_collada_exe} hydrus.urdf hydrus.dae
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907
  DEPENDS ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.urdf)
add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.urdf
  COMMAND ROS_PACKAGE_PATH=${PROJECT_SOURCE_DIR}:$ENV{ROS_PACKAGE_PATH} rosrun xacro xacro robot.urdf.xacro > hydrus.urdf
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907
  DEPENDS ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/robot.urdf.xacro
          ${PROJECT_SOURCE_DIR}/urdf/common.xacro
          ${PROJECT_SOURCE_DIR}/urdf/link.urdf.xacro)

# gte collada-dom version
find_package(PkgConfig)
pkg_check_modules(COLLADA collada-dom>=2.4.4)
if(${COLLADA_FOUND})
  add_custom_target(generate_hydrus_default_lisp ALL DEPENDS ${PROJECT_SOURCE_DIR}/robots/quad/default_mode_201907/hydrus.l)
else()
  pkg_check_modules(COLLADA collada-dom)
  message(WARNING "urdf_to_collada requries collada-dom >= 2.4.4, installed version is ${COLLADA_VERSION}")
endif()

add_library(hydrus_robot_model
  src/hydrus_robot_model.cpp
  src/hydrus_tilted_robot_model.cpp)
target_link_libraries(hydrus_robot_model ${catkin_LIBRARIES})

### flight control plugin
add_library(hydrus_controller_pluginlib
  src/hydrus_lqi_controller.cpp
  src/hydrus_tilted_lqi_controller.cpp)
target_link_libraries(hydrus_controller_pluginlib hydrus_robot_model ${catkin_LIBRARIES} ${EIGEN3_LIBRARIES})
add_dependencies(hydrus_controller_pluginlib aerial_robot_msgs_generate_messages_cpp spinal_generate_messages_cpp ${PROJECT_NAME}_gencfg)

# test
# pre-build test code
add_library(hydrus_numerical_jacobians test/hydrus/numerical_jacobians.cpp)
target_link_libraries(hydrus_numerical_jacobians hydrus_robot_model ${catkin_LIBRARIES})
add_executable(hydrus_jacobian_test test/hydrus/jacobian_test.cpp)
target_link_libraries(hydrus_jacobian_test hydrus_numerical_jacobians ${catkin_LIBRARIES} ${GTEST_LIBRARIES})

if(CATKIN_ENABLE_TESTING)
  find_package(catkin REQUIRED COMPONENTS rostest)
  add_subdirectory(test)
endif()


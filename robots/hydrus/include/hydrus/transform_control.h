// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2016, JSK Lab
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/o2r other materials provided
 *     with the distribution.
 *   * Neither the name of the JSK Lab nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/


#ifndef TRANSFORM_CONTROL_H
#define TRANSFORM_CONTROL_H

/* ros */
#include <ros/ros.h>

#include <spinal/RollPitchYawTerms.h>
#include <aerial_robot_msgs/FourAxisGain.h>
#include <sensor_msgs/JointState.h>
#include <aerial_robot_model/AddExtraModule.h>
#include <hydrus/TargetPose.h>
#include <spinal/DesireCoord.h>
#include <spinal/PMatrixPseudoInverseWithInertia.h>
#include <std_msgs/UInt8.h>
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf_conversions/tf_kdl.h>
#include <tf_conversions/tf_eigen.h>

/* robot model */
#include <urdf/model.h>
#include <kdl/tree.hpp>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/treefksolverpos_recursive.hpp>
#include <kdl/treejnttojacsolver.hpp>
#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainjnttojacsolver.hpp>

/* for eigen cumputation */
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <Eigen/Eigenvalues>

/* kinematics */
#include <aerial_robot_model/transformable_aerial_robot_model.h>

/* util */
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#include <iomanip>
#include <cmath>

/* for dynamic reconfigure */
#include <dynamic_reconfigure/server.h>
#include <hydrus/LQIConfig.h>
#define LQI_GAIN_FLAG 0
#define LQI_RP_P_GAIN 1
#define LQI_RP_I_GAIN 2
#define LQI_RP_D_GAIN 3
#define LQI_Y_P_GAIN 4
#define LQI_Y_I_GAIN 5
#define LQI_Y_D_GAIN 6
#define LQI_Z_P_GAIN 7
#define LQI_Z_I_GAIN 8
#define LQI_Z_D_GAIN 9


class TransformController{
public:
  TransformController(ros::NodeHandle nh, ros::NodeHandle nh_private);
  ~TransformController();

  virtual void actuatorStateCallback(const sensor_msgs::JointStateConstPtr& state);
  bool stabilityMarginCheck(bool verbose = false);
  virtual bool overlapCheck(bool verbose = false){return true; }
  bool modelling(bool verbose = false); //lagrange method

  /* static & stability */
  /** public attributes */
  double joint_angle_min_, joint_angle_max_;
  double stability_margin_thre_;
  double f_max_, f_min_;
  double p_det_thre_;
  double getStabilityMargin() const
  {
    return stability_margin_;
  }
  const Eigen::MatrixXd& getP() const
  {
    return P_;
  }
  double getPdeterminant() const
  {
    return p_det_;
  }
  void setStableState(Eigen::VectorXd f)
  {
    optimal_hovering_f_ = f;
  }
  const Eigen::VectorXd& getOptimalHoveringThrust() const
  {
    return optimal_hovering_f_;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  /* control */
  static constexpr uint8_t LQI_THREE_AXIS_MODE = 3;
  static constexpr uint8_t LQI_FOUR_AXIS_MODE = 4;

  const Eigen::MatrixXd& getK() const
  {
    return K_;
  }
  uint8_t getLqiMode() const
  {
    return lqi_mode_;
  }
  void setLqiMode(uint8_t lqi_mode)
  {
    lqi_mode_ = lqi_mode;
  }
  void param2controller();
  bool hamiltonMatrixSolver(uint8_t lqi_mode);

protected:

  ros::NodeHandle nh_, nh_private_;

  bool kinematic_verbose_;
  bool control_verbose_;
  bool debug_verbose_;
  bool verbose_;

  aerial_robot_model::RobotModel kinematic_model_;
  bool kinematics_flag_;
  double stability_margin_;
  Eigen::VectorXd optimal_hovering_f_;
  Eigen::MatrixXd P_;
  double m_f_rate_; //moment / force rate
  double p_det_;
  int rotor_num_;
  double link_length_;
  std::string baselink_;
  std::string thrust_link_;
  sensor_msgs::JointState current_actuator_state_;

  /* ros param init */
  void initParam();

  /* basic model */
  void desireCoordinateCallback(const spinal::DesireCoordConstPtr& msg);

  /* service */
  ros::ServiceServer add_extra_module_service_;
  ros::ServiceServer end_effector_ik_service_; //need?

  ros::Subscriber actuator_state_sub_;
  ros::Subscriber desire_coordinate_sub_;
  tf2_ros::TransformBroadcaster br_;

  ros::Publisher transform_pub_;
  ros::Publisher rpy_gain_pub_;
  ros::Publisher four_axis_gain_pub_;
  ros::Publisher p_matrix_pseudo_inverse_inertia_pub_;

  std::thread control_thread_;
  std::mutex mutex_;
  double control_rate_;
  bool only_three_axis_mode_;
  bool gyro_moment_compensation_;

  Eigen::MatrixXd K_;
  Eigen::MatrixXd P_orig_pseudo_inverse_; // for compensation of cross term in the rotional dynamics
  //Q: 8/12:r,r_d, p, p_d, y, y_d, z. z_d, r_i, p_i, y_i, z_i
  //   6/9:r,r_d, p, p_d, z. z_d, r_i, p_i, z_i
  Eigen::VectorXd q_diagonal_;
  double q_roll_,q_roll_d_,q_pitch_,q_pitch_d_,q_yaw_,strong_q_yaw_, q_yaw_d_,q_z_,q_z_d_;
  double q_roll_i_,q_pitch_i_,q_yaw_i_,q_z_i_;
  uint8_t lqi_mode_;
  bool a_dash_eigen_calc_flag_;
  std::vector<double> r_; // matrix R

  virtual void control();
  /* LQI parameter calculation */
  void lqi();

  //dynamic reconfigure
  dynamic_reconfigure::Server<hydrus::LQIConfig> lqi_server_;
  dynamic_reconfigure::Server<hydrus::LQIConfig>::CallbackType dynamic_reconf_func_lqi_;
  void cfgLQICallback(hydrus::LQIConfig &config, uint32_t level);
};


#endif

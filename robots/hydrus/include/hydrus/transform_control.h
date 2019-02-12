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

#pragma once

#include <aerial_robot_model/transformable_aerial_robot_model_ros.h>
#include <aerial_robot_msgs/FourAxisGain.h>
#include <dynamic_reconfigure/server.h>
#include <hydrus/hydrus_robot_model.h>
#include <hydrus/LQIConfig.h>
#include <mutex>
#include <sensor_msgs/JointState.h>
#include <spinal/PMatrixPseudoInverseWithInertia.h>
#include <spinal/RollPitchYawTerms.h>
#include <ros/ros.h>
#include <thread>

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


class TransformController : public aerial_robot_model::RobotModelRos {
public:
  TransformController(ros::NodeHandle nh, ros::NodeHandle nh_private, std::unique_ptr<HydrusRobotModel> robot_model = std::make_unique<HydrusRobotModel>(true));
  virtual ~TransformController();

protected:
  //protected functions
  HydrusRobotModel& getRobotModel() const { return static_cast<HydrusRobotModel&>(RobotModelRos::getRobotModel()); }
private:
  //private attributes
  bool a_dash_eigen_calc_flag_;
  double control_rate_;
  std::thread control_thread_;
  bool control_verbose_;
  bool debug_verbose_;
  dynamic_reconfigure::Server<hydrus::LQIConfig>::CallbackType dynamic_reconf_func_lqi_;
  ros::Publisher four_axis_gain_pub_;
  ros::Publisher desired_orientation_pub_;
  bool gyro_moment_compensation_;
  Eigen::MatrixXd K_;
  dynamic_reconfigure::Server<hydrus::LQIConfig> lqi_server_;
  std::mutex mutex_;
  ros::NodeHandle nh_;
  ros::NodeHandle nh_private_;
  ros::Publisher p_matrix_pseudo_inverse_inertia_pub_;
  Eigen::VectorXd q_diagonal_;
  double q_pitch_;
  double q_pitch_d_;
  double q_pitch_i_;
  double q_roll_;
  double q_roll_d_;
  double q_roll_i_;
  double q_yaw_;
  double q_yaw_d_;
  double q_yaw_i_;
  double q_z_;
  double q_z_d_;
  double q_z_i_;
  std::vector<double> r_; // matrix R
  ros::Publisher rpy_gain_pub_;
  double strong_q_yaw_;
  bool verbose_;

  //private functions
  void cfgLQICallback(hydrus::LQIConfig &config, uint32_t level); //dynamic reconfigure
  virtual void control();
  bool hamiltonMatrixSolver();
  void initParam();
  void lqi(); // LQI parameter calculation
  void param2controller();
};

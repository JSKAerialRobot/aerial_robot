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

#include <hydrus/hydrus_robot_model.h>
#include <aerial_robot_model/eigen_utils.h>

class HydrusXiRobotModel : public HydrusRobotModel {
public:
  HydrusXiRobotModel(bool init_with_rosparam,
                     bool verbose = false,
                     std::string baselink = std::string(""),
                     std::string thrust_link = std::string(""),
                     double stability_margin_thre = 0,
                     double p_det_thre = 0,
                     double f_max = 0,
                     double f_min = 0,
                     double m_f_rate = 0,
                     bool only_three_axis_mode = false);
  virtual ~HydrusXiRobotModel() = default;

  bool modelling(bool verbose = false, bool control_verbose = false) override;
  Eigen::MatrixXd calcWrenchAllocationMatrix();
  std::vector<double> calcJointTorque(); //joint only, not including gimbal
  double getMFRate() {return m_f_rate_;}
  Eigen::MatrixXd getJacobian(const sensor_msgs::JointState& joint_state, std::string segment_name);
  inline Eigen::MatrixXd convertJacobian(const Eigen::MatrixXd& in);
  Eigen::MatrixXd getCOGJacobian(const sensor_msgs::JointState& joint_state);

private:
  std::map<std::string, std::vector<std::string> > joint_thrust_map_;
  void makeJointThrustMap();

  int link_joint_num_;
  KDL::Tree tree_with_cog_;

  //  bool stabilityMarginCheck(bool verbose = false) override;
};

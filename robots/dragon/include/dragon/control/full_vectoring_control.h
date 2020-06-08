// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2017, JSK Lab
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

#include <aerial_robot_control/control/pose_linear_controller.h>
#include <dragon/model/full_vectoring_robot_model.h>
#include <geometry_msgs/WrenchStamped.h>
#include <spinal/FourAxisCommand.h>
#include <spinal/RollPitchYawTerm.h>
#include <spinal/TorqueAllocationMatrixInv.h>
#include <std_msgs/Float32MultiArray.h>
#include <tf_conversions/tf_eigen.h>


namespace aerial_robot_control
{
  class DragonFullVectoringController: public PoseLinearController
  {
  public:
    DragonFullVectoringController();
    ~DragonFullVectoringController()
    {
      wrench_estimate_thread_.interrupt();
      wrench_estimate_thread_.join();
    }

    void initialize(ros::NodeHandle nh, ros::NodeHandle nhp,
                    boost::shared_ptr<aerial_robot_model::RobotModel> robot_model,
                    boost::shared_ptr<aerial_robot_estimation::StateEstimator> estimator,
                    boost::shared_ptr<aerial_robot_navigation::BaseNavigator> navigator,
                    double ctrl_loop_rate) override;

  private:

    ros::Publisher flight_cmd_pub_; //for spinal
    ros::Publisher gimbal_control_pub_;
    ros::Publisher target_vectoring_force_pub_;
    ros::Publisher estimate_external_wrench_pub_;

    boost::shared_ptr<Dragon::FullVectoringRobotModel> dragon_robot_model_;
    boost::shared_ptr<aerial_robot_model::RobotModel> robot_model_for_control_;
    std::vector<float> target_base_thrust_;
    std::vector<double> target_gimbal_angles_;
    Eigen::VectorXd target_vectoring_f_;
    bool decoupling_;
    bool gimbal_vectoring_check_flag_;
    double allocation_refine_threshold_;
    int allocation_refine_max_iteration_;
    Eigen::VectorXd target_wrench_acc_cog_;

    /* external wrench */
    boost::mutex wrench_mutex_;
    boost::thread wrench_estimate_thread_;
    Eigen::VectorXd init_sum_momentum_;
    Eigen::VectorXd est_external_wrench_;
    Eigen::MatrixXd momentum_observer_matrix_;
    Eigen::VectorXd integrate_term_;
    double prev_est_wrench_timestamp_;

    void externalWrenchEstimate();
    const Eigen::VectorXd getTargetWrenchAccCog()
    {
      boost::lock_guard<boost::mutex> lock(wrench_mutex_);
      return target_wrench_acc_cog_;
    }
    void setTargetWrenchAccCog(const Eigen::VectorXd target_wrench_acc_cog)
    {
      boost::lock_guard<boost::mutex> lock(wrench_mutex_);
      target_wrench_acc_cog_ = target_wrench_acc_cog;
    }

    void controlCore() override;
    void rosParamInit();
    void sendCmd();
  };
};

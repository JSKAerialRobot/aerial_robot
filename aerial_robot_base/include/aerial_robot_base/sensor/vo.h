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

/* ros */
#include <ros/ros.h>

/* base class */
#include <aerial_robot_base/sensor/base_plugin.h>

/* filter */
#include <kalman_filter/kf_pos_vel_acc_plugin.h>

/* ros msg */
#include <geometry_msgs/Vector3Stamped.h>
#include <nav_msgs/Odometry.h>
#include <spinal/ServoControlCmd.h>
#include <std_msgs/Empty.h>
#include <sensor_msgs/JointState.h>


namespace sensor_plugin
{
  class VisualOdometry :public sensor_plugin::SensorBase
  {
  public:

    VisualOdometry();
    ~VisualOdometry(){}

    void initialize(ros::NodeHandle nh, ros::NodeHandle nhp, StateEstimator* estimator, string sensor_name);
    inline const bool odomPosMode(){return !full_vel_mode_;}

  private:
    /* ros */
    ros::Subscriber vo_sub_;
    ros::Publisher vo_state_pub_;
    ros::Publisher vo_servo_pub_;
    ros::Subscriber vo_servo_debug_sub_;
    ros::Timer  servo_control_timer_;

    /* ros param */
    double level_pos_noise_sigma_;
    double z_pos_noise_sigma_;
    double vel_noise_sigma_;
    double vel_outlier_thresh_;
    double downwards_vo_min_height_;
    double downwards_vo_max_height_;
    bool full_vel_mode_;
    bool z_vel_mode_;
    /* heuristic sepecial flag for fusion */
    bool outdoor_;
    bool z_no_delay_;

    bool debug_verbose_;

    /* servo */
    std::string joint_name_;
    bool servo_auto_change_flag_;
    double servo_height_thresh_;
    double servo_angle_;
    double servo_init_angle_, servo_downwards_angle_;
    double servo_vel_;
    double servo_control_rate_;

    double servo_min_angle_, servo_max_angle_;
    int servo_index_;
    tf::TransformBroadcaster br_;

    tf::Transform world_offset_tf_; // ^{w}H_{w_vo}: transform from true world frame to the vo/vio world frame
    tf::Transform baselink_tf_; // ^{w}H_{b}: transform from true world frame to the baselink frame, but is estimated by vo/vio
    tf::Vector3 raw_global_vel_;
    aerial_robot_msgs::States vo_state_;

    void rosParamInit();
    void servoControl(const ros::TimerEvent & e);
    void estimateProcess();
    void voCallback(const nav_msgs::Odometry::ConstPtr & vo_msg);

    void servoDebugCallback(const std_msgs::Empty::ConstPtr & msg)
    {
      servo_auto_change_flag_ = true;
    }
  };
};



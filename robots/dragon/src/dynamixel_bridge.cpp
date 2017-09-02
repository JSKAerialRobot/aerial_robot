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


#include <dragon/dynamixel_bridge.h>

using namespace std;

namespace dragon
{

  JointInterface::JointInterface(ros::NodeHandle nh, ros::NodeHandle nhp)
    : hydrus::JointInterface(nh, nhp), gimbals_(0), start_gimbal_control_(false)
  {
    nhp_.param("gimbal_num", gimbal_num_, 8);

    for(int i = 0; i < gimbal_num_; i++)
      gimbals_.push_back(JointHandlePtr(new JointHandle(ros::NodeHandle(nh, "gimbal"), ros::NodeHandle(nhp, "gimbal"), i)));

    string topic_name;
    gimbal_ctrl_sub_ = nh_.subscribe("gimbals_ctrl", 1, &JointInterface::gimbalsCtrlCallback, this) ;
    nhp_.param("gimbal_pub_name", topic_name, std::string("/target_gimbal_states"));
    gimbal_ctrl_pub_ = nh_.advertise<hydrus::ServoControl>(topic_name, 1);
    nhp_.param("gimbal_config_cmd_pub_name", topic_name, std::string("/gimbal_config_cmd"));
    gimbal_config_cmd_pub_ = nh_.advertise<hydrus::ServoConfigCmd>(topic_name, 1);


    gimbals_torque_control_srv_ =  nh_.advertiseService("/gimbals_controller/torque_enable", &JointInterface::gimbalsTorqueEnableCallback, this);

  }

  void JointInterface::gimbalsCtrlCallback(const sensor_msgs::JointStateConstPtr& gimbals_ctrl_msg)
  {
    assert(gimbals_ctrl_msg->position.size() == gimbal_num_);

    hydrus::ServoControl target_angle_msg;

    for(int i = 0; i < gimbal_num_; i ++)
      {
        // TODO: right now it is reverse, change back to the correct order
        gimbals_[i]->setTargetVal(gimbals_ctrl_msg->position[(i + 1 )%2 + (i/2) * 2 ]);
        target_angle_msg.angles.push_back(gimbals_[i]->getTargetVal());
      }

    gimbal_ctrl_pub_.publish(target_angle_msg);
  }

  bool JointInterface::gimbalsTorqueEnableCallback(dynamixel_controllers::TorqueEnable::Request &req, dynamixel_controllers::TorqueEnable::Response &res)
  {
    /* direct send torque control flag */
    hydrus::ServoConfigCmd torque_control_msg;
    torque_control_msg.command = req.torque_enable;
    gimbal_config_cmd_pub_.publish(torque_control_msg);

    return true;
  }

  void JointInterface::jointStatePublish()
  {
    sensor_msgs::JointState joints_state_msg;
    joints_state_msg.header.stamp = ros::Time::now();

    for(int i = 0; i < joint_num_; i ++)
      {
        joints_state_msg.name.push_back(joints_[i]->getName());
        joints_state_msg.position.push_back(joints_[i]->getCurrentVal());
      }

    for(int i = 0; i < gimbal_num_; i ++)
      {
        joints_state_msg.name.push_back(gimbals_[i]->getName());
        joints_state_msg.position.push_back(gimbals_[i]->getCurrentVal());
      }

    joints_state_pub_.publish(joints_state_msg);

    /* need to publish dynamixel msg */
    if(bridge_mode_ == MCU_MODE)
      {
        dynamixel_msgs::MotorStateList dynamixel_msg;
        for(auto it = joints_.begin(); it != joints_.end(); ++it)
          {
            dynamixel_msgs::MotorState motor_msg;
            motor_msg.timestamp = ros::Time::now().toSec();
            motor_msg.id = (*it)->getId();
            motor_msg.error = (*it)->getError();
            motor_msg.load = (*it)->getLoad();
            motor_msg.moving = (*it)->getMoving();
            motor_msg.temperature = (*it)->getTemp();
            dynamixel_msg.motor_states.push_back(motor_msg);
          }
        dynamixel_msg_pub_.publish(dynamixel_msg);
      }
  }

    void JointInterface::bridgeFunc(const ros::TimerEvent & e)
    {
      hydrus::JointInterface::bridgeFunc(e);

      if(send_init_joint_pose_cnt_ > 0) return;
      if(!start_gimbal_control_)
        {
          /* send control enable flag */
          hydrus::ServoConfigCmd control_msg;
          control_msg.command = CONTROL_ON;
          gimbal_config_cmd_pub_.publish(control_msg);

          start_gimbal_control_ = true;
        }
    }
};


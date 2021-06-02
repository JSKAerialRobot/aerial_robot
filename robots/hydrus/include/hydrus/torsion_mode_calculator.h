#ifndef TORSION_MODE_CALCULATOR
#define TORSION_MODE_CALCULATOR

#include <ros/ros.h>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include <kdl/chaindynparam.hpp>
#include <kdl/tree.hpp>
#include <kdl_parser/kdl_parser.hpp>
#include <urdf/model.h>

#include <hydrus/util/msg_utils.h>

#include <Eigen/Dense>
#include <algorithm>
#include <utility>
#include <sensor_msgs/JointState.h>
#include <std_msgs/Float32MultiArray.h>

#include <dynamic_reconfigure/server.h>
#include <hydrus/torsion_modeConfig.h>
#define RECONFITURE_TORSION_MODE_FLAG 0
#define TORSION_CONSTANT 1

class TorsionModeCalculator
{
  public:
    TorsionModeCalculator(ros::NodeHandle nh, ros::NodeHandle nhp);
    ~TorsionModeCalculator();

    void calculate();

  private:
    ros::NodeHandle nh_;
    ros::NodeHandle nhp_;

    tf2_ros::Buffer tfBuffer_;
    tf2_ros::TransformListener tfListener_;
    // ros subscribers
    ros::Subscriber joint_sub_;
    ros::Subscriber torsion_joint_sub_;
    // ros publishers
    ros::Publisher eigen_pub_;
    ros::Publisher mode_pub_;
    ros::Publisher K_mode_pub_;

    std::vector<unsigned int> torsion_dof_update_order_;
    std::vector<unsigned int> joint_dof_update_order_;

    KDL::ChainDynParam* dyn_param_;
    KDL::Chain kdl_chain_;
    KDL::JntArray jnt_q_;

    std::string robot_ns_;
    int rotor_num_;
    int torsion_num_;
    double torsion_constant_;
    int mode_num_;
    double eigen_eps_;

    std::vector<double> torsions_;
    std::vector<double> torsions_d_;
    void torsionJointCallback(const sensor_msgs::JointStateConstPtr& msg);

    std::vector<double> joints_;
    std::vector<double> joints_d_;
    void jointsCallback(const sensor_msgs::JointStateConstPtr& msg);

    dynamic_reconfigure::Server<hydrus::torsion_modeConfig>::CallbackType reconf_func_;
    dynamic_reconfigure::Server<hydrus::torsion_modeConfig>* reconf_server_;
    void cfgCallback(hydrus::torsion_modeConfig& config, uint32_t level);
};

#endif /* ifndef TORSION_MODE_CALCULATOR */

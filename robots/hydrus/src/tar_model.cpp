#include <hydrus/robot_model.h>

TARModel::TARModel()
{
  /* robot model */
  if (!model_.initParam("robot_description"))
    ROS_ERROR("Failed to extract urdf model from rosparam");
  if (!kdl_parser::treeFromUrdfModel(model_, tree_))
    ROS_ERROR("Failed to extract kdl tree from xml robot description");

  inertialSetup(tree_.getRootSegment()->second);
  resolveLinkLength();

  for(auto itr = model_.joints_.begin(); itr != model_.joints_.end(); itr++)
    {
      if(itr->first.find("joint1") != std::string::npos)
        {
          joint_angle_min_ = itr->second->limits->lower;
          joint_angle_max_ = itr->second->limits->upper;
          ROS_WARN("the angle range: [%f, %f]", joint_angle_min_, joint_angle_max_);
          break;
        }
    }

  //for(auto it: actuator_map_) ROS_ERROR("%d: %s", it.second, it.first.c_str());

  ROS_ERROR("[kinematics] rotor num; %d", rotor_num_);
  rotors_origin_from_cog_.resize(rotor_num_);
}

KDL::RigidBodyInertia TARModel::inertialSetup(const KDL::TreeElement tree_element)
{
  const KDL::Segment& current_seg = GetTreeElementSegment(tree_element);

  KDL::RigidBodyInertia current_seg_inertia = current_seg.getInertia();
  if(verbose_) ROS_INFO("segment %s, mass is: %f", current_seg.getName().c_str(), current_seg_inertia.getMass());

  /* check error joint */
  assert(model_.getJoint(current_seg.getJoint().getName())->type == urdf::Joint::FLOATING);

  /* check whether this can be a base inertia segment (i.e. link) */
  /* 1. for the "root" parent link (i.e. link1) */
  if(current_seg.getName().find("root") != std::string::npos)
    {
      assert(inertia_map_.size() == 0);
      assert(GetTreeElementChildren(tree_element).size() == 1);

      const KDL::Segment&  child_seg = GetTreeElementSegment(GetTreeElementChildren(tree_element).at(0)->second);
      inertia_map_.insert(std::make_pair(child_seg.getName(), child_seg.getInertia()));
      if(verbose_) ROS_WARN("Add root link: %s", child_seg.getName().c_str());

      /*
      std::cout << "m: " << inertia_map_.find(child_seg.getName())->second.getMass() << std::endl;
      std::cout << "p: \n" << Eigen::Map<const Eigen::Vector3d>(inertia_map_.find(child_seg.getName())->second.getCOG().data) << std::endl;
      std::cout << "I: \n" << Eigen::Map<const Eigen::Matrix3d>(inertia_map_.find(child_seg.getName())->second.getRotationalInertia().data) << std::endl;
      */
    }
  /* 2. for segment that has joint with parent segment */
  if (current_seg.getJoint().getType() != KDL::Joint::None)
    {
      /* add the new inertia base (child) link if the joint is not a rotor */
      if(current_seg.getJoint().getName().find("rotor") == std::string::npos)
        {
          /* create a new inertia base link */
          inertia_map_.insert(std::make_pair(current_seg.getName(), current_seg_inertia));
          actuator_map_.insert(std::make_pair(current_seg.getJoint().getName(), tree_element.q_nr));
          if(verbose_) ROS_WARN("Add new inertia base link: %s", current_seg.getName().c_str());
        }
    }
  /* special process for rotor */
  if(current_seg.getJoint().getName().find("rotor") != std::string::npos)
    {
      /* add the rotor direction */
      if(verbose_) ROS_WARN("%s, rototation is %f", current_seg.getJoint().getName().c_str(), current_seg.getJoint().JointAxis().z());
      rotor_direction_.insert(std::make_pair(std::atoi(current_seg.getJoint().getName().substr(5).c_str()), current_seg.getJoint().JointAxis().z()));
    }

  /* recursion process for children segment */
  //ROS_WARN("child num of current seg %s: %d", current_seg.getName().c_str(), GetTreeElementChildren(tree_element).size());
  for (auto itr: GetTreeElementChildren(tree_element))
    {
      const KDL::Segment& child_seg = GetTreeElementSegment(itr->second);
      KDL::RigidBodyInertia child_seg_inertia = child_seg.getFrameToTip() *  inertialSetup(itr->second);
      KDL::RigidBodyInertia current_seg_inertia_old = current_seg_inertia;
      current_seg_inertia = current_seg_inertia_old + child_seg_inertia;

      if(verbose_) ROS_WARN("Add new child segment %s to direct segment: %s", child_seg.getName().c_str(), current_seg.getName().c_str());
      /*
      std::cout << "child: seg, m: " << child_seg_inertia.getMass() << std::endl;
      std::cout << "current seg, m: " << current_seg_inertia.getMass() << std::endl;
      std::cout << "p: \n" << Eigen::Map<const Eigen::Vector3d>(current_seg_inertia.getCOG().data) << std::endl;
      std::cout << "I: \n" << Eigen::Map<const Eigen::Matrix3d>(current_seg_inertia.getRotationalInertia().data) << std::endl;
      */
    }

  /* count the rotor */
  if(current_seg.getName().find(thrust_link_.c_str()) != std::string::npos) rotor_num_++;

  /* update the inertia if the segment is base */
  if (inertia_map_.find(current_seg.getName()) != inertia_map_.end())
    {
      inertia_map_.at(current_seg.getName()) = current_seg_inertia;

      if(verbose_) ROS_WARN("Total mass of base segment %s is %f", current_seg.getName().c_str(),
                            inertia_map_.at(current_seg.getName()).getMass());
      current_seg_inertia = KDL::RigidBodyInertia::Zero();
    }

  return current_seg_inertia;
}

void TARModel::resolveLinkLength()
{
  unsigned int nj = tree_.getNrOfJoints();
  KDL::JntArray joint_positions(nj);
  KDL::TreeFkSolverPos_recursive fk_solver(tree_);
  KDL::Frame f_link2, f_link3;
  fk_solver.JntToCart(joint_positions, f_link2, "link2"); //hard coding //TODO
  fk_solver.JntToCart(joint_positions, f_link3, "link3"); //hard coding //TODO
  link_length_ = (f_link3.p - f_link2.p).Norm();
  //ROS_ERROR("Update link length: %f", link_length_);
}

void TARModel::forwardKinematics(sensor_msgs::JointState& state)
{
  KDL::TreeFkSolverPos_recursive fk_solver(tree_);
  KDL::JntArray joint_positions(tree_.getNrOfJoints());   /* set joint array */

  //unsigned int j = 0;
  for(unsigned int i = 0; i < state.position.size(); i++)
    {
      std::map<std::string, uint32_t>::iterator itr = actuator_map_.find(state.name[i]);

      if(itr != actuator_map_.end())  joint_positions(actuator_map_.find(state.name[i])->second) = state.position[i];
      //else ROS_FATAL("transform_control: no matching joint called %s", state.name[i].c_str());
    }

  KDL::RigidBodyInertia link_inertia = KDL::RigidBodyInertia::Zero();
  KDL::Frame cog_frame;
  for(auto it = inertia_map_.begin(); it != inertia_map_.end(); ++it)
    {
      KDL::Frame f;
      int status = fk_solver.JntToCart(joint_positions, f, it->first);
      //ROS_ERROR(" %s status is : %d, [%f, %f, %f]", it->first.c_str(), status, f.p.x(), f.p.y(), f.p.z());
      KDL::RigidBodyInertia link_inertia_tmp = link_inertia;
      link_inertia = link_inertia_tmp + f * it->second;

      /* process for the extra module */
      for(std::map<std::string, KDL::Segment>::iterator it_extra = extra_module_map_.begin(); it_extra != extra_module_map_.end(); it_extra++)
        {
          if(it_extra->second.getName() == it->first)
            {
              //ROS_INFO("[kinematics]: find the extra module %s", it_extra->second.getName().c_str());
              KDL::RigidBodyInertia link_inertia_tmp = link_inertia;
              link_inertia = link_inertia_tmp + f *  (it_extra->second.getFrameToTip() * it_extra->second.getInertia());
            }
        }

    }
  /* CoG */
  KDL::Frame f_baselink;
  int status = fk_solver.JntToCart(joint_positions, f_baselink, baselink_);
  if(status < 0) ROS_ERROR("can not get FK to the baselink: %s", baselink_.c_str());
  cog_frame.M = f_baselink.M * cog_desire_orientation_.Inverse();
  cog_frame.p = link_inertia.getCOG();
  tf::Transform cog_transform;
  tf::transformKDLToTF(cog_frame, cog_transform);
  setCog(cog_transform);
  setMass(link_inertia.getMass());

  /* thrust point based on COG */
  std::vector<Eigen::Vector3d> f_rotors;
  for(int i = 0; i < rotor_num_; i++)
    {
      std::stringstream ss;
      ss << i + 1;
      std::string rotor = thrust_link_ + ss.str();

      KDL::Frame f;
      int status = fk_solver.JntToCart(joint_positions, f, rotor);
      //if(verbose) ROS_WARN(" %s status is : %d, [%f, %f, %f]", rotor.c_str(), status, f.p.x(), f.p.y(), f.p.z());
      f_rotors.push_back(Eigen::Map<const Eigen::Vector3d>((cog_frame.Inverse() * f).p.data));

      //std::cout << "rotor" << i + 1 << ": \n"<< f_rotors[i] << std::endl;
    }

  setRotorsOriginFromCog(f_rotors);
  KDL::RigidBodyInertia link_inertia_from_cog = cog_frame.Inverse() * link_inertia;
  setInertia(Eigen::Map<const Eigen::Matrix3d>(link_inertia_from_cog.getRotationalInertia().data));

  tf::Transform cog2baselink_transform;
  tf::transformKDLToTF(cog_frame.Inverse() * f_baselink, cog2baselink_transform);
  setCog2Baselink(cog2baselink_transform);
}

bool TransformController::addExtraModule(int action, std::string module_name, std::string parent_link_name, geometry_msgs::Transform transform, geometry_msgs::Inertia inertia)
{
  switch(action)
    {
    case hydrus::AddExtraModule::Request::ADD:
      {
        std::map<std::string, KDL::Segment>::iterator it = extra_module_map_.find(module_name);
        if(it == extra_module_map_.end())
          {
            if(inertia_map_.find(parent_link_name) == inertia_map_.end())
              {
                ROS_WARN("[extra module]: fail to add new extra module %s, becuase it's parent link (%s) is invalid", module_name.c_str(), parent_link_name.c_str());
                return false;
              }


            if(fabs(1 - tf::Quaternion(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w).length2()) > 1e-6)
              {
                ROS_WARN("[extra module]: fail to add new extra module %s, becuase the orientation is invalid", module_name.c_str());
                return false;
              }

            if(inertia.m <= 0)
              {
                ROS_WARN("[extra module]: fail to add new extra module %s, becuase the mass %f is invalid", module_name.c_str(), inertia.m);
                return false;
              }

            KDL::Frame f;
            tf::transformMsgToKDL(transform, f);
            KDL::RigidBodyInertia rigid_body_inertia(inertia.m, KDL::Vector(inertia.com.x, inertia.com.y, inertia.com.z),
                                                     KDL::RotationalInertia(inertia.ixx, inertia.iyy,
                                                                            inertia.izz, inertia.ixy,
                                                                            inertia.ixz, inertia.iyz));
            KDL::Segment extra_module(parent_link_name, KDL::Joint(KDL::Joint::None), f, rigid_body_inertia);
            extra_module_map_.insert(std::make_pair(module_name, extra_module));
            ROS_INFO("[extra module]: succeed to add new extra module %s", module_name.c_str());
            return true;
          }
        else
          {
            ROS_WARN("[extra module]: fail to add new extra module %s, becuase it already exists", module_name.c_str());
            return false;
          }
        break;
      }
    case hydrus::AddExtraModule::Request::REMOVE:
      {
        std::map<std::string, KDL::Segment>::iterator it = extra_module_map_.find(module_name);
        if(it == extra_module_map_.end())
          {
            ROS_WARN("[extra module]: fail to remove the extra module %s, becuase it does not exists", module_name.c_str());
            return false;
          }
        else
          {
            extra_module_map_.erase(module_name);
            ROS_INFO("[extra module]: suscced to remove the extra module %s", module_name.c_str());
            return true;
          }
        break;
      }
    default:
      {
        ROS_WARN("[extra module]: wrong action %d", action);
        return false;
        break;
      }
    }
  ROS_ERROR("[extra module]: should not reach here ");
  return false;
}

tf::Transform TARModel::getRoot2Link(std::string link, sensor_msgs::JointState state)
{
  KDL::TreeFkSolverPos_recursive fk_solver(tree_);
  unsigned int nj = tree_.getNrOfJoints();
  KDL::JntArray joint_positions(nj);

  unsigned int j = 0;
  for(unsigned int i = 0; i < state.position.size(); i++)
    {
      std::map<std::string, uint32_t>::iterator itr = actuator_map_.find(state.name[i]);
      if(itr != actuator_map_.end())  joint_positions(actuator_map_.find(state.name[i])->second) = state.position[i];
    }

  KDL::Frame f;
  tf::Transform  link_f;
  int status = fk_solver.JntToCart(joint_positions, f, link);
  tf::transformKDLToTF(f, link_f);

  return link_f;
}

void TransformController::setActuatorJointMap(const sensor_msgs::JointState& actuator_state)
{
  /* CAUTION: be sure that the joints are in order !!!!!!! */
  for(auto itr = actuator_state.name.begin(); itr != actuator_state.name.end(); ++itr)
    {
      if(itr->find("joint") != std::string::npos)
        actuator_joint_map_.push_back(std::distance(actuator_state.name.begin(), itr));
    }
}

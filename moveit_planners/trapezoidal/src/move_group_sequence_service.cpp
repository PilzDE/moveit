/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2012, Willow Garage, Inc.
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
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
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

/* Author: Ioan Sucan */

// Modified by Pilz GmbH & Co. KG

#include "trapezoidal_trajectory_generation/move_group_sequence_service.h"

#include "trapezoidal_trajectory_generation/capability_names.h"
#include "trapezoidal_trajectory_generation/command_list_manager.h"
#include "trapezoidal_trajectory_generation/trajectory_generation_exceptions.h"

namespace trapezoidal_trajectory_generation
{
MoveGroupSequenceService::MoveGroupSequenceService() : MoveGroupCapability("SequenceService")
{
}

MoveGroupSequenceService::~MoveGroupSequenceService()
{
}

void MoveGroupSequenceService::initialize()
{
  command_list_manager_.reset(new trapezoidal_trajectory_generation::CommandListManager(
      ros::NodeHandle("~"), context_->planning_scene_monitor_->getRobotModel()));

  sequence_service_ = root_node_handle_.advertiseService(SEQUENCE_SERVICE_NAME, &MoveGroupSequenceService::plan, this);
}

bool MoveGroupSequenceService::plan(moveit_msgs::GetMotionSequence::Request& req,
                                    moveit_msgs::GetMotionSequence::Response& res)
{
  // TODO: Do we lock on the correct scene? Does the lock belong to the scene used for planning?
  planning_scene_monitor::LockedPlanningSceneRO ps(context_->planning_scene_monitor_);

  ros::Time planning_start = ros::Time::now();
  RobotTrajCont traj_vec;
  try
  {
    traj_vec = command_list_manager_->solve(ps, context_->planning_pipeline_, req.commands);
  }
  catch (const MoveItErrorCodeException& ex)
  {
    ROS_ERROR_STREAM("Planner threw an exception (error code: " << ex.getErrorCode() << "): " << ex.what());
    res.error_code.val = ex.getErrorCode();
    return true;
  }
  // LCOV_EXCL_START // Keep moveit up even if lower parts throw
  catch (const std::exception& ex)
  {
    ROS_ERROR_STREAM("Planner threw an exception: " << ex.what());
    // If 'FALSE' then no response will be sent to the caller.
    return false;
  }
  // LCOV_EXCL_STOP

  res.trajectory_start.resize(traj_vec.size());
  res.planned_trajectory.resize(traj_vec.size());
  for (RobotTrajCont::size_type i = 0; i < traj_vec.size(); ++i)
  {
    move_group::MoveGroupCapability::convertToMsg(traj_vec.at(i), res.trajectory_start.at(i),
                                                  res.planned_trajectory.at(i));
  }
  res.error_code.val = moveit_msgs::MoveItErrorCodes::SUCCESS;
  res.planning_time = (ros::Time::now() - planning_start).toSec();
  return true;
}

}  // namespace trapezoidal_trajectory_generation

#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(trapezoidal_trajectory_generation::MoveGroupSequenceService, move_group::MoveGroupCapability)

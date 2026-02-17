// Copyright 2026, Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gmock/gmock.h>

#include <filesystem>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "composition_manager_test_fixture.hpp"
#include "rosbag2_test_common/tested_storage_ids.hpp"

using namespace std::chrono_literals;  // NOLINT

class ComposableSmokeTests : public CompositionManagerTestFixture
{
};

TEST_P(ComposableSmokeTests, player_component_is_loadable)
{
  auto request = std::make_shared<composition_interfaces::srv::LoadNode::Request>();
  request->package_name = "rosbag2_transport";
  request->plugin_name = "rosbag2_transport::Player";

  const auto input_bag = (std::filesystem::path(_SRC_RESOURCES_DIR_PATH) /
    GetParam() / "test_bag_for_seek").generic_string();
  request->parameters.emplace_back(
    rclcpp::Parameter("storage.uri", rclcpp::ParameterValue(input_bag)).to_parameter_msg());
  request->parameters.emplace_back(
    rclcpp::Parameter("storage.storage_id", rclcpp::ParameterValue(GetParam())).to_parameter_msg());
  request->parameters.emplace_back(
    rclcpp::Parameter("play.start_paused", rclcpp::ParameterValue(true)).to_parameter_msg());

  auto future = load_node_client_->async_send_request(request);
  auto ret = exec_->spin_until_future_complete(future, 10s);
  ASSERT_EQ(ret, rclcpp::FutureReturnCode::SUCCESS);

  auto result = future.get();
  ASSERT_TRUE(result->success) << result->error_message;
  EXPECT_EQ(result->full_node_name, "/rosbag2_player");
  EXPECT_GT(result->unique_id, 0u);

  unload_node(result->unique_id);
}

TEST_P(ComposableSmokeTests, recorder_component_is_loadable)
{
  auto request = std::make_shared<composition_interfaces::srv::LoadNode::Request>();
  request->package_name = "rosbag2_transport";
  request->plugin_name = "rosbag2_transport::Recorder";

  request->parameters.emplace_back(
    rclcpp::Parameter(
      "storage.uri", rclcpp::ParameterValue(root_bag_path_.generic_string())).to_parameter_msg());
  request->parameters.emplace_back(
    rclcpp::Parameter("storage.storage_id", rclcpp::ParameterValue(GetParam())).to_parameter_msg());
  request->parameters.emplace_back(
    rclcpp::Parameter("record.all", rclcpp::ParameterValue(false)).to_parameter_msg());
  request->parameters.emplace_back(
    rclcpp::Parameter(
      "record.topics", rclcpp::ParameterValue(std::vector<std::string>{"/smoke/topic"})).
    to_parameter_msg());
  request->parameters.emplace_back(
    rclcpp::Parameter("record.is_discovery_disabled", rclcpp::ParameterValue(true)).
    to_parameter_msg());

  auto future = load_node_client_->async_send_request(request);
  auto ret = exec_->spin_until_future_complete(future, 10s);
  ASSERT_EQ(ret, rclcpp::FutureReturnCode::SUCCESS);

  auto result = future.get();
  ASSERT_TRUE(result->success) << result->error_message;
  EXPECT_EQ(result->full_node_name, "/rosbag2_recorder");
  EXPECT_GT(result->unique_id, 0u);

  unload_node(result->unique_id);
}

INSTANTIATE_TEST_SUITE_P(
  ParametrizedComposableSmokeTests,
  ComposableSmokeTests,
  ValuesIn(rosbag2_test_common::kTestedStorageIDs)
);

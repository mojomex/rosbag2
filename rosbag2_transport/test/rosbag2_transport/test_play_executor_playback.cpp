// Copyright 2026 TIER IV, Inc.
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

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rmw/rmw.h"
#include "rosbag2_cpp/writers/sequential_writer.hpp"
#include "rosbag2_storage/storage_options.hpp"
#include "rosbag2_storage/topic_metadata.hpp"
#include "rosbag2_test_common/memory_management.hpp"
#include "rosbag2_test_common/temporary_directory_fixture.hpp"
#include "rosbag2_transport/play_options.hpp"
#include "rosbag2_transport/player.hpp"
#include "test_msgs/message_fixtures.hpp"
#include "test_msgs/msg/basic_types.hpp"

using namespace std::chrono_literals;  // NOLINT

class PlayExecutorPlaybackTest : public rosbag2_test_common::TemporaryDirectoryFixture
{
public:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);
  }

  void TearDown() override
  {
    rclcpp::shutdown();
  }

protected:
  rosbag2_storage::StorageOptions create_handcrafted_bag()
  {
    const std::string bag_uri = temporary_dir_path_ + "/executor_playback_test_bag";
    rosbag2_storage::StorageOptions storage_options{bag_uri, "sqlite3", 0, 0, 0};

    rosbag2_cpp::writers::SequentialWriter writer;
    const rosbag2_cpp::ConverterOptions converter_options{
      rmw_get_serialization_format(), rmw_get_serialization_format()};

    writer.open(storage_options, converter_options);
    writer.create_topic(
      {"topic1", "test_msgs/BasicTypes", rmw_get_serialization_format(), ""});

    auto msg_1 = get_messages_basic_types()[0];
    msg_1->int32_value = 1;
    auto msg_2 = get_messages_basic_types()[0];
    msg_2->int32_value = 2;

    writer.write(serialize_message("topic1", 0, msg_1));
    writer.write(serialize_message("topic1", 1'000'000, msg_2));
    writer.close();

    return storage_options;
  }

  template<typename MessageT>
  std::shared_ptr<rosbag2_storage::SerializedBagMessage> serialize_message(
    const std::string & topic_name,
    const rcutils_time_point_value_t timestamp,
    const std::shared_ptr<MessageT> & message)
  {
    auto serialized = std::make_shared<rosbag2_storage::SerializedBagMessage>();
    serialized->serialized_data = memory_management_.serialize_message(message);
    serialized->topic_name = topic_name;
    serialized->time_stamp = timestamp;
    return serialized;
  }

  rosbag2_test_common::MemoryManagement memory_management_;
};

TEST_F(PlayExecutorPlaybackTest, executor_playback_requires_executor_spinning)
{
  auto storage_options = create_handcrafted_bag();

  rosbag2_transport::PlayOptions play_options;
  play_options.executor_playback = true;

  auto player = std::make_shared<rosbag2_transport::Player>(storage_options, play_options);

  std::vector<int32_t> received_values;
  auto listener = std::make_shared<rclcpp::Node>("executor_playback_listener");
  auto sub = listener->create_subscription<test_msgs::msg::BasicTypes>(
    "topic1", rclcpp::QoS(10),
    [&received_values](const test_msgs::msg::BasicTypes::SharedPtr msg) {
      received_values.push_back(msg->int32_value);
    });
  (void)sub;

  ASSERT_TRUE(player->play());
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(player);
  executor.add_node(listener);

  executor.spin_some(20ms);
  EXPECT_TRUE(received_values.empty());
  EXPECT_FALSE(player->wait_for_playback_to_finish(20ms));

  auto stepper = std::dynamic_pointer_cast<rslcpp::ExecutorIdleStepper>(player);
  ASSERT_NE(stepper, nullptr);

  rclcpp::Time sim_time(0, 0, RCL_ROS_TIME);
  ASSERT_TRUE(stepper->step_on_executor_idle(sim_time));
  EXPECT_EQ(sim_time.nanoseconds(), 0);
  executor.spin_some(20ms);
  ASSERT_EQ(received_values.size(), 1u);
  EXPECT_EQ(received_values[0], 1);
  EXPECT_FALSE(player->wait_for_playback_to_finish(20ms));

  ASSERT_TRUE(stepper->step_on_executor_idle(sim_time));
  EXPECT_EQ(sim_time.nanoseconds(), 1'000'000);
  executor.spin_some(20ms);
  ASSERT_EQ(received_values.size(), 2u);
  EXPECT_EQ(received_values[1], 2);
  EXPECT_FALSE(player->wait_for_playback_to_finish(20ms));

  EXPECT_FALSE(stepper->step_on_executor_idle(sim_time));

  EXPECT_TRUE(player->wait_for_playback_to_finish(0s));
  executor.remove_node(listener);
  executor.remove_node(player);
}

// Copyright 2023 Patrick Roncagliolo. All Rights Reserved.
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

#include <map>
#include <string>
#include <vector>

#include "rclcpp/duration.hpp"
#include "rosbag2_transport/play_options.hpp"
#include "rosbag2_transport/qos.hpp"

namespace YAML
{

template<>
struct convert<rclcpp::Duration>
{
  static Node encode(const rclcpp::Duration & duration)
  {
    constexpr int64_t kNanosecondsPerSecond = 1000000000LL;
    Node node;
    node["sec"] = duration.nanoseconds() / kNanosecondsPerSecond;
    node["nsec"] = duration.nanoseconds() % kNanosecondsPerSecond;
    return node;
  }

  static bool decode(const Node & node, rclcpp::Duration & duration)
  {
    if (!node.IsMap() || !node["sec"] || !node["nsec"]) {
      return false;
    }
    constexpr int64_t kNanosecondsPerSecond = 1000000000LL;
    const int64_t sec = node["sec"].as<int64_t>();
    const int64_t nsec = node["nsec"].as<int64_t>();
    duration = rclcpp::Duration::from_nanoseconds(sec * kNanosecondsPerSecond + nsec);
    return true;
  }
};

template<>
void optional_assign<rclcpp::Duration>(
  const Node & node, std::string field, rclcpp::Duration & assign_to)
{
  if (!node[field]) {
    return;
  }
  const auto duration_node = node[field];
  if (!duration_node.IsMap() || !duration_node["sec"] || !duration_node["nsec"]) {
    return;
  }
  constexpr int64_t kNanosecondsPerSecond = 1000000000LL;
  const int64_t sec = duration_node["sec"].as<int64_t>();
  const int64_t nsec = duration_node["nsec"].as<int64_t>();
  assign_to = rclcpp::Duration::from_nanoseconds(sec * kNanosecondsPerSecond + nsec);
}

Node convert<rosbag2_transport::PlayOptions>::encode(
  const rosbag2_transport::PlayOptions & play_options)
{
  Node node;
  node["read_ahead_queue_size"] = play_options.read_ahead_queue_size;
  node["node_prefix"] = play_options.node_prefix;
  node["rate"] = play_options.rate;
  node["topics_to_filter"] = play_options.topics_to_filter;
  node["topics_regex_to_filter"] = play_options.topics_regex_to_filter;
  node["topics_regex_to_exclude"] = play_options.topics_regex_to_exclude;
  std::map<std::string, rosbag2_transport::Rosbag2QoS> qos_overrides(
    play_options.topic_qos_profile_overrides.begin(),
    play_options.topic_qos_profile_overrides.end());
  node["topic_qos_profile_overrides"] = qos_overrides;
  node["loop"] = play_options.loop;
  node["topic_remapping_options"] = play_options.topic_remapping_options;
  node["clock_publish_frequency"] = play_options.clock_publish_frequency;
  node["clock_publish_on_topic_publish"] = play_options.clock_publish_on_topic_publish;
  node["clock_trigger_topics"] = play_options.clock_trigger_topics;
  node["delay"] = play_options.delay;
  node["playback_duration"] = play_options.playback_duration;
  node["playback_until_timestamp"] = YAML::convert<rclcpp::Duration>::encode(
    std::chrono::nanoseconds(play_options.playback_until_timestamp));

  node["start_paused"] = play_options.start_paused;
  node["start_offset"] = YAML::convert<rclcpp::Duration>::encode(
    std::chrono::nanoseconds(play_options.start_offset));

  node["disable_keyboard_controls"] = play_options.disable_keyboard_controls;
  node["wait_acked_timeout"] = YAML::convert<rclcpp::Duration>::encode(
    std::chrono::nanoseconds(play_options.wait_acked_timeout));

  node["disable_loan_message"] = play_options.disable_loan_message;

  return node;
}

bool convert<rosbag2_transport::PlayOptions>::decode(
  const Node & node, rosbag2_transport::PlayOptions & play_options)
{
  optional_assign<uint64_t>(node, "read_ahead_queue_size", play_options.read_ahead_queue_size);
  optional_assign<std::string>(node, "node_prefix", play_options.node_prefix);
  optional_assign<float>(node, "rate", play_options.rate);
  optional_assign<std::vector<std::string>>(
    node, "topics_to_filter", play_options.topics_to_filter);
  optional_assign<std::string>(node, "topics_regex_to_filter", play_options.topics_regex_to_filter);
  optional_assign<std::string>(
    node, "topics_regex_to_exclude", play_options.topics_regex_to_exclude);

  // yaml-cpp doesn't implement unordered_map
  std::map<std::string, rosbag2_transport::Rosbag2QoS> qos_overrides;
  optional_assign<std::map<std::string, rosbag2_transport::Rosbag2QoS>>(
    node, "topic_qos_profile_overrides", qos_overrides);
  play_options.topic_qos_profile_overrides.insert(qos_overrides.begin(), qos_overrides.end());

  optional_assign<double>(node, "clock_publish_frequency", play_options.clock_publish_frequency);

  optional_assign<bool>(
    node, "clock_publish_on_topic_publish", play_options.clock_publish_on_topic_publish);

  optional_assign<std::vector<std::string>>(
    node, "clock_trigger_topics", play_options.clock_trigger_topics);

  optional_assign<rclcpp::Duration>(node, "delay", play_options.delay);
  optional_assign<rclcpp::Duration>(node, "playback_duration", play_options.playback_duration);

  rclcpp::Duration playback_until_timestamp(
    std::chrono::nanoseconds(play_options.playback_until_timestamp));
  optional_assign<rclcpp::Duration>(node, "playback_until_timestamp", playback_until_timestamp);
  play_options.playback_until_timestamp = playback_until_timestamp.nanoseconds();

  optional_assign<bool>(node, "start_paused", play_options.start_paused);

  rclcpp::Duration start_offset(std::chrono::nanoseconds(play_options.start_offset));
  optional_assign<rclcpp::Duration>(node, "start_offset", start_offset);
  play_options.start_offset = start_offset.nanoseconds();

  optional_assign<bool>(node, "disable_keyboard_controls", play_options.disable_keyboard_controls);

  rclcpp::Duration wait_acked_timeout(std::chrono::nanoseconds(play_options.wait_acked_timeout));
  optional_assign<rclcpp::Duration>(node, "wait_acked_timeout", wait_acked_timeout);
  play_options.wait_acked_timeout = wait_acked_timeout.nanoseconds();

  optional_assign<bool>(node, "disable_loan_message", play_options.disable_loan_message);

  return true;
}

}  // namespace YAML

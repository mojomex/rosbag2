FROM ros:humble

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    python3-colcon-common-extensions \
    ros-humble-test-msgs \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /rosbag2
COPY . .

RUN set -eo pipefail && \
    source /opt/ros/humble/setup.bash && \
    rm -rf build install log && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release && \
    colcon test --packages-skip ros2bag rosbag2_tests rosbag2_transport --event-handlers console_direct+ --return-code-on-test-failure && \
    colcon test-result --verbose

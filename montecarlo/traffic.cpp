// Nagel-Schreckenberg traffic model simulation.
// As described in Art Owen's "Monte Carlo theory, methods and examples"
// https://artowen.su.domains/mc/

#include <cmath>
#include <cstddef>
#include <numbers>

#include "absl/random/random.h"
#include "polyscope/point_cloud.h"
#include "polyscope/polyscope.h"

struct Vehicle {
  int position;
  int velocity;

  int future_position;
  int future_velocity;
};

struct TrafficCircle {
  void update() {
    for (size_t i = 0; i < vehicles.size(); ++i) {
      auto& vehicle = vehicles[i];
      const auto& next_vehicle =
          i == vehicles.size() - 1 ? vehicles[0] : vehicles[i + 1];

      // Increase speed by 1, because all drivers are eager to move ahead.
      vehicle.future_velocity = std::min(vehicle.velocity + 1, max_velocity);

      // Reduce velocity in order to avoid collision.
      int d = (next_vehicle.position - vehicle.position - 1 + num_zones) %
              num_zones;
      vehicle.future_velocity = std::min(vehicle.future_velocity, d - 1);

      // With probability p, slow down:
      const float p = 0.45f;
      float randuniform = absl::Uniform(gen, 0, 1.0);
      if (randuniform < p) {
        vehicle.future_velocity = std::max(0, vehicle.future_velocity - 1);
      }

      vehicle.future_position =
          (vehicle.position + vehicle.future_velocity) % num_zones;
    }

    // Second loop: Atomically apply all the calculated future states
    for (auto& vehicle : vehicles) {
      vehicle.position = vehicle.future_position;
      vehicle.velocity = vehicle.future_velocity;
    }
  }

  int num_zones;
  int max_velocity;
  std::vector<Vehicle> vehicles;
  absl::BitGen gen;
};

TrafficCircle initTrafficCircle() {
  TrafficCircle circle;
  circle.num_zones = 1000;
  circle.max_velocity = 20;

  circle.vehicles.resize(25);
  int next_starting_pos = 0;
  for (auto& vehicle : circle.vehicles) {
    vehicle.position = next_starting_pos;
    next_starting_pos += 10;
    vehicle.velocity = 2;
  }
  return circle;
}

void myCallback() {
  // <<< CHANGE: Add time-checking logic to slow down the simulation
  static auto last_update_time = std::chrono::steady_clock::now();
  const auto time_step = std::chrono::milliseconds(50);

  static auto circle = initTrafficCircle();

  auto now = std::chrono::steady_clock::now();
  if (now - last_update_time > time_step) {
    circle.update();
    last_update_time = now;
  }

  // Visualization always runs, ensuring a responsive UI
  std::vector<glm::vec2> points;
  points.reserve(circle.vehicles.size());

  for (const auto& vehicle : circle.vehicles) {
    float theta = (2.0f * std::numbers::pi_v<float> * vehicle.position) /
                  circle.num_zones;
    points.push_back(glm::vec2{std::cos(theta), std::sin(theta)});
  }

  polyscope::getPointCloud("flat points")->updatePointPositions2D(points);
}

int main() {
  polyscope::init();

  // Set the camera to 2D mode
  polyscope::view::style = polyscope::view::NavigateStyle::Planar;

  // === FIX IS HERE ===
  // 1. Create the initial state of the simulation.
  auto circle = initTrafficCircle();

  // 2. Generate the initial positions from that state.
  std::vector<glm::vec2> initial_points;
  initial_points.reserve(circle.vehicles.size());
  for (const auto& vehicle : circle.vehicles) {
    float theta = (2.0f * std::numbers::pi_v<float> * vehicle.position) /
                  circle.num_zones;
    initial_points.push_back(glm::vec2{std::cos(theta), std::sin(theta)});
  }

  // 3. Register the point cloud with the correct number of initial points.
  polyscope::registerPointCloud2D("flat points", initial_points);

  // Set the callback that will run each frame
  polyscope::state::userCallback = myCallback;

  polyscope::show();

  return 0;
}
#include "trajectorysolver.h"

#include <cmath>

namespace pose {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRadToDeg = 180.0f / kPi;
} // namespace

PathType TrajectorySolver::parsePathType(const std::string& name) {
    if (name == "line") return PathType::Line;
    return PathType::Circle;
}

TrajectoryPoint TrajectorySolver::evaluate(const PathConfig& config, float distanceTraveled) {
    TrajectoryPoint pt;

    if (config.type == PathType::Circle) {
        float r = (config.radius > 0.05f) ? config.radius : 3.0f;
        float theta = distanceTraveled / r; // angle in radians

        // Position: X = Cx + R * cos(theta), Z = Cz + R * sin(theta)
        pt.position.x = config.center.x + r * std::cos(theta);
        pt.position.y = config.center.y;
        pt.position.z = config.center.z + r * std::sin(theta);

        // Heading tangent to circle: character faces along velocity vector (-sin(theta), 0, cos(theta))
        // Since forward at 0 yaw is +Z (0, 0, 1), yaw = -theta
        float yawRad = -theta;
        pt.yawDegrees = std::fmod(yawRad * kRadToDeg, 360.0f);
    } else {
        // Linear path along +Z
        pt.position.x = config.center.x;
        pt.position.y = config.center.y;
        pt.position.z = config.center.z + distanceTraveled;
        pt.yawDegrees = 0.0f;
    }

    return pt;
}

} // namespace pose

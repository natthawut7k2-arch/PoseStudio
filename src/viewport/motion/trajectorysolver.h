#ifndef TRAJECTORY_SOLVER_H
#define TRAJECTORY_SOLVER_H

#include <glm/glm.hpp>
#include <string>

namespace pose {

enum class PathType {
    Circle,
    Line
};

struct PathConfig {
    PathType  type = PathType::Circle;
    float     radius = 3.0f;
    glm::vec3 center = glm::vec3(0.0f);
    bool      loop = true;
};

struct TrajectoryPoint {
    glm::vec3 position = glm::vec3(0.0f);
    float     yawDegrees = 0.0f;
};

class TrajectorySolver {
public:
    static PathType parsePathType(const std::string& name);

    /// Evaluates the character's position and tangent yaw for the given path and cumulative distance.
    static TrajectoryPoint evaluate(const PathConfig& config, float distanceTraveled);
};

} // namespace pose

#endif // TRAJECTORY_SOLVER_H

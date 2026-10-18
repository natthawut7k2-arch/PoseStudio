#ifndef PROCEDURAL_GAIT_H
#define PROCEDURAL_GAIT_H

#include <glm/glm.hpp>
#include <string>
#include <utility>
#include <vector>

namespace pose {

enum class GaitType {
    Idle,
    Walk,
    Run
};

struct GaitFrame {
    std::vector<std::pair<std::string, glm::vec3>> boneRotations;
    float verticalBounce = 0.0f;
};

class ProceduralGait {
public:
    static GaitType parseGaitType(const std::string& name);
    static float defaultFrequency(GaitType type);
    static float defaultSpeed(GaitType type);

    /// Evaluates a single gait cycle at the given phase in radians [0, 2*pi).
    static GaitFrame evaluate(GaitType type, float phase);

    /// Evaluates a smooth blend between two gait cycles with blend weight w in [0, 1].
    static GaitFrame evaluateBlend(GaitType typeA, GaitType typeB, float phaseA, float phaseB, float weight);

private:
    static GaitFrame evaluateIdle(float phase);
    static GaitFrame evaluateWalk(float phase);
    static GaitFrame evaluateRun(float phase);
};

} // namespace pose

#endif // PROCEDURAL_GAIT_H

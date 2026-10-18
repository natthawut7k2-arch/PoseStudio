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

struct GaitParams {
    float forwardLean = -1.0f; // degrees forward pitch
    float armSwing = -1.0f;    // shoulder pitch amplitude
    float elbowBend = -1.0f;   // elbow resting / base bend
    float kneeBend = -1.0f;    // swing phase knee flexion
    float kneeStance = -1.0f;  // stance phase knee shock absorption
    float torsoTwist = -1.0f;  // chest yaw counter-twist
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
    static GaitFrame evaluate(GaitType type, float phase, const GaitParams& params = {});

    /// Evaluates a smooth blend between two gait cycles with blend weight w in [0, 1].
    static GaitFrame evaluateBlend(GaitType typeA, GaitType typeB,
                                   float phaseA, float phaseB,
                                   float weight,
                                   const GaitParams& paramsA = {},
                                   const GaitParams& paramsB = {});

private:
    static GaitFrame evaluateIdle(float phase, const GaitParams& params);
    static GaitFrame evaluateWalk(float phase, const GaitParams& params);
    static GaitFrame evaluateRun(float phase, const GaitParams& params);
};

} // namespace pose

#endif // PROCEDURAL_GAIT_H

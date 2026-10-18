#include "proceduralgait.h"

#include <algorithm>
#include <cmath>

namespace pose {

namespace {
constexpr float kPi = 3.14159265358979323846f;

float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
} // namespace

GaitType ProceduralGait::parseGaitType(const std::string& name) {
    if (name == "run") return GaitType::Run;
    if (name == "walk") return GaitType::Walk;
    return GaitType::Idle;
}

float ProceduralGait::defaultFrequency(GaitType type) {
    switch (type) {
        case GaitType::Run:  return 3.0f; // 3 strides/sec
        case GaitType::Walk: return 1.8f; // 1.8 strides/sec
        case GaitType::Idle: return 0.5f; // breathing/shift
    }
    return 1.0f;
}

float ProceduralGait::defaultSpeed(GaitType type) {
    switch (type) {
        case GaitType::Run:  return 2.5f; // m/s
        case GaitType::Walk: return 1.2f; // m/s
        case GaitType::Idle: return 0.0f;
    }
    return 0.0f;
}

GaitFrame ProceduralGait::evaluateIdle(float phase) {
    GaitFrame frame;
    // Gentle breathing and subtle weight shift
    float breath = std::sin(phase) * 2.0f;
    float sway = std::sin(phase * 0.5f) * 1.5f;

    frame.boneRotations = {
        {"chest",    glm::vec3(breath, 0.0f, 0.0f)},
        {"hip",      glm::vec3(0.0f, 0.0f, sway)},
        {"lShldr",   glm::vec3(0.0f, 0.0f, -5.0f)},
        {"rShldr",   glm::vec3(0.0f, 0.0f, 5.0f)},
        {"lForeArm", glm::vec3(10.0f, 0.0f, 0.0f)},
        {"rForeArm", glm::vec3(10.0f, 0.0f, 0.0f)}
    };
    frame.verticalBounce = std::sin(phase) * 0.005f;
    return frame;
}

GaitFrame ProceduralGait::evaluateWalk(float phase) {
    GaitFrame frame;
    // Leg pitch
    float lThighPitch = 25.0f * std::sin(phase);
    float rThighPitch = -lThighPitch;

    // Knee flexion (bends backwards when leg swings through)
    float lKnee = 35.0f * std::max(0.0f, std::cos(phase));
    float rKnee = 35.0f * std::max(0.0f, -std::cos(phase));

    // Ankle compensation
    float lFoot = -10.0f * std::sin(phase);
    float rFoot = -lFoot;

    // Arm counterswing (opposite to legs)
    float lArmPitch = -20.0f * std::sin(phase);
    float rArmPitch = -lArmPitch;
    float lForearm = 15.0f * (1.0f - std::cos(phase));
    float rForearm = 15.0f * (1.0f + std::cos(phase));

    // Pelvis tilt and torso sway
    float pelvisRoll = 2.5f * std::sin(phase);
    float chestTwist = -3.0f * std::sin(phase);

    frame.boneRotations = {
        {"lThigh",   glm::vec3(lThighPitch, 0.0f, 0.0f)},
        {"rThigh",   glm::vec3(rThighPitch, 0.0f, 0.0f)},
        {"lShin",    glm::vec3(lKnee, 0.0f, 0.0f)},
        {"rShin",    glm::vec3(rKnee, 0.0f, 0.0f)},
        {"lFoot",    glm::vec3(lFoot, 0.0f, 0.0f)},
        {"rFoot",    glm::vec3(rFoot, 0.0f, 0.0f)},
        {"lShldr",   glm::vec3(lArmPitch, 0.0f, -5.0f)},
        {"rShldr",   glm::vec3(rArmPitch, 0.0f, 5.0f)},
        {"lForeArm", glm::vec3(lForearm, 0.0f, 0.0f)},
        {"rForeArm", glm::vec3(rForearm, 0.0f, 0.0f)},
        {"hip",      glm::vec3(0.0f, 0.0f, pelvisRoll)},
        {"chest",    glm::vec3(2.0f, chestTwist, 0.0f)}
    };

    // Double bounce per walk cycle
    frame.verticalBounce = -0.03f * std::abs(std::cos(phase));
    return frame;
}

GaitFrame ProceduralGait::evaluateRun(float phase) {
    GaitFrame frame;
    // Larger leg swing
    float lThighPitch = 40.0f * std::sin(phase);
    float rThighPitch = -lThighPitch;

    // Deep knee flexion
    float lKnee = 65.0f * std::max(0.0f, std::cos(phase));
    float rKnee = 65.0f * std::max(0.0f, -std::cos(phase));

    // Ankle extension
    float lFoot = -15.0f * std::sin(phase);
    float rFoot = -lFoot;

    // Vigorous arm pump
    float lArmPitch = -35.0f * std::sin(phase);
    float rArmPitch = -lArmPitch;
    float lForearm = 45.0f + 20.0f * (1.0f - std::cos(phase));
    float rForearm = 45.0f + 20.0f * (1.0f + std::cos(phase));

    // Torso forward lean and dynamic sway
    float pelvisRoll = 4.0f * std::sin(phase);
    float chestTwist = -6.0f * std::sin(phase);

    frame.boneRotations = {
        {"lThigh",   glm::vec3(lThighPitch, 0.0f, 0.0f)},
        {"rThigh",   glm::vec3(rThighPitch, 0.0f, 0.0f)},
        {"lShin",    glm::vec3(lKnee, 0.0f, 0.0f)},
        {"rShin",    glm::vec3(rKnee, 0.0f, 0.0f)},
        {"lFoot",    glm::vec3(lFoot, 0.0f, 0.0f)},
        {"rFoot",    glm::vec3(rFoot, 0.0f, 0.0f)},
        {"lShldr",   glm::vec3(lArmPitch, 0.0f, -8.0f)},
        {"rShldr",   glm::vec3(rArmPitch, 0.0f, 8.0f)},
        {"lForeArm", glm::vec3(lForearm, 0.0f, 0.0f)},
        {"rForeArm", glm::vec3(rForearm, 0.0f, 0.0f)},
        {"hip",      glm::vec3(5.0f, 0.0f, pelvisRoll)},
        {"chest",    glm::vec3(8.0f, chestTwist, 0.0f)}
    };

    // Flight phase vertical bounce
    frame.verticalBounce = 0.06f * std::sin(2.0f * phase);
    return frame;
}

GaitFrame ProceduralGait::evaluate(GaitType type, float phase) {
    switch (type) {
        case GaitType::Run:  return evaluateRun(phase);
        case GaitType::Walk: return evaluateWalk(phase);
        case GaitType::Idle: return evaluateIdle(phase);
    }
    return evaluateIdle(phase);
}

GaitFrame ProceduralGait::evaluateBlend(GaitType typeA, GaitType typeB,
                                       float phaseA, float phaseB, float weight) {
    float w = std::clamp(weight, 0.0f, 1.0f);
    if (w <= 0.0f) return evaluate(typeA, phaseA);
    if (w >= 1.0f) return evaluate(typeB, phaseB);

    GaitFrame frameA = evaluate(typeA, phaseA);
    GaitFrame frameB = evaluate(typeB, phaseB);

    GaitFrame blended;
    blended.verticalBounce = (1.0f - w) * frameA.verticalBounce + w * frameB.verticalBounce;

    // Linear interpolate matching bone rotations
    for (const auto& [boneA, rotA] : frameA.boneRotations) {
        glm::vec3 rotB = rotA;
        for (const auto& [bB, rB] : frameB.boneRotations) {
            if (bB == boneA) {
                rotB = rB;
                break;
            }
        }
        glm::vec3 blendedRot = (1.0f - w) * rotA + w * rotB;
        blended.boneRotations.emplace_back(boneA, blendedRot);
    }

    return blended;
}

} // namespace pose

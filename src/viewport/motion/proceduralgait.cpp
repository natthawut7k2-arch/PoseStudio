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

GaitFrame ProceduralGait::evaluateIdle(float phase, const GaitParams& /*params*/) {
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
        {"rForeArm", glm::vec3(10.0f, 0.0f, 0.0f)},
        {"lThigh",   glm::vec3(0.0f, 0.0f, 0.0f)},
        {"rThigh",   glm::vec3(0.0f, 0.0f, 0.0f)},
        {"lShin",    glm::vec3(0.0f, 0.0f, 0.0f)},
        {"rShin",    glm::vec3(0.0f, 0.0f, 0.0f)},
        {"lFoot",    glm::vec3(0.0f, 0.0f, 0.0f)},
        {"rFoot",    glm::vec3(0.0f, 0.0f, 0.0f)}
    };
    frame.verticalBounce = std::sin(phase) * 0.005f;
    return frame;
}

GaitFrame ProceduralGait::evaluateWalk(float phase, const GaitParams& params) {
    GaitFrame frame;

    const float forwardLean = (params.forwardLean >= 0.0f) ? params.forwardLean : 3.0f;
    const float armSwing    = (params.armSwing >= 0.0f)    ? params.armSwing    : 22.0f;
    const float elbowBend   = (params.elbowBend >= 0.0f)   ? params.elbowBend   : 18.0f;
    const float kneeBend    = (params.kneeBend >= 0.0f)    ? params.kneeBend    : 46.0f;
    const float kneeStance  = (params.kneeStance >= 0.0f)  ? params.kneeStance  : 8.0f;
    const float torsoTwist  = (params.torsoTwist >= 0.0f)  ? params.torsoTwist  : 4.0f;

    // 1. Leg pitch (alternating sinusoidal swing)
    float lThighPitch = 25.0f * std::sin(phase);
    float rThighPitch = -lThighPitch;

    // 2. Realistic knee flexion: base slight bend + swing lift + stance shock absorption
    float lCos = std::cos(phase);
    float rCos = -lCos;
    float lSwingKnee = kneeBend * std::pow(std::max(0.0f, lCos), 1.3f);
    float rSwingKnee = kneeBend * std::pow(std::max(0.0f, rCos), 1.3f);
    float lStanceKnee = kneeStance * std::pow(std::max(0.0f, -lCos), 1.5f);
    float rStanceKnee = kneeStance * std::pow(std::max(0.0f, -rCos), 1.5f);
    float lKnee = 4.0f + lSwingKnee + lStanceKnee;
    float rKnee = 4.0f + rSwingKnee + rStanceKnee;

    // 3. Ankle compensation (dorsiflexion at heel strike, plantarflexion at push-off)
    float lFoot = -10.0f * std::sin(phase);
    float rFoot = -lFoot;

    // 4. Natural arm swing:
    // Shoulder pitch counter-swings opposite to legs
    float lArmPitch = -armSwing * std::sin(phase);
    float rArmPitch = -lArmPitch;
    // Slight inward yaw rotation of arm as it swings forward
    float lArmYaw = -torsoTwist * 0.8f * std::sin(phase);
    float rArmYaw = -lArmYaw;
    // Forearm flexion: naturally flexes more on forward swing
    float lForearm = elbowBend + 10.0f * (1.0f - lCos);
    float rForearm = elbowBend + 10.0f * (1.0f - rCos);

    // 5. Torso and Pelvis:
    float pelvisRoll = 2.5f * std::sin(phase);
    float chestTwist = -torsoTwist * std::sin(phase);
    float hipPitch   = forwardLean * 0.4f;
    float chestPitch = forwardLean * 0.6f;

    frame.boneRotations = {
        {"lThigh",   glm::vec3(lThighPitch, 0.0f, 0.0f)},
        {"rThigh",   glm::vec3(rThighPitch, 0.0f, 0.0f)},
        {"lShin",    glm::vec3(lKnee, 0.0f, 0.0f)},
        {"rShin",    glm::vec3(rKnee, 0.0f, 0.0f)},
        {"lFoot",    glm::vec3(lFoot, 0.0f, 0.0f)},
        {"rFoot",    glm::vec3(rFoot, 0.0f, 0.0f)},
        {"lShldr",   glm::vec3(lArmPitch, lArmYaw, -5.0f)},
        {"rShldr",   glm::vec3(rArmPitch, rArmYaw, 5.0f)},
        {"lForeArm", glm::vec3(lForearm, 0.0f, 0.0f)},
        {"rForeArm", glm::vec3(rForearm, 0.0f, 0.0f)},
        {"hip",      glm::vec3(hipPitch, 0.0f, pelvisRoll)},
        {"chest",    glm::vec3(chestPitch, chestTwist, 0.0f)}
    };

    // Double bounce per walk cycle
    frame.verticalBounce = -0.03f * std::abs(std::cos(phase));
    return frame;
}

GaitFrame ProceduralGait::evaluateRun(float phase, const GaitParams& params) {
    GaitFrame frame;

    const float forwardLean = (params.forwardLean >= 0.0f) ? params.forwardLean : 15.0f;
    const float armSwing    = (params.armSwing >= 0.0f)    ? params.armSwing    : 40.0f;
    const float elbowBend   = (params.elbowBend >= 0.0f)   ? params.elbowBend   : 70.0f;
    const float kneeBend    = (params.kneeBend >= 0.0f)    ? params.kneeBend    : 72.0f;
    const float kneeStance  = (params.kneeStance >= 0.0f)  ? params.kneeStance  : 18.0f;
    const float torsoTwist  = (params.torsoTwist >= 0.0f)  ? params.torsoTwist  : 7.0f;

    // 1. Larger leg swing
    float lThighPitch = 40.0f * std::sin(phase);
    float rThighPitch = -lThighPitch;

    // 2. Deep athletic knee flexion: base 8 deg + swing heel-kick + stance cushioning
    float lCos = std::cos(phase);
    float rCos = -lCos;
    float lSwingKnee = kneeBend * std::pow(std::max(0.0f, lCos), 1.2f);
    float rSwingKnee = kneeBend * std::pow(std::max(0.0f, rCos), 1.2f);
    float lStanceKnee = kneeStance * std::pow(std::max(0.0f, -lCos), 1.5f);
    float rStanceKnee = kneeStance * std::pow(std::max(0.0f, -rCos), 1.5f);
    float lKnee = 8.0f + lSwingKnee + lStanceKnee;
    float rKnee = 8.0f + rSwingKnee + rStanceKnee;

    // 3. Ankle extension & push-off
    float lFoot = -15.0f * std::sin(phase);
    float rFoot = -lFoot;

    // 4. Vigorous, bent-arm athletic pump
    float lArmPitch = -armSwing * std::sin(phase);
    float rArmPitch = -lArmPitch;
    float lArmYaw = -torsoTwist * 1.0f * std::sin(phase);
    float rArmYaw = -lArmYaw;
    // Forearms stay bent at acute athletic angle (~70°-95°), flexing as they drive forward
    float lForearm = elbowBend + 15.0f * (1.0f - lCos);
    float rForearm = elbowBend + 15.0f * (1.0f - rCos);

    // 5. Torso forward lean and dynamic rotation:
    float hipPitch   = forwardLean * 0.4f;
    float chestPitch = forwardLean * 0.6f;
    float pelvisRoll = 4.0f * std::sin(phase);
    float chestTwist = -torsoTwist * std::sin(phase);

    frame.boneRotations = {
        {"lThigh",   glm::vec3(lThighPitch, 0.0f, 0.0f)},
        {"rThigh",   glm::vec3(rThighPitch, 0.0f, 0.0f)},
        {"lShin",    glm::vec3(lKnee, 0.0f, 0.0f)},
        {"rShin",    glm::vec3(rKnee, 0.0f, 0.0f)},
        {"lFoot",    glm::vec3(lFoot, 0.0f, 0.0f)},
        {"rFoot",    glm::vec3(rFoot, 0.0f, 0.0f)},
        {"lShldr",   glm::vec3(lArmPitch, lArmYaw, -8.0f)},
        {"rShldr",   glm::vec3(rArmPitch, rArmYaw, 8.0f)},
        {"lForeArm", glm::vec3(lForearm, 0.0f, 0.0f)},
        {"rForeArm", glm::vec3(rForearm, 0.0f, 0.0f)},
        {"hip",      glm::vec3(hipPitch, 0.0f, pelvisRoll)},
        {"chest",    glm::vec3(chestPitch, chestTwist, 0.0f)}
    };

    // Flight phase vertical bounce
    frame.verticalBounce = 0.06f * std::sin(2.0f * phase);
    return frame;
}

GaitFrame ProceduralGait::evaluate(GaitType type, float phase, const GaitParams& params) {
    switch (type) {
        case GaitType::Run:  return evaluateRun(phase, params);
        case GaitType::Walk: return evaluateWalk(phase, params);
        case GaitType::Idle: return evaluateIdle(phase, params);
    }
    return evaluateIdle(phase, params);
}

GaitFrame ProceduralGait::evaluateBlend(GaitType typeA, GaitType typeB,
                                       float phaseA, float phaseB, float weight,
                                       const GaitParams& paramsA,
                                       const GaitParams& paramsB) {
    float w = std::clamp(weight, 0.0f, 1.0f);
    if (w <= 0.0f) return evaluate(typeA, phaseA, paramsA);
    if (w >= 1.0f) return evaluate(typeB, phaseB, paramsB);

    GaitFrame frameA = evaluate(typeA, phaseA, paramsA);
    GaitFrame frameB = evaluate(typeB, phaseB, paramsB);

    GaitFrame blended;
    blended.verticalBounce = (1.0f - w) * frameA.verticalBounce + w * frameB.verticalBounce;

    // 1. Interpolate bones in frameA against frameB (or 0 if missing in B)
    for (const auto& [boneA, rotA] : frameA.boneRotations) {
        glm::vec3 rotB(0.0f);
        for (const auto& [bB, rB] : frameB.boneRotations) {
            if (bB == boneA) {
                rotB = rB;
                break;
            }
        }
        glm::vec3 blendedRot = (1.0f - w) * rotA + w * rotB;
        blended.boneRotations.emplace_back(boneA, blendedRot);
    }

    // 2. Interpolate bones that only exist in frameB (blend from 0)
    for (const auto& [boneB, rotB] : frameB.boneRotations) {
        bool foundInA = false;
        for (const auto& [bA, rA] : frameA.boneRotations) {
            if (bA == boneB) {
                foundInA = true;
                break;
            }
        }
        if (!foundInA) {
            blended.boneRotations.emplace_back(boneB, w * rotB);
        }
    }

    return blended;
}

} // namespace pose

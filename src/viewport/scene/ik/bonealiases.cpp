/**
 * @file bonealiases.cpp
 * @brief The bone-alias table (see bonealiases.h): reference-generation names -> the other
 *        generations' names, tried in order after the name itself.
 */

#include "bonealiases.h"

#include "armature.h"

#include <cstddef>

namespace pose {

namespace {

struct BoneAlias {
    const char* canonical;
    const char* alternates[8];
};

const BoneAlias kBoneAliases[] = {
    {"lHand", {"l_hand", "LeftHand", "cf_j_hand_L", "cf_d_hand_L", "hand.L"}},
    {"rHand", {"r_hand", "RightHand", "cf_j_hand_R", "cf_d_hand_R", "hand.R"}},
    {"lFoot", {"l_foot", "LeftFoot", "Left ankle", "cf_j_foot_L", "foot.L"}},
    {"rFoot", {"r_foot", "RightFoot", "Right ankle", "cf_j_foot_R", "foot.R"}},
    {"lShin", {"l_shin", "lShin", "LeftLeg", "Left knee", "cf_j_leg_L", "shin.L"}},
    {"rShin", {"r_shin", "rShin", "RightLeg", "Right knee", "cf_j_leg_R", "shin.R"}},
    {"lThigh", {"l_thigh", "lThighBend", "LeftUpLeg", "Left leg", "cf_j_thigh_L", "thigh.L"}},
    {"rThigh", {"r_thigh", "rThighBend", "RightUpLeg", "Right leg", "cf_j_thigh_R", "thigh.R"}},
    // The newest generation's thigh twist bones hang OFF the chain (siblings of the shin); the
    // oldest generations have none — the harness's leg diagnostics then skip the twist metric.
    {"lThighTwist", {"l_thightwist1"}},
    {"rThighTwist", {"r_thightwist1"}},
    {"lForeArm", {"lForearmBend", "l_forearm", "LeftForeArm", "cf_s_forearm01_L", "cf_j_forearm_L", "forearm.L"}},
    {"rForeArm", {"rForearmBend", "r_forearm", "RightForeArm", "cf_s_forearm01_R", "cf_j_forearm_R", "forearm.R"}},
    {"lShldr", {"lShldrBend", "l_upperarm", "LeftArm", "Left arm", "cf_j_arm_L", "upper_arm.L"}},
    {"rShldr", {"rShldrBend", "r_upperarm", "RightArm", "Right arm", "cf_j_arm_R", "upper_arm.R"}},
    {"lCollar", {"l_shoulder", "LeftShoulder", "cf_j_shoulder_L"}},
    {"rCollar", {"r_shoulder", "RightShoulder", "cf_j_shoulder_R"}},
    {"lIndex3", {"l_index3"}},
    {"lMid3", {"l_mid3"}},
    {"lThumb3", {"l_thumb3"}},
    {"lBigToe", {"l_bigtoe1"}},
    {"lEye", {"l_eye"}},
    {"lEar", {"l_ear"}},
    {"lowerJaw", {"lowerjaw"}},
    {"head", {"head", "Head", "cf_j_head"}},
    {"neck", {"neckLower", "neck1", "Neck", "cf_j_neck"}},
    {"hip", {"hip", "pelvis", "Hips", "cf_j_hips"}},
    {"abdomenLower", {"abdomen", "spine1", "Spine"}},
    {"chest", {"chestLower", "spine3", "Spine1", "cf_j_spine02", "chest"}},
    {"chest_2", {"chestUpper", "spine4", "chest"}}, // the two-bone-chest generations: the top
};

} // namespace

int aliasedBoneIndex(const Armature& arm, const std::string& canonical) {
    int idx = arm.boneIndex(canonical);
    if (idx >= 0) {
        return idx;
    }
    for (const BoneAlias& alias : kBoneAliases) {
        if (canonical != alias.canonical) {
            continue;
        }
        for (const char* alt : alias.alternates) {
            if (alt != nullptr && (idx = arm.boneIndex(alt)) >= 0) {
                return idx;
            }
        }
    }
    return -1;
}

std::string aliasedBoneName(const Armature& arm, const std::string& canonical) {
    const int idx = aliasedBoneIndex(arm, canonical);
    return idx >= 0 ? arm.boneName(static_cast<std::size_t>(idx)) : canonical;
}

} // namespace pose

/**
 * @file humanoidautorigger.cpp
 * @brief Implementation of HumanoidAutoRigger.
 */

#include "humanoidautorigger.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace pose {

namespace {

struct BoneDef {
    const char* name;
    int         parent;
    glm::vec3   pos;
    glm::vec3   tip; // segment endpoint
    bool        isLeft;
    bool        isRight;
};

float distSqToSegment(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
    const glm::vec3 ab = b - a;
    const float len2 = glm::dot(ab, ab);
    if (len2 < 1e-8f) {
        const glm::vec3 d = p - a;
        return glm::dot(d, d);
    }
    const float t = glm::clamp(glm::dot(p - a, ab) / len2, 0.0f, 1.0f);
    const glm::vec3 proj = a + t * ab;
    const glm::vec3 d = p - proj;
    return glm::dot(d, d);
}

} // namespace

void HumanoidAutoRigger::buildAndSkin(ModelData& data) {
    if (data.meshes.empty()) {
        return;
    }

    // 1. Compute total model bounding box across all meshes
    glm::vec3 bMin(std::numeric_limits<float>::max());
    glm::vec3 bMax(std::numeric_limits<float>::lowest());
    std::size_t totalVerts = 0;

    for (const MeshData& m : data.meshes) {
        for (const Vertex& v : m.vertices) {
            bMin = glm::min(bMin, v.pos);
            bMax = glm::max(bMax, v.pos);
            ++totalVerts;
        }
    }

    if (totalVerts == 0 || bMin.y >= bMax.y) {
        return;
    }

    const float H = std::max(bMax.y - bMin.y, 0.1f);
    const float W = std::max(bMax.x - bMin.x, 0.1f);
    const float D = std::max(bMax.z - bMin.z, 0.05f);
    const float Cx = (bMin.x + bMax.x) * 0.5f;
    const float Cz = (bMin.z + bMax.z) * 0.5f;
    const float Cy = bMin.y;

    // 2. Sample outer arm vertices to detect arm height (A-pose vs T-pose)
    float armYSum = 0.0f;
    int   armVertCount = 0;
    const float outerXThreshold = 0.35f * W;

    for (const MeshData& m : data.meshes) {
        for (const Vertex& v : m.vertices) {
            if (std::abs(v.pos.x - Cx) > outerXThreshold) {
                armYSum += v.pos.y;
                ++armVertCount;
            }
        }
    }

    float relArmY = 0.50f; // default A-pose hand height relative to height
    if (armVertCount > 0) {
        const float avgArmY = armYSum / static_cast<float>(armVertCount);
        relArmY = glm::clamp((avgArmY - Cy) / H, 0.40f, 0.78f);
    }

    const float yElbow = Cy + (0.78f + (relArmY - 0.78f) * 0.5f) * H;
    const float yHand  = Cy + relArmY * H;
    const float handDirY = (yHand - yElbow) * 0.5f;

    // 3. Define 21 standard humanoid bones and segments
    std::vector<BoneDef> defs(21);

    // Trunk & Head
    defs[0]  = {"pelvis",       -1, {Cx, Cy + 0.52f * H, Cz},                 {Cx, Cy + 0.58f * H, Cz}, false, false};
    defs[1]  = {"abdomen",       0, {Cx, Cy + 0.58f * H, Cz},                 {Cx, Cy + 0.70f * H, Cz}, false, false};
    defs[2]  = {"chest",         1, {Cx, Cy + 0.70f * H, Cz},                 {Cx, Cy + 0.83f * H, Cz}, false, false};
    defs[3]  = {"neck",          2, {Cx, Cy + 0.83f * H, Cz},                 {Cx, Cy + 0.89f * H, Cz}, false, false};
    defs[4]  = {"head",          3, {Cx, Cy + 0.89f * H, Cz},                 {Cx, Cy + 1.00f * H, Cz}, false, false};

    // Upper Limbs (Left)
    defs[5]  = {"lCollar",       2, {Cx + 0.08f * W, Cy + 0.79f * H, Cz},      {Cx + 0.18f * W, Cy + 0.78f * H, Cz}, true, false};
    defs[7]  = {"lShldrBend",    5, {Cx + 0.18f * W, Cy + 0.78f * H, Cz},      {Cx + 0.36f * W, yElbow, Cz},         true, false};
    defs[9]  = {"lForearmBend",  7, {Cx + 0.36f * W, yElbow, Cz},              {Cx + 0.50f * W, yHand, Cz},          true, false};
    defs[11] = {"lHand",         9, {Cx + 0.50f * W, yHand, Cz},               {Cx + 0.56f * W, yHand + handDirY, Cz}, true, false};

    // Upper Limbs (Right)
    defs[6]  = {"rCollar",       2, {Cx - 0.08f * W, Cy + 0.79f * H, Cz},      {Cx - 0.18f * W, Cy + 0.78f * H, Cz}, false, true};
    defs[8]  = {"rShldrBend",    6, {Cx - 0.18f * W, Cy + 0.78f * H, Cz},      {Cx - 0.36f * W, yElbow, Cz},         false, true};
    defs[10] = {"rForearmBend",  8, {Cx - 0.36f * W, yElbow, Cz},              {Cx - 0.50f * W, yHand, Cz},          false, true};
    defs[12] = {"rHand",        10, {Cx - 0.50f * W, yHand, Cz},               {Cx - 0.56f * W, yHand + handDirY, Cz}, false, true};

    // Lower Limbs (Left)
    defs[13] = {"lThighBend",    0, {Cx + 0.10f * W, Cy + 0.50f * H, Cz},      {Cx + 0.10f * W, Cy + 0.27f * H, Cz}, true, false};
    defs[15] = {"lShin",        13, {Cx + 0.10f * W, Cy + 0.27f * H, Cz},      {Cx + 0.10f * W, Cy + 0.06f * H, Cz}, true, false};
    defs[17] = {"lFoot",        15, {Cx + 0.10f * W, Cy + 0.06f * H, Cz},      {Cx + 0.10f * W, Cy + 0.02f * H, Cz + 0.08f * H}, true, false};
    defs[19] = {"lToe",         17, {Cx + 0.10f * W, Cy + 0.02f * H, Cz + 0.08f * H}, {Cx + 0.10f * W, Cy + 0.01f * H, Cz + 0.14f * H}, true, false};

    // Lower Limbs (Right)
    defs[14] = {"rThighBend",    0, {Cx - 0.10f * W, Cy + 0.50f * H, Cz},      {Cx - 0.10f * W, Cy + 0.27f * H, Cz}, false, true};
    defs[16] = {"rShin",        14, {Cx - 0.10f * W, Cy + 0.27f * H, Cz},      {Cx - 0.10f * W, Cy + 0.06f * H, Cz}, false, true};
    defs[18] = {"rFoot",        16, {Cx - 0.10f * W, Cy + 0.06f * H, Cz},      {Cx - 0.10f * W, Cy + 0.02f * H, Cz + 0.08f * H}, false, true};
    defs[20] = {"rToe",         18, {Cx - 0.10f * W, Cy + 0.02f * H, Cz + 0.08f * H}, {Cx - 0.10f * W, Cy + 0.01f * H, Cz + 0.14f * H}, false, true};

    // 4. Fill ModelData::bones
    data.bones.clear();
    data.bones.reserve(defs.size());

    for (const BoneDef& bd : defs) {
        ModelBone mb;
        mb.name = bd.name;
        mb.parent = bd.parent;
        mb.bindGlobal = glm::translate(glm::mat4(1.0f), bd.pos);
        mb.orientation = glm::vec3(0.0f);
        mb.rotationOrder = "XYZ";
        mb.rotationMin = glm::vec3(-180.0f);
        mb.rotationMax = glm::vec3(180.0f);
        mb.rotationLimited = glm::bvec3(false);
        data.bones.push_back(std::move(mb));
    }

    // 5. Compute vertex skinning weights
    const float eps = 0.02f * H;
    const float epsSq = eps * eps;
    const float lateralSlack = 0.02f * W;

    struct CandWeight {
        uint32_t jointIndex;
        float    weight;
    };

    for (MeshData& m : data.meshes) {
        for (Vertex& v : m.vertices) {
            std::array<CandWeight, 21> cands;

            for (std::size_t b = 0; b < defs.size(); ++b) {
                const BoneDef& bd = defs[b];
                float d2 = distSqToSegment(v.pos, bd.pos, bd.tip);

                // Lateral separation penalty so left/right limbs don't steal opposite side vertices
                if (bd.isLeft && v.pos.x < Cx - lateralSlack) {
                    const float crossSideDist = (Cx - lateralSlack) - v.pos.x;
                    d2 += crossSideDist * crossSideDist * 25.0f;
                } else if (bd.isRight && v.pos.x > Cx + lateralSlack) {
                    const float crossSideDist = v.pos.x - (Cx + lateralSlack);
                    d2 += crossSideDist * crossSideDist * 25.0f;
                }

                const float rawW = 1.0f / (d2 + epsSq);
                cands[b] = {static_cast<uint32_t>(b), rawW};
            }

            // Pick the 4 bones with highest weights
            std::partial_sort(cands.begin(), cands.begin() + 4, cands.end(),
                              [](const CandWeight& a, const CandWeight& b) {
                                  return a.weight > b.weight;
                              });

            float totalW = cands[0].weight + cands[1].weight + cands[2].weight + cands[3].weight;
            if (totalW < 1e-8f) {
                totalW = 1.0f;
                cands[0].weight = 1.0f;
            }

            const float invTotal = 1.0f / totalW;
            v.joints = glm::uvec4(cands[0].jointIndex, cands[1].jointIndex,
                                  cands[2].jointIndex, cands[3].jointIndex);
            v.weights = glm::vec4(cands[0].weight * invTotal,
                                  cands[1].weight * invTotal,
                                  cands[2].weight * invTotal,
                                  cands[3].weight * invTotal);
        }
    }
}

} // namespace pose

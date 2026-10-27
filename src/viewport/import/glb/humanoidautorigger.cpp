/**
 * @file humanoidautorigger.cpp
 * @brief Implementation of HumanoidAutoRigger.
 */

#include "humanoidautorigger.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <initializer_list>
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

// PoseStudio's world unit is a metre and its figures import ~1.8 units tall (see
// FigureImportService's centimetre conversion). Static models arrive in whatever unit their DCC
// used — a character authored in some asset pipeline's own units can be 13 units tall, which puts
// the default camera INSIDE the model and makes every metre-tuned IK constant microscopic relative
// to the figure. Heights outside a plausible human range are therefore normalized to this.
constexpr float kTargetFigureHeight = 1.8f;
constexpr float kMinPlausibleHeight = 0.6f;
constexpr float kMaxPlausibleHeight = 3.0f;

// How far the feet's centroid must sit forward of the shins' before the model counts as facing
// -Z, as a fraction of figure height. Below this the foot geometry reads as symmetric (or the
// character is bare-footed/oddly posed) and the +Z convention is assumed.
constexpr float kMinFacingShift = 0.01f;

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

bool computeBounds(const ModelData& data, glm::vec3& bMin, glm::vec3& bMax) {
    bMin = glm::vec3(std::numeric_limits<float>::max());
    bMax = glm::vec3(std::numeric_limits<float>::lowest());
    bool any = false;
    for (const MeshData& m : data.meshes) {
        for (const Vertex& v : m.vertices) {
            bMin = glm::min(bMin, v.pos);
            bMax = glm::max(bMax, v.pos);
            any = true;
        }
    }
    return any && bMin.y < bMax.y;
}

// Centroid of the vertices in a horizontal band (used for facing detection and for standing the
// figure over its feet).
struct BandCentroid {
    glm::vec2 xz{0.0f};
    std::size_t count = 0;
};

BandCentroid bandCentroid(const ModelData& data, float y0, float y1, float maxAbsX = 1e30f) {
    BandCentroid out;
    glm::dvec2 sum(0.0);
    for (const MeshData& m : data.meshes) {
        for (const Vertex& v : m.vertices) {
            if (v.pos.y >= y0 && v.pos.y <= y1 && std::abs(v.pos.x) <= maxAbsX) {
                sum.x += v.pos.x;
                sum.y += v.pos.z;
                ++out.count;
            }
        }
    }
    if (out.count > 0) {
        out.xz = glm::vec2(static_cast<float>(sum.x / static_cast<double>(out.count)),
                           static_cast<float>(sum.y / static_cast<double>(out.count)));
    }
    return out;
}

// Forward extent of one foot: the heel and toe tip in Z. A foot's ankle sits near the heel end and
// the toe joint (the ball) about three quarters of the way forward, so measuring the span places
// both right regardless of where the model's origin ended up.
struct FootSpan {
    float heelZ = 0.0f;
    float toeZ = 0.0f;
    bool  valid = false;
};

FootSpan footSpan(const ModelData& data, float y0, float y1, bool leftSide) {
    FootSpan out;
    float lo = std::numeric_limits<float>::max();
    float hi = std::numeric_limits<float>::lowest();
    std::size_t count = 0;
    for (const MeshData& m : data.meshes) {
        for (const Vertex& v : m.vertices) {
            if (v.pos.y < y0 || v.pos.y > y1) {
                continue;
            }
            if (leftSide ? v.pos.x <= 0.0f : v.pos.x >= 0.0f) {
                continue;
            }
            lo = std::min(lo, v.pos.z);
            hi = std::max(hi, v.pos.z);
            ++count;
        }
    }
    if (count >= 8 && hi - lo > 1e-4f) {
        out.heelZ = lo;
        out.toeZ = hi;
        out.valid = true;
    }
    return out;
}

// Horizontal centre of one side's leg in a band (the leg bones follow the figure's actual stance
// instead of a fraction of the arm span, which varies wildly with the pose).
bool legCenterX(const ModelData& data, float y0, float y1, bool leftSide, float& outX) {
    std::vector<float> xs;
    for (const MeshData& m : data.meshes) {
        for (const Vertex& v : m.vertices) {
            if (v.pos.y < y0 || v.pos.y > y1) {
                continue;
            }
            if (leftSide ? v.pos.x > 0.0f : v.pos.x < 0.0f) {
                xs.push_back(v.pos.x);
            }
        }
    }
    if (xs.size() < 8) {
        return false;
    }
    const auto mid = xs.begin() + static_cast<std::ptrdiff_t>(xs.size() / 2);
    std::nth_element(xs.begin(), mid, xs.end());
    outX = *mid;
    return true;
}

std::string lowerAscii(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

bool nameContainsAny(const std::string& lowerName, std::initializer_list<const char*> tokens) {
    for (const char* token : tokens) {
        if (lowerName.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Matches a token against the START of one alphanumeric word of the name (names use '_', '.', '-'
// and spaces as separators), so "eyelash" and "hair_front" match "eye"/"hair" while "forearm"
// never matches "ear" and "surface" never matches "face".
bool nameHasWordPrefix(const std::string& lowerName, std::initializer_list<const char*> tokens) {
    const auto isWordChar = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0;
    };
    std::size_t i = 0;
    while (i < lowerName.size()) {
        while (i < lowerName.size() && !isWordChar(lowerName[i])) {
            ++i;
        }
        const std::size_t start = i;
        while (i < lowerName.size() && isWordChar(lowerName[i])) {
            ++i;
        }
        if (i == start) {
            continue;
        }
        for (const char* token : tokens) {
            const std::size_t len = std::strlen(token);
            if (i - start >= len && lowerName.compare(start, len, token) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Names that identify geometry which belongs rigidly to the head: a face, eyes/lashes, hair, teeth
// and tongue. Distance weighting alone splits these between head and neck (an eye sits almost
// exactly at the boundary), which makes them lag or smear the moment the head is turned. Only a
// hint: unnamed parts fall through to distance weighting, and everything above the head joint is
// bound rigidly regardless (see the vertex loop).
bool isHeadPartName(const std::string& lowerName) {
    return nameHasWordPrefix(lowerName, {"face", "facial", "head", "hair", "eye", "lash",
                                         "hitomi", "mayuge", "tooth", "teeth", "tongue",
                                         "mouth", "jaw", "ear", "nose"});
}

// Nails/claws are small, hard shells: they should ride one hand/foot rigidly rather than deform.
// Substring matching is fine here — "fingernail" should match too.
bool isNailName(const std::string& lowerName) {
    return nameContainsAny(lowerName, {"nail", "claw"});
}

// Normalizes a static figure into the convention the rig (and the rest of PoseStudio) assumes:
// standing on the floor at the origin, facing +Z, at human scale. Mutates positions and normals.
void normalizeFigure(ModelData& data) {
    glm::vec3 bMin;
    glm::vec3 bMax;
    if (!computeBounds(data, bMin, bMax)) {
        return;
    }
    float H = bMax.y - bMin.y;

    // 1. Scale implausible heights to a metre-scaled figure (a 1.8-unit import passes through).
    if (H < kMinPlausibleHeight || H > kMaxPlausibleHeight) {
        const float scale = kTargetFigureHeight / H;
        for (MeshData& m : data.meshes) {
            for (Vertex& v : m.vertices) {
                v.pos *= scale;
            }
        }
        if (!computeBounds(data, bMin, bMax)) {
            return;
        }
        H = bMax.y - bMin.y;
    }

    // 2. Face +Z. Toes protrude forward of the ankle, so the feet's centroid sits ahead of the
    // shins' when the model is already facing forward, and behind it when it faces -Z (many
    // Y-up exports do). Rotating the mesh (not mirroring it) also puts the character's LEFT side
    // on +X, where the rig's l* bones live.
    const float floorY = bMin.y;
    const BandCentroid feet = bandCentroid(data, floorY, floorY + 0.06f * H);
    const BandCentroid shins = bandCentroid(data, floorY + 0.10f * H, floorY + 0.30f * H);
    if (feet.count > 0 && shins.count > 0 &&
        feet.xz.y - shins.xz.y < -kMinFacingShift * H) {
        for (MeshData& m : data.meshes) {
            for (Vertex& v : m.vertices) {
                v.pos.x = -v.pos.x;
                v.pos.z = -v.pos.z;
                v.normal.x = -v.normal.x;
                v.normal.z = -v.normal.z;
            }
        }
    }

    // 3. Stand the figure over the origin with its feet on the floor (height bands below are
    // referenced from there).
    if (!computeBounds(data, bMin, bMax)) {
        return;
    }
    const BandCentroid stand = bandCentroid(data, bMin.y, bMin.y + 0.06f * (bMax.y - bMin.y));
    const glm::vec2 centre = stand.count > 0 ? stand.xz : glm::vec2((bMin.x + bMax.x) * 0.5f,
                                                                    (bMin.z + bMax.z) * 0.5f);
    for (MeshData& m : data.meshes) {
        for (Vertex& v : m.vertices) {
            v.pos.x -= centre.x;
            v.pos.z -= centre.y;
            v.pos.y -= bMin.y;
        }
    }
}

} // namespace

void HumanoidAutoRigger::buildAndSkin(ModelData& data) {
    if (data.meshes.empty()) {
        return;
    }

    normalizeFigure(data);

    // 1. Compute the (now standing, +Z-facing, human-scale) bounding box
    glm::vec3 bMin;
    glm::vec3 bMax;
    if (!computeBounds(data, bMin, bMax)) {
        return;
    }

    const float H = std::max(bMax.y - bMin.y, 0.1f);
    const float W = std::max(bMax.x - bMin.x, 0.1f);
    const float Cy = bMin.y;

    // The trunk's own centre, not the bounding box's: the box is pulled back by long hair and
    // sideways by an asymmetric arm pose, which would put the spine (and every twist around it)
    // behind and beside the body. Vertices well inside the torso width exclude hands/arms.
    const BandCentroid torso = bandCentroid(data, Cy + 0.45f * H, Cy + 0.62f * H, 0.22f * W);
    const float Cx = torso.count > 0 ? torso.xz.x : (bMin.x + bMax.x) * 0.5f;
    const float Cz = torso.count > 0 ? torso.xz.y : (bMin.z + bMax.z) * 0.5f;

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

    // Measured leg centres (fall back to the old fraction of the arm span when a band has no
    // usable geometry). The arm span is a poor proxy for hip width — it changes with the pose.
    float lHipX = Cx + 0.08f * W;
    float rHipX = Cx - 0.08f * W;
    float lAnkleX = Cx + 0.10f * W;
    float rAnkleX = Cx - 0.10f * W;
    legCenterX(data, Cy + 0.44f * H, Cy + 0.49f * H, true, lHipX);
    legCenterX(data, Cy + 0.44f * H, Cy + 0.49f * H, false, rHipX);
    legCenterX(data, Cy + 0.05f * H, Cy + 0.11f * H, true, lAnkleX);
    legCenterX(data, Cy + 0.05f * H, Cy + 0.11f * H, false, rAnkleX);
    // The knee lies on the hip-to-ankle axis. Measuring its own band directly is noisy (bare
    // knees have few vertices), and a kink there makes the shin swing off the actual leg.
    const float lKneeX = (lHipX + lAnkleX) * 0.5f;
    const float rKneeX = (rHipX + rAnkleX) * 0.5f;

    // Ankle/toe joints from each foot's measured heel-to-toe span. The old fixed 0.08H/0.14H
    // forward offsets put the toe joint past the toe tips on figures whose feet are shorter than
    // the reference rig, so bending a toe tore it while the toe nails bound to the ankle instead.
    const float fallbackFootLen = 0.15f * H; // used only when the foot band has no geometry
    float lAnkleZ = Cz;
    float lBallZ = Cz + 0.55f * fallbackFootLen;
    float lToeTipZ = Cz + 0.80f * fallbackFootLen;
    float rAnkleZ = Cz;
    float rBallZ = Cz + 0.55f * fallbackFootLen;
    float rToeTipZ = Cz + 0.80f * fallbackFootLen;
    const float footY0 = Cy;
    const float footY1 = Cy + 0.06f * H;
    const FootSpan lFootSpan = footSpan(data, footY0, footY1, true);
    const FootSpan rFootSpan = footSpan(data, footY0, footY1, false);
    if (lFootSpan.valid) {
        const float len = lFootSpan.toeZ - lFootSpan.heelZ;
        lAnkleZ = lFootSpan.heelZ + 0.30f * len;
        lBallZ = lFootSpan.heelZ + 0.72f * len;
        lToeTipZ = lFootSpan.toeZ;
    }
    if (rFootSpan.valid) {
        const float len = rFootSpan.toeZ - rFootSpan.heelZ;
        rAnkleZ = rFootSpan.heelZ + 0.30f * len;
        rBallZ = rFootSpan.heelZ + 0.72f * len;
        rToeTipZ = rFootSpan.toeZ;
    }
    const float ankleY = Cy + 0.055f * H;
    const float ballY = Cy + 0.025f * H;
    const float toeTipY = Cy + 0.015f * H;

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
    defs[13] = {"lThighBend",    0, {lHipX, Cy + 0.50f * H, Cz},      {lKneeX, Cy + 0.27f * H, Cz}, true, false};
    defs[15] = {"lShin",        13, {lKneeX, Cy + 0.27f * H, Cz},      {lAnkleX, Cy + 0.06f * H, Cz}, true, false};
    defs[17] = {"lFoot",        15, {lAnkleX, ankleY, lAnkleZ}, {lAnkleX, ballY, lBallZ}, true, false};
    defs[19] = {"lToe",         17, {lAnkleX, ballY, lBallZ}, {lAnkleX, toeTipY, lToeTipZ}, true, false};

    // Lower Limbs (Right)
    defs[14] = {"rThighBend",    0, {rHipX, Cy + 0.50f * H, Cz},      {rKneeX, Cy + 0.27f * H, Cz}, false, true};
    defs[16] = {"rShin",        14, {rKneeX, Cy + 0.27f * H, Cz},      {rAnkleX, Cy + 0.06f * H, Cz}, false, true};
    defs[18] = {"rFoot",        16, {rAnkleX, ankleY, rAnkleZ}, {rAnkleX, ballY, rBallZ}, false, true};
    defs[20] = {"rToe",         18, {rAnkleX, ballY, rBallZ}, {rAnkleX, toeTipY, rToeTipZ}, false, true};

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

    // Everything above the head joint is skull/face/hair: rigid to the head (see isHeadPartName).
    const float headLevel = Cy + 0.89f * H;
    constexpr uint32_t kHeadJoint = 4;
    // Hands and feet/toes, for the rigid nails/claws above.
    const std::array<uint32_t, 6> kExtremities{11, 12, 17, 18, 19, 20};

    struct CandWeight {
        uint32_t jointIndex;
        float    weight;
    };

    for (MeshData& m : data.meshes) {
        const std::string lowerName = lowerAscii(m.name);
        const bool headMesh = isHeadPartName(lowerName);
        const bool nailMesh = isNailName(lowerName);

        for (Vertex& v : m.vertices) {
            // Face, eyes, hair and anything above the head joint ride the head rigidly.
            if (headMesh || v.pos.y >= headLevel) {
                v.joints = glm::uvec4(kHeadJoint, 0, 0, 0);
                v.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                continue;
            }
            // Nails/claws are rigid shells: bind each to its nearest hand/foot/toe instead of
            // letting them deform between the finger and the forearm bones.
            if (nailMesh) {
                uint32_t nearest = kExtremities[0];
                float bestD2 = std::numeric_limits<float>::max();
                for (uint32_t joint : kExtremities) {
                    const float d2 = distSqToSegment(v.pos, defs[joint].pos, defs[joint].tip);
                    if (d2 < bestD2) {
                        bestD2 = d2;
                        nearest = joint;
                    }
                }
                v.joints = glm::uvec4(nearest, 0, 0, 0);
                v.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                continue;
            }

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

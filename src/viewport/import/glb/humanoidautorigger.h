/**
 * @file humanoidautorigger.h
 * @brief Automatic humanoid skeleton generation and distance-based skinning for unrigged models.
 *
 * For character models imported without an embedded skeleton (e.g. static mesh GLB exports like
 * Midoriya.glb), HumanoidAutoRigger:
 * 1. Analyzes the model's bounding geometry and detects arm span elevation (A-pose vs T-pose).
 * 2. Constructs a standard 21-bone humanoid skeleton matching PoseStudio's FBIK conventions
 *    (IkRig and BalanceController keywords).
 * 3. Computes bone segment distance falloff weights for every vertex and normalizes them (up to
 *    4 joints per vertex, sum = 1.0), enabling immediate posing in PoseStudio's 3D viewport.
 *
 * Pure C++17 + GLM, strictly Qt-free.
 */

#ifndef HUMANOID_AUTO_RIGGER_H
#define HUMANOID_AUTO_RIGGER_H

#include "modeldata.h"

namespace pose {

class HumanoidAutoRigger {
public:
    /// Constructs a standard 21-bone humanoid skeleton sized and positioned to fit @p data's
    /// bounding geometry, and calculates normalized 4-joint skinning weights on all vertices.
    static void buildAndSkin(ModelData& data);
};

} // namespace pose

#endif // HUMANOID_AUTO_RIGGER_H

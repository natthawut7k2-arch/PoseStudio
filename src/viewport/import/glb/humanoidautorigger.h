/**
 * @file humanoidautorigger.h
 * @brief Automatic humanoid skeleton generation and distance-based skinning for unrigged models.
 *
 * For character models imported without an embedded skeleton (e.g. static mesh GLB exports like
 * Midoriya.glb), HumanoidAutoRigger:
 * 0. Normalizes the figure into the convention everything downstream assumes: standing on the
 *    floor at the origin, facing +Z, at human (metre) scale. Exports routinely hand over a Z-up
 *    mesh, a model facing -Z, or geometry authored at 10x/100x scale; without this step the rig
 *    is built along the wrong axis and the default camera sits inside the model.
 * 1. Analyzes the model's bounding geometry and detects arm span elevation (A-pose vs T-pose),
 *    and measures the stance's actual leg centres (the arm span is not a hip width).
 * 2. Constructs a standard 21-bone humanoid skeleton matching PoseStudio's FBIK conventions
 *    (IkRig and BalanceController keywords).
 * 3. Computes bone segment distance falloff weights for every vertex and normalizes them (up to
 *    4 joints per vertex, sum = 1.0), enabling immediate posing in PoseStudio's 3D viewport.
 *    Head geometry (anything above the head joint, plus meshes named like face/hair/eye parts)
 *    and nails/claws are bound rigidly to one bone so they cannot smear when posed.
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

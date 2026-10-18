/**
 * @file glbimporter.h
 * @brief Binary glTF 2.0 (.glb) and text glTF (.gltf) model importer.
 *
 * Implements MeshImporter (import/meshimporter.h) using the single-header cgltf parser.
 * Handles meshes, materials, embedded PNG/JPEG textures, and embedded skins. If the file lacks
 * a skeleton (such as static-mesh exports like Midoriya.glb), it invokes HumanoidAutoRigger to
 * automatically bind a standard 21-bone humanoid rig in standing pose. Pure C++17 + GLM, strictly Qt-free.
 */

#ifndef GLBIMPORTER_H
#define GLBIMPORTER_H

#include "import/meshimporter.h"

namespace pose {

class GlbImporter : public MeshImporter {
public:
    std::string displayName() const override { return "GL Transmission Format (.glb/.gltf)"; }
    std::vector<std::string> extensions() const override { return {"glb", "gltf"}; }
    ModelData load(const std::string& path) const override;
};

} // namespace pose

#endif // GLBIMPORTER_H

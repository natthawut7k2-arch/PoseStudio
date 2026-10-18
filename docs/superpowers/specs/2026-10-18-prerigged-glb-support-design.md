# Pre-Rigged GLB Import & Skeleton Preservation Design Specification

## 1. Overview & Objective
PoseStudio currently supports importing unrigged humanoid models (like `Midoriya.glb`) by automatically synthesizing a 21-bone standard humanoid skeleton and calculating segment-distance skinning weights via `HumanoidAutoRigger`.

This feature extends `GlbImporter` to support **Pre-Rigged GLB / glTF models** (such as `D:\Male60.glb`, `D:\Ru-haLee_Blender5.glb`, and characters exported from Blender, Mixamo, VRoid, or game engines) that already contain embedded `skins`, bone hierarchies, and vertex weights (`JOINTS_0`, `WEIGHTS_0`), while seamlessly falling back to `HumanoidAutoRigger` when no embedded skin is present.

---

## 2. Architecture & Data Flow

```
                     glTF File (.glb / .gltf)
                                |
                                v
                       cgltf_parse_file()
                                |
                   Has skins & JOINTS_0/WEIGHTS_0?
                         /              \
                   YES  /                \  NO
                       v                  v
          Pre-Rigged Skin Pipeline    HumanoidAutoRigger
          - Extract skin.joints       (Synthesize 21 bones
          - Compute world positions    and distance skinning)
          - Topological order check
          - Filter helper/colliders
                       \                  /
                        \                /
                         v              v
                           ModelData
                     (meshes + bones + DQS)
                                |
                                v
                           VulkanWindow
                    (Armature + IkRig + SSBO)
```

---

## 3. Detailed Component Design

### 3.1 Dual-Mode Branching in `GlbImporter`
In `src/viewport/import/glb/glbimporter.cpp`:
- Inspect `data->skins_count`.
- Check if at least one mesh primitive contains `cgltf_attribute_type_joints` and `cgltf_attribute_type_weights`.
- If true: activate embedded skin pipeline.
- If false: call `HumanoidAutoRigger::buildAndSkin(model)`.

### 3.2 Embedded Joint Extraction & Armature Compatibility
In PoseStudio, `Armature` enforces two structural invariants:
1. **Translation-Only Rest Global Transforms:**
   ```cpp
   mb.bindGlobal = glm::translate(glm::mat4(1.0f), world_pos);
   mb.orientation = glm::vec3(0.0f);
   ```
   At rest pose, `skinTransform = poseGlobal * inverseBind = translate(world_pos) * translate(-world_pos) = Identity`.
   This ensures dual-quaternion skinning renders the bind mesh with 0.0 deformation error at rest.
2. **Topological Order (`parent < i`):**
   `Armature::build` requires that every bone's parent index must be strictly less than its own index.
   If a model has non-sequential joints, topological sorting is applied and `vert.joints` indices are remapped accordingly.

### 3.3 Helper / Collider Filtering in `BalanceController` and `IkRig`
Pre-rigged models exported from engines or Koikatsu (like `Male60.glb`) often bundle non-deforming collision hitboxes (`cf_hit_*`, `KK_Colliders_*`) or IK pole vectors (`cf_pv_*`, `cf_ik_*`):
1. **`BalanceController::classify`:**
   Ignore bones matching `{"hit_", "pv_", "ik_", "collider", "hitbox", "target", "dummy"}` so they do not receive mass fractions or skew the Center of Mass (CoM).
2. **`IkRig::build` Pelvis Rooting:**
   Search for canonical hip/pelvis names (case-insensitive: `"hips"`, `"pelvis"`, `"cf_j_hips"`, `"hip"`) that sit above $0.3 \times \text{height}$, preventing the FBIK root from locking onto a floor origin node (`Center` at $Y = 0$).

---

## 4. Verification Strategy
1. **Unit Tests:**
   - Test embedded skin extraction on `D:\Male60.glb`: 1,170 joints extracted, parent order strictly topological (`parent < i`).
   - Test vertex weights: all vertex weights normalized to $1.0$.
2. **End-to-End Posing & Balance Verification:**
   - FBIK pelvis correctly roots at `Hips` ($Y \approx 0.85\text{ m}$).
   - Center of mass sits inside foot support polygon.
   - Limb poses (arm reach, knee bend) solve cleanly.
3. **Regression Test:**
   - Verify unrigged model `D:\koikatsu\UserData\Midoriya\Midoriya.glb` still auto-rigs seamlessly without regression.

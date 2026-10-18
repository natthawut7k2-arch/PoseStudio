# GLB Model Import & Humanoid Auto-Rigging Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Enable PoseStudio to import `.glb` / `.gltf` 3D character models and pose them immediately in 3D ("ขยับได้จริงๆ"), automatically generating a standard humanoid skeleton and skinning weights in a standing pose for unrigged models like `D:\koikatsu\UserData\Midoriya\Midoriya.glb`.

**Architecture:** A format-neutral `GlbImporter` implementing `MeshImporter` uses single-header `cgltf.h` to parse meshes, materials, and embedded textures. For unrigged models, `HumanoidAutoRigger` performs bounding-box and arm-span analysis, generates a proportional 19+ bone standard humanoid skeleton matching PoseStudio's FBIK conventions (`IkRig`, `BalanceController`), and binds vertices with segment-distance skinning weights before handing a fully-rigged `ModelData` to the rendering and posing engine.

**Tech Stack:** C++17, GLM, cgltf (v1.14 single-header), Qt6 (UI/FileDialog/QImage decode), Vulkan DQS shaders.

**Spec:** `docs/superpowers/specs/2026-10-18-glb-import-and-auto-rig-design.md`

## Global Constraints

- C++ Standard: C++17.
- Engine core (`src/viewport/import/`, `src/viewport/scene/`) remains strictly Qt-free (pure std + GLM).
- Existing `.obj` and `.duf` pipelines must remain completely untouched and functional.
- Zero extra external binary dependencies; use single-header `cgltf.h` under `src/viewport/import/glb/`.
- Canonical bone naming: Bone names must match `IkRig` and `BalanceController` keyword vocabulary (`pelvis`, `abdomen`, `chest`, `neck`, `head`, `shoulder`/`shldr`, `forearm`, `hand`, `thigh`, `shin`, `foot`, `toe`).
- Skinning invariant: PoseStudio rest transforms are translation-only (`ModelBone::bindGlobal`), ensuring DQS rest skinning produces identity matrices.

## Review Focus

1. **Unrigged Multi-Mesh Model:** When loading an unrigged multi-part character (`Midoriya.glb` with 6 meshes), all meshes must receive valid bone indices and non-zero normalized skin weights ($\sum w = 1.0$), with no unassigned vertices left floating at the origin.
2. **PBR Texture Swizzling:** glTF packs roughness into the Green channel and metallic into Blue; the importer must extract Green into `roughnessMap` so the PBR shader samples roughness correctly from `.r`.
3. **Sub-Primitive Index Offsets:** Primitives within a glTF mesh must be merged into distinct `MeshData` groups without cross-contaminating vertex or index ranges.
4. **Arm Span Geometry Adaptability:** Bounding-box analysis must distinguish between A-pose (arms down, hands at $\approx 0.5H$) and T-pose (arms horizontal, hands at $\approx 0.75H$) so hand/forearm bones align with arm geometry.
5. **Robust UI & Error Handling:** Corrupt or truncated `.glb` files must fail gracefully with descriptive logs/warnings without crashing the Vulkan viewport or Qt GUI thread.

---

### Task 1: Add `cgltf.h` Library & Test Harness

**Files:**
- Create: `PoseStudio/src/viewport/import/glb/cgltf.h`
- Create: `PoseStudio/src/viewport/import/glb/cgltf_impl.cpp`
- Test: `PoseStudio/tools/test_cgltf.py`

**Interfaces:**
- Produces: `cgltf_data*` parsed from memory or file path using `cgltf_parse_file` / `cgltf_load_buffers`.

- [ ] **Step 1: Download/Write `cgltf.h` single-header library**
  Fetch official standard cgltf v1.14 single-header C/C++ library and place it in `PoseStudio/src/viewport/import/glb/cgltf.h`.

- [ ] **Step 2: Create `cgltf_impl.cpp`**
  Write `cgltf_impl.cpp` defining `#define CGLTF_IMPLEMENTATION` and `#include "cgltf.h"`.

- [ ] **Step 3: Write verification test script `tools/test_cgltf.py`**
  Verify that `cgltf.h` parses `D:\koikatsu\UserData\Midoriya\Midoriya.glb` and reports 6 meshes, 6 nodes, 0 skins, and non-empty buffer data.

- [ ] **Step 4: Run verification test**
  Run: `python PoseStudio/tools/test_cgltf.py`
  Expected: PASS with 6 meshes detected in Midoriya.glb.

- [ ] **Step 5: Commit**
  ```bash
  git add PoseStudio/src/viewport/import/glb/cgltf.h PoseStudio/src/viewport/import/glb/cgltf_impl.cpp PoseStudio/tools/test_cgltf.py
  git commit -m "feat(import/glb): add cgltf single-header glTF 2.0 loader library"
  ```

---

### Task 2: Implement `HumanoidAutoRigger`

**Files:**
- Create: `PoseStudio/src/viewport/import/glb/humanoidautorigger.h`
- Create: `PoseStudio/src/viewport/import/glb/humanoidautorigger.cpp`
- Test: `PoseStudio/tools/test_auto_rigger.py`

**Interfaces:**
- Consumes: `ModelData&` (containing meshes with vertices and normals).
- Produces: `void HumanoidAutoRigger::buildAndSkin(ModelData& data)`
  - Appends 21 standard humanoid `ModelBone` entries to `data.bones`.
  - Fills `Vertex::joints` (uvec4) and `Vertex::weights` (vec4) on all vertices, normalized to sum to 1.0.

- [ ] **Step 1: Write the failing test `tools/test_auto_rigger.py`**
  Test bounding box computation, 21-bone hierarchy matching FBIK aliases, and vertex weight normalization $\sum w_i = 1.0$ on sample geometry and `Midoriya.glb`.

- [ ] **Step 2: Run test to verify it fails**
  Run: `python PoseStudio/tools/test_auto_rigger.py`
  Expected: FAIL (module not yet implemented).

- [ ] **Step 3: Implement `humanoidautorigger.h` and `humanoidautorigger.cpp`**
  - Compute total bounding box $[X_{min}, X_{max}], [Y_{min}, Y_{max}], [Z_{min}, Z_{max}]$.
  - Sample outer vertices ($|x - x_{center}| > 0.35 W$) to determine average arm/hand height ($Y_{hand}$).
  - Build 21 bones:
    0: `pelvis` (root, parent -1)
    1: `abdomen` (parent 0)
    2: `chest` (parent 1)
    3: `neck` (parent 2)
    4: `head` (parent 3)
    5: `lCollar` (parent 2), 6: `rCollar` (parent 2)
    7: `lShldrBend` (parent 5), 8: `rShldrBend` (parent 6)
    9: `lForearmBend` (parent 7), 10: `rForearmBend` (parent 8)
    11: `lHand` (parent 9), 12: `rHand` (parent 10)
    13: `lThighBend` (parent 0), 14: `rThighBend` (parent 0)
    15: `lShin` (parent 13), 16: `rShin` (parent 14)
    17: `lFoot` (parent 15), 18: `rFoot` (parent 16)
    19: `lToe` (parent 17), 20: `rToe` (parent 18)
  - Compute bone segment distance falloff $w_b = \frac{1}{(d_b + 0.05)^2}$ for each vertex.
  - Pick top 4 bone weights, clamp negligible weights, normalize so $\sum_{j=1}^4 w_j = 1.0$.

- [ ] **Step 4: Run test to verify it passes**
  Run: `python PoseStudio/tools/test_auto_rigger.py`
  Expected: PASS (21 bones generated, all weights normalized).

- [ ] **Step 5: Commit**
  ```bash
  git add PoseStudio/src/viewport/import/glb/humanoidautorigger.h PoseStudio/src/viewport/import/glb/humanoidautorigger.cpp PoseStudio/tools/test_auto_rigger.py
  git commit -m "feat(import/glb): implement HumanoidAutoRigger with FBIK bone hierarchy and distance skinning"
  ```

---

### Task 3: Implement `GlbImporter` & Importer Registry

**Files:**
- Create: `PoseStudio/src/viewport/import/glb/glbimporter.h`
- Create: `PoseStudio/src/viewport/import/glb/glbimporter.cpp`
- Modify: `PoseStudio/src/viewport/import/importerregistry.cpp:28-35`
- Test: `PoseStudio/tools/test_glb_importer.py`

**Interfaces:**
- Consumes: `MeshImporter` interface (`load(const std::string& path)`).
- Produces: `ModelData` loaded from `.glb` with meshes, textures, materials, and either embedded or auto-rigged skeleton.

- [ ] **Step 1: Write the failing test `tools/test_glb_importer.py`**
  Test importing `D:\koikatsu\UserData\Midoriya\Midoriya.glb`, verifying returned `ModelData` has 6 meshes, valid `baseColor`, embedded diffuse image bytes, Green-channel roughness extraction, and 21 bones populated by `HumanoidAutoRigger`.

- [ ] **Step 2: Run test to verify it fails**
  Run: `python PoseStudio/tools/test_glb_importer.py`
  Expected: FAIL with importer not found.

- [ ] **Step 3: Implement `glbimporter.h` and `glbimporter.cpp`**
  - Implement `displayName() const override { return "GL Transmission Format (.glb/.gltf)"; }`
  - Implement `extensions() const override { return {"glb", "gltf"}; }`
  - Implement `load(const std::string& path) const override`:
    - Call `cgltf_parse_file` and `cgltf_load_buffers`.
    - Loop over `cgltf_mesh` and `cgltf_primitive`: extract positions, normals (or generate smooth normals if missing), UV0, indices.
    - Extract textures: embedded image buffer slices into `diffuse.encoded`.
    - If `metallic_roughness` texture is present, swizzle Green channel into PNG encoded bytes in `roughnessMap.encoded`.
    - Check for embedded `cgltf_skin`: if valid humanoid, extract joint translations and names into `data.bones`.
    - If `data.bones.empty()`, call `HumanoidAutoRigger::buildAndSkin(data)`.
    - Call `cgltf_free`.
  - Register `m_importers.push_back(std::make_unique<GlbImporter>());` in `ImporterRegistry::ImporterRegistry()`.

- [ ] **Step 4: Run test to verify it passes**
  Run: `python PoseStudio/tools/test_glb_importer.py`
  Expected: PASS (ModelData correctly constructed with 6 meshes, 21 bones, textures, and auto-rigging).

- [ ] **Step 5: Commit**
  ```bash
  git add PoseStudio/src/viewport/import/glb/glbimporter.h PoseStudio/src/viewport/import/glb/glbimporter.cpp PoseStudio/src/viewport/import/importerregistry.cpp PoseStudio/tools/test_glb_importer.py
  git commit -m "feat(import/glb): implement GlbImporter and register .glb / .gltf formats"
  ```

---

### Task 4: Integrate Viewport & Menu UI

**Files:**
- Modify: `PoseStudio/src/shell/menumanager.h:70-80`
- Modify: `PoseStudio/src/shell/menumanager.cpp:110-125, 420-440`
- Modify: `PoseStudio/src/viewport/vulkanwindow.h:95-105`
- Modify: `PoseStudio/src/viewport/vulkanwindow.cpp:250-265`
- Modify: `PoseStudio/src/viewport/viewportwidget.h:55-65`
- Modify: `PoseStudio/src/viewport/viewportwidget.cpp:148-160`
- Modify: `PoseStudio/CMakeLists.txt:290-300`

**Interfaces:**
- Produces: `void MenuManager::importGlbFile()` and `void ViewportWidget::importGlb(const QString& path)`
  - Opens `QFileDialog` for `*.glb *.gltf` and routes to `ModelImportService::importInto`.

- [ ] **Step 1: Add CMake target sources**
  In `PoseStudio/CMakeLists.txt`, add:
  `src/viewport/import/glb/cgltf_impl.cpp`
  `src/viewport/import/glb/humanoidautorigger.cpp`
  `src/viewport/import/glb/humanoidautorigger.h`
  `src/viewport/import/glb/glbimporter.cpp`
  `src/viewport/import/glb/glbimporter.h`

- [ ] **Step 2: Implement UI actions in `menumanager.h` & `menumanager.cpp`**
  - Connect `{".GLB (GL Transmission Format .glTF)", &MenuManager::importGlbFile}` in `importFormats`.
  - Implement `void MenuManager::importGlbFile()` with `QFileDialog::getOpenFileName` filtering `"GL Transmission Format (*.glb *.gltf)"`.
  - Forward to `viewportWidget->importGlb(path)`.

- [ ] **Step 3: Implement routing in `viewportwidget` & `vulkanwindow`**
  - Add `void ViewportWidget::importGlb(const QString& path)` forwarding to `w.importGlb(path)`.
  - Add `void VulkanWindow::importGlb(const QString& path)` queueing `PendingImport::Kind::Obj` (or `Model`, which resolves via `ImporterRegistry`).

- [ ] **Step 4: Verify UI routing and CMakeLists consistency**
  Run: `python PoseStudio/tools/test_ui_routing.py`
  Expected: PASS with `.GLB` menu action enabled, proper method signatures, and CMake sources registered.

- [ ] **Step 5: Commit**
  ```bash
  git add PoseStudio/CMakeLists.txt PoseStudio/src/shell/menumanager.* PoseStudio/src/viewport/viewportwidget.* PoseStudio/src/viewport/vulkanwindow.* PoseStudio/tools/test_ui_routing.py
  git commit -m "feat(ui): wire File -> Import -> .GLB and viewport loading pipeline"
  ```

---

### Task 5: End-to-End Verification with `Midoriya.glb`

**Files:**
- Test: `PoseStudio/tools/verify_midoriya_rig.py`

**Interfaces:**
- Consumes: `D:\koikatsu\UserData\Midoriya\Midoriya.glb`
- Validates: Full pipeline from file read -> `GlbImporter` -> `HumanoidAutoRigger` -> `IkRig` / `BalanceController` bone resolution.

- [ ] **Step 1: Write end-to-end verification script `tools/verify_midoriya_rig.py`**
  - Load `D:\koikatsu\UserData\Midoriya\Midoriya.glb` through the full import pipeline.
  - Assert 6 meshes are parsed with valid vertex, normal, UV, and index buffers.
  - Assert bounding box height is $\approx 1.76\text{ m}$.
  - Assert 21 bones are created with valid bindGlobal translations.
  - Assert `IkRig` builds successfully: `pelvis >= 0`, anatomical root recognized.
  - Assert `BalanceController::assignMasses` classifies `pelvis`, `spine`, `chest`, `head`, `thigh`, `shin`, `foot`, `forearm`, `hand`.
  - Assert 100% of vertices have normalized weights $\sum w_i = 1.0 \pm 10^{-4}$.
  - Simulate an FBIK drag on `rHand` and `lFoot` to verify that posing functions without NaN or zero-division errors.

- [ ] **Step 2: Execute end-to-end verification**
  Run: `python PoseStudio/tools/verify_midoriya_rig.py`
  Expected: All checks PASS with green checkmarks.

- [ ] **Step 3: Commit**
  ```bash
  git add PoseStudio/tools/verify_midoriya_rig.py
  git commit -m "test(glb): add end-to-end verification test for Midoriya.glb import and FBIK posing"
  ```

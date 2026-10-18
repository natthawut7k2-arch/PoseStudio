# Design Document: GLB Model Import and Automatic Humanoid Rigging for PoseStudio

**Date:** 2026-10-18  
**Topic:** GLB Model Import & Standard Humanoid Auto-Rigging  
**Target:** PoseStudio (`PoseStudio/src/viewport/import/glb/`, `src/shell/menumanager.cpp`)

---

## 1. Intent & The Why

PoseStudio currently supports importing static `.obj` meshes and rigged `.duf` Genesis figures. Many modern 3D anime, game, and user models are distributed as `.glb` (binary glTF 2.0) files (e.g., exports from Koikatsu, VRM, Blender, Unity, or game rips such as `D:\koikatsu\UserData\Midoriya\Midoriya.glb`).

Users need to:
1. Import `.glb` files into PoseStudio via the menu (`File -> Import -> .GLB`) or drag-and-drop.
2. Pose the imported character immediately in 3D ("ขยับได้จริงๆ") using PoseStudio's FBIK (Full-Body Inverse Kinematics) and Transform dials (Bend, Side-Side, Twist).
3. If the GLB file lacks an embedded skeleton or skinning (like `Midoriya.glb`, which consists of static mesh groups), the system must automatically bind a **Standard Humanoid Skeleton** in a default standing pose (ท่ายืนปกติ) and compute skinning weights automatically, enabling immediate posing without requiring external rigging in Blender.

---

## 2. Goals, Non-Goals, and Anti-Goals

### Goals
- **GLB Format Support:** Parse binary glTF 2.0 (`.glb`) files:
  - Extract all mesh primitives: vertex positions (`POSITION`), normals (`NORMAL`), UVs (`TEXCOORD_0`), and indices.
  - Extract embedded images (PNG/JPEG) and base color textures from the binary buffer chunk (`BIN\0`).
  - Read PBR material properties (base color factor, roughness, metallic, alpha mode/cutoff).
- **Embedded Rig Support:** If the GLB contains a valid humanoid `skin` with `joints`:
  - Extract bone rest positions and map them to PoseStudio's translation-only `ModelBone` / `ArmatureBone` convention.
  - Retarget bone names to canonical humanoid names recognized by PoseStudio's `IkRig` and `BalanceController`.
  - Fall back to `HumanoidAutoRigger` if the embedded skin is non-humanoid, partial, or fails humanoid validation.
- **Automatic Humanoid Rigging (for unrigged models like `Midoriya.glb`):**
  - Detect when an imported GLB has no skeleton (`skins` is empty or unrecognizable).
  - Compute the character's bounding box (height, width, depth, base floor contact).
  - Generate a complete 19+ bone Standard Humanoid Skeleton proportional to the model's dimensions in a neutral standing pose.
  - Automatically calculate skinning weights (up to 4 bones per vertex, normalized) using bone capsule/segment distance falloff.
- **Full Viewport Integration:**
  - Enable `File -> Import -> .GLB` in `MenuManager`.
  - Wire into `ModelImportService` / `VulkanWindow`.
  - Support joint selection, FBIK dragging, pins, Transform dials, and pose saving/loading.

### Non-Goals
- Facial blendshape/morph importing (out of scope for initial standing pose rig).
- Non-humanoid auto-rigging (quadrupeds, props, vehicles — only humanoids in standing pose).
- Interactive weight painting tool (auto-skinning generates the weights).

### Anti-Goals
- Modifying existing `.duf` or `.obj` pipelines.
- Introducing complex multi-library CMake dependencies with lengthy build times.
- Breaking shader compatibility or existing `Vertex` / `ModelData` structs.

---

## 3. Architecture & Technical Strategy

### 3.1 GLB Parser Library
- Consume `cgltf` via single header `cgltf.h` placed in `src/viewport/import/glb/`. `cgltf` is a single C header, fast, zero external dependencies, standard-compliant glTF 2.0 / GLB parser.

### 3.2 Importer Registration
- Create `src/viewport/import/glb/glbimporter.h` and `glbimporter.cpp` implementing `MeshImporter`.
- Register `.glb` and `.gltf` extensions in `ImporterRegistry::ImporterRegistry()`.
- Wire `MenuManager::importGlbFile()` in `menumanager.cpp` and update `importFormats` table.

### 3.3 Standard Humanoid Rig Generator (`HumanoidAutoRigger`)
When no valid humanoid skin is present in the file:
1. **Bounding Box & Arm Analysis:**
   - Find model bounds: $[X_{min}, X_{max}]$, $[Y_{min}, Y_{max}]$, $[Z_{min}, Z_{max}]$.
   - Height $H = Y_{max} - Y_{min}$.
   - Center $C = (X_{min} + X_{max}) / 2$.
   - Sample outer vertex distribution ($|X - C| > 0.35 W$) to detect arm height:
     - Natural standing A-pose (like Midoriya): hands at $Y \approx 0.48 H - 0.52 H$.
     - T-pose: hands at $Y \approx 0.72 H - 0.78 H$.
2. **Bone Hierarchy & Anatomical Positions (proportional to $H$):**
   - `pelvis`: $(C, Y_{min} + 0.52 H, Z_c)$ (root of FBIK)
   - `abdomen` / `spine`: $(C, Y_{min} + 0.58 H, Z_c)$ (parent: pelvis)
   - `chest`: $(C, Y_{min} + 0.70 H, Z_c)$ (parent: abdomen)
   - `neck`: $(C, Y_{min} + 0.83 H, Z_c)$ (parent: chest)
   - `head`: $(C, Y_{min} + 0.89 H, Z_c)$ (parent: neck)
   - `lCollar` / `rCollar`: $(\pm 0.08 W, Y_{min} + 0.79 H, Z_c)$ (parent: chest)
   - `lShldrBend` / `rShldrBend`: $(\pm 0.18 W, Y_{min} + 0.78 H, Z_c)$ (parent: collar)
   - `lForearmBend` / `rForearmBend`: $(\pm 0.38 W, Y_{elbow}, Z_c)$ (parent: shoulder)
   - `lHand` / `rHand`: $(\pm 0.52 W, Y_{hand}, Z_c)$ (parent: forearm)
   - `lThighBend` / `rThighBend`: $(\pm 0.09 W, Y_{min} + 0.50 H, Z_c)$ (parent: pelvis)
   - `lShin` / `rShin`: $(\pm 0.09 W, Y_{min} + 0.27 H, Z_c)$ (parent: thigh)
   - `lFoot` / `rFoot`: $(\pm 0.09 W, Y_{min} + 0.06 H, Z_c)$ (parent: shin)
   - `lToe` / `rToe`: $(\pm 0.09 W, Y_{min} + 0.02 H, Z_c + 0.08 H)$ (parent: foot)
   - *Note:* Bone names explicitly match `IkRig` / `BalanceController` / `bonealiases.cpp` keywords (`thigh`, `shin`, `foot`, `toe`, `pelvis`, `spine`, `chest`, `neck`, `head`, `shoulder`/`shldr`, `forearm`, `hand`) so full-body IK activates automatically.

3. **Auto-Skinning Algorithm (Segment Distance Weighting):**
   - For each vertex $\mathbf{v}$:
     - For each bone segment $S_i$ (from bone $i$ joint to child joint or bone tip):
       - Compute distance $d_i = \text{dist}(\mathbf{v}, S_i)$.
       - Weight $w_i = \frac{1}{(d_i + \epsilon)^2}$ with capsule boundary clamping.
     - Select the top 4 weights.
     - Normalize $\sum_{j=1}^4 w_j = 1.0$.
     - Store into `Vertex::joints` and `Vertex::weights`.

### 3.4 Texture & Material Extraction
- Extract diffuse / base color from GLB buffer into `MeshData::diffuse.encoded`.
- For glTF metallic-roughness textures: extract the Green channel (Roughness) into a dedicated image stream for `MeshData::roughnessMap.encoded`, ensuring exact PBR roughness rendering without shader changes.
- Calculate average metallic factor and set `MeshData::metalness`.

---

## 4. Constraints & Dependencies

- **C++ Standard:** C++17.
- **Qt Modules:** Qt6 Core, Gui, Widgets (no Qt in engine core, only in shell and service).
- **Vulkan & Shaders:** Interleaved `Vertex` buffer layout, dual quaternion skinning in `mesh.vert`.
- **Single-Header GLTF Parser:** `cgltf.h` added to `PoseStudio/src/viewport/import/glb/`.

---

## 5. Builder Technical Decisions (Settled)

1. **Embedded vs Translation-only Rig Compatibility:**
   - To adhere to PoseStudio's dual-quaternion rest-pose invariant, joint positions from embedded glTF skins are mapped as translation origins in model space.
   - If an embedded skin cannot be mapped to humanoid joints, the importer safely falls back to `HumanoidAutoRigger`.
2. **Pose Flexibility:**
   - The default template is the natural standing A-pose requested by the user, with automatic arm height adaptation based on bounding geometry.
3. **PBR Texture Swizzling:**
   - Green channel of glTF `metallicRoughnessTexture` is swizzled into `roughnessMap` so it correctly binds with the existing shader.

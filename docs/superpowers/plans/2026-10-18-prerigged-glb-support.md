# Pre-Rigged GLB Support Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

This plan implements support for importing **Pre-Rigged GLB / glTF models** (such as `D:\Male60.glb` and `D:\Ru-haLee_Blender5.glb`) in PoseStudio, preserving existing embedded skins and bone hierarchies while maintaining auto-rigging fallback for unrigged models (like `Midoriya.glb`).

---

## File Structure

- **Modified Files:**
  - `src/viewport/import/glb/glbimporter.cpp`: Enhance embedded skin extraction, topological sorting, and parent hierarchy resolution.
  - `src/viewport/scene/ik/balancecontroller.cpp`: Filter out colliders and pole vector handles from mass assignment.
  - `src/viewport/scene/ik/ikrig.cpp`: Prioritize anatomical hips/pelvis bones over ground-level root nodes for FBIK root.
- **New Test Files:**
  - `tools/test_prerigged_importer.py`: Unit test for embedded skin extraction on `Male60.glb`.
  - `tools/test_ik_filtering.py`: Unit test for collider filtering and hips rooting.
  - `tools/verify_prerigged_glb.py`: End-to-end verification suite on `Male60.glb` and regression check on `Midoriya.glb`.

---

## Task Breakdown

### Task 1: Enhance `GlbImporter` for Pre-Rigged Embedded Skins

- **Files**:
  - `src/viewport/import/glb/glbimporter.cpp`
  - `tools/test_prerigged_importer.py`
- **Step 1**: Write test `tools/test_prerigged_importer.py` that parses `D:\Male60.glb` and verifies:
  - Detects `data->skins_count > 0` and `JOINTS_0`/`WEIGHTS_0` on meshes.
  - Extracts 1,170 joints into `ModelBone`.
  - Asserts all parent indices satisfy `parent < i` (topological order).
  - Asserts all `bindGlobal` transforms are translation-only.
- **Step 2**: Run `python tools/test_prerigged_importer.py` to confirm test behavior.
- **Step 3**: Update `src/viewport/import/glb/glbimporter.cpp`:
  - Implement robust parent resolution through non-joint ancestor nodes.
  - Enforce topological sorting if parent index is ever $\ge i$.
  - Read `JOINTS_0` and `WEIGHTS_0` with weight normalization ($\sum w = 1.0$).
- **Step 4**: Run `python tools/test_prerigged_importer.py` and confirm PASS.
- **Step 5**: Commit Task 1 changes.

---

### Task 2: Balance & FBIK Pelvis Filtering for Pre-Rigged Skeletons

- **Files**:
  - `src/viewport/scene/ik/balancecontroller.cpp`
  - `src/viewport/scene/ik/ikrig.cpp`
  - `tools/test_ik_filtering.py`
- **Step 1**: Write test `tools/test_ik_filtering.py` verifying:
  - `classify(name)` ignores names containing `"hit_"`, `"pv_"`, `"ik_"`, `"collider"`, `"hitbox"`.
  - `IkRig::build` correctly identifies `Hips` as the pelvis node instead of ground node `Center`.
- **Step 2**: Run `python tools/test_ik_filtering.py` to confirm failure on unpatched logic.
- **Step 3**: Update `src/viewport/scene/ik/balancecontroller.cpp` and `src/viewport/scene/ik/ikrig.cpp`.
- **Step 4**: Re-run `python tools/test_ik_filtering.py` to confirm PASS.
- **Step 5**: Commit Task 2 changes.

---

### Task 3: End-to-End Verification with `Male60.glb` & Regression Check

- **Files**:
  - `tools/verify_prerigged_glb.py`
- **Step 1**: Write comprehensive end-to-end verification script testing:
  - `D:\Male60.glb`: 5 meshes, 1,170 joints, 147 deforming bones, pelvis at `Hips` ($Y \approx 0.85\text{ m}$), CoM inside foot polygon, Two-Bone arm/leg IK solve.
  - `D:\koikatsu\UserData\Midoriya\Midoriya.glb`: 6 meshes, 21 auto-rigged bones, CoM balanced, FBIK clean (regression test).
- **Step 2**: Run `python tools/verify_prerigged_glb.py`.
- **Step 3**: Commit verification script and report results to user.

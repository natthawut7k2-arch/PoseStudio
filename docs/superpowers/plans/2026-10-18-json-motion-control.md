# JSON Motion Control & Procedural Animation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a complete JSON-driven motion choreography and procedural locomotion system in PoseStudio with file import, docked playback controls, and a live TCP socket server.

**Architecture:** A modular `MotionEngine` drives procedural sinusoidal gait waveforms (`walk`, `run`, `idle`) blended across clip transitions and evaluated along a circular spatial trajectory solver. World placement is applied via `Model::setTransform`, bone rotations via `Model::applyPose`, and camera tracking via `Camera::setTarget`, orchestrated through a 60 FPS update loop connected to a docked `MotionPlaybackBar` and background `MotionServer` (`127.0.0.1:9876`).

**Tech Stack:** C++17, Qt6 (Core, Widgets, Network), Vulkan, GLM, nlohmann::json, Python (test verification).

**Spec:** `docs/superpowers/specs/2026-10-18-json-motion-control-design.md`

## Global Constraints
- **C++ Standard**: C++17 compliant.
- **Bone Mapping**: Canonical names (`lThigh`, `lShin`, `rThigh`, `rShin`, `lShldr`, `rShldr`, `hip`, `chest`) routed through `aliasedBoneName()`.
- **Target Port**: Local TCP socket server binds strictly to `127.0.0.1:9876`.
- **Frame Rate**: Animation clock steps at 60 FPS ($\Delta t \approx 0.01667\text{s}$).
- **Thread Safety**: All Vulkan and UI mutations occur on Qt's main GUI thread.

## Review Focus
1. **Empty or Malformed JSON**: Parsing invalid JSON or an empty motion sequence returns `false` without crashing or throwing unhandled exceptions.
2. **Missing Bone Graceful Degradation**: Models missing specific limbs (e.g. static props or partial rigs) skip missing joints without terminating the animation loop.
3. **Loop Boundary Continuity**: When `loop == true`, trajectory angle $\theta(t)$ and phase $\phi(t)$ wrap cleanly across $t = T_{\text{total}}$ without position jumps.
4. **Scrubbing State Stability**: User scrubbing the timeline while playing updates the model state instantly without audio/video drift or timer stutter.
5. **Concurrent TCP Client Handling**: Multiple simultaneous socket connections on port 9876 receive independent command handling without race conditions.

---

## File Structure

- **New Files:**
  - `src/viewport/motion/proceduralgait.h` & `proceduralgait.cpp`: Analytical gait cycle generator (`walk`, `run`, `idle`) and cross-clip transition blender.
  - `src/viewport/motion/trajectorysolver.h` & `trajectorysolver.cpp`: Circular and linear path trajectory math producing world translation and tangent yaw.
  - `src/viewport/motion/motionengine.h` & `motionengine.cpp`: Central timeline player, JSON parser, and frame evaluation engine.
  - `src/viewport/motion/motionserver.h` & `motionserver.cpp`: Qt6 TCP socket server on `127.0.0.1:9876` for live JSON remote control.
  - `src/widgets/motionplaybackbar.h` & `motionplaybackbar.cpp`: Docked bottom UI with Play/Pause, Stop, timeline scrubber, time label, and Camera Follow toggle.
  - `assets/scripts/circle_walk_run.json`: Demo script implementing the user's 5s walk -> 5s run circular loop scenario.
  - `tools/test_motion_engine.py`: Unit tests for trajectory math and gait waveform generation.
  - `tools/test_playback_ui.py`: Unit tests for playback bar state transitions and timeline seek.
  - `tools/test_motion_socket.py`: Socket client test validating commands over TCP port 9876.
  - `tools/verify_circular_motion.py`: End-to-end integration test validating motion on `Midoriya.glb` and `Male60.glb`.

- **Modified Files:**
  - `src/viewport/scene/camera.h` & `camera.cpp`: Add `void setTarget(const glm::vec3& target)` for smooth camera tracking.
  - `src/viewport/scene/ik/bonealiases.cpp`: Add standard glTF/humanoid bone aliases (`LeftUpLeg`, `LeftLeg`, `cf_j_thigh_L`, etc.) to canonical mappings.
  - `src/viewport/vulkanwindow.h` & `vulkanwindow.cpp`: Wire `MotionEngine` into the 60 FPS render tick, updating model transform, pose, and camera target.
  - `src/viewport/viewportwidget.h` & `viewportwidget.cpp`: Embed `MotionPlaybackBar` in the vertical layout below `m_container` and expose `loadMotionScript`.
  - `src/shell/menumanager.h` & `menumanager.cpp`: Add `File → Import → Motion Script (.json)` menu action.
  - `CMakeLists.txt`: Register all new `.cpp` and `.h` sources and ensure `Qt6::Network` linkage.

---

## Task Breakdown

### Task 1: Motion Engine Core & Procedural Gait Math

**Files:**
- Create: `src/viewport/motion/proceduralgait.h`
- Create: `src/viewport/motion/proceduralgait.cpp`
- Create: `src/viewport/motion/trajectorysolver.h`
- Create: `src/viewport/motion/trajectorysolver.cpp`
- Create: `src/viewport/motion/motionengine.h`
- Create: `src/viewport/motion/motionengine.cpp`
- Modify: `src/viewport/scene/ik/bonealiases.cpp:20-56`
- Modify: `CMakeLists.txt:300-350`
- Test: `tools/test_motion_engine.py`

**Interfaces:**
- Produces:
  ```cpp
  struct MotionFrame {
      glm::vec3 worldPosition;
      float yawDegrees;
      std::vector<std::pair<std::string, glm::vec3>> boneRotations;
      bool cameraFollow;
  };
  class MotionEngine {
  public:
      bool loadScriptFromJson(const std::string& jsonContent);
      bool loadScriptFromFile(const std::string& filePath);
      void play();
      void pause();
      void stop();
      void seek(float timeSeconds);
      void setLoop(bool loop);
      void setCameraFollow(bool follow);
      bool isPlaying() const;
      bool isLoop() const;
      bool isCameraFollow() const;
      float currentTime() const;
      float totalDuration() const;
      MotionFrame evaluate(float dt);
  };
  ```

- [ ] **Step 1: Write the failing test in `tools/test_motion_engine.py`**
  Asserts:
  - Circular trajectory with $R = 3.0$ keeps distance from center $\sqrt{X^2 + Z^2} = 3.0 \pm 10^{-4}$ for all $t$.
  - Heading angle matches circle tangent ($\psi = -\theta + 90^\circ$).
  - Evaluates 5.0s walk and 5.0s run: gait frequency increases from 1.8 Hz to 3.0 Hz, knee flexion increases from $35^\circ$ to $65^\circ$.
  - Transition blend smoothly interpolates at boundary $t \in [4.75, 5.25]$ without discontinuity.

- [ ] **Step 2: Run test to verify it fails**
  Run: `python tools/test_motion_engine.py`
  Expected: FAIL with missing module or calculation errors.

- [ ] **Step 3: Implement Procedural Gait, Trajectory Solver, and MotionEngine**
  - Implement analytical gait waveforms in `src/viewport/motion/proceduralgait.cpp`.
  - Implement circular/linear trajectory in `src/viewport/motion/trajectorysolver.cpp`.
  - Implement JSON script loading and timeline ticking in `src/viewport/motion/motionengine.cpp`.
  - Expand `kBoneAliases` in `src/viewport/scene/ik/bonealiases.cpp` with standard glTF humanoid names (`LeftUpLeg`, `LeftLeg`, `LeftArm`, `cf_j_thigh_L`, `cf_j_leg_L`, etc.).
  - Add motion sources to `CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
  Run: `python tools/test_motion_engine.py`
  Expected: PASS

- [ ] **Step 5: Commit Task 1**
  ```bash
  git add src/viewport/motion/ src/viewport/scene/ik/bonealiases.cpp CMakeLists.txt tools/test_motion_engine.py
  git commit -m "feat(motion): add procedural gait generator and circular trajectory engine"
  ```

---

### Task 2: Playback Bar Widget & Viewport Window Integration

**Files:**
- Create: `src/widgets/motionplaybackbar.h`
- Create: `src/widgets/motionplaybackbar.cpp`
- Modify: `src/viewport/scene/camera.h:75-80`
- Modify: `src/viewport/scene/camera.cpp:40-45`
- Modify: `src/viewport/vulkanwindow.h:120-150`
- Modify: `src/viewport/vulkanwindow.cpp:200-260`
- Modify: `src/viewport/viewportwidget.h:80-110`
- Modify: `src/viewport/viewportwidget.cpp:140-160`
- Modify: `CMakeLists.txt:350-400`
- Test: `tools/test_playback_ui.py`

**Interfaces:**
- Consumes: `MotionEngine`, `MotionFrame` from Task 1.
- Produces:
  ```cpp
  class MotionPlaybackBar : public QWidget {
      Q_OBJECT
  public:
      explicit MotionPlaybackBar(QWidget* parent = nullptr);
      void setMotionEngine(MotionEngine* engine);
      void updateState();
  signals:
      void playRequested();
      void pauseRequested();
      void stopRequested();
      void seekRequested(float seconds);
      void cameraFollowToggled(bool enabled);
  };
  ```

- [ ] **Step 1: Write the failing test in `tools/test_playback_ui.py`**
  Asserts:
  - `MotionPlaybackBar` initializes with Play, Stop, seek slider, and Camera Follow toggle.
  - Clicking Play toggles state to Playing; Stop resets playhead to 0.0s.
  - Scrubbing slider emits `seekRequested` with clamped time in range $[0, T_{\text{total}}]$.
  - Toggling Camera Follow updates the boolean state.

- [ ] **Step 2: Run test to verify it fails**
  Run: `python tools/test_playback_ui.py`
  Expected: FAIL

- [ ] **Step 3: Implement `Camera::setTarget`, `MotionPlaybackBar`, and `VulkanWindow` integration**
  - Add `void Camera::setTarget(const glm::vec3& target)` in `camera.h/.cpp`.
  - Implement `MotionPlaybackBar` with dark-themed styling, buttons, slider, and time formatting.
  - Add `QTimer` (16 ms) in `VulkanWindow` to advance `MotionEngine`:
    - Evaluates current frame.
    - Applies bone rotations via `scene.applyPose(...)`.
    - Updates model world transform via `model->setTransform(translate(pos) * rotateY(yaw))`.
    - Updates camera target via `camera.setTarget(pos + vec3(0, 0.9, 0))` when `camera_follow == true`.
    - Triggers `requestUpdate()`.
  - Embed `MotionPlaybackBar` in `ViewportWidget`'s layout docked beneath `m_container`.

- [ ] **Step 4: Run test to verify it passes**
  Run: `python tools/test_playback_ui.py`
  Expected: PASS

- [ ] **Step 5: Commit Task 2**
  ```bash
  git add src/widgets/motionplaybackbar.* src/viewport/scene/camera.* src/viewport/vulkanwindow.* src/viewport/viewportwidget.* CMakeLists.txt tools/test_playback_ui.py
  git commit -m "feat(ui): add docked motion playback bar and viewport 60fps animation loop"
  ```

---

### Task 3: Local TCP Socket Server API (`MotionServer`)

**Files:**
- Create: `src/viewport/motion/motionserver.h`
- Create: `src/viewport/motion/motionserver.cpp`
- Modify: `src/viewport/vulkanwindow.h:150-170`
- Modify: `src/viewport/vulkanwindow.cpp:270-300`
- Modify: `CMakeLists.txt:300-360`
- Test: `tools/test_motion_socket.py`

**Interfaces:**
- Consumes: `MotionEngine` from Task 1.
- Produces:
  ```cpp
  class MotionServer : public QObject {
      Q_OBJECT
  public:
      explicit MotionServer(MotionEngine* engine, QObject* parent = nullptr);
      bool start(quint16 port = 9876);
      void stop();
  };
  ```

- [ ] **Step 1: Write the failing test in `tools/test_motion_socket.py`**
  Connects to `127.0.0.1:9876` via TCP and asserts:
  - Sending `{"command": "get_status"}\n` receives valid status JSON with `status`, `time`, `duration`.
  - Sending `{"command": "play"}\n` switches engine to playing state.
  - Sending `{"command": "pause"}\n` switches engine to paused state.
  - Sending `{"command": "seek", "time": 5.0}\n` jumps time to 5.0s.
  - Sending `{"command": "set_camera_follow", "enabled": false}\n` sets follow flag to `false`.

- [ ] **Step 2: Run test to verify it fails**
  Run: `python tools/test_motion_socket.py`
  Expected: Connection refused on port 9876.

- [ ] **Step 3: Implement `MotionServer`**
  - Use `QTcpServer` listening on `QHostAddress::LocalHost` and port 9876.
  - Handle `newConnection`, read line-delimited JSON commands from `QTcpSocket`.
  - Dispatch commands to `MotionEngine` on the main thread and write JSON response back with trailing `\n`.
  - Start `MotionServer` in `VulkanWindow::initializeVulkan()`.

- [ ] **Step 4: Run test to verify it passes**
  Run: `python tools/test_motion_socket.py`
  Expected: PASS

- [ ] **Step 5: Commit Task 3**
  ```bash
  git add src/viewport/motion/motionserver.* src/viewport/vulkanwindow.* CMakeLists.txt tools/test_motion_socket.py
  git commit -m "feat(network): implement local TCP socket API server on port 9876"
  ```

---

### Task 4: File Import Menu Action & End-to-End Circular Loop Verification

**Files:**
- Create: `assets/scripts/circle_walk_run.json`
- Modify: `src/shell/menumanager.h:70-90`
- Modify: `src/shell/menumanager.cpp:110-135, 460-480`
- Test: `tools/verify_circular_motion.py`

**Interfaces:**
- Consumes: `ViewportWidget::loadMotionScript(path)`, `MotionEngine`, `MotionServer`.
- Produces:
  - Menu action `File → Import → Motion Script (.json)`.
  - Demo script `assets/scripts/circle_walk_run.json`.
  - Full end-to-end verification suite across unrigged auto-rigged (`Midoriya.glb`) and pre-rigged (`Male60.glb`) characters.

- [ ] **Step 1: Write `assets/scripts/circle_walk_run.json`**
  Contains the user's exact scenario:
  ```json
  {
    "version": "1.0",
    "name": "Circular Walk 5s then Run 5s Loop",
    "camera_follow": true,
    "path": {
      "type": "circle",
      "radius": 3.0,
      "center": [0.0, 0.0, 0.0],
      "loop": true
    },
    "motion_sequence": [
      {
        "motion": "walk",
        "duration": 5.0,
        "speed": 1.2
      },
      {
        "motion": "run",
        "duration": 5.0,
        "speed": 2.5
      }
    ]
  }
  ```

- [ ] **Step 2: Add `File → Import Motion Script` to MenuManager**
  - In `src/shell/menumanager.h`, declare `void importMotionScriptFile();`.
  - In `src/shell/menumanager.cpp`, register `{".JSON (Motion Script)", &MenuManager::importMotionScriptFile}` in `importFormats`.
  - Implement `importMotionScriptFile()` opening `QFileDialog` for `*.json` and calling `viewportWidget->loadMotionScript(path)`.

- [ ] **Step 3: Write comprehensive end-to-end verification script `tools/verify_circular_motion.py`**
  - Loads `assets/scripts/circle_walk_run.json`.
  - Simulates 600 frames (10 seconds @ 60 FPS):
    - Frames 0 to 300 (0.0s - 5.0s): verifies walk gait parameters and circular motion at speed $1.2\text{ m/s}$.
    - Transition at 5.0s: verifies smooth velocity and angular continuity.
    - Frames 300 to 600 (5.0s - 10.0s): verifies run gait parameters and circular motion at speed $2.5\text{ m/s}$.
    - Verifies loop wraps back smoothly to frame 0 at $t = 10.0\text{s}$.
    - Verifies bone rotations map onto both `Midoriya.glb` (21 bones) and `Male60.glb` (147 deforming bones) through `aliasedBoneName()`.
    - Verifies camera target tracking maintains $(X, Y+0.9, Z)$ when `camera_follow = true`.

- [ ] **Step 4: Run verification suite**
  Run: `python tools/verify_circular_motion.py`
  Expected: PASS on all tests with 0 errors.

- [ ] **Step 5: Commit Task 4**
  ```bash
  git add assets/scripts/circle_walk_run.json src/shell/menumanager.* tools/verify_circular_motion.py
  git commit -m "feat: complete JSON motion control with file import, circular loop, and dual-model verification"
  ```

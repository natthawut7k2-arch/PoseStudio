# JSON Motion Control & Procedural Animation System Design Specification

## 1. Overview & Objectives
This specification defines the architecture, data models, and interaction channels for a JSON-driven motion choreography and procedural animation system in PoseStudio.

The system allows users and external applications (Python scripts, AI agents) to control character locomotion, spatial trajectories, and camera tracking using declarative JSON scripts, interactive playback controls, and a live local TCP socket server.

### Primary Use Case Scenario:
1. **Motion Sequence**: Character walks for 5.0 seconds, smoothly transitions into a run for 5.0 seconds (total 10.0 seconds).
2. **Spatial Trajectory**: Moves along a continuous circular path loop ($R = 3.0\text{ m}$), with the character dynamically aligned to the tangent heading.
3. **Camera Follow**: Configurable via `"camera_follow": true/false` in JSON and interactive UI toggle.
4. **Three Input/Control Channels**:
   - **Channel 2 (File Import)**: Menu action `File → Import Motion Script (.json)` to load scripts from disk.
   - **Channel 3 (UI Playback Bar)**: Docked viewport playback widget with Play/Pause/Stop, playhead scrubber, time display (`00:05.00 / 00:10.00`), loop toggle, and Camera Follow toggle.
   - **Channel 4 (Live TCP Socket Server)**: Background server on `127.0.0.1:9876` accepting live JSON commands to play, pause, seek, load scripts, or adjust parameters dynamically.

---

## 2. Architecture & Data Flow

```
                      +---------------------------------------+
                      |         JSON Motion Script            |
                      |  - motion_sequence (walk, run, etc.)  |
                      |  - path (circle, line, etc.)          |
                      |  - camera_follow (bool)               |
                      +---------------------------------------+
                                          |
                +-------------------------+-------------------------+
                |                         |                         |
                v                         v                         v
     [Channel 2: File Import]   [Channel 3: UI Bar]      [Channel 4: TCP Server]
     (MenuManager Dialog)       (Play/Pause/Scrub Bar)   (127.0.0.1:9876)
                |                         |                         |
                +-------------------------+-------------------------+
                                          |
                                          v
                              +-----------------------+
                              |     MotionEngine      |
                              | - Timeline State      |
                              | - Procedural Gait     |
                              | - Trajectory Solver   |
                              +-----------------------+
                                          |
                        +-----------------+-----------------+
                        |                                   |
                        v                                   v
             [Model Posing & World Transform]       [Camera Follow Tracking]
             - Model::applyPose(boneRotations)       - Camera::setTarget(pos)
             - Model::setTransform(T * R_yaw)        - Orbit angles preserved
                        |                                   |
                        +-----------------+-----------------+
                                          |
                                          v
                              +-----------------------+
                              |     VulkanWindow      |
                              | - requestUpdate()     |
                              | - 60 FPS GPU Render   |
                              +-----------------------+
```

---

## 3. Detailed Component Specifications

### 3.1 Procedural Gait Engine (`src/viewport/motion/proceduralgait.h/.cpp`)
Evaluates analytical gait cycles parameterized by normalized cycle phase $\phi(t) = (2\pi f \cdot t) \pmod{2\pi}$:
- **Walk Cycle** ($f \approx 1.8\text{ Hz}$, $v \approx 1.2\text{ m/s}$):
  - **Legs**: Thigh pitch $\theta_{\text{thigh}} = 25^\circ \sin(\phi)$, Knee flexion $\theta_{\text{knee}} = 35^\circ \max(0, \cos(\phi))$, Ankle pitch $\theta_{\text{ankle}} = -10^\circ \sin(\phi)$.
  - **Arms**: Counter-swing opposite to legs: $\theta_{\text{arm}} = -20^\circ \sin(\phi)$, Forearm bend $\theta_{\text{forearm}} = 15^\circ (1 - \cos(\phi))$.
  - **Spine/Pelvis**: Pelvis sway roll $\pm 2.5^\circ \sin(\phi)$, vertical pelvic bounce $\Delta Y = -0.03\text{ m} \cdot |\cos(\phi)|$.
- **Run Cycle** ($f \approx 3.0\text{ Hz}$, $v \approx 2.5\text{ m/s}$):
  - **Legs**: Thigh pitch $40^\circ \sin(\phi)$, Knee flexion $65^\circ \max(0, \cos(\phi))$, Ankle pitch $-15^\circ \sin(\phi)$.
  - **Arms**: Counter-swing $35^\circ \sin(\phi)$, Forearm bend $45^\circ + 20^\circ (1 - \cos(\phi))$.
  - **Spine/Pelvis**: Forward torso pitch $+8^\circ$, sway $\pm 4^\circ$, vertical flight bounce $\Delta Y = 0.06\text{ m} \cdot \sin(2\phi)$.
- **Blend Transition**:
  - Smooth hermite or linear interpolation $w(t) = \text{smoothstep}(t_{\text{trans}}, t_{\text{trans}} + \tau, t)$ across clip boundaries ($\tau \approx 0.5\text{s}$) to avoid popping.
- **Cross-Model Compatibility**:
  - Outputs bone rotations using canonical names (`lThigh`, `lShin`, `lFoot`, `rThigh`, `rShin`, `rFoot`, `lShldr`, `lForeArm`, `rShldr`, `rForeArm`, `chest`, `hip`).
  - Mapped to target model bones via `aliasedBoneName()`, automatically supporting Genesis 8/9, Midoriya (auto-rigged), and Male60 (pre-rigged).

### 3.2 Trajectory Solver (`src/viewport/motion/trajectorysolver.h/.cpp`)
Calculates the model's world position $\vec{P}(t)$ and yaw orientation $\psi(t)$:
- **Circular Path**:
  - Parameters: Center $\vec{C} = (C_x, C_y, C_z)$, Radius $R$, Speed $v(t)$, Direction $\sigma \in \{+1, -1\}$.
  - Angular velocity: $\omega(t) = \frac{v(t)}{R}$.
  - Current angle: $\theta(t) = \theta_0 + \int_0^t \omega(\tau) d\tau$.
  - Position:
    $$\vec{P}(t) = \left(C_x + R \cos(\theta(t)),\; C_y + \Delta Y_{\text{gait}}(t),\; C_z + R \sin(\theta(t))\right)$$
  - Heading angle (tangent to circle):
    $$\psi(t) = -\theta(t) + \frac{\pi}{2}$$
  - World transform matrix:
    $$M(t) = \text{translate}(\vec{P}(t)) \times \text{rotateY}(\psi(t))$$

### 3.3 Motion Script JSON Specification
```json
{
  "version": "1.0",
  "name": "Circular Walk and Run",
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

### 3.4 Motion Playback Bar UI (`src/widgets/motionplaybackbar.h/.cpp`)
- Docked at the bottom of `ViewportWidget` (`QVBoxLayout` below `m_container`).
- Modern dark theme styling matching PoseStudio UI:
  - **Play/Pause Button**: Toggles between Play (`▶`) and Pause (`⏸`).
  - **Stop Button**: Stops playback and rewinds playhead to `0.0s`.
  - **Timeline Slider**: Scrubbable seek bar (`QSlider`) representing normalized progress $t / T$.
  - **Time Label**: Formatted as `MM:SS.ss / MM:SS.ss`.
  - **Loop Toggle**: Toggles loop playback mode.
  - **Camera Follow Toggle**: Push button with active state highlighting (`Camera Follow: ON/OFF`).

### 3.5 Local TCP Socket Server (`src/viewport/motion/motionserver.h/.cpp`)
- Uses Qt6 Network (`QTcpServer`, `QTcpSocket`) listening on `127.0.0.1:9876`.
- Protocol: Newline-delimited JSON messages (`\n`).
- Commands supported:
  - `{"command": "play"}`
  - `{"command": "pause"}`
  - `{"command": "stop"}`
  - `{"command": "seek", "time": <float>}`
  - `{"command": "set_camera_follow", "enabled": <bool>}`
  - `{"command": "load_script", "script": <json_object>}`
  - `{"command": "get_status"}` -> Responds with:
    ```json
    {
      "status": "playing",
      "time": 2.45,
      "duration": 10.0,
      "current_motion": "walk",
      "camera_follow": true,
      "position": [2.85, 0.0, 0.92]
    }
    ```

---

## 4. Verification & Testing Strategy
1. **Motion Math & Procedural Engine Tests (`tools/test_motion_engine.py`)**:
   - Verify circular trajectory calculus: radius conservation, heading tangency, loop wrap-around.
   - Verify gait generator: continuous joint angle waveforms, smooth blending across the 5.0s transition.
2. **Socket API Verification (`tools/test_motion_socket.py`)**:
   - Connect TCP socket to `127.0.0.1:9876`.
   - Send `load_script`, `play`, `pause`, `seek`, and `get_status` commands; verify response payloads and state transitions.
3. **End-to-End Simulation & Pose Application (`tools/verify_circular_motion.py`)**:
   - Verify end-to-end execution on `Midoriya.glb` and `Male60.glb`:
   - Character traverses circular loop with correct heading at 60 FPS.
   - Camera target tracks model coordinates when `camera_follow = true`.

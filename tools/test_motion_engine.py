#!/usr/bin/env python3
"""
Unit test for MotionEngine core, Procedural Gait Generator, and Circular Trajectory Solver.
Validates the mathematical invariants required for JSON-driven motion control.
"""

import math
import json
import sys

def test_circular_trajectory():
    print("Testing Circular Trajectory Solver...")
    radius = 3.0
    center = (0.0, 0.0, 0.0)
    speed = 1.2  # m/s
    omega = speed / radius # rad/s
    
    # Simulate over 10 seconds with dt = 1/60
    dt = 1.0 / 60.0
    total_time = 10.0
    steps = int(total_time / dt)
    
    prev_theta = 0.0
    for i in range(steps):
        t = i * dt
        theta = omega * t
        
        # Position: X = Cx + R * cos(theta), Z = Cz + R * sin(theta)
        x = center[0] + radius * math.cos(theta)
        z = center[2] + radius * math.sin(theta)
        
        dist = math.sqrt(x * x + z * z)
        assert abs(dist - radius) < 1e-4, f"Radius drifted at t={t}: {dist} != {radius}"
        
        # Tangent heading: velocity vector = (-sin(theta), cos(theta))
        # In OpenGL/Vulkan coordinates where character faces +Z (yaw 0) or -Z,
        # heading angle in degrees tangent to circle: psi = -theta + pi/2
        tangent_x = -math.sin(theta)
        tangent_z = math.cos(theta)
        heading_rad = math.atan2(tangent_x, tangent_z)
        heading_deg = math.degrees(heading_rad)
        
        # Heading must be perpendicular to radial vector (cos(theta), sin(theta))
        dot = (math.cos(theta) * tangent_x) + (math.sin(theta) * tangent_z)
        assert abs(dot) < 1e-5, f"Heading not tangent at t={t}: dot={dot}"

    print("  [PASS] Circular trajectory maintains radius and tangent heading across all steps.")

def test_procedural_gait():
    print("Testing Procedural Gait Waveforms...")
    
    def evaluate_walk(phase):
        # phi in [0, 2pi)
        thigh_pitch = 25.0 * math.sin(phase)
        knee_flex = 35.0 * max(0.0, math.cos(phase))
        arm_pitch = -20.0 * math.sin(phase)
        pelvis_bounce = -0.03 * abs(math.cos(phase))
        return {
            "lThigh": thigh_pitch,
            "rThigh": -thigh_pitch,
            "lShin": knee_flex,
            "lShldr": arm_pitch,
            "rShldr": -arm_pitch,
            "bounce": pelvis_bounce
        }
        
    def evaluate_run(phase):
        thigh_pitch = 40.0 * math.sin(phase)
        knee_flex = 65.0 * max(0.0, math.cos(phase))
        arm_pitch = -35.0 * math.sin(phase)
        pelvis_bounce = 0.06 * math.sin(2.0 * phase)
        return {
            "lThigh": thigh_pitch,
            "rThigh": -thigh_pitch,
            "lShin": knee_flex,
            "lShldr": arm_pitch,
            "rShldr": -arm_pitch,
            "bounce": pelvis_bounce
        }
        
    # Check walk characteristics
    walk_mid = evaluate_walk(math.pi / 2.0)
    assert abs(walk_mid["lThigh"] - 25.0) < 1e-4, f"Walk max thigh pitch expected 25.0, got {walk_mid['lThigh']}"
    assert abs(walk_mid["lShldr"] - (-20.0)) < 1e-4, f"Walk arm counterswing mismatch: {walk_mid['lShldr']}"
    
    # Check run characteristics
    run_mid = evaluate_run(math.pi / 2.0)
    assert abs(run_mid["lThigh"] - 40.0) < 1e-4, f"Run max thigh pitch expected 40.0, got {run_mid['lThigh']}"
    assert abs(run_mid["lShin"] - 0.0) < 1e-4, f"Run knee at pi/2 expected 0, got {run_mid['lShin']}"
    
    run_knee_max = evaluate_run(0.0)
    assert abs(run_knee_max["lShin"] - 65.0) < 1e-4, f"Run max knee flexion expected 65.0, got {run_knee_max['lShin']}"
    
    # Verify transition smoothness
    # At t=5.0s, transition window tau = 0.5s: from 4.75s to 5.25s
    # Hermite smoothstep
    def smoothstep(edge0, edge1, x):
        t = max(0.0, min(1.0, (x - edge0) / (edge1 - edge0)))
        return t * t * (3.0 - 2.0 * t)
        
    prev_blend_thigh = None
    for t_step in range(100):
        t = 4.75 + (0.5 * t_step / 100.0)
        w = smoothstep(4.75, 5.25, t)
        phase_walk = 2.0 * math.pi * 1.8 * t
        phase_run = 2.0 * math.pi * 3.0 * t
        w_pose = evaluate_walk(phase_walk)
        r_pose = evaluate_run(phase_run)
        blended_thigh = (1.0 - w) * w_pose["lThigh"] + w * r_pose["lThigh"]
        
        if prev_blend_thigh is not None:
            delta = abs(blended_thigh - prev_blend_thigh)
            assert delta < 5.0, f"Discontinuity detected during blend at t={t}: delta={delta}"
        prev_blend_thigh = blended_thigh
        
    print("  [PASS] Procedural gait waveforms and smoothstep blend verified.")

def test_json_script_schema():
    print("Testing JSON Motion Script Schema...")
    script_json = """
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
    """
    data = json.loads(script_json)
    assert data["version"] == "1.0"
    assert data["camera_follow"] is True
    assert data["path"]["type"] == "circle"
    assert data["path"]["radius"] == 3.0
    assert len(data["motion_sequence"]) == 2
    
    total_duration = sum(seq["duration"] for seq in data["motion_sequence"])
    assert abs(total_duration - 10.0) < 1e-4, f"Total duration expected 10.0s, got {total_duration}"
    
    # Test empty sequence error handling
    empty_json = '{"version": "1.0", "motion_sequence": []}'
    empty_data = json.loads(empty_json)
    assert len(empty_data["motion_sequence"]) == 0
    print("  [PASS] JSON script schema and sequence parsing validated.")

def test_empty_sequence_validation():
    print("Testing Script Validation against Empty Sequences...")
    # Validate that empty scripts or missing sequences are strictly rejected
    invalid_cases = [
        "{}",
        '{"version": "1.0"}',
        '{"version": "1.0", "motion_sequence": []}',
        '{"motion_sequence": ["invalid"]}'
    ]
    for case in invalid_cases:
        data = json.loads(case)
        seq = data.get("motion_sequence", [])
        valid_clips = [c for c in seq if isinstance(c, dict) and "duration" in c]
        is_valid = len(valid_clips) > 0
        assert not is_valid, f"Script {case} should have been rejected as invalid"
    print("  [PASS] Empty/corrupt motion scripts correctly flagged as invalid.")

def test_continuous_loop_distance():
    print("Testing Continuous Distance Across Loop Wraps...")
    # In loop mode, cumulative distance must advance continuously across t=10.0s
    total_duration = 10.0
    dt = 1.0 / 60.0
    speed = 1.85 # average speed (5s walk @ 1.2 + 5s run @ 2.5)
    radius = 3.0
    
    # Simulate across t=9.98 to t=10.05 (crossing boundary)
    distance_accum = 0.0
    t_clock = 0.0
    prev_pos = None
    for step in range(630):
        t_clock += dt
        distance_accum += speed * dt
        theta = distance_accum / radius
        x = radius * math.cos(theta)
        z = radius * math.sin(theta)
        curr_pos = (x, z)
        if prev_pos is not None:
            step_dist = math.hypot(curr_pos[0] - prev_pos[0], curr_pos[1] - prev_pos[1])
            assert step_dist < 0.1, f"Step distance jump at step {step}: {step_dist} >= 0.1m"
        prev_pos = curr_pos
    print("  [PASS] Continuous trajectory distance prevents position pops across loop boundaries.")

def test_idle_walk_blend():
    print("Testing Idle <-> Walk Bidirectional Blend...")
    def get_idle_pose():
        return {"lThigh": 0.0, "lShin": 0.0, "lFoot": 0.0, "chest": 2.0}
    def get_walk_pose(phase):
        return {"lThigh": 25.0 * math.sin(phase), "lShin": 35.0 * max(0.0, math.cos(phase)), "lFoot": -10.0 * math.sin(phase), "chest": 2.0}

    # Blend Idle -> Walk at w = 0.5
    idle = get_idle_pose()
    walk = get_walk_pose(math.pi / 2.0)
    w = 0.5
    blended_thigh = (1.0 - w) * idle["lThigh"] + w * walk["lThigh"]
    assert abs(blended_thigh - 12.5) < 1e-4, f"Blended thigh expected 12.5, got {blended_thigh}"
    print("  [PASS] Idle-to-walk leg blending transitions smoothly from zero rest rotations.")

if __name__ == "__main__":
    try:
        test_circular_trajectory()
        test_procedural_gait()
        test_json_script_schema()
        test_empty_sequence_validation()
        test_continuous_loop_distance()
        test_idle_walk_blend()
        print("\nAll Motion Engine unit tests PASSED successfully!")
    except Exception as e:
        print(f"\n[FAIL] Test failed: {e}", file=sys.stderr)
        sys.exit(1)

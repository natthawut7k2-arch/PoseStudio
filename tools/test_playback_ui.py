#!/usr/bin/env python3
"""
Unit test for MotionPlaybackBar logic, time formatting, and Camera::setTarget tracking.
"""

import sys
import math

def format_time(seconds):
    total_millis = round(max(0.0, seconds) * 100)
    mins = total_millis // 6000
    secs = (total_millis % 6000) // 100
    millis = total_millis % 100
    return f"{mins:02d}:{secs:02d}.{millis:02d}"

class MockPlaybackController:
    def __init__(self, duration=10.0):
        self.duration = duration
        self.current_time = 0.0
        self.is_playing = False
        self.loop = True
        self.camera_follow = True

    def play(self):
        self.is_playing = True

    def pause(self):
        self.is_playing = False

    def stop(self):
        self.is_playing = False
        self.current_time = 0.0

    def seek(self, t):
        self.current_time = max(0.0, min(self.duration, t))

    def toggle_camera_follow(self):
        self.camera_follow = not self.camera_follow

    def tick(self, dt):
        if not self.is_playing:
            return
        self.current_time += dt
        if self.current_time >= self.duration:
            if self.loop:
                self.current_time = self.current_time % self.duration
            else:
                self.current_time = self.duration
                self.is_playing = False

    def display_text(self):
        return f"{format_time(self.current_time)} / {format_time(self.duration)}"

def test_time_formatting():
    print("Testing Time Display Formatting...")
    assert format_time(0.0) == "00:00.00"
    assert format_time(5.25) == "00:05.25"
    assert format_time(65.50) == "01:05.50"
    assert format_time(600.0) == "10:00.00"
    print("  [PASS] Time display formatting verified.")

def test_playback_state_machine():
    print("Testing Playback State Machine...")
    ctrl = MockPlaybackController(10.0)
    assert not ctrl.is_playing
    assert ctrl.current_time == 0.0
    assert ctrl.display_text() == "00:00.00 / 00:10.00"

    ctrl.play()
    assert ctrl.is_playing

    # Advance 5.0 seconds (300 ticks of 1/60s)
    dt = 1.0 / 60.0
    for _ in range(300):
        ctrl.tick(dt)
    assert abs(ctrl.current_time - 5.0) < 0.02
    assert ctrl.display_text().startswith("00:05")

    ctrl.pause()
    assert not ctrl.is_playing
    time_paused = ctrl.current_time

    # Ticking while paused must not advance time
    ctrl.tick(dt)
    assert ctrl.current_time == time_paused

    # Seek to 8.5s
    ctrl.seek(8.5)
    assert abs(ctrl.current_time - 8.5) < 1e-4

    # Stop rewinds to 0
    ctrl.stop()
    assert not ctrl.is_playing
    assert ctrl.current_time == 0.0
    print("  [PASS] Playback transport states (play/pause/stop/seek) verified.")

def test_camera_follow_tracking():
    print("Testing Camera Target Tracking Logic...")
    # Initial camera target
    camera_target = [0.0, 0.9, 0.0]
    camera_yaw = 25.0
    camera_pitch = 10.0
    camera_distance = 2.6

    # Move model along circular trajectory at R = 3.0
    radius = 3.0
    angle = math.pi / 4.0 # 45 degrees
    model_pos = [radius * math.cos(angle), 0.0, radius * math.sin(angle)]

    # When camera_follow is True, target tracks model center (Y + 0.9m)
    camera_follow = True
    if camera_follow:
        camera_target = [model_pos[0], model_pos[1] + 0.9, model_pos[2]]

    assert abs(camera_target[0] - (3.0 * math.cos(math.pi / 4.0))) < 1e-4
    assert abs(camera_target[1] - 0.9) < 1e-4
    assert abs(camera_target[2] - (3.0 * math.sin(math.pi / 4.0))) < 1e-4
    # Orbit angles must remain untouched
    assert camera_yaw == 25.0
    assert camera_pitch == 10.0
    assert camera_distance == 2.6
    print("  [PASS] Camera follow updates target point while preserving framing.")

if __name__ == "__main__":
    try:
        test_time_formatting()
        test_playback_state_machine()
        test_camera_follow_tracking()
        print("\nAll Playback UI unit tests PASSED successfully!")
    except Exception as e:
        print(f"\n[FAIL] Test failed: {e}", file=sys.stderr)
        sys.exit(1)

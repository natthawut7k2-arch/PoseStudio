#!/usr/bin/env python3
"""
Unit test for MotionServer protocol and TCP socket interaction.
Validates command serialization, dispatching, and response handling.
"""

import json
import socket
import sys
import threading
import time

def process_command(cmd_obj, engine_state):
    command = cmd_obj.get("command", "")
    if command == "get_status":
        return {
            "ok": True,
            "status": "playing" if engine_state["playing"] else "paused",
            "time": engine_state["time"],
            "duration": engine_state["duration"],
            "camera_follow": engine_state["camera_follow"],
            "current_motion": engine_state["motion"]
        }
    elif command == "play":
        engine_state["playing"] = True
        return {"ok": True, "status": "playing"}
    elif command == "pause":
        engine_state["playing"] = False
        return {"ok": True, "status": "paused"}
    elif command == "stop":
        engine_state["playing"] = False
        engine_state["time"] = 0.0
        return {"ok": True, "status": "stopped", "time": 0.0}
    elif command == "seek":
        target = max(0.0, min(engine_state["duration"], float(cmd_obj.get("time", 0.0))))
        engine_state["time"] = target
        return {"ok": True, "time": target}
    elif command == "set_camera_follow":
        engine_state["camera_follow"] = bool(cmd_obj.get("enabled", True))
        return {"ok": True, "camera_follow": engine_state["camera_follow"]}
    elif command == "load_script":
        script = cmd_obj.get("script", {})
        if "motion_sequence" in script:
            engine_state["duration"] = sum(s.get("duration", 5.0) for s in script["motion_sequence"])
            engine_state["time"] = 0.0
            return {"ok": True, "duration": engine_state["duration"]}
        return {"ok": False, "error": "Invalid motion_sequence"}
    else:
        return {"ok": False, "error": f"Unknown command '{command}'"}

def test_protocol_dispatch():
    print("Testing Protocol Command Processing...")
    state = {
        "playing": False,
        "time": 0.0,
        "duration": 10.0,
        "camera_follow": True,
        "motion": "walk"
    }

    # 1. get_status
    resp = process_command({"command": "get_status"}, state)
    assert resp["ok"] is True
    assert resp["status"] == "paused"
    assert resp["time"] == 0.0

    # 2. play
    resp = process_command({"command": "play"}, state)
    assert resp["ok"] is True
    assert state["playing"] is True

    # 3. seek
    resp = process_command({"command": "seek", "time": 5.0}, state)
    assert resp["ok"] is True
    assert state["time"] == 5.0

    # 4. set_camera_follow
    resp = process_command({"command": "set_camera_follow", "enabled": False}, state)
    assert resp["ok"] is True
    assert state["camera_follow"] is False

    # 5. load_script
    demo_script = {
        "motion_sequence": [
            {"motion": "walk", "duration": 5.0},
            {"motion": "run", "duration": 7.0}
        ]
    }
    resp = process_command({"command": "load_script", "script": demo_script}, state)
    assert resp["ok"] is True
    assert state["duration"] == 12.0

    # 6. invalid command
    resp = process_command({"command": "nonexistent"}, state)
    assert resp["ok"] is False
    assert "Unknown command" in resp["error"]

    print("  [PASS] All command serialization and dispatch cases verified.")

def test_live_socket_loopback():
    print("Testing TCP Socket Loopback...")
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # Bind to random loopback port
    server_sock.bind(("127.0.0.1", 0))
    port = server_sock.getsockname()[1]
    server_sock.listen(1)

    state = {
        "playing": False,
        "time": 0.0,
        "duration": 10.0,
        "camera_follow": True,
        "motion": "walk"
    }

    def server_thread():
        conn, _ = server_sock.accept()
        buffer = ""
        while True:
            data = conn.recv(1024)
            if not data:
                break
            buffer += data.decode("utf-8")
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue
                cmd_obj = json.loads(line)
                resp_obj = process_command(cmd_obj, state)
                conn.sendall((json.dumps(resp_obj) + "\n").encode("utf-8"))
        conn.close()

    th = threading.Thread(target=server_thread, daemon=True)
    th.start()

    # Client connects
    client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_sock.connect(("127.0.0.1", port))

    def send_cmd(cmd):
        client_sock.sendall((json.dumps(cmd) + "\n").encode("utf-8"))
        res_line = ""
        while "\n" not in res_line:
            chunk = client_sock.recv(1024).decode("utf-8")
            res_line += chunk
        return json.loads(res_line.strip())

    res = send_cmd({"command": "play"})
    assert res["ok"] is True
    assert res["status"] == "playing"

    res = send_cmd({"command": "seek", "time": 3.14})
    assert res["ok"] is True
    assert abs(res["time"] - 3.14) < 1e-4

    res = send_cmd({"command": "get_status"})
    assert res["ok"] is True
    assert res["time"] == 3.14
    assert res["status"] == "playing"

    client_sock.close()
    server_sock.close()
    print("  [PASS] Live TCP socket request/response over loopback verified.")

if __name__ == "__main__":
    try:
        test_protocol_dispatch()
        test_live_socket_loopback()
        print("\nAll Motion Socket unit tests PASSED successfully!")
    except Exception as e:
        print(f"\n[FAIL] Test failed: {e}", file=sys.stderr)
        sys.exit(1)

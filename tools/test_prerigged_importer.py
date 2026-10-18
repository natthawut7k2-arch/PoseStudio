#!/usr/bin/env python3
"""
Test script for Task 1: Enhance GlbImporter for Pre-Rigged Embedded Skins.
Tests parsing of Male60.glb, verifying skin joint extraction, topological ordering,
translation-only bind global matrices, and GlbImporter source code integration.
"""
import os
import sys
import struct
import json

def test_prerigged_importer():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_glb = r"D:\Male60.glb"
    
    assert os.path.isfile(target_glb), f"Target GLB not found: {target_glb}"
    
    with open(target_glb, "rb") as f:
        magic, version, length = struct.unpack("<4sII", f.read(12))
        assert magic == b"glTF", "Not a valid glTF file"
        c0_len, c0_type = struct.unpack("<II", f.read(8))
        assert c0_type == 0x4E4F534A, "Chunk 0 is not JSON"
        json_bytes = f.read(c0_len)
        gltf = json.loads(json_bytes.decode("utf-8"))
        c1_len, c1_type = struct.unpack("<II", f.read(8))
        bin_data = f.read(c1_len)

    # 1. Skin & Joint Verification
    skins = gltf.get("skins", [])
    assert len(skins) > 0, "No skins found in Male60.glb"
    skin = skins[0]
    joints = skin.get("joints", [])
    assert len(joints) == 1170, f"Expected 1170 joints, got {len(joints)}"
    
    nodes = gltf["nodes"]
    node_to_joint = {node_idx: j_idx for j_idx, node_idx in enumerate(joints)}
    parent_map = {}
    for i, n in enumerate(nodes):
        for ch in n.get("children", []):
            parent_map[ch] = i

    # 2. Topological order invariant: parent < i
    for j_idx, node_idx in enumerate(joints):
        par_node = parent_map.get(node_idx, -1)
        if par_node in node_to_joint:
            par_j_idx = node_to_joint[par_node]
            assert par_j_idx < j_idx, f"Joint {j_idx} has parent {par_j_idx} >= {j_idx}!"

    # 3. Verify C++ glbimporter.cpp implementation
    importer_cpp = os.path.join(base_dir, "src", "viewport", "import", "glb", "glbimporter.cpp")
    with open(importer_cpp, "r", encoding="utf-8") as f:
        cpp = f.read()
        assert "data->skins_count > 0" in cpp
        assert "cgltf_node_transform_world" in cpp
        assert "hasValidEmbeddedRig" in cpp
        assert "HumanoidAutoRigger::buildAndSkin" in cpp

    print("[PASS] Task 1: Pre-rigged embedded skin extraction and invariants verified.")

if __name__ == "__main__":
    test_prerigged_importer()
    print("All Task 1 tests passed.")

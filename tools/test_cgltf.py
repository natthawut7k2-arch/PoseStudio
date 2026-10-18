#!/usr/bin/env python3
"""
Test script to verify cgltf integration and GLB header specifications.
"""
import os
import sys
import struct
import json

def test_cgltf_files():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    header = os.path.join(base_dir, "src", "viewport", "import", "glb", "cgltf.h")
    impl = os.path.join(base_dir, "src", "viewport", "import", "glb", "cgltf_impl.cpp")
    
    assert os.path.isfile(header), f"Missing cgltf.h at {header}"
    assert os.path.isfile(impl), f"Missing cgltf_impl.cpp at {impl}"
    
    with open(header, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()
        assert "cgltf_parse_file" in content, "cgltf_parse_file not found in cgltf.h"
        assert "cgltf_load_buffers" in content, "cgltf_load_buffers not found in cgltf.h"
        assert "cgltf_free" in content, "cgltf_free not found in cgltf.h"
        
    with open(impl, "r", encoding="utf-8") as f:
        impl_content = f.read()
        assert "#define CGLTF_IMPLEMENTATION" in impl_content
        assert '#include "cgltf.h"' in impl_content

    print("[PASS] cgltf.h and cgltf_impl.cpp verified.")

def test_midoriya_glb_cgltf_compat():
    glb_path = r"D:\koikatsu\UserData\Midoriya\Midoriya.glb"
    if not os.path.isfile(glb_path):
        print(f"[SKIP] {glb_path} not found.")
        return

    with open(glb_path, "rb") as f:
        magic, version, length = struct.unpack("<4sII", f.read(12))
        assert magic == b"glTF", f"Invalid GLB magic: {magic}"
        assert version == 2, f"Expected glTF version 2, got {version}"
        
        chunk_len, chunk_type = struct.unpack("<I4s", f.read(8))
        assert chunk_type == b"JSON", f"Expected JSON chunk, got {chunk_type}"
        
        json_bytes = f.read(chunk_len)
        data = json.loads(json_bytes.decode("utf-8"))
        
        meshes = data.get("meshes", [])
        nodes = data.get("nodes", [])
        skins = data.get("skins", [])
        
        assert len(meshes) == 6, f"Expected 6 meshes, got {len(meshes)}"
        assert len(nodes) == 6, f"Expected 6 nodes, got {len(nodes)}"
        assert len(skins) == 0, f"Expected 0 skins, got {len(skins)}"

    print(f"[PASS] Midoriya.glb verified for cgltf compatibility: {len(meshes)} meshes, {len(nodes)} nodes, {len(skins)} skins.")

if __name__ == "__main__":
    test_cgltf_files()
    test_midoriya_glb_compat = test_midoriya_glb_cgltf_compat()
    print("All Task 1 tests passed successfully.")

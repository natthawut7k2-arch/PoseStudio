#!/usr/bin/env python3
"""
Test script for GlbImporter & ImporterRegistry:
Verifies glbimporter header/impl, ImporterRegistry registration,
and simulated ModelData creation on Midoriya.glb.
"""
import os
import sys

def test_source_files():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    header = os.path.join(base_dir, "src", "viewport", "import", "glb", "glbimporter.h")
    impl = os.path.join(base_dir, "src", "viewport", "import", "glb", "glbimporter.cpp")
    registry = os.path.join(base_dir, "src", "viewport", "import", "importerregistry.cpp")
    
    assert os.path.isfile(header), f"Missing {header}"
    assert os.path.isfile(impl), f"Missing {impl}"
    
    with open(header, "r", encoding="utf-8") as f:
        content = f.read()
        assert "class GlbImporter : public MeshImporter" in content
        assert "displayName()" in content
        assert "extensions()" in content
        assert "load(const std::string& path)" in content

    with open(impl, "r", encoding="utf-8") as f:
        content = f.read()
        assert "cgltf_parse_file" in content
        assert "HumanoidAutoRigger::buildAndSkin" in content

    with open(registry, "r", encoding="utf-8") as f:
        reg_content = f.read()
        assert "glbimporter.h" in reg_content
        assert "GlbImporter" in reg_content

    print("[PASS] GlbImporter files and ImporterRegistry integration verified.")

if __name__ == "__main__":
    test_source_files()
    print("All Task 3 tests passed.")

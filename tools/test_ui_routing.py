#!/usr/bin/env python3
"""
Test script for Task 4: UI and Viewport Routing.
Verifies CMakeLists.txt sources and include dirs, MenuManager .GLB action,
and ViewportWidget/VulkanWindow routing methods.
"""
import os
import sys

def test_ui_routing():
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    # 1. CMakeLists.txt
    cmake_path = os.path.join(base_dir, "CMakeLists.txt")
    with open(cmake_path, "r", encoding="utf-8") as f:
        cmake = f.read()
        assert "src/viewport/import/glb/glbimporter.cpp" in cmake
        assert "src/viewport/import/glb/humanoidautorigger.cpp" in cmake
        assert "src/viewport/import/glb/cgltf_impl.cpp" in cmake
        assert "src/viewport/import/glb" in cmake

    # 2. menumanager.h & menumanager.cpp
    mm_h = os.path.join(base_dir, "src", "shell", "menumanager.h")
    with open(mm_h, "r", encoding="utf-8") as f:
        assert "void importGlbFile();" in f.read()
        
    mm_cpp = os.path.join(base_dir, "src", "shell", "menumanager.cpp")
    with open(mm_cpp, "r", encoding="utf-8") as f:
        cpp = f.read()
        assert '{".GLB (GL Transmission Format .glTF)", &MenuManager::importGlbFile}' in cpp
        assert "void MenuManager::importGlbFile()" in cpp

    # 3. viewportwidget.h & viewportwidget.cpp
    vw_h = os.path.join(base_dir, "src", "viewport", "viewportwidget.h")
    with open(vw_h, "r", encoding="utf-8") as f:
        assert "void importGlb(const QString& path);" in f.read()
        
    vw_cpp = os.path.join(base_dir, "src", "viewport", "viewportwidget.cpp")
    with open(vw_cpp, "r", encoding="utf-8") as f:
        assert "void ViewportWidget::importGlb(const QString& path)" in f.read()

    # 4. vulkanwindow.h & vulkanwindow.cpp
    vk_h = os.path.join(base_dir, "src", "viewport", "vulkanwindow.h")
    with open(vk_h, "r", encoding="utf-8") as f:
        assert "void importGlb(const QString& path);" in f.read()

    vk_cpp = os.path.join(base_dir, "src", "viewport", "vulkanwindow.cpp")
    with open(vk_cpp, "r", encoding="utf-8") as f:
        assert "void VulkanWindow::importGlb(const QString& path)" in f.read()

    print("[PASS] Task 4 UI routing and CMakeLists.txt verified.")

if __name__ == "__main__":
    test_ui_routing()
    print("All Task 4 tests passed.")

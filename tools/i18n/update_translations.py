#!/usr/bin/env python3
"""
tools/i18n/update_translations.py
Extracts translatable strings from PoseStudio src/ into TS translation catalogs,
and compiles them into binary QM catalogs.

Usage:
    python tools/i18n/update_translations.py extract    # Runs lupdate to update .ts files
    python tools/i18n/update_translations.py compile    # Runs lrelease to build .qm files
    python tools/i18n/update_translations.py all        # Runs both extract and compile
"""

import os
import sys
import glob
import subprocess
import shutil
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SRC_DIR = os.path.join(ROOT, "src")
TRANS_DIR = os.path.join(ROOT, "resources", "translations")

def find_tool(name_candidates):
    # Check PATH
    for name in name_candidates:
        p = shutil.which(name)
        if p:
            return p
    # Check Python Scripts folder on Windows
    appdata = os.environ.get("APPDATA")
    if appdata:
        py_scripts = os.path.join(appdata, "Python", "Python314", "Scripts")
        for name in name_candidates:
            p = os.path.join(py_scripts, name if name.endswith(".exe") else name + ".exe")
            if os.path.isfile(p):
                return p
    return None

def get_source_files():
    files = []
    for root, dirs, filenames in os.walk(SRC_DIR):
        for f in filenames:
            if f.endswith((".cpp", ".h")):
                files.append(os.path.relpath(os.path.join(root, f), ROOT))
    return sorted(files)

def run_extract(lang="th"):
    lupdate = find_tool(["pyside6-lupdate", "lupdate", "pylupdate6"])
    if not lupdate:
        print("[!] Error: lupdate / pyside6-lupdate not found.")
        return False

    os.makedirs(TRANS_DIR, exist_ok=True)
    ts_file = os.path.join(TRANS_DIR, f"posestudio_{lang}.ts")
    sources = get_source_files()

    cmd = [lupdate] + sources + ["-ts", ts_file]
    print(f"[*] Running lupdate on {len(sources)} source files -> {ts_file}...")
    res = subprocess.run(cmd, cwd=ROOT)
    if res.returncode != 0:
        print(f"[!] lupdate failed with exit code {res.returncode}")
        return False

    # Ensure language attribute is set
    if os.path.isfile(ts_file):
        with open(ts_file, "r", encoding="utf-8") as f:
            content = f.read()
        if 'language="' not in content:
            content = re.sub(r'<TS([^>]*)>', rf'<TS\1 language="{lang}">', content, count=1)
            with open(ts_file, "w", encoding="utf-8") as f:
                f.write(content)
            print(f"[*] Added language=\"{lang}\" to {ts_file}")

    print(f"[+] Successfully extracted strings into {ts_file}")
    return True

def run_compile(lang="th"):
    lrelease = find_tool(["pyside6-lrelease", "lrelease"])
    if not lrelease:
        print("[!] Error: lrelease / pyside6-lrelease not found.")
        return False

    ts_file = os.path.join(TRANS_DIR, f"posestudio_{lang}.ts")
    qm_file = os.path.join(TRANS_DIR, f"posestudio_{lang}.qm")

    if not os.path.isfile(ts_file):
        print(f"[!] TS file not found: {ts_file}")
        return False

    cmd = [lrelease, ts_file, "-qm", qm_file]
    print(f"[*] Compiling {ts_file} -> {qm_file}...")
    res = subprocess.run(cmd, cwd=ROOT)
    if res.returncode != 0:
        print(f"[!] lrelease failed with exit code {res.returncode}")
        return False

    print(f"[+] Successfully compiled {qm_file} ({os.path.getsize(qm_file)} bytes)")
    return True

def main():
    action = sys.argv[1] if len(sys.argv) > 1 else "all"
    if action in ("extract", "all"):
        if not run_extract("th"):
            sys.exit(1)
    if action in ("compile", "all"):
        if not run_compile("th"):
            sys.exit(1)

if __name__ == "__main__":
    main()

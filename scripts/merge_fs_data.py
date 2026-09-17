"""
merge_fs_data.py
Pre-build extra_script: copies ESP32AlpacaDevices web assets from the
PlatformIO library cache into the project's data/ directory before the
LittleFS image is built.

Files from the library are only copied if they do NOT already exist in
data/ so that our own project files (index.html, app.js, …) are never
overwritten.
"""

Import("env")  # noqa: F821 – injected by PlatformIO
import os, shutil, glob


def _find_lib_data_dir(project_dir):
    """Return the 'data/' path inside the fetched ESP32AlpacaDevices library."""
    libdeps_dir = os.path.join(project_dir, ".pio", "libdeps")
    candidates = [
        "ESP32AlpacaDevices",
        "ESP32AlpacaDevices2",
        "esp32alpacadevices",
    ]
    for env_dir in glob.glob(os.path.join(libdeps_dir, "*")):
        for candidate in candidates:
            path = os.path.join(env_dir, candidate, "data")
            if os.path.isdir(path):
                return path
    return None


# SCons calls actions with keyword args (target, source, env) – names must match.
def merge_lib_data(target, source, env):  # noqa: F821
    project_dir = env.get("PROJECT_DIR", os.getcwd())
    lib_data = _find_lib_data_dir(project_dir)
    if lib_data is None:
        print(
            "WARNING [merge_fs_data]: Could not find ESP32AlpacaDevices data dir.\n"
            "         Run 'pio pkg install' then build again."
        )
        return

    project_data = os.path.join(project_dir, "data")
    os.makedirs(project_data, exist_ok=True)

    copied = 0
    for root, _dirs, files in os.walk(lib_data):
        rel = os.path.relpath(root, lib_data)
        dest_dir = os.path.join(project_data, rel)
        os.makedirs(dest_dir, exist_ok=True)
        for fname in files:
            dst = os.path.join(dest_dir, fname)
            if not os.path.exists(dst):
                shutil.copy2(os.path.join(root, fname), dst)
                copied += 1

    if copied:
        print(f"[merge_fs_data] Copied {copied} library web asset(s) into data/")
    else:
        print("[merge_fs_data] Library web assets already present, nothing to copy.")


env.AddPreAction("buildfs", merge_lib_data)  # noqa: F821

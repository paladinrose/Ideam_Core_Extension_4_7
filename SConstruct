#!/usr/bin/env python
import os
import sys

# Inherit the base environment from godot-cpp v10+, explicitly targeting Godot 4.7
env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

# --- DATA OWNERSHIP & IDENTITY ---
project_folder = "godot-project"
out_filename   = "ideam_core_extension"
out_dir        = "#" + project_folder + "/addons/" + out_filename + "/lib/"

# --- COMPILER & LANGUAGE STANDARDS ---
# Targeting C++26 explicitly.
if env.get("is_msvc", False):
    # Remove godot-cpp default /std:c++17 to avoid MSVC D9025 override warnings.
    if "CXXFLAGS" in env:
        env["CXXFLAGS"] = [f for f in env["CXXFLAGS"] if f != "/std:c++17"]
    if "CCFLAGS" in env:
        env["CCFLAGS"] = [f for f in env["CCFLAGS"] if f != "/std:c++17"]
    # MSVC uses /std:c++latest for bleeding edge / C++26.
    # /Zc:__cplusplus is REQUIRED so MSVC correctly sets the __cplusplus macro for C++26 features.
    # _ENABLE_EXTENDED_ALIGNED_STORAGE allows std::stable_sort temporary buffers on alignas(64) PODs.
    env.Append(CXXFLAGS=["/std:c++latest", "/Zc:__cplusplus"])
    env.Append(CPPDEFINES=["_ENABLE_EXTENDED_ALIGNED_STORAGE"])
else:
    # GCC 14+ / Clang 18+ support.
    env.Append(CXXFLAGS=["-std=c++26"])

# --- INCLUDE PATHS ---
env.Append(CPPPATH=["src/"])
env.Append(CPPPATH=["src/external"])

# --- SOURCE DISCOVERY ---
sources = []
for root, dirs, files in os.walk("src"):
    for file in files:
        if file.endswith(".cpp"):
            full_path = os.path.join(root, file).replace("\\", "/")
            sources.append(full_path)

if env["platform"] == "windows":
    if env.get("is_msvc", False):
        env.Append(CXXFLAGS=["/bigobj"])
        env.Append(LINKFLAGS=["/INCREMENTAL:NO", "/PDBPAGESIZE:32768", "/OPT:REF", "/OPT:ICF"])
    else:
        env.Append(CXXFLAGS=["-Wa,-mbig-obj"])

# --- BUILD LOGIC ---
if env["platform"] == "macos":
    lib_path = "{}{}.{}.{}.framework/{}.{}.{}".format(
        out_dir, out_filename, env["platform"], env["target"],
        out_filename, env["platform"], env["target"]
    )
    library = env.SharedLibrary(lib_path, source=sources)

elif env["platform"] == "ios":
    suffix = ".simulator.a" if env["ios_simulator"] else ".a"
    lib_name = "{}.{}.{}{}".format(out_filename, env["platform"], env["target"], suffix)
    library = env.StaticLibrary(out_dir + lib_name, source=sources)

else:
    # Windows / Linux logic
    library = env.SharedLibrary(
        "{}{}{}{}".format(out_dir, out_filename, env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)

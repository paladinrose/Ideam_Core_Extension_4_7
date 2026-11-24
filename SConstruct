#!/usr/bin/env python
import os
import sys

# Inherit the base environment from godot-cpp v10+, explicitly targeting Godot 4.7
env = SConscript("godot-cpp/SConstruct", {"api_version": "4.7"})

# --- SCONS INCREMENTAL BUILD SPEEDUPS ---
# Check file timestamps before MD5-hashing content, and cache #include dependency scans.
# (Run with `--implicit-deps-changed` if you ever want to force a full header re-scan.)
Decider("MD5-timestamp")
SetOption("implicit_cache", 1)

# --- BUILD PROFILE ARGUMENTS ---
def _arg_bool(name, default):
    val = ARGUMENTS.get(name, "yes" if default else "no").strip().lower()
    return val in ("1", "true", "yes", "on")

# Default docs ON for bare CLI runs, OFF via VS Code "Fast Dev Build" task.
build_docs = _arg_bool("build_docs", True)
# Default fast_link ON for debug/editor builds, OFF for release builds.
fast_link = _arg_bool("fast_link", env["target"] != "template_release")
# Optional SIMD target ISA (default | sse2 | avx2 | avx512 | native) & vectorization report.
simd_mode = ARGUMENTS.get("simd", "default").strip().lower()
vec_report = _arg_bool("vec_report", False)

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

# --- OPTIONAL SIMD & VECTORIZATION REPORT FLAGS ---
is_x86 = env.get("arch", "x86_64") in ("x86_64", "x86_32")
if is_x86:
    if env.get("is_msvc", False):
        if simd_mode == "avx2":
            env.Append(CXXFLAGS=["/arch:AVX2"])
        elif simd_mode == "avx512":
            env.Append(CXXFLAGS=["/arch:AVX512"])
        if vec_report:
            env.Append(CXXFLAGS=["/Qvec-report:2"])
    else:
        if simd_mode == "avx2":
            env.Append(CXXFLAGS=["-mavx2", "-mfma"])
        elif simd_mode == "avx512":
            env.Append(CXXFLAGS=["-mavx512f", "-mavx512dq", "-mavx512vl", "-mfma"])
        elif simd_mode == "native":
            env.Append(CXXFLAGS=["-march=native"])

        if vec_report:
            if "clang" in str(env.get("CXX", "")).lower():
                env.Append(CXXFLAGS=[
                    "-Rpass=loop-vectorize",
                    "-Rpass-missed=loop-vectorize",
                    "-Rpass-analysis=loop-vectorize",
                ])
            else:
                env.Append(CXXFLAGS=["-fopt-info-vec-optimized", "-fopt-info-vec-missed"])

# --- INCLUDE PATHS ---
env.Append(CPPPATH=["src/"])
env.Append(CPPPATH=["src/external"])

# --- SOURCE DISCOVERY ---
sources = []
for root, dirs, files in os.walk("src"):
    if "gen" in dirs:
        dirs.remove("gen")
    for file in files:
        if file.endswith(".cpp") and not file.endswith(".gen.cpp"):
            full_path = os.path.join(root, file).replace("\\", "/")
            sources.append(full_path)

# --- GDEXTENSION IN-EDITOR CLASS DOCUMENTATION (Godot 4.3+) ---
# Pass `build_docs=no` for fast incremental development builds.
if build_docs and env["target"] in ["editor", "template_debug"] and hasattr(env, "GodotCPPDocData"):
    doc_xmls = sorted(Glob("doc_classes/*.xml"), key=lambda n: str(n))
    if doc_xmls:
        doc_env = env.Clone()
        if doc_env.get("is_msvc", False):
            # Strip debug info from generated doc strings (saves compile time & PDB size).
            for flag_var in ("CXXFLAGS", "CCFLAGS"):
                if flag_var in doc_env:
                    doc_env[flag_var] = [f for f in doc_env[flag_var] if f not in ("/Zi", "/Z7")]

        # Larger chunk size reduces compiler process spawns while still allowing parallel builds.
        chunk_size = 64
        for i in range(0, len(doc_xmls), chunk_size):
            chunk = doc_xmls[i : i + chunk_size]
            target_cpp = f"src/gen/doc_data_{i // chunk_size:02d}.gen.cpp"
            doc_cpp = doc_env.GodotCPPDocData(target_cpp, source=chunk)
            sources.append(doc_env.SharedObject(doc_cpp))

# --- PLATFORM-SPECIFIC COMPILER & LINKER TUNING ---
if env["platform"] == "windows":
    if env.get("is_msvc", False):
        # Required by template-heavy Task Registry translation units (>65,536 COFF sections).
        env.Append(CXXFLAGS=["/bigobj"])

        if fast_link:
            # Fast Dev Link: keeps /PDBPAGESIZE:32768 (for large symbol tables) but disables
            # /OPT:REF & /OPT:ICF COMDAT folding and uses /DEBUG:FASTLINK for much faster links.
            env.Append(LINKFLAGS=[
                "/INCREMENTAL:NO",
                "/PDBPAGESIZE:32768",
                "/OPT:NOREF",
                "/OPT:NOICF",
            ])
        else:
            # Release Link: full dead-code stripping (/OPT:REF) and identical COMDAT folding (/OPT:ICF).
            env.Append(LINKFLAGS=[
                "/INCREMENTAL:NO",
                "/PDBPAGESIZE:32768",
                "/OPT:REF",
                "/OPT:ICF",
            ])
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

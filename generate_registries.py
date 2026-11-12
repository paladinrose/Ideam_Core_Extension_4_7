import argparse
from pathlib import Path
import re


# Explicit overrides for multi-variant headers or non-standard enum suffixes
EXACT_ENUM_OVERRIDES: dict[str, str] = {
    "morphological_static": "Morphological_Static",
    "dsu_cluster_static": "DSUCluster_Static",
    "stencil_convolution": "Stencil_",
    "image_palette_lut": "ImagePaletteLut",
    "tiling_instance": "TilingInstanceTransform",
}

# Standard acronym tokens that remain uppercase inside PascalCase LogicID enums
ACRONYM_MAP: dict[str, str] = {
    "aabb": "AABB",
    "rgba": "RGBA",
    "hsva": "HSVA",
    "dsu": "DSU",
    "lod": "LOD",
    "cpu": "CPU",
    "gpu": "GPU",
    "ik": "IK",
    "uv": "UV",
    "csg": "CSG",
    "sdf": "SDF",
    "lut": "LUT",
    "astar": "AStar",
    "1d": "1D",
    "2d": "2D",
    "3d": "3D",
    "4d": "4D",
    "spatial2d": "Spatial2D",
    "spatial3d": "Spatial3D",
    "spatial4d": "Spatial4D",
    "wfc": "WFC",
    "edt": "EDT",
    "fft": "FFT",
    "pde": "PDE",
    "xpbd": "XPBD",
    "obb": "OBB",
    "rd": "RD",
    "ssbo": "SSBO",
    "pcm": "PCM",
    "2drd": "2DRD",
}


def get_enum_name(base_name: str) -> str:
  """Converts a snake_case logic base name to the exact PascalCase LogicID enum."""
  if base_name in EXACT_ENUM_OVERRIDES:
    return EXACT_ENUM_OVERRIDES[base_name]
  return "".join(
      ACRONYM_MAP.get(word, word.capitalize()) for word in base_name.split("_")
  )


def increment_manifest_version(
    manifest_path: Path, version_const_name: str
) -> int | None:
  """Increments the specified manifest version constant in manifest_path by 1."""
  if not manifest_path.exists():
    print(f"\n[ERROR] Manifest header not found at: {manifest_path}")
    return None

  content = manifest_path.read_text(encoding="utf-8")

  pattern = re.compile(
      rf"(static\s+constexpr\s+int\s+{re.escape(version_const_name)}\s*=\s*)(\d+)(;)"
  )
  match = pattern.search(content)

  if not match:
    print(
        f"\n[ERROR] Could not find {version_const_name} pattern in"
        f" {manifest_path.name}."
    )
    return None

  old_version = int(match.group(2))
  new_version = old_version + 1

  updated_content = pattern.sub(rf"\g<1>{new_version}\g<3>", content, count=1)
  manifest_path.write_text(updated_content, encoding="utf-8")

  print(
      f"[MANIFEST UPDATED] {manifest_path.name} ({version_const_name}): Version"
      f" {old_version} -> {new_version}"
  )
  return new_version


def verify_registration_coverage(
    base_dir: Path, target: dict, logic_files: list[Path]
) -> int:
  """Checks if every logic header has a matching enum entry and resolver/dispatch specialization."""
  reg_header = base_dir / "registration" / target["registry_header"]
  sub_reg_header = base_dir / "registration" / target["header_include"]
  reg_cpp = reg_header.with_suffix(".cpp")

  reg_text = reg_header.read_text(encoding="utf-8") if reg_header.exists() else ""
  if sub_reg_header.exists():
    dispatch_text = sub_reg_header.read_text(encoding="utf-8")
    dispatch_source_name = sub_reg_header.name
  elif reg_cpp.exists():
    dispatch_text = reg_cpp.read_text(encoding="utf-8")
    dispatch_source_name = reg_cpp.name
  else:
    dispatch_text = ""
    dispatch_source_name = target["header_include"]

  warnings = 0
  for logic_path in sorted(logic_files):
    base_name = logic_path.stem.replace(target["suffix"], "")
    enum_id = get_enum_name(base_name)

    # Multi-variant headers (e.g. stencil_convolution, blend_shape_evaluate) use prefix matches
    enum_prefix_pattern = re.compile(rf"\b{re.escape(enum_id)}")
    if reg_text and not enum_prefix_pattern.search(reg_text):
      print(
          f"  [WARN] {logic_path.name}: '{enum_id}' not found in"
          f" {reg_header.name} ({target['enum_prefix']})"
      )
      warnings += 1

    resolver_pattern = re.compile(
        rf"{re.escape(target['enum_prefix'])}::{re.escape(enum_id)}"
    )
    if dispatch_text and not resolver_pattern.search(dispatch_text):
      print(
          f"  [WARN] {logic_path.name}: '{target['enum_prefix']}::{enum_id}'"
          f" specialization/dispatch not found in {dispatch_source_name}"
      )
      warnings += 1

  return warnings


DOMAINS = [
    {
        "domain_name": "Core",
        "base_dir": Path("src/core/tasks"),
        "manifest_header": Path("src/core/tasks/registration/task_manifest.h"),
        "version_const": "CURRENT_MANIFEST_VERSION",
        "targets": [
            {
                "name": "Query",
                "logic_dir": "query_logic",
                "reg_dir": "query_reg",
                "suffix": "_query_logic",
                "registry_header": "query_task_registry.h",
                "header_include": "query_logic_sub_registry.h",
                "enum_prefix": "QueryLogicID",
                "struct_prefix": "QueryLogicSubRegistry",
            },
            {
                "name": "Metadata",
                "logic_dir": "metadata_logic",
                "reg_dir": "metadata_reg",
                "suffix": "_metadata_logic",
                "registry_header": "metadata_task_registry.h",
                "header_include": "metadata_logic_sub_registry.h",
                "enum_prefix": "MetadataLogicID",
                "struct_prefix": "MetadataLogicSubRegistry",
            },
            {
                "name": "Transform",
                "logic_dir": "transform_logic",
                "reg_dir": "transform_reg",
                "suffix": "_transform_logic",
                "registry_header": "transform_task_registry.h",
                "header_include": "transform_logic_sub_registry.h",
                "enum_prefix": "TransformLogicID",
                "struct_prefix": "TransformLogicSubRegistry",
            },
        ],
    },
    {
        "domain_name": "IdeamGames",
        "base_dir": Path("src/core/tasks/domains/games"),
        "manifest_header": Path(
            "src/core/tasks/domains/games/registration/games_task_domain_pack.h"
        ),
        "version_const": "GAMES_MANIFEST_VERSION",
        "targets": [
            {
                "name": "Games Query",
                "logic_dir": "query_logic",
                "reg_dir": "query_reg",
                "suffix": "_query_logic",
                "registry_header": "games_query_task_registry.h",
                "header_include": "games_query_logic_sub_registry.h",
                "enum_prefix": "GamesQueryLogicID",
                "struct_prefix": "GamesQueryLogicSubRegistry",
            },
            {
                "name": "Games Metadata",
                "logic_dir": "metadata_logic",
                "reg_dir": "metadata_reg",
                "suffix": "_metadata_logic",
                "registry_header": "games_metadata_task_registry.h",
                "header_include": "games_metadata_logic_sub_registry.h",
                "enum_prefix": "GamesMetadataLogicID",
                "struct_prefix": "GamesMetadataLogicSubRegistry",
            },
            {
                "name": "Games Transform",
                "logic_dir": "transform_logic",
                "reg_dir": "transform_reg",
                "suffix": "_transform_logic",
                "registry_header": "games_transform_task_registry.h",
                "header_include": "games_transform_logic_sub_registry.h",
                "enum_prefix": "GamesTransformLogicID",
                "struct_prefix": "GamesTransformLogicSubRegistry",
            },
        ],
    },
    {
        "domain_name": "IdeamNarratives",
        "base_dir": Path("src/core/tasks/domains/narratives"),
        "manifest_header": Path(
            "src/core/tasks/domains/narratives/registration/narratives_task_domain_pack.h"
        ),
        "version_const": "NARRATIVES_MANIFEST_VERSION",
        "targets": [
            {
                "name": "Narratives Query",
                "logic_dir": "query_logic",
                "reg_dir": "query_reg",
                "suffix": "_query_logic",
                "registry_header": "narratives_query_task_registry.h",
                "header_include": "narratives_query_logic_sub_registry.h",
                "enum_prefix": "NarrativesQueryLogicID",
                "struct_prefix": "NarrativesQueryLogicSubRegistry",
            },
            {
                "name": "Narratives Metadata",
                "logic_dir": "metadata_logic",
                "reg_dir": "metadata_reg",
                "suffix": "_metadata_logic",
                "registry_header": "narratives_metadata_task_registry.h",
                "header_include": "narratives_metadata_logic_sub_registry.h",
                "enum_prefix": "NarrativesMetadataLogicID",
                "struct_prefix": "NarrativesMetadataLogicSubRegistry",
            },
            {
                "name": "Narratives Transform",
                "logic_dir": "transform_logic",
                "reg_dir": "transform_reg",
                "suffix": "_transform_logic",
                "registry_header": "narratives_transform_task_registry.h",
                "header_include": "narratives_transform_logic_sub_registry.h",
                "enum_prefix": "NarrativesTransformLogicID",
                "struct_prefix": "NarrativesTransformLogicSubRegistry",
            },
        ],
    },
    {
        "domain_name": "IdeamGeometry",
        "base_dir": Path("src/core/tasks/domains/geometry"),
        "manifest_header": Path(
            "src/core/tasks/domains/geometry/registration/geometry_task_domain_pack.h"
        ),
        "version_const": "GEOMETRY_MANIFEST_VERSION",
        "targets": [
            {
                "name": "Geometry Query",
                "logic_dir": "query_logic",
                "reg_dir": "query_reg",
                "suffix": "_query_logic",
                "registry_header": "geometry_query_task_registry.h",
                "header_include": "geometry_query_logic_sub_registry.h",
                "enum_prefix": "GeometryQueryLogicID",
                "struct_prefix": "GeometryQueryLogicSubRegistry",
            },
            {
                "name": "Geometry Metadata",
                "logic_dir": "metadata_logic",
                "reg_dir": "metadata_reg",
                "suffix": "_metadata_logic",
                "registry_header": "geometry_metadata_task_registry.h",
                "header_include": "geometry_metadata_logic_sub_registry.h",
                "enum_prefix": "GeometryMetadataLogicID",
                "struct_prefix": "GeometryMetadataLogicSubRegistry",
            },
            {
                "name": "Geometry Transform",
                "logic_dir": "transform_logic",
                "reg_dir": "transform_reg",
                "suffix": "_transform_logic",
                "registry_header": "geometry_transform_task_registry.h",
                "header_include": "geometry_transform_logic_sub_registry.h",
                "enum_prefix": "GeometryTransformLogicID",
                "struct_prefix": "GeometryTransformLogicSubRegistry",
            },
        ],
    },
    {
        "domain_name": "IdeamSplines",
        "base_dir": Path("src/core/tasks/domains/splines"),
        "manifest_header": Path(
            "src/core/tasks/domains/splines/registration/splines_task_domain_pack.h"
        ),
        "version_const": "SPLINES_MANIFEST_VERSION",
        "targets": [
            {
                "name": "Splines Query",
                "logic_dir": "query_logic",
                "reg_dir": "query_reg",
                "suffix": "_query_logic",
                "registry_header": "splines_query_task_registry.h",
                "header_include": "splines_query_logic_sub_registry.h",
                "enum_prefix": "SplinesQueryLogicID",
                "struct_prefix": "SplinesQueryLogicSubRegistry",
            },
            {
                "name": "Splines Metadata",
                "logic_dir": "metadata_logic",
                "reg_dir": "metadata_reg",
                "suffix": "_metadata_logic",
                "registry_header": "splines_metadata_task_registry.h",
                "header_include": "splines_metadata_logic_sub_registry.h",
                "enum_prefix": "SplinesMetadataLogicID",
                "struct_prefix": "SplinesMetadataLogicSubRegistry",
            },
            {
                "name": "Splines Transform",
                "logic_dir": "transform_logic",
                "reg_dir": "transform_reg",
                "suffix": "_transform_logic",
                "registry_header": "splines_transform_task_registry.h",
                "header_include": "splines_transform_logic_sub_registry.h",
                "enum_prefix": "SplinesTransformLogicID",
                "struct_prefix": "SplinesTransformLogicSubRegistry",
            },
        ],
    },
    {
        "domain_name": "IdeamTiles",
        "base_dir": Path("src/core/tasks/domains/tiles"),
        "manifest_header": Path(
            "src/core/tasks/domains/tiles/registration/tiles_task_domain_pack.h"
        ),
        "version_const": "TILES_MANIFEST_VERSION",
        "targets": [
            {
                "name": "Tiles Query",
                "logic_dir": "query_logic",
                "reg_dir": "query_reg",
                "suffix": "_query_logic",
                "registry_header": "tiles_query_task_registry.h",
                "header_include": "tiles_query_logic_sub_registry.h",
                "enum_prefix": "TilesQueryLogicID",
                "struct_prefix": "TilesQueryLogicSubRegistry",
            },
            {
                "name": "Tiles Metadata",
                "logic_dir": "metadata_logic",
                "reg_dir": "metadata_reg",
                "suffix": "_metadata_logic",
                "registry_header": "tiles_metadata_task_registry.h",
                "header_include": "tiles_metadata_logic_sub_registry.h",
                "enum_prefix": "TilesMetadataLogicID",
                "struct_prefix": "TilesMetadataLogicSubRegistry",
            },
            {
                "name": "Tiles Transform",
                "logic_dir": "transform_logic",
                "reg_dir": "transform_reg",
                "suffix": "_transform_logic",
                "registry_header": "tiles_transform_task_registry.h",
                "header_include": "tiles_transform_logic_sub_registry.h",
                "enum_prefix": "TilesTransformLogicID",
                "struct_prefix": "TilesTransformLogicSubRegistry",
            },
        ],
    },
]


def main():
  parser = argparse.ArgumentParser(
      description="Generate DOD task sub-registry translation units across domains."
  )
  parser.add_argument(
      "--check",
      action="store_true",
      help="Only verify enum and resolver coverage without writing files.",
  )
  args = parser.parse_args()

  grand_generated = 0
  grand_skipped = 0
  grand_warnings = 0

  for domain in DOMAINS:
    domain_name = domain["domain_name"]
    base_dir: Path = domain["base_dir"]
    manifest_path: Path = domain["manifest_header"]
    version_const: str = domain["version_const"]

    print("\n" + "=" * 55)
    print(f"DOMAIN: {domain_name} ({base_dir})")
    print("=" * 55)

    if not base_dir.exists():
      print(f"[SKIP] Domain directory {base_dir} does not exist yet.")
      continue

    domain_generated = 0
    domain_skipped = 0

    for target in domain["targets"]:
      print(f"\n--- Processing {target['name']} Registries ---")

      logic_dir = base_dir / target["logic_dir"]
      reg_dir = base_dir / "registration" / target["reg_dir"]
      sub_reg_header = base_dir / "registration" / target["header_include"]

      search_pattern = f"*{target['suffix']}.h"
      logic_files = sorted(logic_dir.glob(search_pattern)) if logic_dir.exists() else []

      if not logic_files:
        print(f"No logic headers found in {logic_dir} (0 tasks).")
        continue

      grand_warnings += verify_registration_coverage(
          base_dir, target, logic_files
      )

      if args.check:
        print(
            f"Verified {len(logic_files)} logic header(s) for {target['name']}."
        )
        continue

      # If the domain uses direct dispatch in *_task_registry.cpp rather than a
      # *_logic_sub_registry.h header, skip generating *_sub_registry.cpp files
      # that would #include a non-existent sub-registry header.
      if not sub_reg_header.exists():
        print(
            f"[DIRECT DISPATCH] {sub_reg_header.name} not present; verified"
            f" {len(logic_files)} task(s) directly in"
            f" {target['registry_header']}."
        )
        domain_skipped += len(logic_files)
        continue

      logic_dir.mkdir(parents=True, exist_ok=True)
      reg_dir.mkdir(parents=True, exist_ok=True)

      generated_count = 0
      skipped_count = 0

      for logic_path in logic_files:
        filename = logic_path.stem
        base_name = filename.replace(target["suffix"], "")

        new_filename = f"{base_name}_sub_registry.cpp"
        dest_path = reg_dir / new_filename

        if dest_path.exists():
          print(f"Skipping {new_filename} (Already exists)")
          skipped_count += 1
          continue

        enum_id = get_enum_name(base_name)

        cpp_content = f"""// AUTO-GENERATED FILE
#include "../{target['header_include']}"
#include "../../{target['logic_dir']}/{logic_path.name}" // Ensure the compiler sees the logic struct

namespace ideam::core {{

// Explicitly instantiate the factory matrix for {enum_id}
template struct {target['struct_prefix']}<{target['enum_prefix']}::{enum_id}>;

}} // namespace ideam::core
"""
        dest_path.write_text(cpp_content, encoding="utf-8")
        print(
            f"Generated: {new_filename} -> {target['enum_prefix']}::{enum_id}"
        )
        generated_count += 1

      print(
          f"{target['name']} Generation Complete. Created: {generated_count} |"
          f" Skipped: {skipped_count}"
      )
      domain_generated += generated_count
      domain_skipped += skipped_count

    grand_generated += domain_generated
    grand_skipped += domain_skipped

    if not args.check:
      if domain_generated > 0:
        increment_manifest_version(manifest_path, version_const)
      else:
        print(
            f"\n[{domain_name}] No new registration files generated."
            f" {version_const} retained."
        )

  print("\n" + "=" * 55)
  print(
      f"GRAND TOTAL - Generated: {grand_generated} | Skipped: {grand_skipped} |"
      f" Coverage Warnings: {grand_warnings}"
  )


if __name__ == "__main__":
  main()
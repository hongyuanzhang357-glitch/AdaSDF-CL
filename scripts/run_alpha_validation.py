#!/usr/bin/env python3
"""Run the AdaSDF-CL alpha validation path and write a compact report."""

from __future__ import annotations

import argparse
import os
import re
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass
class StepResult:
    name: str
    command: list[str]
    returncode: int
    output: str
    status: str | None = None


def display_command(command: list[str], source: Path, build: Path) -> str:
    rendered: list[str] = []
    for part in command:
        text = str(part)
        try:
            path = Path(text)
            if path.is_absolute():
                if path == source:
                    text = "<source>"
                elif path == build:
                    text = "<build>"
                elif source in path.parents:
                    text = str(Path("<source>") / path.relative_to(source)).replace("\\", "/")
                elif build in path.parents:
                    text = str(Path("<build>") / path.relative_to(build)).replace("\\", "/")
                elif source.parent in path.parents:
                    text = str(Path("<workspace>") / path.relative_to(source.parent)).replace("\\", "/")
                elif path.is_absolute():
                    text = "<local-path>"
        except (OSError, ValueError):
            pass
        text = sanitize_output(text, source, build)
        rendered.append(shlex.quote(text))
    return " ".join(rendered)


def run_step(name: str, command: list[str], cwd: Path) -> StepResult:
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return StepResult(name, command, completed.returncode, completed.stdout)


def cmake_build_command(build: Path, config: str) -> list[str]:
    command = ["cmake", "--build", str(build), "--config", config]
    generator = os.environ.get("CMAKE_GENERATOR", "").lower()
    if sys.platform.startswith("win") and (not generator or "visual studio" in generator):
        command.extend(["--", "/nodeReuse:false"])
    return command


def find_executable(build: Path, name: str, config: str) -> Path | None:
    suffix = ".exe" if sys.platform.startswith("win") else ""
    candidates = [
        build / config / f"{name}{suffix}",
        build / "tools" / config / f"{name}{suffix}",
        build / "examples" / config / f"{name}{suffix}",
        build / f"{name}{suffix}",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    matches = list(build.rglob(f"{name}{suffix}"))
    return matches[0] if matches else None


def brief_output(output: str, max_lines: int = 18) -> str:
    lines = [line.rstrip() for line in output.splitlines()]
    if len(lines) <= max_lines:
        return "\n".join(lines)
    return "\n".join(lines[:max_lines] + ["..."])


def sanitize_output(output: str, source: Path, build: Path) -> str:
    replacements = {
        str(source): "<source>",
        source.as_posix(): "<source>",
        str(build): "<build>",
        build.as_posix(): "<build>",
        str(source.parent): "<workspace>",
        source.parent.as_posix(): "<workspace>",
    }
    sanitized = output
    for needle, replacement in sorted(replacements.items(), key=lambda item: len(item[0]), reverse=True):
        sanitized = sanitized.replace(needle, replacement)
    sanitized = re.sub(r"[A-Za-z]:[/\\]Users[/\\]\S+", "<local-path>", sanitized)
    return sanitized


def write_report(
    report: Path,
    results: list[StepResult],
    source: Path,
    build: Path,
    config: str,
) -> None:
    report.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Alpha Validation Summary",
        "",
        f"- Source: `<source>`",
        f"- Build: `<build>`",
        f"- Config: `{config}`",
        "",
        "## Results",
        "",
    ]
    for result in results:
        status = result.status or ("PASS" if result.returncode == 0 else "FAIL")
        lines.extend(
            [
                f"### {result.name}: {status}",
                "",
                "```bash",
                display_command(result.command, source, build),
                "```",
                "",
            ]
        )
        if result.output.strip():
            lines.extend(
                [
                    "```text",
                    brief_output(sanitize_output(result.output, source, build)),
                    "```",
                    "",
                ]
            )
    report.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=".")
    parser.add_argument("--build", default="../build/adasdf_cl_alpha_validation")
    parser.add_argument("--config", default="Debug")
    parser.add_argument("--include-install", action="store_true")
    args = parser.parse_args()

    source = Path(args.source).resolve()
    build = Path(args.build).resolve()
    config = args.config
    workspace = source.parent
    report_path = source / "reports" / "alpha_validation_summary.md"
    if report_path.exists():
        report_path.unlink()
    results: list[StepResult] = []

    configure = [
        "cmake",
        "-S",
        str(source),
        "-B",
        str(build),
        "-DADASDF_CL_BUILD_EXAMPLES=ON",
        "-DADASDF_CL_BUILD_TESTS=ON",
        "-DADASDF_CL_BUILD_BENCHMARKS=ON",
        "-DADASDF_CL_USE_EXISTING_CORE=OFF",
        "-DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON",
        "-DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON",
        "-DADASDF_CL_ENABLE_DEMO_BACKEND=ON",
        "-DADASDF_CL_ENABLE_DEMO_SURROGATE=ON",
        "-DADASDF_CL_ENABLE_COLLISION_VIEWER=ON",
        "-DADASDF_CL_ENABLE_CUDA=ON",
    ]
    steps = [
        ("Configure", configure),
        ("Build", cmake_build_command(build, config)),
        ("CTest", ["ctest", "--test-dir", str(build), "-C", config, "--output-on-failure"]),
    ]

    for name, command in steps:
        result = run_step(name, command, workspace)
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, config)
            return result.returncode

    if args.include_install:
        install_build = build.parent / "adasdf_cl_iv"
        install_prefix = build.parent / "adasdf_cl_install"
        install_result = run_step(
            "Install Validation",
            [
                sys.executable,
                str(source / "scripts" / "run_install_validation.py"),
                "--source",
                str(source),
                "--build",
                str(install_build),
                "--install",
                str(install_prefix),
                "--config",
                config,
            ],
            workspace,
        )
        install_result.status = "PASS" if install_result.returncode == 0 else "FAIL"
        results.append(install_result)
        if install_result.returncode != 0:
            write_report(report_path, results, source, build, config)
            return install_result.returncode
    else:
        results.append(
            StepResult(
                "Install Validation",
                ["install-validation-skipped"],
                0,
                "Install validation was not requested. Re-run with --include-install.",
                status="SKIPPED",
            )
        )

    demo_sdfbin = build.parent / "cube_adaptive_v1.sdfbin"
    collision_svg = build.parent / "collision_v1.svg"
    benchmark_csv = build.parent / "adasdf_batch_benchmark_v1.csv"
    quality_csv = build.parent / "adasdf_expansion_quality_v1.csv"
    mesh_fixture = source / "tests" / "data" / "mesh_diagnostics" / "closed_cube_ascii.stl"
    cleanup_fixture = source / "tests" / "data" / "mesh_diagnostics" / "duplicate_and_degenerate_ascii.stl"
    mesh_report = build.parent / "closed_cube_mesh_report.md"
    mesh_json = build.parent / "closed_cube_mesh_report.json"
    cleaned_stl = build.parent / "cleaned_duplicate_and_degenerate.stl"
    cleanup_report = build.parent / "mesh_cleanup_report.md"
    cleaned_check_report = build.parent / "cleaned_mesh_report.md"
    dense_sdfbin = build.parent / "closed_cube_dense_v1_5.sdfbin"
    dense_report = build.parent / "closed_cube_dense_report.md"
    dense_json = build.parent / "closed_cube_dense_report.json"
    adaptive_sdfbin = build.parent / "closed_cube_adaptive_block_v1_7.sdfbin"
    adaptive_report = build.parent / "closed_cube_adaptive_block_report.md"
    adaptive_json = build.parent / "closed_cube_adaptive_block_report.json"
    adaptive_quality_csv = build.parent / "closed_cube_adaptive_block_quality.csv"
    adaptive_benchmark_csv = build.parent / "closed_cube_adaptive_block_benchmark.csv"
    compressed_sdfbin = build.parent / "closed_cube_compressed_block_v1_7.sdfbin"
    compressed_report = build.parent / "closed_cube_compression_report.md"
    compressed_json = build.parent / "closed_cube_compression_report.json"
    compressed_quality_report = build.parent / "closed_cube_compression_quality.md"
    compressed_quality_csv = build.parent / "closed_cube_compressed_quality.csv"
    compressed_benchmark_csv = build.parent / "closed_cube_compressed_benchmark.csv"
    compressed_direct_sdfbin = build.parent / "closed_cube_compressed_direct_v1_7.sdfbin"
    compressed_direct_report = build.parent / "closed_cube_compressed_direct_report.md"
    compressed_direct_compression_report = build.parent / "closed_cube_compressed_direct_compression_report.md"
    compressed_direct_quality_report = build.parent / "closed_cube_compressed_direct_quality.md"
    adaptive_dryrun_sdfbin = build.parent / "closed_cube_adaptive_dryrun.sdfbin"
    adaptive_dryrun_report = build.parent / "closed_cube_adaptive_dryrun_plan.md"
    adaptive_preview_sdfbin = build.parent / "closed_cube_adaptive_preview.sdfbin"
    adaptive_preview_plan = build.parent / "closed_cube_adaptive_preview_plan.md"
    max_contacts = 8
    for generated in (adaptive_preview_sdfbin, adaptive_dryrun_sdfbin):
        if generated.exists():
            generated.unlink()
    required_demo_tools = {
        "adasdf_capabilities": find_executable(build, "adasdf_capabilities", config),
        "adasdf_mesh_check": find_executable(build, "adasdf_mesh_check", config),
        "adasdf_mesh_clean": find_executable(build, "adasdf_mesh_clean", config),
        "adasdf_build_dense_sdf": find_executable(build, "adasdf_build_dense_sdf", config),
        "adasdf_build_adaptive_sdf": find_executable(build, "adasdf_build_adaptive_sdf", config),
        "adasdf_compress_adaptive_sdf": find_executable(build, "adasdf_compress_adaptive_sdf", config),
        "adasdf_build_compressed_sdf": find_executable(build, "adasdf_build_compressed_sdf", config),
        "adasdf_build_adaptive_sdf_preview": find_executable(build, "adasdf_build_adaptive_sdf_preview", config),
        "adasdf_recommend_demo": find_executable(build, "adasdf_recommend_demo", config),
        "adasdf_build_demo_adaptive": find_executable(build, "adasdf_build_demo_adaptive", config),
        "adasdf_info": find_executable(build, "adasdf_info", config),
        "adasdf_query": find_executable(build, "adasdf_query", config),
        "adasdf_collide": find_executable(build, "adasdf_collide", config),
        "adasdf_expansion_quality": find_executable(build, "adasdf_expansion_quality", config),
        "adasdf_collide_boxes_demo": find_executable(build, "adasdf_collide_boxes_demo", config),
        "adasdf_benchmark_batch_query": find_executable(build, "adasdf_benchmark_batch_query", config),
        "adasdf_capability_walkthrough": find_executable(build, "adasdf_capability_walkthrough", config),
    }
    missing_demo_tools = [
        name for name, path in required_demo_tools.items() if path is None
    ]
    if missing_demo_tools:
        result = StepResult(
            "Core-Free Demo Tool Discovery",
            ["find-demo-tools"],
            1,
            "Missing executable(s): " + ", ".join(missing_demo_tools),
        )
        results.append(result)
        write_report(report_path, results, source, build, config)
        return result.returncode

    demo_steps = [
        (
            "Capabilities CLI",
            [
                str(required_demo_tools["adasdf_capabilities"]),
                "--verbose",
            ],
        ),
        (
            "Capability Walkthrough Example",
            [
                str(required_demo_tools["adasdf_capability_walkthrough"]),
            ],
        ),
        (
            "Mesh Diagnostics CLI",
            [
                str(required_demo_tools["adasdf_mesh_check"]),
                str(mesh_fixture),
                "--readiness",
                "--out",
                str(mesh_report),
                "--json",
                str(mesh_json),
            ],
        ),
        (
            "Mesh Cleanup CLI",
            [
                str(required_demo_tools["adasdf_mesh_clean"]),
                str(cleanup_fixture),
                str(cleaned_stl),
                "--report",
                str(cleanup_report),
            ],
        ),
        (
            "Cleaned Mesh Check CLI",
            [
                str(required_demo_tools["adasdf_mesh_check"]),
                str(cleaned_stl),
                "--readiness",
                "--out",
                str(cleaned_check_report),
            ],
        ),
        (
            "DenseSDF Build CLI",
            [
                str(required_demo_tools["adasdf_build_dense_sdf"]),
                str(mesh_fixture),
                str(dense_sdfbin),
                "--resolution",
                "24",
                "--padding",
                "0.05",
                "--report",
                str(dense_report),
                "--json",
                str(dense_json),
            ],
        ),
        (
            "DenseSDF Info CLI",
            [str(required_demo_tools["adasdf_info"]), str(dense_sdfbin)],
        ),
        (
            "DenseSDF Query CLI",
            [
                str(required_demo_tools["adasdf_query"]),
                str(dense_sdfbin),
                "--point",
                "0.5",
                "0.5",
                "0.5",
            ],
        ),
        (
            "DenseSDF Collide CLI",
            [
                str(required_demo_tools["adasdf_collide"]),
                str(dense_sdfbin),
                str(dense_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "AdaptiveBlockSDF Build CLI",
            [
                str(required_demo_tools["adasdf_build_adaptive_sdf"]),
                str(mesh_fixture),
                str(adaptive_sdfbin),
                "--max-level",
                "2",
                "--block-resolution",
                "5",
                "--report",
                str(adaptive_report),
                "--json",
                str(adaptive_json),
            ],
        ),
        (
            "AdaptiveBlockSDF Info CLI",
            [str(required_demo_tools["adasdf_info"]), str(adaptive_sdfbin)],
        ),
        (
            "AdaptiveBlockSDF Query CLI",
            [
                str(required_demo_tools["adasdf_query"]),
                str(adaptive_sdfbin),
                "--point",
                "0.5",
                "0.5",
                "0.5",
            ],
        ),
        (
            "AdaptiveBlockSDF Collide CLI",
            [
                str(required_demo_tools["adasdf_collide"]),
                str(adaptive_sdfbin),
                str(adaptive_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "AdaptiveBlockSDF Expansion Quality CLI",
            [
                str(required_demo_tools["adasdf_expansion_quality"]),
                str(adaptive_sdfbin),
                "--expansion",
                "global",
                "--global-resolution",
                "16",
                "--samples",
                "200",
                "--out",
                str(adaptive_quality_csv),
            ],
        ),
        (
            "AdaptiveBlockSDF Benchmark Model CLI",
            [
                str(required_demo_tools["adasdf_benchmark_batch_query"]),
                "--model",
                str(adaptive_sdfbin),
                "--points",
                "1000",
                "--query-backend",
                "cpu",
                "--expansion",
                "none",
                "--out",
                str(adaptive_benchmark_csv),
            ],
        ),
        (
            "CompressedSDF Compress CLI",
            [
                str(required_demo_tools["adasdf_compress_adaptive_sdf"]),
                str(adaptive_sdfbin),
                str(compressed_sdfbin),
                "--target-error",
                "1e-3",
                "--max-rank",
                "5",
                "--report",
                str(compressed_report),
                "--json",
                str(compressed_json),
                "--quality-report",
                str(compressed_quality_report),
            ],
        ),
        (
            "CompressedSDF Info CLI",
            [str(required_demo_tools["adasdf_info"]), str(compressed_sdfbin)],
        ),
        (
            "CompressedSDF Query CLI",
            [
                str(required_demo_tools["adasdf_query"]),
                str(compressed_sdfbin),
                "--point",
                "0.5",
                "0.5",
                "0.5",
            ],
        ),
        (
            "CompressedSDF Collide CLI",
            [
                str(required_demo_tools["adasdf_collide"]),
                str(compressed_sdfbin),
                str(compressed_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "CompressedSDF Expansion Quality CLI",
            [
                str(required_demo_tools["adasdf_expansion_quality"]),
                str(compressed_sdfbin),
                "--expansion",
                "global",
                "--global-resolution",
                "16",
                "--samples",
                "200",
                "--out",
                str(compressed_quality_csv),
            ],
        ),
        (
            "CompressedSDF Benchmark Model CLI",
            [
                str(required_demo_tools["adasdf_benchmark_batch_query"]),
                "--model",
                str(compressed_sdfbin),
                "--points",
                "1000",
                "--query-backend",
                "cpu",
                "--expansion",
                "none",
                "--out",
                str(compressed_benchmark_csv),
            ],
        ),
        (
            "CompressedSDF One-Step Build CLI",
            [
                str(required_demo_tools["adasdf_build_compressed_sdf"]),
                str(mesh_fixture),
                str(compressed_direct_sdfbin),
                "--target-error",
                "1e-3",
                "--max-level",
                "2",
                "--block-resolution",
                "5",
                "--max-rank",
                "5",
                "--report",
                str(compressed_direct_report),
                "--compression-report",
                str(compressed_direct_compression_report),
                "--quality-report",
                str(compressed_direct_quality_report),
            ],
        ),
        (
            "AdaptiveBlockSDF Dry Run CLI",
            [
                str(required_demo_tools["adasdf_build_adaptive_sdf"]),
                str(mesh_fixture),
                str(adaptive_dryrun_sdfbin),
                "--max-level",
                "2",
                "--block-resolution",
                "5",
                "--dry-run",
                "--report",
                str(adaptive_dryrun_report),
            ],
        ),
        (
            "Adaptive Builder Preview CLI",
            [
                str(required_demo_tools["adasdf_build_adaptive_sdf_preview"]),
                str(mesh_fixture),
                str(adaptive_preview_sdfbin),
                "--target-error",
                "1e-3",
                "--memory-mb",
                "512",
                "--dry-run",
                "--plan",
                str(adaptive_preview_plan),
            ],
        ),
        (
            "Demo Surrogate Recommendation",
            [
                str(required_demo_tools["adasdf_recommend_demo"]),
                "--shape",
                "box",
                "--target-error",
                "1e-3",
                "--memory-mb",
                "64",
                "--block-memory-mb",
                "16",
                "--top-k",
                "5",
            ],
        ),
        (
            "Build Demo Adaptive SDFBin",
            [
                str(required_demo_tools["adasdf_build_demo_adaptive"]),
                str(demo_sdfbin),
                "--shape",
                "box",
                "--target-error",
                "1e-3",
                "--memory-mb",
                "64",
                "--block-memory-mb",
                "16",
                "--use-surrogate",
            ],
        ),
        (
            "Demo Adaptive Info CLI",
            [str(required_demo_tools["adasdf_info"]), str(demo_sdfbin)],
        ),
        (
            "Demo Adaptive Query CLI",
            [
                str(required_demo_tools["adasdf_query"]),
                str(demo_sdfbin),
                "--point",
                "0",
                "0",
                "0",
            ],
        ),
        (
            "Expansion Quality CLI",
            [
                str(required_demo_tools["adasdf_expansion_quality"]),
                str(demo_sdfbin),
                "--expansion",
                "global",
                "--global-resolution",
                "32",
                "--samples",
                "1000",
                "--out",
                str(quality_csv),
            ],
        ),
        (
            "Demo Collide Boxes CLI",
            [
                str(required_demo_tools["adasdf_collide_boxes_demo"]),
                "--target-error",
                "1e-3",
                "--memory-mb",
                "64",
                "--offset",
                "0.25",
                "0",
                "0",
                "--max-contacts",
                str(max_contacts),
                "--view",
                str(collision_svg),
            ],
        ),
        (
            "Batch Query Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_batch_query"]),
                "--points",
                "10000,100000,1000000",
                "--query-backend",
                "cpu,cuda",
                "--out",
                str(benchmark_csv),
            ],
        ),
    ]

    for name, command in demo_steps:
        result = run_step(name, command, workspace)
        results.append(result)
        allowed_returncodes = (0, 2) if name in (
            "Mesh Cleanup CLI",
            "Cleaned Mesh Check CLI",
        ) else (0,)
        if result.returncode not in allowed_returncodes:
            write_report(report_path, results, source, build, config)
            return result.returncode
        if name == "Capabilities CLI":
            required_lines = [
                "AdaSDF-CL version:",
                "Implemented:",
                "Experimental / partial:",
                "Planned:",
                "docs/capability_matrix.md",
            ]
            missing_lines = [
                line for line in required_lines if line not in result.output
            ]
            if missing_lines:
                result.returncode = 1
                result.output += (
                    "\nValidation failed: capabilities output missing: "
                    + ", ".join(missing_lines)
                    + "\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Capability Walkthrough Example":
            required_lines = [
                "AdaSDF-CL capability walkthrough",
                "Point phi:",
                "Collision hit:",
                "Expansion p95 abs error:",
                "Status: ok",
            ]
            missing_lines = [
                line for line in required_lines if line not in result.output
            ]
            if missing_lines:
                result.returncode = 1
                result.output += (
                    "\nValidation failed: walkthrough output missing: "
                    + ", ".join(missing_lines)
                    + "\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Mesh Diagnostics CLI":
            if not mesh_report.exists() or not mesh_json.exists():
                result.returncode = 1
                result.output += "\nValidation failed: mesh reports were not generated.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Mesh Cleanup CLI":
            if result.returncode not in (0, 2):
                write_report(report_path, results, source, build, config)
                return result.returncode
            if not cleaned_stl.exists() or not cleanup_report.exists():
                result.returncode = 1
                result.output += "\nValidation failed: cleanup output files were not generated.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
            report_text = cleanup_report.read_text(encoding="utf-8", errors="replace")
            required_lines = [
                "AdaSDF-CL safe mesh cleanup",
                "Removed degenerate triangles:",
                "Removed duplicate triangles:",
                "After readiness:",
            ]
            missing_lines = [
                line for line in required_lines if line not in result.output
            ]
            if missing_lines or "Before Diagnostics" not in report_text or "After Readiness" not in report_text:
                result.returncode = 1
                result.output += (
                    "\nValidation failed: mesh cleanup output missing: "
                    + ", ".join(missing_lines)
                    + "\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode
            if result.returncode == 2:
                result.returncode = 0
                result.output += "\nValidation note: mesh remained below Ready/Usable readiness after safe cleanup.\n"
        if name == "Cleaned Mesh Check CLI":
            if result.returncode not in (0, 2):
                write_report(report_path, results, source, build, config)
                return result.returncode
            if not cleaned_check_report.exists() or "SDF build readiness:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: cleaned mesh check output/report is missing.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
            required_lines = [
                "AdaSDF-CL mesh diagnostics",
                "Format: ascii",
                "SDF build readiness:",
                "Score:",
                "Boundary edges:",
                "Recommendation:",
            ]
            missing_lines = [
                line for line in required_lines if line not in result.output
            ]
            report_text = cleaned_check_report.read_text(encoding="utf-8", errors="replace")
            if (
                missing_lines
                or "Watertight" not in report_text
                or "SDF Build Readiness" not in report_text
                or "Score" not in report_text
            ):
                result.returncode = 1
                result.output += (
                    "\nValidation failed: cleaned mesh diagnostics output missing: "
                    + ", ".join(missing_lines)
                    + "\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode
            if result.returncode == 2:
                result.returncode = 0
                result.output += "\nValidation note: cleaned mesh still has readiness warnings/errors.\n"
        if name == "DenseSDF Build CLI":
            if (
                not dense_sdfbin.exists()
                or not dense_report.exists()
                or not dense_json.exists()
                or "Reload validation: success" not in result.output
                or "Dense Grid" not in dense_report.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: DenseSDF build output/report is missing.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "DenseSDF Info CLI":
            if "ADASDF_DENSE_SDFBIN_V1" not in result.output or "DenseSDF resolution:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: DenseSDF info output is missing format/resolution.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "DenseSDF Query CLI":
            if "Signed distance:" not in result.output or "Query backend:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: DenseSDF query output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "DenseSDF Collide CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > 4:
                result.returncode = 1
                result.output += "\nValidation failed: DenseSDF collide contact count is missing or too high.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Build CLI":
            if (
                not adaptive_sdfbin.exists()
                or not adaptive_report.exists()
                or not adaptive_json.exists()
                or "Reload validation: success" not in result.output
                or "Block Stats" not in adaptive_report.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF build output/report is missing.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Info CLI":
            if "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1" not in result.output or "AdaptiveBlockSDF block_count:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF info output is missing format/block count.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Query CLI":
            if "Signed distance:" not in result.output or "Query backend:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF query output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Collide CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > 4:
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF collide contact count is missing or too high.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Expansion Quality CLI":
            if (
                not adaptive_quality_csv.exists()
                or "Status: ok" not in result.output
                or "max_abs_error" not in adaptive_quality_csv.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF expansion quality output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Benchmark Model CLI":
            if (
                not adaptive_benchmark_csv.exists()
                or "query_backend" not in adaptive_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                or "cpu,none,all,1000" not in adaptive_benchmark_csv.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF benchmark --model output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF Compress CLI":
            if (
                not compressed_sdfbin.exists()
                or not compressed_report.exists()
                or not compressed_json.exists()
                or not compressed_quality_report.exists()
                or "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1" not in result.output
                or "Reload validation: success" not in result.output
                or "Tucker/HOSVD compression: planned / not implemented" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: compressed SDF compression output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF Info CLI":
            if "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1" not in result.output or "CompressedBlockSDF block_count:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: compressed SDF info output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF Query CLI":
            if "Signed distance:" not in result.output or "compressed adaptive block SDF backend" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: compressed SDF query output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF Collide CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > 4:
                result.returncode = 1
                result.output += "\nValidation failed: compressed SDF collide output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF Expansion Quality CLI":
            if (
                not compressed_quality_csv.exists()
                or "Status: ok" not in result.output
                or "max_abs_error" not in compressed_quality_csv.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: compressed SDF expansion quality output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF Benchmark Model CLI":
            csv_text = compressed_benchmark_csv.read_text(encoding="utf-8", errors="replace") if compressed_benchmark_csv.exists() else ""
            if (
                not compressed_benchmark_csv.exists()
                or not csv_text.splitlines()
                or "query_backend" not in csv_text.splitlines()[0]
                or "cpu,none,all,1000" not in csv_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: compressed SDF benchmark --model output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CompressedSDF One-Step Build CLI":
            if (
                not compressed_direct_sdfbin.exists()
                or not compressed_direct_report.exists()
                or not compressed_direct_compression_report.exists()
                or not compressed_direct_quality_report.exists()
                or "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1" not in result.output
                or "Reload validation: success" not in result.output
                or "Surrogate recommendation: planned for v1.8.0-alpha" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: one-step compressed SDF build output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "AdaptiveBlockSDF Dry Run CLI":
            if (
                adaptive_dryrun_sdfbin.exists()
                or not adaptive_dryrun_report.exists()
                or "Dry run: yes" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: AdaptiveBlockSDF dry-run output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Adaptive Builder Preview CLI":
            plan_text = (
                adaptive_preview_plan.read_text(encoding="utf-8", errors="replace")
                if adaptive_preview_plan.exists()
                else ""
            )
            if (
                adaptive_preview_sdfbin.exists()
                or not adaptive_preview_plan.exists()
                or "Use adasdf_build_adaptive_sdf" not in result.output
                or "LowRankCompression implemented in v1.7.0-alpha" not in plan_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: adaptive preview dry-run output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Demo Collide Boxes CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > max_contacts:
                result.returncode = 1
                result.output += (
                    "\nValidation failed: returned contacts were missing or "
                    "exceeded requested max contacts.\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Batch Query Benchmark":
            if not benchmark_csv.exists():
                result.returncode = 1
                result.output += "\nValidation failed: benchmark CSV was not generated.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
            csv_text = benchmark_csv.read_text(encoding="utf-8", errors="replace")
            required_fields = [
                "query_backend",
                "expansion_mode",
                "selected_blocks",
                "num_points",
                "expanded_memory_mb",
                "gpu_resident_memory_mb",
                "setup_ms",
                "query_kernel_ms",
                "query_total_ms",
                "ns_per_query",
                "queries_per_second",
                "fallback_count",
                "max_abs_phi_error",
                "max_normal_error",
                "cuda_available",
                "max_abs_error",
                "mean_abs_error",
                "rms_error",
                "p95_abs_error",
                "sign_mismatch_count",
                "sign_mismatch_rate",
                "ambiguous_sign_count",
                "ambiguous_sign_rate",
                "near_surface_sign_mismatch_rate",
                "fallback_rate",
                "status",
                "error_message",
            ]
            missing_fields = [
                field for field in required_fields if field not in csv_text.splitlines()[0]
            ]
            if missing_fields or "cpu,none,all,10000" not in csv_text:
                result.returncode = 1
                result.output += (
                    "\nValidation failed: benchmark CSV missing fields or CPU rows: "
                    + ", ".join(missing_fields)
                    + "\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode
            if "CUDA backend unavailable" in result.output:
                result.status = "PASS (CUDA SKIPPED)"
        if name == "Expansion Quality CLI":
            if not quality_csv.exists():
                result.returncode = 1
                result.output += "\nValidation failed: quality CSV was not generated.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
            csv_text = quality_csv.read_text(encoding="utf-8", errors="replace")
            required_fields = [
                "max_abs_error",
                "mean_abs_error",
                "rms_error",
                "p95_abs_error",
                "sign_mismatch_count",
                "sign_mismatch_rate",
                "ambiguous_sign_count",
                "ambiguous_sign_rate",
                "near_surface_sign_mismatch_rate",
                "fallback_count",
                "fallback_rate",
                "worst_point_id",
            ]
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            missing_fields = [field for field in required_fields if field not in header]
            if missing_fields:
                result.returncode = 1
                result.output += (
                    "\nValidation failed: quality CSV missing fields: "
                    + ", ".join(missing_fields)
                    + "\n"
                )
                write_report(report_path, results, source, build, config)
                return result.returncode

    svg_status = 0
    svg_output = ""
    if not collision_svg.exists():
        svg_status = 1
        svg_output = "collision SVG was not generated."
    else:
        svg_text = collision_svg.read_text(encoding="utf-8", errors="replace")
        if "<svg" not in svg_text or "contact-marker" not in svg_text:
            svg_status = 1
            svg_output = "collision SVG is missing <svg or contact markers."
        else:
            svg_output = "collision SVG generated and contains contact markers."
    svg_result = StepResult(
        "Collision SVG Check",
        ["check-collision-svg", str(collision_svg)],
        svg_status,
        svg_output,
    )
    results.append(svg_result)
    if svg_result.returncode != 0:
        write_report(report_path, results, source, build, config)
        return svg_result.returncode

    clean_result = run_step(
        "Clean Check",
        [sys.executable, str(source / "scripts" / "check_repo_clean.py"), str(source)],
        workspace,
    )
    results.append(clean_result)
    if clean_result.returncode != 0:
        write_report(report_path, results, source, build, config)
        return clean_result.returncode

    write_report(report_path, results, source, build, config)
    print("Alpha validation: PASS")
    print("Report: reports/alpha_validation_summary.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

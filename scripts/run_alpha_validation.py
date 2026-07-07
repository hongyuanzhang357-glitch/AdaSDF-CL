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


def run_step(
    name: str,
    command: list[str],
    cwd: Path,
    env: dict[str, str] | None = None,
) -> StepResult:
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
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
    parser.add_argument("--report-root", default="")
    args = parser.parse_args()

    source = Path(args.source).resolve()
    build = Path(args.build).resolve()
    config = args.config
    workspace = source.parent
    report_root = (
        Path(args.report_root).resolve()
        if args.report_root
        else source / "reports"
    )
    report_path = report_root / "alpha_validation_summary.md"
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
        install_prefix = build.parent / f"{build.name}_install"
        install_result = run_step(
            "Install Validation",
            [
                sys.executable,
                str(source / "scripts" / "run_install_validation.py"),
                "--source",
                str(source),
                "--build",
                str(build),
                "--install",
                str(install_prefix),
                "--config",
                config,
                "--reuse-build",
                "--report-root",
                str(report_root),
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
    strict_compressed_direct_json = build.parent / "strict_closed_cube_compressed_direct.json"
    strict_world_sparse_json = build.parent / "strict_closed_cube_world_sparse_collision.json"
    strict_world_benchmark_json = build.parent / "strict_closed_cube_collision_world_benchmark.json"
    strict_sparse_query_json = build.parent / "strict_closed_cube_sparse_query.json"
    strict_solver_contacts_json = build.parent / "strict_closed_cube_solver_contacts.json"
    strict_batch_benchmark_json = build.parent / "strict_batch_query_benchmark.json"
    strict_hierarchical_benchmark_json = build.parent / "strict_hierarchical_sampling_benchmark.json"
    strict_report_list = build.parent / "strict_report_inputs.txt"
    strict_summary_csv = build.parent / "strict_run_summary.csv"
    sample_fixture = source / "tests" / "data" / "samples" / "cube_sparse_samples.csv"
    sparse_results_csv = build.parent / "closed_cube_sparse_results.csv"
    sparse_report = build.parent / "closed_cube_sparse_report.md"
    sparse_collision_report = build.parent / "closed_cube_sparse_collision.md"
    sparse_candidates_csv = build.parent / "closed_cube_contact_candidates.csv"
    sparse_candidates_report = build.parent / "closed_cube_contact_candidates.md"
    stabilized_contacts_csv = build.parent / "closed_cube_stabilized_solver_contacts.csv"
    stabilized_contacts_json = build.parent / "closed_cube_stabilized_solver_contacts.json"
    stabilized_contacts_report = build.parent / "closed_cube_stabilized_solver_contacts.md"
    solver_contacts_csv = build.parent / "closed_cube_solver_contacts.csv"
    solver_contacts_json = build.parent / "closed_cube_solver_contacts.json"
    solver_contacts_report = build.parent / "closed_cube_solver_contacts.md"
    solver_raw_candidates_csv = build.parent / "closed_cube_solver_raw_candidates.csv"
    contact_reduction_benchmark_csv = build.parent / "closed_cube_contact_reduction_benchmark.csv"
    contact_reduction_benchmark_json = build.parent / "closed_cube_contact_reduction_benchmark.json"
    contact_reduction_benchmark_report = build.parent / "closed_cube_contact_reduction_benchmark.md"
    sparse_benchmark_csv = build.parent / "closed_cube_sparse_benchmark.csv"
    active_blocks_csv = build.parent / "closed_cube_active_blocks.csv"
    active_blocks_report = build.parent / "closed_cube_active_blocks.md"
    active_blocks_json = build.parent / "closed_cube_active_blocks.json"
    active_query_csv = build.parent / "closed_cube_active_block_query.csv"
    active_query_report = build.parent / "closed_cube_active_block_query.md"
    block_cache_benchmark_csv = build.parent / "closed_cube_block_cache_benchmark.csv"
    block_cache_benchmark_report = build.parent / "closed_cube_block_cache_benchmark.md"
    block_lookup_benchmark_csv = build.parent / "closed_cube_block_lookup_benchmark.csv"
    block_lookup_benchmark_report = build.parent / "closed_cube_block_lookup_benchmark.md"
    cuda_active_query_csv = build.parent / "closed_cube_cuda_active_block_query.csv"
    cuda_active_query_report = build.parent / "closed_cube_cuda_active_block_query.md"
    cuda_block_cache_benchmark_csv = build.parent / "closed_cube_cuda_block_cache_benchmark.csv"
    cuda_block_cache_benchmark_report = build.parent / "closed_cube_cuda_block_cache_benchmark.md"
    world_scene_csv = build.parent / "closed_cube_collision_world_scene.csv"
    world_broadphase_csv = build.parent / "closed_cube_world_broadphase_pairs.csv"
    world_broadphase_report = build.parent / "closed_cube_world_broadphase.md"
    world_broadphase_json = build.parent / "closed_cube_world_broadphase.json"
    world_sparse_csv = build.parent / "closed_cube_world_sparse_collisions.csv"
    world_sparse_report = build.parent / "closed_cube_world_sparse_collision.md"
    world_sparse_json = build.parent / "closed_cube_world_sparse_collision.json"
    world_solver_csv = build.parent / "closed_cube_world_solver_contacts.csv"
    world_solver_report = build.parent / "closed_cube_world_solver_contacts.md"
    world_solver_json = build.parent / "closed_cube_world_solver_contacts.json"
    world_benchmark_csv = build.parent / "closed_cube_collision_world_benchmark.csv"
    world_benchmark_report = build.parent / "closed_cube_collision_world_benchmark.md"
    world_benchmark_json = build.parent / "closed_cube_collision_world_benchmark.json"
    hierarchical_benchmark_csv = build.parent / "closed_cube_hierarchical_sampling_benchmark.csv"
    hierarchical_benchmark_report = build.parent / "closed_cube_hierarchical_sampling_benchmark.md"
    hierarchical_benchmark_json = build.parent / "closed_cube_hierarchical_sampling_benchmark.json"
    recommendation_md = build.parent / "closed_cube_recommendation.md"
    recommendation_json = build.parent / "closed_cube_recommendation.json"
    adaptive_dryrun_sdfbin = build.parent / "closed_cube_adaptive_dryrun.sdfbin"
    adaptive_dryrun_report = build.parent / "closed_cube_adaptive_dryrun_plan.md"
    adaptive_preview_sdfbin = build.parent / "closed_cube_adaptive_preview.sdfbin"
    adaptive_preview_plan = build.parent / "closed_cube_adaptive_preview_plan.md"
    max_contacts = 8
    for generated in (adaptive_preview_sdfbin, adaptive_dryrun_sdfbin):
        if generated.exists():
            generated.unlink()
    world_scene_csv.parent.mkdir(parents=True, exist_ok=True)
    world_scene_csv.write_text(
        "object_id,name,model_path,samples_path,tx,ty,tz,qw,qx,qy,qz,group,mask,type,enabled\n"
        f"0,world_box_a,{compressed_direct_sdfbin.as_posix()},{sample_fixture.as_posix()},0,0,0,1,0,0,0,0x1,0xffffffff,dynamic,true\n"
        f"1,world_box_b,{compressed_direct_sdfbin.as_posix()},{sample_fixture.as_posix()},0,0,0,1,0,0,0,0x2,0xffffffff,dynamic,true\n"
        f"2,world_box_far,{compressed_direct_sdfbin.as_posix()},{sample_fixture.as_posix()},4,0,0,1,0,0,0,0x4,0xffffffff,dynamic,true\n",
        encoding="utf-8",
    )
    required_demo_tools = {
        "adasdf_capabilities": find_executable(build, "adasdf_capabilities", config),
        "adasdf_mesh_check": find_executable(build, "adasdf_mesh_check", config),
        "adasdf_mesh_clean": find_executable(build, "adasdf_mesh_clean", config),
        "adasdf_build_dense_sdf": find_executable(build, "adasdf_build_dense_sdf", config),
        "adasdf_build_adaptive_sdf": find_executable(build, "adasdf_build_adaptive_sdf", config),
        "adasdf_compress_adaptive_sdf": find_executable(build, "adasdf_compress_adaptive_sdf", config),
        "adasdf_build_compressed_sdf": find_executable(build, "adasdf_build_compressed_sdf", config),
        "adasdf_recommend_build": find_executable(build, "adasdf_recommend_build", config),
        "adasdf_build_adaptive_sdf_preview": find_executable(build, "adasdf_build_adaptive_sdf_preview", config),
        "adasdf_recommend_demo": find_executable(build, "adasdf_recommend_demo", config),
        "adasdf_build_demo_adaptive": find_executable(build, "adasdf_build_demo_adaptive", config),
        "adasdf_info": find_executable(build, "adasdf_info", config),
        "adasdf_query": find_executable(build, "adasdf_query", config),
        "adasdf_collide": find_executable(build, "adasdf_collide", config),
        "adasdf_expansion_quality": find_executable(build, "adasdf_expansion_quality", config),
        "adasdf_sparse_query": find_executable(build, "adasdf_sparse_query", config),
        "adasdf_sparse_collide": find_executable(build, "adasdf_sparse_collide", config),
        "adasdf_contact_candidates": find_executable(build, "adasdf_contact_candidates", config),
        "adasdf_stabilize_contacts": find_executable(build, "adasdf_stabilize_contacts", config),
        "adasdf_solver_contact_candidates": find_executable(build, "adasdf_solver_contact_candidates", config),
        "adasdf_benchmark_sparse_query": find_executable(build, "adasdf_benchmark_sparse_query", config),
        "adasdf_select_active_blocks": find_executable(build, "adasdf_select_active_blocks", config),
        "adasdf_active_block_query": find_executable(build, "adasdf_active_block_query", config),
        "adasdf_benchmark_block_cache": find_executable(build, "adasdf_benchmark_block_cache", config),
        "adasdf_benchmark_block_lookup": find_executable(build, "adasdf_benchmark_block_lookup", config),
        "adasdf_cuda_active_block_query": find_executable(build, "adasdf_cuda_active_block_query", config),
        "adasdf_benchmark_cuda_block_cache": find_executable(build, "adasdf_benchmark_cuda_block_cache", config),
        "adasdf_benchmark_hierarchical_sampling": find_executable(build, "adasdf_benchmark_hierarchical_sampling", config),
        "adasdf_sweep_hierarchical_sampling": find_executable(build, "adasdf_sweep_hierarchical_sampling", config),
        "adasdf_benchmark_contact_reduction": find_executable(build, "adasdf_benchmark_contact_reduction", config),
        "adasdf_collide_boxes_demo": find_executable(build, "adasdf_collide_boxes_demo", config),
        "adasdf_benchmark_batch_query": find_executable(build, "adasdf_benchmark_batch_query", config),
        "adasdf_write_manifest": find_executable(build, "adasdf_write_manifest", config),
        "adasdf_validate_report": find_executable(build, "adasdf_validate_report", config),
        "adasdf_collect_run_summary": find_executable(build, "adasdf_collect_run_summary", config),
        "adasdf_capability_walkthrough": find_executable(build, "adasdf_capability_walkthrough", config),
        "adasdf_world_broadphase": find_executable(build, "adasdf_world_broadphase", config),
        "adasdf_world_sparse_collide": find_executable(build, "adasdf_world_sparse_collide", config),
        "adasdf_world_solver_contacts": find_executable(build, "adasdf_world_solver_contacts", config),
        "adasdf_benchmark_collision_world": find_executable(build, "adasdf_benchmark_collision_world", config),
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

    python_env = os.environ.copy()
    python_env["PYTHONPATH"] = str(source / "python")
    python_env["ADASDF_BIN"] = str(required_demo_tools["adasdf_capabilities"].parent)
    python_env["ADASDF_TEST_STL"] = str(mesh_fixture)
    python_env["ADASDF_TEST_SAMPLES"] = str(sample_fixture)
    python_env["PYTHONDONTWRITEBYTECODE"] = "1"
    python_result = run_step(
        "Python CLI Wrapper Tests",
        [sys.executable, "-m", "unittest", "discover", "-s", str(source / "python" / "tests")],
        workspace,
        env=python_env,
    )
    results.append(python_result)
    if python_result.returncode != 0:
        write_report(report_path, results, source, build, config)
        return python_result.returncode

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
            "Build Recommendation CLI",
            [
                str(required_demo_tools["adasdf_recommend_build"]),
                str(mesh_fixture),
                "--target-error",
                "1e-3",
                "--memory-mb",
                "256",
                "--use-case",
                "contact",
                "--out",
                str(recommendation_md),
                "--json",
                str(recommendation_json),
                "--emit-command",
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
                "--strict-json",
                str(strict_compressed_direct_json),
                "--case-id",
                "alpha_compressed_direct",
            ],
        ),
        (
            "Hierarchical Sampling Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_hierarchical_sampling"]),
                str(mesh_fixture),
                "--max-level",
                "1",
                "--block-resolution",
                "4",
                "--coarse-resolution",
                "2",
                "--quality-check-samples",
                "2",
                "--comparison-samples",
                "3",
                "--target-sampling-error",
                "10",
                "--csv",
                str(hierarchical_benchmark_csv),
                "--report",
                str(hierarchical_benchmark_report),
                "--json",
                str(hierarchical_benchmark_json),
                "--strict-json",
                str(strict_hierarchical_benchmark_json),
                "--case-id",
                "alpha_hierarchical_sampling_benchmark",
            ],
        ),
        (
            "CollisionWorld Broadphase CLI",
            [
                str(required_demo_tools["adasdf_world_broadphase"]),
                str(world_scene_csv),
                "--out",
                str(world_broadphase_csv),
                "--report",
                str(world_broadphase_report),
                "--json",
                str(world_broadphase_json),
            ],
        ),
        (
            "CollisionWorld Sparse Collide CLI",
            [
                str(required_demo_tools["adasdf_world_sparse_collide"]),
                str(world_scene_csv),
                "--threshold",
                "0",
                "--early-exit",
                "--out",
                str(world_sparse_csv),
                "--report",
                str(world_sparse_report),
                "--json",
                str(world_sparse_json),
                "--strict-json",
                str(strict_world_sparse_json),
                "--case-id",
                "alpha_world_sparse",
            ],
        ),
        (
            "CollisionWorld Solver Contacts CLI",
            [
                str(required_demo_tools["adasdf_world_solver_contacts"]),
                str(world_scene_csv),
                "--threshold",
                "1e-3",
                "--top-k",
                "32",
                "--max-contacts",
                str(max_contacts),
                "--out",
                str(world_solver_csv),
                "--report",
                str(world_solver_report),
                "--json",
                str(world_solver_json),
            ],
        ),
        (
            "CollisionWorld Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_collision_world"]),
                str(world_scene_csv),
                "--mode",
                "sparse",
                "--threshold",
                "1e-3",
                "--repeat",
                "2",
                "--warmup",
                "1",
                "--csv",
                str(world_benchmark_csv),
                "--report",
                str(world_benchmark_report),
                "--json",
                str(world_benchmark_json),
                "--strict-json",
                str(strict_world_benchmark_json),
                "--case-id",
                "alpha_world_benchmark",
            ],
        ),
        (
            "Sparse Query CLI",
            [
                str(required_demo_tools["adasdf_sparse_query"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "0",
                "--out",
                str(sparse_results_csv),
                "--report",
                str(sparse_report),
                "--strict-json",
                str(strict_sparse_query_json),
                "--case-id",
                "alpha_sparse_query",
            ],
        ),
        (
            "Sparse Collide CLI",
            [
                str(required_demo_tools["adasdf_sparse_collide"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "0",
                "--early-exit",
                "--report",
                str(sparse_collision_report),
            ],
        ),
        (
            "Contact Candidates CLI",
            [
                str(required_demo_tools["adasdf_contact_candidates"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--top-k",
                "4",
                "--threshold",
                "1e-3",
                "--reduction-radius",
                "0.02",
                "--with-normal",
                "--out",
                str(sparse_candidates_csv),
                "--report",
                str(sparse_candidates_report),
            ],
        ),
        (
            "Contact Stabilization CLI",
            [
                str(required_demo_tools["adasdf_stabilize_contacts"]),
                str(sparse_candidates_csv),
                "--max-contacts",
                str(max_contacts),
                "--patch-radius",
                "0.02",
                "--out",
                str(stabilized_contacts_csv),
                "--json",
                str(stabilized_contacts_json),
                "--report",
                str(stabilized_contacts_report),
            ],
        ),
        (
            "Solver Contact Candidates CLI",
            [
                str(required_demo_tools["adasdf_solver_contact_candidates"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1e-3",
                "--top-k",
                "32",
                "--max-contacts",
                str(max_contacts),
                "--patch-radius",
                "0.02",
                "--with-normal",
                "--out",
                str(solver_contacts_csv),
                "--candidates-out",
                str(solver_raw_candidates_csv),
                "--json",
                str(solver_contacts_json),
                "--report",
                str(solver_contacts_report),
                "--strict-json",
                str(strict_solver_contacts_json),
                "--case-id",
                "alpha_solver_contacts",
            ],
        ),
        (
            "Contact Reduction Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_contact_reduction"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1e-3",
                "--top-k",
                "64",
                "--max-contacts",
                str(max_contacts),
                "--patch-radius",
                "0.02",
                "--repeat",
                "2",
                "--warmup",
                "1",
                "--csv",
                str(contact_reduction_benchmark_csv),
                "--json",
                str(contact_reduction_benchmark_json),
                "--report",
                str(contact_reduction_benchmark_report),
            ],
        ),
        (
            "Sparse Query Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_sparse_query"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--repeat",
                "2",
                "--warmup",
                "1",
                "--mode",
                "phi-only",
                "--csv",
                str(sparse_benchmark_csv),
            ],
        ),
        (
            "Active Block Selection CLI",
            [
                str(required_demo_tools["adasdf_select_active_blocks"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1.0",
                "--selection-band",
                "0.1",
                "--extra-margin",
                "0.02",
                "--out",
                str(active_blocks_csv),
                "--report",
                str(active_blocks_report),
                "--json",
                str(active_blocks_json),
            ],
        ),
        (
            "Active Block Query CLI",
            [
                str(required_demo_tools["adasdf_active_block_query"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1.0",
                "--selection-band",
                "0.1",
                "--extra-margin",
                "0.02",
                "--with-normal",
                "--out",
                str(active_query_csv),
                "--report",
                str(active_query_report),
            ],
        ),
        (
            "Active Block Cache Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_block_cache"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--repeat",
                "2",
                "--warmup",
                "1",
                "--threshold",
                "1.0",
                "--selection-band",
                "0.1",
                "--compare-direct",
                "--csv",
                str(block_cache_benchmark_csv),
                "--report",
                str(block_cache_benchmark_report),
            ],
        ),
        (
            "Block Lookup Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_block_lookup"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--lookup",
                "linear,hash,morton",
                "--cache-lookup",
                "linear,hash,spatial-hash",
                "--repeat",
                "2",
                "--csv",
                str(block_lookup_benchmark_csv),
                "--report",
                str(block_lookup_benchmark_report),
                "--case-id",
                "alpha_block_lookup_benchmark",
            ],
        ),
        (
            "CUDA Active Block Query CLI",
            [
                str(required_demo_tools["adasdf_cuda_active_block_query"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1.0",
                "--selection-band",
                "0.1",
                "--extra-margin",
                "0.02",
                "--with-normal",
                "--out",
                str(cuda_active_query_csv),
                "--report",
                str(cuda_active_query_report),
            ],
        ),
        (
            "CUDA Active Block Cache Benchmark",
            [
                str(required_demo_tools["adasdf_benchmark_cuda_block_cache"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--repeat",
                "2",
                "--warmup",
                "0",
                "--threshold",
                "1.0",
                "--selection-band",
                "0.1",
                "--compare-direct",
                "--csv",
                str(cuda_block_cache_benchmark_csv),
                "--report",
                str(cuda_block_cache_benchmark_report),
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
                "--strict-json",
                str(strict_batch_benchmark_json),
                "--case-id",
                "alpha_batch_benchmark",
            ],
        ),
    ]

    for name, command in demo_steps:
        result = run_step(name, command, workspace)
        if name == "Sparse Collide CLI" and result.returncode == 10:
            result.returncode = 0
            result.output += (
                "\nValidation note: sparse_collide returned 10, which means "
                "collision detected and is expected for this fixture.\n"
            )
        if name == "CollisionWorld Sparse Collide CLI" and result.returncode == 10:
            result.returncode = 0
            result.output += (
                "\nValidation note: world_sparse_collide returned 10, which means "
                "collision detected and is expected for this fixture.\n"
            )
        if name == "Active Block Query CLI" and result.returncode == 10:
            result.returncode = 0
            result.output += (
                "\nValidation note: active_block_query returned 10, which means "
                "collision detected and is expected for this fixture.\n"
            )
        if name == "CUDA Active Block Query CLI" and result.returncode in (10, 20):
            if result.returncode == 10:
                result.output += (
                    "\nValidation note: cuda_active_block_query returned 10, which means "
                    "collision detected and is expected for this fixture.\n"
                )
            else:
                result.output += (
                    "\nValidation note: cuda_active_block_query returned 20, which means "
                    "CUDA was unavailable and the run was skipped as expected.\n"
                )
                result.status = "PASS (CUDA SKIPPED)"
            result.returncode = 0
        if name == "CUDA Active Block Cache Benchmark" and result.returncode == 20:
            result.returncode = 0
            result.status = "PASS (CUDA SKIPPED)"
            result.output += (
                "\nValidation note: benchmark_cuda_block_cache returned 20, which means "
                "CUDA was unavailable and the run was skipped as expected.\n"
            )
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
        if name == "Build Recommendation CLI":
            md_text = (
                recommendation_md.read_text(encoding="utf-8", errors="replace")
                if recommendation_md.exists()
                else ""
            )
            json_text = (
                recommendation_json.read_text(encoding="utf-8", errors="replace")
                if recommendation_json.exists()
                else ""
            )
            if (
                not recommendation_md.exists()
                or not recommendation_json.exists()
                or "AdaSDF-CL build recommender" not in result.output
                or "Recommended path:" not in result.output
                or "adasdf_build_" not in result.output
                or "Build executed: no" not in result.output
                or "Recommended path" not in md_text
                or "CLI command" not in md_text
                or "not a universal trained model" not in md_text
                or "recommended_recipe" not in json_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: build recommendation output/report is missing.\n"
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
                or "adasdf_recommend_build" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: one-step compressed SDF build output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Hierarchical Sampling Benchmark":
            csv_text = (
                hierarchical_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if hierarchical_benchmark_csv.exists()
                else ""
            )
            json_text = (
                hierarchical_benchmark_json.read_text(encoding="utf-8", errors="replace")
                if hierarchical_benchmark_json.exists()
                else ""
            )
            report_text = (
                hierarchical_benchmark_report.read_text(encoding="utf-8", errors="replace")
                if hierarchical_benchmark_report.exists()
                else ""
            )
            if (
                not hierarchical_benchmark_csv.exists()
                or not hierarchical_benchmark_report.exists()
                or not hierarchical_benchmark_json.exists()
                or "AdaSDF-CL hierarchical sampling benchmark" not in result.output
                or "exact_build_time_ms,hierarchical_build_time_ms" not in csv_text
                or "quality_gate_passed" not in json_text
                or "Hierarchical Adaptive Sampling Benchmark" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: hierarchical sampling benchmark output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CollisionWorld Broadphase CLI":
            csv_text = (
                world_broadphase_csv.read_text(encoding="utf-8", errors="replace")
                if world_broadphase_csv.exists()
                else ""
            )
            report_text = (
                world_broadphase_report.read_text(encoding="utf-8", errors="replace")
                if world_broadphase_report.exists()
                else ""
            )
            json_text = (
                world_broadphase_json.read_text(encoding="utf-8", errors="replace")
                if world_broadphase_json.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            if (
                not world_broadphase_csv.exists()
                or not world_broadphase_report.exists()
                or not world_broadphase_json.exists()
                or "Overlap pairs:" not in result.output
                or "pair_id" not in header
                or "CollisionWorld Broadphase Report" not in report_text
                or "overlap_pair_count" not in json_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: CollisionWorld broadphase output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CollisionWorld Sparse Collide CLI":
            csv_text = (
                world_sparse_csv.read_text(encoding="utf-8", errors="replace")
                if world_sparse_csv.exists()
                else ""
            )
            report_text = (
                world_sparse_report.read_text(encoding="utf-8", errors="replace")
                if world_sparse_report.exists()
                else ""
            )
            json_text = (
                world_sparse_json.read_text(encoding="utf-8", errors="replace")
                if world_sparse_json.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            if (
                not world_sparse_csv.exists()
                or not world_sparse_report.exists()
                or not world_sparse_json.exists()
                or "Colliding: true" not in result.output
                or "Sample-based SDF collision" not in result.output
                or "effective_phi" not in header
                or "CollisionWorld Sparse Collision Report" not in report_text
                or "violation_count" not in json_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: CollisionWorld sparse collision output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CollisionWorld Solver Contacts CLI":
            csv_text = (
                world_solver_csv.read_text(encoding="utf-8", errors="replace")
                if world_solver_csv.exists()
                else ""
            )
            report_text = (
                world_solver_report.read_text(encoding="utf-8", errors="replace")
                if world_solver_report.exists()
                else ""
            )
            json_text = (
                world_solver_json.read_text(encoding="utf-8", errors="replace")
                if world_solver_json.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            match = re.search(r"Solver contacts:\s+(\d+)", result.output)
            if (
                not world_solver_csv.exists()
                or not world_solver_report.exists()
                or not world_solver_json.exists()
                or not match
                or int(match.group(1)) > max_contacts
                or "solver-ready candidates, not solver constraints" not in result.output
                or "stable_key" not in header
                or "CollisionWorld Solver-Ready Contacts" not in report_text
                or "solver_contact_count" not in json_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: CollisionWorld solver contacts output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CollisionWorld Benchmark":
            csv_text = (
                world_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if world_benchmark_csv.exists()
                else ""
            )
            report_text = (
                world_benchmark_report.read_text(encoding="utf-8", errors="replace")
                if world_benchmark_report.exists()
                else ""
            )
            json_text = (
                world_benchmark_json.read_text(encoding="utf-8", errors="replace")
                if world_benchmark_json.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            if (
                not world_benchmark_csv.exists()
                or not world_benchmark_report.exists()
                or not world_benchmark_json.exists()
                or "avg_total_ms:" not in result.output
                or "broadphase_pairs" not in header
                or "CollisionWorld Benchmark" not in report_text
                or "avg_total_ms" not in json_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: CollisionWorld benchmark output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Sparse Query CLI":
            csv_text = (
                sparse_results_csv.read_text(encoding="utf-8", errors="replace")
                if sparse_results_csv.exists()
                else ""
            )
            report_text = (
                sparse_report.read_text(encoding="utf-8", errors="replace")
                if sparse_report.exists()
                else ""
            )
            if (
                not sparse_results_csv.exists()
                or not sparse_report.exists()
                or "Sample count:" not in result.output
                or "Colliding:" not in result.output
                or "effective_phi" not in csv_text
                or "Sparse SDF Query" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: sparse query output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Sparse Collide CLI":
            report_text = (
                sparse_collision_report.read_text(encoding="utf-8", errors="replace")
                if sparse_collision_report.exists()
                else ""
            )
            if (
                not sparse_collision_report.exists()
                or "Colliding: true" not in result.output
                or "Return code note: 10 means collision detected" not in result.output
                or "Sparse Collision Query" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: sparse collide output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Contact Candidates CLI":
            csv_text = (
                sparse_candidates_csv.read_text(encoding="utf-8", errors="replace")
                if sparse_candidates_csv.exists()
                else ""
            )
            report_text = (
                sparse_candidates_report.read_text(encoding="utf-8", errors="replace")
                if sparse_candidates_report.exists()
                else ""
            )
            if (
                not sparse_candidates_csv.exists()
                or not sparse_candidates_report.exists()
                or "Reduced candidates:" not in result.output
                or "rank" not in csv_text.splitlines()[0]
                or "Contact Candidate Report" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: contact candidates output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Contact Stabilization CLI":
            csv_text = (
                stabilized_contacts_csv.read_text(encoding="utf-8", errors="replace")
                if stabilized_contacts_csv.exists()
                else ""
            )
            json_text = (
                stabilized_contacts_json.read_text(encoding="utf-8", errors="replace")
                if stabilized_contacts_json.exists()
                else ""
            )
            report_text = (
                stabilized_contacts_report.read_text(encoding="utf-8", errors="replace")
                if stabilized_contacts_report.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            match = re.search(r"Output solver contacts:\s+(\d+)", result.output)
            if (
                not stabilized_contacts_csv.exists()
                or not stabilized_contacts_json.exists()
                or not stabilized_contacts_report.exists()
                or not match
                or int(match.group(1)) > max_contacts
                or "stable_key" not in header
                or "contact_count" not in json_text
                or "Contact Stabilization Report" not in report_text
                or "not solver impulses" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: contact stabilization output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Solver Contact Candidates CLI":
            csv_text = (
                solver_contacts_csv.read_text(encoding="utf-8", errors="replace")
                if solver_contacts_csv.exists()
                else ""
            )
            raw_text = (
                solver_raw_candidates_csv.read_text(encoding="utf-8", errors="replace")
                if solver_raw_candidates_csv.exists()
                else ""
            )
            json_text = (
                solver_contacts_json.read_text(encoding="utf-8", errors="replace")
                if solver_contacts_json.exists()
                else ""
            )
            report_text = (
                solver_contacts_report.read_text(encoding="utf-8", errors="replace")
                if solver_contacts_report.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            raw_header = raw_text.splitlines()[0] if raw_text.splitlines() else ""
            match = re.search(r"Solver contacts:\s+(\d+)", result.output)
            if (
                not solver_contacts_csv.exists()
                or not solver_contacts_json.exists()
                or not solver_contacts_report.exists()
                or not solver_raw_candidates_csv.exists()
                or not match
                or int(match.group(1)) > max_contacts
                or "Raw candidates:" not in result.output
                or "Patches:" not in result.output
                or "stable_key" not in header
                or "rank" not in raw_header
                or "contact_count" not in json_text
                or "Solver-Ready Contact Candidates" not in report_text
                or "No impulses or friction forces are computed" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: solver contact candidates output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Contact Reduction Benchmark":
            csv_text = (
                contact_reduction_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if contact_reduction_benchmark_csv.exists()
                else ""
            )
            json_text = (
                contact_reduction_benchmark_json.read_text(encoding="utf-8", errors="replace")
                if contact_reduction_benchmark_json.exists()
                else ""
            )
            report_text = (
                contact_reduction_benchmark_report.read_text(encoding="utf-8", errors="replace")
                if contact_reduction_benchmark_report.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            match = re.search(r"solver_contact_count:\s+(\d+)", result.output)
            if (
                not contact_reduction_benchmark_csv.exists()
                or not contact_reduction_benchmark_json.exists()
                or not contact_reduction_benchmark_report.exists()
                or not match
                or int(match.group(1)) > max_contacts
                or "avg_reduction_ms:" not in result.output
                or "candidate_reduction_ratio:" not in result.output
                or "avg_reduction_ms" not in header
                or "candidate_reduction_ratio" not in header
                or "avg_reduction_ms" not in json_text
                or "Contact Reduction Benchmark" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: contact reduction benchmark output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Sparse Query Benchmark":
            csv_text = (
                sparse_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if sparse_benchmark_csv.exists()
                else ""
            )
            if (
                not sparse_benchmark_csv.exists()
                or "Average ns per sample:" not in result.output
                or "avg_ns_per_sample" not in csv_text.splitlines()[0]
                or "phi-only" not in csv_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: sparse benchmark output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Active Block Selection CLI":
            csv_text = (
                active_blocks_csv.read_text(encoding="utf-8", errors="replace")
                if active_blocks_csv.exists()
                else ""
            )
            report_text = (
                active_blocks_report.read_text(encoding="utf-8", errors="replace")
                if active_blocks_report.exists()
                else ""
            )
            if (
                not active_blocks_csv.exists()
                or not active_blocks_report.exists()
                or not active_blocks_json.exists()
                or "Active blocks:" not in result.output
                or "block_id" not in csv_text.splitlines()[0]
                or "Active Block Selection Report" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: active block selection output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Active Block Query CLI":
            csv_text = (
                active_query_csv.read_text(encoding="utf-8", errors="replace")
                if active_query_csv.exists()
                else ""
            )
            report_text = (
                active_query_report.read_text(encoding="utf-8", errors="replace")
                if active_query_report.exists()
                else ""
            )
            if (
                not active_query_csv.exists()
                or not active_query_report.exists()
                or "Status: ok" not in result.output
                or "Colliding: true" not in result.output
                or "source" not in csv_text.splitlines()[0]
                or "Active Block Query Report" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: active block query output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Active Block Cache Benchmark":
            csv_text = (
                block_cache_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if block_cache_benchmark_csv.exists()
                else ""
            )
            if (
                not block_cache_benchmark_csv.exists()
                or not block_cache_benchmark_report.exists()
                or "Average ns per sample:" not in result.output
                or "active_block_avg_ms" not in csv_text.splitlines()[0]
                or "direct_avg_ms" not in csv_text.splitlines()[0]
            ):
                result.returncode = 1
                result.output += "\nValidation failed: active block cache benchmark output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "Block Lookup Benchmark":
            csv_text = (
                block_lookup_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if block_lookup_benchmark_csv.exists()
                else ""
            )
            if (
                not block_lookup_benchmark_csv.exists()
                or not block_lookup_benchmark_report.exists()
                or "Status: ok" not in result.output
                or "speedup_vs_linear" not in csv_text.splitlines()[0]
                or "lookup_result_mismatch_count" not in csv_text.splitlines()[0]
                or "performance_claim_allowed" not in csv_text.splitlines()[0]
            ):
                result.returncode = 1
                result.output += "\nValidation failed: block lookup benchmark output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CUDA Active Block Query CLI":
            csv_text = (
                cuda_active_query_csv.read_text(encoding="utf-8", errors="replace")
                if cuda_active_query_csv.exists()
                else ""
            )
            report_text = (
                cuda_active_query_report.read_text(encoding="utf-8", errors="replace")
                if cuda_active_query_report.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            if (
                not cuda_active_query_csv.exists()
                or not cuda_active_query_report.exists()
                or "CUDA available:" not in result.output
                or not ("Status: ok" in result.output or "Status: skipped" in result.output)
                or "source" not in header
                or "CUDA Active Block Query Report" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: CUDA active block query output is incomplete.\n"
                write_report(report_path, results, source, build, config)
                return result.returncode
        if name == "CUDA Active Block Cache Benchmark":
            csv_text = (
                cuda_block_cache_benchmark_csv.read_text(encoding="utf-8", errors="replace")
                if cuda_block_cache_benchmark_csv.exists()
                else ""
            )
            report_text = (
                cuda_block_cache_benchmark_report.read_text(encoding="utf-8", errors="replace")
                if cuda_block_cache_benchmark_report.exists()
                else ""
            )
            header = csv_text.splitlines()[0] if csv_text.splitlines() else ""
            if (
                not cuda_block_cache_benchmark_csv.exists()
                or not cuda_block_cache_benchmark_report.exists()
                or "CUDA available:" not in result.output
                or not ("Status: ok" in result.output or "Status: skipped" in result.output)
                or "cuda_kernel_avg_ms" not in header
                or "cuda_available" not in header
                or "CUDA Active Block Cache Benchmark" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: CUDA block cache benchmark output is incomplete.\n"
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
                or "SurrogateRecommendation implemented in v1.8.0-alpha" not in plan_text
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

    strict_reports = [
        strict_compressed_direct_json,
        strict_world_sparse_json,
        strict_world_benchmark_json,
        strict_sparse_query_json,
        strict_solver_contacts_json,
        strict_batch_benchmark_json,
        strict_hierarchical_benchmark_json,
    ]
    for strict_report in strict_reports:
        result = run_step(
            f"Strict Report Validate {strict_report.name}",
            [str(required_demo_tools["adasdf_validate_report"]), str(strict_report)],
            workspace,
        )
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, config)
            return result.returncode
    strict_report_list.write_text(
        "\n".join(path.name for path in strict_reports) + "\n",
        encoding="utf-8",
    )
    result = run_step(
        "Strict Report Summary CSV",
        [
            str(required_demo_tools["adasdf_collect_run_summary"]),
            "--inputs",
            str(strict_report_list),
            "--out",
            str(strict_summary_csv),
        ],
        strict_report_list.parent,
    )
    results.append(result)
    if result.returncode != 0 or not strict_summary_csv.exists():
        if result.returncode == 0:
            result.returncode = 1
            result.output += "\nValidation failed: strict summary CSV was not created.\n"
        write_report(report_path, results, source, build, config)
        return result.returncode

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

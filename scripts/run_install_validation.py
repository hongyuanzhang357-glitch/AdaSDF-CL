#!/usr/bin/env python3
"""Validate that an installed AdaSDF-CL package is consumable downstream."""

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


def resolve_path(value: str, cwd: Path) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = cwd / path
    return path.resolve()


def sanitize_output(output: str, source: Path, build: Path, install: Path) -> str:
    replacements = {
        str(source): "<source>",
        source.as_posix(): "<source>",
        str(build): "<build>",
        build.as_posix(): "<build>",
        str(install): "<install>",
        install.as_posix(): "<install>",
        str(source.parent): "<workspace>",
        source.parent.as_posix(): "<workspace>",
    }
    sanitized = output
    for needle, replacement in sorted(replacements.items(), key=lambda item: len(item[0]), reverse=True):
        sanitized = sanitized.replace(needle, replacement)
    sanitized = re.sub(r"[A-Za-z]:[/\\]Users[/\\]\S+", "<local-path>", sanitized)
    return sanitized


def display_command(command: list[str], source: Path, build: Path, install: Path) -> str:
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
                elif path == install:
                    text = "<install>"
                elif source in path.parents:
                    text = str(Path("<source>") / path.relative_to(source)).replace("\\", "/")
                elif build in path.parents:
                    text = str(Path("<build>") / path.relative_to(build)).replace("\\", "/")
                elif install in path.parents:
                    text = str(Path("<install>") / path.relative_to(install)).replace("\\", "/")
                elif source.parent in path.parents:
                    text = str(Path("<workspace>") / path.relative_to(source.parent)).replace("\\", "/")
                else:
                    text = "<local-path>"
        except (OSError, ValueError):
            pass
        text = sanitize_output(text, source, build, install)
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


def brief_output(output: str, max_lines: int = 20) -> str:
    lines = [line.rstrip() for line in output.splitlines()]
    if len(lines) <= max_lines:
        return "\n".join(lines)
    return "\n".join(lines[:max_lines] + ["..."])


def cmake_build_type_args(config: str) -> list[str]:
    generator = os.environ.get("CMAKE_GENERATOR", "").lower()
    multi_config_terms = ("multi-config", "visual studio", "xcode")
    if any(term in generator for term in multi_config_terms):
        return []
    if sys.platform.startswith("win") and not generator:
        return []
    return [f"-DCMAKE_BUILD_TYPE={config}"]


def cmake_build_command(build: Path, config: str, parallel: bool = True) -> list[str]:
    command = ["cmake", "--build", str(build), "--config", config]
    if parallel:
        command.append("--parallel")
    generator = os.environ.get("CMAKE_GENERATOR", "").lower()
    if sys.platform.startswith("win") and (not generator or "visual studio" in generator):
        command.extend(["--", "/nodeReuse:false"])
    return command


def find_executable(build: Path, target_name: str, config: str) -> Path:
    target_names = [target_name, f"{target_name}.exe"]
    if sys.platform.startswith("win"):
        target_names.reverse()

    candidates = [
        *(build / config / target for target in target_names),
        *(build / target for target in target_names),
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate

    for target in target_names:
        matches = [path for path in build.rglob(target) if path.is_file()]
        if matches:
            return matches[0]

    checked = "\n".join(f"- {candidate}" for candidate in candidates)
    raise FileNotFoundError(
        f"Could not locate executable target '{target_name}' under {build}.\n"
        f"Checked:\n{checked}"
    )


def print_failure(
    result: StepResult,
    source: Path,
    build: Path,
    install: Path,
) -> None:
    print(f"Install validation failed during: {result.name}", file=sys.stderr)
    print(
        "Command: "
        + display_command(result.command, source, build, install),
        file=sys.stderr,
    )
    if result.output.strip():
        print("Output:", file=sys.stderr)
        print(
            brief_output(sanitize_output(result.output, source, build, install), max_lines=80),
            file=sys.stderr,
        )


def write_report(
    report: Path,
    results: list[StepResult],
    source: Path,
    build: Path,
    install: Path,
    config: str,
) -> None:
    report.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Install Validation Summary",
        "",
        "- Source: `<source>`",
        "- Build: `<build>`",
        "- Install: `<install>`",
        f"- Config: `{config}`",
        "",
        "## Results",
        "",
    ]
    for result in results:
        status = "PASS" if result.returncode == 0 else "FAIL"
        lines.extend(
            [
                f"### {result.name}: {status}",
                "",
                "```bash",
                display_command(result.command, source, build, install),
                "```",
                "",
            ]
        )
        if result.output.strip():
            lines.extend(
                [
                    "```text",
                    brief_output(sanitize_output(result.output, source, build, install)),
                    "```",
                    "",
                ]
            )
    report.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", default=".")
    parser.add_argument("--build", default="../build/adasdf_cl_install_validation")
    parser.add_argument("--install", default="../build/adasdf_cl_install_tree")
    parser.add_argument("--package-build", default="")
    parser.add_argument("--downstream-build", default="")
    parser.add_argument("--config", default="Debug")
    parser.add_argument(
        "--reuse-build",
        action="store_true",
        help="Reuse an existing configured and built source tree for install validation.",
    )
    args = parser.parse_args()

    source = resolve_path(args.source, Path.cwd())
    build = resolve_path(args.build, Path.cwd())
    install = resolve_path(args.install, Path.cwd())
    package_build = (
        resolve_path(args.package_build, Path.cwd())
        if args.package_build
        else build.parent / f"{build.name}_pkg"
    )
    downstream_build = (
        resolve_path(args.downstream_build, Path.cwd())
        if args.downstream_build
        else build.parent / f"{build.name}_ds"
    )
    config = args.config
    report_path = source / "reports" / "install_validation_summary.md"
    if report_path.exists():
        report_path.unlink()

    workspace = source.parent
    package_source = source / "tests" / "package"
    downstream_source = source / "examples" / "downstream_cmake_project"

    steps: list[tuple[str, list[str]]]
    if args.reuse_build:
        steps = [
            (
                "Reuse Existing Build",
                [
                    sys.executable,
                    "-c",
                    (
                        "from pathlib import Path; "
                        f"build = Path(r'''{build}'''); "
                        "cache = build / 'CMakeCache.txt'; "
                        "print('Reusing existing build:', build); "
                        "raise SystemExit(0 if cache.is_file() else 1)"
                    ),
                ],
            )
        ]
    else:
        steps = [
            (
                "Configure",
                [
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
                    "-DADASDF_CL_ENABLE_CUDA=OFF",
                    f"-DCMAKE_INSTALL_PREFIX={install}",
                    *cmake_build_type_args(config),
                ],
            ),
            ("Build", cmake_build_command(build, config)),
        ]
    steps.extend(
        [
            ("Install", ["cmake", "--install", str(build), "--config", config, "--prefix", str(install)]),
            (
                "Package Configure",
                [
                    "cmake",
                    "-S",
                    str(package_source),
                    "-B",
                    str(package_build),
                    f"-DCMAKE_PREFIX_PATH={install}",
                    *cmake_build_type_args(config),
                ],
            ),
            ("Package Build", cmake_build_command(package_build, config)),
        ]
    )

    results: list[StepResult] = []
    for name, command in steps:
        print(f"[install-validation] {name}", flush=True)
        result = run_step(name, command, workspace)
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, install, config)
            print_failure(result, source, build, install)
            return result.returncode

    python_env = os.environ.copy()
    python_env["PYTHONPATH"] = str(source / "python")
    python_env["ADASDF_BIN"] = str(install / "bin")
    python_env["ADASDF_TEST_STL"] = str(
        source / "tests" / "data" / "mesh_diagnostics" / "closed_cube_ascii.stl"
    )
    python_env["ADASDF_TEST_SAMPLES"] = str(
        source / "tests" / "data" / "samples" / "cube_sparse_samples.csv"
    )
    python_env["PYTHONDONTWRITEBYTECODE"] = "1"
    print("[install-validation] Python CLI Wrapper Tests", flush=True)
    result = run_step(
        "Python CLI Wrapper Tests",
        [sys.executable, "-m", "unittest", "discover", "-s", str(source / "python" / "tests")],
        workspace,
        env=python_env,
    )
    results.append(result)
    if result.returncode != 0:
        write_report(report_path, results, source, build, install, config)
        print_failure(result, source, build, install)
        return result.returncode

    try:
        capabilities_exe = find_executable(install, "adasdf_capabilities", config)
    except FileNotFoundError as exc:
        result = StepResult("Installed Capabilities CLI", ["adasdf_capabilities"], 1, str(exc))
    else:
        print("[install-validation] Installed Capabilities CLI", flush=True)
        result = run_step(
            "Installed Capabilities CLI",
            [str(capabilities_exe), "--verbose"],
            workspace,
        )
        if (
            result.returncode == 0
            and "AdaSDF-CL version:" not in result.output
        ):
            result.returncode = 1
            result.output += "\nValidation failed: installed capabilities CLI output is missing version.\n"
    results.append(result)
    if result.returncode != 0:
        write_report(report_path, results, source, build, install, config)
        print_failure(result, source, build, install)
        return result.returncode

    try:
        mesh_check_exe = find_executable(install, "adasdf_mesh_check", config)
    except FileNotFoundError as exc:
        result = StepResult("Installed Mesh Check CLI", ["adasdf_mesh_check"], 1, str(exc))
    else:
        print("[install-validation] Installed Mesh Check CLI", flush=True)
        mesh_fixture = source / "tests" / "data" / "mesh_diagnostics" / "closed_cube_ascii.stl"
        mesh_report = build.parent / "install_validation_mesh_report.md"
        usage = run_step(
            "Installed Mesh Check CLI",
            [str(mesh_check_exe)],
            workspace,
        )
        result = usage
        if result.returncode == 0:
            result = run_step(
                "Installed Mesh Check CLI",
                [
                    str(mesh_check_exe),
                    str(mesh_fixture),
                    "--readiness",
                    "--out",
                    str(mesh_report),
                ],
                workspace,
            )
        if (
            result.returncode == 0
            and (
                "Watertight: yes" not in result.output
                or "SDF build readiness: Ready" not in result.output
                or "Score:" not in result.output
                or not mesh_report.exists()
                or "SDF Build Readiness"
                not in mesh_report.read_text(encoding="utf-8", errors="replace")
            )
        ):
            result.returncode = 1
            result.output += "\nValidation failed: installed mesh_check output/report is missing.\n"
    results.append(result)
    if result.returncode != 0:
        write_report(report_path, results, source, build, install, config)
        print_failure(result, source, build, install)
        return result.returncode

    try:
        mesh_clean_exe = find_executable(install, "adasdf_mesh_clean", config)
    except FileNotFoundError as exc:
        result = StepResult("Installed Mesh Clean CLI", ["adasdf_mesh_clean"], 1, str(exc))
    else:
        print("[install-validation] Installed Mesh Clean CLI", flush=True)
        cleanup_fixture = source / "tests" / "data" / "mesh_diagnostics" / "duplicate_and_degenerate_ascii.stl"
        cleaned_stl = build.parent / "install_validation_cleaned.stl"
        cleanup_report = build.parent / "install_validation_cleanup_report.md"
        usage = run_step(
            "Installed Mesh Clean CLI",
            [str(mesh_clean_exe)],
            workspace,
        )
        result = usage
        if result.returncode == 0:
            result = run_step(
                "Installed Mesh Clean CLI",
                [
                    str(mesh_clean_exe),
                    str(cleanup_fixture),
                    str(cleaned_stl),
                    "--report",
                    str(cleanup_report),
                ],
                workspace,
            )
        if result.returncode in (0, 2):
            if (
                not cleaned_stl.exists()
                or not cleanup_report.exists()
                or "Removed duplicate triangles:" not in result.output
                or "After readiness:" not in result.output
                or "Cleanup Operations"
                not in cleanup_report.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed mesh_clean output/report is missing.\n"
            elif result.returncode == 2:
                result.returncode = 0
                result.output += "\nValidation note: mesh remained below Ready/Usable readiness after safe cleanup.\n"
    results.append(result)
    if result.returncode not in (0, 2):
        write_report(report_path, results, source, build, install, config)
        print_failure(result, source, build, install)
        return result.returncode

    mesh_fixture = source / "tests" / "data" / "mesh_diagnostics" / "closed_cube_ascii.stl"
    dense_sdfbin = build.parent / "install_validation_dense.sdfbin"
    dense_report = build.parent / "install_validation_dense_report.md"
    dense_json = build.parent / "install_validation_dense_report.json"
    adaptive_sdfbin = build.parent / "install_validation_adaptive_block.sdfbin"
    adaptive_report = build.parent / "install_validation_adaptive_block_report.md"
    adaptive_json = build.parent / "install_validation_adaptive_block_report.json"
    compressed_sdfbin = build.parent / "install_validation_compressed_block.sdfbin"
    compressed_report = build.parent / "install_validation_compression_report.md"
    compressed_json = build.parent / "install_validation_compression_report.json"
    compressed_quality = build.parent / "install_validation_compression_quality.md"
    compressed_quality_csv = build.parent / "install_validation_compressed_quality.csv"
    compressed_benchmark_csv = build.parent / "install_validation_compressed_benchmark.csv"
    compressed_direct_sdfbin = build.parent / "install_validation_compressed_direct.sdfbin"
    compressed_direct_report = build.parent / "install_validation_compressed_direct_report.md"
    compressed_direct_compression_report = build.parent / "install_validation_compressed_direct_compression_report.md"
    compressed_direct_quality = build.parent / "install_validation_compressed_direct_quality.md"
    strict_compressed_direct_json = build.parent / "install_strict_compressed_direct.json"
    strict_world_sparse_json = build.parent / "install_strict_world_sparse_collision.json"
    strict_world_benchmark_json = build.parent / "install_strict_collision_world_benchmark.json"
    strict_sparse_query_json = build.parent / "install_strict_sparse_query.json"
    strict_solver_contacts_json = build.parent / "install_strict_solver_contacts.json"
    strict_compressed_benchmark_json = build.parent / "install_strict_compressed_benchmark.json"
    strict_report_list = build.parent / "install_strict_report_inputs.txt"
    strict_summary_csv = build.parent / "install_strict_run_summary.csv"
    sample_fixture = source / "tests" / "data" / "samples" / "cube_sparse_samples.csv"
    sparse_results_csv = build.parent / "install_validation_sparse_results.csv"
    sparse_report = build.parent / "install_validation_sparse_report.md"
    sparse_collision_report = build.parent / "install_validation_sparse_collision.md"
    sparse_candidates_csv = build.parent / "install_validation_contact_candidates.csv"
    sparse_candidates_report = build.parent / "install_validation_contact_candidates.md"
    stabilized_contacts_csv = build.parent / "install_validation_stabilized_solver_contacts.csv"
    stabilized_contacts_json = build.parent / "install_validation_stabilized_solver_contacts.json"
    stabilized_contacts_report = build.parent / "install_validation_stabilized_solver_contacts.md"
    solver_contacts_csv = build.parent / "install_validation_solver_contacts.csv"
    solver_contacts_json = build.parent / "install_validation_solver_contacts.json"
    solver_contacts_report = build.parent / "install_validation_solver_contacts.md"
    solver_raw_candidates_csv = build.parent / "install_validation_solver_raw_candidates.csv"
    contact_reduction_benchmark_csv = build.parent / "install_validation_contact_reduction_benchmark.csv"
    contact_reduction_benchmark_json = build.parent / "install_validation_contact_reduction_benchmark.json"
    contact_reduction_benchmark_report = build.parent / "install_validation_contact_reduction_benchmark.md"
    sparse_benchmark_csv = build.parent / "install_validation_sparse_benchmark.csv"
    active_blocks_csv = build.parent / "install_validation_active_blocks.csv"
    active_blocks_report = build.parent / "install_validation_active_blocks.md"
    active_blocks_json = build.parent / "install_validation_active_blocks.json"
    active_query_csv = build.parent / "install_validation_active_block_query.csv"
    active_query_report = build.parent / "install_validation_active_block_query.md"
    block_cache_benchmark_csv = build.parent / "install_validation_block_cache_benchmark.csv"
    block_cache_benchmark_report = build.parent / "install_validation_block_cache_benchmark.md"
    cuda_active_query_csv = build.parent / "install_validation_cuda_active_block_query.csv"
    cuda_active_query_report = build.parent / "install_validation_cuda_active_block_query.md"
    cuda_block_cache_benchmark_csv = build.parent / "install_validation_cuda_block_cache_benchmark.csv"
    cuda_block_cache_benchmark_report = build.parent / "install_validation_cuda_block_cache_benchmark.md"
    world_scene_csv = build.parent / "install_validation_collision_world_scene.csv"
    world_broadphase_csv = build.parent / "install_validation_world_broadphase_pairs.csv"
    world_broadphase_report = build.parent / "install_validation_world_broadphase.md"
    world_broadphase_json = build.parent / "install_validation_world_broadphase.json"
    world_sparse_csv = build.parent / "install_validation_world_sparse_collisions.csv"
    world_sparse_report = build.parent / "install_validation_world_sparse_collision.md"
    world_sparse_json = build.parent / "install_validation_world_sparse_collision.json"
    world_solver_csv = build.parent / "install_validation_world_solver_contacts.csv"
    world_solver_report = build.parent / "install_validation_world_solver_contacts.md"
    world_solver_json = build.parent / "install_validation_world_solver_contacts.json"
    world_benchmark_csv = build.parent / "install_validation_collision_world_benchmark.csv"
    world_benchmark_report = build.parent / "install_validation_collision_world_benchmark.md"
    world_benchmark_json = build.parent / "install_validation_collision_world_benchmark.json"
    recommendation_md = build.parent / "install_validation_recommendation.md"
    recommendation_json = build.parent / "install_validation_recommendation.json"
    adaptive_dryrun_sdfbin = build.parent / "install_validation_adaptive_dryrun.sdfbin"
    adaptive_dryrun_report = build.parent / "install_validation_adaptive_dryrun_plan.md"
    adaptive_preview_sdfbin = build.parent / "install_validation_adaptive_preview.sdfbin"
    adaptive_preview_plan = build.parent / "install_validation_adaptive_preview_plan.md"
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

    dense_tool_names = [
        "adasdf_build_dense_sdf",
        "adasdf_build_adaptive_sdf",
        "adasdf_compress_adaptive_sdf",
        "adasdf_build_compressed_sdf",
        "adasdf_write_manifest",
        "adasdf_validate_report",
        "adasdf_collect_run_summary",
        "adasdf_recommend_build",
        "adasdf_info",
        "adasdf_query",
        "adasdf_collide",
        "adasdf_expansion_quality",
        "adasdf_benchmark_batch_query",
        "adasdf_sparse_query",
        "adasdf_sparse_collide",
        "adasdf_contact_candidates",
        "adasdf_stabilize_contacts",
        "adasdf_solver_contact_candidates",
        "adasdf_benchmark_sparse_query",
        "adasdf_select_active_blocks",
        "adasdf_active_block_query",
        "adasdf_benchmark_block_cache",
        "adasdf_cuda_active_block_query",
        "adasdf_benchmark_cuda_block_cache",
        "adasdf_benchmark_contact_reduction",
        "adasdf_build_adaptive_sdf_preview",
        "adasdf_world_broadphase",
        "adasdf_world_sparse_collide",
        "adasdf_world_solver_contacts",
        "adasdf_benchmark_collision_world",
    ]
    dense_tools: dict[str, Path] = {}
    for tool_name in dense_tool_names:
        try:
            dense_tools[tool_name] = find_executable(install, tool_name, config)
        except FileNotFoundError as exc:
            result = StepResult(
                f"Installed {tool_name}",
                [tool_name],
                1,
                str(exc),
            )
            results.append(result)
            write_report(report_path, results, source, build, install, config)
            print_failure(result, source, build, install)
            return result.returncode

    installed_dense_steps: list[tuple[str, list[str]]] = [
        (
            "Installed Build Recommendation CLI",
            [
                str(dense_tools["adasdf_recommend_build"]),
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
            "Installed DenseSDF Build CLI",
            [
                str(dense_tools["adasdf_build_dense_sdf"]),
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
            "Installed DenseSDF Info CLI",
            [str(dense_tools["adasdf_info"]), str(dense_sdfbin)],
        ),
        (
            "Installed DenseSDF Query CLI",
            [
                str(dense_tools["adasdf_query"]),
                str(dense_sdfbin),
                "--point",
                "0.5",
                "0.5",
                "0.5",
            ],
        ),
        (
            "Installed DenseSDF Collide CLI",
            [
                str(dense_tools["adasdf_collide"]),
                str(dense_sdfbin),
                str(dense_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "Installed AdaptiveBlockSDF Build CLI",
            [
                str(dense_tools["adasdf_build_adaptive_sdf"]),
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
            "Installed AdaptiveBlockSDF Info CLI",
            [str(dense_tools["adasdf_info"]), str(adaptive_sdfbin)],
        ),
        (
            "Installed AdaptiveBlockSDF Query CLI",
            [
                str(dense_tools["adasdf_query"]),
                str(adaptive_sdfbin),
                "--point",
                "0.5",
                "0.5",
                "0.5",
            ],
        ),
        (
            "Installed AdaptiveBlockSDF Collide CLI",
            [
                str(dense_tools["adasdf_collide"]),
                str(adaptive_sdfbin),
                str(adaptive_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "Installed CompressedSDF Compress CLI",
            [
                str(dense_tools["adasdf_compress_adaptive_sdf"]),
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
                str(compressed_quality),
            ],
        ),
        (
            "Installed CompressedSDF Info CLI",
            [str(dense_tools["adasdf_info"]), str(compressed_sdfbin)],
        ),
        (
            "Installed CompressedSDF Query CLI",
            [
                str(dense_tools["adasdf_query"]),
                str(compressed_sdfbin),
                "--point",
                "0.5",
                "0.5",
                "0.5",
            ],
        ),
        (
            "Installed CompressedSDF Collide CLI",
            [
                str(dense_tools["adasdf_collide"]),
                str(compressed_sdfbin),
                str(compressed_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "Installed CompressedSDF Expansion Quality CLI",
            [
                str(dense_tools["adasdf_expansion_quality"]),
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
            "Installed CompressedSDF Benchmark Model CLI",
            [
                str(dense_tools["adasdf_benchmark_batch_query"]),
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
                "--strict-json",
                str(strict_compressed_benchmark_json),
                "--case-id",
                "install_compressed_benchmark",
            ],
        ),
        (
            "Installed CompressedSDF One-Step Build CLI",
            [
                str(dense_tools["adasdf_build_compressed_sdf"]),
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
                str(compressed_direct_quality),
                "--strict-json",
                str(strict_compressed_direct_json),
                "--case-id",
                "install_compressed_direct",
            ],
        ),
        (
            "Installed CollisionWorld Broadphase CLI",
            [
                str(dense_tools["adasdf_world_broadphase"]),
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
            "Installed CollisionWorld Sparse Collide CLI",
            [
                str(dense_tools["adasdf_world_sparse_collide"]),
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
                "install_world_sparse",
            ],
        ),
        (
            "Installed CollisionWorld Solver Contacts CLI",
            [
                str(dense_tools["adasdf_world_solver_contacts"]),
                str(world_scene_csv),
                "--threshold",
                "1e-3",
                "--top-k",
                "32",
                "--max-contacts",
                "8",
                "--out",
                str(world_solver_csv),
                "--report",
                str(world_solver_report),
                "--json",
                str(world_solver_json),
            ],
        ),
        (
            "Installed CollisionWorld Benchmark CLI",
            [
                str(dense_tools["adasdf_benchmark_collision_world"]),
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
                "install_world_benchmark",
            ],
        ),
        (
            "Installed Sparse Query CLI",
            [
                str(dense_tools["adasdf_sparse_query"]),
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
                "install_sparse_query",
            ],
        ),
        (
            "Installed Sparse Collide CLI",
            [
                str(dense_tools["adasdf_sparse_collide"]),
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
            "Installed Contact Candidates CLI",
            [
                str(dense_tools["adasdf_contact_candidates"]),
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
            "Installed Contact Stabilization CLI",
            [
                str(dense_tools["adasdf_stabilize_contacts"]),
                str(sparse_candidates_csv),
                "--max-contacts",
                "8",
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
            "Installed Solver Contact Candidates CLI",
            [
                str(dense_tools["adasdf_solver_contact_candidates"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1e-3",
                "--top-k",
                "32",
                "--max-contacts",
                "8",
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
                "install_solver_contacts",
            ],
        ),
        (
            "Installed Contact Reduction Benchmark CLI",
            [
                str(dense_tools["adasdf_benchmark_contact_reduction"]),
                str(compressed_direct_sdfbin),
                str(sample_fixture),
                "--threshold",
                "1e-3",
                "--top-k",
                "64",
                "--max-contacts",
                "8",
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
            "Installed Sparse Query Benchmark CLI",
            [
                str(dense_tools["adasdf_benchmark_sparse_query"]),
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
            "Installed Active Block Selection CLI",
            [
                str(dense_tools["adasdf_select_active_blocks"]),
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
            "Installed Active Block Query CLI",
            [
                str(dense_tools["adasdf_active_block_query"]),
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
            "Installed Active Block Cache Benchmark CLI",
            [
                str(dense_tools["adasdf_benchmark_block_cache"]),
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
            "Installed CUDA Active Block Query CLI",
            [
                str(dense_tools["adasdf_cuda_active_block_query"]),
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
            "Installed CUDA Active Block Cache Benchmark CLI",
            [
                str(dense_tools["adasdf_benchmark_cuda_block_cache"]),
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
            "Installed AdaptiveBlockSDF Dry Run CLI",
            [
                str(dense_tools["adasdf_build_adaptive_sdf"]),
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
            "Installed Adaptive Builder Preview CLI",
            [
                str(dense_tools["adasdf_build_adaptive_sdf_preview"]),
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
    ]
    for name, command in installed_dense_steps:
        print(f"[install-validation] {name}", flush=True)
        result = run_step(name, command, workspace)
        if name == "Installed Sparse Collide CLI" and result.returncode == 10:
            result.returncode = 0
            result.output += (
                "\nValidation note: sparse_collide returned 10, which means "
                "collision detected and is expected for this fixture.\n"
            )
        if name == "Installed CollisionWorld Sparse Collide CLI" and result.returncode == 10:
            result.returncode = 0
            result.output += (
                "\nValidation note: world_sparse_collide returned 10, which means "
                "collision detected and is expected for this fixture.\n"
            )
        if name == "Installed Active Block Query CLI" and result.returncode == 10:
            result.returncode = 0
            result.output += (
                "\nValidation note: active_block_query returned 10, which means "
                "collision detected and is expected for this fixture.\n"
            )
        if name == "Installed CUDA Active Block Query CLI" and result.returncode in (10, 20):
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
            result.returncode = 0
        if name == "Installed CUDA Active Block Cache Benchmark CLI" and result.returncode == 20:
            result.returncode = 0
            result.output += (
                "\nValidation note: benchmark_cuda_block_cache returned 20, which means "
                "CUDA was unavailable and the run was skipped as expected.\n"
            )
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, install, config)
            print_failure(result, source, build, install)
            return result.returncode
        if name == "Installed DenseSDF Build CLI":
            if (
                not dense_sdfbin.exists()
                or not dense_report.exists()
                or not dense_json.exists()
                or "Reload validation: success" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed DenseSDF build outputs are missing.\n"
        elif name == "Installed Build Recommendation CLI":
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
                result.output += "\nValidation failed: installed build recommendation output is incomplete.\n"
        elif name == "Installed DenseSDF Info CLI":
            if "ADASDF_DENSE_SDFBIN_V1" not in result.output or "DenseSDF resolution:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: installed DenseSDF info output is incomplete.\n"
        elif name == "Installed DenseSDF Query CLI":
            if "Signed distance:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: installed DenseSDF query output is incomplete.\n"
        elif name == "Installed DenseSDF Collide CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > 4:
                result.returncode = 1
                result.output += "\nValidation failed: installed DenseSDF collide output is incomplete.\n"
        elif name == "Installed AdaptiveBlockSDF Build CLI":
            if (
                not adaptive_sdfbin.exists()
                or not adaptive_report.exists()
                or not adaptive_json.exists()
                or "Reload validation: success" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed AdaptiveBlockSDF build outputs are missing.\n"
        elif name == "Installed AdaptiveBlockSDF Info CLI":
            if "ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1" not in result.output or "AdaptiveBlockSDF block_count:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: installed AdaptiveBlockSDF info output is incomplete.\n"
        elif name == "Installed AdaptiveBlockSDF Query CLI":
            if "Signed distance:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: installed AdaptiveBlockSDF query output is incomplete.\n"
        elif name == "Installed AdaptiveBlockSDF Collide CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > 4:
                result.returncode = 1
                result.output += "\nValidation failed: installed AdaptiveBlockSDF collide output is incomplete.\n"
        elif name == "Installed CompressedSDF Compress CLI":
            if (
                not compressed_sdfbin.exists()
                or not compressed_report.exists()
                or not compressed_json.exists()
                or not compressed_quality.exists()
                or "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1" not in result.output
                or "Reload validation: success" not in result.output
                or "Tucker/HOSVD compression: planned / not implemented" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed compressed SDF compression output is incomplete.\n"
        elif name == "Installed CompressedSDF Info CLI":
            if "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1" not in result.output or "CompressedBlockSDF block_count:" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: installed compressed SDF info output is incomplete.\n"
        elif name == "Installed CompressedSDF Query CLI":
            if "Signed distance:" not in result.output or "compressed adaptive block SDF backend" not in result.output:
                result.returncode = 1
                result.output += "\nValidation failed: installed compressed SDF query output is incomplete.\n"
        elif name == "Installed CompressedSDF Collide CLI":
            match = re.search(r"Returned contacts:\s+(\d+)", result.output)
            if not match or int(match.group(1)) > 4:
                result.returncode = 1
                result.output += "\nValidation failed: installed compressed SDF collide output is incomplete.\n"
        elif name == "Installed CompressedSDF Expansion Quality CLI":
            if (
                not compressed_quality_csv.exists()
                or "Status: ok" not in result.output
                or "max_abs_error" not in compressed_quality_csv.read_text(encoding="utf-8", errors="replace")
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed compressed SDF expansion quality output is incomplete.\n"
        elif name == "Installed CompressedSDF Benchmark Model CLI":
            csv_text = compressed_benchmark_csv.read_text(encoding="utf-8", errors="replace") if compressed_benchmark_csv.exists() else ""
            if (
                not compressed_benchmark_csv.exists()
                or not csv_text.splitlines()
                or "query_backend" not in csv_text.splitlines()[0]
                or "cpu,none,all,1000" not in csv_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed compressed SDF benchmark --model output is incomplete.\n"
        elif name == "Installed CompressedSDF One-Step Build CLI":
            if (
                not compressed_direct_sdfbin.exists()
                or not compressed_direct_report.exists()
                or not compressed_direct_compression_report.exists()
                or not compressed_direct_quality.exists()
                or "ADASDF_COMPRESSED_BLOCK_SDFBIN_V1" not in result.output
                or "Reload validation: success" not in result.output
                or "adasdf_recommend_build" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed one-step compressed SDF build output is incomplete.\n"
        elif name == "Installed CollisionWorld Broadphase CLI":
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
                result.output += "\nValidation failed: installed CollisionWorld broadphase output is incomplete.\n"
        elif name == "Installed CollisionWorld Sparse Collide CLI":
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
                result.output += "\nValidation failed: installed CollisionWorld sparse collision output is incomplete.\n"
        elif name == "Installed CollisionWorld Solver Contacts CLI":
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
                or int(match.group(1)) > 8
                or "solver-ready candidates, not solver constraints" not in result.output
                or "stable_key" not in header
                or "CollisionWorld Solver-Ready Contacts" not in report_text
                or "solver_contact_count" not in json_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed CollisionWorld solver contacts output is incomplete.\n"
        elif name == "Installed CollisionWorld Benchmark CLI":
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
                result.output += "\nValidation failed: installed CollisionWorld benchmark output is incomplete.\n"
        elif name == "Installed Sparse Query CLI":
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
                result.output += "\nValidation failed: installed sparse query output is incomplete.\n"
        elif name == "Installed Sparse Collide CLI":
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
                result.output += "\nValidation failed: installed sparse collide output is incomplete.\n"
        elif name == "Installed Contact Candidates CLI":
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
                result.output += "\nValidation failed: installed contact candidates output is incomplete.\n"
        elif name == "Installed Contact Stabilization CLI":
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
                or int(match.group(1)) > 8
                or "stable_key" not in header
                or "contact_count" not in json_text
                or "Contact Stabilization Report" not in report_text
                or "not solver impulses" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed contact stabilization output is incomplete.\n"
        elif name == "Installed Solver Contact Candidates CLI":
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
                or int(match.group(1)) > 8
                or "Raw candidates:" not in result.output
                or "Patches:" not in result.output
                or "stable_key" not in header
                or "rank" not in raw_header
                or "contact_count" not in json_text
                or "Solver-Ready Contact Candidates" not in report_text
                or "No impulses or friction forces are computed" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed solver contact candidates output is incomplete.\n"
        elif name == "Installed Contact Reduction Benchmark CLI":
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
                or int(match.group(1)) > 8
                or "avg_reduction_ms:" not in result.output
                or "candidate_reduction_ratio:" not in result.output
                or "avg_reduction_ms" not in header
                or "candidate_reduction_ratio" not in header
                or "avg_reduction_ms" not in json_text
                or "Contact Reduction Benchmark" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed contact reduction benchmark output is incomplete.\n"
        elif name == "Installed Sparse Query Benchmark CLI":
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
                result.output += "\nValidation failed: installed sparse benchmark output is incomplete.\n"
        elif name == "Installed Active Block Selection CLI":
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
                result.output += "\nValidation failed: installed active block selection output is incomplete.\n"
        elif name == "Installed Active Block Query CLI":
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
                result.output += "\nValidation failed: installed active block query output is incomplete.\n"
        elif name == "Installed Active Block Cache Benchmark CLI":
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
                result.output += "\nValidation failed: installed block cache benchmark output is incomplete.\n"
        elif name == "Installed CUDA Active Block Query CLI":
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
            if (
                not cuda_active_query_csv.exists()
                or not cuda_active_query_report.exists()
                or "CUDA available:" not in result.output
                or not ("Status: ok" in result.output or "Status: skipped" in result.output)
                or "source" not in (csv_text.splitlines()[0] if csv_text.splitlines() else "")
                or "CUDA Active Block Query Report" not in report_text
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed CUDA active block query output is incomplete.\n"
        elif name == "Installed CUDA Active Block Cache Benchmark CLI":
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
                result.output += "\nValidation failed: installed CUDA block cache benchmark output is incomplete.\n"
        elif name == "Installed AdaptiveBlockSDF Dry Run CLI":
            if (
                adaptive_dryrun_sdfbin.exists()
                or not adaptive_dryrun_report.exists()
                or "Dry run: yes" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed AdaptiveBlockSDF dry-run output is incomplete.\n"
        elif name == "Installed Adaptive Builder Preview CLI":
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
                result.output += "\nValidation failed: installed adaptive preview output is incomplete.\n"
        if result.returncode != 0:
            write_report(report_path, results, source, build, install, config)
            print_failure(result, source, build, install)
            return result.returncode

    strict_reports = [
        strict_compressed_direct_json,
        strict_world_sparse_json,
        strict_world_benchmark_json,
        strict_sparse_query_json,
        strict_solver_contacts_json,
        strict_compressed_benchmark_json,
    ]
    for strict_report in strict_reports:
        print(f"[install-validation] Strict Report Validate {strict_report.name}", flush=True)
        result = run_step(
            f"Strict Report Validate {strict_report.name}",
            [str(dense_tools["adasdf_validate_report"]), str(strict_report)],
            workspace,
        )
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, install, config)
            print_failure(result, source, build, install)
            return result.returncode
    strict_report_list.write_text(
        "\n".join(str(path) for path in strict_reports) + "\n",
        encoding="utf-8",
    )
    print("[install-validation] Strict Report Summary CSV", flush=True)
    result = run_step(
        "Strict Report Summary CSV",
        [
            str(dense_tools["adasdf_collect_run_summary"]),
            "--inputs",
            str(strict_report_list),
            "--out",
            str(strict_summary_csv),
        ],
        workspace,
    )
    results.append(result)
    if result.returncode != 0 or not strict_summary_csv.exists():
        if result.returncode == 0:
            result.returncode = 1
            result.output += "\nValidation failed: strict summary CSV was not created.\n"
        write_report(report_path, results, source, build, install, config)
        print_failure(result, source, build, install)
        return result.returncode

    try:
        package_exe = find_executable(package_build, "test_find_package", config)
    except FileNotFoundError as exc:
        result = StepResult("Package Run", ["test_find_package"], 1, str(exc))
    else:
        print("[install-validation] Package Run", flush=True)
        result = run_step("Package Run", [str(package_exe)], workspace)
    results.append(result)
    if result.returncode != 0:
        write_report(report_path, results, source, build, install, config)
        print_failure(result, source, build, install)
        return result.returncode

    downstream_steps: list[tuple[str, list[str]]] = [
        (
            "Downstream Configure",
            [
                "cmake",
                "-S",
                str(downstream_source),
                "-B",
                str(downstream_build),
                f"-DCMAKE_PREFIX_PATH={install}",
                *cmake_build_type_args(config),
            ],
        ),
        ("Downstream Build", cmake_build_command(downstream_build, config)),
    ]
    for name, command in downstream_steps:
        print(f"[install-validation] {name}", flush=True)
        result = run_step(name, command, workspace)
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, install, config)
            print_failure(result, source, build, install)
            return result.returncode

    try:
        downstream_exe = find_executable(downstream_build, "adasdf_downstream", config)
    except FileNotFoundError as exc:
        result = StepResult(
            "Downstream Run",
            ["adasdf_downstream"],
            1,
            str(exc),
        )
    else:
        print("[install-validation] Downstream Run", flush=True)
        result = run_step("Downstream Run", [str(downstream_exe)], workspace)
    results.append(result)

    write_report(report_path, results, source, build, install, config)
    if result.returncode != 0:
        print_failure(result, source, build, install)
        return result.returncode

    print("Install validation: PASS")
    print("Report: reports/install_validation_summary.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

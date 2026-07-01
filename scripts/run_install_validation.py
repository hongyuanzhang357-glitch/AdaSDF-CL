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

    steps: list[tuple[str, list[str]]] = [
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

    results: list[StepResult] = []
    for name, command in steps:
        print(f"[install-validation] {name}", flush=True)
        result = run_step(name, command, workspace)
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
    adaptive_dryrun_sdfbin = build.parent / "install_validation_adaptive_dryrun.sdfbin"
    adaptive_dryrun_report = build.parent / "install_validation_adaptive_dryrun_plan.md"
    adaptive_preview_sdfbin = build.parent / "install_validation_adaptive_preview.sdfbin"
    adaptive_preview_plan = build.parent / "install_validation_adaptive_preview_plan.md"
    for generated in (adaptive_preview_sdfbin, adaptive_dryrun_sdfbin):
        if generated.exists():
            generated.unlink()

    dense_tool_names = [
        "adasdf_build_dense_sdf",
        "adasdf_build_adaptive_sdf",
        "adasdf_compress_adaptive_sdf",
        "adasdf_build_compressed_sdf",
        "adasdf_info",
        "adasdf_query",
        "adasdf_collide",
        "adasdf_expansion_quality",
        "adasdf_benchmark_batch_query",
        "adasdf_build_adaptive_sdf_preview",
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
                or "Surrogate recommendation: planned for v1.8.0-alpha" not in result.output
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed one-step compressed SDF build output is incomplete.\n"
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
            ):
                result.returncode = 1
                result.output += "\nValidation failed: installed adaptive preview output is incomplete.\n"
        if result.returncode != 0:
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

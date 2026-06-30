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
        ("Build", ["cmake", "--build", str(build), "--config", config, "--parallel"]),
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
        ("Package Build", ["cmake", "--build", str(package_build), "--config", config, "--parallel"]),
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
        ("Downstream Build", ["cmake", "--build", str(downstream_build), "--config", config, "--parallel"]),
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

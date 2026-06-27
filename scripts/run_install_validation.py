#!/usr/bin/env python3
"""Validate that an installed AdaSDF-CL package is consumable downstream."""

from __future__ import annotations

import argparse
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


def downstream_executable(build: Path, config: str) -> Path | None:
    suffix = ".exe" if sys.platform.startswith("win") else ""
    candidates = [
        build / config / f"adasdf_downstream{suffix}",
        build / f"adasdf_downstream{suffix}",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    matches = list(build.rglob(f"adasdf_downstream{suffix}"))
    return matches[0] if matches else None


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
    package_script = source / "tests" / "package" / "run_package_test.cmake"
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
                "-DADASDF_CL_USE_EXISTING_CORE=OFF",
                "-DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON",
                "-DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON",
                f"-DCMAKE_INSTALL_PREFIX={install}",
            ],
        ),
        ("Build", ["cmake", "--build", str(build), "--config", config, "--parallel"]),
        ("Install", ["cmake", "--install", str(build), "--config", config, "--prefix", str(install)]),
        (
            "Package Config Test",
            [
                "cmake",
                f"-DADASDF_CL_INSTALL_PREFIX={install}",
                f"-DADASDF_CL_PACKAGE_TEST_BUILD={package_build}",
                f"-DADASDF_CL_CONFIG={config}",
                "-P",
                str(package_script),
            ],
        ),
        (
            "Downstream Configure",
            [
                "cmake",
                "-S",
                str(downstream_source),
                "-B",
                str(downstream_build),
                f"-DCMAKE_PREFIX_PATH={install}",
                f"-DCMAKE_BUILD_TYPE={config}",
            ],
        ),
        ("Downstream Build", ["cmake", "--build", str(downstream_build), "--config", config, "--parallel"]),
    ]

    results: list[StepResult] = []
    for name, command in steps:
        result = run_step(name, command, workspace)
        results.append(result)
        if result.returncode != 0:
            write_report(report_path, results, source, build, install, config)
            return result.returncode

    exe = downstream_executable(downstream_build, config)
    if exe is None:
        result = StepResult(
            "Downstream Run",
            ["adasdf_downstream"],
            1,
            "Could not locate downstream executable after build.",
        )
    else:
        result = run_step("Downstream Run", [str(exe)], workspace)
    results.append(result)

    write_report(report_path, results, source, build, install, config)
    if result.returncode != 0:
        return result.returncode

    print("Install validation: PASS")
    print("Report: reports/install_validation_summary.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

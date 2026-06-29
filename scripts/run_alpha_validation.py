#!/usr/bin/env python3
"""Run the AdaSDF-CL alpha validation path and write a compact report."""

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
        "-DADASDF_CL_USE_EXISTING_CORE=OFF",
        "-DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON",
        "-DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON",
        "-DADASDF_CL_ENABLE_DEMO_BACKEND=ON",
    ]
    steps = [
        ("Configure", configure),
        ("Build", ["cmake", "--build", str(build), "--config", config]),
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

    demo_sdfbin = build.parent / "cube_demo_v0_8.sdfbin"
    required_demo_tools = {
        "adasdf_make_demo_box": find_executable(build, "adasdf_make_demo_box", config),
        "adasdf_info": find_executable(build, "adasdf_info", config),
        "adasdf_query": find_executable(build, "adasdf_query", config),
        "adasdf_collide": find_executable(build, "adasdf_collide", config),
        "adasdf_core_free_demo_collision": find_executable(
            build, "adasdf_core_free_demo_collision", config
        ),
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
            "Make Demo Box SDFBin",
            [str(required_demo_tools["adasdf_make_demo_box"]), str(demo_sdfbin)],
        ),
        (
            "Demo Info CLI",
            [str(required_demo_tools["adasdf_info"]), str(demo_sdfbin)],
        ),
        (
            "Demo Query CLI",
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
            "Demo Collide CLI",
            [
                str(required_demo_tools["adasdf_collide"]),
                str(demo_sdfbin),
                str(demo_sdfbin),
                "--max-contacts",
                "4",
            ],
        ),
        (
            "Core-Free Demo Collision Example",
            [str(required_demo_tools["adasdf_core_free_demo_collision"])],
        ),
    ]

    for name, command in demo_steps:
        result = run_step(name, command, workspace)
        results.append(result)
        if result.returncode != 0:
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

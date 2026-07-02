"""Subprocess runner for AdaSDF-CL command-line tools."""

from __future__ import annotations

import subprocess
import time
from pathlib import Path
from typing import Dict, List, Optional

from .exceptions import AdaSDFCommandError
from .results import CommandResult


def run_command(
    command: List[str],
    *,
    check: bool = True,
    timeout: Optional[float] = None,
    cwd: Optional[Path] = None,
    env: Optional[Dict[str, str]] = None,
    dry_run: bool = False,
) -> CommandResult:
    """Run a command and return captured stdout/stderr.

    dry_run=True never executes the command and returns a successful
    CommandResult with stdout containing the preview command.
    """

    rendered = [str(part) for part in command]
    if dry_run:
        return CommandResult(
            command=rendered,
            returncode=0,
            stdout=" ".join(rendered),
            stderr="",
            elapsed_seconds=0.0,
        )

    start = time.perf_counter()
    try:
        completed = subprocess.run(
            rendered,
            cwd=str(cwd) if cwd is not None else None,
            env=env,
            timeout=timeout,
            text=True,
            encoding="utf-8",
            errors="replace",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        elapsed = time.perf_counter() - start
        result = CommandResult(
            command=rendered,
            returncode=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
            elapsed_seconds=elapsed,
        )
    except subprocess.TimeoutExpired as exc:
        elapsed = time.perf_counter() - start
        stdout = exc.stdout if isinstance(exc.stdout, str) else ""
        stderr = exc.stderr if isinstance(exc.stderr, str) else ""
        result = CommandResult(
            command=rendered,
            returncode=-1,
            stdout=stdout,
            stderr=stderr + f"\nCommand timed out after {timeout} seconds.",
            elapsed_seconds=elapsed,
        )
        if check:
            raise AdaSDFCommandError(rendered, result.returncode, result.stdout, result.stderr)
        return result

    if check and result.returncode != 0:
        raise AdaSDFCommandError(rendered, result.returncode, result.stdout, result.stderr)
    return result

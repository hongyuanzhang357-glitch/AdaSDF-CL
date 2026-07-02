"""Exceptions for the AdaSDF-CL Python CLI wrapper."""

from __future__ import annotations

from typing import Iterable, Sequence


class AdaSDFError(Exception):
    """Base exception for the lightweight AdaSDF-CL wrapper."""


class AdaSDFToolNotFound(AdaSDFError):
    """Raised when an AdaSDF-CL command-line tool cannot be found."""


class AdaSDFCommandError(AdaSDFError):
    """Raised when an AdaSDF-CL command exits with an error."""

    def __init__(
        self,
        command: Sequence[str],
        returncode: int,
        stdout: str,
        stderr: str,
    ) -> None:
        self.command = [str(part) for part in command]
        self.returncode = int(returncode)
        self.stdout = stdout
        self.stderr = stderr
        message = (
            "AdaSDF-CL command failed with return code "
            f"{self.returncode}: {format_command(self.command)}"
        )
        stdout_preview = preview(stdout)
        stderr_preview = preview(stderr)
        if stdout_preview:
            message += f"\nstdout:\n{stdout_preview}"
        if stderr_preview:
            message += f"\nstderr:\n{stderr_preview}"
        super().__init__(message)


class AdaSDFParseWarning(Warning):
    """Warning category for best-effort parser limitations."""


def preview(text: str, max_chars: int = 1200) -> str:
    """Return a compact text preview for exception messages."""

    if not text:
        return ""
    text = str(text).strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "\n..."


def format_command(command: Iterable[str]) -> str:
    """Format a command list without requiring platform-specific shell quoting."""

    return " ".join(str(part) for part in command)

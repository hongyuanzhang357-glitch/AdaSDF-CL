"""Result dataclasses returned by the AdaSDF-CL Python CLI wrapper."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


@dataclass
class CommandResult:
    command: List[str]
    returncode: int
    stdout: str
    stderr: str
    elapsed_seconds: Optional[float] = None


@dataclass
class MeshCheckResult:
    command_result: CommandResult
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class BuildResult:
    command_result: CommandResult
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class RecommendationResult:
    command_result: CommandResult
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None
    recommended_command: Optional[str] = None
    recommended_path: Optional[str] = None


@dataclass
class QueryResult:
    command_result: CommandResult
    phi: Optional[float] = None
    normal: Optional[Tuple[float, float, float]] = None


@dataclass
class CollisionResult:
    command_result: CommandResult
    colliding: Optional[bool] = None
    contact_count: Optional[int] = None
    minimum_distance: Optional[float] = None


@dataclass
class InfoResult:
    command_result: CommandResult
    format: Optional[str] = None


@dataclass
class BenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)

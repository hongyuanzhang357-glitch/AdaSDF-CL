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
    profile_json_path: Optional[Path] = None
    progress_json_path: Optional[Path] = None


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
    json_data: Optional[Dict[str, Any]] = None


@dataclass
class BackendJsonResult:
    command_result: CommandResult
    json_data: Optional[Dict[str, Any]] = None
    json_path: Optional[Path] = None


@dataclass
class BenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)


@dataclass
class ContactBandBenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    csv_path: Optional[Path] = None
    marker_debug_csv_path: Optional[Path] = None


@dataclass
class StrictManifestResult:
    command_result: CommandResult
    report_path: Optional[Path] = None


@dataclass
class StrictReportValidationResult:
    command_result: CommandResult
    report_path: Optional[Path] = None
    valid: Optional[bool] = None


@dataclass
class RunSummaryResult:
    command_result: CommandResult
    inputs_path: Optional[Path] = None
    csv_path: Optional[Path] = None


@dataclass
class SparseQueryResult:
    command_result: CommandResult
    colliding: Optional[bool] = None
    sample_count: Optional[int] = None
    queried_count: Optional[int] = None
    result_count: Optional[int] = None
    min_effective_phi: Optional[float] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class SparseCollisionResult:
    command_result: CommandResult
    colliding: Optional[bool] = None
    min_effective_phi: Optional[float] = None
    queried_count: Optional[int] = None
    early_exit: Optional[bool] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class ContactCandidatesResult:
    command_result: CommandResult
    threshold_candidate_count: Optional[int] = None
    candidate_count: Optional[int] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class ContactStabilizationResult:
    command_result: CommandResult
    input_candidate_count: Optional[int] = None
    patch_count: Optional[int] = None
    solver_contact_count: Optional[int] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class SolverContactsResult:
    command_result: CommandResult
    sample_count: Optional[int] = None
    raw_candidate_count: Optional[int] = None
    patch_count: Optional[int] = None
    solver_contact_count: Optional[int] = None
    output_path: Optional[Path] = None
    candidates_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class ContactReductionBenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None
    csv_path: Optional[Path] = None


@dataclass
class SparseBenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None
    csv_path: Optional[Path] = None


@dataclass
class ActiveBlockSelectionResult:
    command_result: CommandResult
    active_block_count: Optional[int] = None
    candidate_sample_count: Optional[int] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class ActiveBlockQueryResult:
    command_result: CommandResult
    colliding: Optional[bool] = None
    sample_count: Optional[int] = None
    queried_count: Optional[int] = None
    result_count: Optional[int] = None
    cache_query_count: Optional[int] = None
    fallback_query_count: Optional[int] = None
    resident_blocks: Optional[int] = None
    min_effective_phi: Optional[float] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class BlockCacheBenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None
    csv_path: Optional[Path] = None


@dataclass
class BlockLookupBenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    csv_path: Optional[Path] = None


@dataclass
class CudaActiveBlockQueryResult:
    command_result: CommandResult
    cuda_available: Optional[bool] = None
    colliding: Optional[bool] = None
    sample_count: Optional[int] = None
    queried_count: Optional[int] = None
    result_count: Optional[int] = None
    active_block_count: Optional[int] = None
    expanded_block_count: Optional[int] = None
    fallback_query_count: Optional[int] = None
    gpu_memory_bytes: Optional[int] = None
    kernel_time_ms: Optional[float] = None
    total_time_ms: Optional[float] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class CudaBlockCacheBenchmarkResult:
    command_result: CommandResult
    cuda_available: Optional[bool] = None
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None
    csv_path: Optional[Path] = None


@dataclass
class WorldBroadphaseResult:
    command_result: CommandResult
    object_count: Optional[int] = None
    tested_pair_count: Optional[int] = None
    overlap_pair_count: Optional[int] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class WorldSparseCollisionResult:
    command_result: CommandResult
    colliding: Optional[bool] = None
    broadphase_pair_count: Optional[int] = None
    queried_pair_count: Optional[int] = None
    queried_sample_count: Optional[int] = None
    violation_count: Optional[int] = None
    min_effective_phi: Optional[float] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class WorldSolverContactsResult:
    command_result: CommandResult
    raw_candidate_count: Optional[int] = None
    reduced_candidate_count: Optional[int] = None
    patch_count: Optional[int] = None
    solver_contact_count: Optional[int] = None
    max_penetration: Optional[float] = None
    output_path: Optional[Path] = None
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None


@dataclass
class CollisionWorldBenchmarkResult:
    command_result: CommandResult
    metrics: Dict[str, Any] = field(default_factory=dict)
    report_path: Optional[Path] = None
    json_path: Optional[Path] = None
    csv_path: Optional[Path] = None

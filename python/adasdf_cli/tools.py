"""Public Python helpers that call installed AdaSDF-CL CLI tools."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable, List, Mapping, Optional, Sequence, Union

from .config import AdaSDFConfig, find_tool, preview_tool
from .parsers import (
    parse_benchmark_metrics,
    parse_build_acceleration_stats,
    parse_block_cache_benchmark_metrics,
    parse_collision_world_benchmark_metrics,
    parse_collision_colliding,
    parse_contact_band_sampling_benchmark_metrics,
    parse_contact_count,
    parse_contact_reduction_benchmark_metrics,
    parse_cuda_block_cache_benchmark_metrics,
    parse_field_bool,
    parse_field_float,
    parse_field_int,
    parse_exact_hotpath_benchmark_metrics,
    parse_hierarchical_sampling_benchmark_metrics,
    parse_info_format,
    parse_minimum_distance,
    parse_query_normal,
    parse_query_phi,
    parse_recommended_command,
    parse_recommended_path,
    parse_raw_candidate_count,
    parse_run_summary_csv_path,
    parse_patch_count,
    parse_solver_contact_count,
    parse_sparse_benchmark_metrics,
    parse_strict_report_path,
    parse_strict_report_valid,
)
from .results import (
    BenchmarkResult,
    ActiveBlockQueryResult,
    ActiveBlockSelectionResult,
    BlockCacheBenchmarkResult,
    BuildResult,
    CollisionResult,
    CollisionWorldBenchmarkResult,
    CommandResult,
    ContactBandBenchmarkResult,
    ContactCandidatesResult,
    ContactReductionBenchmarkResult,
    ContactStabilizationResult,
    CudaActiveBlockQueryResult,
    CudaBlockCacheBenchmarkResult,
    InfoResult,
    MeshCheckResult,
    QueryResult,
    RecommendationResult,
    RunSummaryResult,
    SolverContactsResult,
    SparseBenchmarkResult,
    SparseCollisionResult,
    SparseQueryResult,
    StrictManifestResult,
    StrictReportValidationResult,
    WorldBroadphaseResult,
    WorldSolverContactsResult,
    WorldSparseCollisionResult,
)
from .runner import run_command
from .exceptions import AdaSDFCommandError

PathLike = Union[str, Path]


def _config(bin_dir: Optional[PathLike]) -> AdaSDFConfig:
    return AdaSDFConfig(bin_dir=Path(bin_dir) if bin_dir is not None else None)


def _tool(name: str, bin_dir: Optional[PathLike], dry_run: bool) -> str:
    config = _config(bin_dir)
    if dry_run:
        if bin_dir is not None:
            return preview_tool(name, Path(bin_dir))
        try:
            return str(find_tool(name, config))
        except Exception:
            return preview_tool(name, None)
    return str(find_tool(name, config))


def _path(value: PathLike) -> str:
    return str(Path(value))


def _append_value(command: List[str], flag: str, value: object) -> None:
    if value is not None:
        command.extend([flag, str(value)])


def _append_bool(command: List[str], flag: str, enabled: bool) -> None:
    if enabled:
        command.append(flag)


def _append_bool_switch(
    command: List[str],
    enabled: Optional[bool],
    yes_flag: str,
    no_flag: str,
) -> None:
    if enabled is True:
        command.append(yes_flag)
    elif enabled is False:
        command.append(no_flag)


def _append_strict(
    command: List[str],
    strict_json: Optional[PathLike],
    case_id: Optional[str],
) -> None:
    _append_value(command, "--strict-json", _path(strict_json) if strict_json is not None else None)
    _append_value(command, "--case-id", case_id)


def _append_signed(command: List[str], signed: bool, unsigned: bool) -> None:
    if unsigned:
        command.append("--unsigned")
    elif signed:
        command.append("--signed")


def _append_sampling_options(
    command: List[str],
    *,
    sampling: Optional[str],
    coarse_resolution: Optional[int],
    quality_check_samples: Optional[int],
    far_field_interpolation: Optional[bool],
    transition_prediction: Optional[bool],
    near_surface_exact: bool,
    quality_guard: Optional[bool],
    target_sampling_error: Optional[float],
) -> None:
    _append_value(command, "--sampling", sampling)
    _append_value(command, "--coarse-resolution", coarse_resolution)
    _append_value(command, "--quality-check-samples", quality_check_samples)
    _append_bool_switch(
        command,
        far_field_interpolation,
        "--far-field-interpolation",
        "--no-far-field-interpolation",
    )
    _append_bool_switch(
        command,
        transition_prediction,
        "--transition-prediction",
        "--no-transition-prediction",
    )
    _append_bool(command, "--near-surface-exact", near_surface_exact)
    _append_bool_switch(command, quality_guard, "--quality-guard", "--no-quality-guard")
    _append_value(command, "--target-sampling-error", target_sampling_error)


def _run(
    command: List[str],
    *,
    check: bool,
    dry_run: bool,
) -> CommandResult:
    return run_command(command, check=check, dry_run=dry_run)


def capabilities(*, bin_dir: Optional[PathLike] = None, check: bool = True, dry_run: bool = False) -> CommandResult:
    command = [_tool("adasdf_capabilities", bin_dir, dry_run), "--verbose"]
    return _run(command, check=check, dry_run=dry_run)


def write_manifest(
    *,
    out: PathLike,
    case_id: str = "default",
    tool: str = "python",
    input_path: Optional[PathLike] = None,
    output_path: Optional[PathLike] = None,
    params: Optional[Mapping[str, object]] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> StrictManifestResult:
    command = [
        _tool("adasdf_write_manifest", bin_dir, dry_run),
        "--case-id",
        case_id,
        "--tool",
        tool,
        "--out",
        _path(out),
    ]
    _append_value(command, "--input", _path(input_path) if input_path is not None else None)
    _append_value(command, "--output", _path(output_path) if output_path is not None else None)
    for key, value in (params or {}).items():
        command.extend(["--param", f"{key}={value}"])
    result = _run(command, check=check, dry_run=dry_run)
    return StrictManifestResult(result, report_path=Path(out))


def validate_report(
    report_json: PathLike,
    *,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> StrictReportValidationResult:
    command = [_tool("adasdf_validate_report", bin_dir, dry_run), _path(report_json)]
    result = _run(command, check=check, dry_run=dry_run)
    parsed_path = parse_strict_report_path(result.stdout)
    parsed_valid = parse_strict_report_valid(result.stdout)
    return StrictReportValidationResult(
        result,
        report_path=Path(parsed_path) if parsed_path else Path(report_json),
        valid=None if dry_run else (parsed_valid if parsed_valid is not None else result.returncode == 0),
    )


def collect_run_summary(
    inputs: PathLike,
    out: PathLike,
    *,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> RunSummaryResult:
    command = [
        _tool("adasdf_collect_run_summary", bin_dir, dry_run),
        "--inputs",
        _path(inputs),
        "--out",
        _path(out),
    ]
    result = _run(command, check=check, dry_run=dry_run)
    parsed_path = parse_run_summary_csv_path(result.stdout)
    return RunSummaryResult(result, inputs_path=Path(inputs), csv_path=Path(parsed_path) if parsed_path else Path(out))


def mesh_check(
    input_stl: PathLike,
    *,
    out: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    readiness: bool = False,
    strict: bool = False,
    lenient: bool = False,
    require_watertight: bool = False,
    allow_open: bool = False,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> MeshCheckResult:
    command = [_tool("adasdf_mesh_check", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_strict(command, strict_json, case_id)
    _append_bool(command, "--readiness", readiness)
    _append_bool(command, "--strict", strict)
    _append_bool(command, "--lenient", lenient)
    _append_bool(command, "--require-watertight", require_watertight)
    _append_bool(command, "--allow-open", allow_open)
    result = _run(command, check=check, dry_run=dry_run)
    return MeshCheckResult(
        command_result=result,
        report_path=Path(out) if out is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def mesh_clean(
    input_stl: PathLike,
    output_stl: PathLike,
    *,
    report: Optional[PathLike] = None,
    merge_tolerance: Optional[float] = None,
    area_eps: Optional[float] = None,
    no_merge_vertices: bool = False,
    no_remove_degenerate: bool = False,
    no_remove_duplicates: bool = False,
    no_remove_unused: bool = False,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BuildResult:
    command = [_tool("adasdf_mesh_clean", bin_dir, dry_run), _path(input_stl), _path(output_stl)]
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--merge-tolerance", merge_tolerance)
    _append_value(command, "--area-eps", area_eps)
    _append_bool(command, "--no-merge-vertices", no_merge_vertices)
    _append_bool(command, "--no-remove-degenerate", no_remove_degenerate)
    _append_bool(command, "--no-remove-duplicates", no_remove_duplicates)
    _append_bool(command, "--no-remove-unused", no_remove_unused)
    result = _run(command, check=check, dry_run=dry_run)
    return BuildResult(
        command_result=result,
        output_path=Path(output_stl),
        report_path=Path(report) if report is not None else None,
    )


def recommend_build(
    input_stl: PathLike,
    *,
    target_error: float = 1e-3,
    memory_mb: float = 512.0,
    use_case: str = "contact",
    signed: bool = True,
    unsigned: bool = False,
    allow_open_unsigned: bool = False,
    allow_dense_fallback: bool = True,
    prefer_compression: bool = True,
    prefer_fast_build: bool = False,
    profile: Optional[PathLike] = None,
    out: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    emit_command: bool = True,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> RecommendationResult:
    command = [_tool("adasdf_recommend_build", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--memory-mb", memory_mb)
    _append_value(command, "--use-case", use_case)
    _append_signed(command, signed, unsigned)
    _append_bool(command, "--allow-open-unsigned", allow_open_unsigned)
    command.append("--allow-dense-fallback" if allow_dense_fallback else "--no-dense-fallback")
    command.append("--prefer-compression" if prefer_compression else "--no-prefer-compression")
    _append_bool(command, "--prefer-fast-build", prefer_fast_build)
    _append_value(command, "--profile", _path(profile) if profile is not None else None)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_bool(command, "--emit-command", emit_command)
    result = _run(command, check=check, dry_run=dry_run)
    return RecommendationResult(
        command_result=result,
        report_path=Path(out) if out is not None else None,
        json_path=Path(json) if json is not None else None,
        recommended_command=parse_recommended_command(result.stdout),
        recommended_path=parse_recommended_path(result.stdout),
    )


def build_dense_sdf(
    input_stl: PathLike,
    output_sdfbin: PathLike,
    *,
    resolution: int = 64,
    padding: float = 0.05,
    signed: bool = True,
    unsigned: bool = False,
    auto_clean: bool = False,
    accel: Optional[str] = None,
    threads: Optional[int] = None,
    benchmark_brute_reference: bool = False,
    sampling: Optional[str] = None,
    coarse_resolution: Optional[int] = None,
    quality_check_samples: Optional[int] = None,
    far_field_interpolation: Optional[bool] = None,
    transition_prediction: Optional[bool] = None,
    near_surface_exact: bool = False,
    quality_guard: Optional[bool] = None,
    target_sampling_error: Optional[float] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BuildResult:
    command = [_tool("adasdf_build_dense_sdf", bin_dir, dry_run), _path(input_stl), _path(output_sdfbin)]
    _append_value(command, "--resolution", resolution)
    _append_value(command, "--padding", padding)
    _append_signed(command, signed, unsigned)
    _append_bool(command, "--auto-clean", auto_clean)
    _append_value(command, "--accel", accel)
    _append_value(command, "--threads", threads)
    _append_bool(command, "--benchmark-brute-reference", benchmark_brute_reference)
    _append_sampling_options(
        command,
        sampling=sampling,
        coarse_resolution=coarse_resolution,
        quality_check_samples=quality_check_samples,
        far_field_interpolation=far_field_interpolation,
        transition_prediction=transition_prediction,
        near_surface_exact=near_surface_exact,
        quality_guard=quality_guard,
        target_sampling_error=target_sampling_error,
    )
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return BuildResult(result, Path(output_sdfbin), Path(report) if report is not None else None, Path(json) if json is not None else None)


def build_adaptive_sdf(
    input_stl: PathLike,
    output_sdfbin: PathLike,
    *,
    target_error: float = 1e-3,
    max_level: int = 5,
    min_level: Optional[int] = None,
    block_resolution: int = 8,
    padding: float = 0.05,
    signed: bool = True,
    unsigned: bool = False,
    auto_clean: bool = False,
    accel: Optional[str] = None,
    threads: Optional[int] = None,
    benchmark_brute_reference: bool = False,
    sampling: Optional[str] = None,
    coarse_resolution: Optional[int] = None,
    quality_check_samples: Optional[int] = None,
    far_field_interpolation: Optional[bool] = None,
    transition_prediction: Optional[bool] = None,
    near_surface_exact: bool = False,
    quality_guard: Optional[bool] = None,
    target_sampling_error: Optional[float] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    dry_run_builder: bool = False,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BuildResult:
    command = [_tool("adasdf_build_adaptive_sdf", bin_dir, dry_run), _path(input_stl), _path(output_sdfbin)]
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--min-level", min_level)
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_value(command, "--padding", padding)
    _append_signed(command, signed, unsigned)
    _append_bool(command, "--auto-clean", auto_clean)
    _append_value(command, "--accel", accel)
    _append_value(command, "--threads", threads)
    _append_bool(command, "--benchmark-brute-reference", benchmark_brute_reference)
    _append_sampling_options(
        command,
        sampling=sampling,
        coarse_resolution=coarse_resolution,
        quality_check_samples=quality_check_samples,
        far_field_interpolation=far_field_interpolation,
        transition_prediction=transition_prediction,
        near_surface_exact=near_surface_exact,
        quality_guard=quality_guard,
        target_sampling_error=target_sampling_error,
    )
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_strict(command, strict_json, case_id)
    _append_bool(command, "--dry-run", dry_run_builder)
    result = _run(command, check=check, dry_run=dry_run)
    output_path = None if dry_run_builder else Path(output_sdfbin)
    return BuildResult(result, output_path, Path(report) if report is not None else None, Path(json) if json is not None else None)


def compress_adaptive_sdf(
    input_adaptive_sdfbin: PathLike,
    output_compressed_sdfbin: PathLike,
    *,
    target_error: float = 1e-3,
    max_rank: int = 8,
    fixed_rank: Optional[int] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    quality_report: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BuildResult:
    command = [
        _tool("adasdf_compress_adaptive_sdf", bin_dir, dry_run),
        _path(input_adaptive_sdfbin),
        _path(output_compressed_sdfbin),
    ]
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--max-rank", max_rank)
    _append_value(command, "--fixed-rank", fixed_rank)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--quality-report", _path(quality_report) if quality_report is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return BuildResult(result, Path(output_compressed_sdfbin), Path(report) if report is not None else None, Path(json) if json is not None else None)


def build_compressed_sdf(
    input_stl: PathLike,
    output_sdfbin: PathLike,
    *,
    target_error: float = 1e-3,
    max_level: int = 5,
    min_level: Optional[int] = None,
    block_resolution: int = 8,
    max_rank: int = 8,
    fixed_rank: Optional[int] = None,
    padding: float = 0.05,
    signed: bool = True,
    unsigned: bool = False,
    auto_clean: bool = False,
    accel: Optional[str] = None,
    threads: Optional[int] = None,
    benchmark_brute_reference: bool = False,
    sampling: Optional[str] = None,
    coarse_resolution: Optional[int] = None,
    quality_check_samples: Optional[int] = None,
    far_field_interpolation: Optional[bool] = None,
    transition_prediction: Optional[bool] = None,
    near_surface_exact: bool = False,
    quality_guard: Optional[bool] = None,
    target_sampling_error: Optional[float] = None,
    report: Optional[PathLike] = None,
    compression_report: Optional[PathLike] = None,
    quality_report: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BuildResult:
    command = [_tool("adasdf_build_compressed_sdf", bin_dir, dry_run), _path(input_stl), _path(output_sdfbin)]
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--min-level", min_level)
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_value(command, "--padding", padding)
    _append_signed(command, signed, unsigned)
    _append_bool(command, "--auto-clean", auto_clean)
    _append_value(command, "--accel", accel)
    _append_value(command, "--threads", threads)
    _append_bool(command, "--benchmark-brute-reference", benchmark_brute_reference)
    _append_sampling_options(
        command,
        sampling=sampling,
        coarse_resolution=coarse_resolution,
        quality_check_samples=quality_check_samples,
        far_field_interpolation=far_field_interpolation,
        transition_prediction=transition_prediction,
        near_surface_exact=near_surface_exact,
        quality_guard=quality_guard,
        target_sampling_error=target_sampling_error,
    )
    _append_value(command, "--fixed-rank", fixed_rank)
    _append_value(command, "--max-rank", max_rank)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--compression-report", _path(compression_report) if compression_report is not None else None)
    _append_value(command, "--quality-report", _path(quality_report) if quality_report is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return BuildResult(result, Path(output_sdfbin), Path(report) if report is not None else None)


def info(sdfbin: PathLike, *, bin_dir: Optional[PathLike] = None, check: bool = True, dry_run: bool = False) -> InfoResult:
    command = [_tool("adasdf_info", bin_dir, dry_run), _path(sdfbin)]
    result = _run(command, check=check, dry_run=dry_run)
    return InfoResult(result, format=parse_info_format(result.stdout))


def query(
    sdfbin: PathLike,
    *,
    point: Sequence[float],
    output: Optional[str] = None,
    backend: Optional[str] = None,
    expansion: Optional[str] = None,
    blocks: Optional[Union[str, Iterable[int]]] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> QueryResult:
    unsupported = {
        "output": output,
        "backend": backend,
        "expansion": expansion,
        "blocks": blocks,
    }
    enabled_unsupported = [name for name, value in unsupported.items() if value is not None]
    if enabled_unsupported:
        raise ValueError(
            "adasdf_query currently supports single-point phi+normal CPU-backend "
            "queries only; unsupported option(s): " + ", ".join(enabled_unsupported)
        )
    values = list(point)
    if len(values) != 3:
        raise ValueError("point must contain exactly three coordinates")
    command = [_tool("adasdf_query", bin_dir, dry_run), _path(sdfbin), "--point", *(str(value) for value in values)]
    result = _run(command, check=check, dry_run=dry_run)
    return QueryResult(result, phi=parse_query_phi(result.stdout), normal=parse_query_normal(result.stdout))


def collide(
    sdfbin_a: PathLike,
    sdfbin_b: PathLike,
    *,
    max_contacts: int = 4,
    backend: Optional[str] = None,
    expansion: Optional[str] = None,
    blocks: Optional[Union[str, Iterable[int]]] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> CollisionResult:
    command = [_tool("adasdf_collide", bin_dir, dry_run), _path(sdfbin_a), _path(sdfbin_b)]
    _append_value(command, "--max-contacts", max_contacts)
    _append_value(command, "--backend", backend)
    _append_value(command, "--expansion", expansion)
    if blocks is not None:
        block_text = blocks if isinstance(blocks, str) else ",".join(str(block) for block in blocks)
        _append_value(command, "--blocks", block_text)
    result = _run(command, check=check, dry_run=dry_run)
    return CollisionResult(
        result,
        colliding=parse_collision_colliding(result.stdout),
        contact_count=parse_contact_count(result.stdout),
        minimum_distance=parse_minimum_distance(result.stdout),
    )


def expansion_quality(
    sdfbin: PathLike,
    *,
    resolution: Optional[int] = None,
    out: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> CommandResult:
    del json
    command = [_tool("adasdf_expansion_quality", bin_dir, dry_run), _path(sdfbin), "--expansion", "global"]
    _append_value(command, "--global-resolution", resolution)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_strict(command, strict_json, case_id)
    return _run(command, check=check, dry_run=dry_run)


def benchmark_batch_query(
    *,
    model: Optional[PathLike] = None,
    points: Union[int, Iterable[int], str] = 10000,
    query_backend: str = "cpu",
    expansion: str = "global",
    output: str = "phi",
    warmup: Optional[int] = None,
    repeat: Optional[int] = None,
    out: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BenchmarkResult:
    if isinstance(points, str):
        points_text = points
    elif isinstance(points, int):
        points_text = str(points)
    else:
        points_text = ",".join(str(point_count) for point_count in points)
    command = [_tool("adasdf_benchmark_batch_query", bin_dir, dry_run)]
    _append_value(command, "--model", _path(model) if model is not None else None)
    _append_value(command, "--points", points_text)
    _append_value(command, "--query-backend", query_backend)
    _append_value(command, "--expansion", expansion)
    _append_value(command, "--output", output)
    _append_value(command, "--warmup", warmup)
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return BenchmarkResult(result, metrics=parse_benchmark_metrics(result.stdout))


def benchmark_builder_acceleration(
    input_stl: PathLike,
    *,
    builder: str = "dense",
    accel: str = "bvh",
    threads: Optional[int] = None,
    repeat: int = 3,
    warmup: int = 1,
    benchmark_brute_reference: bool = False,
    resolution: Optional[int] = None,
    min_level: Optional[int] = None,
    max_level: Optional[int] = None,
    block_resolution: Optional[int] = None,
    unsigned: bool = False,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BenchmarkResult:
    command = [_tool("adasdf_benchmark_builder_acceleration", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--builder", builder)
    _append_value(command, "--accel", accel)
    _append_value(command, "--threads", threads)
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--warmup", warmup)
    _append_bool(command, "--benchmark-brute-reference", benchmark_brute_reference)
    _append_value(command, "--resolution", resolution)
    _append_value(command, "--min-level", min_level)
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_bool(command, "--unsigned", unsigned)
    result = _run(command, check=check, dry_run=dry_run)
    return BenchmarkResult(result, metrics=parse_build_acceleration_stats(result.stdout))


def benchmark_hierarchical_sampling(
    input_stl: PathLike,
    *,
    min_level: Optional[int] = None,
    max_level: Optional[int] = None,
    block_resolution: Optional[int] = None,
    target_error: Optional[float] = None,
    target_sampling_error: Optional[float] = None,
    coarse_resolution: Optional[int] = None,
    quality_check_samples: Optional[int] = None,
    transition_quality_check_samples: Optional[int] = None,
    far_field_quality_check: Optional[str] = None,
    far_field_safety_factor: Optional[float] = None,
    far_field_sign_policy: Optional[str] = None,
    near_surface_mode: Optional[str] = None,
    near_surface_band_factor: Optional[float] = None,
    near_surface_check_samples: Optional[int] = None,
    halo_exact_layers: Optional[int] = None,
    near_surface_node_fallback: Optional[bool] = None,
    comparison_samples: Optional[int] = None,
    accel: Optional[str] = None,
    threads: Optional[int] = None,
    unsigned: bool = False,
    far_field_interpolation: Optional[bool] = None,
    transition_prediction: Optional[bool] = None,
    near_surface_exact: bool = False,
    hierarchical_diagnostics: Optional[bool] = None,
    quality_guard: Optional[bool] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    csv: Optional[PathLike] = None,
    diagnostics_report: Optional[PathLike] = None,
    diagnostics_csv: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BenchmarkResult:
    command = [_tool("adasdf_benchmark_hierarchical_sampling", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--min-level", min_level)
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--target-sampling-error", target_sampling_error)
    _append_value(command, "--coarse-resolution", coarse_resolution)
    _append_value(command, "--quality-check-samples", quality_check_samples)
    _append_value(
        command,
        "--transition-quality-check-samples",
        transition_quality_check_samples,
    )
    _append_value(command, "--far-field-quality-check", far_field_quality_check)
    _append_value(command, "--far-field-safety-factor", far_field_safety_factor)
    _append_value(command, "--far-field-sign-policy", far_field_sign_policy)
    _append_value(command, "--near-surface-mode", near_surface_mode)
    _append_value(command, "--near-surface-band-factor", near_surface_band_factor)
    _append_value(command, "--near-surface-check-samples", near_surface_check_samples)
    _append_value(command, "--halo-exact-layers", halo_exact_layers)
    _append_bool_switch(
        command,
        near_surface_node_fallback,
        "--near-surface-node-fallback",
        "--no-near-surface-node-fallback",
    )
    _append_value(command, "--comparison-samples", comparison_samples)
    _append_value(command, "--accel", accel)
    _append_value(command, "--threads", threads)
    _append_bool(command, "--unsigned", unsigned)
    _append_bool_switch(
        command,
        far_field_interpolation,
        "--far-field-interpolation",
        "--no-far-field-interpolation",
    )
    _append_bool_switch(
        command,
        transition_prediction,
        "--transition-prediction",
        "--no-transition-prediction",
    )
    _append_bool(command, "--near-surface-exact", near_surface_exact)
    _append_bool_switch(
        command,
        hierarchical_diagnostics,
        "--hierarchical-diagnostics",
        "--no-hierarchical-diagnostics",
    )
    _append_bool_switch(command, quality_guard, "--quality-guard", "--no-quality-guard")
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    _append_value(
        command,
        "--diagnostics-report",
        _path(diagnostics_report) if diagnostics_report is not None else None,
    )
    _append_value(
        command,
        "--diagnostics-csv",
        _path(diagnostics_csv) if diagnostics_csv is not None else None,
    )
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return BenchmarkResult(
        result,
        metrics=parse_hierarchical_sampling_benchmark_metrics(result.stdout),
    )


def benchmark_exact_hotpath(
    input_stl: PathLike,
    *,
    max_level: Optional[int] = None,
    block_resolution: Optional[int] = None,
    threads: Optional[int] = None,
    csv: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BenchmarkResult:
    command = [_tool("adasdf_benchmark_exact_hotpath", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_value(command, "--threads", threads)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--case-id", case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return BenchmarkResult(result, metrics=parse_exact_hotpath_benchmark_metrics(result.stdout))


def benchmark_contact_band_sampling(
    input_stl: PathLike,
    *,
    max_level: Optional[int] = None,
    min_level: Optional[int] = None,
    block_resolution: Optional[int] = None,
    target_error: Optional[float] = None,
    contact_band_width: Optional[float] = None,
    contact_band_layers: Optional[int] = None,
    halo_exact_layers: Optional[int] = None,
    contact_band_marker: Optional[str] = None,
    marker_cell_size_factor: Optional[float] = None,
    marker_safety_factor: Optional[float] = None,
    local_halo_only: bool = False,
    far_field_resolution: Optional[int] = None,
    far_field_mode: Optional[str] = None,
    reuse_far_field_sign: Optional[bool] = None,
    normal_audit: bool = False,
    coverage_audit: bool = False,
    coverage_samples_per_axis: Optional[int] = None,
    marker_cost_audit: bool = False,
    save_marker_debug_csv: Optional[PathLike] = None,
    threads: Optional[int] = None,
    csv: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> ContactBandBenchmarkResult:
    command = [_tool("adasdf_benchmark_contact_band_sampling", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--min-level", min_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--contact-band-width", contact_band_width)
    _append_value(command, "--contact-band-layers", contact_band_layers)
    _append_value(command, "--halo-exact-layers", halo_exact_layers)
    _append_value(command, "--contact-band-marker", contact_band_marker)
    _append_value(command, "--marker-cell-size-factor", marker_cell_size_factor)
    _append_value(command, "--marker-safety-factor", marker_safety_factor)
    _append_bool(command, "--local-halo-only", local_halo_only)
    _append_value(command, "--far-field-resolution", far_field_resolution)
    _append_value(command, "--far-field-mode", far_field_mode)
    _append_bool_switch(
        command,
        reuse_far_field_sign,
        "--reuse-far-field-sign",
        "--no-reuse-far-field-sign",
    )
    _append_bool(command, "--normal-audit", normal_audit)
    _append_bool(command, "--coverage-audit", coverage_audit)
    _append_value(command, "--coverage-samples-per-axis", coverage_samples_per_axis)
    _append_bool(command, "--marker-cost-audit", marker_cost_audit)
    _append_value(
        command,
        "--save-marker-debug-csv",
        _path(save_marker_debug_csv) if save_marker_debug_csv is not None else None,
    )
    _append_value(command, "--threads", threads)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--case-id", case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return ContactBandBenchmarkResult(
        command_result=result,
        metrics=parse_contact_band_sampling_benchmark_metrics(result.stdout),
        report_path=Path(report) if report is not None else None,
        csv_path=Path(csv) if csv is not None else None,
        marker_debug_csv_path=Path(save_marker_debug_csv) if save_marker_debug_csv is not None else None,
    )


def sweep_hierarchical_sampling(
    input_stl: PathLike,
    *,
    max_level: Optional[str] = None,
    block_resolution: Optional[int] = None,
    target_error: Optional[float] = None,
    target_sampling_error: Optional[float] = None,
    coarse_resolution: Optional[str] = None,
    transition_quality_check_samples: Optional[str] = None,
    far_field_quality_check: Optional[str] = None,
    far_field_safety_factor: Optional[str] = None,
    quality_check_samples: Optional[int] = None,
    comparison_samples: Optional[int] = None,
    accel: Optional[str] = None,
    threads: Optional[int] = None,
    unsigned: bool = False,
    csv: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BenchmarkResult:
    command = [_tool("adasdf_sweep_hierarchical_sampling", bin_dir, dry_run), _path(input_stl)]
    _append_value(command, "--max-level", max_level)
    _append_value(command, "--block-resolution", block_resolution)
    _append_value(command, "--target-error", target_error)
    _append_value(command, "--target-sampling-error", target_sampling_error)
    _append_value(command, "--coarse-resolution", coarse_resolution)
    _append_value(
        command,
        "--transition-quality-check-samples",
        transition_quality_check_samples,
    )
    _append_value(command, "--far-field-quality-check", far_field_quality_check)
    _append_value(command, "--far-field-safety-factor", far_field_safety_factor)
    _append_value(command, "--quality-check-samples", quality_check_samples)
    _append_value(command, "--comparison-samples", comparison_samples)
    _append_value(command, "--accel", accel)
    _append_value(command, "--threads", threads)
    _append_bool(command, "--unsigned", unsigned)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return BenchmarkResult(result, metrics=parse_hierarchical_sampling_benchmark_metrics(result.stdout))


def sparse_query(
    model: PathLike,
    samples_csv: PathLike,
    *,
    threshold: float = 0.0,
    with_normal: bool = False,
    early_exit: bool = False,
    use_radius: bool = True,
    include_non_colliding: bool = True,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> SparseQueryResult:
    command = [_tool("adasdf_sparse_query", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--threshold", threshold)
    command.append("--with-normal" if with_normal else "--phi-only")
    _append_bool(command, "--early-exit", early_exit)
    _append_bool(command, "--no-radius", not use_radius)
    command.append("--include-non-colliding" if include_non_colliding else "--colliding-only")
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return SparseQueryResult(
        command_result=result,
        colliding=parse_collision_colliding(result.stdout),
        sample_count=parse_field_int(result.stdout, "Sample count"),
        queried_count=parse_field_int(result.stdout, "Queried samples"),
        result_count=parse_field_int(result.stdout, "Result count"),
        min_effective_phi=parse_field_float(result.stdout, "Min effective phi"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def sparse_collide(
    model: PathLike,
    samples_csv: PathLike,
    *,
    mode: str = "collision-only",
    threshold: float = 0.0,
    early_exit: bool = True,
    with_normal: bool = False,
    use_radius: bool = True,
    return_all_violations: bool = False,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> SparseCollisionResult:
    command = [_tool("adasdf_sparse_collide", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--mode", mode)
    _append_value(command, "--threshold", threshold)
    command.append("--early-exit" if early_exit else "--no-early-exit")
    _append_bool(command, "--with-normal", with_normal)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--return-all-violations", return_all_violations)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = run_command(command, check=False, dry_run=dry_run)
    if check and result.returncode not in (0, 10):
        raise AdaSDFCommandError(result.command, result.returncode, result.stdout, result.stderr)
    return SparseCollisionResult(
        command_result=result,
        colliding=parse_collision_colliding(result.stdout) if result.returncode != 10 else True,
        min_effective_phi=parse_field_float(result.stdout, "Min effective phi"),
        queried_count=parse_field_int(result.stdout, "Queried samples"),
        early_exit=parse_field_bool(result.stdout, "Early exit"),
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def contact_candidates(
    model: PathLike,
    samples_csv: PathLike,
    *,
    top_k: int = 8,
    threshold: float = 0.0,
    reduction_radius: float = 0.0,
    with_normal: bool = True,
    use_radius: bool = True,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> ContactCandidatesResult:
    command = [_tool("adasdf_contact_candidates", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--top-k", top_k)
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--reduction-radius", reduction_radius)
    command.append("--with-normal" if with_normal else "--no-normal")
    _append_bool(command, "--no-radius", not use_radius)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return ContactCandidatesResult(
        command_result=result,
        threshold_candidate_count=parse_field_int(result.stdout, "Threshold candidates"),
        candidate_count=parse_field_int(result.stdout, "Reduced candidates"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def stabilize_contacts(
    candidates_csv: PathLike,
    *,
    max_contacts: int = 8,
    max_contacts_per_link: Optional[int] = None,
    max_contacts_per_patch: Optional[int] = None,
    patch_radius: float = 0.02,
    normal_cos: Optional[float] = None,
    min_penetration: Optional[float] = None,
    cluster: bool = True,
    normal_consistency: bool = True,
    duplicate_removal: bool = True,
    out: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> ContactStabilizationResult:
    command = [_tool("adasdf_stabilize_contacts", bin_dir, dry_run), _path(candidates_csv)]
    _append_value(command, "--max-contacts", max_contacts)
    _append_value(command, "--max-contacts-per-link", max_contacts_per_link)
    _append_value(command, "--max-contacts-per-patch", max_contacts_per_patch)
    _append_value(command, "--patch-radius", patch_radius)
    _append_value(command, "--normal-cos", normal_cos)
    _append_value(command, "--min-penetration", min_penetration)
    _append_bool(command, "--no-cluster", not cluster)
    _append_bool(command, "--no-normal-consistency", not normal_consistency)
    _append_bool(command, "--no-duplicate-removal", not duplicate_removal)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return ContactStabilizationResult(
        command_result=result,
        input_candidate_count=parse_field_int(result.stdout, "Input candidates"),
        patch_count=parse_patch_count(result.stdout),
        solver_contact_count=parse_solver_contact_count(result.stdout),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def solver_contact_candidates(
    model: PathLike,
    samples_csv: PathLike,
    *,
    threshold: float = 1e-3,
    top_k: int = 32,
    reduction_radius: float = 0.0,
    max_contacts: int = 8,
    max_contacts_per_link: Optional[int] = None,
    max_contacts_per_patch: Optional[int] = None,
    patch_radius: float = 0.02,
    normal_cos: Optional[float] = None,
    min_penetration: Optional[float] = None,
    with_normal: bool = True,
    use_radius: bool = True,
    out: Optional[PathLike] = None,
    candidates_out: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> SolverContactsResult:
    command = [_tool("adasdf_solver_contact_candidates", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--top-k", top_k)
    _append_value(command, "--reduction-radius", reduction_radius)
    _append_value(command, "--max-contacts", max_contacts)
    _append_value(command, "--max-contacts-per-link", max_contacts_per_link)
    _append_value(command, "--max-contacts-per-patch", max_contacts_per_patch)
    _append_value(command, "--patch-radius", patch_radius)
    _append_value(command, "--normal-cos", normal_cos)
    _append_value(command, "--min-penetration", min_penetration)
    command.append("--with-normal" if with_normal else "--no-normal")
    _append_bool(command, "--no-radius", not use_radius)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--candidates-out", _path(candidates_out) if candidates_out is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return SolverContactsResult(
        command_result=result,
        sample_count=parse_field_int(result.stdout, "Samples"),
        raw_candidate_count=parse_raw_candidate_count(result.stdout),
        patch_count=parse_patch_count(result.stdout),
        solver_contact_count=parse_solver_contact_count(result.stdout),
        output_path=Path(out) if out is not None else None,
        candidates_path=Path(candidates_out) if candidates_out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def benchmark_contact_reduction(
    model: PathLike,
    samples_csv: PathLike,
    *,
    threshold: float = 1e-3,
    top_k: int = 64,
    max_contacts: int = 8,
    patch_radius: float = 0.02,
    repeat: int = 10,
    warmup: int = 1,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    csv: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> ContactReductionBenchmarkResult:
    command = [_tool("adasdf_benchmark_contact_reduction", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--top-k", top_k)
    _append_value(command, "--max-contacts", max_contacts)
    _append_value(command, "--patch-radius", patch_radius)
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--warmup", warmup)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return ContactReductionBenchmarkResult(
        command_result=result,
        metrics=parse_contact_reduction_benchmark_metrics(result.stdout),
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
        csv_path=Path(csv) if csv is not None else None,
    )


def benchmark_sparse_query(
    model: PathLike,
    samples_csv: PathLike,
    *,
    repeat: int = 10,
    warmup: int = 1,
    mode: str = "phi-only",
    threshold: float = 0.0,
    top_k: Optional[int] = None,
    early_exit: bool = False,
    with_normal: bool = False,
    use_radius: bool = True,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    csv: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> SparseBenchmarkResult:
    command = [_tool("adasdf_benchmark_sparse_query", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--warmup", warmup)
    _append_value(command, "--mode", mode)
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--top-k", top_k)
    _append_bool(command, "--early-exit", early_exit)
    _append_bool(command, "--with-normal", with_normal)
    _append_bool(command, "--no-radius", not use_radius)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return SparseBenchmarkResult(
        command_result=result,
        metrics=parse_sparse_benchmark_metrics(result.stdout),
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
        csv_path=Path(csv) if csv is not None else None,
    )


def select_active_blocks(
    model: PathLike,
    samples_csv: PathLike,
    *,
    threshold: float = 0.0,
    selection_band: float = 0.0,
    extra_margin: float = 0.0,
    use_radius: bool = True,
    include_neighbors: bool = True,
    query_phi_for_selection: bool = True,
    max_active_blocks: Optional[int] = None,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> ActiveBlockSelectionResult:
    command = [_tool("adasdf_select_active_blocks", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--selection-band", selection_band)
    _append_value(command, "--extra-margin", extra_margin)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--no-neighbors", not include_neighbors)
    _append_bool(command, "--no-phi-selection", not query_phi_for_selection)
    _append_value(command, "--max-active-blocks", max_active_blocks)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return ActiveBlockSelectionResult(
        command_result=result,
        active_block_count=parse_field_int(result.stdout, "Active blocks"),
        candidate_sample_count=parse_field_int(result.stdout, "Candidate samples"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def active_block_query(
    model: PathLike,
    samples_csv: PathLike,
    *,
    threshold: float = 0.0,
    selection_band: float = 0.0,
    extra_margin: float = 0.0,
    with_normal: bool = False,
    early_exit: bool = False,
    use_radius: bool = True,
    include_neighbors: bool = True,
    fallback: bool = True,
    include_non_colliding: bool = True,
    sort: bool = False,
    cache_max_blocks: Optional[int] = None,
    cache_max_mb: Optional[float] = None,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> ActiveBlockQueryResult:
    command = [_tool("adasdf_active_block_query", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--selection-band", selection_band)
    _append_value(command, "--extra-margin", extra_margin)
    command.append("--with-normal" if with_normal else "--phi-only")
    _append_bool(command, "--early-exit", early_exit)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--no-neighbors", not include_neighbors)
    _append_bool(command, "--no-fallback", not fallback)
    _append_bool(command, "--colliding-only", not include_non_colliding)
    _append_bool(command, "--sort", sort)
    _append_value(command, "--cache-max-blocks", cache_max_blocks)
    _append_value(command, "--cache-max-mb", cache_max_mb)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = run_command(command, check=False, dry_run=dry_run)
    if check and result.returncode not in (0, 10):
        raise AdaSDFCommandError(result.command, result.returncode, result.stdout, result.stderr)
    return ActiveBlockQueryResult(
        command_result=result,
        colliding=True if result.returncode == 10 else parse_collision_colliding(result.stdout),
        sample_count=parse_field_int(result.stdout, "Sample count"),
        queried_count=parse_field_int(result.stdout, "Queried count"),
        result_count=parse_field_int(result.stdout, "Result count"),
        cache_query_count=parse_field_int(result.stdout, "Cache queries"),
        fallback_query_count=parse_field_int(result.stdout, "Fallback queries"),
        resident_blocks=parse_field_int(result.stdout, "Resident blocks"),
        min_effective_phi=parse_field_float(result.stdout, "Min effective phi"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def benchmark_block_cache(
    model: PathLike,
    samples_csv: PathLike,
    *,
    repeat: int = 10,
    warmup: int = 1,
    mode: str = "phi-only",
    threshold: float = 0.0,
    selection_band: float = 0.0,
    extra_margin: float = 0.0,
    use_radius: bool = True,
    include_neighbors: bool = True,
    cache_max_blocks: Optional[int] = None,
    cache_max_mb: Optional[float] = None,
    compare_direct: bool = False,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    csv: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BlockCacheBenchmarkResult:
    command = [_tool("adasdf_benchmark_block_cache", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--warmup", warmup)
    _append_value(command, "--mode", mode)
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--selection-band", selection_band)
    _append_value(command, "--extra-margin", extra_margin)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--no-neighbors", not include_neighbors)
    _append_value(command, "--cache-max-blocks", cache_max_blocks)
    _append_value(command, "--cache-max-mb", cache_max_mb)
    _append_bool(command, "--compare-direct", compare_direct)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return BlockCacheBenchmarkResult(
        command_result=result,
        metrics=parse_block_cache_benchmark_metrics(result.stdout),
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
        csv_path=Path(csv) if csv is not None else None,
    )


def cuda_active_block_query(
    model: PathLike,
    samples_csv: PathLike,
    *,
    threshold: float = 0.0,
    selection_band: float = 0.0,
    extra_margin: float = 0.0,
    with_normal: bool = False,
    early_exit: bool = False,
    use_radius: bool = True,
    include_neighbors: bool = True,
    fallback: bool = True,
    sort: bool = False,
    cache_max_blocks: Optional[int] = None,
    cache_max_mb: Optional[float] = None,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> CudaActiveBlockQueryResult:
    command = [_tool("adasdf_cuda_active_block_query", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--selection-band", selection_band)
    _append_value(command, "--extra-margin", extra_margin)
    command.append("--with-normal" if with_normal else "--phi-only")
    _append_bool(command, "--early-exit", early_exit)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--no-neighbors", not include_neighbors)
    _append_bool(command, "--no-fallback", not fallback)
    _append_bool(command, "--sort", sort)
    _append_value(command, "--cache-max-blocks", cache_max_blocks)
    _append_value(command, "--cache-max-mb", cache_max_mb)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = run_command(command, check=False, dry_run=dry_run)
    if check and result.returncode not in (0, 10, 20):
        raise AdaSDFCommandError(result.command, result.returncode, result.stdout, result.stderr)
    return CudaActiveBlockQueryResult(
        command_result=result,
        cuda_available=parse_field_bool(result.stdout, "CUDA available"),
        colliding=True if result.returncode == 10 else parse_collision_colliding(result.stdout),
        sample_count=parse_field_int(result.stdout, "Sample count"),
        queried_count=parse_field_int(result.stdout, "Queried count"),
        result_count=parse_field_int(result.stdout, "Result count"),
        active_block_count=parse_field_int(result.stdout, "Active blocks"),
        expanded_block_count=parse_field_int(result.stdout, "Expanded blocks"),
        fallback_query_count=parse_field_int(result.stdout, "Fallback queries"),
        gpu_memory_bytes=parse_field_int(result.stdout, "GPU memory bytes"),
        kernel_time_ms=parse_field_float(result.stdout, "Kernel time ms"),
        total_time_ms=parse_field_float(result.stdout, "Total time ms"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def benchmark_cuda_block_cache(
    model: PathLike,
    samples_csv: PathLike,
    *,
    repeat: int = 20,
    warmup: int = 5,
    mode: str = "phi-only",
    threshold: float = 0.0,
    selection_band: float = 0.0,
    extra_margin: float = 0.0,
    use_radius: bool = True,
    include_neighbors: bool = True,
    cache_max_blocks: Optional[int] = None,
    cache_max_mb: Optional[float] = None,
    compare_cpu_active: bool = True,
    compare_direct: bool = True,
    compare_global: bool = False,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    csv: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> CudaBlockCacheBenchmarkResult:
    command = [_tool("adasdf_benchmark_cuda_block_cache", bin_dir, dry_run), _path(model), _path(samples_csv)]
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--warmup", warmup)
    _append_value(command, "--mode", mode)
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--selection-band", selection_band)
    _append_value(command, "--extra-margin", extra_margin)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--no-neighbors", not include_neighbors)
    _append_value(command, "--cache-max-blocks", cache_max_blocks)
    _append_value(command, "--cache-max-mb", cache_max_mb)
    _append_bool(command, "--no-compare-cpu-active", not compare_cpu_active)
    _append_bool(command, "--no-compare-direct", not compare_direct)
    _append_bool(command, "--compare-global", compare_global)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    result = run_command(command, check=False, dry_run=dry_run)
    if check and result.returncode not in (0, 20):
        raise AdaSDFCommandError(result.command, result.returncode, result.stdout, result.stderr)
    return CudaBlockCacheBenchmarkResult(
        command_result=result,
        cuda_available=parse_field_bool(result.stdout, "CUDA available"),
        metrics=parse_cuda_block_cache_benchmark_metrics(result.stdout),
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
        csv_path=Path(csv) if csv is not None else None,
    )


def world_broadphase(
    scene_csv: PathLike,
    *,
    include_disabled: bool = False,
    include_static_static: bool = False,
    use_group_mask: bool = True,
    aabb_margin: Optional[float] = None,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> WorldBroadphaseResult:
    command = [_tool("adasdf_world_broadphase", bin_dir, dry_run), _path(scene_csv)]
    _append_bool(command, "--include-disabled", include_disabled)
    _append_bool(command, "--include-static-static", include_static_static)
    _append_bool(command, "--no-group-mask", not use_group_mask)
    _append_value(command, "--aabb-margin", aabb_margin)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return WorldBroadphaseResult(
        command_result=result,
        object_count=parse_field_int(result.stdout, "Objects"),
        tested_pair_count=parse_field_int(result.stdout, "Tested pairs"),
        overlap_pair_count=parse_field_int(result.stdout, "Overlap pairs"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def world_sparse_collide(
    scene_csv: PathLike,
    *,
    mode: str = "collision-only",
    threshold: float = 0.0,
    bidirectional: bool = True,
    early_exit: Optional[bool] = None,
    with_normal: bool = False,
    use_radius: bool = True,
    include_static_static: bool = False,
    use_group_mask: bool = True,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> WorldSparseCollisionResult:
    command = [_tool("adasdf_world_sparse_collide", bin_dir, dry_run), _path(scene_csv)]
    _append_value(command, "--mode", mode)
    _append_value(command, "--threshold", threshold)
    command.append("--bidirectional" if bidirectional else "--one-way")
    if early_exit is not None:
        command.append("--early-exit" if early_exit else "--no-early-exit")
    _append_bool(command, "--with-normal", with_normal)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--include-static-static", include_static_static)
    _append_bool(command, "--no-group-mask", not use_group_mask)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_strict(command, strict_json, case_id)
    result = run_command(command, check=False, dry_run=dry_run)
    if check and result.returncode not in (0, 10):
        raise AdaSDFCommandError(result.command, result.returncode, result.stdout, result.stderr)
    return WorldSparseCollisionResult(
        command_result=result,
        colliding=True if result.returncode == 10 else parse_collision_colliding(result.stdout),
        broadphase_pair_count=parse_field_int(result.stdout, "Broadphase pairs"),
        queried_pair_count=parse_field_int(result.stdout, "Queried pairs"),
        queried_sample_count=parse_field_int(result.stdout, "Queried samples"),
        violation_count=parse_field_int(result.stdout, "Violations"),
        min_effective_phi=parse_field_float(result.stdout, "Min effective phi"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def world_solver_contacts(
    scene_csv: PathLike,
    *,
    threshold: float = 1e-3,
    top_k: int = 64,
    reduction_radius: float = 0.0,
    max_contacts: int = 8,
    patch_radius: float = 0.02,
    use_radius: bool = True,
    include_static_static: bool = False,
    out: Optional[PathLike] = None,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> WorldSolverContactsResult:
    command = [_tool("adasdf_world_solver_contacts", bin_dir, dry_run), _path(scene_csv)]
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--top-k", top_k)
    _append_value(command, "--reduction-radius", reduction_radius)
    _append_value(command, "--max-contacts", max_contacts)
    _append_value(command, "--patch-radius", patch_radius)
    _append_bool(command, "--no-radius", not use_radius)
    _append_bool(command, "--include-static-static", include_static_static)
    _append_value(command, "--out", _path(out) if out is not None else None)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    result = _run(command, check=check, dry_run=dry_run)
    return WorldSolverContactsResult(
        command_result=result,
        raw_candidate_count=parse_raw_candidate_count(result.stdout),
        reduced_candidate_count=parse_field_int(result.stdout, "Reduced candidates"),
        patch_count=parse_patch_count(result.stdout),
        solver_contact_count=parse_solver_contact_count(result.stdout),
        max_penetration=parse_field_float(result.stdout, "Max penetration"),
        output_path=Path(out) if out is not None else None,
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
    )


def benchmark_collision_world(
    scene_csv: PathLike,
    *,
    mode: str = "sparse",
    threshold: float = 0.0,
    repeat: int = 10,
    warmup: int = 1,
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    csv: Optional[PathLike] = None,
    strict_json: Optional[PathLike] = None,
    case_id: Optional[str] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> CollisionWorldBenchmarkResult:
    command = [_tool("adasdf_benchmark_collision_world", bin_dir, dry_run), _path(scene_csv)]
    _append_value(command, "--mode", mode)
    _append_value(command, "--threshold", threshold)
    _append_value(command, "--repeat", repeat)
    _append_value(command, "--warmup", warmup)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
    _append_value(command, "--csv", _path(csv) if csv is not None else None)
    _append_strict(command, strict_json, case_id)
    result = _run(command, check=check, dry_run=dry_run)
    return CollisionWorldBenchmarkResult(
        command_result=result,
        metrics=parse_collision_world_benchmark_metrics(result.stdout),
        report_path=Path(report) if report is not None else None,
        json_path=Path(json) if json is not None else None,
        csv_path=Path(csv) if csv is not None else None,
    )

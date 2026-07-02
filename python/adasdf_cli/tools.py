"""Public Python helpers that call installed AdaSDF-CL CLI tools."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable, List, Optional, Sequence, Union

from .config import AdaSDFConfig, find_tool, preview_tool
from .parsers import (
    parse_benchmark_metrics,
    parse_collision_colliding,
    parse_contact_count,
    parse_info_format,
    parse_minimum_distance,
    parse_query_normal,
    parse_query_phi,
    parse_recommended_command,
    parse_recommended_path,
)
from .results import (
    BenchmarkResult,
    BuildResult,
    CollisionResult,
    CommandResult,
    InfoResult,
    MeshCheckResult,
    QueryResult,
    RecommendationResult,
)
from .runner import run_command

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


def _append_signed(command: List[str], signed: bool, unsigned: bool) -> None:
    if unsigned:
        command.append("--unsigned")
    elif signed:
        command.append("--signed")


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


def mesh_check(
    input_stl: PathLike,
    *,
    out: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
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
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> BuildResult:
    command = [_tool("adasdf_build_dense_sdf", bin_dir, dry_run), _path(input_stl), _path(output_sdfbin)]
    _append_value(command, "--resolution", resolution)
    _append_value(command, "--padding", padding)
    _append_signed(command, signed, unsigned)
    _append_bool(command, "--auto-clean", auto_clean)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
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
    report: Optional[PathLike] = None,
    json: Optional[PathLike] = None,
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
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--json", _path(json) if json is not None else None)
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
    report: Optional[PathLike] = None,
    compression_report: Optional[PathLike] = None,
    quality_report: Optional[PathLike] = None,
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
    _append_value(command, "--fixed-rank", fixed_rank)
    _append_value(command, "--max-rank", max_rank)
    _append_value(command, "--report", _path(report) if report is not None else None)
    _append_value(command, "--compression-report", _path(compression_report) if compression_report is not None else None)
    _append_value(command, "--quality-report", _path(quality_report) if quality_report is not None else None)
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
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> CommandResult:
    del json
    command = [_tool("adasdf_expansion_quality", bin_dir, dry_run), _path(sdfbin), "--expansion", "global"]
    _append_value(command, "--global-resolution", resolution)
    _append_value(command, "--out", _path(out) if out is not None else None)
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
    result = _run(command, check=check, dry_run=dry_run)
    return BenchmarkResult(result, metrics=parse_benchmark_metrics(result.stdout))

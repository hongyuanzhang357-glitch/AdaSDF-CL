"""Convenience workflows composed from the AdaSDF-CL Python CLI helpers."""

from __future__ import annotations

from typing import Dict, Optional, Union
from pathlib import Path

from . import tools

PathLike = Union[str, Path]


def recommend_then_build_compressed(
    input_stl: PathLike,
    output_sdfbin: PathLike,
    *,
    target_error: float = 1e-3,
    memory_mb: float = 512.0,
    use_case: str = "contact",
    recommendation_report: Optional[PathLike] = None,
    build_report: Optional[PathLike] = None,
    compression_report: Optional[PathLike] = None,
    quality_report: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> Dict[str, object]:
    """Recommend parameters and run an explicit compressed build command.

    This is a convenience wrapper, not a scheduler. It does not parse and
    execute the recommender-emitted command; the build step uses explicit
    arguments for reproducibility.
    """

    recommendation = tools.recommend_build(
        input_stl,
        target_error=target_error,
        memory_mb=memory_mb,
        use_case=use_case,
        out=recommendation_report,
        bin_dir=bin_dir,
        check=check,
        dry_run=dry_run,
    )
    build = tools.build_compressed_sdf(
        input_stl,
        output_sdfbin,
        target_error=target_error,
        report=build_report,
        compression_report=compression_report,
        quality_report=quality_report,
        bin_dir=bin_dir,
        check=check,
        dry_run=dry_run,
    )
    return {"recommendation": recommendation, "build": build}


def preprocess_and_build_compressed(
    input_stl: PathLike,
    cleaned_stl: PathLike,
    output_sdfbin: PathLike,
    *,
    target_error: float = 1e-3,
    memory_mb: float = 512.0,
    mesh_report: Optional[PathLike] = None,
    cleanup_report: Optional[PathLike] = None,
    recommendation_report: Optional[PathLike] = None,
    build_report: Optional[PathLike] = None,
    bin_dir: Optional[PathLike] = None,
    check: bool = True,
    dry_run: bool = False,
) -> Dict[str, object]:
    """Run mesh check, cleanup, recommendation, and explicit compressed build."""

    mesh = tools.mesh_check(
        input_stl,
        readiness=True,
        out=mesh_report,
        bin_dir=bin_dir,
        check=check,
        dry_run=dry_run,
    )
    cleanup = tools.mesh_clean(
        input_stl,
        cleaned_stl,
        report=cleanup_report,
        bin_dir=bin_dir,
        check=check,
        dry_run=dry_run,
    )
    recommendation = tools.recommend_build(
        cleaned_stl,
        target_error=target_error,
        memory_mb=memory_mb,
        out=recommendation_report,
        bin_dir=bin_dir,
        check=check,
        dry_run=dry_run,
    )
    build = tools.build_compressed_sdf(
        cleaned_stl,
        output_sdfbin,
        target_error=target_error,
        report=build_report,
        bin_dir=bin_dir,
        check=check,
        dry_run=dry_run,
    )
    return {"mesh_check": mesh, "mesh_clean": cleanup, "recommendation": recommendation, "build": build}

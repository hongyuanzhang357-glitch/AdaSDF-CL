"""Tool discovery configuration for the AdaSDF-CL Python CLI wrapper."""

from __future__ import annotations

import os
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional

from .exceptions import AdaSDFToolNotFound


SUPPORTED_TOOLS = {
    "adasdf_mesh_check",
    "adasdf_mesh_clean",
    "adasdf_recommend_build",
    "adasdf_build_dense_sdf",
    "adasdf_build_adaptive_sdf",
    "adasdf_compress_adaptive_sdf",
    "adasdf_build_compressed_sdf",
    "adasdf_info",
    "adasdf_query",
    "adasdf_collide",
    "adasdf_expansion_quality",
    "adasdf_benchmark_batch_query",
    "adasdf_write_manifest",
    "adasdf_validate_report",
    "adasdf_collect_run_summary",
    "adasdf_sparse_query",
    "adasdf_sparse_collide",
    "adasdf_contact_candidates",
    "adasdf_benchmark_sparse_query",
    "adasdf_capabilities",
    "adasdf_select_active_blocks",
    "adasdf_active_block_query",
    "adasdf_benchmark_block_cache",
    "adasdf_cuda_active_block_query",
    "adasdf_benchmark_cuda_block_cache",
    "adasdf_world_broadphase",
    "adasdf_world_sparse_collide",
    "adasdf_world_solver_contacts",
    "adasdf_benchmark_collision_world",
}


@dataclass
class AdaSDFConfig:
    bin_dir: Optional[Path] = None
    timeout_seconds: Optional[float] = None
    verbose: bool = False


def default_config() -> AdaSDFConfig:
    """Return configuration based on process environment defaults."""

    env_bin = os.environ.get("ADASDF_BIN") or os.environ.get("ADASDF_CL_BIN")
    return AdaSDFConfig(bin_dir=Path(env_bin) if env_bin else None)


def executable_names(name: str) -> list[str]:
    """Return platform-aware executable name candidates."""

    names = [name]
    if sys.platform.startswith("win") and not name.lower().endswith(".exe"):
        names.insert(0, f"{name}.exe")
    return names


def _candidate_dirs(config: Optional[AdaSDFConfig]) -> Iterable[Path]:
    if config is not None and config.bin_dir is not None:
        yield Path(config.bin_dir)
    for env_name in ("ADASDF_BIN", "ADASDF_CL_BIN"):
        env_value = os.environ.get(env_name)
        if env_value:
            yield Path(env_value)


def find_tool(name: str, config: Optional[AdaSDFConfig] = None) -> Path:
    """Find an installed AdaSDF-CL tool.

    Search order:
    1. explicit config.bin_dir;
    2. ADASDF_BIN;
    3. ADASDF_CL_BIN;
    4. PATH via shutil.which.
    """

    if name not in SUPPORTED_TOOLS:
        # Unknown tools may still be useful for forward compatibility, so do
        # not fail solely on the name. The final error remains explicit.
        pass

    for directory in _candidate_dirs(config):
        for executable in executable_names(name):
            candidate = directory / executable
            if candidate.is_file():
                return candidate

    for executable in executable_names(name):
        found = shutil.which(executable)
        if found:
            return Path(found)

    searched = [str(path) for path in _candidate_dirs(config)]
    searched.append("PATH")
    raise AdaSDFToolNotFound(
        f"Could not find AdaSDF-CL tool '{name}'. Searched: "
        + ", ".join(searched)
        + ". Set ADASDF_BIN, ADASDF_CL_BIN, or pass bin_dir."
    )


def preview_tool(name: str, bin_dir: Optional[Path] = None) -> str:
    """Return a command preview path without requiring the tool to exist."""

    executable = executable_names(name)[0]
    if bin_dir is not None:
        return str(Path(bin_dir) / executable)
    return executable

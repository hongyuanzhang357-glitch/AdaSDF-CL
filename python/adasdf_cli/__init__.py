"""Lightweight pure-Python CLI wrapper for AdaSDF-CL tools."""

from __future__ import annotations

from .config import AdaSDFConfig, default_config, find_tool
from .exceptions import AdaSDFCommandError, AdaSDFError, AdaSDFParseWarning, AdaSDFToolNotFound
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
from .tools import (
    benchmark_batch_query,
    build_adaptive_sdf,
    build_compressed_sdf,
    build_dense_sdf,
    capabilities,
    collide,
    compress_adaptive_sdf,
    expansion_quality,
    info,
    mesh_check,
    mesh_clean,
    query,
    recommend_build,
)
from .workflows import preprocess_and_build_compressed, recommend_then_build_compressed

__version__ = "1.8.1-alpha"

__all__ = [
    "AdaSDFConfig",
    "AdaSDFCommandError",
    "AdaSDFError",
    "AdaSDFParseWarning",
    "AdaSDFToolNotFound",
    "BenchmarkResult",
    "BuildResult",
    "CollisionResult",
    "CommandResult",
    "InfoResult",
    "MeshCheckResult",
    "QueryResult",
    "RecommendationResult",
    "benchmark_batch_query",
    "build_adaptive_sdf",
    "build_compressed_sdf",
    "build_dense_sdf",
    "capabilities",
    "collide",
    "compress_adaptive_sdf",
    "default_config",
    "expansion_quality",
    "find_tool",
    "info",
    "mesh_check",
    "mesh_clean",
    "preprocess_and_build_compressed",
    "query",
    "recommend_build",
    "recommend_then_build_compressed",
]

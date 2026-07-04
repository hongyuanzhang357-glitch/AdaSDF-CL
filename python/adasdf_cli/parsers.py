"""Best-effort parsers for AdaSDF-CL CLI stdout."""

from __future__ import annotations

import re
from typing import Dict, Optional, Tuple


_FLOAT = r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?"


def _search(pattern: str, stdout: str, flags: int = re.IGNORECASE | re.MULTILINE) -> Optional[re.Match]:
    return re.search(pattern, stdout or "", flags)


def parse_recommended_command(stdout: str) -> Optional[str]:
    match = _search(r"^CLI command:\s*(.+)$", stdout)
    if match:
        return match.group(1).strip()
    match = _search(r"^Command:\s*(.+)$", stdout)
    if match:
        return match.group(1).strip()
    for line in (stdout or "").splitlines():
        line = line.strip()
        if line.startswith("adasdf_build_"):
            return line
    return None


def parse_recommended_path(stdout: str) -> Optional[str]:
    match = _search(r"^Recommended path:\s*(.+)$", stdout)
    return match.group(1).strip() if match else None


def parse_query_phi(stdout: str) -> Optional[float]:
    for pattern in (
        rf"^Signed distance:\s*({_FLOAT})\s*$",
        rf"^Phi:\s*({_FLOAT})\s*$",
        rf"^phi:\s*({_FLOAT})\s*$",
    ):
        match = _search(pattern, stdout)
        if match:
            try:
                return float(match.group(1))
            except ValueError:
                return None
    return None


def parse_query_normal(stdout: str) -> Optional[Tuple[float, float, float]]:
    match = _search(rf"^Normal:\s*({_FLOAT})\s+({_FLOAT})\s+({_FLOAT})\s*$", stdout)
    if not match:
        match = _search(rf"^normal:\s*({_FLOAT})\s+({_FLOAT})\s+({_FLOAT})\s*$", stdout)
    if not match:
        return None
    try:
        return (float(match.group(1)), float(match.group(2)), float(match.group(3)))
    except ValueError:
        return None


def parse_collision_colliding(stdout: str) -> Optional[bool]:
    match = _search(r"^Colliding:\s*(true|false|yes|no|1|0)\s*$", stdout)
    if not match:
        return None
    return match.group(1).strip().lower() in {"true", "yes", "1"}


def parse_contact_count(stdout: str) -> Optional[int]:
    for pattern in (
        r"^Contact count:\s*(\d+)\s*$",
        r"^Returned contacts:\s*(\d+)\s*$",
        r"^Solver contacts:\s*(\d+)\s*$",
        r"^Output solver contacts:\s*(\d+)\s*$",
    ):
        match = _search(pattern, stdout)
        if match:
            try:
                return int(match.group(1))
            except ValueError:
                return None
    return None


def parse_minimum_distance(stdout: str) -> Optional[float]:
    match = _search(rf"^Minimum distance:\s*({_FLOAT})\s*$", stdout)
    if not match:
        return None
    try:
        return float(match.group(1))
    except ValueError:
        return None


def parse_info_format(stdout: str) -> Optional[str]:
    match = _search(r"^Format:\s*(.+)$", stdout)
    if not match:
        match = _search(r"^format:\s*(.+)$", stdout)
    return match.group(1).strip() if match else None


def parse_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for line in (stdout or "").splitlines():
        if "|" not in line:
            continue
        cells = [cell.strip() for cell in line.split("|")]
        if len(cells) < 3 or cells[0].lower() == "backend":
            continue
        if cells[0] and cells[-1]:
            metrics["backend"] = cells[0]
            metrics["status"] = cells[-1]
            if len(cells) > 4:
                metrics["points"] = cells[4]
            if len(cells) > 8:
                metrics["ns_per_query"] = cells[8]
            return metrics
    return metrics


def parse_field_int(stdout: str, label: str) -> Optional[int]:
    match = _search(rf"^{re.escape(label)}:\s*(\d+)\s*$", stdout)
    if not match:
        return None
    try:
        return int(match.group(1))
    except ValueError:
        return None


def parse_field_float(stdout: str, label: str) -> Optional[float]:
    match = _search(rf"^{re.escape(label)}:\s*({_FLOAT})\s*$", stdout)
    if not match:
        return None
    try:
        return float(match.group(1))
    except ValueError:
        return None


def parse_field_bool(stdout: str, label: str) -> Optional[bool]:
    match = _search(rf"^{re.escape(label)}:\s*(true|false|yes|no|1|0)\s*$", stdout)
    if not match:
        return None
    return match.group(1).strip().lower() in {"true", "yes", "1"}


def parse_speedup(stdout: str) -> Optional[float]:
    return parse_field_float(stdout, "Speedup vs brute reference")


def parse_bvh_build_time_ms(stdout: str) -> Optional[float]:
    return parse_field_float(stdout, "BVH build time ms")


def parse_sampling_time_ms(stdout: str) -> Optional[float]:
    return parse_field_float(stdout, "Sampling time ms")


def parse_threads(stdout: str) -> Optional[int]:
    value = parse_field_int(stdout, "Threads used")
    if value is not None:
        return value
    return parse_field_int(stdout, "Threads")


def parse_solver_contact_count(stdout: str) -> Optional[int]:
    return parse_contact_count(stdout)


def parse_patch_count(stdout: str) -> Optional[int]:
    value = parse_field_int(stdout, "Patches")
    if value is not None:
        return value
    return parse_field_int(stdout, "patch_count")


def parse_raw_candidate_count(stdout: str) -> Optional[int]:
    value = parse_field_int(stdout, "Raw candidates")
    if value is not None:
        return value
    return parse_field_int(stdout, "raw_candidate_count")


def parse_reduction_ratio(stdout: str) -> Optional[float]:
    return parse_field_float(stdout, "candidate_reduction_ratio")


def parse_avg_reduction_ms(stdout: str) -> Optional[float]:
    return parse_field_float(stdout, "avg_reduction_ms")


def parse_build_acceleration_stats(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for label, key in (
        ("Builder", "builder"),
        ("Acceleration", "acceleration"),
        ("Used BVH", "used_bvh"),
        ("Sample count", "sample_count"),
        ("BVH nodes", "bvh_nodes"),
        ("BVH leaves", "bvh_leaves"),
        ("Total build time ms", "total_build_time_ms"),
        ("Build time ms", "build_time_ms"),
        ("Brute reference time ms", "brute_reference_time_ms"),
    ):
        match = _search(rf"^{re.escape(label)}:\s*(.+)$", stdout)
        if match:
            metrics[key] = match.group(1).strip()
    threads = parse_threads(stdout)
    if threads is not None:
        metrics["threads"] = threads
    bvh_ms = parse_bvh_build_time_ms(stdout)
    if bvh_ms is not None:
        metrics["bvh_build_time_ms"] = bvh_ms
    sampling_ms = parse_sampling_time_ms(stdout)
    if sampling_ms is not None:
        metrics["sampling_time_ms"] = sampling_ms
    speedup = parse_speedup(stdout)
    if speedup is not None:
        metrics["speedup_vs_bruteforce"] = speedup
    for line in (stdout or "").splitlines():
        if line.startswith("builder,acceleration,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics


def parse_sparse_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for line in (stdout or "").splitlines():
        if line.startswith("Sparse benchmark mode:"):
            metrics["mode"] = line.split(":", 1)[1].strip()
        elif line.startswith("Average ns per sample:"):
            metrics["avg_ns_per_sample"] = line.split(":", 1)[1].strip()
    return metrics


def parse_contact_reduction_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for label, key in (
        ("sample_count", "sample_count"),
        ("raw_candidate_count", "raw_candidate_count"),
        ("patch_count", "patch_count"),
        ("solver_contact_count", "solver_contact_count"),
        ("candidate_reduction_ratio", "candidate_reduction_ratio"),
        ("avg_query_ms", "avg_query_ms"),
        ("avg_reduction_ms", "avg_reduction_ms"),
        ("avg_total_ms", "avg_total_ms"),
        ("max_contacts", "max_contacts"),
        ("patch_radius", "patch_radius"),
        ("repeat", "repeat"),
        ("warmup", "warmup"),
    ):
        match = _search(rf"^{re.escape(label)}:\s*(.+)$", stdout)
        if match:
            metrics[key] = match.group(1).strip()
    for line in (stdout or "").splitlines():
        if line.startswith("sample_count,raw_candidate_count,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics


def parse_block_cache_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for line in (stdout or "").splitlines():
        if line.startswith("Active block cache benchmark mode:"):
            metrics["mode"] = line.split(":", 1)[1].strip()
        elif line.startswith("Average ns per sample:"):
            metrics["avg_ns_per_sample"] = line.split(":", 1)[1].strip()
        elif line.startswith("sample_count,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics


def parse_cuda_block_cache_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for line in (stdout or "").splitlines():
        if line.startswith("CUDA block cache benchmark mode:"):
            metrics["mode"] = line.split(":", 1)[1].strip()
        elif line.startswith("Average ns per sample:"):
            metrics["avg_ns_per_sample"] = line.split(":", 1)[1].strip()
        elif line.startswith("CUDA total avg ms:"):
            metrics["cuda_total_avg_ms"] = line.split(":", 1)[1].strip()
        elif line.startswith("CUDA kernel avg ms:"):
            metrics["cuda_kernel_avg_ms"] = line.split(":", 1)[1].strip()
        elif line.startswith("CPU active avg ms:"):
            metrics["cpu_active_avg_ms"] = line.split(":", 1)[1].strip()
        elif line.startswith("Direct sparse avg ms:"):
            metrics["direct_sparse_avg_ms"] = line.split(":", 1)[1].strip()
        elif line.startswith("sample_count,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics


def parse_collision_world_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for label, key in (
        ("mode", "mode"),
        ("broadphase_pairs", "broadphase_pairs"),
        ("queried_pairs", "queried_pairs"),
        ("violations", "violations"),
        ("solver_contacts", "solver_contacts"),
        ("avg_total_ms", "avg_total_ms"),
        ("repeat", "repeat"),
        ("warmup", "warmup"),
    ):
        match = _search(rf"^{re.escape(label)}:\s*(.+)$", stdout, flags=re.MULTILINE)
        if match:
            metrics[key] = match.group(1).strip()
    for line in (stdout or "").splitlines():
        if line.startswith("mode,broadphase_pairs,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics

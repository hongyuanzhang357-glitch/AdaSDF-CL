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


def parse_strict_report_valid(stdout: str) -> Optional[bool]:
    if _search(r"^Strict report valid:\s*.+$", stdout):
        return True
    if _search(r"^Strict report invalid:\s*.+$", stdout):
        return False
    return None


def parse_strict_report_path(stdout: str) -> Optional[str]:
    match = _search(r"^Strict report valid:\s*(.+)$", stdout)
    return match.group(1).strip() if match else None


def parse_run_summary_csv_path(stdout: str) -> Optional[str]:
    match = _search(r"^Run summary CSV:\s*(.+)$", stdout)
    return match.group(1).strip() if match else None


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


def parse_hierarchical_sampling_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for label, key in (
        ("Exact build time ms", "exact_build_time_ms"),
        ("Hierarchical build time ms", "hierarchical_build_time_ms"),
        ("Speedup", "speedup"),
        ("Max abs error", "max_abs_error"),
        ("Mean abs error", "mean_abs_error"),
        ("RMS error", "rms_error"),
        ("P95 error", "p95_error"),
        ("Sign mismatches", "sign_mismatch_count"),
        ("Near-surface sign mismatches", "near_surface_sign_mismatch_count"),
        ("Exact sample count", "exact_sample_count"),
        ("Hierarchical exact sample count", "hierarchical_exact_sample_count"),
        ("Predicted sample count", "predicted_sample_count"),
        ("Fallback block count", "fallback_block_count"),
        ("Quality gate passed", "quality_gate_passed"),
        ("Effective speedup claim allowed", "effective_speedup_claim_allowed"),
        ("Far-field quality check", "far_field_quality_check"),
        ("Far-field sign policy", "far_field_sign_policy"),
        ("Near-surface mode", "near_surface_mode"),
        ("Total blocks", "total_block_count"),
        ("Near-surface blocks", "near_surface_block_count"),
        ("Transition blocks", "transition_block_count"),
        ("Far-field blocks", "far_field_block_count"),
        ("Exact BVH samples", "exact_bvh_sample_count"),
        ("Coarse samples", "coarse_sample_count"),
        ("Reused coarse samples", "reused_coarse_sample_count"),
        ("Quality check samples", "quality_check_sample_count"),
        ("Near-surface banded blocks", "near_surface_banded_block_count"),
        ("Near-surface local exact nodes", "near_surface_local_exact_node_count"),
        ("Near-surface predicted nodes", "near_surface_predicted_node_count"),
        ("Local fallback nodes", "local_fallback_node_count"),
        (
            "Near-surface local prediction acceptance rate",
            "near_surface_local_prediction_acceptance_rate",
        ),
        ("Fallback rate", "fallback_rate"),
        ("Prediction acceptance rate", "prediction_acceptance_rate"),
        ("Classification time ms", "classification_time_ms"),
        ("Prediction time ms", "prediction_time_ms"),
        ("Quality check time ms", "quality_check_time_ms"),
        ("Fallback exact time ms", "fallback_exact_time_ms"),
    ):
        match = _search(rf"^{re.escape(label)}:\s*(.+)$", stdout)
        if match:
            metrics[key] = match.group(1).strip()
    for line in (stdout or "").splitlines():
        if line.startswith("exact_build_time_ms,hierarchical_build_time_ms,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics


def parse_contact_band_sampling_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for label, key in (
        ("Case id", "case_id"),
        ("Timing mode", "timing_mode"),
        ("Include audit in wall time", "include_audit_in_wall_time"),
        ("Include marker in speedup", "include_marker_in_speedup"),
        ("Exclude audit from speedup", "exclude_audit_from_speedup"),
        ("Exclude marker from speedup", "exclude_marker_from_speedup"),
        ("Exact reference time ms", "exact_reference_time_ms"),
        ("Contact-band time ms", "contact_band_time_ms"),
        ("Speedup", "speedup"),
        ("Exact reference wall time ms", "exact_reference_wall_time_ms"),
        ("Contact-band wall time ms", "contact_band_wall_time_ms"),
        ("Speedup end-to-end", "speedup_end_to_end"),
        ("Exact reference core build time ms", "exact_reference_core_build_time_ms"),
        ("Contact-band core build time ms", "contact_band_core_build_time_ms"),
        ("Speedup core build", "speedup_core_build"),
        ("Effective speedup including marker", "effective_speedup_including_marker"),
        ("Effective speedup excluding marker", "effective_speedup_excluding_marker"),
        ("Speedup excluding audit", "speedup_excluding_audit"),
        ("Speedup excluding diagnostics", "speedup_excluding_diagnostics"),
        ("Speedup excluding marker", "speedup_excluding_marker"),
        ("Marker mode", "marker_mode"),
        ("Contact-band blocks", "contact_band_block_count"),
        ("Far-field blocks", "far_field_block_count"),
        ("Exact node ratio", "exact_node_ratio"),
        ("Candidate cells", "candidate_cell_count"),
        ("Candidate triangles", "candidate_triangle_count"),
        ("Refined candidates", "refined_candidate_count"),
        ("Rejected candidates", "rejected_candidate_count"),
        ("Accepted contact cells", "accepted_contact_cell_count"),
        ("Marker false-positive proxy", "marker_false_positive_proxy"),
        ("Marker time ms", "marker_time_ms"),
        ("Contact-band marker time ms", "contact_band_marker_time_ms"),
        ("Contact-band sampling time ms", "contact_band_sampling_time_ms"),
        ("Contact-band interpolation time ms", "contact_band_interpolation_time_ms"),
        ("Contact-band audit time ms", "contact_band_audit_time_ms"),
        ("Contact-band report time ms", "contact_band_report_time_ms"),
        ("Contact-band diagnostics time ms", "contact_band_diagnostics_time_ms"),
        ("Marker time fraction", "marker_time_fraction"),
        ("Marker time fraction of wall", "marker_time_fraction_of_wall"),
        ("Audit time fraction of wall", "audit_time_fraction_of_wall"),
        ("Diagnostics time fraction of wall", "diagnostics_time_fraction_of_wall"),
        ("Triangle BVH query time ms", "triangle_bvh_query_time_ms"),
        ("Box-triangle distance time ms", "box_triangle_distance_time_ms"),
        ("Marker refinement time ms", "marker_refinement_time_ms"),
        ("Contact-band sign mismatches", "contact_band_sign_mismatch_count"),
        ("Near-surface sign mismatches", "near_surface_sign_mismatch_count"),
        ("P95 normal angle error deg", "p95_normal_angle_error_deg"),
        ("Coverage passed", "coverage_passed"),
        ("Coverage check count", "contact_band_coverage_check_count"),
        ("Missed contact-band points", "missed_contact_band_point_count"),
        ("Missed contact-band cells", "missed_contact_band_cell_count"),
        ("Contact-band quality passed", "contact_band_quality_passed"),
        ("Effective speedup claim allowed", "effective_speedup_claim_allowed"),
        ("Performance claim allowed", "performance_claim_allowed"),
    ):
        match = _search(rf"^{re.escape(label)}:\s*(.+)$", stdout)
        if match:
            metrics[key] = match.group(1).strip()
    for line in (stdout or "").splitlines():
        if line.startswith("case_id,exact_reference_time_ms,contact_band_time_ms,"):
            metrics["csv_header"] = line
        elif metrics.get("csv_header") and "csv_values" not in metrics:
            metrics["csv_values"] = line
    return metrics


def parse_exact_hotpath_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for label, key in (
        ("Case id", "case_id"),
        ("Block count", "block_count"),
        ("Sample count", "sample_count"),
        ("Reference exact time ms", "reference_exact_time_ms"),
        ("Hierarchical exact time ms", "hierarchical_exact_time_ms"),
        (
            "Optimized hierarchical exact time ms",
            "optimized_hierarchical_exact_time_ms",
        ),
        ("Reference ns/sample", "reference_ns_per_sample"),
        ("Hierarchical ns/sample", "hierarchical_ns_per_sample"),
        (
            "Optimized hierarchical ns/sample",
            "optimized_hierarchical_ns_per_sample",
        ),
        ("Hierarchical overhead ratio", "hierarchical_overhead_ratio"),
        ("Optimized overhead ratio", "optimized_overhead_ratio"),
        ("Distance query count", "distance_query_count"),
        ("Sign query count", "sign_query_count"),
        ("Allocation count estimate", "allocation_count_estimate"),
        ("Diagnostics enabled", "diagnostics_enabled"),
        ("Counters enabled", "counters_enabled"),
    ):
        match = _search(rf"^{re.escape(label)}:\s*(.+)$", stdout)
        if match:
            metrics[key] = match.group(1).strip()
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

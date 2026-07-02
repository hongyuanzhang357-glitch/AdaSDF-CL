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
    for pattern in (r"^Contact count:\s*(\d+)\s*$", r"^Returned contacts:\s*(\d+)\s*$"):
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


def parse_sparse_benchmark_metrics(stdout: str) -> Dict[str, object]:
    metrics: Dict[str, object] = {}
    for line in (stdout or "").splitlines():
        if line.startswith("Sparse benchmark mode:"):
            metrics["mode"] = line.split(":", 1)[1].strip()
        elif line.startswith("Average ns per sample:"):
            metrics["avg_ns_per_sample"] = line.split(":", 1)[1].strip()
    return metrics

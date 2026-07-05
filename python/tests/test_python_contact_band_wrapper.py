import unittest

import adasdf_cli as adasdf
from adasdf_cli.parsers import parse_contact_band_sampling_benchmark_metrics


class ContactBandWrapperTests(unittest.TestCase):
    def test_contact_band_benchmark_dry_run_command(self):
        result = adasdf.benchmark_contact_band_sampling(
            "model.stl",
            max_level=5,
            block_resolution=8,
            target_error=1e-3,
            contact_band_width=5e-4,
            contact_band_layers=1,
            halo_exact_layers=1,
            contact_band_marker="distance-aware",
            marker_cell_size_factor=0.5,
            marker_safety_factor=1.0,
            local_halo_only=True,
            far_field_resolution=3,
            far_field_mode="coarse-interpolate",
            reuse_far_field_sign=True,
            normal_audit=True,
            coverage_audit=True,
            coverage_samples_per_axis=3,
            marker_cost_audit=True,
            save_marker_debug_csv="marker_debug.csv",
            timing_mode="end-to-end",
            include_audit_in_wall_time=True,
            include_marker_in_speedup=True,
            threads=2,
            csv="contact.csv",
            report="contact.md",
            case_id="contact_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_contact_band_sampling", stdout)
        self.assertIn("--contact-band-marker", stdout)
        self.assertIn("distance-aware", stdout)
        self.assertIn("--local-halo-only", stdout)
        self.assertIn("--coverage-audit", stdout)
        self.assertIn("--marker-cost-audit", stdout)
        self.assertIn("--save-marker-debug-csv", stdout)
        self.assertIn("--timing-mode", stdout)
        self.assertIn("end-to-end", stdout)
        self.assertIn("--include-audit-in-wall-time", stdout)
        self.assertIn("--include-marker-in-speedup", stdout)

    def test_contact_band_metrics_parser(self):
        stdout = "\n".join(
            [
                "AdaSDF-CL contact-band sampling benchmark",
                "Case id: hardening_case",
                "Timing mode: end-to-end",
                "Include marker in speedup: yes",
                "Speedup: 1.25",
                "Exact reference wall time ms: 100",
                "Contact-band wall time ms: 80",
                "Speedup end-to-end: 1.25",
                "Exact reference core build time ms: 100",
                "Contact-band core build time ms: 70",
                "Speedup core build: 1.42857",
                "Marker mode: distance-aware",
                "Exact node ratio: 0.125",
                "Marker time ms: 12.5",
                "Contact-band marker time ms: 10",
                "Contact-band audit time ms: 2",
                "Contact-band diagnostics time ms: 1",
                "Marker time fraction: 0.25",
                "Marker time fraction of wall: 0.125",
                "Audit time fraction of wall: 0.025",
                "Diagnostics time fraction of wall: 0.0125",
                "Speedup excluding marker: 1.42857",
                "P95 normal angle error deg: 0.5",
                "Coverage passed: yes",
                "Coverage check count: 27",
                "Missed contact-band points: 0",
                "Contact-band quality passed: yes",
                "Effective speedup claim allowed: yes",
                "Performance claim allowed: yes",
            ]
        )
        metrics = parse_contact_band_sampling_benchmark_metrics(stdout)
        self.assertEqual(metrics["case_id"], "hardening_case")
        self.assertEqual(metrics["timing_mode"], "end-to-end")
        self.assertEqual(metrics["speedup_end_to_end"], "1.25")
        self.assertEqual(metrics["speedup_core_build"], "1.42857")
        self.assertEqual(metrics["speedup_excluding_marker"], "1.42857")
        self.assertEqual(metrics["marker_mode"], "distance-aware")
        self.assertEqual(metrics["coverage_passed"], "yes")
        self.assertEqual(metrics["missed_contact_band_point_count"], "0")
        self.assertEqual(metrics["performance_claim_allowed"], "yes")


if __name__ == "__main__":
    unittest.main()

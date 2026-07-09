import unittest

import adasdf_cli as adasdf
from adasdf_cli.parsers import parse_build_cache_benchmark_metrics


class BuildCacheWrapperTests(unittest.TestCase):
    def test_build_compressed_cache_options_dry_run(self):
        result = adasdf.build_compressed_sdf(
            "model.stl",
            "model.sdfbin",
            sampling="contact-band",
            sample_cache="block",
            corner_cache="block",
            sign_cache="on",
            distance_cache="on",
            marker_cache="on",
            cache_max_entries=1024,
            cache_quantization_epsilon=1e-10,
            report_cache_stats=True,
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_build_compressed_sdf", stdout)
        self.assertIn("--sample-cache", stdout)
        self.assertIn("block", stdout)
        self.assertIn("--corner-cache", stdout)
        self.assertIn("--sign-cache", stdout)
        self.assertIn("--distance-cache", stdout)
        self.assertIn("--marker-cache", stdout)
        self.assertIn("--cache-max-entries", stdout)
        self.assertIn("--cache-quantization-epsilon", stdout)
        self.assertIn("--report-cache-stats", stdout)

    def test_benchmark_build_cache_dry_run(self):
        result = adasdf.benchmark_build_cache(
            "model.stl",
            builder="compressed",
            max_level=4,
            block_resolution=8,
            target_error=1e-3,
            max_rank=8,
            sampling="contact-band",
            contact_band_width=5e-4,
            contact_band_marker="distance-aware",
            marker_cell_size_factor=0.5,
            marker_safety_factor=1.0,
            local_halo_only=True,
            cache_modes=["off", "block", "global"],
            distance_backend="bvh",
            threads=2,
            csv="build_cache.csv",
            report="build_cache.md",
            case_id="cache_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_build_cache", stdout)
        self.assertIn("--cache-modes", stdout)
        self.assertIn("off,block,global", stdout)
        self.assertIn("--distance-backend", stdout)
        self.assertIn("--case-id", stdout)

    def test_parse_build_cache_metrics(self):
        stdout = "\n".join(
            [
                "AdaSDF-CL build cache benchmark",
                "Case id: build_cache_case",
                "Builder: compressed",
                "Reference cache mode: off",
                (
                    "case_id,builder,sampling,cache_mode,no_cache_time_ms,"
                    "cache_time_ms,speedup_vs_no_cache,"
                    "num_distance_queries_no_cache,num_distance_queries_cache,"
                    "num_sign_queries_no_cache,num_sign_queries_cache,"
                    "num_triangle_tests_no_cache,num_triangle_tests_cache,"
                    "sample_cache_hit_rate,corner_cache_hit_rate,"
                    "sign_cache_hit_rate,distance_queries_saved,"
                    "sign_queries_saved,duplicate_point_count,"
                    "marker_cache_hit_rate,max_abs_phi_diff,rms_phi_diff,"
                    "p95_phi_diff,sign_mismatch_count,normal_p95_error_deg,"
                    "quality_passed,performance_claim_allowed"
                ),
                (
                    "build_cache_case,compressed,contact-band,block,10,8,"
                    "1.25,100,80,100,80,500,400,0.2,0.1,0.2,20,20,"
                    "4,0.5,0,0,0,0,0,true,true"
                ),
            ]
        )
        metrics = parse_build_cache_benchmark_metrics(stdout)
        self.assertEqual(metrics["case_id"], "build_cache_case")
        self.assertEqual(metrics["builder"], "compressed")
        self.assertEqual(metrics["cache_mode"], "block")
        self.assertEqual(metrics["speedup_vs_no_cache"], "1.25")
        self.assertEqual(metrics["distance_queries_saved"], "20")
        self.assertEqual(metrics["quality_passed"], "true")
        self.assertEqual(metrics["performance_claim_allowed"], "true")


if __name__ == "__main__":
    unittest.main()

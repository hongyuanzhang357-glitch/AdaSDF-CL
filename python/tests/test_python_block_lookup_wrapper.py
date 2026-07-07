import unittest

import adasdf_cli as adasdf
from adasdf_cli.parsers import parse_block_lookup_benchmark_metrics


class BlockLookupWrapperTests(unittest.TestCase):
    def test_benchmark_block_lookup_dry_run(self):
        result = adasdf.benchmark_block_lookup(
            "model.sdfbin",
            "samples.csv",
            lookup=["linear", "hash", "morton"],
            cache_lookup=["linear", "hash", "spatial-hash"],
            repeat=3,
            csv="lookup.csv",
            report="lookup.md",
            case_id="python_lookup",
            dry_run=True,
        )
        self.assertIn("adasdf_benchmark_block_lookup", result.command_result.stdout)
        self.assertIn("--case-id", result.command_result.stdout)
        self.assertIn("python_lookup", result.command_result.stdout)

    def test_parse_block_lookup_metrics(self):
        stdout = "\n".join(
            [
                "case_id,model_type,sample_count,block_count,active_block_count,lookup_mode,cache_lookup_mode,index_build_time_ms,cache_map_build_time_ms,linear_query_time_ms,hash_query_time_ms,morton_query_time_ms,spatial_hash_query_time_ms,speedup_vs_linear,lookup_result_mismatch_count,phi_mismatch_count,max_abs_phi_diff,rms_phi_diff,p95_phi_diff,cache_hit_count,cache_miss_count,cache_hit_rate,linear_fallback_count,missed_lookup_count,quality_passed,performance_claim_allowed",
                "case,compressed,6,8,8,hash,spatial-hash,0.1,0.2,1.0,0.5,0.7,0.4,2.0,0,0,0,0,0,6,0,1,0,0,true,true",
                "Block lookup benchmark",
                "Case id: case",
                "Linear query time ms: 1.0",
                "Status: ok",
            ]
        )
        metrics = parse_block_lookup_benchmark_metrics(stdout)
        self.assertEqual(metrics["speedup_vs_linear"], "2.0")
        self.assertEqual(metrics["lookup_result_mismatch_count"], "0")
        self.assertEqual(metrics["max_abs_phi_diff"], "0")
        self.assertEqual(metrics["cache_hit_rate"], "1")
        self.assertEqual(metrics["performance_claim_allowed"], "true")


if __name__ == "__main__":
    unittest.main()

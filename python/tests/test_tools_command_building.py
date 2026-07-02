import unittest

import adasdf_cli as adasdf


class ToolCommandBuildingTests(unittest.TestCase):
    def test_mesh_check_dry_run_command(self):
        result = adasdf.mesh_check("model.stl", readiness=True, out="mesh.md", dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_mesh_check", stdout)
        self.assertIn("--readiness", stdout)
        self.assertIn("--out", stdout)

    def test_recommend_build_dry_run_command(self):
        result = adasdf.recommend_build("model.stl", target_error=1e-3, memory_mb=256, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_recommend_build", stdout)
        self.assertIn("--target-error", stdout)
        self.assertIn("--memory-mb", stdout)

    def test_build_compressed_sdf_dry_run_command(self):
        result = adasdf.build_compressed_sdf("model.stl", "model.sdfbin", max_level=4, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_build_compressed_sdf", stdout)
        self.assertIn("--max-level", stdout)

    def test_query_dry_run_command(self):
        result = adasdf.query("model.sdfbin", point=[0, 0, 0], dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_query", stdout)
        self.assertIn("--point", stdout)

    def test_query_rejects_unsupported_cli_options(self):
        with self.assertRaises(ValueError):
            adasdf.query("model.sdfbin", point=[0, 0, 0], backend="cuda", dry_run=True)

    def test_collide_dry_run_command(self):
        result = adasdf.collide("a.sdfbin", "b.sdfbin", max_contacts=4, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_collide", stdout)
        self.assertIn("--max-contacts", stdout)

    def test_benchmark_dry_run_command(self):
        result = adasdf.benchmark_batch_query(points=[100, 1000], dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_batch_query", stdout)
        self.assertIn("--points", stdout)

    def test_sparse_query_dry_run_command(self):
        result = adasdf.sparse_query("model.sdfbin", "samples.csv", dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_sparse_query", stdout)
        self.assertIn("--phi-only", stdout)

    def test_sparse_collide_dry_run_command(self):
        result = adasdf.sparse_collide("model.sdfbin", "samples.csv", dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_sparse_collide", stdout)
        self.assertIn("--mode", stdout)

    def test_contact_candidates_dry_run_command(self):
        result = adasdf.contact_candidates("model.sdfbin", "samples.csv", top_k=4, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_contact_candidates", stdout)
        self.assertIn("--top-k", stdout)

    def test_sparse_benchmark_dry_run_command(self):
        result = adasdf.benchmark_sparse_query("model.sdfbin", "samples.csv", repeat=2, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_sparse_query", stdout)
        self.assertIn("--repeat", stdout)

    def test_select_active_blocks_dry_run_command(self):
        result = adasdf.select_active_blocks("model.sdfbin", "samples.csv", selection_band=0.1, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_select_active_blocks", stdout)
        self.assertIn("--selection-band", stdout)

    def test_active_block_query_dry_run_command(self):
        result = adasdf.active_block_query("model.sdfbin", "samples.csv", threshold=0.1, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_active_block_query", stdout)
        self.assertIn("--phi-only", stdout)

    def test_block_cache_benchmark_dry_run_command(self):
        result = adasdf.benchmark_block_cache("model.sdfbin", "samples.csv", repeat=2, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_block_cache", stdout)
        self.assertIn("--repeat", stdout)

    def test_bin_dir_is_used_in_preview(self):
        result = adasdf.info("model.sdfbin", bin_dir="tools-bin", dry_run=True)
        self.assertIn("tools-bin", result.command_result.stdout)


if __name__ == "__main__":
    unittest.main()

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

    def test_bin_dir_is_used_in_preview(self):
        result = adasdf.info("model.sdfbin", bin_dir="tools-bin", dry_run=True)
        self.assertIn("tools-bin", result.command_result.stdout)


if __name__ == "__main__":
    unittest.main()

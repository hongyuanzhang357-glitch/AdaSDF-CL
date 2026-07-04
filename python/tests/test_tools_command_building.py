import unittest

import adasdf_cli as adasdf


class ToolCommandBuildingTests(unittest.TestCase):
    def test_mesh_check_dry_run_command(self):
        result = adasdf.mesh_check(
            "model.stl",
            readiness=True,
            out="mesh.md",
            strict_json="mesh.strict.json",
            case_id="mesh_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_mesh_check", stdout)
        self.assertIn("--readiness", stdout)
        self.assertIn("--out", stdout)
        self.assertIn("--strict-json", stdout)
        self.assertIn("--case-id", stdout)

    def test_manifest_tools_dry_run_command(self):
        manifest = adasdf.write_manifest(
            out="manifest.json",
            case_id="case001",
            tool="unit",
            input_path="in.stl",
            output_path="out.sdfbin",
            params={"resolution": 32},
            dry_run=True,
        )
        self.assertIn("adasdf_write_manifest", manifest.command_result.stdout)
        self.assertIn("--param", manifest.command_result.stdout)

        validation = adasdf.validate_report("manifest.json", dry_run=True)
        self.assertIn("adasdf_validate_report", validation.command_result.stdout)

        summary = adasdf.collect_run_summary("reports.txt", "summary.csv", dry_run=True)
        self.assertIn("adasdf_collect_run_summary", summary.command_result.stdout)

    def test_recommend_build_dry_run_command(self):
        result = adasdf.recommend_build("model.stl", target_error=1e-3, memory_mb=256, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_recommend_build", stdout)
        self.assertIn("--target-error", stdout)
        self.assertIn("--memory-mb", stdout)

    def test_build_compressed_sdf_dry_run_command(self):
        result = adasdf.build_compressed_sdf(
            "model.stl",
            "model.sdfbin",
            max_level=4,
            sampling="hierarchical",
            coarse_resolution=2,
            quality_check_samples=2,
            far_field_interpolation=False,
            transition_prediction=True,
            near_surface_exact=True,
            quality_guard=True,
            target_sampling_error=0.01,
            strict_json="build.strict.json",
            case_id="build_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_build_compressed_sdf", stdout)
        self.assertIn("--max-level", stdout)
        self.assertIn("--sampling", stdout)
        self.assertIn("hierarchical", stdout)
        self.assertIn("--no-far-field-interpolation", stdout)
        self.assertIn("--target-sampling-error", stdout)
        self.assertIn("--strict-json", stdout)

    def test_build_adaptive_hierarchical_sampling_dry_run_command(self):
        result = adasdf.build_adaptive_sdf(
            "model.stl",
            "model.sdfbin",
            sampling="hierarchical",
            coarse_resolution=3,
            quality_check_samples=2,
            quality_guard=False,
            target_sampling_error=0.02,
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_build_adaptive_sdf", stdout)
        self.assertIn("--sampling", stdout)
        self.assertIn("--coarse-resolution", stdout)
        self.assertIn("--quality-check-samples", stdout)
        self.assertIn("--no-quality-guard", stdout)

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
        result = adasdf.benchmark_batch_query(
            points=[100, 1000],
            out="benchmark.csv",
            strict_json="benchmark.strict.json",
            case_id="bench_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_batch_query", stdout)
        self.assertIn("--points", stdout)
        self.assertIn("--strict-json", stdout)

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

    def test_stabilize_contacts_dry_run_command(self):
        result = adasdf.stabilize_contacts("candidates.csv", max_contacts=4, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_stabilize_contacts", stdout)
        self.assertIn("--max-contacts", stdout)

    def test_solver_contact_candidates_dry_run_command(self):
        result = adasdf.solver_contact_candidates(
            "model.sdfbin",
            "samples.csv",
            top_k=16,
            strict_json="solver.strict.json",
            case_id="solver_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_solver_contact_candidates", stdout)
        self.assertIn("--top-k", stdout)
        self.assertIn("--strict-json", stdout)

    def test_contact_reduction_benchmark_dry_run_command(self):
        result = adasdf.benchmark_contact_reduction("model.sdfbin", "samples.csv", repeat=2, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_contact_reduction", stdout)
        self.assertIn("--repeat", stdout)

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

    def test_cuda_active_block_query_dry_run_command(self):
        result = adasdf.cuda_active_block_query("model.sdfbin", "samples.csv", threshold=0.1, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_cuda_active_block_query", stdout)
        self.assertIn("--phi-only", stdout)

    def test_cuda_block_cache_benchmark_dry_run_command(self):
        result = adasdf.benchmark_cuda_block_cache("model.sdfbin", "samples.csv", repeat=2, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_cuda_block_cache", stdout)
        self.assertIn("--repeat", stdout)

    def test_hierarchical_sampling_benchmark_dry_run_command(self):
        result = adasdf.benchmark_hierarchical_sampling(
            "model.stl",
            max_level=1,
            block_resolution=4,
            target_sampling_error=0.01,
            coarse_resolution=2,
            quality_check_samples=2,
            comparison_samples=3,
            report="hier.md",
            json="hier.json",
            csv="hier.csv",
            strict_json="hier.strict.json",
            case_id="hier_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_hierarchical_sampling", stdout)
        self.assertIn("--comparison-samples", stdout)
        self.assertIn("--strict-json", stdout)

    def test_world_broadphase_dry_run_command(self):
        result = adasdf.world_broadphase("scene.csv", aabb_margin=0.01, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_world_broadphase", stdout)
        self.assertIn("--aabb-margin", stdout)

    def test_world_sparse_collide_dry_run_command(self):
        result = adasdf.world_sparse_collide(
            "scene.csv",
            threshold=0.001,
            with_normal=True,
            strict_json="world.strict.json",
            case_id="world_case",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_world_sparse_collide", stdout)
        self.assertIn("--threshold", stdout)
        self.assertIn("--with-normal", stdout)
        self.assertIn("--strict-json", stdout)

    def test_world_solver_contacts_dry_run_command(self):
        result = adasdf.world_solver_contacts("scene.csv", top_k=16, max_contacts=4, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_world_solver_contacts", stdout)
        self.assertIn("--top-k", stdout)
        self.assertIn("--max-contacts", stdout)

    def test_collision_world_benchmark_dry_run_command(self):
        result = adasdf.benchmark_collision_world(
            "scene.csv",
            mode="contacts",
            repeat=2,
            strict_json="world_bench.strict.json",
            case_id="world_bench",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("adasdf_benchmark_collision_world", stdout)
        self.assertIn("--mode", stdout)
        self.assertIn("--repeat", stdout)
        self.assertIn("--strict-json", stdout)

    def test_bin_dir_is_used_in_preview(self):
        result = adasdf.info("model.sdfbin", bin_dir="tools-bin", dry_run=True)
        self.assertIn("tools-bin", result.command_result.stdout)


if __name__ == "__main__":
    unittest.main()

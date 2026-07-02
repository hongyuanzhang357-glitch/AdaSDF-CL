import os
import tempfile
import unittest
from pathlib import Path

import adasdf_cli as adasdf


def _smoke_available():
    bin_dir = os.environ.get("ADASDF_BIN") or os.environ.get("ADASDF_CL_BIN")
    test_stl = os.environ.get("ADASDF_TEST_STL")
    return bool(bin_dir and test_stl and Path(bin_dir).is_dir() and Path(test_stl).is_file())


def _samples_available():
    samples = os.environ.get("ADASDF_TEST_SAMPLES")
    return bool(samples and Path(samples).is_file())


@unittest.skipUnless(_smoke_available(), "ADASDF_BIN/ADASDF_TEST_STL not set; skipping real CLI smoke")
class PythonCliSmokeTests(unittest.TestCase):
    def setUp(self):
        self.bin_dir = os.environ.get("ADASDF_BIN") or os.environ.get("ADASDF_CL_BIN")
        self.test_stl = Path(os.environ["ADASDF_TEST_STL"])
        self.tmp = tempfile.TemporaryDirectory()
        self.tmp_path = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_real_cli_smoke(self):
        caps = adasdf.capabilities(bin_dir=self.bin_dir)
        self.assertIn("AdaSDF-CL version:", caps.stdout)

        mesh_report = self.tmp_path / "mesh_report.md"
        mesh = adasdf.mesh_check(self.test_stl, readiness=True, out=mesh_report, bin_dir=self.bin_dir)
        self.assertEqual(mesh.command_result.returncode, 0)
        self.assertTrue(mesh_report.exists())

        rec_report = self.tmp_path / "recommendation.md"
        rec = adasdf.recommend_build(
            self.test_stl,
            target_error=1e-3,
            memory_mb=256,
            use_case="contact",
            out=rec_report,
            bin_dir=self.bin_dir,
        )
        self.assertIsNotNone(rec.recommended_path)
        self.assertTrue(rec_report.exists())

        sdfbin = self.tmp_path / "cube_compressed.sdfbin"
        build = adasdf.build_compressed_sdf(
            self.test_stl,
            sdfbin,
            target_error=1e-3,
            max_level=2,
            block_resolution=5,
            max_rank=5,
            bin_dir=self.bin_dir,
        )
        self.assertEqual(build.command_result.returncode, 0)
        self.assertTrue(sdfbin.exists())

        info = adasdf.info(sdfbin, bin_dir=self.bin_dir)
        self.assertIsNotNone(info.format)

        query = adasdf.query(sdfbin, point=[0.5, 0.5, 0.5], bin_dir=self.bin_dir)
        self.assertIsNotNone(query.phi)

        if _samples_available():
            samples = Path(os.environ["ADASDF_TEST_SAMPLES"])
            sparse_csv = self.tmp_path / "sparse.csv"
            sparse = adasdf.sparse_query(
                sdfbin,
                samples,
                threshold=0.0,
                out=sparse_csv,
                bin_dir=self.bin_dir,
            )
            self.assertTrue(sparse_csv.exists())
            self.assertIsNotNone(sparse.colliding)

            collide = adasdf.sparse_collide(
                sdfbin,
                samples,
                threshold=0.0,
                early_exit=True,
                bin_dir=self.bin_dir,
            )
            self.assertIn(collide.command_result.returncode, (0, 10))
            self.assertIsNotNone(collide.colliding)

            candidates_csv = self.tmp_path / "candidates.csv"
            candidates = adasdf.contact_candidates(
                sdfbin,
                samples,
                top_k=4,
                threshold=1e-3,
                out=candidates_csv,
                bin_dir=self.bin_dir,
            )
            self.assertTrue(candidates_csv.exists())
            self.assertIsNotNone(candidates.candidate_count)

            benchmark_csv = self.tmp_path / "sparse_benchmark.csv"
            benchmark = adasdf.benchmark_sparse_query(
                sdfbin,
                samples,
                repeat=2,
                warmup=1,
                csv=benchmark_csv,
                bin_dir=self.bin_dir,
            )
            self.assertTrue(benchmark_csv.exists())
            self.assertIn("avg_ns_per_sample", benchmark.metrics)


if __name__ == "__main__":
    unittest.main()

import os
import tempfile
import unittest
from pathlib import Path

import adasdf_cli as adasdf


def _smoke_available():
    bin_dir = os.environ.get("ADASDF_BIN") or os.environ.get("ADASDF_CL_BIN")
    test_stl = os.environ.get("ADASDF_TEST_STL")
    return bool(bin_dir and test_stl and Path(bin_dir).is_dir() and Path(test_stl).is_file())


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


if __name__ == "__main__":
    unittest.main()

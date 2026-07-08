import unittest

import adasdf_cli as adasdf


class PythonBuildProfileTests(unittest.TestCase):
    def test_build_compressed_profile_flags_dry_run(self):
        result = adasdf.build_compressed_sdf(
            "model.stl",
            "model.sdfbin",
            profile=True,
            profile_json="build_profile.json",
            progress=True,
            progress_json="progress.jsonl",
            max_seconds=60,
            distance_backend="bvh",
            threads="auto",
            dry_run=True,
        )
        stdout = result.command_result.stdout
        self.assertIn("--profile", stdout)
        self.assertIn("--profile-json", stdout)
        self.assertIn("--progress", stdout)
        self.assertIn("--progress-json", stdout)
        self.assertIn("--max-seconds", stdout)
        self.assertIn("--distance-backend", stdout)
        self.assertIn("--threads", stdout)
        self.assertIn("auto", stdout)


if __name__ == "__main__":
    unittest.main()

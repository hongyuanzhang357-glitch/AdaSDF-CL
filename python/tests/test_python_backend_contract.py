import unittest

import adasdf_cli as adasdf
from adasdf_cli.parsers import parse_backend_json


class PythonBackendContractTests(unittest.TestCase):
    def test_parse_backend_json(self):
        parsed = parse_backend_json('{"schema_id":"adasdf.info.v1","success":true}')
        self.assertEqual(parsed["schema_id"], "adasdf.info.v1")
        self.assertTrue(parsed["success"])

    def test_info_json_dry_run(self):
        result = adasdf.info("model.sdfbin", json=True, full=True, dry_run=True)
        stdout = result.command_result.stdout
        self.assertIn("adasdf_info", stdout)
        self.assertIn("--json", stdout)
        self.assertIn("--full", stdout)

    def test_export_helpers_dry_run(self):
        self.assertIn(
            "adasdf_export_structure",
            adasdf.export_structure("model.sdfbin", dry_run=True).command_result.stdout,
        )
        self.assertIn(
            "adasdf_export_block_grid",
            adasdf.export_block_grid("model.sdfbin", dry_run=True).command_result.stdout,
        )
        self.assertIn(
            "adasdf_export_compression",
            adasdf.export_compression("model.sdfbin", dry_run=True).command_result.stdout,
        )

    def test_benchmark_json_stdout_dry_run(self):
        result = adasdf.benchmark_sparse_query(
            "model.sdfbin",
            "samples.csv",
            json=True,
            dry_run=True,
        )
        self.assertIn("--json", result.command_result.stdout)


if __name__ == "__main__":
    unittest.main()

import unittest
from pathlib import Path

import adasdf_cli as adasdf
from adasdf_cli.parsers import (
    parse_run_summary_csv_path,
    parse_strict_report_path,
    parse_strict_report_valid,
)
from adasdf_cli.results import RunSummaryResult, StrictManifestResult, StrictReportValidationResult


class StrictReportPythonTests(unittest.TestCase):
    def test_parse_validate_report_stdout(self):
        stdout = "Strict report valid: reports/build_case001.json\n"
        self.assertTrue(parse_strict_report_valid(stdout))
        self.assertEqual(parse_strict_report_path(stdout), "reports/build_case001.json")

    def test_parse_invalid_report_stdout(self):
        stdout = "Strict report invalid: missing required field schema_version\n"
        self.assertFalse(parse_strict_report_valid(stdout))
        self.assertIsNone(parse_strict_report_path(stdout))

    def test_parse_summary_stdout(self):
        stdout = "Run summary CSV: reports/summary.csv\n"
        self.assertEqual(parse_run_summary_csv_path(stdout), "reports/summary.csv")

    def test_manifest_tool_wrappers_dry_run(self):
        manifest = adasdf.write_manifest(
            out="manifest.json",
            case_id="case001",
            tool="build_compressed_sdf",
            input_path="model.stl",
            output_path="model.sdfbin",
            params={"target_error": 1e-3},
            dry_run=True,
        )
        self.assertIsInstance(manifest, StrictManifestResult)
        self.assertEqual(manifest.report_path, Path("manifest.json"))
        self.assertIn("--case-id", manifest.command_result.stdout)
        self.assertIn("--param", manifest.command_result.stdout)

        validation = adasdf.validate_report("manifest.json", dry_run=True)
        self.assertIsInstance(validation, StrictReportValidationResult)
        self.assertIsNone(validation.valid)

        summary = adasdf.collect_run_summary("reports.txt", "summary.csv", dry_run=True)
        self.assertIsInstance(summary, RunSummaryResult)
        self.assertEqual(summary.csv_path, Path("summary.csv"))

    def test_core_wrappers_accept_strict_report_options(self):
        compressed = adasdf.build_compressed_sdf(
            "model.stl",
            "model.sdfbin",
            strict_json="build_report.json",
            case_id="case001",
            dry_run=True,
        )
        self.assertIn("--strict-json", compressed.command_result.stdout)
        self.assertIn("--case-id", compressed.command_result.stdout)

        sparse = adasdf.sparse_query(
            "model.sdfbin",
            "samples.csv",
            strict_json="query_report.json",
            case_id="case001_query",
            dry_run=True,
        )
        self.assertIn("--strict-json", sparse.command_result.stdout)
        self.assertIn("--case-id", sparse.command_result.stdout)


if __name__ == "__main__":
    unittest.main()

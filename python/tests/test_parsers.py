import unittest

from adasdf_cli.parsers import (
    parse_benchmark_metrics,
    parse_collision_colliding,
    parse_contact_count,
    parse_info_format,
    parse_minimum_distance,
    parse_query_normal,
    parse_query_phi,
    parse_recommended_command,
    parse_recommended_path,
)


class ParserTests(unittest.TestCase):
    def test_parse_recommended_command(self):
        stdout = "Command:\nignored\nCLI command: adasdf_build_compressed_sdf model.stl model.sdfbin\n"
        self.assertEqual(
            parse_recommended_command(stdout),
            "adasdf_build_compressed_sdf model.stl model.sdfbin",
        )

    def test_parse_recommended_path(self):
        self.assertEqual(parse_recommended_path("Recommended path: CompressedAdaptiveBlockSDF\n"), "CompressedAdaptiveBlockSDF")

    def test_parse_phi(self):
        self.assertEqual(parse_query_phi("Signed distance: -0.125\n"), -0.125)
        self.assertEqual(parse_query_phi("phi: 1.5e-3\n"), 1.5e-3)

    def test_parse_normal(self):
        self.assertEqual(parse_query_normal("Normal: 1 0 -0.5\n"), (1.0, 0.0, -0.5))

    def test_parse_colliding(self):
        self.assertTrue(parse_collision_colliding("Colliding: true\n"))
        self.assertFalse(parse_collision_colliding("Colliding: false\n"))

    def test_parse_contact_count(self):
        self.assertEqual(parse_contact_count("Contact count: 4\n"), 4)
        self.assertEqual(parse_contact_count("Returned contacts: 2\n"), 2)

    def test_parse_minimum_distance(self):
        self.assertEqual(parse_minimum_distance("Minimum distance: -0.25\n"), -0.25)

    def test_parse_info_format(self):
        self.assertEqual(parse_info_format("Format: ADASDF_DENSE_SDFBIN_V1\n"), "ADASDF_DENSE_SDFBIN_V1")

    def test_parse_benchmark_metrics(self):
        stdout = (
            "backend | expansion | output | blocks | points | setup ms | total mean ms | kernel mean ms | ns/query | status\n"
            "cpu | none | phi | all | 1000 | 0 | 1 | NA | 1000 | ok\n"
        )
        metrics = parse_benchmark_metrics(stdout)
        self.assertEqual(metrics["backend"], "cpu")
        self.assertEqual(metrics["status"], "ok")
        self.assertEqual(metrics["points"], "1000")


if __name__ == "__main__":
    unittest.main()

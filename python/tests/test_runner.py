import sys
import unittest

from adasdf_cli.exceptions import AdaSDFCommandError
from adasdf_cli.runner import run_command


class RunnerTests(unittest.TestCase):
    def test_dry_run_returns_command_preview(self):
        result = run_command(["adasdf_info", "model.sdfbin"], dry_run=True)
        self.assertEqual(result.returncode, 0)
        self.assertIn("adasdf_info", result.stdout)
        self.assertEqual(result.stderr, "")

    def test_check_false_preserves_nonzero_result(self):
        result = run_command(
            [sys.executable, "-c", "import sys; print('bad'); sys.exit(7)"],
            check=False,
        )
        self.assertEqual(result.returncode, 7)
        self.assertIn("bad", result.stdout)

    def test_check_true_raises_command_error(self):
        with self.assertRaises(AdaSDFCommandError) as raised:
            run_command(
                [sys.executable, "-c", "import sys; print('bad'); sys.exit(5)"],
                check=True,
            )
        self.assertEqual(raised.exception.returncode, 5)
        self.assertIn("bad", raised.exception.stdout)

    def test_command_result_contains_stdout_stderr_returncode(self):
        result = run_command([sys.executable, "-c", "print('ok')"])
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.strip(), "ok")
        self.assertEqual(result.stderr, "")
        self.assertIsNotNone(result.elapsed_seconds)


if __name__ == "__main__":
    unittest.main()

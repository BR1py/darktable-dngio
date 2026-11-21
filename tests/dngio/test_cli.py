"""Fixture-free smoke checks for the explicitly selected darktable build."""

import os
from pathlib import Path
import subprocess
import unittest

CLI = os.environ.get("DARKTABLE_CLI") or os.environ.get("DT_CLI")

class CliSmokeTests(unittest.TestCase):
    @unittest.skipUnless(CLI, "set DARKTABLE_CLI to the darktable-cli executable")
    def test_cli_reports_version(self):
        result = subprocess.run(
            [CLI, "--version"],
            capture_output=True,
            text=True,
            timeout=30,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("darktable ", result.stdout)
        version = (Path(__file__).resolve().parents[2] / "DNGIO_VERSION").read_text().strip()
        self.assertEqual(result.stdout.splitlines()[0], "darktable " + version)


if __name__ == "__main__":
    unittest.main()

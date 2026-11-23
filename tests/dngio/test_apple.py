"""Apple DNG import checks using one licensed fixture per encoding/type."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


CLI = os.environ.get("DARKTABLE_CLI") or os.environ.get("DT_CLI")
SAMPLES = Path(os.environ.get("APPLE_DNG_DIR", Path(__file__).parent / "fixtures"))


@unittest.skipUnless(CLI, "set DARKTABLE_CLI to the darktable-cli executable")
class AppleLoaderTests(unittest.TestCase):
    def render(self, name):
        source = SAMPLES / name
        if not source.is_file():
            self.skipTest(f"No fixture available (only licensed samples are bundled): {name}")
        from PIL import Image, ImageStat

        with tempfile.TemporaryDirectory(prefix="dt-apple-") as directory:
            work = Path(directory)
            output = work / "render.png"
            result = subprocess.run(
                [CLI, str(source), str(output), "--width", "256", "--height", "256",
                 "--core", "--configdir", str(work / "config"),
                 "--cachedir", str(work / "cache"), "--disable-opencl", "-t", "2",
                 "-d", "imageio", "--conf", "libraw_extensions=",
                 "--conf", "write_sidecar_files=never"],
                capture_output=True, text=True, timeout=120)
            log = result.stdout + result.stderr
            self.assertEqual(result.returncode, 0, log)
            self.assertTrue(output.is_file() and output.stat().st_size > 0, log)
            with Image.open(output) as image:
                mean = sum(ImageStat.Stat(image.convert("RGB")).mean) / 3
            self.assertGreater(mean, 0, log)
            return log, mean

    def test_proraw_uses_libraw(self):
        log, _ = self.render("apple-proraw-ljpeg-linear-12bit.dng")
        self.assertIn("delegating Apple ProRAW to LibRaw", log)
        self.assertIn("[libraw_open]", log)

    def test_mosaic_dng_uses_rawspeed(self):
        log, _ = self.render("apple-mosaic-ljpeg-16bit.dng")
        self.assertNotIn("delegating Apple ProRAW to LibRaw", log)
        self.assertNotIn("[libraw_open]", log)

    def test_jxl_proraw_linearization_brightness(self):
        # An ignored local sample can exercise this case until a licensed fixture is available.
        log, mean = self.render("apple-proraw-jxl-linearization.dng")
        self.assertIn("[libraw_open]", log)
        # Unlinearized 10-bit codes interpreted as 16-bit produce a near-black render.
        self.assertGreater(mean, 40, f"ProRAW render is underexposed: mean={mean}")


if __name__ == "__main__":
    unittest.main()

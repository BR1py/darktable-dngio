"""Optional loader checks using the downloaded PhotoPrism Apple samples."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


CLI = os.environ.get("DARKTABLE_CLI") or os.environ.get("DT_CLI")
SAMPLES = os.environ.get("APPLE_DNG_DIR")


@unittest.skipUnless(CLI and SAMPLES, "set DARKTABLE_CLI and APPLE_DNG_DIR")
class AppleLoaderTests(unittest.TestCase):
    def test_jxl_proraw_linearization_brightness(self):
        from PIL import Image, ImageStat

        source = Path(SAMPLES) / "IMG_1345.DNG"
        self.assertTrue(source.is_file(), f"Missing sample: {source}")
        with tempfile.TemporaryDirectory(prefix="dt-apple-jxl-") as directory:
            work = Path(directory)
            output = work / "render.png"
            result = subprocess.run(
                [CLI, str(source), str(output), "--width", "256", "--height", "256",
                 "--core", "--configdir", str(work / "config"),
                 "--cachedir", str(work / "cache"), "--disable-opencl", "-t", "2",
                 "--conf", "write_sidecar_files=never"],
                capture_output=True, text=True, timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            with Image.open(output) as image:
                mean = sum(ImageStat.Stat(image.convert("RGB")).mean) / 3
            # Unlinearized 10-bit codes interpreted as 16-bit produce a near-black render.
            self.assertGreater(mean, 40, f"ProRAW render is underexposed: mean={mean}")

    def test_proraw_uses_libraw_and_mosaic_dng_uses_rawspeed(self):
        samples = Path(SAMPLES)
        names = ["Apple - iPhone 12 Pro - 8bit (4_3).DNG",
                 "Apple - iPhone 6s Plus - 16bit (4_3).DNG",
                 "Apple - iPhone 7 Plus - 16bit (4_3).DNG",
                 "Apple - iPhone 8 - 16bit (4_3).dng",
                 "Apple - iPhone SE - 16bit (4_3).dng",
                 "Apple - iPhone XS - 16bit (4_3).dng"]
        for index, name in enumerate(names):
            with self.subTest(sample=name), tempfile.TemporaryDirectory(prefix="dt-apple-") as directory:
                source = samples / name
                self.assertTrue(source.is_file(), f"Missing sample: {source}")
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
                if index == 0:
                    self.assertIn("delegating Apple ProRAW to LibRaw", log)
                    self.assertIn("[libraw_open]", log)
                else:
                    self.assertNotIn("[libraw_open]", log)


if __name__ == "__main__":
    unittest.main()

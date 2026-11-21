"""DNG integration checks run by CTest against the selected darktable build."""

import json
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


FIXTURES = Path(__file__).parent / "fixtures"
CLI = os.environ.get("DARKTABLE_CLI") or os.environ.get("DT_CLI")
EXIFTOOL = os.environ.get("EXIFTOOL", "exiftool")


def raw_samples(path):
    """Read the 16-bit, uncompressed CFA plane produced by the SDK, not previews."""
    data = path.read_bytes()
    endian = {b"II": "<", b"MM": ">"}[data[:2]]

    def values(offset, code, count=1):
        return struct.unpack_from(endian + str(count) + code, data, offset)

    if values(2, "H")[0] != 42:
        raise ValueError("Expected classic TIFF")
    pending = [values(4, "I")[0]]
    seen = set()
    while pending:
        offset = pending.pop()
        if not offset or offset in seen:
            continue
        seen.add(offset)
        count = values(offset, "H")[0]
        tags = {}
        for i in range(count):
            entry = offset + 2 + 12 * i
            tag, kind, size = values(entry, "H")[0], values(entry + 2, "H")[0], values(entry + 4, "I")[0]
            if kind not in (1, 3, 4, 13):
                continue
            code, width = {1: ("B", 1), 3: ("H", 2), 4: ("I", 4), 13: ("I", 4)}[kind]
            start = entry + 8 if width * size <= 4 else values(entry + 8, "I")[0]
            tags[tag] = values(start, code, size)
        pending.append(values(offset + 2 + 12 * count, "I")[0])
        pending.extend(tags.get(330, ()))
        if tags.get(262) != (32803,) or tags.get(254, (0,))[0] & 1:
            continue
        if tags.get(259) != (1,) or tags.get(258) != (16,):
            raise ValueError("Expected an uncompressed 16-bit CFA plane")
        offsets, counts = tags.get(273), tags.get(279)
        if not offsets or not counts or len(offsets) != len(counts):
            raise ValueError("Expected matching strip offsets and byte counts")
        samples = b"".join(data[o:o + n] for o, n in zip(offsets, counts))
        width, height = tags[256][0], tags[257][0]
        if len(samples) != width * height * 2:
            raise ValueError("Unexpected CFA payload size")
        return width, height, samples, endian
    raise ValueError("No CFA plane found")


@unittest.skipUnless(CLI, "set DARKTABLE_CLI to the build under test")
class DngTests(unittest.TestCase):
    threads = 2

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="dt-dngio-")
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.counter = 0

    def fixture(self, name):
        path = FIXTURES / name
        self.assertTrue(path.is_file(), f"Missing required fixture: {path}")
        return path

    def metadata(self, path):
        result = subprocess.run([EXIFTOOL, "-j", "-n", str(path)], capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(result.stdout)[0]

    def export(self, source, name, target=2, compression=1, xmp=None, succeeds=True, **settings):
        self.counter += 1
        job = self.work / str(self.counter)
        job.mkdir()
        output = job / name
        command = [CLI, str(source)]
        if xmp:
            command.append(str(xmp))
        command += [str(output)]
        if output.suffix == ".png":
            command += ["--width", "256", "--height", "256"]
        command += ["--core", "--configdir", str(job / "config"), "--cachedir", str(job / "cache"),
                    "--disable-opencl", "-t", str(self.threads), "--conf", "write_sidecar_files=never",
                    "--conf", f"worker_threads={self.threads}", "--conf", "resourcelevel=reference"]
        options = dict(target_version=target, compression=compression, effort=1,
                       preview_size=0, consider_crop="false", clone_xmp="false")
        options.update(settings)
        for key, value in options.items():
            command += ["--conf", f"plugins/imageio/format/dng_jxl/{key}={value}"]
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        log = result.stdout + result.stderr
        if succeeds:
            self.assertEqual(result.returncode, 0, log)
            self.assertTrue(output.is_file() and output.stat().st_size > 0, log)
        else:
            self.assertNotEqual(result.returncode, 0, log)
            self.assertFalse(output.exists(), log)
        return output, log

    def test_import_fixtures(self):
        for name in ("TEST1.ARW", "TEST2.dng", "TEST2_loss_dng_converter.dng",
                     "TEST2_lossy_dng_converter.dng", "TEST2.lossy-d0.50-3ev.dng",
                     "TEST3.dng", "TEST4.dng", "TEST5.ORF", "TEST6.dng"):
            self.fixture(name)
        files = sorted(p for p in FIXTURES.iterdir() if p.suffix.lower() in (".dng", ".arw", ".orf"))
        self.assertGreaterEqual(len(files), 1)
        for source in files:
            with self.subTest(source=source.name):
                output, _ = self.export(source, "render.png")
                image = self.metadata(output)
                self.assertGreater(image["ImageWidth"], 0)
                self.assertGreater(image["ImageHeight"], 0)
                self.assertLessEqual(max(image["ImageWidth"], image["ImageHeight"]), 256)

    def test_fixture_privacy(self):
        for line in (FIXTURES / "SHA256SUMS").read_text().splitlines():
            digest, name = line.split("  ", 1)
            source = self.fixture(name)
            with self.subTest(source=name):
                self.assertEqual(hashlib.sha256(source.read_bytes()).hexdigest(), digest)
                result = subprocess.run([EXIFTOOL, "-j", "-G1", "-a", str(source)],
                                        capture_output=True, text=True, timeout=60)
                self.assertEqual(result.returncode, 0, result.stderr)
                for key, value in json.loads(result.stdout)[0].items():
                    tag = key.split(":")[-1]
                    if "Serial" in tag:
                        self.assertFalse(str(value).strip("0 "), key)
                    if tag in ("Artist", "Copyright", "Creator", "Rights"):
                        self.assertIn(value, ("", "BR1py", ["BR1py"]), key)
                    if tag == "OriginalRawFileName":
                        self.assertNotIn("/", str(value), key)
                        self.assertNotIn("\\", str(value), key)

    def test_lossless_roundtrip(self):
        for name in ("TEST2.dng", "TEST6.dng"):
            with self.subTest(source=name):
                source = self.fixture(name)
                baseline, _ = self.export(source, "baseline.dng", compression=0)
                compressed, _ = self.export(source, "lossless.dng")
                decoded, _ = self.export(compressed, "decoded.dng", compression=0)
                self.assertEqual(raw_samples(baseline), raw_samples(decoded))
                before, after = self.metadata(source), self.metadata(compressed)
                for tag in ("ActiveArea", "DefaultCropOrigin", "DefaultCropSize", "CFAPattern2",
                            "ColorMatrix1", "ColorMatrix2", "AsShotNeutral", "BlackLevel", "WhiteLevel"):
                    if tag in before:
                        self.assertEqual(after.get(tag), before[tag], tag)
                self.export(compressed, "roundtrip.png")

    def test_camera_raw_conversion(self):
        for name in ("TEST1.ARW", "TEST5.ORF"):
            with self.subTest(source=name):
                output, _ = self.export(self.fixture(name), "converted.dng")
                info = self.metadata(output)
                self.assertEqual(info["Compression"], 52546)
                self.assertEqual(info["PhotometricInterpretation"], 32803)
                self.assertIn("ColorMatrix1", info)
                self.assertIn("AsShotNeutral", info)
                before = self.metadata(self.fixture(name))
                for tag in ("DateTimeOriginal", "ExposureTime", "ISO", "FNumber"):
                    # The SDK omits an unknown aperture recorded as zero by manual lenses.
                    if tag == "FNumber" and before.get(tag) == 0:
                        continue
                    if tag in before:
                        self.assertEqual(info.get(tag), before[tag], tag)
                self.export(output, "roundtrip.png")

    def test_preview_and_threading(self):
        samples = []
        previews = []
        for threads in (1, 4):
            with self.subTest(threads=threads):
                self.threads = threads
                output, _ = self.export(self.fixture("TEST6.dng"), "preview.dng", preview_size=25)
                metadata = subprocess.run([EXIFTOOL, "-j", "-G1", "-n", str(output)],
                                          capture_output=True, text=True, timeout=60)
                self.assertEqual(metadata.returncode, 0, metadata.stderr)
                info = json.loads(metadata.stdout)[0]
                self.assertEqual(info["IFD0:ImageWidth"], 256)
                self.assertEqual(info["SubIFD:Compression"], 52546)
                preview = subprocess.run([EXIFTOOL, "-b", "-PreviewImage", str(output)],
                                         capture_output=True, timeout=60)
                self.assertEqual(preview.returncode, 0, preview.stderr)
                self.assertTrue(preview.stdout.startswith(b"\xff\xd8"), "missing JPEG preview")
                previews.append(hashlib.sha256(preview.stdout).digest())
                self.export(output, "render.png")
                decoded, _ = self.export(output, "decoded.dng", compression=0)
                samples.append(raw_samples(decoded))
        self.assertEqual(samples[0], samples[1])
        self.assertEqual(previews[0], previews[1], "thread count changed the generated preview")

    def test_compression_and_versions(self):
        for target, modes in ((0, (0, 1)), (1, (0, 1, 3)), (2, (0, 1, 2, 3))):
            for mode in modes:
                with self.subTest(target=target, compression=mode):
                    output, _ = self.export(self.fixture("TEST6.dng"), "encoded.dng", target, mode)
                    info = self.metadata(output)
                    # DNGVersion identifies the writer's format; compatibility is DNGBackwardVersion.
                    backward = tuple(map(int, info["DNGBackwardVersion"].split()))
                    self.assertLessEqual(backward, ((1, 1, 0, 0), (1, 4, 0, 0), (1, 7, 1, 0))[target])
                    self.assertEqual(info["PhotometricInterpretation"], 34892 if mode == 3 else 32803)
                    expected = 1 if mode == 0 else (52546 if target == 2 else (34892 if mode == 3 else 7))
                    self.assertEqual(info["Compression"], expected)
                    minimum = (1, 7, 0, 0) if expected == 52546 else ((1, 4, 0, 0) if mode == 3 else (1, 1, 0, 0))
                    self.assertGreaterEqual(backward, minimum)
                    self.export(output, "roundtrip.png")

    def test_crop_and_xmp(self):
        params = struct.pack("<4f2i", 0.25, 0.25, 0.75, 0.75, 0, 1).hex()
        xmp = self.work / "crop.xmp"
        xmp.write_text(f'''<x:xmpmeta xmlns:x="adobe:ns:meta/">
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
<rdf:Description xmlns:darktable="http://darktable.sf.net/" darktable:xmp_version="5"
 darktable:history_end="1" darktable:iop_order_version="4">
<darktable:history><rdf:Seq><rdf:li darktable:num="0" darktable:operation="crop"
 darktable:enabled="1" darktable:modversion="3" darktable:params="{params}"
 darktable:multi_priority="0" darktable:multi_name=""/>
</rdf:Seq></darktable:history></rdf:Description></rdf:RDF></x:xmpmeta>''')
        source = self.fixture("TEST6.dng")
        output, _ = self.export(source, "crop.dng", xmp=xmp, consider_crop="true", clone_xmp="true", crop_extension=0)
        before, after = self.metadata(source), self.metadata(output)
        for dimension in ("ImageWidth", "ImageHeight"):
            self.assertAlmostEqual(after[dimension], before[dimension] / 2, delta=4)
            self.assertEqual(after[dimension] % 2, 0)
        self.assertEqual(after["CFAPattern2"], before["CFAPattern2"])
        sidecar = Path(str(output) + ".xmp")
        self.assertTrue(sidecar.is_file())
        ns = "{http://darktable.sf.net/}"
        for entry in ET.parse(sidecar).iter():
            if entry.get(ns + "operation") == "crop":
                self.assertEqual(entry.get(ns + "enabled"), "0", "crop must not be applied twice")
        self.export(output, "cropped.png")

    def test_reject_invalid_exports(self):
        source = self.fixture("TEST6.dng")
        for target, mode in ((0, 2), (0, 3), (1, 2)):
            with self.subTest(target=target, compression=mode):
                _, log = self.export(source, "invalid.dng", target, mode, succeeds=False)
                self.assertIn("could not export", log)
        rgb, _ = self.export(source, "render.png")
        _, log = self.export(rgb, "invalid.dng", succeeds=False)
        self.assertIn("could not export", log)


if __name__ == "__main__":
    unittest.main()

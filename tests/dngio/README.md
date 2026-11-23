# dngio integration tests

These tests exercise the fork's Adobe DNG SDK integration through
`darktable-cli`. They use darktable's CTest environment, enabled by
`BUILD_TESTING`, and live outside the upstream integration-test submodule.

DT's C unit tests use cmocka. Its separate rendering suite in
`src/tests/integration` runs CLI/XMP cases against reference PNG images and
also supports custom drivers. The dngio tests use the same CLI and CPU-testing
approach, with assertions on DNG encoding, decoded raw samples, and metadata
instead of stored reference renders.

## Configure and run

Requirements: darktable's normal build dependencies, cmocka (required by DT
when enabling tests), Python 3, Pillow (`python3-pil` on Debian/Ubuntu),
ExifTool, and the local image fixtures. Configure the build with bundled LibRaw
and the Adobe SDK enabled, using the SDK path appropriate to your checkout.

From the DT source directory, use the [repository build helper](../../doc/dngio/build.md):

```bash
tools/build-dngio.sh
ctest --test-dir build-dngio/darktable -R '^dngio\.' --output-on-failure
```

The helper selects bundled LibRaw, `USE_DNGIO=ON`, the repository SDK and
JPEG XL sources, and `BUILD_TESTING=ON`. CTest selects the executable from that
build. To list registered cases:

```bash
ctest --test-dir build-dngio/darktable -N -R '^dngio\.'
```

Run against an installed executable without reconfiguring:

```bash
DARKTABLE_CLI="$PWD/install-dngio/bin/darktable-cli" \
  python3 -m unittest discover -s tests/dngio -p 'test_*.py' -v
```

`DT_CLI` remains an alias for `DARKTABLE_CLI`. Set `EXIFTOOL` if ExifTool is
not on PATH. Direct Python runs skip executable-dependent checks when no CLI
is selected; skipped checks do not establish that a build works. Missing
required fixtures fail the image tests. Apple-specific cases skip individually
when their representative fixture is unavailable. Only licensed images are
bundled; the JPEG XL case can use an ignored private local sample. Apple tests
use `fixtures/` by default; `APPLE_DNG_DIR` can select an external directory with the same
descriptive filenames.

## Coverage

| CTest case | Assertions |
| --- | --- |
| `dngio.cli` | The selected executable starts and reports the release specified by `DNGIO_VERSION`. |
| `dngio.import_to_catalog` | Persistent-library checks verify the default-off behavior, successful DNG transcoding and cropped/camera-raw imports, and no catalog entry after failed export. |
| `dngio.cloned_xmp_brightness` | Rendered brightness survives cloned-XMP re-import for DNG and camera raw inputs, lossless and both lossy modes, with and without cropping. Custom sensor levels and exposure edits are retained. |
| `dngio.fixture_privacy` | Approved fixture checksums match and metadata contains no personal names, nonzero serials, or private paths. |
| `dngio.apple_import` | One licensed fixture per Apple DNG type checks ProRAW and mosaic loader selection. The JPEG XL brightness regression uses an ignored local sample or skips when absent. |
| `dngio.import_fixtures` | All DNG, ARW, and ORF files in `fixtures/` render into nonempty PNGs of the requested dimensions. This includes the restored lossy mosaic DNG. |
| `dngio.lossless_roundtrip` | TEST2 and TEST6 survive JPEG XL lossless transcoding with identical decoded 16-bit CFA samples and geometry, retained calibration metadata, and successful re-import. |
| `dngio.camera_raw_conversion` | Sony ARW and Olympus ORF convert into JPEG XL CFA DNGs with calibration metadata and render after re-import. |
| `dngio.preview_and_threading` | One- and four-thread exports produce identical decoded raw samples and JPEG previews; previews and exported DNGs can be read. |
| `dngio.compression_and_versions` | Nine supported target/compression combinations have the expected encoding, CFA or linear representation, compatible backward-version tags, and successful re-import. |
| `dngio.crop_and_xmp` | A generated XMP applies a central 50% crop, produces CFA-aligned dimensions, clones the sidecar, and allows re-import. |
| `dngio.reject_invalid_exports` | Unsupported target/compression combinations and rendered RGB input fail without leaving a DNG output. |

Every export uses a fresh configuration/cache directory and OpenCL disabled.
Most checks use two CPU threads; the threading case compares one and four.
Outputs use temporary directories removed after each test.
Input sidecar writing is disabled. CTest captures failure details in its usual
`Testing/Temporary/LastTest.log`; use `--output-on-failure` to display them.

## GUI regression checks

The optional GUI checks require a desktop display and a Lua-enabled DNGIO
build. They open isolated test windows and verify that automatic catalog import
preserves the lighttable scroll position, darkroom filmstrip position, and image
selection while registering the exported file. Run them separately from the
headless CTest suite:

```bash
DARKTABLE_GUI="$PWD/install-dngio/bin/darktable" \
  python3 tests/dngio/test_gui.py -v
```

These checks use temporary catalogs, generated JPEG thumbnails, and a copy of
`TEST6.dng`. They do not use your normal catalog or configuration. Without
`DARKTABLE_GUI`, unittest discovery skips them.

## Extend the suite

### Benchmark export time

Run the reusable benchmark against a selected build from the DT source directory:

```bash
DARKTABLE_CLI="$PWD/build-dngio/darktable/bin/darktable-cli" \
  python3 tests/dngio/benchmark_export.py > /tmp/dngio-benchmark.jsonl
```

It records two wall-time and child-process CPU measurements for each of four paths: DNG transcoding
without previews, DNG transcoding with a 25% preview, camera-raw conversion, and
the crop-enabled export path. It uses the integration-test helper's isolated
settings, two CPU threads by default, effort 1, and temporary outputs. Use
`--threads 1` or `--threads 8` to compare thread limits. Times include CLI
startup and output cleanup. Compare runs on the same machine and fixtures;
this benchmark reports measurements rather than enforcing a timing threshold.
Run timing comparisons sequentially with other builds and tests idle. The
`average_cpu_cores` field is child CPU time divided by elapsed time, not a
measurement of an individual processing stage.

### Export optimization measurements (2026-11-21)

Indicative median wall times from two sequential runs per case on the development
machine, with the same fixtures, two DT threads, and effort 1:

| Path | Before | After |
| --- | ---: | ---: |
| TEST2 DNG, no preview | 6.52 s | 1.98 s |
| TEST2 DNG, 25% preview | 6.58 s | 4.31 s |
| TEST1 camera raw | 5.54 s | 1.63 s |
| TEST6 crop-enabled path | 2.30 s | 1.67 s |

The changes skip DT's unused RGB export rendering while retaining pipeline
geometry, omit SDK stage 3 when rendering and opcode compatibility do not require
it, pass intermediate DNGs through memory, and use DT's thread limit for SDK
processing and JPEG XL. Eight-thread measurements varied by case and did not
justify increasing the default thread count. Timings include CLI startup and
temporary-output cleanup; these are observations, not portable performance
guarantees. Peak memory use was not measured.

Local measurement records are in the ignored `results/export-before.jsonl`,
`results/export-after-2threads.jsonl`, and `results/export-after-8threads.jsonl`
files. Re-run the benchmark to produce results on another machine.

### Add regression cases

Add assertions to `test_dng.py`, then register new test methods in the local
`CMakeLists.txt`. Keep each case independent. Use `subTest` to identify fixture
or compression combinations, and make failures report the CLI output.

The raw-sample reader deliberately accepts classic TIFF, uncompressed 16-bit
CFA strips produced by the SDK. It rejects unsupported layouts instead of
comparing embedded previews. The lossless test decodes both paths to that
representation before comparing samples.

The [fixture notes](fixtures/README.md) describe local inputs and publication
records. The approved image files use Git LFS and CC0; run `git lfs pull` before testing.

## Remaining coverage

The suite does not yet measure lossy quality tolerances, compare renders to
independently approved reference pixels, test X-Trans raw conversion, check
opcode/private-metadata preservation, or measure preview color accuracy against
an independent reference. GUI, OpenCL,
and AI paths are outside these CPU CLI checks. An Adobe validation-tool pass
would provide an additional independent check of DNG conformance.

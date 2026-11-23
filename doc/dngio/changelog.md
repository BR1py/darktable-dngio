# DNGIO changelog

Changes to this fork, based on darktable 5.6.2. See the
[feature overview](features.md) for usage and supported formats.

## 5.6.2-dngio.3 — local development build

- Fixed Apple ProRAW import by routing full-resolution linear RGB DNGs to
  LibRaw and the Adobe DNG SDK. Apple mosaic DNGs retain the RawSpeed path.
- Fixed severely underexposed JPEG XL ProRAW images with an explicit
  `LinearizationTable`.
  The decoder now applies that table while preserving the existing behavior
  for JPEG XL DNGs without one.
- Added optional regression checks for loader selection and JPEG XL ProRAW
  brightness using externally supplied local files. ProRAW files are not
  included in the test suite.
- Grouped fork-specific README information under **About this darktable fork**
  and documented the import limitation for manufacturer-specific DNG attributes.
- Updated the application version to `5.6.2-dngio.3`.

Validation: brightness and loader checks using external local Apple files,
lossless round trips, and compression/version checks passed. Apple's
profile gain map, embedded tone curve, and baseline exposure adjustment are
not applied by this import path; matching the manufacturer’s preview is not
guaranteed. No version 3 AppImage has been built or released.

## 5.6.2-dngio.2 — local development build

- Fixed automatic catalog import after export resetting the lighttable and
  darkroom filmstrip scroll positions. The collection refresh now preserves
  the current position and image selection.
- Documented command-line DNG export in the feature guide, including AppImage
  usage, compression settings, XMP cloning, cropping, persistent catalog import,
  and whole-folder conversion with Bash platform requirements.
- Added **import to catalog** to DNG export, disabled by default. Successful
  **file on disk** exports can be imported automatically after their XMP
  sidecars are written, while retaining the current image selection.
- Fixed overly bright images after re-importing lossy DNGs with cloned XMP.
  Sensor black/white settings are rescaled to the decoded DNG range, while
  exposure edits and representable custom sensor-level adjustments are retained.
- Fixed cropped XMP cloning to keep sensor normalization enabled while
  resetting sensor crop margins. Added handling for legacy sensor parameters
  and brightness regression tests for DNG and camera raw inputs.
- Updated the application version to `5.6.2-dngio.2`. Existing export presets
  retain their settings, with automatic catalog import initially disabled.

The local build passed all 13 configured tests and a packaged brightness/catalog
import check. Version 2 is under development and has not been released.

For previously affected images, re-export from the original raw file to a new
filename to generate corrected XMP. Existing catalog histories are not rewritten
automatically.

## 5.6.2-dngio.1 — published

- Added Adobe SDK import support for mosaic and linear JPEG XL DNGs.
- Added raw DNG export with DNG 1.1, 1.4, and 1.7.1 targets, including
  lossless compression and lossy mosaic/linear modes where supported.
- Added embedded previews, sensor-aligned cropping, and optional cloned XMP
  sidecars. Every supported DNG export mode can be imported again.
- Improved export speed by avoiding unused RGB rendering and intermediate
  disk files, and by using DT's thread limit for SDK processing and JPEG XL.
- Added `USE_DNGIO`, repository-owned SDK sources and build/packaging scripts,
  regression tests, an export benchmark, and sanitized CC0 fixtures using Git LFS.

[Release and AppImage download](https://github.com/BR1py/darktable-dngio/releases/tag/5.6.2-dngio.1).
The application version for that release is `5.6.2-dngio-1`.

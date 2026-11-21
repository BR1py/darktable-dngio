# DNG import and export in 5.6.2-dngio-1

This release extends darktable 5.6.2 with Adobe DNG SDK support for importing
and exporting DNG files. Every supported DNG export mode can be imported again
by this build, including JPEG XL (JXL) compression for both mosaic and linear
raw images.

These features require a build with `USE_DNGIO=ON`; see the
[build instructions](build.md). They are additions provided by this fork.

## Import

Import DNG files through the usual darktable import workflow. The integration
adds decoding for JPEG XL DNGs, including lossless and lossy mosaic DNGs and
linear DNGs. Mosaic DNGs retain the sensor's color filter array (CFA); linear
DNGs contain already demosaiced image data.

The Adobe SDK handles DNG decoding paths that need it, including linear DNG
processing and its associated DNG opcodes. Existing raw import paths remain
available for other supported files. The exported combinations in the table
below can all be reopened for further editing.

## Export

Select **DNG** as the file format in the export panel to convert a supported
camera raw file or transcode an existing DNG. This exports raw image data:
darkroom adjustments such as exposure, color grading, and sharpening are not
baked into the raw pixels. Use a rendered format such as JPEG or TIFF when
you want to export the edited appearance.

![DNG export panel showing target version, compression, quality, encoding effort, preview size, crop, and XMP sidecar controls](images/export_dialog.png)

Example settings for DNG 1.7.1 with lossy mosaic compression. The settings
shown are an example rather than the export defaults.

### Target version and compression

The **DNG target version** controls the available compression modes:

| Compression | DNG 1.1 | DNG 1.4 | DNG 1.7.1 |
| --- | --- | --- | --- |
| Uncompressed | Yes | Yes | Yes |
| Lossless | Lossless JPEG | Lossless JPEG | Lossless JPEG XL |
| Lossy, preserve mosaic | — | — | JPEG XL |
| Lossy, linear raw | — | JPEG | JPEG XL |

Lossless compression preserves the raw samples supplied to the encoder.
Recompressing an already lossy source does not restore discarded information.
**Lossy, preserve mosaic** keeps the CFA layout; **lossy, linear raw** produces
demosaiced data. Choose a target and compression supported by the application
that will read the file.

For lossy modes, **quality** ranges from 1 to 100 (default 90). Higher values
preserve more detail and generally produce larger files. JPEG XL lossy modes
also expose **encoding effort**, from 1 to 9 (default 7); higher effort can
reduce file size at the cost of encoding time.

### Preview, crop, and editing settings

- **Preview size** embeds an SDK-rendered JPEG preview, sized from 0 to 100%
  of the visible raw image's dimensions (default 50%). Set it to 0 to omit
  previews. The preview comes from the Adobe SDK's rendering of the raw data
  and does not reproduce darktable's darkroom edits.
- **Consider crop** exports an enclosing raw rectangle corresponding to the
  current crop, mapped back into the original sensor orientation. **Extend
  crop** adds surrounding raw data toward the full image (0–99%, default 0%).
  This retains raw data rather than baking geometric transformations into it.
- **Clone XMP sidecar** writes the current darktable settings next to the
  exported file as `<filename>.dng.xmp`. With cropping enabled, the control
  becomes **partly clone XMP sidecar**: masks and geometric operations are
  reset because the raw image coordinates have changed. Both sidecar options
  are off by default.

The default export is DNG 1.7.1 with lossless JPEG XL compression, a 50%
preview, and cropping disabled.

## Scope and verification

Existing DNGs are transcoded directly when **consider crop** is disabled.
Conversion from other camera raw formats, and the crop path, require
single-channel 16-bit Bayer or X-Trans raw data. Rendered RGB inputs such as
JPEG and TIFF are not supported by this raw DNG exporter.

The [test suite](../../tests/dngio/README.md) checks import, re-import of all
nine target/compression combinations, lossless raw-sample round trips,
camera raw conversion, previews, cropping, and XMP sidecars. X-Trans export
is implemented but does not yet have a regression fixture in this suite.

See [licensing and third-party notices](../../LICENSING.md) for the bundled
Adobe SDK and other dependencies.

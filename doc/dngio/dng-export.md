---
title: DNG export
id: dng-export
weight: 90
---

The DNG export format writes raw sensor data to a new Digital Negative (DNG)
file. Unlike a conventional image export, it does not render the complete
darktable pixelpipe into RGB pixels. The result remains a raw file that can be
developed again in darktable or another DNG-compatible application.

The exporter accepts raw images with a 16-bit Bayer or X-Trans mosaic, including
camera raw formats other than DNG. Exporting a non-raw image, such as JPEG,
TIFF, PNG or an already-rendered RGB image, fails because it contains no raw
sensor mosaic to store in the output DNG.

An existing DNG is handled as a special case when **consider crop** is disabled:
darktable transcodes it directly instead of rebuilding it from the normalized
raw buffer. This retains the source DNG's authoritative raw geometry,
calibration data, private metadata and opcode lists. This special path does not
limit DNG export to DNG input; supported non-DNG camera raw files are converted
to DNG through the regular raw export path.

> **Warning:** Lossy compression and physical cropping are irreversible. Lossy
> compression discards sample information, while **consider crop** permanently
> removes sensor samples outside the stored rectangle. Converting the resulting
> DNG later to lossless compression or to an uncompressed DNG cannot reconstruct
> either kind of discarded information; it only stores the already-reduced data
> without further compression loss. Keep the original raw file whenever the
> complete sensor data or original sample values may be needed.

## motivation

High-resolution camera sensors produce very large raw files, which can make
long-term storage, backup and transfer expensive. JPEG XL compression can
substantially reduce the size of DNG files while preserving a reusable raw
workflow. With a moderate quality setting of 90, reductions of around 70% or
more compared with an uncompressed DNG may be possible, although the actual
result depends on image content, sensor noise, resolution and the source
encoding. Physical cropping can reduce the file further by omitting sensor
areas that are no longer required. These savings must be balanced against the
irreversible information loss introduced by lossy compression and cropping.

## format options

### DNG target version

Select the minimum DNG specification required to read the exported file. A
newer target enables additional compression methods but may reduce compatibility
with older software.

| target | uncompressed | lossless | lossy, preserve mosaic | lossy, linear raw |
|---|---:|---:|---:|---:|
| DNG 1.1 | yes | yes | no | no |
| DNG 1.4 | yes | yes | no | yes |
| DNG 1.7.1 | yes | yes, JPEG XL | yes, JPEG XL | yes |

Use DNG 1.1 or 1.4 when compatibility is more important than file size. Use
DNG 1.7.1 only when the applications that will open the file support the newer
DNG and JPEG XL features.

### compression

**uncompressed**

: Store the raw samples without image compression. This produces the largest
files and offers the broadest, simplest decoding path.

**lossless**

: Compress the raw data without changing sample values. DNG 1.7.1 uses
lossless JPEG XL; older targets use the lossless compression available for
their selected DNG version.

**lossy, preserve mosaic**

: Apply lossy JPEG XL compression while retaining the Bayer or X-Trans mosaic.
The output remains a mosaic raw file and requires DNG 1.7.1 support. Although
the mosaic layout is retained, some original sample information is discarded
and cannot be recovered by later transcoding to lossless or uncompressed DNG.

**lossy, linear raw**

: Convert the mosaic to a demosaicked LinearRaw representation and apply lossy
compression. This is available with DNG 1.4 and DNG 1.7.1. Because demosaicing
is baked into the file, later applications cannot choose a different demosaic
algorithm. Both the mosaic-to-LinearRaw conversion and the lossy compression
are irreversible; later lossless or uncompressed output cannot recreate the
original mosaic samples.

The compression list changes automatically when a different target version is
selected. If the current compression is not supported by the new target,
darktable selects a compatible mode.

### quality

Set the quality of lossy compression from 1 to 100. Higher values preserve more
detail and normally create larger files. The default is 90. This control is
only displayed for lossy compression modes and has no effect on uncompressed
or lossless output.

darktable uses libjxl's standard
[`JxlEncoderDistanceFromQuality`](https://libjxl.readthedocs.io/en/latest/api_encoder.html#c.JxlEncoderDistanceFromQuality)
mapping:

- **100** maps to distance 0 and makes the JPEG XL compression stage
  mathematically lossless.
- **90** maps to distance 1, which libjxl describes as visually lossless.
- Lower values permit progressively greater differences in exchange for
  potentially smaller files. Libjxl recommends approximately 68–96 for normal
  lossy use.

“Visually lossless” is a perceptual target rather than a guarantee that decoded
sample values are identical. The result also depends on image content and
encoder effort. In particular, quality 100 only makes the JPEG XL coding stage
mathematically lossless. It does not make **lossy, linear raw** equivalent to a
lossless mosaic export, because conversion to LinearRaw has already baked in
demosaicing.

### encoding effort

Set JPEG XL encoder effort from 1 to 9. Higher values may reduce the file size
but take longer to encode. The default is 7. Effort changes encoder search and
resource use, not the requested quality. This control is displayed for lossy
DNG 1.7.1 output.

### preview size

Set the long edge of the embedded rendered preview as a percentage of the
visible raw image dimensions:

- **0%** embeds no preview.
- **1–100%** embeds a JPEG preview at the requested relative size.
- If a preview is requested, a thumbnail of at most 256 pixels on its long edge
  is also embedded.

The preview is rendered in sRGB, or Gray Gamma 2.2 for monochrome images. It is
provided for browsing and does not replace the raw data.

### consider crop

Choose whether the current developed-image crop should reduce the raw data
stored in the exported DNG.

**no**

: Export the complete raw mosaic. The output retains the source raw dimensions,
so the discarded area remains available for later reframing. For a DNG source,
this also enables direct DNG transcoding without rebuilding the raw image.

**yes**

: Map the boundary of the current pixelpipe output back through the active
geometric modules and store the smallest enclosing rectangle in the original
sensor orientation. The rectangle is expanded when necessary to preserve the
2×2 Bayer or 6×6 X-Trans CFA phase.

Enabling this option has the following effects on the exported DNG:

- Raw samples outside the calculated rectangle are physically omitted. This
  reduces the pixel dimensions and can reduce the file size.
- The removed sensor area cannot be recovered from the exported DNG. Keep the
  original raw file if later reframing might be required.
- Converting the cropped DNG to an uncompressed or losslessly compressed DNG
  does not restore the omitted area. It only changes how the remaining
  rectangle is encoded.
- The stored rectangle is expressed in the original sensor orientation. The
  current rotation, perspective correction or other geometric transformation
  is not rendered into the mosaic.
- Rotated, curved or perspective-corrected output boundaries require an
  enclosing axis-aligned rectangle and can therefore retain pixels outside the
  visible composition.
- The rectangle is expanded to the Bayer or X-Trans grid when required. The
  resulting dimensions can differ slightly from the visible crop.
- An existing DNG can no longer use the direct-transcode path. It is exported
  through the same cropped-mosaic path as another supported camera raw file.
- The embedded preview is generated from the cropped DNG, while the stored raw
  data remains an undeveloped mosaic.

Physical cropping changes the raw coordinate system. If an XMP sidecar is
cloned, geometric operations and coordinate-dependent masks are reset to safe
defaults so that old coordinates are not applied to the new raw dimensions.
The corresponding editing modules remain available for new adjustments.

### extend crop

When **consider crop** is enabled, progressively extend the stored rectangle
toward the full raw image:

- **0%** stores only the enclosing crop rectangle.
- **50%** restores half of each excluded margin.
- Values approaching **100%** retain almost all surrounding raw pixels.

The allowed range is 0–99%. CFA alignment can add a small number of pixels
beyond the mathematically requested boundary.

### clone XMP sidecar / partly clone XMP sidecar

Write a darktable XMP sidecar next to the exported DNG. The sidecar is named
`<exported-file>.dng.xmp`.

With **consider crop** disabled, development settings are cloned. For lossy
exports, sensor black/white settings are rescaled to match the normalized DNG
data; creative exposure settings remain unchanged. Sensor-level adjustments
outside the normalized range cannot be cloned and cause a sidecar error,
leaving the exported DNG on disk. Reopening remains subject to the usual module
and version compatibility requirements and the chosen lossy compression.

With **consider crop** enabled, the option is labelled **partly clone XMP
sidecar**. Metadata and non-geometric development settings are retained, while
the following coordinate-dependent information is reset:

- crop, rotation, perspective and other geometric module parameters,
- drawn-mask history,
- spot-removal and retouch coordinates,
- history hashes that no longer describe the adapted history.

Geometric modules are stored as disabled default instances. They remain
visible in the darkroom and can be used to create new edits for the cropped
DNG. Sensor black/white correction remains enabled; its crop margins are reset
to match the newly stored sensor rectangle.

### import to catalog

Enable this option to add the completed DNG to darktable's catalog when using
**file on disk** storage. It is disabled by default. The destination folder is
registered as a film roll if needed, and the current image selection is retained.

Import happens after the export and any cloned XMP sidecar have been written,
so the imported image can use those editing settings. Failed exports are not
imported. If catalog import fails, the exported file remains on disk and
darktable reports the failure. Other storage targets do not import temporary
export files.

## recommended settings

For archival output with broad compatibility, select DNG 1.4, **lossless**, no
crop and optionally clone the XMP sidecar.

For smaller files while retaining the original mosaic, select DNG 1.7.1,
**lossy, preserve mosaic**, quality 90 and effort 7. Confirm that all target
applications support DNG 1.7.1 JPEG XL before adopting this as an archival
format.

For a compact raw file containing only the useful composition, enable
**consider crop**, choose an extension that leaves enough margin for later
reframing, and enable **partly clone XMP sidecar** if non-geometric darktable
settings should accompany the export.

## limitations and behavior

- The raw export path currently accepts 16-bit Bayer and X-Trans mosaic images.
- A normal DNG export does not bake the darktable development into the raw
  samples; use TIFF, JPEG, AVIF or another rendered format for a finished image.
- A cropped export stores an enclosing sensor-aligned rectangle, not an
  arbitrarily rotated or perspective-transformed raster.
- Lossy LinearRaw output permanently bakes in demosaicing.
- DNG 1.7.1 JPEG XL files require compatible reader software.
- XMP sidecars are optional and darktable-specific development data may be
  ignored by other applications.

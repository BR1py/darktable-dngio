# DNG test fixtures

The nine `TEST*` image files were supplied by BR1py and dedicated under
[CC0 1.0 Universal](LICENSE.md). [SHA256SUMS](SHA256SUMS) identifies the sanitized
files. They are explicitly allowed by `.gitignore` and tracked through Git LFS
via the repository's `.gitattributes`. Run `git lfs pull` after checkout.

The repository's `.lfsconfig` selects
[BR1py/darktable-dngio-fixtures](https://github.com/BR1py/darktable-dngio-fixtures)
as the LFS endpoint. This standalone repository hosts the images because
GitHub blocks new LFS objects in the upstream darktable public fork network.
The original nine sanitized files, checksums, and CC0 dedication are published
there.

For the `TEST*` files, author fields use BR1py. Camera/lens serial values and
private directory paths have been cleared. Do not replace a fixture with an unsanitized original.

| Files | Test role |
| --- | --- |
| `TEST1.ARW` | Sony camera-raw import and CFA DNG export. |
| `TEST2.dng` | Original TEST2 DNG input and lossless raw-sample round-trip comparison. |
| `TEST2_loss_dng_converter.dng` | Adobe Converter lossless reference input. |
| `TEST2_lossy_dng_converter.dng` | Adobe Converter lossy linear reference input. |
| `TEST2.lossy-d0.50-3ev.dng` | Restored lossy mosaic input. |
| `TEST3.dng`, `TEST4.dng` | Additional DNG import cases. |
| `TEST5.ORF` | Olympus camera-raw import and CFA DNG export. |
| `TEST6.dng` | Smaller DNG for compression, crop, validation, and round-trip checks. |

DT-generated outputs belong in temporary test directories, not in this fixture
set. Source-image permission does not automatically extend to unrelated sample
files from other packages.

## Apple fixtures

One licensed sample is retained per type: `apple-proraw-ljpeg-linear-12bit.dng`
for linear ProRAW with lossless JPEG compression, and
`apple-mosaic-ljpeg-16bit.dng` for conventional mosaic DNG with lossless JPEG
compression. Both are CC0 contributions from raw.pixls.us; see
[Third-party notices](../../../THIRD_PARTY_NOTICES.md#third-party-dng-test-images)
for source records and original checksums. GPS metadata was removed without
changing the compressed image data.

No suitable licensed JPEG XL ProRAW image is bundled. A private local sample
may be copied as `apple-proraw-jxl-linearization.dng`; this filename is ignored
by Git and must not be added to commits or the distributed checksum list. The
corresponding test skips when the file is absent. A future fixture
must contain JPEG XL raw data with an explicit linearization table and have
an exposure appropriate for the brightness assertion. Merely converting an
image after baking the table into its pixels does not cover that regression.

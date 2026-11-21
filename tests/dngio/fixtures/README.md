# DNG test fixtures

The nine image files were supplied by BR1py and dedicated under
[CC0 1.0 Universal](LICENSE.md). [SHA256SUMS](SHA256SUMS) identifies the sanitized
files. They are explicitly allowed by `.gitignore` and tracked through Git LFS
via the repository's `.gitattributes`. Run `git lfs pull` after checkout.

The repository's `.lfsconfig` selects
[BR1py/darktable-dngio-fixtures](https://github.com/BR1py/darktable-dngio-fixtures)
as the LFS endpoint. This standalone repository hosts the images because
GitHub blocks new LFS objects in the upstream darktable public fork network.
The same sanitized files, checksums, and CC0 dedication are published there.

Author fields use BR1py. Camera/lens serial values and private directory paths
have been cleared. Do not replace a fixture with an unsanitized original.

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

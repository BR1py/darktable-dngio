# Adobe DNG SDK sources

Imported from Adobe DNG SDK 1.7.1 build 2724, archive
`adobe-dng-sdk-1.7.1-2724.zip`, SHA-256
`740fbe95c69e09e9cd17654a5e4fef2d7021254b06fd2b8c5557b79a1496b50c`.

`source/` contains the SDK source used by this fork. `libjxl/` is the matching,
unmodified JPEG XL source tree and its bundled dependencies from the archive.
`LICENSE.txt` is Adobe's unmodified agreement. Preserve all per-file notices.
Adobe SDK documentation, sample photographs, bundled XMP, and bundled libjpeg
are omitted; this build disables SDK XMP and uses the platform JPEG library.

Seven SDK files differ from the archive. `LOCAL_CHANGES.patch` records those
changes; `SOURCE_MANIFEST.json` records original and imported SDK source hashes.
The libjxl source tree matches the archive. SDK sources retain Adobe's license;
this import does not relicense them under darktable's GPL.

This product includes DNG technology under license by Adobe.

# Build and validate dngio

The repository contains the Adobe SDK source and matching JPEG XL sources at
`src/external/adobe-dng-sdk/`. No Adobe download is required. Other system
libraries are installed through the platform package manager.

## Checkout and build

Install Git LFS before cloning the repository, then obtain the submodules and
approved test images:

```bash
git clone --branch dngio-5.6.2 --recurse-submodules https://github.com/BR1py/darktable-dngio.git
cd darktable-dngio
git lfs pull
sudo tools/install-dngio-build-deps.sh
tools/build-dngio.sh
```

The root `.lfsconfig` directs image downloads to the standalone
[`BR1py/darktable-dngio-fixtures`](https://github.com/BR1py/darktable-dngio-fixtures)
repository. Source code remains in the DT fork; no extra checkout is needed
for the test images. GitHub restricts new LFS uploads to public forks when
their upstream repository network has no existing LFS objects.

The helper builds and installs the bundled JPEG XL sources, configures DT with
`USE_DNGIO=ON`, builds it, runs CTest, and installs into `install-dngio/`.
`BUILD_DIR`, `INSTALL_DIR`, and `JOBS` environment variables override its paths
and concurrency. Additional arguments are passed to DT's CMake configuration.

To validate the disabled configuration in a separate build:

```bash
DNGIO=OFF BUILD_DIR="$PWD/build-dngio-disabled" \
  INSTALL_DIR="$PWD/install-dngio-disabled" tools/build-dngio.sh
```

`USE_DNGIO` replaces the earlier `USE_ADOBE_DNG_SDK` build option. It defaults
to OFF in direct CMake builds. Enabled builds require bundled LibRaw and use
the repository SDK path by default. Disabled builds omit SDK support, DNG
export, and dngio integration tests, and report the base version without the
DNGIO suffix. Other shared DT source changes remain part of this fork.

LibRaw is a submodule pinned to published commit
`9f992f377548359a22067782f9029d6948cb9631` in `BR1py/LibRaw`, on branch
`5.6.2-dngio.3` (LibRaw 0.22.0). The integration patch is also recorded in
`patches/libraw/`; the build helper recognizes that the pinned source already
contains it.

## Create an AppImage

After building and testing the enabled configuration:

```bash
packaging/AppImage/build-dngio.sh
```

This packages `install-dngio/` into `dist/`, including DT and DNG license texts.
It downloads linuxdeploy and its GTK plugin if they are absent from
`build-appimage/tools/`. Packaging tools may require network access and executable
temporary storage. `INSTALL_DIR`, `APPDIR`, `OUTPUT_DIR`, and `TOOLS_DIR` can
select other locations. `APPIMAGE_OUTPUT` overrides the full output filename.

For this release the default artifact name is
`darktable-5.6.2-dngio.3-x86_64.AppImage` on x86-64. Test the actual artifact,
not only its installation directory, before publishing it. Building on a recent
Linux distribution does not establish compatibility with older systems.

## Publication remains separate

The dedicated dngio CI workflow builds enabled and disabled configurations and
runs their configured tests. It takes effect only after the workflow and Git
LFS objects are pushed. Binary release publication, a fresh remote recursive
clone check, and the release tag are separate from organizing and local validation.

# Publication preparation

The LibRaw integration commit is published. DT changes are organized into local
commits; the DT push, Git LFS upload, and release tag remain pending.

| Item | Repository location | State |
| --- | --- | --- |
| LibRaw integration changes | `src/external/LibRaw/`, `patches/libraw/` | Published in [BR1py/LibRaw](https://github.com/BR1py/LibRaw/tree/0.22.0-dt_dngio.1). DT pins `1b58cd78f3b1ab3cc3cd930650fcc7d2943367c6`; `.gitmodules` uses the fork URL. |
| Adobe SDK and matching JPEG XL sources | `src/external/adobe-dng-sdk/` | Sources, licenses, original archive hash, imported hashes, and local-change patch are included. |
| Build and dependency scripts | `tools/build-dngio.sh`, `tools/install-dngio-build-deps.sh` | Repository-owned entry points replace workspace dependencies. |
| AppImage packaging | `packaging/AppImage/build-dngio.sh` | Repository-owned packaging script includes local license records. |
| Approved test images | `tests/dngio/fixtures/` | CC0 dedication, checksums, sanitized metadata, Git LFS attributes, and privacy regression test are prepared. LFS objects have not been uploaded. |
| Feature switch | `DefineOptions.cmake` | `USE_DNGIO` selects SDK import/export and dngio test registration; disabled builds use the base version. |
| Fork CI | `.github/workflows/dngio.yml` | Enabled/disabled build jobs are prepared. Hosted execution requires publishing the workflow and LFS objects. |
| Build and test instructions | `doc/dngio/build.md`, `tests/dngio/README.md` | Commands and scope are documented. |

## Local validation

The repository-source builds and packaged executable passed these checks:

| Configuration | Result |
| --- | --- |
| `USE_DNGIO=ON`, clean build | All 11 configured CTest cases passed. |
| `USE_DNGIO=OFF`, clean build | Both configured CTest cases passed; camera-raw rendering and rejection of DNG export were also checked. |
| Built x86-64 AppImage | All 9 DNGIO unittest groups passed through the packaged CLI. |
| Imported sources and fixtures | SDK source hashes, the LibRaw patch, and all nine fixture checksums matched their recorded values. |

The AppImage checks include import, re-import of every supported export
combination, lossless raw-sample round trips, previews, threading, crop/XMP
behavior, fixture privacy, and invalid exports. They do not cover GUI behavior,
OpenCL, other platforms, or compatibility with older Linux distributions.

## Remaining publication steps

After reviewing local build and test results:

1. Review the intended files, including all vendored sources, licenses, Git LFS
   pointers, and fixture dedication. Keep generated builds and outputs excluded.
2. Push the reviewed DT branch and its LFS objects.
3. Clone `dngio-5.6.2` into a fresh directory with submodules and LFS objects;
   repeat the documented build and tests without the original workspace.
4. Review hosted CI results, choose the fork's default branch, and tag the
   validated source as `5.6.2-dngio-1` before publishing matching release artifacts.

The source-license and bundled-dependency considerations in
[LICENSING.md](../../LICENSING.md) still apply. No raw2dng or raw2pic source,
script, or notice is part of this DT preparation.

# Licensing of the dngio DNG integration

Reviewed on 2026-11-21 for darktable 5.6.2 and the repository's Adobe DNG SDK
1.7.1 build 2724. This document collects the relevant license texts, attribution, and
distribution considerations for this fork. It is
a source-based licensing record, not a declaration that every possible binary
combination has been legally cleared.

## Required DNG attribution

> This product includes DNG technology under license by Adobe.

Adobe's [DNG Specification patent license](https://www.adobe.com/support/downloads/dng/dng_sdk.html)
requires that notice prominently in source code and documentation for compliant
implementations distributed under that license. The patent grant covers
specified essential claims for compliant implementations and includes a patent
litigation revocation provision. It is separate from the SDK software license.

## Our Adobe SDK agreement

The complete, unmodified agreement from the local SDK archive is included at
[licenses/Adobe-DNG-SDK-LICENSE.txt](licenses/Adobe-DNG-SDK-LICENSE.txt).
Its SHA-256 is
`89d75e1bfd65269c41ec57be67adacd48926af81554574d92742f96296ed9166`.
The following summarizes its numbered clauses; the linked text controls.

| Clause | Terms relevant to use and distribution |
| --- | --- |
| Acceptance and definitions | Downloading, installing, using, modifying, or distributing the SDK or its derivatives constitutes acceptance. Software and Documentation are defined separately. |
| 1: Software | Grants worldwide, non-exclusive, royalty-free rights to use, reproduce, modify, display, perform, distribute, and sublicense the software for any purpose, subject to the agreement. |
| 1: Documentation | Permits a limited number of development copies and their display, performance, and distribution. Modification of the Documentation is prohibited. |
| 2: Notices and ownership | Preserve copyright and other notices and include them in human-readable distributed copies. Adobe retains ownership and rights not expressly granted. |
| 3–4: Warranty and liability | Provides the SDK and Documentation as-is, disclaims specified warranties, and limits liability. Preserve the full clauses with the agreement. |
| 5: Commercial distribution | Distribution in a commercial product carries an obligation to defend, indemnify, and hold Adobe harmless against specified claims arising from that distribution. |
| 6: Trademarks | Adobe and DNG marks cannot be used to endorse or promote a product without separate permission. |
| 7: Termination | Material noncompliance terminates rights; if all rights terminate, use and distribution must cease. |
| 8–9: Legal terms | California law, specified California court jurisdiction, and agreement/general provisions apply. |

The SDK agreement explicitly refers readers elsewhere for DNG specification
licensing. Neither the specification patent grant nor an open-source license
on an adjacent component replaces this agreement. Do not label the entire SDK
archive MIT, BSD, or GPL merely because individual files or dependencies use
those licenses.

Adobe's download page is a moving reference. The local build and preserved
agreement above identify what was reviewed; a later download needs a fresh
comparison of its terms and contents.

## Repository copy of the SDK sources

The fork includes the Adobe DNG SDK sources at
[src/external/adobe-dng-sdk](src/external/adobe-dng-sdk/README.md), independently
of Adobe's download pages. The imported version is 1.7.1 build 2724. That
folder preserves the agreement, source notices, matching JPEG XL sources,
original archive checksum, source hashes, and a patch recording seven locally
modified SDK files. Build configuration defaults to this repository copy.

SDK documentation, sample photographs, bundled XMP, and bundled libjpeg are
omitted. XMP is disabled and JPEG is provided by the platform library. The
import does not relicense Adobe code under darktable's GPL notice.

## darktable, LibRaw, and GPL compatibility

darktable's [LICENSE](LICENSE) contains GPL version 3; individual source files
also carry their applicable notices. Distribution of modified GPL-covered
software must preserve those notices and satisfy the relevant source and
distribution requirements. For a downloadable binary, GPLv3 section 6(d)
describes providing equivalent access to its Corresponding Source. That source
includes applicable modifications and build/install scripts, not just a link
to unmodified upstream darktable. See GPLv3 sections 1, 5, and 6.

LibRaw's [COPYRIGHT](src/external/LibRaw/COPYRIGHT) offers a choice between
[LGPL 2.1](src/external/LibRaw/LICENSE.LGPL) and
[CDDL 1.0](src/external/LibRaw/LICENSE.CDDL). The LGPL option is the relevant
route to assess for this GPL-based build. Its notices also describe certain
older Adobe-derived fragments under MIT terms; that does not relicense the
separately integrated full Adobe SDK 1.7.1.

Our [build integration](src/external/LibRaw-cmake/CMakeLists.txt) compiles SDK
source into LibRaw; it is not simply an unrelated SDK archive stored next to
darktable. The [main build](src/CMakeLists.txt) links the integration into DT.
Consequently, review compatibility of the actual combined distribution,
including the SDK's documentation and commercial-indemnification clauses,
against the GPL's distribution requirements. This record does not assert a
linking exception or permission from darktable's copyright holders.

## Additional component licenses

These are unmodified license copies from the local SDK bundle. Applicable
per-file notices and additional notices in dependency source trees must also
be preserved. This is an inventory of the DNG integration's main components,
not a complete inventory of every library collected into an AppImage.

| Component | License text | Build scope |
| --- | --- | --- |
| Adobe DNG SDK | [DNG SDK License Agreement](licenses/Adobe-DNG-SDK-LICENSE.txt) | Used for the optional SDK integration. |
| JPEG XL | [BSD 3-Clause](licenses/JPEG-XL-LICENSE.txt) | Used for JPEG XL decoding and encoding. |
| Brotli | [MIT](licenses/Brotli-LICENSE.txt) | JPEG XL dependency. |
| Highway | [Apache 2.0](licenses/Highway-LICENSE.txt) and [BSD 3-Clause](licenses/Highway-LICENSE-BSD3.txt) | The bundled Highway README describes dual licensing; retain the relevant notices. |
| skcms | [BSD 3-Clause](licenses/skcms-LICENSE.txt) | Retain when included in the selected JPEG XL build. |
| UUID code in bundled XMP | [COPYING](licenses/UUID-COPYING.txt) | Kept as a reference for the bundled component; not evidence that XMP is linked. |

The current CMake integration disables SDK XMP with `qDNGUseXMP=0`,
`qDNGXMPFiles=0`, and `qDNGXMPDocOps=0`, and excludes the SDK XMP source files.
JPEG support is resolved through `find_package(JPEG)`. If SDK XMP or the
SDK-bundled libjpeg is enabled later, inventory those exact sources and licenses
as well. Merely shipping additional SDK folders still requires checking their
redistribution terms even if they are not compiled.

## Publishing source and binaries

For a release of this integration:

1. Record the exact DT commit, modified LibRaw commit, SDK archive/build, codec
   versions, build configuration, and dependency sources.
2. Preserve upstream copyright notices, this attribution, and full applicable
   license texts. Mark modified GPL source as required by its license.
3. Publish the Corresponding Source appropriate to the actual binary under the
   applicable GPL/LGPL requirements, including integration changes and necessary
   build scripts. An unmodified upstream source link does not cover our changes.
4. Keep SDK Software and SDK Documentation separate in the inventory. Do not
   assume unrestricted copying or modification of Adobe's documentation or
   automatically publish every file in the SDK archive.
5. Include notices for the libraries actually bundled, including dynamically
   collected dependencies. Check license/notice handling in the finished
   artifact rather than inferring it from a successful build.
6. Describe the fork as an unofficial modified build. Avoid claims of Adobe
   or darktable endorsement, and review the SDK's commercial-product
   indemnification provision if distributing commercially.
7. Record separate redistribution permission for test photographs and sample
   files. SDK or application licenses do not automatically license those images.

The repository's [AppImage build script](packaging/AppImage/build-dngio.sh)
includes DT's GPL, this document, the third-party notices, and the collected
license texts. See the [build instructions](doc/dngio/build.md). An audit of
all dynamically bundled dependencies and the combined-license assessment
remain release preparation work; this document does not certify every binary
configuration as compliant.

The nine test images listed in
[their checksum manifest](tests/dngio/fixtures/SHA256SUMS) are dedicated by
BR1py under [CC0 1.0 Universal](tests/dngio/fixtures/LICENSE.md). They are stored
with Git LFS; source cloning and CI must fetch the LFS objects.

## Primary references

- [Adobe DNG downloads and specification patent license](https://www.adobe.com/support/downloads/dng/dng_sdk.html).
- Local authoritative texts: [GPLv3](LICENSE), [Adobe SDK agreement](licenses/Adobe-DNG-SDK-LICENSE.txt),
  and [LibRaw notices](src/external/LibRaw/COPYRIGHT).

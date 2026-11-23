# Third-party notices

This is an unofficial modified build of darktable 5.6.2. It is not produced,
endorsed, or supported by the darktable project or Adobe Inc.

## darktable

darktable is free software distributed under the GNU General Public License,
version 3. The complete license text is included as `GPL-3.0.txt` alongside
this file. Source code for this modified build, including the corresponding
build files, must be made available when this binary is distributed.

## Adobe DNG technology and DNG SDK

This product includes DNG technology under license by Adobe.

Portions of this build use the Adobe DNG SDK. The complete DNG SDK License
Agreement is included as `Adobe-DNG-SDK-LICENSE.txt`. Adobe and the DNG logo
are trademarks or registered trademarks of Adobe Inc. This build does not use
those trademarks to indicate Adobe endorsement.

The Adobe DNG SDK source code is not covered by darktable's GPL notice; it is
provided under Adobe's separate DNG SDK License Agreement. Adobe copyright and
other notices must not be removed from human-readable SDK source copies.

## JPEG XL and other bundled components

The Adobe DNG SDK and this build use JPEG XL, Brotli, Highway, skcms, and other
third-party software. Their license texts are included in the `licenses`
directory. Copyright and license records for shared libraries collected from
the build system are installed under `/usr/share/doc` inside the AppImage.

No warranty is provided. Refer to each included license for its applicable
terms, conditions, disclaimers, and attribution requirements.

## Third-party DNG test images

Two Apple DNG test fixtures come from [raw.pixls.us](https://raw.pixls.us/).
The archive's [per-file catalog](https://raw.pixls.us/json/getrepository.php)
marks both images as **CC0 1.0 Universal**, permitting copying, modification,
and redistribution, including commercial use, without attribution requirements.
The dedication is by the original contributors, not by this fork's author.
See the [CC0 legal text](https://creativecommons.org/publicdomain/zero/1.0/legalcode).

| Fixture | Original source | Original SHA-256 |
| --- | --- | --- |
| `apple-proraw-ljpeg-linear-12bit.dng` | [iPhone 12 Pro, record 4264](https://raw.pixls.us/getfile.php/4264/nice/) | `e91e77a4533ed7cce551d83330676ea5c47dd5e55fb38adda7819366afdbdfc2` |
| `apple-mosaic-ljpeg-16bit.dng` | [iPhone 6s Plus, record 915](https://raw.pixls.us/getfile.php/915/nice/) | `845fee4f09c832af3728778737f5ae9b99124debb821b6e2066b94aed1daa91c` |

The files have been renamed and GPS metadata removed. Compressed image data
is unchanged. Checksums for the distributed copies are recorded in
`tests/dngio/fixtures/SHA256SUMS`. These image licenses do not change the
licenses of application code or other bundled components. No endorsement by
Apple, raw.pixls.us, or the photographers is implied.

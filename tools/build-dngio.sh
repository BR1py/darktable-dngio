#!/usr/bin/env bash
set -euo pipefail
source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${BUILD_DIR:-$source_dir/build-dngio}"
install_dir="${INSTALL_DIR:-$source_dir/install-dngio}"
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN)}"
dngio="${DNGIO:-ON}"
deps_dir="$build_dir/deps"
codec_args=()

if [[ "$dngio" == ON ]]; then
  patch_file="$source_dir/patches/libraw/dngsdk-glue.patch"
  if ! git -C "$source_dir/src/external/LibRaw" apply --reverse --check "$patch_file" 2>/dev/null; then
    git -C "$source_dir/src/external/LibRaw" apply --check "$patch_file"
    git -C "$source_dir/src/external/LibRaw" apply "$patch_file"
  fi
  cmake -S "$source_dir/src/external/adobe-dng-sdk/libjxl" -B "$build_dir/libjxl" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$deps_dir" -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DJPEGXL_ENABLE_TOOLS=OFF \
    -DJPEGXL_ENABLE_EXAMPLES=OFF -DJPEGXL_ENABLE_BENCHMARK=OFF \
    -DJPEGXL_ENABLE_JPEGLI=OFF -DJPEGXL_ENABLE_SJPEG=OFF -DJPEGXL_ENABLE_OPENEXR=OFF \
    -DJPEGXL_ENABLE_DOXYGEN=OFF -DJPEGXL_ENABLE_MANPAGES=OFF -DJPEGXL_ENABLE_JNI=OFF \
    -DJPEGXL_ENABLE_SKCMS=ON
  cmake --build "$build_dir/libjxl" --parallel "$jobs"
  cmake --install "$build_dir/libjxl"
  codec_args=(-DJXL_INCLUDE_DIR="$deps_dir/include" -DJXL_LIBRARY="$deps_dir/lib/libjxl.a"
              -DJXL_THREADS_LIBRARY="$deps_dir/lib/libjxl_threads.a")
fi

export PKG_CONFIG_PATH="$deps_dir/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
cmake -S "$source_dir" -B "$build_dir/darktable" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$install_dir" \
  -DCMAKE_PREFIX_PATH="$deps_dir" -DUSE_DNGIO="$dngio" \
  -DUSE_LIBRAW=ON -DDONT_USE_INTERNAL_LIBRAW=OFF -DBUILD_TESTING=ON "${codec_args[@]}" "$@"
cmake --build "$build_dir/darktable" --parallel "$jobs"
ctest --test-dir "$build_dir/darktable" --output-on-failure --parallel "$jobs"
cmake --install "$build_dir/darktable"

#!/usr/bin/env bash

# Install the Ubuntu/Debian packages needed to build darktable dngio.
# Run with: sudo tools/install-dngio-build-deps.sh

set -euo pipefail

if [[ ${EUID} -ne 0 ]]; then
  echo "error: run this script as root (for example: sudo $0)" >&2
  exit 1
fi

if ! command -v apt-get >/dev/null 2>&1; then
  echo "error: this script requires an Ubuntu/Debian system with apt-get" >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install --yes --no-install-recommends \
  build-essential \
  cmake \
  libcmocka-dev \
  exiftool \
  git-lfs \
  desktop-file-utils \
  gettext \
  git \
  intltool \
  ninja-build \
  pkg-config \
  libarchive-dev \
  libatk1.0-dev \
  libavif-dev \
  libcairo2-dev \
  libcolord-dev \
  libcolord-gtk-dev \
  libcups2-dev \
  libcurl4-gnutls-dev \
  libexiv2-dev \
  libgdk-pixbuf-2.0-dev \
  libglib2.0-dev \
  libgmic-dev \
  libgphoto2-dev \
  libgraphicsmagick1-dev \
  libgtk-3-dev \
  libheif-dev \
  libjpeg-dev \
  libjson-glib-dev \
  libjxl-dev \
  liblcms2-dev \
  liblensfun-dev \
  liblua5.4-dev \
  libopenexr-dev \
  libopenjp2-7-dev \
  libosmgpsmap-1.0-dev \
  libpango1.0-dev \
  libpng-dev \
  libportmidi-dev \
  libpotrace-dev \
  libpugixml-dev \
  libraw-dev \
  librsvg2-dev \
  libsdl2-dev \
  libsecret-1-dev \
  libsqlite3-dev \
  libtiff-dev \
  libwebp-dev \
  libx11-dev \
  libxml2-dev \
  libxml2-utils \
  xsltproc

echo "darktable build dependencies installed successfully."

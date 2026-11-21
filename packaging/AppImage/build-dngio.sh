#!/usr/bin/env bash
set -euo pipefail

source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
install_dir="${INSTALL_DIR:-$source_dir/install-dngio}"
appdir="${APPDIR:-$source_dir/build-appimage/AppDir}"
output_dir="${OUTPUT_DIR:-$source_dir/dist}"
tools_dir="${TOOLS_DIR:-$source_dir/build-appimage/tools}"
arch="$(uname -m)"

if [[ "$arch" != "x86_64" && "$arch" != "aarch64" ]]; then
  echo "Unsupported AppImage architecture: $arch" >&2
  exit 1
fi

for command_name in desktop-file-validate wget; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "Required command not found: $command_name" >&2
    exit 1
  fi
done

if [[ ! -x "$install_dir/bin/darktable" ]]; then
  echo "Current darktable install not found at: $install_dir" >&2
  exit 1
fi

mkdir -p "$tools_dir" "$output_dir"
rm -rf "$appdir"
mkdir -p "$appdir/usr"
cp -a "$install_dir/." "$appdir/usr/"

legal_dir="$appdir/usr/share/doc/darktable-dngio"
mkdir -p "$legal_dir/licenses"
cp "$source_dir/THIRD_PARTY_NOTICES.md" "$source_dir/LICENSING.md" "$legal_dir/"
cp "$source_dir/LICENSE" "$legal_dir/GPL-3.0.txt"
cp "$source_dir/licenses/Adobe-DNG-SDK-LICENSE.txt" "$legal_dir/"
cp "$source_dir/licenses/"* "$legal_dir/licenses/"

desktop_file="$appdir/usr/share/applications/org.darktable.darktable.desktop"
sed -i 's|Exec=.*/darktable|Exec=darktable|' "$desktop_file"

# Modules below are loaded dynamically and cannot all be discovered by ldd.
multiarch="$(gcc -print-multiarch 2>/dev/null || true)"
if [[ -n "$multiarch" ]]; then
  if [[ -d "/usr/lib/$multiarch/libgphoto2" ]]; then
    mkdir -p "$appdir/usr/lib/libgphoto2"
    cp -a "/usr/lib/$multiarch/libgphoto2/." "$appdir/usr/lib/libgphoto2/"
  fi
  if [[ -d "/usr/lib/$multiarch/libgphoto2_port" ]]; then
    mkdir -p "$appdir/usr/lib/libgphoto2_port"
    cp -a "/usr/lib/$multiarch/libgphoto2_port/." "$appdir/usr/lib/libgphoto2_port/"
  fi
  if [[ -d "/usr/lib/$multiarch/gio/modules" ]]; then
    mkdir -p "$appdir/usr/lib/gio/modules"
    cp -a "/usr/lib/$multiarch/gio/modules/." "$appdir/usr/lib/gio/modules/"
  fi
fi

linuxdeploy="$tools_dir/linuxdeploy-$arch.AppImage"
gtk_plugin="$tools_dir/linuxdeploy-plugin-gtk.sh"

if [[ ! -x "$linuxdeploy" ]]; then
  wget -q -O "$linuxdeploy" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$arch.AppImage"
  chmod +x "$linuxdeploy"
fi
if [[ ! -x "$gtk_plugin" ]]; then
  wget -q -O "$gtk_plugin" \
    "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
  chmod +x "$gtk_plugin"
fi

export APPIMAGE_EXTRACT_AND_RUN=1
export DEPLOY_GTK_VERSION=3
export AS_VALIDATE_NONET=1
export LINUXDEPLOY_PLUGIN_GTK="$gtk_plugin"
version="${APPIMAGE_VERSION:-$("$install_dir/bin/darktable-cli" --version | awk 'NR == 1 {print $2}')}"
export LINUXDEPLOY_OUTPUT_VERSION="$version"
export OUTPUT="${APPIMAGE_OUTPUT:-$output_dir/darktable-$version-$arch.AppImage}"

deploy_args=(
  --appdir "$appdir"
  --plugin gtk
  --custom-apprun "$source_dir/packaging/AppImage/AppRun"
)

for module_dir in \
  "$appdir/usr/lib/darktable/plugins" \
  "$appdir/usr/lib/darktable/plugins/imageio/format" \
  "$appdir/usr/lib/darktable/plugins/imageio/storage" \
  "$appdir/usr/lib/darktable/plugins/lighttable" \
  "$appdir/usr/lib/darktable/views"; do
  if [[ -d "$module_dir" ]]; then
    deploy_args+=(--deploy-deps-only "$module_dir")
  fi
done

"$linuxdeploy" "${deploy_args[@]}" --output appimage
chmod +x "$OUTPUT"
echo "Created $OUTPUT"

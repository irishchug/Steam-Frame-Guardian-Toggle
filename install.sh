#!/usr/bin/env bash
# Install the guardian toggle for the current user.
# Run on the headset, or from a PC: ./install.sh [ssh-host]
set -e
src=$(cd "$(dirname "$0")" && pwd)

if [ -n "$1" ]; then
    # Copy this directory to the host and run the installer there.
    ssh "$1" 'rm -rf /tmp/guardian-toggle-src && mkdir -p /tmp/guardian-toggle-src'
    scp -q -r "$src/install.sh" "$src/uninstall.sh" "$src/src" "$src/assets" "$src/third_party" \
        "$1:/tmp/guardian-toggle-src/"
    ssh "$1" 'bash /tmp/guardian-toggle-src/install.sh && rm -rf /tmp/guardian-toggle-src'
    exit
fi

vr_lib=/opt/steamvr/bin/linuxarm64
data=${XDG_DATA_HOME:-$HOME/.local/share}
install_dir=$data/guardian-toggle
apps_dir=$data/applications
unit_dir=${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user

if ! command -v cc >/dev/null; then
    echo "A C compiler (cc) is required to build the dashboard bar service." >&2
    exit 1
fi

mkdir -p "$install_dir" "$apps_dir" "$unit_dir"

# Menu entry, plus a copy of the uninstaller so removal needs no checkout.
install -m 755 "$src/src/toggle.sh" "$install_dir/toggle.sh"
install -m 755 "$src/uninstall.sh" "$install_dir/uninstall.sh"
sed "s|@INSTALL_DIR@|$install_dir|" "$src/src/guardian-toggle.desktop" > "$apps_dir/guardian-toggle.desktop"

# Dashboard bar service, linked against the headset's own OpenVR library.
cc -O2 -o "$install_dir/guardian-bar" "$src/src/guardian-bar.c" \
    -isystem "$src/third_party/openvr" -L"$vr_lib" -lopenvr_api -Wl,-rpath,"$vr_lib"
install -m 644 "$src/assets/icon.png" "$src/assets/notice-hidden.png" "$src/assets/notice-shown.png" "$install_dir/"
sed "s|@INSTALL_DIR@|$install_dir|" "$src/src/guardian-toggle.service" > "$unit_dir/guardian-toggle.service"

systemctl --user daemon-reload
systemctl --user enable guardian-toggle.service
if systemctl --user is-active --quiet steamvr.service; then
    systemctl --user restart guardian-toggle.service
fi
echo "Installed to $install_dir"

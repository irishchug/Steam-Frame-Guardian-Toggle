#!/usr/bin/env bash
# Install the guardian toggle for the current user.
# Run on the headset, or from a PC: ./install.sh [ssh-host]
set -e
src=$(cd "$(dirname "$0")" && pwd)

if [ -n "$1" ]; then
    # Copy this directory to the host and run the installer there.
    ssh "$1" 'rm -rf /tmp/guardian-toggle-src && mkdir -p /tmp/guardian-toggle-src'
    scp -q -r "$src/install.sh" "$src/uninstall.sh" "$src/src" "$1:/tmp/guardian-toggle-src/"
    ssh "$1" 'bash /tmp/guardian-toggle-src/install.sh && rm -rf /tmp/guardian-toggle-src'
    exit
fi

install_dir=${XDG_DATA_HOME:-$HOME/.local/share}/guardian-toggle
apps_dir=${XDG_DATA_HOME:-$HOME/.local/share}/applications

mkdir -p "$install_dir" "$apps_dir"
install -m 755 "$src/src/toggle.sh" "$install_dir/toggle.sh"
sed "s|@INSTALL_DIR@|$install_dir|" "$src/src/guardian-toggle.desktop" > "$apps_dir/guardian-toggle.desktop"
echo "Installed to $install_dir"

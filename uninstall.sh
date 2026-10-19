#!/usr/bin/env bash
# Remove the guardian toggle. Restores the walls first if they are hidden.
set -e
data=${XDG_DATA_HOME:-$HOME/.local/share}
state=${XDG_STATE_HOME:-$HOME/.local/state}/guardian-toggle
unit=${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/guardian-toggle.service

if [ -f "$unit" ]; then
    systemctl --user disable --now guardian-toggle.service || true
    rm -f "$unit"
    systemctl --user daemon-reload
fi

vrcmd=/opt/steamvr/bin/linuxarm64/vrcmd
key=collisionBounds.CollisionBoundsFadeDistance
if [ -x "$vrcmd" ]; then
    export LD_LIBRARY_PATH=/opt/steamvr/bin/linuxarm64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
    cur=$("$vrcmd" --settings-float "$key" 2>/dev/null | sed -n 's/^.*=//p' | head -1)
    if awk "BEGIN{exit !(\"${cur:-1}\" + 0 == 0)}"; then
        "$vrcmd" --set-settings-float "$key" "$(cat "$state/fade-distance" 2>/dev/null || echo 0.7)" || true
    fi
fi

rm -rf "$data/guardian-toggle" "$state" "$data/applications/guardian-toggle.desktop"
echo "Removed."

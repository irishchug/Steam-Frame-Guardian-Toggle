#!/usr/bin/env bash
# Toggle the guardian walls by setting the wall fade distance to 0 (hidden)
# or back to the user's previous value (default 0.7).
set -e
export LD_LIBRARY_PATH=/opt/steamvr/bin/linuxarm64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
VRCMD=/opt/steamvr/bin/linuxarm64/vrcmd
KEY=collisionBounds.CollisionBoundsFadeDistance
STATE_DIR=${XDG_STATE_HOME:-$HOME/.local/state}/guardian-toggle
SAVED=$STATE_DIR/fade-distance
DEFAULT=0.7

cur=$("$VRCMD" --settings-float "$KEY" | sed -n 's/^.*=//p' | head -1)

if awk "BEGIN{exit !($cur > 0)}"; then
    # Walls are visible: remember the current distance, then hide them.
    mkdir -p "$STATE_DIR"
    echo "$cur" > "$SAVED"
    new=0
else
    # Walls are hidden: restore the remembered distance.
    new=$(cat "$SAVED" 2>/dev/null || true)
    awk "BEGIN{exit !(\"$new\" + 0 > 0)}" || new=$DEFAULT
fi

"$VRCMD" --set-settings-float "$KEY" "$new"

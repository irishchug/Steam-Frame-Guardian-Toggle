# Steam Frame Guardian Toggle

Show or hide the guardian (chaperone) walls on a Steam Frame with one tap from the
dashboard's bottom bar.

> **Warning:** the guardian exists to keep you from walking into things. Hiding
> the walls removes that warning. Only use this in a space you know is clear.
> The floor outline stays visible while the walls are hidden.

## What it does

- Adds a **Guardian Walls** icon to the bottom bar of the SteamVR dashboard. Tapping it
  hides or shows the walls, flashes a short notice, and returns to the Steam window.
- Walls are always shown again when SteamVR starts, so hiding them only lasts for the
  current session.
- **Add/Remove Guardian Bar Icon** in the app list takes the icon off the bar (turning
  the walls back on if they are hidden), or puts it back. The icon also returns on its
  own the next time SteamVR starts.
- Over SSH, `~/.local/share/guardian-toggle/toggle.sh` toggles the walls from the
  command line.
- Restores your own wall fade distance when showing the walls (default `0.7`).

<p>
  <img src="assets/notice-hidden.png" alt="Notice: guardian walls hidden" width="320">
  &nbsp;
  <img src="assets/notice-shown.png" alt="Notice: guardian walls shown" width="320">
</p>

The notice shown after each tap. The bar icon is the drawing on the "hidden" notice, in white.

The walls are controlled by the `collisionBounds.CollisionBoundsFadeDistance` SteamVR
setting: `0` hides them. Nothing under `/opt/steamvr` is modified.

The bar icon is served by a small background program that starts and stops with
SteamVR. It sleeps between checks and uses about 0.2% CPU and 15 MB of memory.

Tested on a Steam Frame running SteamOS, both standalone and while streaming from a PC.
SteamVR updates may change behavior.

## Install

1. Enable Developer Mode and set a password on the headset by following Valve's
   [Steam Frame setup guide](https://partner.steamgames.com/doc/steamframe/setup#Developer_Mode).
2. SSH into the headset (`ssh steamos@frame`) and run:

   ```bash
   git clone https://github.com/irishchug/Steam-Frame-Guardian-Toggle.git
   cd Steam-Frame-Guardian-Toggle
   ./install.sh
   ```

   The installer builds the bar program on the headset with its C compiler (`cc`, part
   of SteamOS).

To update, run `git pull` in the same folder, then `./install.sh` again.

You can also install from a PC with this repository checked out, using a shell with
`ssh` and `scp` (Linux, macOS, or Git Bash on Windows):
`./install.sh steamos@frame`.

## Uninstall

While SSH'd into the headset:

```bash
~/.local/share/guardian-toggle/uninstall.sh
```

This stops and removes the background service, the bar icon, the app list entry and
saved state, and turns the walls back on if they are hidden.

To keep it installed but stop the bar icon from coming back each time SteamVR starts
(**Add/Remove Guardian Bar Icon** still works for the current session):

```bash
systemctl --user disable --now guardian-toggle.service
```

Turn it back on with `systemctl --user enable --now guardian-toggle.service`.

## License

MIT. `third_party/openvr/openvr_capi.h` is from Valve's
[OpenVR SDK](https://github.com/ValveSoftware/openvr) and is covered by its own
BSD-3-Clause license in `third_party/openvr/LICENSE`.

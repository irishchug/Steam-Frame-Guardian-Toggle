# Steam Frame Guardian Toggle

Show or hide the guardian (chaperone) walls on a Steam Frame with a single tap.

> **Warning:** the guardian exists to keep you from walking into things. Hiding
> the walls removes that warning. Only use this in a space you know is clear.
> The floor outline stays visible while the walls are hidden.

## How it works

The walls fade in based on the `collisionBounds.CollisionBoundsFadeDistance`
SteamVR setting. The toggle sets it to `0` (walls never appear) and restores
your previous value (default `0.7`) when toggled back. It uses the `vrcmd`
tool that ships with SteamVR, so nothing in `/opt/steamvr` is modified.

Tested on a Steam Frame running SteamOS (Linux 6.18). SteamVR updates may change
behavior.

## Install

Enable SSH on the headset, then from a PC:

```bash
./install.sh <ssh-host>
```

Or, with this folder on the headset:

```bash
./install.sh
```

This adds a **Guardian Walls Toggle** entry to the headset's app list.

## Uninstall

```bash
./uninstall.sh
```

Run it on the headset. If the walls are currently hidden, they are restored first.

## License

MIT

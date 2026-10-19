#!/usr/bin/env bash
# Add the guardian toggle icon to the dashboard bar, or remove it if it is there.
if systemctl --user is-active --quiet guardian-toggle.service; then
    systemctl --user stop guardian-toggle.service
else
    systemctl --user start guardian-toggle.service
fi

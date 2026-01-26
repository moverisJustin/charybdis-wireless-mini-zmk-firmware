import json
from pathlib import Path

# === CONFIGURATION ===
board = "nice_nano"
dongle_board = "xiao_ble"

# automatically find all *.keymap filenames under ../config/keymap
keymap_dir = Path(__file__).parent.parent / "config" / "keymap"
keymaps = sorted(p.stem for p in keymap_dir.glob("*.keymap"))

# Map each format to the shields it should build
# All shields now live in charybdis_bt directory
format_shields = {
    # BT mode: right acts as central (standalone)
    "bt": ["charybdis_left", "charybdis_right_standalone"],
    # Dongle mode: right acts as peripheral, dongle is central
    "dongle": ["charybdis_left", "dongle_charybdis_right", "dongle_prospector"],
    "reset": ["settings_reset"],
}

# Map shields to their boards (most use nice_nano, dongle uses xiao)
shield_boards = {
    "charybdis_left": "nice_nano",
    "charybdis_right_standalone": "nice_nano",
    "dongle_charybdis_right": "nice_nano",
    "dongle_prospector": "xiao_ble",
    "settings_reset": "nice_nano",
}

groups = []
for keymap in keymaps:
    for fmt in ["bt", "dongle"]:
        groups.append({
            "keymap": keymap,
            "format": fmt,
            "name": f"{keymap}-{fmt}",
            "board": board,
        })

# single reset entry
groups.append({
    "keymap": "default",
    "format": "reset",
    "name": "reset-nanov2",
    "board": board,
})

# Dump matrix as compact JSON (GitHub expects it this way)
print(json.dumps(groups))

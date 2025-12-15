import json
from pathlib import Path

# === CONFIGURATION ===
board_bt = "nice_nano"          # Board for BT keyboard halves
board_dongle = "xiao_ble"       # Board for dongle (beekeeb hardware)

# automatically find all *.keymap filenames under ../config/keymap
keymap_dir = Path(__file__).parent.parent / "config" / "keymap"
keymaps = sorted(p.stem for p in keymap_dir.glob("*.keymap"))

# Map each format to the shields it should build and which board to use
format_config = {
    "bt": {
        "shields": ["charybdis_left", "charybdis_right"],
        "board": board_bt,
    },
    "dongle": {
        "shields": ["charybdis_left", "charybdis_right", "charybdis_dongle"],
        "board": board_bt,  # Left and right halves use nice_nano
        "dongle_board": board_dongle,  # Dongle itself uses xiao_ble
    },
    "reset": {
        "shields": ["settings_reset"],
        "board": board_bt,
    },
}

groups = []
for keymap in keymaps:
    for fmt in ["bt", "dongle"]:
        groups.append({
            "keymap": keymap,
            "format": fmt,
            "name": f"{keymap}-{fmt}",
            "board": format_config[fmt]["board"],
            "dongle_board": format_config[fmt].get("dongle_board"),
        })

# single reset entry
groups.append({
    "keymap": "default",
    "format": "reset",
    "name": "reset-nanov2",
    "board": board_bt,
})

# Dump matrix as compact JSON (GitHub expects it this way)
print(json.dumps(groups))

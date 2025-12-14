import json
from pathlib import Path

# === CONFIGURATION ===
board_nice_nano = "nice_nano"
board_xiao = "xiao_ble"
# automatically find all *.keymap filenames under ../config/keymap
keymap_dir = Path(__file__).parent.parent / "config" / "keymap"
keymaps = sorted(p.stem for p in keymap_dir.glob("*.keymap"))

groups = []
for keymap in keymaps:
    # BT format: left and right halves with nice_nano
    groups.append({
        "keymap": keymap,
        "format": "bt",
        "name": f"{keymap}-bt",
        "board": board_nice_nano,
    })
    
    # Dongle format: left and right halves with nice_nano (peripherals)
    groups.append({
        "keymap": keymap,
        "format": "dongle",
        "name": f"{keymap}-dongle",
        "board": board_nice_nano,
    })
    
    # Dongle format: dongle shield with XIAO board (central)
    groups.append({
        "keymap": keymap,
        "format": "dongle-xiao",
        "name": f"{keymap}-dongle-xiao",
        "board": board_xiao,
    })

# Reset firmware entries
groups.append({
    "keymap": "default",
    "format": "reset",
    "name": "reset-nanov2",
    "board": board_nice_nano,
})

groups.append({
    "keymap": "default",
    "format": "reset-xiao",
    "name": "reset-xiao",
    "board": board_xiao,
})

# Dump matrix as compact JSON (GitHub expects it this way)
print(json.dumps(groups))

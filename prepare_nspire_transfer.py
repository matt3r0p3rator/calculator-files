#!/usr/bin/env python3
"""
prepare_nspire_transfer.py

Copies the Angband lib files needed by the TI-Nspire port into a staging
directory, appending ".tns" to every filename so the TI-Nspire Computer
Link software will accept them.

Usage:
    python3 prepare_nspire_transfer.py

Output:
    nspire_transfer/       <-- copy this whole folder hierarchy to the
                               calculator using TI-Nspire Student Software

On-calculator target layout (relative to /documents/):
    angband/lib/gamedata/monster.txt.tns
    angband/lib/gamedata/object.txt.tns
    ...
    angband/lib/screens/  ...
    angband/lib/help/     ...
    angband/lib/customize/...
    angband/lib/user/save/   (empty folder -- create manually)

Then copy angband.tns to /documents/  (or anywhere convenient).

The app itself (via nspire_fopen) will find e.g. "monster.txt" by
transparently trying "monster.txt.tns" when the plain name isn't found.
"""

import os
import shutil
from pathlib import Path

# ---- configuration --------------------------------------------------------
ANGBAND_LIB = Path(__file__).parent / "angband" / "lib"
OUT_DIR      = Path(__file__).parent / "nspire_transfer" / "angband" / "lib"

# Only these subdirectories are needed -- tiles/fonts/sounds/icons are skipped
NEEDED_DIRS = [
    "gamedata",
    "screens",
    "help",
    "customize",
]

# Empty directories that must exist on the calculator (the app writes into them)
EMPTY_DIRS = [
    "user/save",
    "user/scores",
    "user/panic",
    "user/archive",
]
# ---------------------------------------------------------------------------

def main():
    if OUT_DIR.exists():
        shutil.rmtree(OUT_DIR)

    count = 0
    for sub in NEEDED_DIRS:
        src_dir = ANGBAND_LIB / sub
        dst_dir = OUT_DIR / sub
        dst_dir.mkdir(parents=True, exist_ok=True)
        for src_file in src_dir.iterdir():
            if src_file.is_file():
                dst_file = dst_dir / (src_file.name + ".tns")
                shutil.copy2(src_file, dst_file)
                count += 1

    for rel in EMPTY_DIRS:
        (OUT_DIR / rel).mkdir(parents=True, exist_ok=True)
        # Place a zero-byte placeholder so the folder is visible after transfer
        placeholder = OUT_DIR / rel / ".keep.tns"
        placeholder.touch()

    print(f"Staged {count} data files → {OUT_DIR.parent.parent}")
    print()
    print("Transfer instructions")
    print("=====================")
    print("1. Open TI-Nspire Student Software (or Computer Link).")
    print("2. Connect the calculator.")
    print('3. In the software, create these folders under "My Documents":')
    print("     angband/")
    print("     angband/lib/")
    for sub in NEEDED_DIRS:
        print(f"     angband/lib/{sub}/")
    for rel in EMPTY_DIRS:
        print(f"     angband/lib/{rel}/")
    print()
    print("4. Transfer every file inside nspire_transfer/angband/lib/ to the")
    print("   matching folder on the calculator (drag & drop in the software).")
    print()
    print("5. Transfer  angband/src/angband.tns  to  angband/  on the calculator")
    print("   (or to the root of My Documents -- anywhere is fine).")
    print()
    print("6. Run angband.tns via the Ndless home screen.")

if __name__ == "__main__":
    main()

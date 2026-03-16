# Angband — TI-Nspire CX II Keybindings

## Modifiers

| Key      | Effect                                                       |
|----------|--------------------------------------------------------------|
| `shift`  | Produces the shifted symbol (see tables below)               |
| `ctrl`   | Combined with a letter produces the control character (^A–^Z)|
| `esc`    | Cancel / back                                                |
| `menu`   | Also mapped to Escape                                        |

---

## Special Keys

| Nspire Key   | Sends        | Notes                              |
|--------------|--------------|------------------------------------|
| `enter`      | Return (CR)  | Confirm                            |
| `ret`        | Return (CR)  | Also confirm                       |
| `del`        | Backspace    |                                    |
| `tab`        | Tab          |                                    |
| `space`      | Space        |                                    |
| `home`       | Home         | (Angband HOME key)                 |
| `esc` / `menu` | Escape     |                                    |
| `?` (ques)   | `?`          | Help / look — bypasses OS menu     |
| `doc`        | `?`          | Alternative help key               |

---

## Movement

Angband uses the "roguelike numpad" convention.  All eight compass
directions are reachable from the touchpad.

| Nspire Input             | Sends        | Direction        |
|--------------------------|--------------|------------------|
| Touchpad ↑               | `8`          | North            |
| Touchpad ↓               | `2`          | South            |
| Touchpad ←               | `4`          | West             |
| Touchpad →               | `6`          | East             |
| Touchpad ↗ (up-right)    | `9`          | North-East       |
| Touchpad ↘ (right-down)  | `3`          | South-East       |
| Touchpad ↙ (down-left)   | `1`          | South-West       |
| Touchpad ↖ (left-up)     | `7`          | North-West       |
| Touchpad click           | `5`          | Wait in place    |
| Arrow keys (↑↓←→)        | Arrow codes  | Navigate menus   |

> **Tip:** The arrow keys work in menus and lists.  Use the touchpad
> directions for in-dungeon movement.

---

## Digits & Shifted Symbols

| Key  | Normal | Shift |
|------|--------|-------|
| `0`  | `0`    | `)`   |
| `1`  | `1`    | `!`   |
| `2`  | `2`    | `@`   |
| `3`  | `3`    | `#`   |
| `4`  | `4`    | `$`   |
| `5`  | `5`    | `%`   |
| `6`  | `6`    | `^`   |
| `7`  | `7`    | `&`   |
| `8`  | `8`    | `*`   |
| `9`  | `9`    | `(`   |

---

## Punctuation & Symbols

All printable ASCII (32–126) is accessible.

| Nspire Key    | Normal | Shift | Notes                                |
|---------------|--------|-------|--------------------------------------|
| `.` (period)  | `.`    | `>`   |                                      |
| `,` (comma)   | `,`    | `<`   |                                      |
| `-` (minus)   | `-`    | `_`   |                                      |
| `+` (plus)    | `+`    | `=`   |                                      |
| `(` (LP)      | `(`    | `[`   |                                      |
| `)` (RP)      | `)`    | `]`   |                                      |
| `÷` (divide)  | `/`    | `/`   |                                      |
| `×` (multiply)| `*`    | `\|`  |                                      |
| `=` (equals)  | `=`    | `+`   |                                      |
| `^` (EXP)     | `^`    | `~`   | `~` opens knowledge browser          |
| `?` (ques)    | `?`    | `?`   |                                      |
| `:` (colon)   | `:`    | `;`   |                                      |
| `"` (quote)   | `"`    | `'`   |                                      |
| `'` (apostrophe)| `'`  | `` ` ``|                                    |
| `\|` (bar)    | `\|`   | `\`   |                                      |
| `>` (GTHAN)   | `>`    | `}`   |                                      |
| `<` (LTHAN)   | `<`    | `{`   |                                      |
| `(-)` (negative sign) | `-` | `_` | Duplicate of minus key           |

---

## Key Mapping Summary for Common Angband Commands

| Angband command            | Key(s) to press                     |
|----------------------------|-------------------------------------|
| Move                       | Touchpad directions (see above)     |
| Wait a turn                | Touchpad click (`5`)                |
| Pick up item               | `g`                                 |
| Drop item                  | `d`                                 |
| Wear / wield               | `w`                                 |
| Take off                   | `t`                                 |
| Inventory                  | `i`                                 |
| Equipment                  | `e`                                 |
| Use item / use staff       | `u`                                 |
| Quaff potion               | `q`                                 |
| Read scroll                | `r`                                 |
| Zap wand                   | `z`                                 |
| Fire missile               | `f`                                 |
| Cast spell                 | `m`                                 |
| Look around                | `l`                                 |
| Help                       | `?` or `doc`                        |
| Knowledge browser          | shift+`^` (sends `~`)               |
| Character screen           | `C`                                 |
| Save & quit                | `Ctrl`+`X`                          |
| Save game                  | `Ctrl`+`S`                          |
| Stairs (go down/up)        | `>` (shift+`.`) / `<` (shift+`,`)   |
| Open door                  | `o`                                 |
| Close door                 | `c`                                 |
| Tunnel                     | `T` (shift+`t`)                     |
| Search                     | `s`                                 |
| Run                        | `.` then a direction                |
| Target                     | `*`  (shift+`8`)                    |
| Escape / cancel            | `esc`                               |

---

## File Layout on Calculator

```
/documents/angband/lib/        ← game data (all files end in .tns)
/documents/angband/lib/save/   ← save files (PLAYER.tns)
```

Transfer `angband.tns` and the entire `lib/` tree (with `.tns` on every
file) to `/documents/angband/` using TI-Nspire Computer Link software or
ndless-send.

# TI-Nspire Gauss-Jordan Elimination App

A native C++ application for TI-Nspire calculators (via Ndless) that performs step-by-step Gauss-Jordan elimination. It features a custom native-looking GUI, variable matrix sizes, and interactive UI for data entry.

## 🌟 Features
* **Variable Matrix Dimensions**: Support for dynamically sized matrices up to 6x6.
* **Interactive Grid Editing**: Navigate cells using arrow keys, supports negative numbers and decimals.
* **Step-by-Step Solver**: Shows accurate mathematical operations (e.g., `R2 -> R2 - (1.50 * R1)`) as it reduces the matrix to Row Echelon Form.
* **Native TI-OS Styling**: Uses a custom UI manager to perfectly mimic the calculator's native visual elements (blue title bars, light gray borders).

---

## 📂 File Structure

* `main.cpp` - The core application. Includes the state machine, user inputs, and the Gauss-Jordan fractional math logic.
* `ti_gui.hpp` - A reusable header-only GUI framework built on top of nSDL. It handles screen rendering, rectangles, fonts, colors, and text input parsing so `main.cpp` doesn't get cluttered with raw SDL commands.
* `Makefile` - The Ndless build script. Defines how `nspire-g++` compiles the code and strings it into `.elf` and ultimately `.tns` binaries using `genzehn` and `make-prg`.

---

## ⚙️ How It Works (Architecture)

The application runs on a continuous `while (running)` loop driven by `SDL_WaitEvent`. It employs a basic **State Machine** with three phases:

1. **`STATE_DIMS` (Dimensions Setup)**:
   * The user uses the directional pad to pick the total `Rows` and `Cols` of the matrix.
   * Pressing `[ENTER]` allocates a 2D vector for the grid and transitions to data entry.

2. **`STATE_INPUT` (Data Entry)**:
   * Dynamically spaces and scales a grid layout onto the screen based on the chosen dimensions.
   * `ti_gui.hpp` captures numeric keys, decimals, and minus signs and appends them to strings within a 2D `grid` array.
   * Pressing `[ENTER]` converts all strings to `double` arrays and feeds them into the solver algorithm.

3. **`STATE_SOLVING` (Iteration Viewer)**:
   * Uses the `solve_gauss_jordan()` function, which pre-computes every mathematical step and saves them as `Step` structs inside a `vector`.
   * The user scrolls left and right with the arrows to step backward and forward in time, displaying the transformation of the matrix at that specific step.

---

## 🛠 Prerequisites

To compile this project, you need:
1. **Ndless SDK**: Needs to be built and installed on your system.
2. **nSDL**: The SDL 1.2 port for Ndless (included in your Ndless compilation).
3. **Cross-Compilation Toolchain**: Provided by Ndless (includes `nspire-g++`, `nspire-ld`, `genzehn`, `make-prg`).

---

## 🚀 How to Build and Compile

Because this is a cross-compiled ARM binary for the calculator, you must build it using the specific `nspire-` toolchain rather than standard gcc.

1. **Open a terminal** and navigate to this project folder:
   ```bash
   cd /home/matt3r/calculator-files/gauss_app
   ```

2. **Export the Ndless Toolchain to your PATH**:
   Point your `PATH` variable to the Ndless `bin` and `toolchain/install/bin` directories. Since your workspace has Ndless parallel to this folder, run:
   ```bash
   export PATH="$PWD/../Ndless/ndless-sdk/toolchain/install/bin:$PWD/../Ndless/ndless-sdk/bin:$PATH"
   ```

3. **Compile the App**:
   Run the `make` command to clean old binaries and generate a fresh executable.
   ```bash
   make clean
   make
   ```
   *This process will compile `main.cpp` to an `.o` object, link it to `.elf`, patch it for OS compatibility using `genzehn`, and construct the final `gauss.tns` executable.*

4. **Transfer to Calculator**:
   * Use the TI Computer Link software or TiLP to drag and drop **`gauss.tns`** to your calculator.
   * Use the Ndless file browser on your host calculator to open the file!

---

## 🎮 Usage Instructions (On Calculator)
* **Arrows / Keypad (8,4,2,6)**: Move cursor / Change tabs.
* **[ 0-9 ], [ . ], [ (-) ]**: Type numbers into the selected matrix cell.
* **[ DEL ] / [ Backspace ]**: Erase a character.
* **[ ENTER ]**: Confirm and progress to the next screen.
* **[ ESC ]**: Go back to the previous screen or exit the application entirely.

/**
 * \file main-nspire.c
 * \brief Entry point for BrogueCE on the TI-Nspire CX II (Ndless)
 *
 * Replaces main.c for the Nspire build.  Provides:
 *   - All global variables normally defined in main.c
 *   - Nspire-specific initialisation (display, event queue, filesystem)
 *   - int main(void) that runs the game and returns
 *
 * On-calculator layout:
 *   /documents/brogue.tns           – the executable
 *   /documents/brogue/              – working directory
 *     BrogueHighScores.txt.tns      – high scores (auto-created)
 *     *.broguesave.tns              – save files
 *     *.broguerec.tns               – recordings
 *
 * Copyright (c) 2026 – Ndless/BrogueCE contributors
 * License: GNU Affero General Public License v3 or later
 */

#ifdef BROGUE_NSPIRE

#include <libndls.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "platform.h"
#include "Rogue.h"
#include "GlobalsBase.h"

#include "nspire/nspire-draw.h"
#include "nspire/nspire-event.h"

/* -----------------------------------------------------------------------
 * Globals normally defined in main.c
 * -------------------------------------------------------------------- */
struct brogueConsole currentConsole;

/* dataDirectory is used by tiles.c (SDL-only) for asset loading.
 * In the Nspire text-mode port it's set to "." (the working directory),
 * which is /documents/brogue/ after our chdir() below.             */
char dataDirectory[BROGUE_FILENAME_MAX] = ".";

boolean serverMode          = false;
boolean nonInteractivePlayback = false;
boolean hasGraphics         = false;
enum graphicsModes graphicsMode = TEXT_GRAPHICS;
boolean isCsvFormat         = false;

/* Declared in nspire-platform.c */
extern struct brogueConsole nspireConsole;

/* -----------------------------------------------------------------------
 * tryParseUint64 – also normally in main.c; used by MainMenu.c
 * -------------------------------------------------------------------- */
boolean tryParseUint64(char *str, uint64_t *num) {
    unsigned long long n;
    char buf[100];
    if (strlen(str)
        && sscanf(str, "%llu", &n)
        && sprintf(buf, "%llu", n)
        && !strcmp(buf, str)) {
        *num = (uint64_t)n;
        return true;
    } else {
        return false;
    }
}

/* -----------------------------------------------------------------------
 * init_filesystem: create /documents/brogue/ and chdir into it
 * -------------------------------------------------------------------- */
static void init_filesystem(void)
{
    const char *docs = get_documents_dir();  /* e.g. "/documents/" */

    char brogue_dir[512];
    snprintf(brogue_dir, sizeof(brogue_dir), "%sbrogue", docs);

    /* Create the directory if it doesn't exist yet. */
    struct stat st;
    if (stat(brogue_dir, &st) != 0) {
        mkdir(brogue_dir, 0755);
    }

    /* Change working directory so that relative file paths (save files,
     * high scores, keymap.txt) resolve under /documents/brogue/.       */
    chdir(brogue_dir);
}

/* -----------------------------------------------------------------------
 * Private 512 KB stack
 *
 * BrogueCE's titleMenu() alone allocates ~172 KB of local variables:
 *   flames[100][36][3]      ~21 KB   (signed short)
 *   colorSources[1136][4]    ~9 KB   (signed short)
 *   colors[100][36]         ~14 KB   (pointer array)
 *   mainShadowBuf           ~61 KB   (screenDisplayBuffer)
 *   flyoutShadowBuf         ~61 KB   (screenDisplayBuffer)
 *   ...other locals...
 *
 * The default Ndless OS stack (≈ 64 KB) overflows before the title screen
 * can draw a single pixel.  We allocate a private 512 KB stack in BSS
 * and switch to it at the very start of main().
 * -------------------------------------------------------------------- */
static unsigned char nspire_bigstack[512u * 1024u] __attribute__((aligned(8)));

/* Pointer to the top of the private stack; initialised at link time. */
static const unsigned char * const s_stack_top =
    nspire_bigstack + sizeof(nspire_bigstack);

/* -----------------------------------------------------------------------
 * brogue_run: all game logic, executed on the large private stack.
 * main() switches to nspire_bigstack and then calls this function.
 * -------------------------------------------------------------------- */
__attribute__((noreturn, noinline))
static void brogue_run(void)
{
    /* Require a colour-capable Nspire CX model. */
    if (!has_colors) {
        show_msgbox("Brogue",
                    "Brogue requires a TI-Nspire CX (colour screen).\n"
                    "Classic monochrome models are not supported.");
        exit(1);
    }

    /* Run at full clock speed for best performance. */
    set_cpu_speed(CPU_SPEED_150MHZ);

    /* Initialise framebuffer display. */
    nspire_video_init();
    nspire_clear(NSPIRE_BLACK);
    nspire_video_flush();

    /* Initialise event queue. */
    if (!nspire_event_init()) {
        nspire_draw_char(0, 0, 'E', NSPIRE_WHITE, NSPIRE_BLACK);
        nspire_video_flush();
        wait_key_pressed();
        exit(1);
    }

    /* Set up /documents/brogue/ as CWD. */
    init_filesystem();

    /* Wire up the Nspire console. */
    currentConsole = nspireConsole;

    /* Notify the platform of graphics mode (always text-only). */
    hasGraphics  = false;
    graphicsMode = TEXT_GRAPHICS;
    if (currentConsole.setGraphicsMode)
        currentConsole.setGraphicsMode(TEXT_GRAPHICS);

    /* Initialise game state (mirrors the setup in main.c). */
    rogue.nextGame             = NG_NOTHING;
    rogue.nextGamePath[0]      = '\0';
    rogue.nextGameSeed         = 0;
    rogue.mode                 = GAME_MODE_NORMAL;
    rogue.displayStealthRangeMode = false;
    rogue.trueColorMode        = false;

    /* Load any keymap remappings (file probably won't exist; that's fine). */
    loadKeymap();

    /* Run the game. */
    currentConsole.gameLoop();

    /* Restore normal CPU speed and leave cleanly. */
    set_cpu_speed(CPU_SPEED_90MHZ);
    exit(0);
}

/* -----------------------------------------------------------------------
 * main(): switch to private 512 KB stack, then run the game.
 *
 * The GCC prologue for main() (push {r4-r11, lr}) uses the OS stack
 * before our asm fires.  Those pushed values are orphaned when we
 * switch SP, but that is harmless because:
 *   a) brogue_run() is noreturn, so we never try to pop them, and
 *   b) exit() → __crt0_exit restores the ORIGINAL OS SP from the
 *      value saved in __crt0_savedsp before _start pushed anything.
 * -------------------------------------------------------------------- */
int main(void)
{
    /* Load the pre-computed stack-top pointer from BSS and transfer
     * control to brogue_run() on the new stack.                   */
    __asm__ volatile("ldr sp, %0" : : "m"(s_stack_top) : "memory");
    brogue_run();
    /* unreachable; suppresses -Wreturn-type */
    __builtin_unreachable();
}

#endif /* BROGUE_NSPIRE */

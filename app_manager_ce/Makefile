# ─────────────────────────────────────────────────────────────────────────────
# TI-84 CE C/C++ Toolchain Makefile
# Target: App Manager CE  (TI-84 CE / CE Python Edition via Artifice)
# ─────────────────────────────────────────────────────────────────────────────

NAME        = CHEMAPP
DESCRIPTION = "Chemistry Explorer CE"
COMPRESSED  = YES
ARCHIVED    = NO

LIBS = graphx keypadc

# Only compile files actually used — exclude dead math-app sources
SRCS = src/main.cpp src/chemistry.cpp

# -Oz = optimise for size; -std=c++17 for range-based loops
CFLAGS   = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz -std=c++17

include $(CEDEV)/meta/makefile.mk

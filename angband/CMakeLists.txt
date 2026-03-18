cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(Angband LANGUAGES C)

# ---------------------------------------------------------------------------
# TI-Nspire CX II (Ndless) build
#
# This project builds solely for the TI-Nspire CX II running Ndless.
# CMake is used only as a thin wrapper around src/Makefile.nspire.
#
# Prerequisites:
#   export PATH=$PATH:/path/to/ndless-sdk/bin
#
# Build:
#   cmake -B build && cmake --build build
# or simply:
#   cd src && make -f Makefile.nspire
# ---------------------------------------------------------------------------

find_program(MAKE_EXECUTABLE NAMES make gmake REQUIRED)

add_custom_target(angband ALL
    COMMAND ${MAKE_EXECUTABLE} -f Makefile.nspire
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
    COMMENT "Building Angband for TI-Nspire (Ndless)"
    VERBATIM
)

add_custom_target(clean-nspire
    COMMAND ${MAKE_EXECUTABLE} -f Makefile.nspire clean
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
    COMMENT "Cleaning Nspire build artefacts"
    VERBATIM
)

#!/usr/bin/env bash
# Source this file before building in the prepared managed Linux environment.
REELGLUE_TOOLS=/workspace/tooling
export PATH="$REELGLUE_TOOLS/cmake/data/bin:$REELGLUE_TOOLS/bin:$PATH"
export CPATH="$REELGLUE_TOOLS/sysroot/usr/include:$REELGLUE_TOOLS/sysroot/usr/include/freetype2${CPATH:+:$CPATH}"
export LIBRARY_PATH="$REELGLUE_TOOLS/sysroot/usr/lib/x86_64-linux-gnu${LIBRARY_PATH:+:$LIBRARY_PATH}"
export PKG_CONFIG_SYSROOT_DIR="$REELGLUE_TOOLS/sysroot"
export PKG_CONFIG_PATH="$REELGLUE_TOOLS/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:$REELGLUE_TOOLS/sysroot/usr/share/pkgconfig"
export CMAKE_BUILD_PARALLEL_LEVEL=3

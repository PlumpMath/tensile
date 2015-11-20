#!/bin/sh

CC="${1:-cc}"
CC_NAME=$(basename "${1##[[:space:]]*}")

TARGET_ARCH="$(${CC} -E test_arch.h 2>/dev/null)"
TARGET_OS="$(${CC} -E test_os.h  2>/dev/null)"
TARGET_CCVER="$(${CC} -E test_cc.h  2>/dev/null)"

TARGET_PLATFORM="${TARGET_OS:-unknown}_${TARGET_ARCH:-unknown}_${CC_NAME}${TARGET_CCVER}"

BUILDDIR="build/${TARGET_PLATFORM}"

mkdir -p "$BUILDDIR" || exit 1
mkdir -p "$BUILDDIR/tests" || exit 1

echo "Target platform is $TARGET_PLATFORM"
cat  >$BUILDDIR/Makefile <<EOF
VPATH = ../..
CC = $CC
TARGET_ARCH = $TARGET_ARCH
TARGET_OS = $TARGET_OS

include ../../Makefile.generic

EOF
exit 0

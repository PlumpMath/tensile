#!/bin/sh

CC="${1:-cc}"

NORMALIZE='/^#/d; s/[^[:alnum:]_.+:-]//g; /^$/d'

TARGET_ARCH="$(${CC} -E setup/test_arch.h | sed -e "$NORMALIZE")"  || TARGET_OS=unknown
TARGET_OS="$(${CC} -E setup/test_os.h | sed -e "$NORMALIZE")" || TARGET_OS=unknown
TARGET_CC_ID="$(${CC} -E setup/test_cc.h | sed -e "$NORMALIZE")" || TARGET_CC_ID=unknown
TARGET_VARIANT="$2"

case "$TARGET_CC_ID" in
    gcc*)
        TARGET_CC_FAMILY=gcc
        TARGET_CC_VERSION=${TARGET_CC_ID#gcc}
        ;;
    clang*)
        TARGET_CC_FAMILY=clang
        TARGET_CC_VERSION=${TARGET_CC_ID#clang}
        ;;
    *)
        TARGET_CC_FAMILY=other
        TARGET_CC_VERSION=unknown
        ;;
esac

TARGET_PLATFORM="${TARGET_OS}_${TARGET_ARCH}_${TARGET_CC_ID}${TARGET_VARIANT:+_}${TARGET_VARIANT}"

BUILDDIR="build/${TARGET_PLATFORM}"

mkdir -p "$BUILDDIR" || exit 1
mkdir -p "$BUILDDIR/tests" || exit 1

echo "Target platform is $TARGET_PLATFORM"
cat  >$BUILDDIR/Makefile <<EOF
TOPDIR = ../..
CC = $CC
TARGET_ARCH = $TARGET_ARCH
TARGET_OS = $TARGET_OS
TARGET_VARIANT = $TARGET_VARIANT
TARGET_CC_FAMILY = $TARGET_CC_FAMILY
TARGET_CC_VERSION = $TARGET_CC_VERSION

include ../../Makefile.generic

EOF
exit 0

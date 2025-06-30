#!/bin/bash -eu

DIR=$(dirname "$(readlink -f "$0")")
REPO_DIR="$DIR/mupdf"
WORK="${TUNER_OUTPUT}/work"
mkdir -p "${TUNER_OUTPUT}"
rm -rf "${TUNER_OUTPUT:?}/"*
mkdir -p "$WORK"

cd "$REPO_DIR"
CXX="$TUNER_COMPILER_BIN/g++" CC="$TUNER_COMPILER_BIN/gcc" CFLAGS="$TUNER_FLAGS" LDFLAGS="$TUNER_FLAGS" \
    make -j "$TUNER_CORE_COUNT" HAVE_GLUT=no build=debug OUT="$WORK" "$WORK/libmupdf-third.a" "$WORK/libmupdf.a"

"$TUNER_COMPILER_BIN/g++" -std=c++17 $TUNER_FLAGS -Iinclude "$DIR/pdf_fuzzer.cc" -o "$TUNER_OUTPUT/tune_me" \
    "$WORK/libmupdf.a" "$WORK/libmupdf-third.a"

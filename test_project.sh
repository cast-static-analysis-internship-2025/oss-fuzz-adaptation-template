#!/bin/bash -eu
# This script is intended to test the build and run interface of the project in a Docker container.

APP_DIR="/app"
BUILD_SCRIPT="/app/build.sh"
CORPUS_TAR_GZ="/app/corpus.tar.gz"
CORPUS_DIR="/app/corpus"
GCC_BIN="/gcc/gcc-10.3.0-bin/bin"
BUILD_CORE_COUNT=1

if [ ! -d "$APP_DIR" ]; then
  echo "$APP_DIR directory doesn't exist!"
  exit 1
fi

if [ ! -f "$BUILD_SCRIPT" ]; then
  echo "$BUILD_SCRIPT script doesn't exist!"
  exit 1
fi

if [ ! -f "$CORPUS_TAR_GZ" ]; then
  echo "$CORPUS_TAR_GZ archive doesn't exist!"
  exit 1
fi

tar -xzf /app/corpus.tar.gz -C /app/

if [ ! -d "$CORPUS_DIR" ]; then
  echo "$CORPUS_DIR directory not found!"
  exit 1
fi

build_O() {
  local LOCAL_BUILD_DIR="$APP_DIR/out_$1"
  mkdir -p "$LOCAL_BUILD_DIR"
  TUNER_COMPILER_BIN=$GCC_BIN \
  TUNER_CORE_COUNT=$BUILD_CORE_COUNT \
  TUNER_OUTPUT=$LOCAL_BUILD_DIR \
  TUNER_FLAGS="-O$1" \
  $BUILD_SCRIPT
}

test_O() {
  local LOCAL_TUNE_BINARY="$APP_DIR/out_$1/tune_me"
  $LOCAL_TUNE_BINARY $CORPUS_DIR
}

wait_for_processes() {
  local pids=("$@")
  local failed=false

  for pid in "${pids[@]}"; do
    if ! wait "$pid"; then
      echo "A process with PID $pid failed!"
      failed=true
    fi
  done

  if $failed; then
    exit 1
  fi
}

dump_asm() {
  cd "$APP_DIR/out_$1/"
  objdump -d -j .text tune_me > dump.asm
}

pids=()
for i in 0 1 2 3 s; do
  build_O "$i" &
  pids+=($!)
done

wait_for_processes "${pids[@]}"

pids=()
for i in 0 1 2 3 s; do
  test_O "$i" &
  pids+=($!)
done

wait_for_processes "${pids[@]}"

dump_asm 0
dump_asm 3

dump_0="$APP_DIR/out_0/dump.asm"
dump_3="$APP_DIR/out_3/dump.asm"

if diff -q $dump_0 $dump_3 > /dev/null; then
  echo "Flags passed to build.sh don't have any effect!"
  exit 1
fi

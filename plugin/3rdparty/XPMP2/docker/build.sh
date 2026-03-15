#!/bin/bash
set -eu

function build() {
  local src_dir="$1"
  export platform="$2"
  shift             # eat the first two arguments, then treat the remainder as flags
  shift
  local flags=("$@")
  echo "----------------- Building for $platform -----------------"

  local build_dir="$src_dir/build-$platform"

  echo "pwd       = $(pwd)"
  echo "src_dir   = $src_dir"
  echo "build_dir = $build_dir"
  echo "flags     = ${flags[@]}"

  case "$platform" in
    win)
      flags+=('-DCMAKE_TOOLCHAIN_FILE=../docker/Toolchain-mingw-w64-x86-64.cmake')
      ;;
    mac*)
      flags+=('-DCMAKE_TOOLCHAIN_FILE=../docker/Toolchain-ubuntu-osxcross.cmake')
      ;;
  esac

  mkdir -p "$build_dir" && cd "$build_dir"
  cmake -G Ninja "${flags[@]}" ..
  ninja
}

src_dir="$(pwd)"
gflags=()

# Interpret anything starting with "-" as additional flags, anything else as platform to build
for param in $@; do
  case $param in
  -*)
      gflags+=($param)
      ;;
  *)
      build "$src_dir" "$param" "${gflags[@]}"
      ;;
  esac
done

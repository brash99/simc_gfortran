#!/bin/bash
set -euo pipefail

build_dir=gep_hallc_reboot/.nps_inclusive_build
mkdir -p "$build_dir"
platform_flags=()
if [[ $(uname -s) == Darwin ]]; then
  # The current Homebrew GCC installation otherwise inherits an invalid
  # macOS 18 deployment target from the newer command-line tools.
  sdk_path=$(xcrun --sdk macosx --show-sdk-path)
  platform_flags=(-mmacosx-version-min=15.0 -isysroot "$sdk_path")
fi

gfortran -O2 -ffixed-line-length-132 -ff2c -fno-automatic \
  -fdefault-real-8 -fdefault-double-8 -fallow-argument-mismatch \
  "${platform_flags[@]}" \
  -c F1F2IN21_v1.0.f -o "$build_dir/F1F2IN21_v1.0.o"
gfortran -O2 "${platform_flags[@]}" \
  -c gep_hallc_reboot/nps_inclusive_singles_generator.f90 \
  -o "$build_dir/nps_inclusive_singles_generator.o"
gfortran "${platform_flags[@]}" "$build_dir/nps_inclusive_singles_generator.o" \
  "$build_dir/F1F2IN21_v1.0.o" \
  -o gep_hallc_reboot/nps_inclusive_singles_generator

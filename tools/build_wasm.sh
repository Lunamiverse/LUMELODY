#!/bin/sh
# Builds build/lumi.wasm (the LUMI engine for the browser's AudioWorklet) with clang + a WASI sysroot.
# WASI_DIR must contain wasi-sysroot-24.0 and libclang_rt.builtins-wasm32-wasi-24.0 (from the wasi-sdk 24 release).
set -e
cd "$(dirname "$0")/.."
W="${WASI_DIR:-/tmp/claude-0/-home-claude/1d3f7a74-71e7-50dd-b486-b0730ece1134/scratchpad/wasi}"
clang++ --target=wasm32-wasi --sysroot="$W/wasi-sysroot-24.0" -O3 -std=c++17 -fno-exceptions -mexec-model=reactor \
  -nodefaultlibs -Wl,--strip-all -Wl,--initial-memory=33554432 -Wl,--max-memory=1073741824 \
  web/lumi_wasm.cpp -lc++ -lc++abi -lc "$W/libclang_rt.builtins-wasm32-wasi-24.0/libclang_rt.builtins-wasm32.a" -o build/lumi.wasm
ls -la build/lumi.wasm

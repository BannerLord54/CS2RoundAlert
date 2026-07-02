#!/usr/bin/env bash
set -euo pipefail

export PATH="/mingw64/bin:/usr/bin:$PATH"

cd "$(dirname "$0")/.."
mkdir -p artifacts/native

/mingw64/bin/g++ \
  -std=c++20 \
  -Os \
  -s \
  -municode \
  -mwindows \
  -static \
  -static-libgcc \
  -static-libstdc++ \
  -D_GLIBCXX_USE_CXX11_ABI=0 \
  -DUNICODE \
  -D_UNICODE \
  -D_WIN32_WINNT=0x0A00 \
  -o artifacts/native/CS2RoundAlert.exe \
  src/CS2RoundAlert.Native/CS2RoundAlert.cpp \
  -lws2_32 \
  -lwinmm \
  -lole32 \
  -lshell32 \
  -luuid \
  -ladvapi32 \
  -luser32

sha256sum artifacts/native/CS2RoundAlert.exe > artifacts/native/CS2RoundAlert.exe.sha256
ls -lh artifacts/native

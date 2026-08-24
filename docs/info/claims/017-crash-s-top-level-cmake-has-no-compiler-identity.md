---
id: C017
kind: claim
status: holds
created: 2026-08-24
tags: build,compiler,launcher
depends: CMakeLists.txt, tools/verify.py
---

## Claim

Crash's top-level CMake has no compiler identity allowlist or denylist, while the authoritative verifier explicitly selects Clang

## Evidence

2026-08-24: `cmake -S . -B scratch/build-clang -DCMAKE_C_COMPILER=clang
-DCMAKE_CXX_COMPILER=clang++` configured and generated successfully; generated compiler metadata
reports Clang, source search finds no `CMAKE_C_COMPILER_ID`/`CMAKE_CXX_COMPILER_ID` gate, and
`tools/verify.py` remains the explicit Clang selector.

## What would falsify it

A top-level configure path rejects an otherwise compatible compiler by identity, or tools/verify.py stops selecting Clang for authoritative verification.

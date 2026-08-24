---
id: C017
kind: claim
status: holds
created: 2026-08-24
tags: build,compiler,launcher
depends: CMakeLists.txt, tools/verify.py
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:24:10
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

## Re-confirmed 2026-08-24

At bb3a497, top-level CMake contains no compiler identity allowlist/denylist. Authoritative verification explicitly used Clang 22.1.8, while launcher tests passed explicit unfamiliar CC/CXX values through and allowed normal CMake discovery.

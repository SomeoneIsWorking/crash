---
id: C017
kind: claim
status: holds
created: 2026-08-24
tags: build,compiler,launcher
depends: CMakeLists.txt, tools/verify.py
reconfirmed: 2026-09-04
verified_at: 2026-09-04
---

## Claim

Crash's build and verifier accept the caller's compatible compiler without an identity allowlist or denylist

## Evidence

The top-level CMake and `tools/verify.py` contain no compiler-identity gate or forced compiler.
Maintainers select Clang in their invocation; the player launcher passes through an available
compatible compiler. The verifier uses `build/verify`, outside disposable scratch space.

## What would falsify it

A top-level configure or verification path rejects an otherwise compatible compiler by identity or
silently replaces the caller's compiler selection.

## Re-confirmed 2026-08-24

The 2026-09-04 migration build selected Clang 22.1.8 at configure time; CMake accepted it without a
project compiler gate, and launcher tests continued to pass caller-selected toolchains through.

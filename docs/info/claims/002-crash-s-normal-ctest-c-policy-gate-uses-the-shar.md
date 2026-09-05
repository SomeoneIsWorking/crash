---
id: C002
kind: claim
status: holds
created: 2026-08-21
tags: build,verification,quality
depends: CMakeLists.txt, tools/verify.py, psxport.pin
reconfirmed: 2026-09-04
verified_at: 2026-09-04
---

## Claim

Crash's normal verifier reconfigures the authoritative Ninja tree, builds the current graph, runs
the full CTest graph, and checks that the built PSXPort revision matches `psxport.pin`. Maintainers
select Clang in their invocation; the repository does not reject compatible compilers by identity.

## Evidence

The configured Clang 22.1.8 Ninja tree records the current PSXPort/Lightrec dependency boundaries.
Its C++ policy test runs the shared formatter, 1,200-line structure, and compile-database-backed
clang-tidy checks over first-party code, excluding generated build products and dependencies.

## What would falsify it

The verifier can consume a stale test graph, skips configure/build/CTest/pin checking, stops using
Ninja, a tracked first-party C/C++ source escapes the shared quality gate, dependency/build output
enters that denominator, or a project compiler-identity restriction is introduced.

## Re-confirmed 2026-09-04

An isolated `build/verify-pin` tree configured with Clang 22.1.8 and Ninja against PSXPort
`dc7f53cb3d5e32439bb5d42f2405748418a008de` and Lightrec
`c9f0a37dbbc7e24d841c84751d9619ad1bfcb7d8`, built the asset-free native/Lightrec product graph,
passed all 17 CTest cases, passed the product/repository execution-boundary audit, and matched the
recorded framework pin.

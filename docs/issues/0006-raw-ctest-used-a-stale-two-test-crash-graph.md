---
id: 6
title: Raw CTest used a stale two-test Crash graph
status: resolved
symptom: Raw CTest consumed a stale generated graph and omitted current title tests
tags: workflow,verification,cmake,crash3
created: 2026-08-22
updated: 2026-08-22
---

## Root cause

CTest consumes the test graph already generated in its build directory. It does not reconfigure CMake. The README named raw ctest as the normal verifier, so a checkout whose CMakeLists changed could run an obsolete graph and appear authoritative.

## Resolution

`tools/verify.py` is the normal verifier. It configures `build/verify` with the caller's selected
compiler, builds that same tree, runs its full CTest graph, and checks the psxport provenance.

## Evidence

The stale graph listed 2 tests and referenced deleted tests/test_provision_crash1.py. The shipping verifier then regenerated the same scratch/build-clang directory and ran 5/5: Crash 1, Crash 2, and Crash 3 runtime inheritance, crash_cpp_policy, and crash_title_provision_tests. All Crash 1/2/3 real gates subsequently passed from scratch/build-clang.

### Note (updated 2026-09-04)

The provenance checker and verifier now share `build/verify` as their one default build root. The
earlier mismatch proved that reading another tree can report stale framework provenance even after a
current build passes.

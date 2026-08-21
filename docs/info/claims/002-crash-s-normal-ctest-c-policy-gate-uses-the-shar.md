---
id: C002
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt
reconfirmed: 2026-08-21 03:06:27
verified_at: 2026-08-21 03:06:27
---

## Claim

Crash's normal CTest C++ policy gate uses the shared psxport checker; its empty scaffold produces explicit zero denominators, and CMake refuses a GNU C++ compiler before configuring

## Evidence

Clang 22 configure plus the scoped CTest passed; a fresh CC=gcc CXX=g++ configure failed at the project compiler guard; the checker reported 0 format, 0 size, and 0 touched clang-tidy units

## What would falsify it

A first-party C++ source is added without becoming format/tidy/size checked, a non-Clang compiler configures successfully, or the shared checker/pin changes without re-verification

## Re-confirmed 2026-08-21

Reverified Crash from a fresh Clang 22 build against clean pinned psxport eb2465b2: scaffold linked, psxport_smoke passed 8/8, scoped CTest policy passed with explicit empty-scaffold denominators, and psxport_sync.py --check confirmed the built SHA equals the recorded pin.

## Re-confirmed 2026-08-21

Reverified Crash after final framework landing be381503: CMake reconfigured with Clang 22, scaffold rebuilt and linked, psxport_smoke passed 8/8, normal CTest C++ policy passed, and psxport_sync.py --check confirmed build/ used the exact recorded be381503 pin.

## Re-confirmed 2026-08-21

Reverified after adding the Crash 1 provisioning CTest: Clang 22 configured the tree, crash_scaffold linked, psxport_smoke passed 8/8, crash_cpp_policy passed, and psxport_sync.py --check matched be381503.

## Re-confirmed 2026-08-21

Final reverify after registering the bytecode-clean provisioning CTest: Clang 22 configured, crash_scaffold linked, psxport_smoke passed 8/8, crash_cpp_policy and nine-case provisioning CTest passed 2/2, and psxport_sync.py --check matched be381503.

## Re-confirmed 2026-08-21

Reverified the final Crash tree against pinned psxport 2b5ef7b5: Clang 22 configured both build trees, crash_scaffold linked, psxport_smoke passed 8/8, normal CTests passed 2/2, the C++ policy printed explicit zero scaffold denominators, and psxport_sync.py --check matched the recorded pin.

## Re-confirmed 2026-08-21 03:05:54

2026-08-21 post-integration: clean Clang builds in build/ and scratch/build-clang passed crash_cpp_policy 1/1 with explicit 0/0 scaffold denominators; normal CTests passed 2/2 and psxport_smoke passed 8/8 at framework 2b5ef7b5.

## Re-confirmed 2026-08-21 03:06:27

2026-08-21 landed integration recheck: crash_cpp_policy remained 1/1 with 0/0 scaffold denominators, CTests 2/2, smoke 8/8 at psxport 2b5ef7b5.

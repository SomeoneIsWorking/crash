---
id: C002
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt
reconfirmed: 2026-08-22
verified_at: 2026-08-22 18:17:20
---

## Claim

Crash's normal CTest C++ policy gate uses the shared psxport checker over every tracked first-party
C/C++ file, and CMake refuses a non-Clang C++ compiler before configuring.

## Evidence

At pinned psxport `2b5ef7b5`, Clang 22 configured and built the repository. The normal policy gate
format- and size-checked `tests/crash1_recomp_boundary.cpp`, then clang-tidy checked its real compile
command 1/1. Generated and external files were excluded. The earlier source-free configuration
produced explicit zero denominators, and a GNU C++ configuration was separately refused by the
project compiler guard.

## What would falsify it

A tracked first-party C/C++ file escapes format, size or compile-backed clang-tidy checks; generated
or vendored code enters the first-party denominator; a non-Clang compiler configures; or the shared
checker/framework pin changes without re-verification.

## Re-confirmed 2026-08-21

Clang 22 configured both build trees; shared policy format/size/tidy checked the one compile-backed first-party TU 1/1; normal CTests passed 2/2 and smoke passed 8/8 at psxport 2b5ef7b5.

## Re-confirmed 2026-08-21

Post-landing CTest 2/2 passed; crash_cpp_policy format/size/clang-tidy now covers the one compile-backed boundary runner.

## Re-confirmed 2026-08-22

Pinned psxport 7f5d3f13 policy gate format-checked 4 first-party files, size-checked all 4 at the 1,200-line default, and ran clang-tidy over all 3 compile-backed C++ translation units; full CTest passed 3/3.

## Re-confirmed 2026-08-22

Fresh pinned Clang build ran normal CTest: crash1_runtime_inheritance, crash_cpp_policy, and the provisioning contract passed 3/3; the C++ policy gate checked touched first-party sources.

## Re-confirmed 2026-08-22

Clean Clang 22 build against recorded psxport ad5cf802 passed normal CTest 4/4; crash_cpp_policy passed complete format, 1,200-line ownership, and compile-database clang-tidy coverage.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

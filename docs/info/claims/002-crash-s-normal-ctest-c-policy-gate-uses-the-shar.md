---
id: C002
kind: claim
status: holds
created: 2026-08-21
tags:
depends: CMakeLists.txt, tools/verify.py, tools/run.py, psxport.pin
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:23:47
---

## Claim

Crash's normal verifier reconfigures and builds the authoritative Clang tree before running the full
CTest graph; its C++ policy gate uses the shared psxport checker over every tracked first-party C/C++
file, and CMake refuses a non-Clang C++ compiler before configuring.

## Evidence

At pinned psxport `2b5ef7b5`, Clang 22 configured and built the repository. The normal policy gate
format- and size-checked `tests/crash1_recomp_boundary.cpp`, then clang-tidy checked its real compile
command 1/1. Generated and external files were excluded. The earlier source-free configuration
produced explicit zero denominators, and a GNU C++ configuration was separately refused by the
project compiler guard.

## What would falsify it

The normal verifier can run an old generated CTest graph, skips configure/build/pin checking, or uses
a path other than `scratch/build-clang`; a tracked first-party C/C++ file escapes format, size or
compile-backed clang-tidy checks; generated or vendored code enters the first-party denominator; a
non-Clang compiler configures; or the shared checker/framework pin changes without re-verification.

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

## Re-confirmed 2026-08-22

Clang 22 normal CTest passed 5/5 after the three-title CMake/runtime integration; crash_cpp_policy passed full format, 1200-line, and compile-database clang-tidy checks.

## Re-confirmed 2026-08-22

tools/verify.py reconfigured and built authoritative scratch/build-clang with Clang 22, then ran current CTest 5/5 including all three runtime owners, full cpp policy, and serial provisioning; psxport pin check matched ad5cf802. The immediately preceding raw CTest graph had exposed the opposite stale 2-test answer.

## Re-confirmed 2026-08-22

Post-commit authoritative tools/verify.py reconfigured and built scratch/build-clang with Clang, ran the current 5/5 CTest graph, and passed the exact ad5cf802 pin check.

## Re-confirmed 2026-08-22

At recorded psxport 57a17a14, tools/verify.py reconfigured authoritative scratch/build-clang with Clang, built it, ran current CTest 5/5 including full format/size/clang-tidy policy, then tools/psxport_sync.py --check read that same tree and matched the pin. The retired build/ tree still held ad5cf802, proving the corrected checker can expose the opposite stale-path answer.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, python3 tools/verify.py reconfigured the authoritative Clang tree, built it, passed CTest 5/5 including direct runtime inheritance and C++ policy, then verified the same built framework SHA against psxport.pin.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, python3 tools/verify.py reconfigured the authoritative Clang tree, built it, passed CTest 5/5 including all three explicit guest-picture policy checks and C++ policy, then matched the built SHA to psxport.pin.

## Re-confirmed 2026-08-24

After aca3ecb, root reran tools/verify.py: clean Clang configure/build, CTest 5/5, and pin check all passed.

## Re-confirmed 2026-08-24

On 2026-08-24, uv run --frozen python tools/verify.py configured scratch/build-clang with clang/clang++, built crash1_port and all normal targets, passed CTest 6/6 including format/size/clang-tidy and launcher tests, then passed the recorded psxport 9c2e3f1c pin check.

## Re-confirmed 2026-08-24

At bb3a497, uv-frozen tools/verify.py explicitly configured Clang, built the current graph, passed CTest 6/6, and confirmed recorded psxport 9c2e3f1c. The separate player configuration carries BUILD_TESTING=OFF and builds only named shipping dependencies.

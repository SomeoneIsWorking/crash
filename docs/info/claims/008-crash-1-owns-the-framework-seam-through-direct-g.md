---
id: C008
kind: claim
status: holds
created: 2026-08-22
tags: runtime,inheritance
depends: titles/crash1/core/crash1_runtime.cpp, titles/crash1/core/crash1_runtime.h, tests/crash1_runtime.cpp, tests/crash1_recomp_boundary.cpp, CMakeLists.txt
reconfirmed: 2026-08-24
verified_at: 2026-08-24 20:05:17
---

## Claim

Crash 1 owns the framework seam through direct GameRuntime inheritance with no legacy compatibility views or unmeasured runtime products.

## Evidence

crash1_runtime_inheritance passed from the normal CTest path: the process runtime, Game, and Core all named one Crash1Runtime; Core cfg/hooks and the runtime legacy views were null; context, FrameDriver, TaskScheduler were absent. The real generated boundary runner installs that same runtime and retains 34/34 oracle agreement at all eight calls.

## What would falsify it

A Crash 1 execution path constructs Game before installing Crash1Runtime; either legacy view becomes non-null; an unmeasured runtime product appears; or the installed boundary runner no longer passes the eight-call oracle gate.

## Re-confirmed 2026-08-22

Post-recording reconfirmation: CTest crash1_runtime_inheritance passed, the real 7f5d3f13 boundary runner matched all eight calls 34/34, SELFTEST 9/9, and direct code inspection found no first-party GameConfig/GameHooks runtime installation.

## Re-confirmed 2026-08-22

Fresh pinned CTest passed crash1_runtime_inheritance and the installed direct Crash1Runtime remained in the real 12/12 generated boundary runner with null legacy views and no invented runtime products.

## Re-confirmed 2026-08-22

Crash1Runtime now derives through the title-agnostic BoundaryRuntime; crash1_runtime_inheritance passed and still reports null legacy views/products; the full boundary SELFTEST passed 12/12.

## Re-confirmed 2026-08-22

Against pinned ad5cf802, Crash1Runtime inherits through BoundaryRuntime without legacy views or unmeasured products; runtime CTest passed and the full boundary SELFTEST remained 12/12.

## Re-confirmed 2026-08-22

Post-change verification on clean psxport ad5cf802: Clang CTest 4/4, title provision tests 9/9, Crash 1 boundary SELFTEST 12/12 with all eight calls 34/34 and EnterCriticalSection IRQ 1->0, Crash 1 oracle 39/39 and crt0 6/6, Crash 2 identity 11/11/runtime facts 15/15/oracle 39/39/crt0 6/6.

## Re-confirmed 2026-08-22

Normal CTest passed Crash1Runtime inheritance after the shared boundary-title CMake extraction; full generated runner still installed and exercised Crash1Runtime.

## Re-confirmed 2026-08-22

Post-commit authoritative Clang verifier passed the direct Crash1Runtime inheritance contract within the current 5/5 CTest graph.

## Re-confirmed 2026-08-22

Post-change authoritative verification: python3 tools/verify.py passed Clang build and CTest 5/5; cached serial-scoped executable/oracle/runtime gates passed as documented on 2026-08-22.

## Re-confirmed 2026-08-24

At recorded psxport d2266f4b, CTest passed Crash1Runtime direct inheritance with null legacy views and no invented products; the refactored serial-scoped boundary runner retained real SCUS_949.00 SELFTEST 12/12.

## Re-confirmed 2026-08-24

At fetchable recorded psxport bc8c8897, CTest passed Crash1Runtime direct inheritance with null legacy views, no invented products, and explicit guestVramIsPicture=false; its boundary gate retained SELFTEST 12/12.

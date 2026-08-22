---
id: C008
kind: claim
status: holds
created: 2026-08-22
tags: runtime,inheritance
depends: titles/crash1/core/crash1_runtime.cpp, titles/crash1/core/crash1_runtime.h, tests/crash1_runtime.cpp, tests/crash1_recomp_boundary.cpp, CMakeLists.txt
reconfirmed: 2026-08-22
verified_at: 2026-08-22 17:39:21
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

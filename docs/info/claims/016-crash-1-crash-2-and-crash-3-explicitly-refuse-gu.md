---
id: C016
kind: claim
status: holds
created: 2026-08-24
tags: runtime,rendering,inheritance
depends: titles/crash1/core/crash1_runtime.cpp#guestVramIsPicture, titles/crash2/core/crash2_runtime.cpp#guestVramIsPicture, titles/crash3/core/crash3_runtime.cpp#guestVramIsPicture, tests/title_runtime_contract.h#verifyTitleRuntimeContract
---

## Claim

Crash 1, Crash 2, and Crash 3 explicitly refuse guest-VRAM picture ownership through their serial-specific direct runtimes

## Evidence

Against fetchable pinned psxport bc8c8897, the authoritative Clang build and CTest passed 5/5. The shared production-seam runtime contract constructed each derived runtime and required guestVramIsPicture(Game) to return false; all three title tests passed. This records only absence of picture ownership because no measured frame producer exists.

## What would falsify it

Any title returns true without real frame evidence, omits the required virtual override, shares another serial runtime policy implicitly, or a measured native/guest picture producer changes the correct ownership answer.

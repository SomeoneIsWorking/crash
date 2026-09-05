---
id: C016
kind: claim
status: holds
created: 2026-08-24
tags: runtime,rendering,inheritance
depends: titles/crash1/core/crash1_runtime.cpp#guestVramIsPicture, titles/crash2/core/crash2_runtime.cpp#guestVramIsPicture, titles/crash3/core/crash3_runtime.cpp#guestVramIsPicture, tests/title_runtime_contract.h#verifyTitleRuntimeContract
reconfirmed: 2026-09-04
verified_at: 2026-09-04
---

## Claim

Crash 1, Crash 2, and Crash 3 explicitly refuse guest-VRAM picture ownership through their serial-specific direct runtimes

## Evidence

Against exact pinned PSXPort `dc7f53cb3d5e32439bb5d42f2405748418a008de` and Lightrec
`c9f0a37dbbc7e24d841c84751d9619ad1bfcb7d8`, the Clang/Ninja build and 17-test CTest graph pass.
The shared production-seam runtime contract constructs each derived runtime and requires
`guestVramIsPicture(Game)` to return false. This records only absence of picture ownership; it does
not claim a native graphics producer or real-game Lightrec run.

## What would falsify it

Any title returns true without real frame evidence, omits the required virtual override, shares another serial runtime policy implicitly, or a measured native/guest picture producer changes the correct ownership answer.

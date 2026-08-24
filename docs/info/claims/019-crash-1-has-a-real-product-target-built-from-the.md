---
id: C019
kind: claim
status: holds
created: 2026-08-24
tags: crash1,product,recomp
depends: CMakeLists.txt
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:08:08
---

## Claim

Crash 1 has a real product target built from the verified retail resident substrate; crash1_port installs the title runtime and generated registry, dispatches retail crt0, and shares its first-syscall owner with the differential harness, while later boot and frame output remain explicitly unclaimed.

## Evidence

Clean Clang builds in scratch/build-crash-product and authoritative scratch/build-clang linked crash1_port against recorded psxport 9c2e3f1c. The shared implementation's focused Crash 1 harness passed 13/13 and runtime facts matched the executable 15/15. The product binary itself was intentionally not run.

## What would falsify it

if crash1_port stops linking the verified generated substrate or shared ResidentProgram/EnterCritical owner, bypasses retail crt0, proceeds beyond the unverified syscall transition, or is described as rendered/playable without new independent evidence

## Re-confirmed 2026-08-24

Clean recorded psxport 9c2e3f1c authoritative Clang build linked crash1_port from the generated substrate and shared shipping execution owners; Crash 1 runtime facts passed 15/15 and the shared focused path passed 13/13. Product binary remained intentionally unrun.

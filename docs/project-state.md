# Project state

Factual capability coverage for the Crash trilogy port. Epic intent lives in
`docs/project-goals.md`, migration order in `docs/migration.md`, atomic work in `docs/issues/`,
ownership in `docs/codemap.md`, and ordered binary evidence in `docs/re-frontier.md`.

## Comparison baseline

The comparison baseline is each North American retail Crash title under an accurate vanilla
PlayStation emulator. This project separately tracks its native PC host, runtime Lightrec execution,
native rendering, widescreen, interpolation, and player setup.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Serial-identified USA executable facts for Crash 1, 2, and 3 | verified | — | G001 |
| S002 | Disc provisioning selects and verifies each title's boot executable | verified | S001 | G001 |
| S003 | Independent CPU comparison of the resident boot spine | partial | S001 | G001 |
| S004 | The preserved Crash 1 compatibility path reaches the measured menu frontier | partial | S002, S003 | G001 |
| S005 | Game-owned native renderer submission path | missing | S011 | G002 |
| S006 | Widescreen through owned camera/projection state | missing | S005 | G002 |
| S007 | Interpolation through owned simulation and transform state | missing | S005 | G002 |
| S008 | Crash 2 and Crash 3 native/Lightrec products | missing | S002, S003, S011 | G001 |
| S009 | Playable Crash trilogy product | missing | S005, S008, S011 | G001 |
| S010 | Host-owned native frame-loop contract and typed guest-VSync boundary | partial | S001 | G001, G002 |
| S011 | Crash 1 native/Lightrec product preserves the menu and BIOS PadRead frontier | partial | S002, S003, S004, S010 | G001 |
| S012 | Linux x86-64 native/Lightrec product qualification | partial | S011 | G001 |
| S013 | Windows x86-64 native/Lightrec product qualification | missing | S011 | G001 |
| S014 | Apple Silicon macOS native/Lightrec product qualification | missing | S011 | G001 |
| S015 | Android arm64-v8a native/Lightrec product qualification | missing | S011 | G001 |

## Current focus

S011 is the current focus: wire the authenticated Crash 1 image and existing native owners through
psxport's per-`Core` Lightrec executor, preserve the BIOS `PadRead` owner, and reproduce the current
menu frontier. The static path is already deleted; representative interactive gameplay remains the
fidelity gate for its replacement.

## Capability details

### S001 — serial-identified executable facts

Evidence: the three title manifests match the real USA executables' size, hashes, PS-X EXE headers,
region markers, and complete title-specific VSync bodies through the executable verifier and its
negative controls. Crash 1 is `SCUS_949.00`, Crash 2 is `SCUS_941.54`, and Crash 3 is
`SCUS_942.44`. Crash 3's `SYSTEM.CNF` selection excludes the unrelated `DRAGON/SPYRO.EXE` on the
same disc.

### S002 — title-scoped disc provisioning

Evidence: the shared provisioner follows `SYSTEM.CNF`, verifies every tracked executable fact, and
publishes only into the selected title's untracked cache. Its controls cover three-title selection,
precedence, missing configured paths, ambiguous drop-ins, identity disagreement, and Crash 3's decoy
executable.

### S003 — independent resident boot comparison

Partial evidence: the independent Mednafen CPU and the recorded static path agree 34/34 for each
title through its first post-syscall B0 dispatch. Crash 1 additionally validates B(56h) facts; all
three validate Cause/EPC and EPC+4 resume. Crash 1's ordered oracle applies selector-1 return,
B(56h), the C0 slot-6 seed `0x00000C80`, and the fourteen-word copy.

Gap: Crash 1's oracle stops at local wrapper `0x8004323C` before the non-link A(44h) tail dispatch,
so the final A(44h) register/memory comparison remains issue 0008. The old static candidate counts
are not execution coverage and will not be extended during migration.

### S004 — preserved Crash 1 compatibility frontier

Partial evidence: recorded real-disc runs crossed the authenticated boot, native libcd setup and
disc-index I/O, callback/event/pad initialization, GPU watchdog, host-owned field loop, and
presentation. One isolated run reconciled 1,172/1,172 field fences and visibly advanced through the
publisher logos to the 3D title menu. Live traces measured CamUpdate, GfxUpdateMatrices, and
GfxLoadWorlds on all fields; `GoolObjectTransform` first ran at field 324. Exact addresses and earlier
120-field evidence remain in `docs/re-frontier.md`.

Gap: this is frozen evidence from the retired static route, not the target product. Host input did
not reach Crash's BIOS auto-pad word; issue 0012 records the root cause and in-flight owner at
`0x80057054`. Do not rebuild or rerun the static product. S011 must reproduce the menu and input
behavior through Lightrec, then continue to representative gameplay.

### S005 — game-owned native renderer submission

Missing capability: live tracing grounds `GfxUpdateMatrices 0x80017A14` and
`GoolObjectTransform 0x8001DE78` as pre-GTE candidates, but no game-state primitive producer,
native render queue, or native ordering/depth owner exists. Compatibility presentation and guest
primitive records do not satisfy this capability.

### S006 — widescreen

Missing capability: no owned camera/projection producer exists, so there is no grounded state that
can widen horizontal view geometry. GTE/OT/GP0/framebuffer reconstruction and final-image stretching
are excluded.

### S007 — interpolation

Missing capability: no authoritative native simulation tick and no owned previous/current camera or
object-transform snapshots exist. Presentation therefore has no grounded state pair to interpolate.

### S008 — Crash 2 and Crash 3 products

Missing capability: Crash 2 and Crash 3 have verified identities, title runtimes, VSync facts, and
recorded pre-B0 boundaries, but no native/Lightrec gameplay products. They remain sequential work
after Crash 1 passes representative gameplay.

### S009 — playable trilogy

Missing capability: none of the three native/Lightrec products reaches verified representative
gameplay. Crash 1 has only preserved menu evidence from the old route; Crash 2 and Crash 3 do not
have product execution.

### S010 — native frame-loop ownership contract

Partial evidence: the three verified executables bind distinct VSync leaves—Crash 1
`0x8003E4F0`, Crash 2 `0x8004A484`, and Crash 3 `0x8004B2A8`—to typed psxport
`FrameBoundary` exits. Crash 1 has recorded finite field-loop evidence; Crash 2 and Crash 3 still
have refusing drivers.

Gap: only Crash 1 has a measured advancing loop, and none has consumed a frame boundary produced by
the pinned Lightrec executor during real guest execution.

### S011 — Crash 1 native/Lightrec product

Partial capability: the static translator, corpus, dispatcher, seed inputs, and static-only tests
are absent. Crash 1 composition now targets psxport's image-aware executor and native override/
original-call boundary, while preserving the BIOS `PadRead` owner. The asset-free product builds and
passes its link/repository boundary audit, but real-game execution proof is not complete, so this is
not yet a runnable gameplay claim.

On 2026-09-05 the canonical locked verifier passed all 19 title CTests against PSXPort
`eb5f23a8b3506f8853b3cfadcedc024cd90818a0` and Lightrec
`b1457137c31cedff5f440d59da29401d021ba2da`, with the maintained GNU Lightning prefix.
The synthetic test exercises Crash's shipping dispatcher: unmapped override refusal, native
augmentation calling the original guest function, native-memory-write invalidation, and a JIT turn
that services native calls before its typed frame exit. It requires nonzero translated/executed
blocks and instructions, with zero fallback. The same gate covers the retained native contracts,
BIOS pad publication, launcher/dependency refusals, full C++ policy, and linked product inspection.

Gap: reach the existing 3D menu with nonzero Lightrec blocks while preserving all
current native owners and issue 0012's BIOS `PadRead` publication, then prove representative gameplay,
deterministic oracle/device comparison, invalidation controls, and released-host qualification.

### S012 — Linux x86-64 host qualification

Evidence: S011's exact-pinned local Clang/Ninja product gate passes all 19 CTests and the linked
execution-boundary audit. Hosted CI uses the same canonical verifier and maintained dependencies.

Gap: no hosted result for this pending commit, representative real-game execution, or release
performance qualification.

### S013 — Windows x86-64 host qualification

Missing capability: PSXPort currently refuses the Windows product target. Its Lightrec host
integration must be implemented and verified before Crash's real product CI can run on Windows.

### S014 — Apple Silicon macOS host qualification

Missing capability: PSXPort currently refuses AArch64. Executable-memory, cache-coherence, ABI,
exception, packaging, and gameplay boundaries remain missing, blocking real macOS product CI.

### S015 — Android arm64-v8a host qualification

Missing capability: there is no Crash Android package, shared Android-port integration, touch layer,
device performance matrix, or verified AArch64 gameplay run. The absent backend and package block
real Android product CI.

Policy-only jobs on another operating system do not qualify a host. Add a Windows, macOS, or Android
workflow only when it exercises that host's real product/runtime boundary; until then these capabilities stay
explicitly missing.

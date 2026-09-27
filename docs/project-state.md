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
| S006 | Widescreen through owned camera/projection state | partial | S005 | G002 |
| S007 | Interpolation through owned simulation and transform state | missing — IN SCOPE (prior text said the opposite) | S005 | G002 |
| S008 | Crash 2 and Crash 3 native/Lightrec products | missing | S002, S003, S011 | G001 |
| S009 | Playable Crash trilogy product | missing | S005, S008, S011 | G001 |
| S010 | Host-owned native frame-loop contract and typed guest-VSync boundary | partial | S001 | G001, G002 |
| S011 | Crash 1 native/Lightrec product preserves the menu and BIOS PadRead frontier | partial | S002, S003, S004, S010 | G001 |
| S012 | Linux x86-64 native/Lightrec product qualification | partial | S011 | G001 |
| S013 | Windows x86-64 native/Lightrec product qualification | missing | S011 | G001 |
| S014 | Apple Silicon macOS native/Lightrec product qualification | missing | S011 | G001 |
| S015 | Android arm64-v8a native/Lightrec product qualification | missing | S011 | G001 |

## Current focus

S011 is the current focus: determine why the authenticated Lightrec menu run does not consume
published BIOS `PadRead` input, then enter representative interactive gameplay. The static path is
already deleted; gameplay remains the fidelity gate for its replacement.

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

**Crash 1: partial. Crash 2 and Crash 3: missing.** The owner exists and is gated; the live wide-leg
measurement is not available on this machine.

Evidence, and it is guest-state evidence rather than a config value: a whole-image census of all
72192 instruction words of `SCUS_949.00` finds exactly **two** GTE control-register writers for
`OFX` (`CR[24]`, `0x80042B88` in `gte_init` and `0x80042F94` in `SetGeomOffset`), two for `OFY`
(`CR[25]`) and two for `H` (`CR[26]`), and **zero** control-register readers of any of them
(`tools/probe_crash1_projection.py`, ranges in `titles/crash1/executable.json` → `runtime.projection`).
The retail 4:3 projection is therefore **`OFX = 0`, `OFY = 0`, `H = 0x3E8` (1000)** with a
per-camera-mode `H` republished every frame — and the real boot prints exactly that from the guest's
own registers. `crash1_widescreen.*` widens `OFX` by the plan's horizontal margin at the measured
per-frame publication site, leaves `OFY` and `H` untouched, and is 4:3-identical by construction
(`tests/crash1_widescreen.cpp`, 12 cases including an install proof with no HLE plan in existence).

Gap, stated rather than papered over: the product faults on frame 0 at `unimplemented BIOS A0:0x27`
because **no disc media is provisioned** on this machine, so the per-frame `SetGeomOffset` is never
reached in a live run and the widened centre has no live leg. Two framework-side facts also mean
`render_width > native_width` cannot be shown for this title as the tree stands, and both are
reported rather than worked around:

- `runtime/psx/picture_announce.cpp:69` judges a widening with
  `classifyWide(aspect, core.rsub.mode.enhancementsAllowed(), ...)`, and `enhancementsAllowed()` is
  `mPath == RenderPath::Native`. A `widescreenOnly` title's path is **Gte**, so the classifier always
  returns `RefusedPure` and logs `any widescreen claim from this run is void` — for a *guest*
  widening, which the contract deliberately allows on Gte.
- `picture_announce` prints only on CHANGE, and in the measured legs the single `[wide]` line
  precedes the guest's own publication by 125 ms, so the post-latch steady state is never announced.

What the legs do prove, from the guest's own registers and the framework's own latch: the real boot
publishes `H 1000, OFX 0, OFY 0` in both legs, and the latched plan widens with the requested
aspect — `host canvas 428 (native 320)` at 16:9 against `host canvas 320 (native 320)` at 4:3.
`tools/probe_crash1_widescreen_legs.py` reports the missing leg as a failure rather than substituting
a value. Separately, a cull census over the same 72192 words finds **no** compare against a 4:3 dot
width (320 appears only in two stack frames; 319/318/352/368 appear zero times), so there is no
literal horizontal cull to widen — but a *variable-bound* cull carries no immediate and is invisible
to that scan, which is the honest null. See `docs/issues/0015`.

Pre-existing and not caused by the widescreen work: `crash_dynarec_dispatch` fails, because psxport
now leaves an unstamped exit's `guestPc` to the consumer
(`runtime/cpu/execution_control.cpp:29`) while `tests/dynarec_dispatch.cpp:81` still asserts the old
stamping. The frame-loop contract owns that decision.

Crash 2 and Crash 3 own no projection owner at all. `RenderCapabilities::widescreenOnly()` is
returned by `BoundaryRuntime` for them, which is a **declaration this repository cannot currently
keep**; it is left in place only because those runtimes refuse to boot, and it must be corrected
before either title is allowed to present.

### S007 — interpolation

**CORRECTED 2026-09-27. The previous text here was self-contradictory and inverted the scope.** It read:

> "**Out of scope for Crash 1.** Crash 1 is 30 fps, so this repository will not add an fps60 mode, an
> interpolation path, a lerp, or any temporal pipeline to support one."

Crash 1 being 30 fps is the reason it is **IN** scope, not out of it. The deliverable is interpolated
(lerp) 60 fps for the titles that run 30 fps; the only exclusions are the titles that are *already* 60 fps
(Tekken 3 `SLUS_004.02`, Tomba! 1 `SCUS_942.36`, Mega Man X4 `SLUS_005.61`). So the previous paragraph named
Crash 1 as 30 fps and then used that fact to exclude it — a conclusion that follows from nothing, in a living
document that is the authority a reader would take the scope from. The three 30 fps titles in this
workspace (Crash 1, Crash 2, Crash 3) are all in lerp scope. Nothing else in the workspace states this
inversion, so it was a local mistake here rather than a convention.

**What the work actually is, and it is smaller than the paragraph implied.** The framework owns the
interpolation machinery: `runtime/psx/frame_presenter.cpp` and `runtime/psx/fps60_game_hooks.h` implement
the presenter and the source-reconstruction seam. A port opts in by returning
`RenderCapabilities::interpolatedNative()` (`runtime/psx/render_capabilities.h`), which sets
`temporalInterpolation`, and that flag is what gates the `fps60` row in the options UI
(`runtime/ui/mod_row_model.cpp:247`: `id != "fps60" || m.temporalInterpolationSupported()`). A port that
never declares it cannot be toggled into 60 fps at all. `Tomba2Engine` (both titles) and `spyro` already
declare it.

**So S007 is `missing` for the honest reason: Crash 1 has no native/Lightrec product to interpolate.**
`BoundaryRuntime` returns `RenderCapabilities::widescreenOnly()` and the Crash 2/3 runtimes refuse to boot
(S008, S009). Interpolating frames that are never produced is not a smaller task; it is the same task one
step later. The work is therefore sequenced after the Crash 1 product, and its first observable step is
`BoundaryRuntime` returning `interpolatedNative()` for Crash 1 — which also discharges the widescreen
declaration problem recorded above it.

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
are absent. The authenticated Crash 1 native/Lightrec product reaches the preserved 3D title menu
through psxport's image-aware executor and native override/original-call boundary. The 2026-09-12
headless/silent run recorded 3,389 translated blocks, 13,964,056 executed blocks, and zero fallback.
Before correcting controller-0 halfword placement, the BIOS `PadRead` word responded to held Start
and Cross, but the retail pad structs remained zero and the menu did not advance (issue 0012). The
corrected input path still needs a retail acceptance run; this is not yet a runnable gameplay claim.

Crash 1's product loader now checks the manifest size and SHA-256 on the same bounded buffer it gives
the shared PS-X EXE mapper. Its asset-free contract refuses altered and truncated inputs before
changing Core state; the real Lightrec menu run above used the authenticated image.

The title currently declares guest GTE geometry as its default and refuses native rendering and
temporal interpolation requests while the corresponding producers remain missing (issue #5).

On 2026-09-05 the canonical locked verifier passed all 19 title CTests against PSXPort
`eb5f23a8b3506f8853b3cfadcedc024cd90818a0` and Lightrec
`b1457137c31cedff5f440d59da29401d021ba2da`, with the maintained GNU Lightning prefix.
The synthetic test exercises Crash's shipping dispatcher: unmapped override refusal, native
augmentation calling the original guest function, native-memory-write invalidation, and a JIT turn
that services native calls before its typed frame exit. It requires nonzero translated/executed
blocks and instructions, with zero fallback. The same gate covers the retained native contracts,
BIOS pad publication, launcher/dependency refusals, full C++ policy, and linked product inspection.

Gap: resolve issue 0012's downstream pad consumption, then prove representative gameplay,
deterministic oracle/device comparison, invalidation controls, and released-host qualification.

### S012 — Linux x86-64 host qualification

Evidence: S011's exact-pinned local Clang/Ninja product gate passes all 19 CTests and the linked
execution-boundary audit. Hosted CI uses the same canonical verifier and maintained dependencies.
The Linux x86-64 asset-free product composition gate passed on main commit
`fd277b409fd8d12a8bde2824de705d7335c53eec` in
[run 33960149147](https://github.com/SomeoneIsWorking/crash/actions/runs/33960149147).

Gap: representative real-game execution and release performance qualification remain unverified.

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

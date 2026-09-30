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
| S016 | Crash 1, 2, and 3: load operations complete without loading-only waits or presentation; logos cancel through the recovered route | missing | S008, S011 | G003 |

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

Correction to this entry, measured 2026-09-28 (issue 0021): it read as though the guest submitted
nothing. It does — **238,624 primitives over 400 frames** on the disc-backed run, measured with
`PSXPORT_PRIMDUMP` and a 238,624-row CSV. The `OtAttr spans recorded 0` line that produced the claim is
a dead tap for a typed `GameRuntime` with no declared packet-pool window. The capability above is still
missing, but it is missing because no **native** producer exists, not because the guest leg is empty.

### S006 — widescreen

**Crash 1: partial. Crash 2 and Crash 3: missing.** The owner exists and is gated, and the live
wide-leg measurement now runs on this machine with the disc: the 16:9 leg announces
`native_width=512 render_width=684` against `512/512` in 4:3, and the host canvas is 428 against 320.
**What is still missing is the GUEST's own widened centre, and the reason is now named and is not the
owner** — see `docs/issues/0023`. A whole-image scan finds exactly **two** direct callers of the
widened leaf `0x80042F8C`, `0x8001783C` and `0x80017F00`, and **zero** `lui`+`addiu`
materialisations, so the leaf is reached only through camera code: caller A publishes the distance and
reads the near-plane global `0x800578D0`, caller B re-authors the horizontal centre every frame from
the camera-shake word. **Neither runs**, and this is now measured with input driving the title into a live, unpaused level
in a 16:9 leg (issue 0024): present frame 3903, `paused 0`, N. Sanity Beach, 1,384 polygons, and the
leaf still publishes 0 centres. **The owner is not the cause, and a positive control proves it** —
`call 0x80042F8C` with a non-zero centre in a wide leg prints
`guest centre 5 -> 91 (retail 5 + margin 86, OFY 0), host canvas 684 (native 512)`, so the key
intercepts, the owner runs, and the widening arithmetic is right.

The reason nobody noticed sooner is that `publishCentre` prints its line under
`latched.widescreen() && retailX != 0`, so a 4:3 leg — and a call passing `$a0 = 0` — cannot report a
centre whether or not the owner ran. That is the fifth dead tap in this workspace and the first that
lives in an owner rather than in a counter.

What is left open is the census's own blind spot: a direct-call scan did not close the indirect
`jalr`/computed-register set, and it did not ask the larger question, who ELSE writes `CR[24]`.

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

Gap, stated rather than papered over: the product leaves guest execution on frame 0, so the per-frame
`SetGeomOffset` is never reached in a live run and the widened centre has no live leg. **The cause
recorded here before was wrong on both halves and is corrected below**, because a wrong gap text is
what makes a reader stop looking:

- it said the stop was `unimplemented BIOS A0:0x27`. That call is implemented; the run now advances
  8,177,050 guest cycles before stopping.
- it said no disc media is provisioned on this machine. The disc **is** on this machine (its path is the operator's, so it is not recorded here). What was missing was that
  `tools/probe_crash1_widescreen_legs.py` set no disc path at all, so **every leg on record ran with
  the CD model reporting no media** — `The CD model will run with NO MEDIA` and
  `CdRead: LBA 16 unreadable ... 0 sector(s) delivered` are in both logs verbatim. "No frame at all"
  was a measurement of the tool. The probe now takes `--disc` and REFUSES to report a picture verdict
  for a media-less leg; asked about the existing legs it answers `(False, 'The CD model will run with
  NO MEDIA')` for both.

The stop itself is now attributed with evidence, not inferred: it is a **fault**, in
`crash1_block_pool.*` at guest `0x80015978`, and issue 0020 carries the instruction words, the six
measured call sites and the pointer the guest published. Whether an uninitialised pool is *why* it is
uninitialised is not established and needs one run with media. Two framework-side facts also mean
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

**Where the run stops, measured 2026-09-28 (issue 0020).** Guest execution leaves frame 0 at
`0x800159A8` with reason `Fault` and the detail `Lightrec execution fault` after 8,177,050 cycles —
not a budget exit, and not a translation refusal (`fallback_blocks=0` in the same log). That address
is the word `0x8C620004`, `lw $v0,0x4($v1)`, the second class read of the engine's size-class block
cell lookup at `0x80015978`; the guest printed `invalid load/store at address PC 0x00800004`, so the
pointer it followed was `0x00800000`, which is unmapped. `crash1_block_pool.*` is the recovered
function, registered as a native override, and it is the engine's BOUNDED form: the walk is bounded by
`((cell - poolBase) >> 3) >= liveCellCount` with `poolBase` from `0x8005C534` and the count from
`*(0x8005C540)+0x404`, which is the rule the image's own unused `0x800159C4` states, and an
unsatisfiable class returns the engine's own `0xFFFFFFF6` instead of reading past the cell array. A
bucket that is not a pointer into main RAM is refused outright, with the value named. The owner
records the request, class, bucket, pool base, live count, cells walked, the first caller's `ra` and
the `lookups/found/exhausted/unreadableBucket` denominators, so a run can name the wrong value.

**Which call site actually ran, measured at run time rather than by census:** every lookup in every
run came from `ra = 0x80015164`, the return address of the `jal` at `0x8001515C` inside
`FUN_80015118`. A call-graph census could not have chosen it — 818 distinct `jal` targets exist in the
same 72,192 words, so the graph saturates.

**The engine's own bound at `0x800159C4` was applied, measured to break a path retail completes, and
removed.** With it enforced, a disc-backed run reported "no cell serving class 702 within 576 live
cells past pool base 0x80061FA0" and faulted after 320,508 cycles; with it removed the same run reached
the title's first measured display wait. The owner therefore walks as retail does and stops only at the
edge of main RAM, and reports the engine's base, live count and signed distance as a measurement. A
controlled pair — the same binary and disc with the override OFF — reaches the same boundary as the
override ON, so the owner is transparent for every case retail can execute.

**The next stop after the pool was the frame driver's own guard, and it was wrong.** The measured
boundary was `guestPc == r[31] == 0x800170FC`, which is the continuation of the guest's own `jal` to
the libetc VSync leaf at `0x800170F4` — the common case, which the driver did not accept.
`Crash1FrameDriver::isMeasuredFrameBoundary` now accepts both provenances and refuses a mixture, the
leaf's own entry, and any other address. **After that correction the run completes: exit 0**, 1,549
translated blocks, 1,689,262 executed blocks, 26,457,241 executed instructions, **0 fallback blocks**,
and the CD read the disc (104 hunk lookups, 85 hits).

**STILL NO PICTURE, and this is the honest bottom line.** The end-of-run line is
`[producers] run-end: OtAttr spans recorded 0 (overflow 0)`. **That line is a DEAD TAP for this title
and the premise it encodes was false** (issue 0021). The guest is not silent: with the framework's own
GP0 packet dump on the same binary, same disc, 400 frames, it submits **238,624 primitives** —
210,144 `0x7C` sprites, 28,436 `0x30` Gouraud polygons, 44 `0x2A` textured Gouraud polygons, 1 on frame
1 and ~900 per frame by frame 355. `OtAttr` attributes guest stores by asking `RenderNoiseMask` for the
game's packet-pool window, which comes from `LegacyGameConfig::packetPoolBase/Stride`; **Crash 1 is a
typed `GameRuntime` and declares neither**, so the mask is empty for every store and the count is 0 by
construction. The framework says so itself at `psxport/runtime/psx/ot_attr.cpp:110-121` — "an empty span
table means 'not measured', NOT 'the guest submitted nothing'" — but that warning is unreachable from
this path, so `runtime/psx/native_boot.cpp:286` prints a bare `0` with no caveat. **Whether frames are
PRESENTED was a separate question, and it is answered (issue 0022): they are.** One disc-backed
400-frame run presents frames whose pixels are 35.7% non-black at fence 400 (512x240) — 6 of 6
captured presented frames carry a picture, the UIS copyright screen at fence 205 and a lit 3D scene at
fence 400 — measured by `tools/probe_crash1_primitives.py` in the SAME process that printed
`OtAttr spans recorded 0`. **No drawn aspect is still claimed**; every `[wide]` line reads
`render_width == native_width` on the 4:3 leg, and nothing above is a parity or gameplay claim.

**THE CAUSE WAS A FAULT IN THE POOL OWNER, and it is corrected** (issue 0021).
`crash1_block_pool.h` recovered the lookup at `0x80015978` as comparing each cell's class field
against `request >> 13`, from the entry `srl $v0,$a0,13`. The image says the class is the request
WHOLE: `0x8001599C` `beq $2,$4` and `0x800159B0` `bne $2,$4` compare against `$a0`, which nothing
between the entry and them redefines; the bounded sibling at `0x800159E8` does the same; and the
pool's own allocate path writes and compares the unshifted word (`0x80012FFC`), and stores the word it
is about to look up in the delay slot at `0x80013140`. So the owner could never match a cell the engine
wrote: every lookup ran off the end of main RAM and returned a cell at `0x80200000`. Controlled pair,
same binary, same disc, 400 frames — **unservable classes 5 → 0**, executed blocks 1,689,262 → 3,461,249,
executed instructions 26,457,241 → 46,438,388, `fallback_blocks` 0 in both.
`tools/probe_crash1_block_pool.py` now decodes the register operands of all three comparisons and fails
unless each is `$a0`, and its selftest requires one decoder to accept a whole-word fixture and reject a
shifted-key fixture (12 of 12 cases). A prior claim in the same header — that the engine "can only ever"
pass a word-aligned key — is **falsified**: 3 of the 4 live requests index unaligned buckets (700, 8,
660, 156).

**`0x80012F10` is recovered** as the pool's node allocate path, not an "initialiser" and not data: 259
words, `0x80012F10..0x8001331C`, sha256 `217a4cd9…30dce`, two `jal` call sites, a 44-byte node table at
`0x8005C554` with its cursor at `0x8005CFAC`, a five-way kind dispatch, the bucket/class rule above, a
cell link, the `+0x0A` live counter, and a 28-byte-stride per-type callback table at `0x800514EC`. It is
`titles/crash1/core/crash1_pool_node.{h,cpp}` — a readable model over an injected memory seam, gated by
`tools/probe_crash1_pool_node.py` (12 of 12) and `tests/crash1_pool_node.cpp` — and it is **not**
installed as a native override, because nothing yet measures the guest reaching it: every lookup in the
disc-backed run came from `ra = 0x80015164` (`FUN_80015118`), never from `0x80013140`.

Gap: the pool owner reports `1-of-1 found a cell, 0-of-1 left main RAM` on its first lookup and every
later walk that leaves main RAM, but the run's TOTAL `lookups/found/leftMainRam` is never printed, so
the denominators grow with the run instead of closing on it. Resolve issue
0012's downstream pad consumption, then prove representative gameplay, deterministic oracle/device
comparison, invalidation controls, and released-host qualification.

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

### S016 — Crash 1, 2, and 3 loading removal

Missing. No load operation has been censused or classified for Crash 1, 2, and 3. Gap: enumerate its load
issuers and the wait and presentation each drives, then complete each through the title's own load
mechanics without its loading-only wait, with payload and terminal state compared against retail
and the absence of loading presentation captured.

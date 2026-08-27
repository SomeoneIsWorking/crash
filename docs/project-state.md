# Project state

Factual capability coverage for the Crash trilogy port. Epic intent is not tracked here; atomic work
lives in `docs/issues/`, ownership in `docs/codemap.md`, and ordered binary evidence in
`docs/re-frontier.md`.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Serial-identified USA executable facts for Crash 1, 2, and 3 | verified | — | G001 |
| S002 | Disc provisioning selects and verifies each title's boot executable | verified | S001 | G001 |
| S003 | Independent CPU comparison of the resident boot spine | partial | S001 | G001 |
| S004 | Crash 1 shipping product executes the measured resident boot spine | partial | S002, S003 | G001 |
| S005 | Game-owned native renderer submission path | missing | S004 | G002 |
| S006 | Widescreen through owned camera/projection state | missing | S005 | G002 |
| S007 | Interpolation through owned simulation and transform state | missing | S005 | G002 |
| S008 | Crash 2 and Crash 3 shipping products | missing | S002, S003 | G001 |
| S009 | Playable Crash trilogy product | missing | S004, S005, S008 | G001 |
| S010 | Host-owned native frame-loop contract and fatal guest-VSync boundary | partial | S001 | G001, G002 |

## Current focus

S004 is the current focus: extend Crash 1's isolated compatibility run beyond the static 120-frame
intro splash and drive input toward gameplay; then use the grounded submitters to advance the native
renderer capability.

## Capability details

### S001 — serial-identified executable facts

Evidence: `titles/crash1/executable.json`, `titles/crash2/executable.json`, and
`titles/crash3/executable.json` match each retail USA executable's size, hashes, PS-X EXE header, and
region markers and complete VSync body through `tools/verify_executable.py`. Crash 1 additionally
binds its CoreLoop done address to the authenticated LUI/LW load site; its nine-case selftest and the
other titles' positive/negative controls pass.

### S002 — title-scoped disc provisioning

Evidence: `tools/provision_title.py` follows `SYSTEM.CNF`, verifies all tracked executable facts, and
publishes only into the selected title's gitignored cache. `tests/test_provision_titles.py` covers the
three-title positive route, precedence, invalid configured paths, ambiguity, identity disagreement,
and Crash 3's unrelated `DRAGON/SPYRO.EXE` decoy.

### S003 — independent resident boot comparison

The independent Mednafen CPU and shipping generated path agree 34/34 for each title through its first
post-syscall B0 dispatch. Crash 1 additionally validates manifest-backed B(56h) facts and all three
titles validate Cause/EPC plus EPC+4 resume. Clean psxport `99a42aa3` and the Crash consumer CTest now
agree that B(56h)'s C0 slot 6 contains `0x00000C80`.

Gap: Equality stops before the first B0 HLE call. The real ordered oracle now applies selector-1 return,
B(56h), and the C0 seed in one CPU and proves the fourteen-word copy completed, but its generic
post-model policy stops at local wrapper `0x8004323C`. It cannot yet cross that wrapper's non-link tail
dispatch to A(44h) while preserving the chain (issue #8). Static emission also contains data-like
candidates tracked by issue #3, so candidate count is not execution coverage.

### S004 — Crash 1 shipping product

`crash1_port` now loads the verified executable and enters psxport's direct native boot/frame shell;
the former syscall → B(56h) → A(44h) composition remains a focused diagnostic. A real headless product
launch crossed crt0, static constructors, the retail `Init` path, native libcd software initialization,
and native callback/event initialization. The first launch made the otherwise hidden libcd
`VSync(-1)` caller fatal; a title-local owner now preserves its guest-visible software reset while the
already-native controller remains initialized. The next launch crossed that owner and made the four
`VSync(5)` calls in callback/event/pad initialization fatal; its native owner preserves all 21
state-producing calls and performs no display wait. Both focused production-path tests and their
Clang product build pass. The following launch crossed both owners and exposed `0x80042864` as the
next caller. Ghidra identifies `[0x80042864,0x80042A04)` as the GPU command-queue watchdog: it queried
VSync only for a 240-field timeout while retaining a separate poll-count deadline. Its native owner
now consumes `Timing::vblank`, which only the host frame loop advances, while preserving timeout
diagnostics and GPU recovery; its focused Clang test and clang-tidy pass. A later product launch
crossed the watchdog owner and made libcd command polling fatal at `0x800440EC` through
`0x80044640 → 0x80043520 → 0x8002F8C4 → Init`. Ghidra identifies `0x8002F8C4` as Crash's startup
disc-index reader: it reads the disc directory sectors and builds the game's file-location tables.
Ghidra then identifies the stock-libcd wrappers used by that reader as CdControl `0x80043520`,
CdControlF `0x80043668`, CdRead `0x800439A4`, CdReadSync `0x80043984`, and the stock CdSync
wrapper/body at `0x80043498`/`0x800440EC`.
A title-local routing owner binds those exact entries to psxport's existing synchronous owners; it
does not duplicate Sony libcd semantics. The real-disc product crossed directory construction, the
NSD read, and synchronous page reads without entering guest VSync. Static extraction then falsified
the apparent packed-page failure: the page loader's separate CdControlF body was not bound, so native
reads continued from the stale sequential head. A title ABI adapter now binds that authenticated
body to the existing shared command owner without inventing a result pointer; the focused Clang test
exercises its BCD Setloc and synchronous status contracts. An isolated real-disc run proved `pgid
0x0D` seeks LBA 51468 and delivers the exact extracted NSD entry-6 payload, SHA-256
`ec3f010049a1b98cf0561ce1163c45992ca4b10bcdeb5bd792df46aaad22c937`, after which the retail
decompressor progressed through later pages. The next refusal came from a manifest typo: the driver
checked `0x80036428`, while the authenticated transition body loads the done flag from `0x80056428`.
The corrected manifest now records both the address and its load site, and the executable verifier
decodes that LUI/LW pair. Against pinned psxport `3c342ec3`, a fully isolated PSX-render run then
reconciled 120/120 host frame fences through frame 119, returned normally, and emitted no guest VSync,
fatal, error, or watchdog-timeout line. Presented frames 30, 60, and 119 were captured and visually
inspected as the centered SCEA Presents splash.

Gap: the isolated run covers only the static intro splash, not title progression or gameplay. It must
be extended and driven with input before the product can be called playable. A stable-source native
transform was removed after it advanced the downstream symptom without grounding the missing Setloc.
The earlier independent
B(56h)/A(44h) comparison limitation remains issue #8; direct runtime progress does not retroactively
provide oracle equality.

### S005 — game-owned native renderer submission

Missing capability: the isolated compatibility run reached 120 host frame fences and observed 62,840
primitive records, but earned zero native producer rows or claims. No game-code graphics submitter,
native primitive producer, or renderer submission owner has been identified or implemented. All title
runtimes explicitly declare the required native/interpolation product profile but continue to report
`guestVramIsPicture=false`; capability declaration and frame progression are not a native rendered
frame.

### S006 — widescreen

Missing capability: no owned camera or projection producer exists, so there is no grounded state that
can widen view geometry. Post-projection GTE/OT/GP0 reconstruction is explicitly not ownership.

### S007 — interpolation

Missing capability: no authoritative simulation tick and no owned previous/current camera or object
transform producers exist, so there is no state pair a native renderer can interpolate.

### S008 — Crash 2 and Crash 3 products

Missing capability: Crash 2 and Crash 3 have verified executable identities, typed runtimes, and
pre-B0 differential boundaries, but no shipping player executables or launch routes. Each runtime owns
a refusing title driver and fatal guest-VSync address so a future product cannot silently fall back to
the retail frame wait; that contract does not make either title runnable.

### S009 — playable trilogy

Missing capability: none of the three titles reaches verified gameplay. Crash 1 has isolated
host-frame progression and a visually inspected compatibility splash, but no proven title progression
or native producer; Crash 2 and Crash 3 have no product targets.

### S010 — native frame-loop ownership contract

Partial capability: the three verified executables bind distinct VSync leaves—Crash 1
`0x8003E4F0`, Crash 2 `0x8004A484`, and Crash 3 `0x8004B2A8`—to psxport's sole fatal trap through
title-owned `PlatformHlePlan` values. All three runtimes create non-null title drivers. The executable
gate fingerprints each complete VSync body and carries controls for an altered entry and altered body;
the runtime and death-oriented contract tests compare the shipping plan to those facts and require
frame-step attempts, query and wait modes, and fatal-trap replacement attempts to abort.

Gap: Crash 1 has isolated evidence for 120 host frame fences under the fatal boundary. Crash 2 and
Crash 3 still refuse frame steps and have no shipping products. Issue #9 owns the fatal contract; the
remaining work is title-specific progression and Crash 2/3 boot/loop/service/presentation RE.

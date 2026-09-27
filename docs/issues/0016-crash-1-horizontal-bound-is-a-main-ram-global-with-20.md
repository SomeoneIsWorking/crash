---
id: 16
title: Crash 1's horizontal bound is a main-RAM global with 20 readers, and the literal cull census could not see any of them
status: open
symptom: "S006's widening rests on 'no literal horizontal cull in 72,192 instructions', a scan that structurally cannot see a variable-bound cull; the variable bound had never been enumerated"
state_items: S006, S005
tags: crash1,projection,widescreen,re-census,horizontal-bound
created: 2026-09-27
updated: 2026-09-27
---

## What the gap actually was

`crash1_widescreen.*` widens by moving OFX and holding H, and two measured facts support that:

1. A whole-image `ctc2`/`cfc2` census finds **zero** control-register readers of OFX, OFY or H, so no
   guest branch reads a coprocessor register. Complete census, no hole.
2. A whole-image scan for a compare against a 4:3 dot width finds **no horizontal cull**. This one had
   a hole, and the repository said so: a cull against a *variable* bound carries no immediate and is
   invisible to a scan over immediates.

So "there is no horizontal cull" was an argument from a scan's inability to see, not a measurement.
Issue 0015 recorded that honestly. This entry is the measurement that closes it.

## What was found

**The bound is not a coprocessor register.** The guest keeps it in a main-RAM global, **`0x800578D0`**,
which is *outside* the executable's own text segment: `SCUS_949.00` is 290,816 bytes = `0x800` header
+ `0x46800` text and nothing else, so this is loader-created zero-initialised memory the title reaches
with `lui $reg,0x8005`. That is why the coprocessor census could not see it.

**One writer.** `0x80017820` `sw $v0,0x78D0($at)` (word `0xAC2278D0`) inside the camera setup
`FUN_80017790 [0x80017790,0x80017968)`, sha256 `18742099…`. It is the per-camera-mode H: `0x25`→500,
`0x1E`→960, `0x38`→800, `0x3C`→460, `0x5A`→288, with `gte_init` publishing `0x3E8` = 1000 once.

**The per-frame re-send is a consumer, not a producer.** `0x80026770` `jal 0x80042FAC` (word
`0x0C010BEB`) in `FUN_80026650 [0x80026650,0x80026A40)`, fed by `lw $a0,0x78D0($a0)` at `0x80026764`.
`FUN_80026650` reads the global and re-sends it into CR[26] every frame. The repository already
described `0x80026770` as "republished per frame", which is right, but the ownership was not stated:
the global's writer is one function and its per-frame re-send is another.

**Twenty readers, none carrying an immediate.** Measured by `tools/probe_crash1_horizontal_bound.py`
three ways, each printing its denominator and its own null:

| instrument | sites | what it cannot reach |
|---|---|---|
| A — `lui` + displacement word scan | 17 (1 store, 16 load) | a displacement of zero |
| B — `lw $rt,0($rN)` with a `lui`/`addiu` pair within 6 instructions | 3 | a pair further back |
| C — the manifest's *named* pair | 1 | nothing; it names the words it relies on |

The 17 + 3 + 1 = 21 sites are 1 store + 20 loads. `0x8001DFFC` is the one that needs instrument C: it
is **229 instructions** past the `lui $s5,0x8005` at `0x8001DF64`, so no bounded window reaches it, and
widening a window until it covers a whole function would prove nothing because this title emits a
`lui` of the same page throughout.

**And none of the twenty is a screen-space cull.** The two that turn the bound into a decision:

- `FUN_8003A144 [0x8003A144,0x8003A76C)`, sha256 `d8b81f60…` — the pre-GTE object lighting setup. It
  takes the bound as its 5th stack argument (`0x8003A18C` `lw $v0,0x14($sp)`, word `0x8FA20014`) and
  rejects when `NOT (H < Z)` at `0x8003A240`/`0x8003A244`, then when `11999 < Z` at `0x8003A248`/
  `0x8003A24C`, where `Z` is the projected depth from the `rtps` at `0x8003A220` (`cop2 0x49E012`).
  The band is `H < Z < 12000`: **H is the GTE near-plane distance**, which is what a PSX H is.
- `FUN_8001DE78 [0x8001DE78,0x8001E3D4)`, sha256 `567e1ecd…` — `GoolObjectTransform`, the pre-GTE
  object submitter the repository already grounds as a producer seam. It reads the bound at
  `0x8001DF6C` and `0x8001DFFC` through `$s5`, proved equal to `0x800578D0` by `0x8001DF64`/
  `0x8001DF68`, halves it at `0x8001E008`–`0x8001E010`, and passes
  `(object+0x138) + 0x800 - H/2` to `FUN_8003A76C`, which uses it as
  `param_5*4 + (IR0.r+IR0.g+IR0.b >> 5)*-4` — a **GTE light-intensity offset**.

## Verification

- The Ghidra program this came from was checked against the authenticated executable before any reading
  was relied on: **72,192 of 72,192 text words served and compared, zero mismatches**, executable
  sha256 `aabf1464…` equal to the manifest's.
- Every decision site named above was re-read as an instruction word from the image and compared to the
  executable's own bytes: **29 of 29 matched, 0 mismatches**.
- The two consumer bodies are hash-pinned in the manifest and re-hashed by the probe on every run.
- The probe's own instruments were each shown able to produce the other answer on fixtures built to
  contain one, and the census itself was shown able to fail: a manifest with the writer moved by four
  bytes, and a manifest with a consumer body hash zeroed, each produce a named `FAIL` and exit 1.
- `titles/crash1/executable.json`'s reader list is **diffed against the constants this port ships** in
  `crash1_horizontal_bound.h`, so the two copies cannot drift.

## What this changes, and what it does not

It converts a widescreen safety claim from *argued from a scan hole* to *measured at the consumers*.
Because a widening holds H fixed, all twenty readers keep reading a retail H, and the newly revealed
horizontal geometry cannot be clipped by a cull — because there is none.

It does **not** advance S005. `Crash1HorizontalBound` is a guard on the widening's contract, not a
producer: it reads pre-GTE guest state, runs the retail submitter, and writes no guest state. It has
**not** decompiled the other eighteen reader functions, and it has no live leg: no disc media is
provisioned on this machine, so no run has reached the submitter. Its runtime evidence is the installed
override plus the four submissions its test drives.

`RenderCapabilities` is untouched. `widescreenOnly()` remains the honest profile for Crash 1 — it now
has a measured basis for both halves. `interpolatedNative()` stays blocked on S005: it sets
`nativeRenderPath = true` and `temporalInterpolation = true`, which require a native render path and a
`TemporalSceneSource`, and neither exists. See the report for why `BoundaryRuntime`'s own
`widescreenOnly()` is a different and separate problem.

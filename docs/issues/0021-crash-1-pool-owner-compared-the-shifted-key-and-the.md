---
id: 21
title: "The pool lookup owner compared the SHIFTED key, and the \"0 primitives\" number came from an instrument that cannot see this title"
status: resolved
symptom: "[producers] run-end: OtAttr spans recorded 0 (overflow 0) was read as \"the guest submits no primitives\", and the block-pool owner reported five unservable classes in a row"
state_items: S005,S006,S011
tags: crash1,re,block-pool,class-word,dead-instrument,primdump
created: 2026-09-28
updated: 2026-09-28
---

## The premise of the question was false, and the instrument that produced it said nothing

Issue 0020 closed with "the run completes, and the guest reaches its measured display wait and submits
**ZERO primitives**", on the strength of one line:

    [producers] run-end: OtAttr spans recorded 0 (overflow 0) — the guest leg's feed

**That line is a dead tap for this title, and the guest is not silent.** Measured with the framework's
own GP0 packet dump, same binary, same disc, 400 frames:

    PSXPORT_PRIMDUMP=0:399 ./build/agent-clang/crash1_port
    [primdump] wrote scratch/logs/prims_f0.csv — 238624 prims over frames 0..399

`scratch/logs/prims_f0.csv` holds 238,624 data rows (915 lines includes the header) over 400 frames —
1 prim on frame 1, ~528 per frame through frame ~350, ~900 per frame from ~frame 355 on. By command
byte, over all 238,624: **210,144 `0x7C` sprites, 28,436 `0x30` Gouraud polygons, 44 `0x2A` textured
Gouraud polygons, 0 of anything else.** Screen extent x −323..1622, y −104..288.

Why the span feed reads 0: `OtAttr` attributes guest stores by asking `RenderNoiseMask::from(c->cfg)`
for the game's packet-pool window, and that window comes from `LegacyGameConfig::packetPoolBase` /
`packetPoolStride` (or the `packetPoolBasePtrs` pair). **Crash 1 is a typed `GameRuntime` and declares
neither**, so the mask is empty for every store the guest makes. The framework KNOWS this — see
`psxport/runtime/psx/ot_attr.cpp:110-121`, whose own text is "packet-pool attribution is STRUCTURALLY
BLIND here, so an empty span table means 'not measured', NOT 'the guest submitted nothing'" — but that
warning is emitted only from `pool_range_uncached`, which nothing on this path reaches, so the run-end
line in `runtime/psx/native_boot.cpp:286` prints a bare `0` with no caveat. **This is the same class of
defect the workspace map already records for `is3d`**: a metric reading a tap nothing writes returns a
confident answer about the wrong subject, and the zero it returns is the most believable possible
output. Naming the seam as the issue itself asks: `psxport/runtime/psx/native_boot.cpp:286` and
`psxport/runtime/psx/ot_attr.cpp:110`. Nothing in the framework needs changing for the pool or for the
frame boundary.

## The real defect: the class is the WHOLE request word, and the owner compared the shifted key

`crash1_block_pool.h` recovered the lookup at 0x80015978 as

    key  = request >> 13
    if (cell->class == key) return cell;

from the entry instruction `srl $v0,$a0,13` and the header's own transcription of it. **The image says
otherwise**, in four places that do not depend on each other:

| what | where | what it says |
|---|---|---|
| the first-cell test | `0x8001599C` `beq $2,$4` | the cell's class field is compared against **`$a0`**, the request, whole |
| the walk's back edge | `0x800159B0` `bne $2,$4` | and again, and `$a0` is not redefined anywhere between the entry and it |
| the bounded sibling | `0x800159E8` `beq $2,$4` | the same rule in the form at 0x800159C4, which is not called |
| the pool's own allocate path | `0x80012FFC` `lw $3,4($5)` vs the cell class at `0x80012FF8` | the engine writes and compares the **unshifted** word, and the shift at `0x80012FC8` only picks the bucket |
| the call-then-store | `0x8001313C` / `0x80013140` | the lookup is called with `*(cell+0x18)` and the **delay slot** stores the class word into `cell+0x14` — the engine stores the word it is about to ask for |

So the shipped owner could never match a cell the engine itself wrote. Every lookup ran off the end of
main RAM and returned a cell at `0x80200000`; the owner reported it correctly and the log said "left
main RAM", so the instrument was honest and the CODE was wrong.

### The fix and what it measured

`searchCells` now takes the whole `request`, and `findCell` masks the shift into a bucket offset that
never reaches the comparison. Controlled pair, same binary, same disc, `--frames 400`:

| run | unservable classes | executed blocks | executed instructions | `fallback_blocks` |
|---|---|---|---|---|
| before | **5**, all from `ra=0x80015164` | 1,689,262 | 26,457,241 | 0 |
| after | **0** | 3,461,249 | 46,438,388 | 0 |

`tools/probe_crash1_block_pool.py` grew the check that keeps this honest: it now decodes the register
operands of all three class comparisons and fails unless each is `$a0`, decodes the `srl`'s
source/destination and fails unless it is `srl $v0,$a0,13`, and scans the words between each entry and
each branch for a write to `$a0`. Its selftest builds the WHOLE-word and SHIFTED-key fixtures and
requires the same decoder to accept one and reject the other — 12 of 12 cases fire. The previous
selftest agreed with the misreading because it was written from it.

### The one line that settles it

The owner now reports its own first lookup and its first served cell, with the run's denominators,
because "no pool error appeared in this log" is otherwise indistinguishable from "the owner never
ran" — the same dead-instrument failure as the span count. From the disc-backed 400-frame run:

    [crash1-pool] the first lookup of this run — request 0x0057CCFB (bucket byte offset 700), bucket
      table 0x80061A80, first cell 0x80062EF0, engine base 0x80061FA0 publishing 576 live cell(s);
      1 of 1 lookups served so far, caller 0x80015164
    [crash1-pool] the first lookup that found a cell — request 0x0057CCFB (bucket byte offset 700),
      cell 0x80062EF0 after 0 cell(s) walked, first cell of the bucket 0x80062EF0; 1-of-1 lookups
      served have found a cell, 0-of-1 left main RAM, caller 0x80015164

**0 cells walked.** The same request previously walked 211,490 cells to the edge of main RAM and found
nothing, because it was searching for `702` instead of `0x0057CCFB`.

## A prior claim, falsified by measurement

`crash1_block_pool.h` used to assert that the engine "can only ever" call the lookup with a key that is
a multiple of four, because the bucket index is a byte offset into a word-strided table. Four requests
captured from the live run index **unaligned** buckets: `0x0057CCFB` → 700, `0x15814CE7` → 8,
`0x5452D94D` → 660, `0x4E938CCD` → 156. psxport's word accessor tolerates them. The claim is deleted and
replaced by the measurement in `tests/crash1_block_pool.cpp`; what a retail CPU does with them is
**not** established and cannot be from a host run.

## The unit of the class word is still not established

It is a 32-bit word whose top 19 bits select one of 256 buckets and whose low 13 bits are part of the
identity — two requests differing only below bit 13 share a bucket and are two different classes. The
four measured words are not byte counts, sizes, or handles this repository can name. The model and both
owners therefore name the field for what it is COMPARED against and for nothing more.

## What 0x80012F10 turned out to be

Issue 0020 called 0x80012F10 "the pool's INITIALISER" and left it unrecovered. It is code, not data
(`0x80012F10` is `0x27BDFFC0`, `addiu $sp,$sp,-0x40`), and it is the pool's **node allocate path**:
259 words, `0x80012F10..0x8001331C`, sha256 `217a4cd9…30dce`, reached by `jal` from exactly two sites
(`0x8001402C`, `0x800141A4`). It takes a 44-byte node from the table at `0x8005C554` (clearing the
cursor at `0x8005CFAC` when the cursor already names that node), dispatches on the node's halfword kind
at `+0x02` into five cases, and in the kind-0 case walks a bucket for the class word, links the matching
cell, bumps the node's live counter at `+0x0A`, and calls the per-type callback table at `0x800514EC`
with a 28-byte stride. It is now `titles/crash1/core/crash1_pool_node.{h,cpp}`, a readable model over
an injected memory seam, gated by `tools/probe_crash1_pool_node.py` (12 of 12 selftest cases) and
`tests/crash1_pool_node.cpp`, and recorded in `titles/crash1/executable.json` under `runtime.pool_node`.

**It is not installed as a native override, and this issue claims no runtime behaviour from it.** Two
reasons, in order: it is 259 instructions across five cases making three guest calls plus an indirect
`jalr`, so a faithful override is a porting job rather than a transcription; and nothing yet measures
the guest REACHING it — in the disc-backed run every pool lookup came from `ra = 0x80015164`
(`FUN_80015118`), never from `0x80013140`. Owning a function the run does not execute would be a claim
with no evidence behind it.

## A tool defect found on the way, and worked around rather than trusted

`scratch/mipsdis.py` wraps the text in a minimal ELF whose program header sets `p_offset = 0`, so
`llvm-objdump` maps the section header bytes as code. Every window it printed was displaced, and its
output at a real function entry was `<unknown>`. That is what produced the first, wrong reading of
0x80012F10 as data. The correct disassembly used for this recovery came from wrapping the **whole** text
with `p_offset = ehsize + phentsize`, and **every** address in the recovery above was cross-checked
against `struct.unpack` of the raw file bytes at `0x800 + (addr - 0x80010000)`. `scratch/` is
gitignored and untracked; the durable fix belongs to whoever owns that scratch tool.

## Not established, stated so it is not read as a result

- **Any picture, or any widening.** Both legs still exit 0 with `render_width == native_width` on the
  4:3 leg, and the 16:9 leg never reaches its per-frame `SetGeomOffset` at `0x80017F00`, so
  `probe_crash1_widescreen_legs.py` still exits 1 on that NOTE. Nothing here changes S006.
- **That the guest reaches the title screen.** 400 frames of submission is not a scene, and the prims'
  x extent reaching 1622 on a 320-wide canvas is unexplained.
- **That the pool's cells are correct**, only that every lookup the run made found a cell. The owner
  reports `1-of-1 found a cell, 0-of-1 left main RAM` on its first lookup and every later walk that
  leaves main RAM, but there is still no framework shutdown hook for a title to print the run's TOTAL
  `lookups/found/leftMainRam`, so the denominators grow with the run rather than closing on it.
- **What 0x8001402C and 0x800141A4 are**, and therefore when the node allocate path runs.

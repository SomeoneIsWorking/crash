# 0015 — Crash 1's projection is GTE OFX, and the widescreen owner is built; two things are left open

**State:** owner built and gated; the wide-leg live measurement and one framework question are open.
**Opened:** 2026-09-27. **Scope:** `titles/crash1/**`, `game/**`, `docs/**` in this repository only.

## What Crash 1's projection actually is

Not `H`/`OFX`/`OFY` as an authored triple, and not a viewport rectangle. It is the **GTE screen offset
`OFX`, cop2 control register 24**, and the retail value is **zero**.

Beetle's `gte.c` names `CR[24]=OFX`, `CR[25]=OFY`, `CR[26]=H`, and `RTPS` computes
`h_div_sz = Divide(H, Z_FIFO(3))` then `TransformXY(h_div_sz, ...)`, so a projected point lands at
`SX = OFX + (H * IR1) / SZ` — `OFX` is the pixel centre and `H` is the scale. Crash 1 publishes
`OFX = 0`, and the centring that makes the picture sit inside the display lives in the title's own
3x3 view matrix, published into the GTE translation vector through `SetGeomTranslation` `0x80042C2C`
and the matrix leaves `0x80042E9C` / `0x80042ECC` / `0x80042EFC`.

### The complete owner census (denominator: 72192 instruction words)

`tools/probe_crash1_projection.py` re-derives all of this from the authenticated image and refuses
when the image disagrees. Ranges are recorded in `titles/crash1/executable.json`
(`runtime.projection`).

| GTE control register | control writers | control readers |
|---|---|---|
| `CR[24]` OFX | **2** — `0x80042B88` (gte_init), `0x80042F94` (`SetGeomOffset`) | **0** |
| `CR[25]` OFY | **2** — `0x80042B8C`, `0x80042F98` | **0** |
| `CR[26]` H | **2** — `0x80042B68`, `0x80042FAC` (`SetGeomScreen`) | **0** |

886 opcode-`0x12` (COP2) words: 241 `mfc2` data reads, 15 `cfc2` control reads, 259 `mtc2` data
writes, 242 `ctc2` control writes, 129 GTE commands.

**The zero readers are load-bearing.** No guest branch tests `OFX`, `OFY` or `H`, so moving `OFX`
cannot flip a decision the title makes. The three `mfc2 $x,0xC000` sites at `0x800347B4`,
`0x80034CF0` and `0x80035194` *look* like `OFX` readers and are not: they are `mfc2` **data** reads
of GTE data register 24, a reserved slot that always reads zero, and the `beq $sp,$zero` after each
is a test of that reserved slot.

### Retail 4:3 values and where they come from

- `gte_init` `0x80042B1C` (`li $t0,0x155` / `0x100` / `0x3E8` / `-0x1062`, `lui $t0,0x140`,
  `ctc2 $zero,0xC000`, `ctc2 $zero,0xC800`): **`H = 0x3E8` (1000), `OFX = 0`, `OFY = 0`**, plus
  `ZSF3 = 0x155`, `ZSF4 = 0x100`, `DQA = 0xEF9E`, `DQB = 0x01400000`. One `jal` call site,
  `0x80016558`, inside `Init` `0x8001652C`.
- `FUN_80017790` `0x80017790` republishes `H` per camera mode from
  `*(int *)(DAT_8005C53C + 0x114)`: `0x25 -> 500`, `0x1E -> 960`, `0x38 -> 800`, `0x3C -> 460`,
  `0x5A -> 288`, into the global `DAT_800578D0`, and `SetGeomOffset(0, 0)` alongside it.
  Call sites `0x80017830` (`H`) and `0x8001783C` (centre).
- `FUN_80017A14` `0x80017A14` calls `SetGeomScreen(DAT_800578D0)` at `0x80026770` **every frame** and
  `SetGeomOffset(DAT_8006193C, (DAT_80061894 >> 8) + DAT_80061940)` at `0x80017F00` **every frame**.
  `DAT_8006193C` is a live camera-shake word, so the centre the title publishes is not a constant.

### The draw area, and the null that finds it

The guest writes GP1 `0xE1` (draw area) in exactly **one** place, `FUN_80042A04` at `0x80042A58`,
and the value is `(GPUSTAT & 0x3FFF) | 0xE1001000` — origin `(0, 4)` with width and height zero,
which on a PSX **is the whole display area**. Measured consequence: this title has no separate clip
rectangle, so there is no second rectangle for a widening to move, and the single horizontal extent
it does publish is the display extent it sends through GP1 `0xC0`.

That publisher, `FUN_80041C38`, has **zero direct `jal`/`j` call sites in the whole image**. It is
entry index 7 of the GPU driver function-pointer table at `0x80054A24` (18 guest-code words,
`0x80054A24..0x80054A64`, with the end marker at `0x80054A6C`). An address-target scan cannot reach
it; a resident-word scan finds it immediately. `tools/probe_crash1_projection.py` checks both
directions and fails if the table reachability goes stale.

### The cull census, with its null

Over all 72192 instruction words, every immediate-bearing signed compare (`slti` / `sltiu`),
`andi` / `ori` and `addiu` against a 4:3 dot width or a PSX visibility/pad mask:

- **320** appears **twice**, both `addiu $sp,$sp,+/-320` stack frames (`0x80024034`, `0x8004E32C`).
- **319, 318, 352, 368: zero sites.**
- 288 and 384 appear as four- and eight-times 3-bit bitmasks (`0x120`, `0x180`), not widths.
- The single `0xBFFF` PSX visibility-mask site is `0x80016824`, inside `FUN_800167A4` calling the BIOS
  `PadRead` leaf `0x8003E460` — it is the **active-low digital pad button mask**, classified and
  excluded.

**Null, stated rather than hidden.** A cull against a *variable* bound (a global, a viewport record)
carries no immediate and is invisible to this scan. The tool counts what it scanned and what it
matched; it does not claim the image contains no variable-bound cull. What it does establish is that
there is no *literal* 4:3 horizontal compare to widen, and that the picture's horizontal extent is
owned entirely by the GTE projection plus the PSX default draw area.

### Three scan traps this title reproduces

1. **Ghidra's reference model** reports **0** references to `SetGeomScreen` `0x80042FAC` and 2 to
   `SetGeomOffset` `0x80042F8C`. Each leaf has exactly 2 `jal` call sites.
2. **No gp-relative addressing.** Crash 1 is compiled with globals at `lui $r,0x8005` + a 16-bit
   offset — measured on the BIOS `PadRead` result cell `0x80057054`, reached by
   `lui v0,0x8005; lw v0,0x7054(v0)` at `0x8003E470`. A gp-relative reference scan therefore returns
   zero for every global in the image, including ones the title's own verified owners name.
3. **The GTE cop2op layout.** `ctc2 $rt, <cop2op>` writes control register `<cop2op> bits 11..15`,
   and the transfer class is `bits 21..25` — not the immediate's low five bits. Reading the register
   from the low bits reports "`OFX`/`OFY`/`H` are never written anywhere", which is exactly wrong.

## What was built

`titles/crash1/core/crash1_widescreen.{h,cpp}`, wired from `Crash1Runtime::registerOverrides` and
returned from `Crash1Runtime::guestWidescreenProjection()`.

The widening is **`OFX' = retail_OFX + projectionHorizontalMargin`, `OFY' = retail_OFY`, `H' = retail_H`**.
With `SX = OFX + H*x/z` and the guest's own centring baked into its matrix, the world half-width
visible at depth `pz` is `(displayWidth/2 - OFX) * pz / H`, so a 428-wide canvas holding the same 4:3
frame at its original pixel scale is the same equation with `OFX = (428-320)/2 = 54` and `H`
unchanged. Substituting a smaller `H` would be a zoom, and this owner never touches `H` at all —
`SetGeomScreen` is overridden only to record the value the guest published, which is what turns
"never touches `H`" from an intention into a checked fact.

Three properties worth naming:

- **No per-frame host re-publish.** The guest itself calls `SetGeomOffset` every frame
  (`0x80017F00`), so the publication site *is* per-frame. There is no frame-boundary guest call and
  no direct write to a coprocessor register from the host.
- **Idempotent by construction, not by a guard.** The widening base is the `$a0` argument as it
  stands right now, and the coprocessor register is never fed back into it, so repeated publication
  cannot compound. (The first revision used a *captured* baseline and the test caught it: `DAT_8006193C`
  is live, so yesterday's value is history, not retail.)
- **4:3 identity is exact by construction.** The margin is zero, so the title's own argument reaches
  the leaf untouched.

## Still open

1. **The wide leg cannot be measured on this machine**, for three independent reasons, all quoted
   from the product's own log rather than inferred:

   a. **No disc media is provisioned.** `[disc:warn] no disc image: tried PSXPORT_CRASH1_DISC,
      PSXPORT_DISC (env and ./.env), and a *.chd drop-in in the working directory. The CD model will
      run with NO MEDIA.` → `[native-dispatch:error] unimplemented BIOS A0:0x27` →
      `[crash1-frame:error] frame 0 left guest execution at 0x000000A0 with fault after 2252 cycles`.
      The product therefore never reaches its per-frame `SetGeomOffset` at `0x80017F00`, so the
      widened centre has no live leg. This is a missing user asset, not a failing owner.

   b. **The only `[wide]` line in a run predates the plan latch, and nothing re-announces after it.**
      `picture_announce` prints only on CHANGE. In the 16:9 leg the announce is at `08:42:57.869` and
      the guest's own publication is at `08:42:57.994` — 125 ms later — so the steady state was never
      announced. This is exactly the trap the workspace already records for Tomba! 1, and the reason
      the log TAIL is quoted rather than a mid-run sample.

   c. **`picture_announce` would classify the widening as `RefusedPure` even after the latch.**
      `runtime/psx/picture_announce.cpp:69` calls
      `classifyWide(now.aspect, core.rsub.mode.enhancementsAllowed(), ...)` and
      `render_mode.h:120` defines `enhancementsAllowed()` as `mPath == RenderPath::Native`. A
      `widescreenOnly` title's path is **Gte**, so that gate is always false and the classifier
      returns `RefusedPure` — quoting the product:
      > `[wide:warn] a wide picture was REQUESTED and did not happen: render_width=320 is not greater
      > than native_width=320 because this Core's render mode is PURE, where no PC enhancement may
      > touch the picture; any widescreen claim from this run is void`

      The presentation contract is explicit that the broad `enhancementsAllowed()` gate "remains
      Native-only" and that guest widescreen "does not relax it" — so the *gate* is correct, but using
      it to judge whether a **guest**-widescreen widening happened cannot ever be true on `Gte`. That
      is a framework classification defect. Reported, **not** worked around: flipping Crash 1 to
      `RenderPath::Native` to silence the warning would be exactly the "forcing a plan to look
      applied" this work refuses, and would hand the title an enhancement path it does not have.

   The evidence that *does* exist, and it is the guest's own state rather than a config value: the
   real boot publishes `H 1000, OFX 0, OFY 0` from the guest's registers in **both** legs, and the
   latched plan widens with the requested aspect — `host canvas 428 (native 320)` in the 16:9 leg
   against `host canvas 320 (native 320)` in the 4:3 leg. That horizontal change is read from the
   framework's own latch at the guest's own publication site.
2. **Framework question, not worked around.** `gpu_vk.cpp:342` widens the *host picture* to
   `plan().presentationExtent.width` (428) while the guest's own GP1 display extent stays 320. Whether
   the guest's rasterizer honours the host canvas width is a framework display/present-policy
   question. Reported, not fixed — this repository does not edit `external/psxport`.
3. **Pre-existing, not caused by this work: `crash_dynarec_dispatch` fails.** psxport changed the
   unstamped-exit contract — `runtime/cpu/execution_control.cpp:29` now passes `guestPc = 0` with
   "the consumer supplies the correct continuation", because stamping `core.pc` "is wrong for every
   request raised inside a `jal`ed native override". `tests/dynarec_dispatch.cpp:81` still asserts
   the old `frame.guestPc == kFrame` and receives `kTurn + 8`. `crash1_frame_driver.cpp:250` asserts
   the same old stamping. Both are the frame-loop contract owner's decision, not this issue's.
   (`crash_native_frame_contract`, issue 0014, now passes — the same framework change fixed it.)

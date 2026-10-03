# 0025 — the recorded `set_geom_offset` call sites are mis-decoded, and `$r31` names no call site

## The situation

Merging Crash 1's widescreen owner onto the shared `crash::GuestProjectionPublication` applied the
shared rule's per-call-site policy to this title for the first time, and a live disc-backed leg
aborted in the first seconds:

    [crash1-wide:error] set_geom_offset override was reached from 0x80017840, which the title
    manifest does not name; this repository will not widen a call site whose argument provenance it
    has not measured

The shared rule recovers a call site as `$r31 - 4`, which is exact for a `jal`. Here `$r31` is
`0x80017844`, so no `jal` in the image reaches the leaf that way.

## What the bytes say

Decoding every instruction word of the authenticated executable (`scratch/bin/crash1/SCUS_949.00`,
header 0x800, text 0x80010000 + 0x46800, words little-endian in this dump):

- No `jal` anywhere in the image targets `0x80042F8C`, and none targets `0x80042FAC` or `0x80042B1C`.
- The leaf's address appears as no resident word, in either byte order, so it is not in a function
  pointer table the image carries itself.
- The instruction at `0x8001783C` — recorded in `titles/crash1/executable.json` as a call site, and
  quoted as `jal 0x80042F8C` in docs/issues/0023 — decodes as `jal 0x8005A7CC`, an address inside this
  title's own bss (`0x80056598..0x80061A78`). The recorded call site is therefore a mis-decode, not a
  call.

So the leaf is reached indirectly, `$r31` carries the enclosing call chain's return address, and the
`$r31 - 4` recovery has nothing to recover here. The same is true of the two other Crash titles: no
`jal` in SCUS_941.54 or SCUS_942.44 targets its own `set_geom_offset` leaf either, so Crash 2's and
Crash 3's `centreCallSites` lists are unproven on the same grounds.

## What changed now

`ProjectionTitleFacts` gained `CentreReach`, because a title's reach decides whether `$r31` means
anything:

- `ReturnAddressCallSites` keeps the existing behaviour and still refuses a site the title did not
  name. Crash 2 and Crash 3 keep this reach, so nothing about them changes yet.
- `IndirectCall` widens every reach and never reads `$r31`. Crash 1 declares it, which is exactly the
  widening behaviour its owner had before this rule was applied to it, and it is sound for this title
  because the image has no `cfc2` control read of CR[24]/CR[25], so no argument can arrive already
  carrying the margin.
- A title that declares `IndirectCall` together with a pass-through list is refused at construction:
  such a list cannot be honoured, because `$r31` cannot select from it.

## What is NOT established

- HOW the guest reaches the leaf. The camera setup at `0x80017830`/`0x8001783C` jumps into the
  0x8005A7xx region, and the argument the leaf receives in a live run has not been traced to its
  producer. That is RE work, not a structure decision.
- Crash 3's pass-through decision. Its read-back at `0x8004F6E4` is a real measurement, but if the
  leaf is reached indirectly there too, its `kPassThroughCallSites` cannot be selected by `$r31` and
  the widening decision for that title needs a discriminator that does not depend on the return
  address. Crash 3 cannot boot yet, so nothing has run it either way.
- `titles/crash1/executable.json`'s `_call_sites` block still records the mis-decoded addresses. It is
  evidence, so it is left for the RE pass that owns the census rather than edited from here.
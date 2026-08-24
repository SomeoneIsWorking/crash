---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`tools/resident_recomp.py` behind the serial-scoped `tools/crash1_recomp.py`,
`tools/scus_94154_recomp.py`, and `tools/scus_94244_recomp.py` entry points, plus the title-specific runners over
`tests/recomp_boundary_support.h` — emits a verified title's resident candidates, consumes psxport's
canonical independent-oracle ordinal call captures, captures the same generated-call states, and
compares complete register files without sharing title addresses.

## Validated by

The shared oracle selftest passes 8/8; it proves that calls one and two capture distinct targets,
the first-call alias selects ordinal one, a capture contains all 33 canonical registers, and missing
call three refuses without a boundary block. On the real verified USA executable the consumer
reported port/oracle agreement 34/34 at calls one `0x80011A18`, two `0x80011D88`, three
`0x8003E0C0`, four `0x8001652C`, five `0x8003F224`, six `0x80042B1C`, seven `0x8004319C`, and
eight `0x8003E1F8`. The clean runner constructs the framework's whole `Game`, so generated tick
accounting reaches an owned `Timing` subsystem instead of dereferencing a bare `Core`'s null owner.
Its production emitter refused an explicit seed outside executable text. A real oracle trace capped
immediately before call eight refused with only `7/8` boundaries. Its production state comparator
reported exactly one named mismatch when fed the real eighth port capture with `a0`
changed by one bit, proving both missing-boundary and disagreement answers rather than only the
all-equal answer.

The same shipping runner has a controlled syscall mode. On each real executable it validates the
eighth target's `addiu $a0, 1; syscall 0` words from loaded guest memory, executes the generated body,
and records selector `1`, return value `1`, IRQ delivery `1 -> 0`, and the matching COP0 IE-bit clear.
Pointing that mode at the seventh execution-proven function refuses before executing the syscall,
proving the discriminator can produce the opposite answer.

On real `SCUS_942.44`, emitter `2026-08-22.1` produced 986 candidates from 297 seeds. Generated and
oracle states agreed 34/34 at all eight calls: `0x800112B8`, `0x80011628`, game main
`0x80048AA0`, `0x800154AC`, `0x8004BFBC`, `0x8004F37C`, `0x8004F914`, and syscall wrapper
`0x80048C38`. The short trace refused at 7/8, an altered register produced one disagreement, and a
different execution-proven target refused syscall execution. The same refactored implementation then
reproduced all eight `SCUS_949.00` 34/34 comparisons and its existing syscall result.

On real `SCUS_941.54`, emitter `2026-08-22.1` produced 998 candidates from 270 seeds. Generated and
oracle states agreed 34/34 at all eight calls: `0x8001144C`, `0x800117BC`, game main
`0x80049BD4`, `0x80015614`, `0x8004B1B8`, `0x8004EC30`, `0x8004F1F8`, and syscall wrapper
`0x80049D1C`. The same negative controls refused a 7/8 short trace, an out-of-text seed, an ambiguous
repeated target, and a different function requested as the syscall wrapper; flipping one captured
register produced exactly one named disagreement.

## Known failure modes

The symbolic crt0 decoder selects only the expected first target. In `SCUS_942.44` that structural
`libcInit` slot is not an A(39h) thunk, so the A0 modeled-return mechanism does not apply. The shared
oracle recovers all
later targets from actual execution; the consumer does not reimplement `jal` or delay-slot tracking.
The target-override runner refuses repeated targets because it cannot distinguish two ordinal
occurrences at the same address. It captures function-call boundaries, not every guest instruction.
The modeled syscall return is deliberately narrow: only the expected selector may resume, and the
first post-return comparison stops before the B0 HLE mutates state. It therefore certifies Cause/EPC,
one Status mode-stack pop, EPC+4 resume, and the first B0 dispatch state, not the B0 service effect,
remaining emitted candidates, later boot, devices, overlays, or gameplay. Static seed and
emitted-candidate counts remain discovery denominators.

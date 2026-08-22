---
id: I005
kind: instrument
status: trusted
created: 2026-08-21
---

## Instrument

`tools/crash1_recomp.py` plus `tests/crash1_recomp_boundary.cpp` — emits Crash 1's resident
candidates, consumes psxport's canonical independent-oracle ordinal call captures, captures the same
generated-call states, and compares complete register files.

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

The same shipping runner now has a controlled syscall mode. On the real executable it validates the
eighth target's `addiu $a0, 1; syscall 0` words from loaded guest memory, executes the generated body,
and records selector `1`, return value `1`, IRQ delivery `1 -> 0`, and the matching COP0 IE-bit clear.
Pointing that mode at the seventh execution-proven function refuses before executing the syscall,
proving the discriminator can produce the opposite answer.

## Known failure modes

The symbolic crt0 decoder selects only the expected first target. The shared oracle recovers all
later targets from actual execution; the consumer does not reimplement `jal` or delay-slot tracking.
The target-override runner refuses repeated targets because it cannot distinguish two ordinal
occurrences at the same address. It captures function-call boundaries, not every guest instruction,
and this result covers only entry-through-eighth-call equality. The port-side syscall behavior is a
separate controlled result: the independent oracle enters `0xBFC00180` and currently cannot expose
Cause/EPC or resume the syscall at EPC+4, so the instrument does not claim post-syscall equality. It
does not certify the remaining emitted candidates, later boot, devices, overlays, or gameplay. The 115
static seeds and 653 emitted candidates are discovery denominators; only nine addresses currently
have execution provenance.

# Crash Bandicoot

The selected target is the North American NTSC-U release, serial `SCUS-94900`, executable
`SCUS_949.00`. `executable.json` records the identity and PS-X EXE header measured from the real image;
`tools/verify_executable.py` compares that tracked record to the bytes.

`recomp_seeds.json` deliberately adds no guessed addresses: psxport roots discovery in these real
executable bytes. The current asset-gated gate expands 115 static candidate seeds into 653 emitted
candidates and proves the generated path agrees with the independent oracle on all 34 CPU-state
fields at each of the first eight executed calls, from `0x80011A18` through `0x8003E1F8`. Every
target is recovered from execution, not entered in the seed file.

This is an entry-through-eighth-call recompile boundary, not a runnable port. The 653-candidate
emission count is not a code-correctness denominator: only the entry and eight call targets have
execution provenance so far. The boundary runner installs the direct, title-owned `Crash1Runtime`;
that runtime deliberately has no legacy configuration view or invented host behavior and refuses
native boot until the next BIOS transition is modeled. Later resident code, BIOS, hardware,
overlays, and gameplay remain unverified.

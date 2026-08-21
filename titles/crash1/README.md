# Crash Bandicoot

The selected target is the North American NTSC-U release, serial `SCUS-94900`, executable
`SCUS_949.00`. `executable.json` records the identity and PS-X EXE header measured from the real image;
`tools/verify_executable.py` compares that tracked record to the bytes.

`recomp_seeds.json` deliberately adds no guessed addresses: psxport roots discovery in these real
executable bytes. The current asset-gated gate expands 115 static candidate seeds into 653 emitted
candidates and proves the generated path agrees with the independent oracle on all 34 CPU-state
fields at the first call, `0x80011A18`, and the next executed call, `0x80011D88`. The second target
is recovered from execution, not entered in the seed file.

This is an entry-through-second-call recompile boundary, not a runnable port. The 653-candidate
emission count is not a code-correctness denominator: only the entry, first callee, and next call
target have executable provenance so far. Later resident code, BIOS, hardware, overlays, the game
seam, and gameplay remain unverified.

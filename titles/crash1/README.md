# Crash Bandicoot

The selected target is the North American NTSC-U release, serial `SCUS-94900`, executable
`SCUS_949.00`. `executable.json` records the identity and PS-X EXE header measured from the real image;
`tools/verify_executable.py` compares that tracked record to the bytes.

`recomp_seeds.json` deliberately adds no guessed addresses: psxport roots discovery in these real
executable bytes. The current asset-gated gate expands 115 binary-discovered seeds into 653 emitted
candidates and proves the generated entry path agrees with the independent oracle on all 34
CPU-state fields at the first call, `0x80011A18`.

This is an entry-to-first-call recompile boundary, not a runnable port. Later resident code, BIOS,
hardware, overlays, the game seam, and gameplay remain unverified.

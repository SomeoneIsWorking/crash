# Crash Bandicoot

The selected target is North American `SCUS-94900`, executable `SCUS_949.00`.
`executable.json` owns its identity, PS-X EXE header, frame, BIOS, and input facts; the verifier
compares those facts to the user-supplied bytes.

Recorded independent execution reaches the first eight calls through the
`EnterCriticalSection` wrapper `0x8003E1F8`, records Cause `0x20` and EPC `0x8003E1FC`, resumes at
`0x8003E200`, and agrees 34/34 at B(56h) with `ra=0x800431B8`. Retail disassembly of
`[0x8004319C,0x80043248)` grounds the following C0 slot-6 read, fourteen-word copy, and A(44h)
tail-dispatch with `ra=0x800431E8`. The independent chain stops at local wrapper `0x8004323C` before
that non-link tail dispatch; issue 0008 retains the exact gap.

The preserved compatibility route later reached 1,172/1,172 fields and the 3D title menu. Issue 0012
grounds the BIOS auto-pad word at `0x80057054` and the in-flight publisher that must run before
retail `PadUpdate`. Live evidence identifies `GfxUpdateMatrices 0x80017A14` and
`GoolObjectTransform 0x8001DE78` as pre-GTE ownership candidates.

These are frozen migration facts, not a static-product contract. New work maps the authenticated
executable into psxport's Lightrec executor and preserves the same native owners and menu/PadRead
frontier. Do not emit, build, or run generated guest code. See `../../docs/migration.md`.

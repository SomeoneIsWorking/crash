# RE frontier

Ordered reverse-engineering dependencies for the Crash trilogy. A title advances independently until
cross-title measurements prove that an implementation belongs in shared `game/`; directory shape is
not evidence of shared ownership.

Statuses: ✅ `re-verified` · 🟡 `re-partial` · 🔬 `in-progress` · ⬜ `todo` · ⏸ blocked upstream ·
➖ `skip-by-design` · ⚠️ `hack`.

## Target identity

### CRASH1-01 — select and measure the Crash Bandicoot executable
- status: re-verified
- deps:
- evidence: Real `SCUS_949.00` measured as 290,816 bytes, SHA-1 `b2af088a162e046ad07f532d46b655cfcc42a05c`, SHA-256 `aabf1464f90b2e0b81e712b77aebbdb88f303b16ce830535e2b0cd886ee280f2`. Its PS-X EXE header declares entry `0x8003E018`, text `[0x80010000,0x80056800)` (`0x46800` bytes), gp `0`, and stack `0x801FFFF0`; the entry lies inside the declared text. Embedded strings `Sony Computer Entertainment Inc. for North America area` and `BASCUS-94900` establish the North American `SCUS-94900` target. `python3 tools/verify_executable.py --check --exe <SCUS_949.00>` matches 11/11 tracked facts; `--selftest` passes 4/4 including wrong-manifest, mutated-image, and malformed-corpus cases.
- where: `titles/crash1/executable.json`, `tools/verify_executable.py`
- gap: This proves executable identity and header only. The disc is not provisioned, disc provenance is not checked, and no game code has been recompiled or run.
- notes: The executable remains outside git. The manifest contains only derived metadata and embedded text evidence.

### CRASH2-01 — select and measure the Crash Bandicoot 2 executable
- status: todo
- deps:
- evidence:
- where: `titles/crash2/`
- gap: Region, serial, executable identity, hash, entry and load extent are unmeasured.
- notes: Do not inherit Crash 1 addresses or assume the same executable layout from engine lineage.

### CRASH3-01 — select and measure the Crash Bandicoot 3 executable
- status: todo
- deps:
- evidence:
- where: `titles/crash3/`
- gap: Region, serial, executable identity, hash, entry and load extent are unmeasured.
- notes: Do not inherit another title's addresses.

## Provisioning and oracle

### CRASH1-02 — provision the selected disc without tracking copyrighted data
- status: todo
- deps: CRASH1-01
- evidence:
- where: `titles/crash1/`, `tools/`
- gap: No CLI/environment/drop-in disc resolver or extraction path exists for Crash 1.
- notes: Resolution must follow CLI argument, environment, `.env`, then drop-in image and refuse an invalid configured path rather than silently falling through.

### CRASH1-03 — deterministic psxport/oracle boot harness
- status: todo
- deps: CRASH1-02
- evidence:
- where: `titles/crash1/`, `tools/`
- gap: No game seam, substrate, oracle runner, or agreement/disagreement gate exists.
- notes: The harness must prove both answers and print its denominator before any boot claim is accepted.

## Recompilation and ownership

### CRASH1-04 — recompile to the first real divergence
- status: todo
- deps: CRASH1-03
- evidence:
- where: `titles/crash1/`, `generated/`
- gap: No seeds or generated substrate exist. Grow seeds from executable control flow and observed misses; never copy another game's set.
- notes: Generated code is sacrosanct.

### SHARED-01 — prove cross-title shared-engine ownership
- status: todo
- deps: CRASH1-04, CRASH2-01, CRASH3-01
- evidence:
- where: `game/`
- gap: No cross-title function/state correspondence has been measured, so no native implementation belongs in shared `game/` yet.
- notes: Similar directory roles or franchise lineage are leads, not ownership proof.

### CRASH1-05 — identify camera state and graphics submitters
- status: todo
- deps: CRASH1-04
- evidence:
- where: `titles/crash1/`, eventually `game/render/` only where SHARED-01 proves ownership
- gap: Camera state and game-code submitters are unknown; no native producer exists.
- notes: Resolve from game state before GTE submission, never from GTE/OT/GP0 output.

### CRASH1-06 — owned widescreen and interpolation paths
- status: todo
- deps: CRASH1-05
- evidence:
- where: not assigned
- gap: Blocked until PC-native code owns the camera and transform producers it would widen/interpolate.
- notes: Enhancements must remain off during faithful/oracle verification.

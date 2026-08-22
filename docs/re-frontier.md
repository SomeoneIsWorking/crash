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
- gap: This step proves executable identity and header only; CRASH1-02 separately binds it to the supplied disc's boot target. No whole-disc authentication, game recompilation, or gameplay is claimed.
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
- status: re-verified
- deps: CRASH1-01
- evidence: `python3 tools/provision_crash1.py <real USA CHD>` extracted `SYSTEM.CNF` and its boot target through psxport `discdump`; `SYSTEM.CNF` selects `SCUS_949.00`, and the extracted 290,816-byte executable matched 11/11 manifest facts including SHA-256 `aabf1464f90b2e0b81e712b77aebbdb88f303b16ce830535e2b0cd886ee280f2`. The real Crash 2 USA disc refused because it boots `SCUS_941.54`. Nine production-seam tests cover every resolution source, precedence, invalid configured-path refusal, ambiguous drop-ins, successful publish, and identity disagreement without publish; the independent executable selftest remains 4/4.
- where: `titles/crash1/executable.json`, `tools/provision_crash1.py`, `tools/verify_executable.py`, `tests/test_provision_crash1.py`
- gap: Provisioning proves which executable the supplied disc boots and that executable's identity; it does not authenticate every disc sector or prove the game boots.
- notes: Resolution is CLI argument, per-title/generic environment, per-title/generic `.env`, then exactly one repository-root CHD. Invalid configured paths and ambiguous drop-ins refuse rather than selecting another image. Extracted data remains gitignored under `scratch/bin/crash1/`.

### CRASH1-03 — deterministic psxport/oracle boot harness
- status: re-verified
- deps: CRASH1-02
- evidence: Against pinned psxport `7f5d3f13`, the independent CPU fixture passed 39/39 positive, hardware-stop negative, instruction-stepping, RAM-mirroring, and modeled-return checks. `oracle_trace` independently captured the first real crt0 call at step 57,910, target `0x80011A18`; `crossvalidate_crt0.py` compared 6 fields and agreed 6/6 on gp, call target, a0, planned sp, planned a0, and planned a1. The bounded crossvalidator selftest passed 5/5, including unreached-call refusal and wrong-target disagreement; the canonical tracer CLI selftest passed 8/8, including insufficient-window refusal. CMake `crash1_oracle_boot_check` reproduces provisioning, the fixture, and the real comparison from one asset-gated target when a disc is configured.
- where: CMake `crash1_oracle_boot_check`; framework `oracle_spike`, `oracle_trace`, `crt0_extract`, and `crossvalidate_crt0.py`; gitignored boundary trace
- gap: This proves the first independent crt0 window only. The direct Crash 1 runtime seam and generated boundary runner now exist, but BIOS, GPU, SPU, CD, DMA, timers, native boot, and gameplay remain outside this window.
- notes: The call capture is independent of the symbolic expected target: it records the first executed jal after the entry prologue. Do not call this a full oracle boot or a PC port boot.

## Recompilation and ownership

### CRASH1-04 — recompile to the first real divergence
- status: re-partial
- deps: CRASH1-03
- evidence: The verified USA executable and deliberately empty title seed additions produced 115 static candidate seeds and 653 emitted candidates with zero configured overlays under pinned psxport `7f5d3f13`, emitter version `2026-08-12.1`. A focused port runner with the direct Crash 1 runtime installed executed entry `0x8003E018` through eight distinct call targets: `0x80011A18`, `0x80011D88`, `0x8003E0C0`, `0x8001652C`, `0x8003F224`, `0x80042B1C`, `0x8004319C`, and `0x8003E1F8`. Against the independent Mednafen CPU oracle's canonical ordinal captures, `pc`, all 31 nonzero GPRs, `lo`, and `hi` agreed 34/34 at steps 57,910, 57,931, 57,935, 57,962, 62,065, 62,073, 62,077, and 62,081. The eighth target is the real `addiu $a0, 1; syscall 0` wrapper. At step 62,083 the independent CPU enters `0xBFC00180`; the controlled port boundary instead executes that exact generated wrapper through shipping `rec_syscall`, observes selector `1`, return value `1`, and IRQ delivery `1 -> 0`. A different execution-proven function is refused before syscall execution. The complete boundary gate passes 12/12, while the short real trace refuses at 7/8, the emitter refuses an out-of-text seed, repeated call targets refuse as ordinal-ambiguous, and an altered eighth-boundary register produces one named mismatch.
- where: `titles/crash1/core/crash1_runtime.cpp`, `titles/crash1/recomp_seeds.json`, `tools/crash1_recomp.py`, `tests/crash1_runtime.cpp`, `tests/crash1_recomp_boundary.cpp`, CMake `crash1_recomp_boundary_check`, gitignored `generated/crash1/` and traces
- gap: Port/oracle equality is proven only from the executable entry through the eighth call boundary. The port-side generated `EnterCriticalSection` wrapper and shipping HLE effect are separately proven, but the independent oracle exposes only A0/B0/C0 modeled returns: it cannot validate syscall Cause/EPC or resume at EPC+4. Add that framework oracle capability with refusal controls before comparing or advancing later boot. BIOS, hardware and 645 unexecuted emitted bodies are not certified. Of 653 emitted candidates, only nine addresses have execution provenance (eight generated bodies executed and one next target observed). Static table discovery also emits data-like fragments (issue #3), so neither the 115-seed nor 653-candidate count is a correctness denominator.
- notes: Generated code is sacrosanct. The boundary runner installs a direct `Crash1Runtime`; both legacy views remain null and its native-boot method refuses rather than jumping past the unresolved syscall transition. Keep explicit title seed additions empty until a real fail-fast indirect miss supplies an address and rationale; never copy another game's set. The exact framework reproducer is `oracle_trace SCUS_949.00 --steps 62100`: step 62,081 enters `0x8003E1F8`, step 62,082 executes `syscall 0`, and step 62,083 enters `0xBFC00180`.

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

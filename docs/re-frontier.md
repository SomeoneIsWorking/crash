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
- status: re-verified
- deps:
- evidence: The real USA disc's `SYSTEM.CNF` selects `SCUS_941.54`. The extracted 327,680-byte PS-X EXE has SHA-1 `aabdade44a4ca863f71224daef51c45f34fb1bf8`, SHA-256 `6e5b2449310b1ed87915f53c38f296f2152bfd0b42eaca5ef86f74a3d07b6043`, entry `0x80049B2C`, text `[0x80010000,0x8005F800)`, gp `0`, and stack `0x801FFFF0`. Embedded strings identify the North American area, `BASCUS-94154`, and `Crash Bandicoot 2`. `python3 tools/verify_executable.py --title crash2 --selftest --exe scratch/bin/crash2/SCUS_941.54` passes 4/4, including altered-hash, altered-image, and malformed-image controls.
- where: `titles/crash2/executable.json`, `tools/verify_executable.py`
- gap: This proves executable identity and header only; CRASH2-02 separately binds it to the supplied disc and validates the first executed boundary. It does not authenticate the whole disc or prove boot.
- notes: The measured `SCUS-94154` title codeword and every address are independent of Crash 1's `SCUS-94900` integration.

### CRASH2-02 — provision the selected disc and verify the first crt0 call
- status: re-verified
- deps: CRASH2-01
- evidence: The shared serial-aware provisioning path publishes only after SYSTEM.CNF boots SCUS_941.54 and the executable matches 11/11 facts. Its 10-case both-answer suite includes three-way title environment isolation and wrong-title refusal. Against clean pinned psxport bc8c8897, cached real SCUS_941.54 passes executable selftest 4/4, oracle_spike 43/43, and crt0_extract/independent CPU agreement 6/6 at executed jal ordinal 1, step 81,725, target 0x8001144C: gp 0x8005F17C, a0 0x8006F1F4, sp 0x801FFFF8, and heap size 0x0018FE08.
- where: `tools/provision_title.py`, `tests/test_provision_titles.py`, CMake `crash2_oracle_boot_check`, gitignored `scratch/bin/crash2/SCUS_941.54`
- gap: This proves only the first real crt0 call boundary. No Crash 2 generated execution, BIOS continuation, hardware, native boot, or gameplay is claimed.
- notes: The 6/6 real-byte cross-check was reproduced at psxport bc8c8897. No disc was configured for a fresh extraction in that run; prior SYSTEM.CNF provisioning evidence remains the disc-selection authority.

### CRASH2-03 — own measured executable facts through the derived runtime
- status: re-verified
- deps: CRASH2-02
- evidence: Against clean pinned psxport bc8c8897, Crash2Runtime directly inherits title-agnostic BoundaryRuntime, which inherits GameRuntime; legacy views and unmeasured runtime products remain null, and the title explicitly reports `guestVramIsPicture=false` because no frame producer exists. GuestProgramImage matches 15/15 fields from real SCUS_941.54 and shipping crt0_extract, including game main 0x80049BD4 independently observed at oracle call three; an altered global pointer produces one named disagreement, authoritative Clang CTest passes 5/5, and psxport_sync checks the same scratch/build-clang provenance.
- where: `game/core/boundary_runtime.*`, `titles/crash2/core/crash2_runtime.*`, `tools/verify_runtime_image.py`, `tests/crash2_runtime*.cpp`
- gap: The typed runtime now owns measured executable facts and game main 0x80049BD4; CRASH2-04 tracks generated execution and the unresolved syscall continuation.
- notes: BoundaryRuntime shares only refusal/no-invented-products integration invariants. The game-main address is owned by Crash 2's manifest and runtime because the independent CPU reaches it at call three.

### CRASH2-04 — recompile to the first real divergence
- status: re-partial
- deps: CRASH2-03
- evidence: Real SCUS_941.54 emits 998 candidates from 270 static seeds with zero overlays under clean recorded psxport `9c2e3f1c` (emitter 2026-08-22.1). Generated execution and the independent Mednafen CPU agree 34/34 at each of eight calls: 0x8001144C, 0x800117BC, game main 0x80049BD4, 0x80015614, 0x8004B1B8, 0x8004EC30, 0x8004F1F8, and 0x80049D1C. The manifest tracks game main as call three and first syscall as call eight, and the gate requires both by name. Call eight is the measured addiu-a0-1/syscall-0 wrapper; the oracle enters 0xBFC00180 at step 85898, while the generated wrapper through shipping HLE returns prior IRQ 1 and disables delivery. The 13/13 suite includes an altered tracked-frontier control plus out-of-text seed, 7/8 short-window, repeated-target, altered-register, and wrong-syscall-target refusals.
- where: tools/scus_94154_recomp.py, tools/resident_recomp.py, tests/crash2_recomp_boundary.cpp, titles/crash2/recomp_seeds.json, CMake crash2_recomp_boundary_check, gitignored generated/crash2/ and scratch/raw/scus-94154-recomp/
- gap: Port/oracle equality is proven only from entry through the eight call boundaries. The port-side generated syscall effect is separately proven, but the independent oracle cannot expose Cause/EPC or resume syscall execution at EPC+4, so post-syscall equality, later BIOS/hardware boot, frame output, and the remaining 990 emitted bodies are unverified.
- notes: Generated code remains sacrosanct. Static discovery counts are not execution provenance; only nine addresses are execution-proven. No Crash 1/3 seed or address was reused.

### CRASH3-01 — select and measure the Crash Bandicoot 3 executable
- status: re-verified
- deps:
- evidence: The real USA disc's `SYSTEM.CNF` selects `SCUS_942.44`, not its bundled `DRAGON/SPYRO.EXE`. The extracted 333,824-byte PS-X EXE has SHA-1 `ffa9252384eb16ce5ae14061bb87c86701ab4893`, SHA-256 `1b93cc563c90966e1d79b750f7d19bc1662859cceeb991775e9e9a22656d71d1`, entry `0x800489F8`, text `[0x80010000,0x80061000)`, gp `0`, and stack `0x801FFFF0`; the North America marker is present. The executable gate matches 11/11 tracked facts and passes its 4/4 both-answer suite. The separately extracted 393,216-byte `DRAGON/SPYRO.EXE` disagrees on name, size, both hashes, entry, and text size.
- where: `titles/crash3/executable.json`, `tools/verify_executable.py`, gitignored `scratch/bin/crash3/SCUS_942.44`
- gap: This proves executable identity and the disc's configured boot selection only; it does not authenticate the whole disc, execute the game, or make bundled demo content part of Crash 3.
- notes: The title codeword is `SCUS-94244`. No Crash 1, Crash 2, or Spyro address is inherited.

### CRASH3-02 — provision the selected disc and verify the first crt0 call
- status: re-verified
- deps: CRASH3-01
- evidence: The shared serial-aware provisioner publishes only after SYSTEM.CNF boots SCUS_942.44 and the executable matches 11/11 facts. Its 10-case suite includes three-way environment isolation, wrong-Crash-title refusal, a positive Crash 3 fixture carrying DRAGON/SPYRO.EXE, and refusal when that bundled path is made the boot target. Against clean pinned psxport 57a17a14, crt0_extract and the independent Mednafen CPU oracle agree 6/6 at executed jal ordinal 1, step 71,790, target 0x800112B8: gp 0x80060878, a0 0x8006EA7C, sp 0x801FFFF8, and heap size 0x00190580. The oracle's 39-case fixture passes independently.
- where: `tools/provision_title.py`, `tests/test_provision_titles.py`, CMake `crash3_oracle_boot_check`, gitignored `scratch/bin/crash3/SCUS_942.44`
- gap: This proves executable selection and the first real crt0 call boundary. Generated execution is separately tracked by CRASH3-04; this step alone proves no BIOS continuation, hardware, native boot, or gameplay.
- notes: CMake assigns Crash 3 only scratch/bin/crash3/ and generated/crash3/. The first structural call target is not an A(39h) thunk.

### CRASH3-03 — own measured executable facts through the derived runtime
- status: re-verified
- deps: CRASH3-02
- evidence: Against clean pinned psxport 57a17a14, Crash3Runtime directly inherits the title-agnostic BoundaryRuntime, which in turn inherits GameRuntime; both legacy views and unmeasured runtime products remain null. Its GuestProgramImage matches 15/15 fields derived from real SCUS_942.44 and the shipping crt0 decoder, including gameMainEntry 0x80048AA0 verified as oracle call three; an altered global pointer produces one named disagreement. Authoritative Clang CTest passes 5/5, including all three runtime inheritance tests, the 10-case provisioning contract, and format/size/clang-tidy policy.
- where: `game/core/boundary_runtime.*`, `titles/crash3/core/crash3_runtime.*`, `tools/verify_runtime_image.py`, `tests/crash3_runtime*.cpp`, `tests/title_runtime_facts.h`
- gap: The typed runtime owns measured executable facts and game main but still refuses native boot; CRASH3-04 tracks generated execution and the unresolved syscall continuation.
- notes: BoundaryRuntime shares only the refusal/no-invented-products invariant. It contains no inferred Crash engine behavior and does not satisfy SHARED-01.

### CRASH3-04 — recompile to the first real divergence
- status: re-partial
- deps: CRASH3-03
- evidence: Real SCUS_942.44 emits 986 candidates from 297 static seeds with zero overlays under clean recorded psxport `9c2e3f1c` (emitter 2026-08-22.1). Generated execution and the independent Mednafen CPU agree 34/34 at each of eight calls: 0x800112B8, 0x80011628, game main 0x80048AA0, 0x800154AC, 0x8004BFBC, 0x8004F37C, 0x8004F914, and 0x80048C38. The manifest tracks game main as call three and first syscall as call eight, and the gate requires both by name. Call eight is the measured addiu-a0-1/syscall-0 wrapper; the oracle enters 0xBFC00180 at step 75963, while the generated wrapper through shipping HLE returns prior IRQ 1 and disables delivery. The 13/13 suite includes an altered tracked-frontier control plus out-of-text seed, 7/8 short-window, repeated-target, altered-register, and wrong-syscall-target refusals.
- where: tools/scus_94244_recomp.py, tools/resident_recomp.py, tests/crash3_recomp_boundary.cpp, titles/crash3/recomp_seeds.json, CMake crash3_recomp_boundary_check, gitignored generated/crash3/ and scratch/raw/scus-94244-recomp/
- gap: Port/oracle equality is proven only from entry through the eight call boundaries. The port-side generated syscall effect is separately proven, but the independent oracle cannot yet expose Cause/EPC or resume syscall execution at EPC+4, so post-syscall equality, later BIOS/hardware boot, frame output, and the remaining 978 emitted bodies are unverified.
- notes: Generated code remains sacrosanct. Static discovery counts are not execution provenance; only nine addresses are execution-proven. The crt0 decoder's structural libcInit slot 0x800112B8 is not an A(39h) thunk in SCUS_942.44; do not apply the A0 modeled-return path to it.


## Provisioning and oracle

### CRASH1-02 — provision the selected disc without tracking copyrighted data
- status: re-verified
- deps: CRASH1-01
- evidence: `python3 tools/provision_title.py --title crash1 <real USA CHD>` extracted `SYSTEM.CNF` and its boot target through psxport `discdump`; `SYSTEM.CNF` selects `SCUS_949.00`, and the extracted 290,816-byte executable matched 11/11 manifest facts including SHA-256 `aabf1464f90b2e0b81e712b77aebbdb88f303b16ce830535e2b0cd886ee280f2`. The real Crash 2 USA disc refused because it boots `SCUS_941.54`. Nine production-seam tests cover every resolution source, precedence, invalid configured-path refusal, ambiguous drop-ins, successful publish, and identity disagreement without publish; the independent executable selftest remains 4/4.
- where: `titles/crash1/executable.json`, `tools/provision_title.py`, `tools/verify_executable.py`, `tests/test_provision_titles.py`
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

### CRASH1-04 — ship the recompiled path to the first real divergence
- status: re-partial
- deps: CRASH1-03
- evidence: Against clean recorded psxport `9c2e3f1c`, the verified USA executable and deliberately empty title seed additions produce 115 static candidate seeds and 653 emitted candidates with zero configured overlays under emitter version `2026-08-22.1`. Generated execution with the direct Crash 1 runtime installed starts at entry `0x8003E018` and reaches eight distinct call targets: `0x80011A18`, `0x80011D88`, game main `0x8003E0C0`, `0x8001652C`, `0x8003F224`, `0x80042B1C`, `0x8004319C`, and first syscall `0x8003E1F8`. Against the independent Mednafen CPU oracle's canonical ordinal captures, `pc`, all 31 nonzero GPRs, `lo`, and `hi` agree 34/34 at steps 57,910, 57,931, 57,935, 57,962, 62,065, 62,073, 62,077, and 62,081. The game-main and first-syscall addresses and call ordinals are tracked in the executable manifest and checked by name. The eighth target is the real `addiu $a0, 1; syscall 0` wrapper. At step 62,083 the independent CPU enters `0xBFC00180`; the controlled port boundary instead executes that exact generated wrapper through shipping `rec_syscall`, observes selector `1`, return value `1`, and IRQ delivery `1 -> 0`. The real `crash1_port` product uses the same `ResidentProgram` and `EnterCriticalSection` owners as the harness: it loads the retail executable and dispatches the generated crt0 path to this exact boundary, with no diagnostic target in its launch route. The complete boundary gate passes 13/13, including a manifest-altered frontier control; the short real trace refuses at 7/8, the emitter refuses an out-of-text seed, repeated call targets refuse as ordinal-ambiguous, a different execution-proven function is refused before syscall execution, and an altered eighth-boundary register produces one named mismatch.
- where: `game/core/resident_program.*`, `game/core/enter_critical_frontier.*`, `titles/crash1/core/crash1_runtime.*`, `titles/crash1/core/crash1_port.*`, `titles/crash1/main.cpp`, `titles/crash1/executable.json`, `titles/crash1/recomp_seeds.json`, `tools/crash1_recomp.py`, `tools/resident_recomp.py`, `tests/crash1_runtime*.cpp`, `tests/crash1_recomp_boundary.cpp`, CMake `crash1_port` and `crash1_recomp_boundary_check`, gitignored `generated/crash1/` and traces
- gap: Port/oracle equality is proven only from the executable entry through the eighth call boundary. The port-side generated `EnterCriticalSection` wrapper and shipping HLE effect are separately proven, but the independent oracle exposes only A0/B0/C0 modeled returns: it cannot validate syscall Cause/EPC or resume at EPC+4. Add that framework oracle capability with refusal controls before comparing or advancing later boot. BIOS, hardware and 645 unexecuted emitted bodies are not certified. Of 653 emitted candidates, only nine addresses have execution provenance (eight generated bodies executed and one next target observed). Static table discovery also emits data-like fragments (issue #3), so neither the 115-seed nor 653-candidate count is a correctness denominator.
- notes: Generated code is sacrosanct. The product is runnable only to this measured boundary and exits after reporting the unresolved transition; this is neither a rendered frame nor a playable-game claim. Both legacy views remain null. Keep explicit title seed additions empty until a real fail-fast indirect miss supplies an address and rationale; never copy another game's set. The exact framework reproducer is `oracle_trace SCUS_949.00 --steps 62100`: step 62,081 enters `0x8003E1F8`, step 62,082 executes `syscall 0`, and step 62,083 enters `0xBFC00180`.

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

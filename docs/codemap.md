# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Oracle and recompiled-boundary gates verified against psxport `3418a79b`; no game seam |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks over the boundary runner; generated code remains excluded |
| Executable identity gate | ✅ verified | `tools/verify_executable.py` | Manifest-vs-real-image check uses psxport's PS-X EXE loader; 4/4 positive/negative selftest |
| Crash 1 disc provisioning | ✅ verified | `tools/provision_crash1.py`, `tests/test_provision_crash1.py` | CLI/env/`.env`/single-drop-in resolution; real USA disc boot target plus 11/11 executable facts pass, while wrong-title disc and nine synthetic cases prove refusal paths |
| Project registries | 🟡 partial | `docs/info/`, `docs/issues/`, `docs/re-frontier.md` | Data exists and the shared skill engines work; local command entrypoints remain absent (issue #1) |
| Crash 1 executable identity | ✅ verified | `titles/crash1/executable.json`, `tools/verify_executable.py` | North American `SCUS-94900`: the real USA disc's `SYSTEM.CNF` selects `SCUS_949.00`, whose 290,816 bytes match all tracked facts |
| Crash 1 resident recompile | 🟡 first slice | `titles/crash1/recomp_seeds.json`, `tools/crash1_recomp.py`, `tests/crash1_recomp_boundary.cpp`, CMake `crash1_recomp_boundary_check`, gitignored `generated/crash1/` | Real USA bytes expand 115 static candidate seeds into 653 emitted candidates; generated execution agrees 34/34 at calls one through four, including the call-three return. Five addresses have execution provenance; continue inside `0x8001652C` toward the first real divergence |
| Crash 2 integration | ⬜ missing | `titles/crash2/` | Select region and measure executable |
| Crash 3 integration | ⬜ missing | `titles/crash3/` | Select region and measure executable |
| Shared engine | ⬜ missing | `game/` | Begins only after cross-title RE proves ownership |
| Native graphics producers | ⬜ missing | — | No producer exists |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 first recompiled boundaries | CMake `crash1_oracle_boot_check`, `crash1_recomp_boundary_check`; `tools/crash1_recomp.py`; framework oracle tools | Symbolic/oracle crt0 remains 6/6; canonical oracle ordinal capture and the generated port agree 34/34 at calls one through four. Out-of-text emission, a short 3/4 call window and an altered fourth-call register prove refusal/disagreement. No port-side per-instruction lockstep yet |

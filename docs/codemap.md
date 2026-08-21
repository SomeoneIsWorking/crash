# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Smoke and first oracle window verified against psxport `2b5ef7b5`; no game seam |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks; currently 0 translation units |
| Executable identity gate | ✅ verified | `tools/verify_executable.py` | Manifest-vs-real-image check uses psxport's PS-X EXE loader; 4/4 positive/negative selftest |
| Crash 1 disc provisioning | ✅ verified | `tools/provision_crash1.py`, `tests/test_provision_crash1.py` | CLI/env/`.env`/single-drop-in resolution; real USA disc boot target plus 11/11 executable facts pass, while wrong-title disc and nine synthetic cases prove refusal paths |
| Project registries | 🟡 partial | `docs/info/`, `docs/issues/`, `docs/re-frontier.md` | Data exists and the shared skill engines work; local command entrypoints remain absent (issue #1) |
| Crash 1 executable identity | ✅ verified | `titles/crash1/executable.json`, `tools/verify_executable.py` | North American `SCUS-94900`: the real USA disc's `SYSTEM.CNF` selects `SCUS_949.00`, whose 290,816 bytes match all tracked facts |
| Crash 1 integration | 🟡 partial | `titles/crash1/`, `tools/provision_crash1.py`, CMake `crash1_oracle_boot_check` | Provisioning and the first independent crt0 window are verified; next is a generated substrate compared against the oracle |
| Crash 2 integration | ⬜ missing | `titles/crash2/` | Select region and measure executable |
| Crash 3 integration | ⬜ missing | `titles/crash3/` | Select region and measure executable |
| Shared engine | ⬜ missing | `game/` | Begins only after cross-title RE proves ownership |
| Native graphics producers | ⬜ missing | — | No producer exists |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 first independent window | CMake `crash1_oracle_boot_check`; framework `oracle_spike`, `oracle_trace`, `crossvalidate_crt0.py` | Permanent fixture passes 22/22; real crt0 call state agrees 6/6 with symbolic decoding; 50,000-step no-call control refuses without comparing |

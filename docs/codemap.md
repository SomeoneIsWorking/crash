# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Smoke target verified against psxport `be381503`; no game seam |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks; currently 0 translation units |
| Executable identity gate | ✅ verified | `tools/verify_executable.py` | Manifest-vs-real-image check uses psxport's PS-X EXE loader; 4/4 positive/negative selftest |
| Project registries | 🟡 partial | `docs/info/`, `docs/issues/`, `docs/re-frontier.md` | Data exists and the shared skill engines work; local command entrypoints remain absent (issue #1) |
| Crash 1 executable identity | ✅ verified | `titles/crash1/executable.json`, `tools/verify_executable.py` | North American `SCUS-94900`: real 290,816-byte executable measured and checked; disc provisioning/boot still absent |
| Crash 1 integration | 🟡 partial | `titles/crash1/` | Next: provision the selected disc without tracking it, then build an oracle-first boot harness |
| Crash 2 integration | ⬜ missing | `titles/crash2/` | Select region and measure executable |
| Crash 3 integration | ⬜ missing | `titles/crash3/` | Select region and measure executable |
| Shared engine | ⬜ missing | `game/` | Begins only after cross-title RE proves ownership |
| Native graphics producers | ⬜ missing | — | No producer exists |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | ⬜ missing | — | Stand up oracle before game logic |

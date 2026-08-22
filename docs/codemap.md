# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Oracle and recompiled-boundary gates use psxport `7f5d3f13`; Crash 1 owns a derived title runtime but still has no native boot |
| Crash 1 runtime seam | 🔬 boundary-only | `titles/crash1/core/crash1_runtime.cpp`, `tests/crash1_runtime.cpp` | Direct `GameRuntime` inheritance is installed by the real boundary runner with null legacy views; no context, frame driver, scheduler, overrides, or native boot exists before the oracle can resume the measured syscall |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks over the runtime and boundary translation units; generated code remains excluded |
| Executable identity gate | ✅ verified | `tools/verify_executable.py` | Manifest-vs-real-image check uses psxport's PS-X EXE loader; 4/4 positive/negative selftest |
| Crash 1 disc provisioning | ✅ verified | `tools/provision_crash1.py`, `tests/test_provision_crash1.py` | CLI/env/`.env`/single-drop-in resolution; real USA disc boot target plus 11/11 executable facts pass, while wrong-title disc and nine synthetic cases prove refusal paths |
| Project registries | 🟡 partial | `docs/info/`, `docs/issues/`, `docs/re-frontier.md` | Data exists and the shared skill engines work; local command entrypoints remain absent (issue #1) |
| Crash 1 executable identity | ✅ verified | `titles/crash1/executable.json`, `tools/verify_executable.py` | North American `SCUS-94900`: the real USA disc's `SYSTEM.CNF` selects `SCUS_949.00`, whose 290,816 bytes match all tracked facts |
| Crash 1 resident recompile | 🟡 syscall boundary | `titles/crash1/recomp_seeds.json`, `tools/crash1_recomp.py`, `tests/crash1_recomp_boundary.cpp`, CMake `crash1_recomp_boundary_check`, gitignored `generated/crash1/` | Calls one through eight agree 34/34; the controlled port then executes the real `EnterCriticalSection` wrapper and proves IRQ `1 -> 0`. The independent oracle enters `0xBFC00180` but cannot model/resume the syscall yet; no post-syscall equality is claimed |
| Crash 2 integration | ⬜ missing | `titles/crash2/` | Select region and measure executable |
| Crash 3 integration | ⬜ missing | `titles/crash3/` | Select region and measure executable |
| Shared engine | ⬜ missing | `game/` | Begins only after cross-title RE proves ownership |
| Native graphics producers | ⬜ missing | — | No producer exists |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 syscall boundary | CMake `crash1_oracle_boot_check`, `crash1_recomp_boundary_check`; `tools/crash1_recomp.py`; framework oracle tools | Symbolic/oracle crt0 remains 6/6; calls one through eight agree 34/34. Port-side EnterCriticalSection behavior and three refusal/disagreement controls are proven; the oracle exposes no syscall-exception resume, so later boot remains blocked without pretending equality |

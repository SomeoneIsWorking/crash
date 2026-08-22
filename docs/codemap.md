# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Clean Clang, CTest, both generated-boundary regressions, and cached real-data gates use recorded psxport `57a17a14`; no title has native boot |
| Boundary-only runtime policy | ✅ verified policy | `game/core/` (`boundary_runtime.h`, `boundary_runtime.cpp`) | Crash 1/2/3 inheritance tests pass against pinned `57a17a14`; this owns no inferred Crash engine behavior and does not satisfy SHARED-01 |
| Crash 1 runtime seam | 🔬 boundary-only | `titles/crash1/core/crash1_runtime.cpp`, `tests/crash1_runtime.cpp` | Direct `GameRuntime` inheritance is installed by the real boundary runner with null legacy views; no context, frame driver, scheduler, overrides, or native boot exists before the oracle can resume the measured syscall |
| Authoritative verifier | ✅ enforced | `tools/verify.py`, `scratch/build-clang/` | The normal entry point configures and builds the documented Clang tree before full CTest and pin checking, preventing stale generated test graphs |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks over every runtime and boundary translation unit; generated code remains excluded |
| Executable identity gate | ✅ verified | `tools/verify_executable.py` | All three title manifests are compared to real images through psxport's PS-X EXE loader; each 4/4 positive/negative selftest passes |
| Serial-aware disc provisioning | ✅ verified | `tools/provision_title.py`, `tests/test_provision_titles.py` | One shared title-selected CLI/env/`.env`/drop-in implementation; 10 cases include three-way key isolation and prove Crash 3 follows `SYSTEM.CNF`, not the bundled Spyro demo executable |
| Project registries | 🟡 partial | `docs/info/`, `docs/issues/`, `docs/re-frontier.md` | Data exists and the shared skill engines work; local command entrypoints remain absent (issue #1) |
| Crash 1 executable identity | ✅ verified | `titles/crash1/executable.json`, `tools/verify_executable.py` | North American `SCUS-94900`: the real USA disc's `SYSTEM.CNF` selects `SCUS_949.00`, whose 290,816 bytes match all tracked facts |
| Serial-scoped resident recompile | 🟡 syscall boundaries | `tools/resident_recomp.py`, `tests/recomp_boundary_support.h`, title entry points/runners, CMake `crash{1,3}_recomp_boundary_check` | `SCUS-94900` and `SCUS-94244` each agree 34/34 at calls one through eight, then separately prove their own generated `EnterCriticalSection` wrapper changes IRQ `1 -> 0`; the oracle cannot yet resume either syscall |
| Crash 2 integration | 🟡 first boundary | `titles/crash2/executable.json`, `titles/crash2/core/crash2_runtime.*`, CMake `crash2_oracle_boot_check` | USA `SCUS-94154` identity, typed runtime facts 15/15, and first crt0 call 6/6 are verified against pinned `57a17a14`; generated execution is next |
| Crash 3 integration | 🟡 syscall boundary | `titles/crash3/executable.json`, `titles/crash3/core/crash3_runtime.*`, `tools/scus_94244_recomp.py`, CMake `crash3_recomp_boundary_check` | `SCUS-94244` identity and typed runtime match real bytes; eight generated/oracle calls agree 34/34 through game main `0x80048AA0` and syscall wrapper `0x80048C38`. Post-syscall equality and first frame remain blocked |
| Shared engine | ⬜ missing | `game/` | Begins only after cross-title RE proves ownership |
| Native graphics producers | ⬜ missing | — | No producer exists |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 title boundaries | CMake oracle targets plus `crash1_recomp_boundary_check` and `crash3_recomp_boundary_check`; framework oracle tools | Crash 1 and Crash 3 reach separate serial-scoped syscall boundaries; Crash 2 symbolic/oracle crt0 agrees 6/6 at call one. No result licenses post-syscall equality |

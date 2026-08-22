# Codemap

| Subsystem | Status | Where | Gap / next |
|---|---|---|---|
| Framework consumer | 🟡 scaffold | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | Clean Clang, CTest, typed-image, Crash 1 regression, and all three real oracle gates use recorded psxport `ad5cf802`; no title has native boot |
| Boundary-only runtime policy | ✅ verified policy | `game/core/` (`boundary_runtime.h`, `boundary_runtime.cpp`) | Crash 1/2/3 inheritance tests pass against pinned `ad5cf802`; this owns no inferred Crash engine behavior and does not satisfy SHARED-01 |
| Crash 1 runtime seam | 🔬 boundary-only | `titles/crash1/core/crash1_runtime.cpp`, `tests/crash1_runtime.cpp` | Direct `GameRuntime` inheritance is installed by the real boundary runner with null legacy views; no context, frame driver, scheduler, overrides, or native boot exists before the oracle can resume the measured syscall |
| Authoritative verifier | ✅ enforced | `tools/verify.py`, `scratch/build-clang/` | The normal entry point configures and builds the documented Clang tree before full CTest and pin checking, preventing stale generated test graphs |
| C++ verification | ✅ enforced | `.clang-format`, `.clang-tidy`, `CMakeLists.txt`, `external/psxport/tools/check_cpp_style.py` | Normal CTest runs shared first-party-only format, 1,200-line ownership, and compile-database clang-tidy checks over every runtime and boundary translation unit; generated code remains excluded |
| Executable identity gate | ✅ verified | `tools/verify_executable.py` | All three title manifests are compared to real images through psxport's PS-X EXE loader; each 4/4 positive/negative selftest passes |
| Serial-aware disc provisioning | ✅ verified | `tools/provision_title.py`, `tests/test_provision_titles.py` | One shared title-selected CLI/env/`.env`/drop-in implementation; 10 cases include three-way key isolation and prove Crash 3 follows `SYSTEM.CNF`, not the bundled Spyro demo executable |
| Project registries | 🟡 partial | `docs/info/`, `docs/issues/`, `docs/re-frontier.md` | Data exists and the shared skill engines work; local command entrypoints remain absent (issue #1) |
| Crash 1 executable identity | ✅ verified | `titles/crash1/executable.json`, `tools/verify_executable.py` | North American `SCUS-94900`: the real USA disc's `SYSTEM.CNF` selects `SCUS_949.00`, whose 290,816 bytes match all tracked facts |
| Crash 1 resident recompile | 🟡 syscall boundary | `titles/crash1/recomp_seeds.json`, `tools/crash1_recomp.py`, `tests/crash1_recomp_boundary.cpp`, CMake `crash1_recomp_boundary_check`, gitignored `generated/crash1/` | Calls one through eight agree 34/34; the controlled port then executes the real `EnterCriticalSection` wrapper and proves IRQ `1 -> 0`. The independent oracle enters `0xBFC00180` but cannot model/resume the syscall yet; no post-syscall equality is claimed |
| Crash 2 integration | 🟡 first boundary | `titles/crash2/executable.json`, `titles/crash2/core/crash2_runtime.*`, CMake `crash2_oracle_boot_check` | USA `SCUS-94154` identity, typed runtime facts 15/15, and first crt0 call 6/6 are verified against pinned `ad5cf802`; generated execution is next |
| Crash 3 integration | 🟡 first boundary | `titles/crash3/executable.json`, `titles/crash3/core/crash3_runtime.*`, CMake `crash3_oracle_boot_check` | USA `SCUS-94244` identity, title-scoped cache/generated paths, typed runtime facts 15/15, and first crt0 call 6/6 are verified against pinned `ad5cf802`; generated execution is next |
| Shared engine | ⬜ missing | `game/` | Begins only after cross-title RE proves ownership |
| Native graphics producers | ⬜ missing | — | No producer exists |
| Widescreen | ⬜ missing | — | Blocked on native camera and producers |
| Interpolation | ⬜ missing | — | Blocked on PC ownership of transform producers |
| Differential harness | 🟡 title boundaries | CMake `crash1_oracle_boot_check`, `crash1_recomp_boundary_check`, `crash2_oracle_boot_check`, `crash3_oracle_boot_check`; framework oracle tools | Crash 1 reaches its syscall boundary; Crash 2 and Crash 3 symbolic/oracle crt0 each agree 6/6 at call one. These results do not license later generated execution or post-syscall Crash 1 equality |

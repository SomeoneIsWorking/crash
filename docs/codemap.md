# Codemap

Ownership and placement map for the Crash trilogy port. Capability status belongs in
`docs/project-state.md`, ordered reverse-engineering evidence in `docs/re-frontier.md`, and atomic
work in `docs/issues/`.

The player path flows from the launcher to one title product, then through reusable resident-program
mechanics into title-owned runtime and boot-frontier facts. Verification follows a separate path from
the retail executable through an independent CPU comparator. Nothing becomes shared Crash engine code
until cross-title binary evidence proves common ownership.

## Ownership

| Subsystem | Responsibility | Current / target location | Entry point | Deep doc | Placement rule |
|---|---|---|---|---|---|
| Framework consumption | Resolve, record, and build one psxport checkout | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | top-level CMake configure | `CLAUDE.md` | Framework-neutral consumer wiring stays top-level; reusable framework behavior belongs in psxport |
| Player launch | Frozen-uv setup, title asset selection, product-only build, and default Crash 1 launch | `run.sh`, `bootstrap.py`, `tools/run.py` | `run.sh` | `README.md` | Portable setup and launch policy stays in `tools/run.py`; the shell remains a slim shim |
| Executable identity | Measure and reject serial/header/hash disagreement | `titles/<title>/executable.json`, `tools/verify_executable.py` | `verify_executable.py` CLI | `titles/crash1/README.md` | Title facts stay in title manifests; generic validation has one shared implementation |
| Disc provisioning | Follow SYSTEM.CNF, select one serial, verify it, and publish to the title cache | `tools/provision_title.py` | `provision_title.py` CLI | `README.md` | Precedence, extraction, and verification policy stays in this one tool |
| Boundary runtime policy | Direct `GameRuntime` ownership, the trilogy-wide native/interpolation product target, and refusal of unearned picture/native-boot claims | `game/core/boundary_runtime.*`, `titles/<title>/core/<title>_runtime.*` | `BoundaryRuntime::renderCapabilities` and title runtime constructor | `CLAUDE.md` | Framework-facing invariants and the user-required trilogy capability profile stay in `game/core/`; measured title behavior stays title-local |
| Resident program execution | Install runtime/registry, load an executable, apply transitions, and stop at an exact boundary | `game/core/resident_program.*` | `runResidentProgram` | `CLAUDE.md` | Generic generated-program mechanics stay here; title addresses and semantics do not |
| EnterCriticalSection frontier | Validate and execute the measured generated syscall wrapper as a returning transition | `game/core/enter_critical_frontier.*` | `runEnterCriticalFrontier` | `docs/re-frontier.md` | Shared only because all three serials independently prove the same syscall contract |
| Crash 1 boot composition | Own the measured syscall → B(56h) → A(44h) product spine | `titles/crash1/core/crash1_boot_frontier.*`, `titles/crash1/core/crash1_port.*` | `crash1::runPort` | `titles/crash1/README.md` | Further Crash 1 BIOS/frame boundaries stay title-local until evidence proves a narrower or cross-title owner |
| Crash 1 BIOS consumer contract | Record retail C0 facts and verify the shipping framework table through the production HLE seam | `titles/crash1/bios_contract.json`, `tests/crash1_c0_exception_contract.cpp` | CTest `crash1_c0_exception_contract` | `docs/issues/0007-crash-patches-address-zero-because-the-native-c0.md` | Crash owns the observed consumer and patch range; BIOS table semantics and exact post-model oracle capture belong in psxport |
| Crash 2 integration | Own SCUS-94154 facts and title runtime seam | `titles/crash2/` | `Crash2Runtime` | `docs/re-frontier.md` | Do not reuse Crash 1 addresses or seeds |
| Crash 3 integration | Own SCUS-94244 facts, disc selection, and runtime seam | `titles/crash3/` | `Crash3Runtime` | `docs/re-frontier.md` | Ignore unrelated executables on the disc; SYSTEM.CNF and serial identity own selection |
| Generated substrate | Hold disposable emitter output derived from the user's executable | `generated/` | generated `main_dispatch` | `titles/crash1/README.md` | Never hand-edit or commit; executable provenance belongs in the harness and frontier |
| Resident differential harness | Emit retail-derived code, capture independent CPU boundaries, compare state, and enforce negatives | `tools/resident_recomp.py`, title wrapper tools, `tests/recomp_boundary_support.h` | title `crash*_recomp.py` wrapper | `docs/info/instruments/005-serial-scoped-resident-recompiled-boundary-comparator.md` | Comparison/parsing stays in `resident_recomp.py`; title wrappers only select a title |
| C++ verification | Clang maintainer build, format, line cap, and compile-database clang-tidy | `.clang-format`, `.clang-tidy`, `tools/verify.py` | `tools/verify.py` | `README.md` | Normal verification orchestration stays in the Python verifier |
| Project information | Claims, instruments, factual state, atomic issues, and ordered RE frontier | `docs/info/`, `docs/project-state.md`, `docs/issues/`, `docs/re-frontier.md` | project-info brief | `docs/project-state.md` | Put each fact in its single registry authority; do not use this codemap as status |
| Shared Crash engine | Cross-title game behavior after direct correspondence proves common ownership | `game/` | unassigned | `docs/re-frontier.md` | Add no engine implementation until SHARED-01 is evidence-backed |
| Native graphics producers | Own game-state-to-native-primitive production before GTE/OT/GP0 output | title-local target under `titles/crash1/`, then `game/` only if proven shared | unassigned | `docs/re-frontier.md` | Begin beside the proven submitter; move only invariant cross-title behavior |
| Widescreen | Widen an owned camera/projection producer | beside the future camera owner | unassigned | `docs/project-state.md` | Never implement in presentation or reconstruct from post-projection output |
| Interpolation | Interpolate owned previous/current simulation transforms at presentation time | beside future simulation snapshot and renderer owners | unassigned | `docs/project-state.md` | Keep simulation snapshot ownership separate from renderer consumption |

## Annotated source tree

```text
game/  —  312 lines, 6 files
└─ core/  312 lines  6 files  [.cpp .h]
titles/  —  387 lines, 11 files
├─ crash1/  277 lines  7 files  [.cpp .h]
│  └─ core/  272 lines  6 files
├─ crash2/  55 lines  2 files  [.cpp .h]
│  └─ core/  55 lines  2 files
└─ crash3/  55 lines  2 files  [.cpp .h]
│  └─ core/  55 lines  2 files
tools/  —  2,585 lines, 10 files
tests/  —  873 lines, 16 files
```

Generated with `codemap.py tree game titles tools tests --depth 2`; generated code and vendored
framework sources are deliberately outside first-party ownership counts.

## Where does it go?

| Responsibility | Owner |
|---|---|
| A new serial-specific address or BIOS fact | That title's executable manifest and boot-frontier module |
| A retail BIOS table semantic required by a title | psxport; the title records only its consumer contract and evidence |
| A reusable generated-program transition mechanism | `game/core/resident_program.*` |
| A cross-title engine behavior | `game/`, only after SHARED-01 evidence |
| A Crash 1 camera or graphics submitter | Title-local beside its recovered game-state owner |
| Widescreen camera policy | Beside the proven camera/projection producer |
| Previous/current transform capture | Beside the authoritative simulation tick owner |
| Native interpolation consumption | Beside the native renderer submission owner |
| A new evidence claim, capability status, or atomic blocker | `docs/info/claims/`, `docs/project-state.md`, or `docs/issues/` respectively |

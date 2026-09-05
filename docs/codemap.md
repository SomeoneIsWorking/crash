# Codemap

Ownership and placement map for the Crash trilogy port. Capability state belongs in
`docs/project-state.md`, migration order in `docs/migration.md`, binary evidence in
`docs/re-frontier.md`, and atomic work in `docs/issues/`.

The product path is `run.sh` → title composition → native owners plus psxport's per-`Core` Lightrec
executor → title frame/presentation owner. The separately built interpreter/oracle path never links
into a gameplay executable; Lightrec alone owns any bounded per-block fallback.

## Ownership

| Subsystem | Responsibility | Current / target location | Entry point | Placement rule |
|---|---|---|---|---|
| Framework consumption | Resolve and record the psxport revision providing the Lightrec executor | `CMakeLists.txt`, `external/psxport/`, `psxport.pin` | top-level configure | Runtime translation, Core synchronization, device/HLE callbacks, override dispatch, original calls, exits, and invalidation belong in psxport |
| Shared integration core | Hold only cross-title framework-facing contracts and behavior proven common across the trilogy | `game/core/` | `BoundaryRuntime`, native frame-loop contract | Title addresses and unproven engine behavior remain title-local |
| Player launch | Frozen-uv media discovery, identity validation, native/Lightrec product build, and current-title launch | `run.sh`, `bootstrap.py`, `tools/run.py` | `run.sh` | The target path never emits guest code or exposes an interpreter/engine selector |
| Hosted verification | Exercise the asset-free native/Lightrec boundary only on hosts that can build the real product | `.github/workflows/ci.yml`, `tools/verify.py` | `uv run --frozen python tools/verify.py` | Platform qualification belongs to a real host runtime/build; unsupported hosts remain explicit state gaps |
| Executable identity | Verify serial, header, hashes, and title-owned binary facts | `titles/<title>/executable.json`, `tools/verify_executable.py` | `verify_executable.py` | Facts stay title-specific; reusable validation remains shared |
| Disc provisioning | Follow `SYSTEM.CNF`, select one identity, and publish validated user data | `tools/provision_title.py` | `provision_title.py` | Provisioning publishes bytes for runtime mapping, not offline translation |
| Lightrec executor integration | Bind image-aware native overrides and bounded guest/original calls; run frame turns through the shared host-service dispatcher | `game/core/dynarec_dispatch.*`, `game/core/boundary_runtime.*`, `titles/<title>/core/<title>_runtime.*` | `crash::dynarec::executeTurn`, title runtime composition | psxport owns CPU synchronization, cache invalidation and service dispatch; Crash owns title identity and policy |
| Product composition | Compose one title's native owners and executor without absorbing their implementations | `titles/crash1/core/crash1_port.*`, later title-local equivalents | `crash1::runPort` | Crash 1 remains active until representative gameplay; Crash 2/3 follow sequentially |
| Native frame ownership | Own exact title input, audio, simulation, host-service, render, and presentation order; consume typed executor exits | `titles/<title>/core/<title>_frame_driver.*`, `game/core/native_frame_loop_contract.*` | title `FrameDriver::stepFrame` | Guest VSync produces a typed boundary; no C++ unwind crosses JIT frames |
| Crash 1 native boot services | Preserve measured libcd state, disc-index I/O, callback/event/pad initialization, and GPU watchdog behavior | `titles/crash1/core/crash1_{cd_boot,disc_index_io,callback_boot,gpu_watchdog}.*` | `Crash1Runtime::registerOverrides` | Original guest bodies re-enter through executor original calls; shared Sony semantics stay in psxport |
| Crash 1 BIOS pad input | Publish the finalized host mask in Crash's authenticated BIOS `PadRead` word before retail `PadUpdate` | `titles/crash1/core/crash1_bios_pad_input.*` | `bios_pad_input::publishPrimary` | psxport owns device polling; this owner owns address `0x80057054` and byte order only |
| Crash 1 BIOS consumer | Record C0 facts and verify the shared BIOS table contract | `titles/crash1/bios_contract.json`, `tests/crash1_c0_exception_contract.cpp` | consumer contract | Crash owns observed use; psxport owns BIOS semantics |
| Crash 2 integration | Own `SCUS_941.54` identity, runtime, exits, and native owners | `titles/crash2/` | `Crash2Runtime` | No Crash 1 address or behavior is inherited without evidence |
| Crash 3 integration | Own `SCUS_942.44` identity, runtime, exits, and native owners | `titles/crash3/` | `Crash3Runtime` | `SYSTEM.CNF` selects the title; the bundled Spyro demo executable is not Crash 3 |
| Differential evidence | Compare deterministic runtime state with the independent emulator and exercise negative controls | target dynamic harness; existing recorded evidence in `docs/info/` and `docs/re-frontier.md` | separately built diagnostic target | The oracle/interpreter never becomes a product fallback |
| Shared Crash engine | Hold behavior proven common across title binaries | `game/` | assigned only by evidence | Similar names and franchise lineage do not establish ownership |
| Native graphics producers | Convert pre-GTE game camera/object/material state to typed primitives | title-local producer modules, then `game/` if proven shared | future producer interfaces | Never consume GTE/OT/GP0/framebuffer output as product source |
| Widescreen | Widen owned camera/projection, viewport, scissor, and proven horizontal culling | beside the camera/renderer owners | future projection owner | Deterministic geometry expansion only; no final-image stretch |
| Temporal presentation | Interpolate authoritative previous/current simulation transforms | beside simulation snapshots and renderer consumption | future presentation decorator | Simulation and guest memory remain unchanged |

## Where does it go?

| Responsibility | Owner |
|---|---|
| R3000A runtime translation or cache invalidation | psxport's Lightrec executor |
| A title-specific native override or original call | That title runtime, keyed by image identity and guest address |
| A frame/host-work/interrupt stop | A typed psxport executor exit handled by the title frame owner |
| A serial-specific address or BIOS fact | That title's executable manifest and owning module |
| Host controller polling | psxport `Pad` |
| Crash 1 BIOS auto-pad layout | `crash1_bios_pad_input.*` |
| A cross-title engine behavior | `game/`, only after direct correspondence |
| Capability, migration order, evidence, or an atomic blocker | `docs/project-state.md`, `docs/migration.md`, `docs/re-frontier.md`, or `docs/issues/` respectively |

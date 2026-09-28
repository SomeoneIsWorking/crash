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
| Guest projection widening rule | Latch the framework's plan onto a title's own GTE screen centre: move OFX, hold OFY and H, refuse an unmeasured call site, and pass through a call site whose argument comes back out of the register it moved | `game/core/guest_projection_publication.*` | `crash::GuestProjectionPublication::publishCentre` | Belongs in `game/` only because the three leaves are measured identical across the binaries; every address comes from a per-title `ProjectionTitleFacts`, and no title restates the rule |
| Player launch | Frozen-uv media discovery, identity validation, native/Lightrec product build, and current-title launch | `run.sh`, `bootstrap.py`, `tools/run.py` | `run.sh` | The target path never emits guest code or exposes an interpreter/engine selector |
| Hosted verification | Exercise the asset-free native/Lightrec boundary only on hosts that can build the real product | `.github/workflows/ci.yml`, `tools/verify.py` | `uv run --frozen python tools/verify.py` | Platform qualification belongs to a real host runtime/build; unsupported hosts remain explicit state gaps |
| Executable identity | Verify serial, header, hashes, and title-owned binary facts | `titles/<title>/executable.json`, `tools/verify_executable.py` | `verify_executable.py` | Facts stay title-specific; reusable validation remains shared |
| Guest projection facts | The measured publication entries, call sites, screen-distance bound, its readers and their consumers, as re-derived from the authenticated image | `titles/<title>/executable.json` → `runtime.projection`, read by `tools/probe_title_projection.py` | `probe_title_projection.py --title <title>` | One probe serves every title that has a `runtime.projection` block; the compiled constants are diffed against the manifest so the two copies cannot drift |
| Disc provisioning | Follow `SYSTEM.CNF`, select one identity, and publish validated user data | `tools/provision_title.py` | `provision_title.py` | Provisioning publishes bytes for runtime mapping, not offline translation |
| Lightrec executor integration | Bind image-aware native overrides and bounded guest/original calls; run frame turns through the shared host-service dispatcher | `game/core/dynarec_dispatch.*`, `game/core/boundary_runtime.*`, `titles/<title>/core/<title>_runtime.*` | `crash::dynarec::executeTurn`, title runtime composition | psxport owns CPU synchronization, cache invalidation and service dispatch; Crash owns title identity and policy |
| Product composition | Compose one title's native owners and executor without absorbing their implementations | `titles/crash1/core/crash1_port.*`, later title-local equivalents | `crash1::runPort` | Crash 1 remains active until representative gameplay; Crash 2/3 follow sequentially |
| Crash 1 executable admission | Check the retail size and SHA-256 on one bounded byte buffer, then hand that same buffer to psxport's PS-X EXE mapper | `titles/crash1/core/crash1_executable.*` | `crash1::loadResidentExecutable` | Title identity stays in the Crash 1 manifest; psxport owns structural parsing, publication, and invalidation |
| Native frame ownership | Own exact title input, audio, simulation, host-service, render, and presentation order; consume typed executor exits | `titles/<title>/core/<title>_frame_driver.*`, `game/core/native_frame_loop_contract.*` | title `FrameDriver::stepFrame` | Guest VSync produces a typed boundary; no C++ unwind crosses JIT frames |
| Crash 1 native boot services | Preserve measured libcd state, disc-index I/O, callback/event/pad initialization, and GPU watchdog behavior | `titles/crash1/core/crash1_{cd_boot,disc_index_io,callback_boot,gpu_watchdog}.*` | `Crash1Runtime::registerOverrides` | Original guest bodies re-enter through executor original calls; shared Sony semantics stay in psxport |
| Crash 1 BIOS pad input | Publish the finalized host mask in Crash's authenticated BIOS `PadRead` word before retail `PadUpdate` | `titles/crash1/core/crash1_bios_pad_input.*` | `bios_pad_input::publishPrimary` | psxport owns device polling; this owner owns address `0x80057054` and byte order only |
| Crash 1 size-class block pool | Own the engine's block-cell lookup: the 8-byte cell, its class field, the 256-entry bucket table, and the walk's bound. Where the live lookup could only walk off into unmapped memory, the owner applies the bound the ENGINE ITSELF states at `0x800159C4` and returns the engine's own `0xFFFFFFF6`; it refuses to read a bucket that is not a pointer into main RAM, and records the request, class, bucket, pool base, live count, cells walked and first caller so a run can name the wrong value | `titles/crash1/core/crash1_block_pool.*` | `crash1::installCrash1BlockPool`, from `Crash1Runtime::registerOverrides` | The bound is the engine's, transcribed from its own bounded sibling, never a number this repository chose. The JIT still runs the pool's allocate and release callers (issue 0020) |
| Crash 1 BIOS consumer | Record C0 facts and verify the shared BIOS table contract | `titles/crash1/bios_contract.json`, `tests/crash1_c0_exception_contract.cpp` | consumer contract | Crash owns observed use; psxport owns BIOS semantics |
| Crash 2 integration | Own `SCUS_941.54` identity, runtime, exits, and native owners | `titles/crash2/` | `Crash2Runtime` | No Crash 1 address or behavior is inherited without evidence; its projection owner is `crash2_widescreen.*` over the shared rule (issue 0017) |
| Crash 3 integration | Own `SCUS_942.44` identity, runtime, exits, and native owners | `titles/crash3/` | `Crash3Runtime` | `SYSTEM.CNF` selects the title; the bundled Spyro demo executable is not Crash 3. Its projection owner is `crash3_widescreen.*`, which needs a measured pass-through call site because this title reads its published centre back out of CR[24] (issue 0018) |
| Differential evidence | Compare deterministic runtime state with the independent emulator and exercise negative controls | target dynamic harness; existing recorded evidence in `docs/info/` and `docs/re-frontier.md` | separately built diagnostic target | The oracle/interpreter never becomes a product fallback |
| Shared Crash engine | Hold behavior proven common across title binaries | `game/` | assigned only by evidence | Similar names and franchise lineage do not establish ownership |
| Native graphics producers | Convert pre-GTE game camera/object/material state to typed primitives | title-local producer modules, then `game/` if proven shared | future producer interfaces | Never consume GTE/OT/GP0/framebuffer output as product source |
| Widescreen | Widen owned camera/projection, viewport, scissor, and proven horizontal culling | `game/core/guest_projection_publication.*` (the rule) over `titles/crash1/core/crash1_widescreen.*`, `titles/crash2/core/crash2_widescreen.*`, `titles/crash3/core/crash3_widescreen.*` (the measured facts) | `crash1::installCrash1Widescreen`, `crash2::installCrash2Widescreen`, `crash3::installCrash3Widescreen`, and each runtime's `guestWidescreenProjection` | Deterministic geometry expansion only; no final-image stretch, no fps60/interpolation path for these titles; the measured horizontal-bound GUARD for Crash 1 is `titles/crash1/core/crash1_horizontal_bound.*` (issue 0016) | Temporal presentation | Interpolate authoritative previous/current simulation transforms | beside simulation snapshots and renderer consumption | future presentation decorator | Simulation and guest memory remain unchanged; **out of scope for Crash 1** (30 fps, widescreen-only) |

## Where does it go?

| Responsibility | Owner |
|---|---|
| R3000A runtime translation or cache invalidation | psxport's Lightrec executor |
| A title-specific native override or original call | That title runtime, keyed by image identity and guest address |
| A frame/host-work/interrupt stop | A typed psxport executor exit handled by the title frame owner |
| A serial-specific address or BIOS fact | That title's executable manifest and owning module |
| The Crash 1 executable bytes accepted for runtime mapping | `crash1_executable.*`, using the Crash 1 manifest identity |
| Host controller polling | psxport `Pad` |
| Crash 1 BIOS auto-pad layout | `crash1_bios_pad_input.*` |
| A cross-title engine behavior | `game/`, only after direct correspondence |
| Crash 1's horizontal field of view | `crash1_widescreen.*`, at GTE control register 24 (OFX) — measured, not H and not a viewport rectangle |
| Crash 2's and Crash 3's horizontal field of view | `crash2_widescreen.*` / `crash3_widescreen.*`, at GTE control register 24, over `game/core/guest_projection_publication.*` — measured per title, not inherited from Crash 1 |
| A title's projection-plane distance H | **never widened, in any of the three titles**: it is the GTE near plane (`H < Z < 12000`) and, in Crash 2 and Crash 3, a 2D overlay rectangle's height. Held through CR[26] and checked by `observeScreenDistance` |
| A guest read-back of the published centre | `crash3_widescreen.h`'s `kPassThroughCallSites` — the only call site whose argument arrives out of CR[24], and the reason a `retail + margin` owner is not idempotent there |
| Crash 1's horizontal culling | **nothing to widen**: a whole-image census of all 72192 instruction words finds no compare against a 4:3 dot width (see `docs/issues/0015`) |
| Crash 1's size-class cell lookup and its walk bound | `crash1_block_pool.*` at guest `0x80015978`, bounded by the image's own form at `0x800159C4` — the stop address `0x800159A8` is an arm of that loop, not an override key |
| Capability, migration order, evidence, or an atomic blocker | `docs/project-state.md`, `docs/migration.md`, `docs/re-frontier.md`, or `docs/issues/` respectively |

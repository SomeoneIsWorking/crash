# Codemap

Ownership and placement map for the Crash trilogy port. Capability state belongs in
`docs/project-state.md`, migration order in `docs/migration.md`, the ordered binary evidence chain in
`docs/re-frontier.md`, and atomic work in `docs/issues/`. A title's measured binary facts live in
`titles/<title>/executable.json` and the owning header.

The product path is `run.sh` → `titles/crash1/main.cpp` → `crash1::boot::ProductBoot` → psxport's
per-`Core` Lightrec executor with this repository's native owners → `crash1::Crash1FrameDriver`. The
separately built interpreter/oracle path never links into a gameplay executable; Lightrec alone owns
any bounded per-block fallback.

Rules for placement in this repository:

- `titles/<title>/` — everything measured out of ONE executable. A title's addresses, byte order and
  engine behaviour never move to `game/`.
- `game/core/` — behaviour proven COMMON across the three binaries, over `crash::`. Franchise lineage
  and similar names are not evidence; a matching instruction or an identical body hash is.
- `external/psxport/` — the framework. This repository does not edit it; a needed framework change is
  written down in `docs/issues/` instead.

## Directories, namespaces and owners

### `titles/crash1/boot/` — `crash1::boot`

| Symbol | Responsibility |
|---|---|
| `ProductBoot` | Everything between `main` and the guest's first instruction: installs the runtime before constructing `Game`, publishes the disc key, arms the watchdog, admits the authenticated executable, starts the framework services, installs the native owners, enters psxport's spine and logs the run-end counters. |
| `kDiscEnvironmentKey` | The one environment key this title reads its disc through; the framework's disc owner does the reading. |
| `defaultExecutablePath()` | The authenticated retail executable inside the gitignored scratch disc cache. |

### `titles/crash1/core/` — `crash1` (runtime, frame turn) and its subsystem namespaces

| Symbol | Responsibility |
|---|---|
| `Crash1Runtime` (`crash1`) | The title's `GameRuntime`: image facts, platform HLE plan, native override registration, boot dispatch, the widescreen policy, and the owner of the widescreen, horizontal-bound and block-pool objects. |
| `Crash1FrameProgram` / `Crash1FrameDriver` (`crash1`) | The frame turn: enter CoreLoop, service the root counter, publish the BIOS pad word, deliver each measured display field, and commit the presented frame at the measured boundary. |
| `loadVerifiedExecutable` / `loadResidentExecutable` (`crash1`) | Authenticate the retail bytes against `titles/crash1/executable.json` and hand that same buffer to psxport's PS-X EXE mapper. |

| Namespace | Symbols | Responsibility |
|---|---|---|
| `crash1::cd_boot` | `Program`, `registerOverride`, `initializeDriver` | Preserve the measured libcd software state at initialization without entering its VSync-driven controller wait. |
| `crash1::disc_index_io` | `Program`, `registerOverrides`, `applyControl`, `applyControlF`, `applySync`, `applyRead`, `applyReadSync` | Bind the measured stock-libcd wrappers to the framework's synchronous command and data owners. |
| `crash1::callback_boot` | `Program`, `MainDispatch`, `registerOverride`, `initializeDriver` | Create the eight BIOS events, initialize and start the pad service, close the handles — with no guest display wait. |
| `crash1::gpu_watchdog` | `Program`, `registerOverrides`, `start`, `check` | GPU queue timeout bookkeeping against the host-owned display counter. |
| `crash1::bios_pad_input` | `kDisconnectedPort`, `wordAddress`, `publishPrimary` | Publish the framework's active-low PSX mask in Crash 1's BIOS `PadRead` word, in Crash's byte order, before retail `PadUpdate`. |
| `crash1::` widescreen | `kProjectionInit`, `kSetGeomOffset`, `kSetGeomScreen`, `kCentreCallSites`, `Crash1Widescreen`, `installCrash1Widescreen` | This title's measured projection facts and the install of the three projection leaves. The RULE is `crash::GuestProjectionPublication`. |
| `crash1::` horizontal bound | `kHorizontalBound`, `kHorizontalSubmitter`, `Crash1HorizontalBound`, `installCrash1HorizontalBound` | Own the main-RAM bound the guest uses as its horizontal bound and check at the submitter that a widening left it at a value the camera setup can publish. |
| `crash1::` block pool | `Crash1BlockPool`, `installCrash1BlockPool` | The engine's size-class block-cell lookup at `0x80015978`: the 8-byte cell, its class field, the 256-entry bucket table, and the walk. |
| `crash1::pool_node` | `kEntry`, the node-table facts, `takeNode`, the injected seam | A readable, tested model of the pool's allocate path at `0x80012F10`. Not installed as an override; it claims no runtime behaviour. |
| `crash1::` boot frontier | `BootFrontierFacts`, `BiosDispatchResult`, `bootFrontierFacts`, `checkFirstBiosDispatch`, `checkPostGetC0Dispatch` | The manifest's first-syscall and BIOS-dispatch facts as pure predicates. Diagnostic only; no product path calls them. |

### `titles/crash2/core/` and `titles/crash3/core/` — `crash2`, `crash3`

| Symbol | Responsibility |
|---|---|
| `Crash2Runtime` / `Crash3Runtime` (`crash2`, `crash3`) | Each title's measured image facts, platform plan and widescreen policy. They extend `crash::BoundaryRuntime`, so a boot request is a refusal until the title has native boot services and a measured frame boundary. |
| `Crash2FrameDriver` / `Crash3FrameDriver` (`crash2`, `crash3`) | Each title's measured VSync contract and, through `crash::RefusingFrameDriver`, the single honest answer to an attempted frame step. |
| `Crash2Widescreen` / `Crash3Widescreen` (`crash2`, `crash3`) | Each title's measured projection facts over `crash::GuestProjectionPublication`, plus `facts()` and `from()`. |
| `installCrash2Widescreen` / `installCrash3Widescreen` | Install that title's three projection leaves. |

### `game/core/` — `crash` (behaviour proven common across the three binaries)

| Symbol | Responsibility |
|---|---|
| `BoundaryRuntime` | The refusal invariant shared by titles whose measured frontier is not yet a native boot: widescreen-only capabilities and a fatal boot request. |
| `RefusingFrameDriver<TitleDriver>` | A host frame request is fatal, using the title's own measured contract. |
| `GuestFunctionRange`, `NativeFrameLoopState`, `NativeFrameLoopContract` | The title-owned facts at the host/guest frame boundary and what they honestly permit. |
| `makeNativeFramePlatformPlan`, `initializeNativeFrameLoopContract`, `abortUnprovenFrameStep` | The one mapping from a frame contract to a platform HLE plan, the seam a direct product route calls before guest execution, and the refusal beyond the frontier. |
| `GuestProjectionPublication` and `ProjectionTitleFacts` | The trilogy's ONE guest-projection widening rule over per-title measured facts. |
| `crash::dynarec::installOverride`, `executeTurn`, `callGuest`, `callOriginal`, `requireGuestReturn` | The boundary every native owner crosses: an image-scoped override key, a guest turn, a guest call, an original call, and the refusal when a call does not return. |

### `titles/crash1/main.cpp`

Argument handling and composition only: `-h`/`--help`, a usage refusal for any other argument, then
`Crash1Runtime` → `ProductBoot` → run. No boot logic lives here.

## Who owns it

### The frame turn

| Hop | Owner |
|---|---|
| product spine | `crash1::boot::ProductBoot::run` → psxport `native_boot_run` |
| one host frame | `crash1::Crash1FrameDriver::stepFrame` (`Game::frameDriver`) |
| guest work | `crash::dynarec::executeTurn` / `runGuestToBoundary`, through psxport's Lightrec executor |
| boundary | guest `jal` to the libetc VSync leaf, or the `CoreLoop` transition override `finishFrameIteration`, which raises a typed `FrameBoundary` exit |
| boundary check | `crash1::Crash1FrameDriver::isMeasuredFrameBoundary` — the two measured provenances, or a refusal |
| per display field | `deliverDisplayField` → `serviceRootCounter` → `game.spu_audio.frame()` |
| presentation | `FramePresenter::commit` (psxport) at the end of the frame |
| timers | psxport `NativeFrameLoopContract`'s HLE plan guards the guest's VSync; the host never executes it |

While a CD read or a boot service blocks, the host does not step: those bodies run inside
`crash::dynarec::callGuest`/`callOriginal` on the executor's own budget, and the frame turn resumes
only from a typed exit.

### Host input → guest pad buffer

| Hop | Owner |
|---|---|
| host devices and keys | psxport `psx::input::HostInput` (`host_input.h`) — the ONE host input owner: SDL device polling, key events, and the injected/replay sources |
| the pad the guest sees | psxport `Pad::pollHostInput` / `Pad::serviceFrame` (`Game::pad`) |
| effective mask | psxport `Pad::buttons`, active-low PSX order |
| this title's word | `crash1::bios_pad_input::publishPrimary`, into `0x80057054`, before retail `PadUpdate` |
| the guest reads | its own BIOS `PadRead` result; the title owns the address and the byte order only |
| debug control channel | psxport's loopback `dbg_server` (`PSXPORT_DEBUG_SERVER=1`, default 127.0.0.1:5959), driven by the framework's `dbgclient`; it reads host state through `psx::input::HostInput` and does not publish the guest word |

### Guest draw → presentation

| Hop | Owner |
|---|---|
| guest primitives | Lightrec executing the retail draw code; `GpuState` and `RenderQueue` (psxport) receive them |
| widening | `crash1::Crash1Widescreen` → `crash::GuestProjectionPublication::publishCentre` at the measured `set_geom_offset` leaf, moving OFX and holding OFY and H |
| the guard beside it | `crash1::Crash1HorizontalBound::observeSubmitter`, proving the bound a widening must hold did not move |
| presentation | psxport `FramePresenter::commit` at the measured frame boundary, with `game.temporalPresentation` (null here: this title is widescreen-only, with no native producer and no interpolation) |

### CD and streaming

| Hop | Owner |
|---|---|
| disc selection | psxport's disc owner, from `crash1::boot::kDiscEnvironmentKey` |
| library state | `crash1::cd_boot::initializeDriver` |
| the measured stock-libcd leaves | `crash1::disc_index_io::registerOverrides` → psxport's synchronous command and data owners |
| guest CD requests | psxport's `CdcState` / `XaState`, owned by `Game` |

### Audio

| Hop | Owner |
|---|---|
| per display field | `crash1::Crash1FrameDriver::deliverDisplayField` → psxport `SpuAudio::frame` |
| boot | `ProductBoot::startFrameworkServices` → `SpuAudio::init` and `spu_init` |
| streaming | psxport's `XaState`, fed by the framework's CDC |

### Debug and control channel

| Hop | Owner |
|---|---|
| transport | psxport `dbg_server` on loopback; open in every run, moved by `PSXPORT_DEBUG_SERVER` |
| client | the framework's `dbgclient` (`LiveClient`), which `tools/probe_crash1_*.py` drive |
| maintainer legs | `tools/probe_crash1_input.py`, `tools/probe_crash1_primitives.py`, `tools/probe_crash1_widescreen_legs.py` — hand-run, never a gate |
| title-owned state a leg reads | `Crash1Widescreen::published`, `Crash1HorizontalBound::observedBounds`, `Crash1BlockPool::firstCaller` |

## Where does it go?

| Responsibility | Owner |
|---|---|
| R3000A runtime translation or cache invalidation | psxport's Lightrec executor |
| A title-specific native override or original call | `crash::dynarec` over that title's measured addresses |
| A frame/host-work/interrupt stop | a typed psxport executor exit handled by `Crash1FrameDriver` |
| A serial-specific address or BIOS fact | that title's `executable.json` and its owning module |
| The executable bytes accepted for runtime mapping | `crash1_executable.*`, using the Crash 1 manifest identity |
| Host controller polling | psxport `Pad` |
| Crash 1's BIOS auto-pad layout | `crash1_bios_pad_input.*` |
| A cross-title engine behaviour | `game/core/`, only after direct correspondence between the binaries |
| Crash 1's horizontal field of view | `crash1_widescreen.*` at GTE control register 24 (OFX) — measured, not H and not a viewport rectangle |
| Crash 2's and Crash 3's horizontal field of view | `crash2_widescreen.*` / `crash3_widescreen.*` over `game/core/guest_projection_publication.*` — measured per title, never inherited |
| A title's projection-plane distance H | **never widened**: it is the GTE near plane (`H < Z < 12000`) and, in Crash 2 and 3, a 2D overlay rectangle's height. Held through CR[26] and checked by `observeScreenDistance` |
| How the guest reaches the centre leaf, and whether `$r31` names a call site | `ProjectionTitleFacts::centreReach` (`crash::CentreReach`); Crash 1 declares `IndirectCall` (docs/issues/0025), Crash 2 and 3 declare the `$r31 - 4` recovery, which no measured `jal` supports yet |
| A guest read-back of the published centre | `crash3_widescreen.h`'s `kPassThroughCallSites` — measured, but selectable only under the `$r31` recovery (docs/issues/0025) |
| Crash 1's horizontal culling | **nothing to widen**: a whole-image census finds no compare against a 4:3 dot width, and the variable bound the census cannot see is owned by `crash1_horizontal_bound.*` (docs/issues/0015) |
| Crash 1's size-class cell lookup and its walk bound | `crash1_block_pool.*` at guest `0x80015978`; the engine's own bound at `0x800159C4` is reported, not applied (docs/issues/0020) |
| Capability, migration order, a measured fact, or an atomic blocker | `docs/project-state.md`, `docs/migration.md`, `titles/<title>/executable.json`, `docs/issues/` |
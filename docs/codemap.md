# Codemap

Ownership and placement map for the Crash trilogy port. Capability state belongs in
`docs/project-state.md`, migration order in `docs/migration.md`, the ordered binary evidence chain in
`docs/re-frontier.md`, and atomic work in `docs/issues/`. A title's measured binary facts live in
`titles/<title>/executable.json` and the owning header.

The product path is `run.sh` → `titles/crash1/main.cpp` → `crash1::boot::ProductBoot` → psxport's
per-`Core` Lightrec executor with this repository's native owners → `crash1::Crash1FrameDriver`.

## Placement rules

- **One directory is one concept**, and a concept lives in exactly one directory. `CMakeLists.txt`
  declares the concept lists (`CRASH_GAME_DIRS`, `CRASH_TITLE_DIRS`) and gives each target exactly the
  concepts it links, so a header can only be reached through the concept that owns it.
- `titles/<title>/<concept>/` — everything measured out of ONE executable. A title's addresses, byte
  order and engine behaviour never move to `game/`.
- `game/<concept>/` — behaviour proven COMMON across the three binaries, in namespace `crash`. Franchise
  lineage and similar names are not evidence; a matching instruction or an identical body hash is.
- `external/psxport/` — the framework, and in particular `runtime/cpu/`: the image-scoped override key
  (`psx::cpu::installNativeOverride`), the guest turn (`dispatchGuestUntilExit`), the finite guest call
  (`dispatchGuestToReturn`, `callOriginalToReturn`), the call that may cross display fields
  (`psx::cpu::ResumableGuestCall`) and its per-`Core` tally (`Core::guestCallCensus()`). A title
  supplies the addresses, the owner names and the turn caps; it does not own the mechanism. This
  repository does not edit psxport; a needed framework change is written down in `docs/issues/`.
- Nothing is in the global namespace. A concept with state is a class; a stateless rule or a
  Core-to-owner lookup is a function in the same namespace as its directory's owner.

## Directories

| Directory | Namespace | What it owns |
|---|---|---|
| `game/boot/` | `crash` | The refusal invariant for a title with no measured native boot: `BoundaryRuntime`. |
| `game/frame/` | `crash` | The host/guest frame contract, its platform HLE plan, and the refusing frame driver for a title whose frontier is not a frame loop. |
| `game/render/widescreen/` | `crash` | The trilogy's ONE guest-projection widening rule, the measured per-title facts it needs, the Core-to-owner lookup and the install of the three projection leaves. |
| `titles/crash1/boot/` | `crash1::boot`, `crash1::cd_boot`, `crash1::callback_boot`, `crash1::gpu_watchdog`, `crash1` | Process boot (`ProductBoot`), executable admission, and the three native boot services that replace retail VSync-driven waits. |
| `titles/crash1/disc/` | `crash1::disc_index_io` | The measured stock-libcd leaves bound to the framework's synchronous command and data owners. |
| `titles/crash1/entry/` | `crash1` | The title's `GameRuntime`: image facts, HLE plan, override registration, boot dispatch, and the owners it holds. |
| `titles/crash1/frame/` | `crash1` | The frame turn: enter CoreLoop, service the root counter, publish the BIOS pad word, deliver each measured display field, commit the frame. |
| `titles/crash1/input/` | `crash1::bios_pad_input` | Crash 1's BIOS `PadRead` word: the address and Crash's byte order, nothing else. |
| `titles/crash1/render/` | `crash1`, `crash1::pool_node` | The engine's size-class block pool: the cell lookup owner installed as an override, and the allocate-path model that claims no runtime behaviour. |
| `titles/crash1/render/widescreen/` | `crash1` | This title's projection facts over the shared rule, and the horizontal-bound guard beside it. |
| `titles/crash1/debug/` | `crash1` | The manifest's first-syscall and BIOS-dispatch predicates. Diagnostic only: no product path calls them. |
| `titles/crash2/`, `titles/crash3/` | `crash2`, `crash3` | The same `entry/`, `frame/`, `render/widescreen/` shape for Crash 2 and Crash 3. |
| `titles/crash1/main.cpp` | — | Argument handling and composition only. |
| `tests/` | — | Focused tests that drive the production seams; they never restate a rule an owner implements. |
| `tools/` | — | Provisioning, launcher and the hand-run maintainer probes. No C++ owner lives here. |

## `game/` — behaviour proven common across the three binaries

| Class / function | Responsibility |
|---|---|
| `crash::BoundaryRuntime` (`game/boot/`) | The refusal invariant shared by titles whose measured frontier is not yet a native boot: widescreen-only capabilities and a fatal boot request. |
| `crash::GuestFunctionRange`, `NativeFrameLoopState`, `NativeFrameLoopContract` (`game/frame/`) | The title-owned facts at the host/guest frame boundary and what they honestly permit. |
| `crash::makeNativeFramePlatformPlan`, `initializeNativeFrameLoopContract`, `abortUnprovenFrameStep` (`game/frame/`) | The one mapping from a frame contract to a platform HLE plan, the seam a direct product route calls before guest execution, and the refusal beyond the frontier. |
| `crash::RefusingFrameDriver<TitleDriver>` (`game/frame/`) | A host frame request is fatal, using the title's own measured contract. |
| `crash::CentreReach`, `ProjectionTitleFacts` (`game/render/widescreen/`) | The measured per-title projection facts, and whether `$r31` names a call site at all (docs/issues/0025). |
| `crash::GuestProjectionPublication` (`game/render/widescreen/`) | The one widening rule: publish the centre at the measured leaf moving OFX and holding OFY and H, read the retail tuple back out of the coprocessor, and record the H the guest published. Also `from(Core&, site)` — the one Core-to-owner lookup — and `installSites(Core&)`, the one install of the three leaves for all three titles. |

## `titles/crash1/` — one executable, one namespace per concept

| Class / function | Responsibility |
|---|---|
| `crash1::boot::ProductBoot` (`boot/crash1_boot.*`) | Everything between `main` and the guest's first instruction: install the runtime before constructing `Game`, publish the disc key, arm the watchdog, admit the executable, bind the framework's devices through `psx::Machine::bindDevices`, install the native owners, enter psxport's spine, log the run-end counters. |
| `crash1::loadVerifiedExecutable`, `loadResidentExecutable` (`boot/crash1_executable.*`) | Authenticate the retail bytes against `titles/crash1/executable.json`, then hand that same buffer to psxport's PS-X EXE mapper. |
| `crash1::cd_boot::registerOverride`, `initializeDriver` (`boot/crash1_cd_boot.*`) | Preserve the measured libcd software state at initialization without entering its VSync-driven controller wait. |
| `crash1::callback_boot::registerOverride`, `initializeDriver` (`boot/crash1_callback_boot.*`) | The eight BIOS events, the pad service start, and the handle closes — with no guest display wait. |
| `crash1::gpu_watchdog::registerOverrides`, `start`, `check` (`boot/crash1_gpu_watchdog.*`) | GPU queue timeout bookkeeping against the host-owned display counter. |
| `crash1::disc_index_io::registerOverrides`, `applyControl`, `applyControlF`, `applySync`, `applyRead`, `applyReadSync` (`disc/crash1_disc_index_io.*`) | The measured `CdControl`/`CdControlF`/`CdSync`/`CdRead`/`CdReadSync` leaves bound to the framework's synchronous owners. |
| `crash1::Crash1Runtime` (`entry/crash1_runtime.*`) | The title's `GameRuntime`: image facts, platform plan, override registration, boot dispatch, the widescreen policy, and the owners it holds. |
| `crash1::Crash1FrameProgram`, `Crash1FrameDriver` (`frame/crash1_frame_driver.*`) | One host frame: pad publication, the guest turn to the measured boundary, one display field per wait, and the presentation commit. |
| `crash1::bios_pad_input::publishPrimary`, `wordAddress` (`input/crash1_bios_pad_input.*`) | Publish the framework's active-low PSX mask in Crash 1's BIOS `PadRead` word, in Crash's byte order, before retail `PadUpdate`. |
| `crash1::Crash1Widescreen`, `kCentreReach`, the three leaf addresses (`render/widescreen/crash1_widescreen.*`) | This title's measured projection facts and nothing else; the rule, the lookup and the install are the shared owner's. |
| `crash1::Crash1HorizontalBound` (`render/widescreen/crash1_horizontal_bound.*`) | Own the main-RAM bound the guest uses as its horizontal bound, and check at the submitter that a widening left it at a value the measured camera setup can publish. |
| `crash1::Crash1BlockPool` (`render/crash1_block_pool.*`) | The engine's size-class cell lookup at `0x80015978`: the 8-byte cell, its class field, the 256-entry bucket table, and the walk, installed as an override. |
| `crash1::pool_node::takeNode` + the injected seam (`render/crash1_pool_node.*`) | A readable, tested model of the pool's allocate path at `0x80012F10`. Not installed; claims no runtime behaviour. |
| `crash1::bootFrontierFacts`, `checkFirstBiosDispatch`, `checkPostGetC0Dispatch` (`debug/crash1_boot_frontier.*`) | The manifest's first-syscall and BIOS-dispatch facts as pure predicates. |

## `titles/crash2/` and `titles/crash3/`

| Class / function | Responsibility |
|---|---|
| `crash2::Crash2Runtime`, `crash3::Crash3Runtime` (`entry/`) | Each title's measured image facts, platform plan and widescreen policy. They extend `crash::BoundaryRuntime`, so a boot request is a refusal until the title has native boot services and a measured frame boundary. |
| `crash2::Crash2FrameDriver`, `crash3::Crash3FrameDriver` (`frame/`) | Each title's measured VSync contract and, through `crash::RefusingFrameDriver`, the single honest answer to an attempted frame step. |
| `crash2::Crash2Widescreen`, `crash3::Crash3Widescreen` (`render/widescreen/`) | Each title's measured projection facts over `crash::GuestProjectionPublication`. Crash 3 is the only title with a pass-through site. |

## Who owns it

### The frame turn

| Hop | Owner |
|---|---|
| product spine | `crash1::boot::ProductBoot::run` → psxport `native_boot_run` |
| one host frame | `crash1::Crash1FrameDriver::stepFrame` (`Game::frameDriver`) |
| guest work | `psx::cpu::dispatchGuestUntilExit` through `Crash1FrameDriver::runGuestToBoundary` |
| boundary | the guest's own `jal` to the libetc VSync leaf, or the `CoreLoop` transition override `finishFrameIteration`, which raises a typed `FrameBoundary` exit |
| boundary check | `Crash1FrameDriver::isMeasuredFrameBoundary` — the two measured provenances, or a refusal |
| per display field | `deliverDisplayField` → `serviceRootCounter` → `game.spu_audio.frame()` |
| presentation | psxport `FramePresenter::commit` at the end of the frame |

While a CD read or a boot service blocks, the host does not step: those bodies run inside
`psx::cpu::dispatchGuestToReturn`/`callOriginalToReturn` on the executor's own budget, and the frame
turn resumes only from a typed exit.

### Host input → guest pad buffer

| Hop | Owner |
|---|---|
| host devices and keys | psxport `psx::input::HostInput` (`host_input.h`) — the ONE host input owner |
| the pad the guest sees | psxport `Pad::pollHostInput` / `Pad::serviceFrame` (`Game::pad`) |
| this title's word | `crash1::bios_pad_input::publishPrimary`, into `0x80057054`, before retail `PadUpdate` |
| the guest reads | its own BIOS `PadRead` result; the title owns the address and the byte order only |
| debug control channel | psxport's loopback `dbg_server` (`PSXPORT_DEBUG_SERVER=1`, default 127.0.0.1:5959), driven by the framework's `dbgclient`; it reads host state through `HostInput` and does not publish the guest word |

### Guest draw → presentation

| Hop | Owner |
|---|---|
| guest primitives | Lightrec executing the retail draw code; `GpuState` and `RenderQueue` (psxport) receive them |
| widening | `crash::GuestProjectionPublication::publishCentre` at the measured `set_geom_offset` leaf, installed by `installSites` |
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
| device binds | `ProductBoot::bindFrameworkDevices` → `psx::Machine::bindDevices` (the framework's measured order and per-instance binds) |
| streaming | psxport's `XaState`, fed by the framework's CDC |

### Debug and control channel

| Hop | Owner |
|---|---|
| transport | psxport `dbg_server` on loopback; open in every run, moved by `PSXPORT_DEBUG_SERVER` |
| client | the framework's `dbgclient` (`LiveClient`), which the `tools/probe_crash1_*.py` maintainer legs drive |
| title-owned state a leg reads | `Crash1Widescreen::published`, `Crash1HorizontalBound::observedBounds`, `Crash1BlockPool::firstCaller` |

## Where does it go?

| Responsibility | Owner |
|---|---|
| R3000A runtime translation or cache invalidation | psxport's Lightrec executor |
| A title-specific native override or original call | `psx::cpu::installNativeOverride` over that title's measured addresses, and `psx::cpu::callOriginalToReturn` / `dispatchGuestToReturn` when the override needs its own guest body back. A call that may outlive one display field is `psx::cpu::ResumableGuestCall` (`begin`/`advance`, `callGuestToReturnResuming`, `callOriginalResumingToReturn`), and `Core::guestCallCensus()` reports what the run spent. |
| A frame/host-work/interrupt stop | a typed psxport executor exit handled by `Crash1FrameDriver` |
| A serial-specific address or BIOS fact | that title's `executable.json` and its owning module |
| The executable bytes accepted for runtime mapping | `titles/crash1/boot/crash1_executable.*` |
| Host controller polling | psxport `Pad`, fed by `psx::input::HostInput` |
| Crash 1's BIOS auto-pad layout | `titles/crash1/input/crash1_bios_pad_input.*` |
| A cross-title engine behaviour | `game/<concept>/`, only after direct correspondence between the binaries |
| Crash 1's horizontal field of view | `titles/crash1/render/widescreen/crash1_widescreen.*` at GTE control register 24 (OFX) — measured, not H and not a viewport rectangle |
| Crash 2's and Crash 3's horizontal field of view | their `render/widescreen/` owners over `game/render/widescreen/` — measured per title, never inherited |
| A title's projection-plane distance H | **never widened**: it is the GTE near plane (`H < Z < 12000`) and, in Crash 2 and 3, a 2D overlay rectangle's height. Held through CR[26] and checked by `observeScreenDistance` |
| How the guest reaches the centre leaf, and whether `$r31` names a call site | `ProjectionTitleFacts::centreReach` (`crash::CentreReach`); Crash 1 declares `IndirectCall` (docs/issues/0025) |
| A guest read-back of the published centre | `crash3_widescreen.h`'s `kPassThroughCallSites` — measured, but selectable only under the `$r31` recovery (docs/issues/0025) |
| Crash 1's horizontal culling | **nothing to widen**: a whole-image census finds no compare against a 4:3 dot width, and the variable bound the census cannot see is owned by `crash1_horizontal_bound.*` (docs/issues/0015) |
| Crash 1's size-class cell lookup and its walk bound | `crash1_block_pool.*` at guest `0x80015978`; the engine's own bound at `0x800159C4` is reported, not applied (docs/issues/0020) |
| A measured fact with no product consumer | `titles/crash1/debug/` |
| Capability, migration order, a measured fact, or an atomic blocker | `docs/project-state.md`, `docs/migration.md`, `titles/<title>/executable.json`, `docs/issues/` |
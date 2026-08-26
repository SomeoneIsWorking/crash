# Crash trilogy port

This repository ports Crash Bandicoot 1, 2, and 3 on one shared engine lineage. Shared engine code
belongs in `game/`; title-specific seams, executable facts, and assets belong in
`titles/crash1/`, `titles/crash2/`, and `titles/crash3/`.

The repository product is complete only when all three title integrations run: Crash Bandicoot,
Crash Bandicoot 2, and Crash Bandicoot 3. A runnable Crash 1 integration alone does not satisfy that
contract. This is product scope, not evidence that Crash 2 or Crash 3 works today; current measured
coverage is tracked in `docs/project-state.md`, ownership in `docs/codemap.md`, and ordered binary
evidence in `docs/re-frontier.md`.

Read `external/psxport/CLAUDE.md` and `external/psxport/docs/workspace/PROTOCOL.md` before work.
Generated code is sacrosanct. Never commit discs, extracted executables, `generated/`, `.env`, or
machine-specific paths. Run artifacts go under `scratch/`, never `/tmp`.

`python3 tools/verify.py` is the authoritative normal verifier. It always configures and builds the
documented Clang tree at `scratch/build-clang/` before running its complete CTest graph and framework
pin check. A raw `ctest --test-dir scratch/build-clang` is only a post-build focused rerun: CTest does
not reconfigure CMake and can otherwise execute a stale test graph.

Clang is the maintainer verification toolchain, not a player-facing compiler identity restriction.
Top-level CMake accepts the compatible compiler selected by the caller; `tools/verify.py` explicitly
selects Clang for authoritative evidence. Do not add compiler identity allowlists or denylists.

`./run.sh` is the player entry point and currently selects Crash Bandicoot 1, the repository's only
title with a real product executable. Its slim shell shim enters the frozen uv environment and
`tools/run.py` resolves psxport, provisions the user's USA disc, emits the serial-scoped resident
substrate, builds `crash1_port`, and launches that exact product. The launcher never runs
`crash_scaffold`, CTest, an oracle, or a boundary diagnostic. `--prepare-only` exercises the same
provision/build route without starting the product. The executable begins the real retail program at
crt0. Its shipping composition now treats the independently verified `EnterCriticalSection` wrapper as
a returning transition, permits B(56h) through shipping HLE, and stops at the retail-proven following
A(44h) pre-HLE dispatch. A focused Clang build and hermetic product-composition test prove that wiring;
a serialized product execution is still required before claiming runtime reach. The differential proof
still ends at B(56h) before HLE. PSX-SPX grounds Crash's immediate C0 slot 6 consumer as C(06h)
ExceptionHandler at `0x00000C80`, and clean psxport `99a42aa3` plus the Crash consumer CTest now satisfy
that table contract. The real ordered oracle completes the fourteen-word patch but stops at Crash's
local wrapper before its non-link A(44h) tail dispatch; issue #8 owns exact post-model target capture.
This is a real current-frontier product, not a claim that Crash 1 renders a frame or that Crash 2/3
have product targets.

**`external/psxport` is NOT a git submodule** (2026-08-16): it is a symlink to the workspace's shared
framework clone when one exists, or a private clone at this repo's `psxport.pin` on a fresh machine.
`tools/psxport_sync.py --auto` establishes whichever applies; `psxport_sync.py --bump` records the
framework commit this game is built and VERIFIED against, and `--check` fails when the built framework
is not the recorded pin. Framework edits happen in the shared clone (`$PSX/psxport`), never here.

All picture work is RE-driven. Widescreen and interpolation require PC-native graphics producers
reading game state; do not reconstruct pictures from GTE/OT/GP0 output. Each title must first reach a
faithful, measurable base before enhancements.

Crash 1, 2, and 3 are direct runtimes and each explicitly reports `guestVramIsPicture=false` because
none has a measured frame or native picture producer. This is a refusal to claim picture ownership,
not evidence that presentation works; change it per title only when real frame evidence proves the
guest VRAM is that title's picture. Their common `BoundaryRuntime` declares
`RenderCapabilities::interpolatedNative()` because native rendering plus temporal interpolation is the
required product target for the full Crash trilogy; that policy declaration does not invent the
still-missing native producers or interpolation state.

Crash 1's framework seam lives in `titles/crash1/core/crash1_runtime.*`, and its product composition
lives in `titles/crash1/core/crash1_port.*`; `crash1_boot_frontier.*` owns its measured two-stage boot
boundary. It follows Dusklight's composition/owner boundary:
`game/core/resident_program.*` owns the reusable generated-program setup, while
`game/core/enter_critical_frontier.*` owns the measured syscall transition used by both the product
and differential harness. The runtime stays title-owned until cross-title RE proves shared ownership;
the shared modules encode framework execution mechanics, not inferred Crash engine behavior. There
are still no legacy `GameConfig`/`GameHooks` views or a native-boot bypass.

Crash 2 targets the independently measured North American `SCUS-94154` executable `SCUS_941.54`.
Its title runtime lives in `titles/crash2/core/` and owns the measured boot group through the typed
`GuestProgramImage` seam, never through `GameConfig`. `game/core/boundary_runtime.*` shares only the
framework refusal/no-invented-products invariant between title runtimes; it is not evidence of shared
Crash engine behavior. Crash 2 must not reuse Crash 1 seeds, addresses, or syscall conclusions. Its
generated path now agrees with the independent CPU at eight
serial-specific calls through game main `0x80049BD4` and its own `EnterCriticalSection` wrapper
`0x80049D1C`. Cause/EPC, EPC+4 resume, and the following B0 dispatch now agree 34/34; the B0 HLE and frame do not yet execute.

Crash 3 targets the independently measured North American `SCUS-94244` executable `SCUS_942.44`.
`SYSTEM.CNF` is the identity authority: the retail disc also carries `DRAGON/SPYRO.EXE`, which is a
different executable and must never be selected as Crash 3 merely because it is present. Crash 3 owns
its measured boot group through `titles/crash3/core/Crash3Runtime`, with title-scoped cache and
generated paths under `scratch/bin/crash3/` and `generated/crash3/`. Its generated path agrees with
the independent CPU at eight call boundaries through game main `0x80048AA0` and its own
`EnterCriticalSection` wrapper `0x80048C38`. Cause/EPC, EPC+4 resume, and the following B0 dispatch
agree 34/34; the B0 HLE and rendered frame do not yet execute.

Crash 1 targets the North American NTSC-U release (`SCUS-94900`, executable `SCUS_949.00`). The exact
identity/header evidence lives in `titles/crash1/executable.json` and is compared to the real bytes by
`tools/verify_executable.py`. Current execution reaches the real `EnterCriticalSection` syscall wrapper:
the `crash1_port` product and focused harness share the port-side generated wrapper/HLE transition.
The focused gate now validates Cause/EPC, resumes both CPUs at EPC+4, and agrees 34/34 at B(56h), with
`t1=0x56` and `ra=0x800431B8`. It does not execute the B0 HLE. Retail disassembly proves the subsequent
C0 `+0x18` read, fourteen-word copy, and A(44h) tail dispatch with `ra=0x800431E8`; the product is now
composed to stop at that A(44h) boundary after B(56h). The focused product builds and its 14/14
composition test passes, but a serialized product run has not been made and post-B(56h) state is not
independently compared at A(44h). PSX-SPX identifies the slot value as `0x00000C80`; psxport `99a42aa3`
now publishes it without a Crash-local override. The ordered oracle proves the copy completes, then
captures local wrapper `0x8004323C` before its non-link tail dispatch, so A(44h) equality remains
missing. It is not a playable or rendered game.

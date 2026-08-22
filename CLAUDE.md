# Crash trilogy port

This repository ports Crash Bandicoot 1, 2, and 3 on one shared engine lineage. Shared engine code
belongs in `game/`; title-specific seams, executable facts, and assets belong in
`titles/crash1/`, `titles/crash2/`, and `titles/crash3/`.

The repository product is complete only when all three title integrations run: Crash Bandicoot,
Crash Bandicoot 2, and Crash Bandicoot 3. A runnable Crash 1 integration alone does not satisfy that
contract. This is product scope, not evidence that Crash 2 or Crash 3 works today; current measured
coverage is tracked in `docs/codemap.md` and `docs/re-frontier.md`.

Read `external/psxport/CLAUDE.md` and `external/psxport/docs/workspace/PROTOCOL.md` before work.
Generated code is sacrosanct. Never commit discs, extracted executables, `generated/`, `.env`, or
machine-specific paths. Run artifacts go under `scratch/`, never `/tmp`.

`python3 tools/verify.py` is the authoritative normal verifier. It always configures and builds the
documented Clang tree at `scratch/build-clang/` before running its complete CTest graph and framework
pin check. A raw `ctest --test-dir scratch/build-clang` is only a post-build focused rerun: CTest does
not reconfigure CMake and can otherwise execute a stale test graph.

**`external/psxport` is NOT a git submodule** (2026-08-16): it is a symlink to the workspace's shared
framework clone when one exists, or a private clone at this repo's `psxport.pin` on a fresh machine.
`tools/psxport_sync.py --auto` establishes whichever applies; `psxport_sync.py --bump` records the
framework commit this game is built and VERIFIED against, and `--check` fails when the built framework
is not the recorded pin. Framework edits happen in the shared clone (`$PSX/psxport`), never here.

All picture work is RE-driven. Widescreen and interpolation require PC-native graphics producers
reading game state; do not reconstruct pictures from GTE/OT/GP0 output. Each title must first reach a
faithful, measurable base before enhancements.

Crash 1's framework seam lives in `titles/crash1/core/crash1_runtime.*`. It follows Dusklight's
composition/owner boundary: the boundary entry point installs one process-lifetime derived runtime,
then drives only the already-measured generated CPU path. The runtime stays title-owned until
cross-title RE proves shared ownership; it does not invent a shared `game/` layer, legacy
`GameConfig`/`GameHooks` views, runtime products, or a native-boot bypass.

Crash 2 targets the independently measured North American `SCUS-94154` executable `SCUS_941.54`.
Its title runtime lives in `titles/crash2/core/` and owns the measured boot group through the typed
`GuestProgramImage` seam, never through `GameConfig`. `game/core/boundary_runtime.*` shares only the
framework refusal/no-invented-products invariant between title runtimes; it is not evidence of shared
Crash engine behavior. Crash 2 currently ends at the first crt0 call and must not reuse Crash 1 seeds,
addresses, or syscall conclusions.

Crash 3 targets the independently measured North American `SCUS-94244` executable `SCUS_942.44`.
`SYSTEM.CNF` is the identity authority: the retail disc also carries `DRAGON/SPYRO.EXE`, which is a
different executable and must never be selected as Crash 3 merely because it is present. Crash 3 owns
its measured boot group through `titles/crash3/core/Crash3Runtime`, with title-scoped cache and
generated paths under `scratch/bin/crash3/` and `generated/crash3/`. Its generated path agrees with
the independent CPU at eight call boundaries through game main `0x80048AA0` and its own
`EnterCriticalSection` wrapper `0x80048C38`. The oracle stops at the following syscall exception;
no post-syscall equality or rendered frame exists yet.

Crash 1 targets the North American NTSC-U release (`SCUS-94900`, executable `SCUS_949.00`). The exact
identity/header evidence lives in `titles/crash1/executable.json` and is compared to the real bytes by
`tools/verify_executable.py`. Current execution reaches the real `EnterCriticalSection` syscall wrapper:
the port-side generated wrapper and HLE transition are verified, while the independent CPU oracle stops
at `0xBFC00180` because it has no BIOS/syscall-return model. Do not call that post-syscall equality or
advance later boot until the oracle validates Cause/EPC and resumes at EPC+4. None of this implies a
runnable port.

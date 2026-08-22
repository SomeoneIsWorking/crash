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

Crash 1 targets the North American NTSC-U release (`SCUS-94900`, executable `SCUS_949.00`). The exact
identity/header evidence lives in `titles/crash1/executable.json` and is compared to the real bytes by
`tools/verify_executable.py`. Current execution reaches the real `EnterCriticalSection` syscall wrapper:
the port-side generated wrapper and HLE transition are verified, while the independent CPU oracle stops
at `0xBFC00180` because it has no BIOS/syscall-return model. Do not call that post-syscall equality or
advance later boot until the oracle validates Cause/EPC and resumes at EPC+4. None of this implies a
runnable port.

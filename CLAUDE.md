# Crash trilogy port

`AGENTS.md` is the repository-local instruction authority. Read it completely before work. The
product architecture and ordered migration are in `docs/migration.md`; factual coverage is in
`docs/project-state.md`; binary evidence is in `docs/re-frontier.md`.

The target is not the existing static generated-source path. Each Crash product combines native
title owners with psxport's pinned Lightrec executor over the user's verified executable. The
gameplay executable must not link an interpreter or generated guest corpus, and unsupported retail
execution must fail precisely rather than enter a fallback. Do not regenerate, build, or run the
static product while the migration is in progress.

Crash 1's already-measured boot, frame, CD, timing, presentation, camera, and submitter facts remain
the migration frontier. Preserve the in-flight input owner: psxport polls the host pad and finalizes
the active-low mask; `crash1_bios_pad_input.*` publishes Crash's authenticated BIOS `PadRead` word
at `0x80057054` before retail `PadUpdate`. The first Lightrec discriminator is to reach the current
menu frontier with that owner active. This checkpoint does not authorize static-path deletion;
representative interactive gameplay does.

Crash 2 (`SCUS_941.54`) and Crash 3 (`SCUS_942.44`) retain their title-specific executable,
crt0/syscall, game-main, and VSync facts. Crash 3 selection follows `SYSTEM.CNF`, never the unrelated
`DRAGON/SPYRO.EXE` on its disc. Finish Crash 1's representative-gameplay gate before starting either
title's execution migration.

Keep title behavior local until direct cross-title evidence proves a shared `game/` owner. Native
graphics begin at game-state camera and submitter boundaries before GTE projection. Never reconstruct
a product renderer from GTE, ordering-table, GP0, or framebuffer output.

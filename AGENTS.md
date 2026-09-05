# Crash trilogy port agent instructions

This repository targets one native PC product per title, with title-owned native subsystems and
psxport's pinned Lightrec integration executing every remaining retail instruction at runtime.
Read `docs/migration.md`, `docs/project-state.md`, `docs/codemap.md`, and `docs/re-frontier.md` before
implementation. The workspace rules in `../AGENTS.md` and framework-consumer rules in
`external/psxport/AGENTS.md` also apply at the shared runtime boundary.

## Execution contract

- The gameplay products consume the user's identity-verified executable directly through the
  per-`Core` psxport-Lightrec executor. They never emit, compile, link, or select generated guest
  source.
- No full-game interpreter or player-selectable interpreter mode may exist. Lightrec remains the
  mandatory gameplay backend; its internal, bounded block fallback is allowed only for explicit
  backend reasons and must remain measured rather than becoming a second execution engine.
- Native overrides are keyed by complete runtime image identity and guest address. Their original
  calls re-enter retail code through the executor; do not retain generated `super` bodies.
- Frame suspension, host work, interrupts, exceptions, and title exit use explicit bounded executor
  exits. C++ unwinding through JIT frames is forbidden.
- The static translator, generated corpus, dispatch adapters, seed inputs, and static-only tests are
  deleted. Do not reintroduce them; retained measurements in `docs/re-frontier.md` are evidence only.

## Current title discipline

Crash Bandicoot (`SCUS_949.00`) is the active title. Preserve its current menu frontier and the
in-flight BIOS `PadRead` work in issue 0012. Framework `Pad` owns device polling and the finalized
active-low PSX mask; `titles/crash1/core/crash1_bios_pad_input.*` owns only the authenticated
combined word at `0x80057054` and Crash's byte order, published before retail `PadUpdate`.

Do not begin Crash 2 or Crash 3 execution migration until Crash 1 reaches representative gameplay
through Lightrec with its native owners active. Their verified identities, addresses, VSync bodies,
and independent CPU boundaries remain valid evidence, not permission to reuse Crash 1 behavior.

All picture work remains RE-driven. Native producers consume pre-GTE game state. Widescreen changes
owned camera/projection state deterministically; interpolation consumes authoritative
previous/current simulation transforms at presentation time. GTE/OT/GP0 output and framebuffer
pixels are diagnostics, never native producer input.

Never commit game media, extracted executables, generated guest code, `.env`, traces, or
machine-specific paths. Runtime diagnostics use `scratch/`; compiler output uses top-level `build/`.

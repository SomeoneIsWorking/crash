# Crash

Native PC ports of the North American Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport). Each product combines title-owned native
subsystems with psxport's pinned Lightrec executor for every retail instruction that remains guest
owned.

The repository is one engine lineage with title-specific integration under `titles/crash1/`,
`titles/crash2/`, and `titles/crash3/`. Crash Bandicoot is the active title; Crash 2 and Crash 3 do
not begin execution migration until Crash 1 passes representative gameplay.

## Current evidence

- Crash 1 (`SCUS_949.00`), Crash 2 (`SCUS_941.54`), and Crash 3 (`SCUS_942.44`) have verified USA
  executable and disc-selection facts. Crash 3 explicitly rejects its disc's unrelated
  `DRAGON/SPYRO.EXE` as the boot target.
- Independent CPU evidence reaches the recorded resident boundaries in all three titles. Exact
  addresses, hashes, syscall state, VSync bodies, and controls remain in `docs/re-frontier.md`.
- The existing Crash 1 compatibility path reached 1,172/1,172 host frame fences and the 3D title
  menu. Live traces grounded `GfxUpdateMatrices` at `0x80017A14` and `GoolObjectTransform` at
  `0x8001DE78` as pre-GTE ownership candidates.
- Host input reached psxport but not Crash's BIOS auto-pad word. The in-flight owner publishes the
  finalized mask to authenticated address `0x80057054`; issue 0012 records its exact state and
  remaining real-game proof.

These facts define the preserved frontier. They do not make the old generated-source executable the
product and do not prove gameplay, a native renderer, widescreen, or interpolation.

## Product and migration contract

The target `run.sh` path provisions the user's disc, validates its exact title identity, builds the
native/Lightrec product, and launches it without offline guest-code emission. A player build neither
links nor selects an interpreter. The interpreter is a separately built diagnostic oracle only.

The current launcher still belongs to the retired static pipeline and is not a supported migration
command. Do not run it, regenerate its corpus, or collect new static-product evidence. Follow
`docs/migration.md`: first wire the authenticated Crash 1 image and existing native owners through
the psxport executor, then reproduce the current menu/PadRead frontier, then prove representative
interactive gameplay. Only that final gameplay gate authorizes removal of the static generator,
corpus, dispatcher, seeds, and generated-symbol tests.

The game media, extracted executable, traces, and runtime cache remain untracked. Player media
resolution remains explicit argument, title/generic environment or `.env`, then one unambiguous
repository-root CHD. Incorrect or conflicting inputs refuse without replacing a valid selection.

## Native enhancements

Native rendering consumes game-owned camera, transform, material, primitive, and ordering state
before GTE/OT/GP0 submission. Widescreen widens the owned projection and viewport without stretching
the final image. Interpolation retains authoritative previous/current simulation transforms and
decorates presentation only. These enhancements remain off during faithful oracle comparison.

See `docs/project-state.md` for factual coverage, `docs/project-goals.md` for completion conditions,
`docs/codemap.md` for ownership, and `docs/re-frontier.md` for the preserved evidence chain.

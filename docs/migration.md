# Native/Lightrec migration

This is the local execution migration plan. The portfolio authority is
`../../../shared/jit-common/docs/migration.md`; this document applies that contract to the Crash
trilogy without changing the binary evidence in `docs/re-frontier.md`.

## Product boundary

Each gameplay executable contains Crash's native host composition plus psxport's pinned Lightrec
executor. It consumes the user's verified PS-X executable directly. It contains no generated guest
source, static dispatcher, interpreter, or engine fallback. An interpreter may be built only in a
separate diagnostic target.

psxport owns the per-`Core` Lightrec instance, architectural-state synchronization, device/HLE
callbacks, image-aware override table, original calls, bounded exits, and invalidation. Crash owns
title identity, native service/input/frame/render owners, title-specific addresses, and product
composition. Lightrec owns its code cache and executable memory.

## Ordered migration

1. Integrate the shared psxport executor without changing Crash's recorded frontier. Prove one
   authenticated resident override, one override-bypassing original call, and a controlled
   invalidation case through the shipping dispatcher. Product link/selector inspection must prove
   that the interpreter and generated corpus are absent.
2. Migrate Crash 1's runtime composition and native overrides by complete image identity and guest
   address. Replace generated `super` calls with executor original calls. Preserve every measured
   boot, CD, timing, frame, presentation, camera, and submitter contract.
3. Preserve the dirty PadRead/menu work. Framework `Pad` continues to own host polling and the final
   active-low mask; Crash's input adapter publishes the authenticated combined word at `0x80057054`
   before retail `PadUpdate`. Reach the existing 3D menu with nonzero Lightrec blocks and demonstrate
   Cross/Start through the same title owner.
4. Continue Crash 1 to a representative interactive gameplay scenario. Compare timing, interrupts,
   CPU/memory and relevant device state against the independent oracle; exercise native overrides,
   original calls, and executable-memory invalidation with positive and controlled-negative cases;
   qualify the declared released hosts.
5. Only after step 4 passes, remove Crash 1's generated corpus, static dispatch adapters, emitter
   inputs, generated-symbol tests, and obsolete methodology. Do not retain a compatibility switch.
6. Finish Crash 1 before migrating Crash 2, then Crash 3. Each title repeats the same gameplay and
   deletion gate using its own authenticated image and addresses. Shared Crash behavior moves into
   `game/` only after direct cross-title evidence.

## Frozen static path

Do not regenerate, build, or run the static product during this migration. Existing generated-path
measurements remain evidence for the boundary the dynamic product must reproduce. The first Lightrec
wiring discriminator is deliberately weaker than the deletion gate: menu/PadRead reach proves
integration, while representative gameplay proves replacement.

## Enhancement ordering

After faithful representative gameplay, implement native producers from pre-GTE camera, transform,
material, primitive, and ordering state. Widescreen changes the owned projection/viewport; temporal
presentation interpolates authoritative previous/current simulation transforms. Neither enhancement
may become an execution fallback or weaken the faithful baseline.

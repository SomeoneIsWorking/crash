# Native/Lightrec migration

This is the local execution migration plan for the Crash trilogy. Binary evidence remains in
`docs/re-frontier.md`; psxport owns the shared runtime contract.

## Product boundary

Each gameplay executable contains Crash's native host composition plus psxport's pinned Lightrec
executor. It consumes the user's verified PS-X executable directly. It contains no generated guest
source, static dispatcher, full-game interpreter, or engine selector. Lightrec's internal bounded
block fallback is allowed only for explicit backend reasons and remains measured; an independent
interpreter may be built only in a separate diagnostic target.

psxport owns the per-`Core` Lightrec instance, architectural-state synchronization, device/HLE
callbacks, image-aware override table, original calls, bounded exits, and invalidation. Crash owns
title identity, native service/input/frame/render owners, title-specific addresses, and product
composition. Lightrec owns its code cache and executable memory.

## Ordered migration

1. Completed break-first: remove the translator, generated corpus, static dispatch adapters, seed
   inputs, static-only tests, and obsolete methodology without building or running the old product.
2. Integrate the shared psxport executor without changing Crash's recorded frontier. Prove one
   authenticated resident override, one override-bypassing original call, and a controlled
   invalidation case through the shipping dispatcher. Product link/selector inspection must prove
   that a full-game/player-selectable interpreter and generated corpus are absent, and that bounded
   backend fallback cannot become a gameplay mode.
3. Migrate Crash 1's runtime composition and native overrides by complete image identity and guest
   address. Replace generated `super` calls with executor original calls. Preserve every measured
   boot, CD, timing, frame, presentation, camera, and submitter contract.
4. Preserve the PadRead/menu work. Framework `Pad` continues to own host polling and the final
   active-low mask; Crash's input adapter publishes the authenticated combined word at `0x80057054`
   before retail `PadUpdate`. Reach the existing 3D menu with nonzero Lightrec blocks and demonstrate
   Cross/Start through the same title owner.
5. Continue Crash 1 to a representative interactive gameplay scenario. Compare timing, interrupts,
   CPU/memory and relevant device state against the independent oracle; exercise native overrides,
   original calls, and executable-memory invalidation with positive and controlled-negative cases;
   qualify the declared released hosts.
6. Finish Crash 1 before migrating Crash 2, then Crash 3. Each title repeats the same gameplay gate
   using its own authenticated image and addresses. Shared Crash behavior moves into
   `game/` only after direct cross-title evidence.

## Preserved evidence

The removed route's measurements remain evidence for the boundary the dynamic product must
reproduce. They are not executable inputs, compatibility machinery, or permission to restore static
translation. Menu/PadRead reach proves integration; representative gameplay proves fidelity.

## Enhancement ordering

After faithful representative gameplay, implement native producers from pre-GTE camera, transform,
material, primitive, and ordering state. Widescreen changes the owned projection/viewport; temporal
presentation interpolates authoritative previous/current simulation transforms. Neither enhancement
may become an execution fallback or weaken the faithful baseline.

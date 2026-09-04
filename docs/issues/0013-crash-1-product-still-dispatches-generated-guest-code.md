---
id: 13
title: Crash 1 product still dispatches generated guest code
status: open
symptom: The preserved menu frontier depends on an offline-emitted guest corpus instead of the required psxport-Lightrec gameplay executor
state_items: S011
tags: crash1,dynarec,lightrec,migration,product
created: 2026-09-04
updated: 2026-09-04
---

## Root cause

Crash 1 was built around psxport's retired static dispatcher before the portfolio selected one
native/Lightrec product architecture. Ordinary retail calls and native-owner original calls still
resolve to generated host functions, while the existing interpreter is not yet isolated to a
separate diagnostic target.

## Required resolution

Consume psxport's per-`Core` Lightrec executor, map the authenticated `SCUS_949.00` image directly,
and register Crash's existing native owners by complete image identity and guest address. Original
calls must bypass only the current override and execute through Lightrec. Frame/host-work/interrupt
suspension uses explicit executor exits. Product link and selector audits must prove the gameplay
binary contains no interpreter, generated corpus, or fallback.

The first discriminator preserves issue 0012's PadRead owner and reaches the existing 3D menu with
nonzero Lightrec blocks. It does not authorize deletion. Remove the static generator, corpus,
dispatcher, seed inputs, and generated-symbol tests only after representative interactive gameplay,
independent state/device comparison, override/original-call coverage, invalidation controls, and
released-host qualification all pass.

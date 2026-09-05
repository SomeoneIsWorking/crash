---
id: 13
title: Crash 1 product needs verified Lightrec execution
status: open
symptom: Static execution is removed, but the replacement psxport-Lightrec path has not executed the preserved menu frontier
state_items: S011
tags: crash1,dynarec,lightrec,migration,product
created: 2026-09-04
updated: 2026-09-04
---

## Root cause

Crash 1 was built around a static dispatcher rather than a runtime executor. Break-first removal has
eliminated that dispatcher and its corpus. The product now composes the pinned Lightrec executor, but
the preserved real-game menu and input frontier has not been reproduced through it.

## Required resolution

Map the authenticated `SCUS_949.00` image directly through psxport's per-`Core` Lightrec executor
and prove Crash's existing native owners dispatch by complete image identity and guest address.
Original calls must bypass only the current override and execute through Lightrec. Frame/host-work/
interrupt suspension uses explicit executor exits. The existing product link and repository audit
continues to prove the gameplay binary contains no explicit interpreter mode or generated corpus.

The first discriminator preserves issue 0012's PadRead owner and reaches the existing 3D menu with
nonzero Lightrec blocks. Representative interactive gameplay, independent state/device comparison,
override/original-call coverage, invalidation controls, and released-host qualification then prove
the replacement. The deleted static machinery must not return.

---
id: 13
title: Crash 1 product needs verified Lightrec execution
status: investigating
symptom: Lightrec reaches the preserved 3D title menu; interactive gameplay and downstream pad consumption remain unverified
state_items: S011
tags: crash1,dynarec,lightrec,migration,product
created: 2026-09-04
updated: 2026-09-12
---

## Root cause

Crash 1 was built around a static dispatcher rather than a runtime executor. Break-first removal has
eliminated that dispatcher and its corpus. The native frame driver initially treated one finite
Lightrec execution budget as a complete display field; a real run exited normally with
`BudgetExhausted` at `0x80013ADC` during the authenticated decompressor. The driver now resumes the
exact returned guest PC across positive-progress slices while preserving its CNT2 clock. Zero-progress
budget exits become typed faults.

## Required resolution

Map the authenticated `SCUS_949.00` image directly through psxport's per-`Core` Lightrec executor
and prove Crash's existing native owners dispatch by complete image identity and guest address.
Original calls must bypass only the current override and execute through Lightrec. Frame/host-work/
interrupt suspension uses explicit executor exits. The existing product link and repository audit
continues to prove the gameplay binary contains no explicit interpreter mode or generated corpus.

The 2026-09-12 headless/silent authenticated product reached the 3D menu at frame 1,172 and ended
cleanly at frame 1,185. It reported 3,389 translated blocks, 13,964,056 executed blocks,
284,051,734 executed instructions, zero fallback blocks/instructions, and zero refused fallback.
The BIOS PadRead word changed for held Start and Cross, but the menu did not consume either input;
issue 0012 owns that remaining discriminator. Representative interactive gameplay, independent
state/device comparison, override/original-call coverage, invalidation controls, and released-host
qualification then prove the replacement. The deleted static machinery must not return.

The former product path reopened the Python-verified executable with psxport's structural-only
`load_exe`, so a changed cached file could be mapped without title authentication. The Crash 1
loader now checks the manifest size and SHA-256 and passes those same bytes to
`loadPsxExeImage`. Synthetic altered-digest, altered-byte, short-file, and positive publication
controls exercise the shipping loader without putting retail bytes in the test. This closes the
runtime image-admission gap; the menu run above proves execution, while gameplay remains open.

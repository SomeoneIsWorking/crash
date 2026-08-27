---
id: 10
title: Crash 1 page reads ignore CdControlF Setloc and consume stale sequential LBAs
status: resolved
symptom: The real Crash 1 product reads valid NSF bytes for the wrong page because native CdRead never receives the page loader's CdControlF Setloc
state_items: S004
tags: crash1,paging,decompression,disc,boot
created: 2026-08-27
updated: 2026-08-27
---

## Root cause

Crash's page loader uses stock `CdControlF` at `[0x80043668,0x80043984)` for every page Setloc, while
the title had bound only stock `CdControl` at `[0x80043520,0x80043668)` to the shared synchronous
command owner. The generated `CdControlF` body entered an unserviced asynchronous controller path and
never updated native `Cd::setloc_lba`; native `CdRead` then consumed the stale sequential head.

The LBA sequence is the falsifier. The NSD read ends at LBA 51342, after which page reads continue at
51342, 51362, 51376, and 51405—exactly the preceding head plus 20, 14, and 29 sectors. The `pgid
0x0D` payload delivered from LBA 51376 is byte-for-byte the NSF record at offset 34 sectors, while
NSD entry 6 requires offset 126 and LBA 51468. The decompressor and later mixed-group symptoms are
downstream consequences of reading valid bytes for the wrong page. The wrong delivered 29-sector
payload hashes to SHA-256 `32b00ffa225bae3cb72a9ee5c74e021e86335f719047291a54ea427d1d48aa42`;
the required entry-6 payload hashes to
`ec3f010049a1b98cf0561ce1163c45992ca4b10bcdeb5bd792df46aaad22c937`.

## What was tried / dead ends

- Allocation corruption was ruled out: dynamic writes show all 19 physical pages allocated below
  the 2 MiB RAM boundary before the decompressor fault.
- The earlier claim that `pgid 0x0D` matched its real-disc payload was falsified by extracting the
  actual `S1/S0000019.NSD/.NSF`. Guest RAM matches NSF offset 34, not entry 6 at offset 126; the first
  mismatch against the required record is header byte 4.
- A title-local native implementation with a stable source snapshot reproduced the measured token
  and raw-copy algorithms and advanced through the first three inputs. It was removed from the
  product because this only advanced the symptom without grounding the upstream packed-group rule.
- Eagerly snapshotting a whole read group was removed after it inferred the same invalid boundary
  from incomplete semantics.

## Resolution

Authenticated Crash 1 `CdControlF` now routes its two-argument ABI through the existing synchronous
command owner with a null result pointer. Its focused Clang test proves BCD Setloc updates the native
head without treating stale caller `a2` as a writable result address. An isolated real-disc run read
`pgid 0x0D` from LBA 51468; the delivered 29 sectors hashed
`ec3f010049a1b98cf0561ce1163c45992ca4b10bcdeb5bd792df46aaad22c937`, exactly matching extracted NSD
entry 6. The retail decompressor then progressed through subsequent page reads before the distinct
frame-driver boundary.

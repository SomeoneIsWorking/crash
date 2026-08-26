---
id: 8
title: ordered oracle stops before Crash's non-link A44 tail dispatch
status: investigating
symptom: The real ordered syscall and B56 trace captures Crash's local 0x8004323C wrapper instead of the following A(44h) boundary
tags: crash1,oracle,bios,a44,framework
state_items: S003,S004
created: 2026-08-26
updated: 2026-08-26
---

## Root cause

On the verified `SCUS_949.00`, clean psxport `99a42aa3` successfully models selector-1 syscall return,
B(56h) returning `0x8000F800`, and main-RAM seed `0x8000F818=0x00000C80` in one independent CPU. Crash
then completes its fourteen-word copy: at the next captured boundary `v0=0x00000CB8`, exactly one word
past destination range `0x00000C80..0x00000CB7`.

An altered real trace seeds `0x00000C84` instead and reaches the same wrapper and step with
`v0=0x00000CBC`. The four-byte shift is the required opposite answer: the instrument observes the
consumer-owned table word affecting the shipping copy loop rather than printing a fixed expected end.

The generic tracer captures the first subsequent linked call, which is Crash's local wrapper at
`0x8004323C` with `ra=0x800431E8`. That wrapper loads A0 and function 44, then tail-dispatches via
non-link `jr t2`. Because `--capture-at` is mutually exclusive with modeled returns, the current
instrument cannot preserve this same CPU/RAM chain through the exact external A(44h) target.

## Proper fix

The shared oracle must allow an ordered modeled sequence to request exact post-model target
`0x000000A0`, while permitting intermediate linked calls and non-link tail dispatch. It must preserve
the same independent CPU and RAM, refuse if unsupported hardware occurs first, and provide an
opposite-answer control for an unreachable or altered target. Its fixture must include an intermediate
linked wrapper and a non-link external tail dispatch so the direct-call shortcut cannot pass.

After that shared capability lands, Crash compares the A(44h) register file and all fourteen patched
words at `0x00000C80..0x00000CB7`. A separate trace starting at the wrapper is not a substitute because
it discards the syscall, B(56h), seeded table, and copy state that the chain exists to preserve.

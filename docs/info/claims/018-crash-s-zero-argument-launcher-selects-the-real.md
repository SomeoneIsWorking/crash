---
id: C018
kind: claim
status: holds
created: 2026-08-24
tags: launcher,product
depends: CMakeLists.txt
reconfirmed: 2026-09-04
verified_at: 2026-09-04
---

## Claim

Crash's zero-argument launcher selects the native/Lightrec `crash1_port` product through one
frozen-uv Python runtime; provisioning and building use that runtime, missing native dependencies
name actionable install commands, and no launcher path invokes CTest, an oracle, or an offline guest
translator.

## Evidence

The hermetic launcher suite exercises the real `tools/run.py` orchestration through an injected host
boundary. It covers the default route plus dependency, media, toolchain, prepare-only, and launch
refusals without needing copyrighted game data.

## What would falsify it

If a real launcher command trace escapes the locked uv Python runtime, invokes a test/diagnostic or
offline translator instead of `crash1_port`, silently proceeds without a required native dependency,
or zero arguments cease to select `crash1_port`.

## Re-confirmed 2026-09-04

The hermetic launcher test proves the default route provisions the authenticated executable, builds
`crash1_port` directly under `build/player`, and launches it without a guest-source generation stage
or any CTest/oracle command. `run.sh` remains the only shell entry point and delegates immediately to
the locked Python bootstrap.

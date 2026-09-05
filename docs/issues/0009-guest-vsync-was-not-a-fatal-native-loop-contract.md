---
id: 9
title: Guest VSync was not a fatal native-loop contract
status: resolved
symptom: Direct Crash runtimes could reach retail VSync without a host-owned FrameDriver or an installed fatal trap
state_items: S003,S004,S008,S010
tags: crash1,crash2,crash3,frame-loop,vsync,ownership
created: 2026-08-27
updated: 2026-09-04
---

## Root cause

All three title runtimes inherited the default null `GameRuntime::createFrameDriver`, and the runtime
contract test required that null. Their direct `PlatformHlePlan` was also null. Crash 1's resident
product bypasses psxport's native bootstrap, so it did not initialize or require platform HLE before
generated guest dispatch. Reaching libetc VSync would therefore run the retail busy-wait and report
`VSync: timeout` rather than identify a forbidden guest-owned frame dependency.

The verified executable bodies identify separate title facts: Crash 1 VSync
`[0x8003E4F0,0x8003E638)`, Crash 2 `[0x8004A484,0x8004A5CC)`, and Crash 3
`[0x8004B2A8,0x8004B3F0)`. Similar libetc structure does not make their addresses interchangeable.

## Proper fix

Each title runtime supplies its measured VSync entry through `PlatformHlePlan::vsyncAddress`, creates
a title-owned `FrameDriver`, and admits only the measured VSync body in its declared platform window.
The framework owns the one fatal handler and refuses replacement. Crash's resident product must
initialize and require that contract before its first generated dispatch.

Crash 2 and Crash 3 still abort every attempted frame step. Crash 1 now has a measured candidate frame
driver and a direct product route, but remains partial until consecutive product frames return. A driver
becomes runnable only after title-specific RE proves the loop top, simulation boundary, exact pad/audio/
render order, and one presentation commit or measured unpresented fence. Returning a fabricated VSync
value, treating the retail timeout as pacing, or calling guest VSync from the host loop is not a fix.

### Resolution (2026-08-27)
Each title now owns a non-null FrameDriver (initially refusing in all three titles) and manifest-derived
PlatformHlePlan for its measured VSync body. Crash 1 custom resident boot initializes and requires the
shared fatal trap before generated dispatch; VSync(-1), VSync(0), replacement attempts, and unproven
frame steps aborted for all titles at this contract milestone. Clang CTest passed the six focused
runtime/frame/policy tests, retail executable selftests passed 6/6 per title, runtime facts matched
20/20 per title, and crash1_port rebuilt without launch.

Crash 1 subsequently replaced its refusing driver with the measured CoreLoop/GpuUpdate owner. Real
product launches prove the fatal boundary catches boot-time callers too: first libcd initialization,
then callback/event/pad initialization. Their title-local native owners retain the generated bodies and
preserve state-producing initialization without calling VSync. This advances the product frontier but
does not yet prove a completed frame.

### Note (2026-08-27)
The complete configured Clang CTest graph passed 9/9 after the product rebuild, including the boot/C0 contracts, all three runtimes, native frame/VSync death contract, C++ policy, provisioning, and launcher tests.

### Migration note (2026-09-04)

The Lightrec migration replaces the earlier fatal stop with psxport's typed `FrameBoundary`. Crash 1
consumes that result, performs host field work, and resumes at the measured guest return address;
Crash 2 and Crash 3 retain refusing frame drivers. The prior abort evidence above is historical and
must not be read as the current runtime contract.

# Project state

Factual capability coverage. Epic intent lives in `docs/project-goals.md`, ownership in
`docs/codemap.md`, migration order in `docs/migration.md`, atomic work in `docs/issues/`.

| ID | Capability | State | Evidence or gap |
|---|---|---|---|
| S001 | Serial-identified USA executable facts for Crash 1, 2, 3 | verified | `tools/verify_executable.py` matches size, SHA-1/SHA-256, PS-X EXE header, region markers and VSync body for `SCUS_949.00`, `SCUS_941.54`, `SCUS_942.44` |
| S002 | Disc provisioning selects and verifies one boot executable | verified | `tools/provision_title.py` follows `SYSTEM.CNF`; `tests/test_provision_titles.py` covers three-title selection, precedence, ambiguity and Crash 3's `DRAGON/SPYRO.EXE` decoy |
| S003 | Independent CPU comparison of the resident boot spine | partial | 34/34 register/memory agreement through the first post-syscall B0 dispatch per title; Crash 1's ordered oracle still stops at local wrapper `0x8004323C`, before the non-link A(44h) tail dispatch |
| S004 | Preserved Crash 1 compatibility frontier | partial | frozen evidence from the retired static route (1,172/1,172 field fences, 3D menu); do not rebuild or rerun it |
| S005 | Game-owned native renderer submission | missing | no pre-GTE producer, render queue or ordering owner exists; the guest submits its own GP0 traffic, which does not satisfy this |
| S006 | Widescreen through owned camera/projection state | partial | `crash1_widescreen` + `game/core/guest_projection_publication` move OFX and hold OFY/H; a live 16:9 leg widens the host canvas and a positive control widens a real centre, but the guest's own camera never calls `0x80042F8C` in a live level, so no widened frame is proven (issue 0023) |
| S007 | 60 fps interpolation through owned simulation/transform state | missing | requires S005; no `TemporalSceneSource` and no native render path |
| S008 | Crash 2 and Crash 3 native/Lightrec products | missing | identities, runtimes, VSync facts and widescreen owners exist; both runtimes refuse to boot and their overrides are installed by tests only (issues 0017, 0018) |
| S009 | Playable trilogy | missing | no title reaches representative gameplay |
| S010 | Host-owned frame-loop contract with typed guest-VSync exits | partial | three measured VSync leaves bound to typed exits; only Crash 1 has a measured advancing loop |
| S011 | Crash 1 native/Lightrec product | partial | disc-backed 400-frame run exits 0 with 3,461,249 executed blocks, 46,438,388 instructions, 0 fallback blocks, 239,549 guest GP0 primitives and 6/6 captured presented frames carrying a picture; input reaches the title and the camera path still does not run (issues 0023, 0024), so this is not a gameplay claim |
| S012 | Linux x86-64 host qualification | partial | the locked asset-free gate and hosted CI pass; real-game execution and release performance are unqualified |
| S013 | Windows x86-64 host qualification | missing | psxport refuses the Windows product target |
| S014 | Apple Silicon macOS host qualification | missing | psxport refuses AArch64; executable memory, ABI, packaging and gameplay unqualified |
| S015 | Android arm64-v8a host qualification | missing | no package, shared Android-port integration, touch layer or AArch64 gameplay run |
| S016 | Loading removal for all three titles | missing | no load operation enumerated or classified yet |

## Comparison baseline

Vanilla PlayStation emulation of each North American retail disc.

| Delta | State | Evidence or gap |
|---|---|---|
| Widescreen | partial | owner widens the host canvas and its arithmetic is proven by a control; no live widened frame |
| 60 fps interpolation | missing | S007 |
| Loading removal | missing | S016 |
| Setup | partial | `run.sh` provisions, identity-checks and launches from a fresh clone; no packaged first-run picker yet |
| Platforms | partial | Linux x86-64 only; S013–S015 |

Current focus: S011 — find why the level's camera never publishes a centre through `SetGeomOffset`
in a live unpaused level, then reach representative interactive gameplay.

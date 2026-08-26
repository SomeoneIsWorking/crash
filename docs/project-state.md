# Project state

Factual capability coverage for the Crash trilogy port. Epic intent is not tracked here; atomic work
lives in `docs/issues/`, ownership in `docs/codemap.md`, and ordered binary evidence in
`docs/re-frontier.md`.

| ID | Capability / observable outcome | State | Dependencies | Goals |
|---|---|---|---|---|
| S001 | Serial-identified USA executable facts for Crash 1, 2, and 3 | verified | — | G001 |
| S002 | Disc provisioning selects and verifies each title's boot executable | verified | S001 | G001 |
| S003 | Independent CPU comparison of the resident boot spine | partial | S001 | G001 |
| S004 | Crash 1 shipping product executes the measured resident boot spine | partial | S002, S003 | G001 |
| S005 | Game-owned native renderer submission path | missing | S004 | G002 |
| S006 | Widescreen through owned camera/projection state | missing | S005 | G002 |
| S007 | Interpolation through owned simulation and transform state | missing | S005 | G002 |
| S008 | Crash 2 and Crash 3 shipping products | missing | S002, S003 | G001 |
| S009 | Playable Crash trilogy product | missing | S004, S005, S008 | G001 |

## Current focus

S004 is the current focus: advance Crash 1's serialized and independently compared execution spine
from B(56h) through A(44h), then continue toward the first frame-producing loop.

## Capability details

### S001 — serial-identified executable facts

Evidence: `titles/crash1/executable.json`, `titles/crash2/executable.json`, and
`titles/crash3/executable.json` match each retail USA executable's size, hashes, PS-X EXE header, and
region markers through `tools/verify_executable.py`; the three positive/negative selftests pass.

### S002 — title-scoped disc provisioning

Evidence: `tools/provision_title.py` follows `SYSTEM.CNF`, verifies all tracked executable facts, and
publishes only into the selected title's gitignored cache. `tests/test_provision_titles.py` covers the
three-title positive route, precedence, invalid configured paths, ambiguity, identity disagreement,
and Crash 3's unrelated `DRAGON/SPYRO.EXE` decoy.

### S003 — independent resident boot comparison

The independent Mednafen CPU and shipping generated path agree 34/34 for each title through its first
post-syscall B0 dispatch. Crash 1 additionally validates manifest-backed B(56h) facts and all three
titles validate Cause/EPC plus EPC+4 resume. Clean psxport `99a42aa3` and the Crash consumer CTest now
agree that B(56h)'s C0 slot 6 contains `0x00000C80`.

Gap: Equality stops before the first B0 HLE call. The real ordered oracle now applies selector-1 return,
B(56h), and the C0 seed in one CPU and proves the fourteen-word copy completed, but its generic
post-model policy stops at local wrapper `0x8004323C`. It cannot yet cross that wrapper's non-link tail
dispatch to A(44h) while preserving the chain (issue #8). Static emission also contains data-like
candidates tracked by issue #3, so candidate count is not execution coverage.

### S004 — Crash 1 shipping product

`crash1_port` loads the verified executable at its real crt0 entry. Its composition returns from the
measured `EnterCriticalSection` syscall, permits B(56h) through shipping HLE, and stops at the
retail-proven following A(44h) pre-HLE boundary. A focused Clang build and 14/14 positive/negative
composition checks pass.

Gap: No serialized product process was launched for this milestone, so runtime reach to A(44h) is not
verified. Shipping B(56h) now supplies the grounded C0 slot and the real ordered oracle executes the
fourteen-word copy, but capture stops at Crash's local `0x8004323C` wrapper before its non-link A(44h)
tail dispatch. The A(44h) register file and patched words at `0x00000C80..0x00000CB7` therefore remain
uncompared. Later BIOS/hardware boot is absent and no frame loop has executed. Issue #8 owns this
atomic capture gap.

### S005 — game-owned native renderer submission

Missing capability: no Crash frame-producing loop, game-code graphics submitter, native primitive
producer, or renderer submission owner has been reached or implemented. All title runtimes explicitly
declare the required native/interpolation product profile but continue to report
`guestVramIsPicture=false`; capability declaration is not a rendered frame.

### S006 — widescreen

Missing capability: no owned camera or projection producer exists, so there is no grounded state that
can widen view geometry. Post-projection GTE/OT/GP0 reconstruction is explicitly not ownership.

### S007 — interpolation

Missing capability: no authoritative simulation tick and no owned previous/current camera or object
transform producers exist, so there is no state pair a native renderer can interpolate.

### S008 — Crash 2 and Crash 3 products

Missing capability: Crash 2 and Crash 3 have verified executable identities, typed runtimes, and
pre-B0 differential boundaries, but no shipping player executables or launch routes.

### S009 — playable trilogy

Missing capability: none of the three titles reaches verified gameplay; Crash 1 has no frame/input/audio
loop, while Crash 2 and Crash 3 have no product targets.

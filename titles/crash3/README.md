# Crash Bandicoot: Warped (`SCUS-94244`)

This integration targets the North American executable `SCUS_942.44`. The real disc's `SYSTEM.CNF`
selects that file even though the same disc also contains `DRAGON/SPYRO.EXE`; file presence never
overrides the boot configuration. `executable.json` records the measured identity and PS-X EXE header.

`core/Crash3Runtime` directly inherits the boundary-only shared runtime and owns the measured
`GuestProgramImage`, including game main `0x80048AA0` observed as executed call three. The title's
provisioned cache, generated substrate, and evidence traces are isolated at `scratch/bin/crash3/`,
`generated/crash3/`, and `scratch/raw/scus-94244-recomp/`.

Generated execution agrees with the independent CPU oracle 34/34 at each of the first eight calls,
ending at `SCUS_942.44`'s `0x80048C38` `EnterCriticalSection` wrapper. Two instructions later the
oracle enters `0xBFC00180`; Cause `0x20`, EPC `0x80048C3C`, the EPC+4 resume, and the following B0
dispatch agree exactly. The B0 HLE itself has not yet executed under the differential gate.
The structural first-call slot `0x800112B8` is not an A(39h) thunk, so applying the oracle's A0
modeled-return mechanism there is explicitly refused. Later boot, a frame, gameplay, and enhancements
remain unimplemented.

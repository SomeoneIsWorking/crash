# Crash Bandicoot: Warped (`SCUS-94244`)

This integration targets the North American executable `SCUS_942.44`. The real disc's `SYSTEM.CNF`
selects that file even though the same disc also contains `DRAGON/SPYRO.EXE`; file presence never
overrides the boot configuration. `executable.json` records the measured identity and PS-X EXE header.

`core/Crash3Runtime` directly inherits the boundary-only shared runtime and owns the measured
`GuestProgramImage`. The title's provisioned cache and future generated substrate are isolated at
`scratch/bin/crash3/` and `generated/crash3/`. Current verification ends at the first executed crt0
call to `0x800112B8`; generated execution, later BIOS/hardware boot, gameplay, and enhancements remain
unimplemented.

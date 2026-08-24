# Crash Bandicoot 2: Cortex Strikes Back

This title targets the measured North American `SCUS-94154` release, executable `SCUS_941.54`.
`executable.json` records the exact hashes and PS-X EXE header facts; copyrighted disc/executable data
stays under gitignored `scratch/`.

`Crash2Runtime` owns the measured crt0 and resident-image facts through `GuestProgramImage`, reached by
direct `GameRuntime` inheritance through the boundary-only integration base. It exposes no legacy
`GameConfig`/`GameHooks`, context, frame driver, scheduler, overrides, or native-boot bypass.

Current execution evidence reaches eight real calls. The generated path and independent Mednafen CPU
agree 34/34 at each boundary, including game main `0x80049BD4` and the title's own
`EnterCriticalSection` wrapper `0x80049D1C`. The generated wrapper separately proves the shipping HLE
effect, while the independent CPU enters `0xBFC00180` at step 85,898. The oracle cannot yet expose the
syscall Cause/EPC or resume at EPC+4, so there is no post-syscall equality, BIOS continuation, frame,
graphics producer, widescreen path, or gameplay claim.

# Crash Bandicoot 2: Cortex Strikes Back

This title targets the measured North American `SCUS-94154` release, executable `SCUS_941.54`.
`executable.json` records the exact hashes and PS-X EXE header facts; copyrighted disc/executable data
stays under gitignored `scratch/`.

`Crash2Runtime` owns the measured crt0 and resident-image facts through `GuestProgramImage`, reached by
direct `GameRuntime` inheritance through the boundary-only integration base. It exposes no legacy
`GameConfig`/`GameHooks`, context, frame driver, scheduler, overrides, or native-boot bypass.

Current execution evidence ends at the first real crt0 call: the symbolic decoder and independent
Mednafen CPU agree 6/6 at step 81,725, target `0x8001144C`. There is no generated Crash 2 execution,
BIOS continuation, frame, graphics producer, widescreen path, or gameplay claim yet. `gameMainEntry`
therefore remains zero instead of borrowing or guessing an address.

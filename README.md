# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

The shipping contract covers all three games. Crash 2 and Crash 3 are not optional follow-ups, but
their presence in this repository must not be read as implementation status; the codemap records the
measured frontier for each serial-identified title.

Current status: framework scaffold plus verified North American disc/executable identities for Crash 1
(`SCUS-94900`), Crash 2 (`SCUS-94154`), and Crash 3 (`SCUS-94244`). Crash 2 and Crash 3 each have a
symbolic crt0 decode agreeing 6/6 with the independent CPU oracle at the first real call, and each
derived runtime's typed executable facts agree 15/15 with its retail bytes. Crash 3's `SYSTEM.CNF`
selection is explicit because its disc also contains the unrelated bootable-looking
`DRAGON/SPYRO.EXE`. The clean Clang/CTest/real-data gates and all title regressions use recorded
psxport `57a17a14`. Crash 1 has 653 emitted static candidates. Its generated path agrees with the independent oracle on all 34
CPU-state fields at the first eight executed calls. Nine addresses in the candidate set have
execution provenance: eight generated bodies execute before the eighth observed target. The eighth
target is the real `addiu $a0, 1; syscall 0` wrapper for `EnterCriticalSection`; the controlled port
boundary executes that generated wrapper and proves the shipping HLE returns the prior IRQ state and
disables delivery. Crash 3 independently emits 986 candidates from `SCUS_942.44`; its first eight
calls also agree 34/34, including tracked game main `0x80048AA0` and its own syscall wrapper
`0x80048C38`. The independent CPU instead enters `0xBFC00180`, correctly, because its focused
oracle has no BIOS or syscall-return model. There is now a direct, title-owned `Crash1Runtime` seam
with no legacy `GameConfig`/`GameHooks` views, but still no runnable port, full oracle boot, native
producer, widescreen path, or interpolation path.

## Verify the framework scaffold

```sh
python3 tools/psxport_sync.py --auto
python3 tools/verify.py
```

The verifier always configures and builds the authoritative `scratch/build-clang/` tree with Clang
before running its complete CTest graph and framework pin check. This ordering is required: invoking
`ctest` directly does not reconfigure CMake and can run a stale test graph after targets change. Raw
`ctest --test-dir scratch/build-clang` is only a focused rerun after `tools/verify.py` has refreshed
the tree. The shared framework checker applies this repository's tracked `clang-format` and
`clang-tidy` policy and the 1,200-line ownership cap without linting `external/psxport` or generated
code.

## Provision a measured title

Disc images and extracted executables are never committed. After configuring the Clang build above,
provision the USA disc; the tool builds psxport's disc reader target when needed:

```sh
python3 tools/provision_title.py --title crash1 /path/to/Crash-Bandicoot-USA.chd
python3 tools/provision_title.py --title crash2 /path/to/Crash-Bandicoot-2-USA.chd
python3 tools/provision_title.py --title crash3 /path/to/Crash-Bandicoot-Warped-USA.chd
```

The disc argument may instead come from the title key (`PSXPORT_CRASH1_DISC` or
`PSXPORT_CRASH2_DISC` or `PSXPORT_CRASH3_DISC`), `PSXPORT_DISC`, the same keys in the gitignored
`.env`, or one `*.chd` drop-in at the repository root, in that order. A configured missing path and
multiple drop-ins are refused. The shared implementation publishes into the matching
`scratch/bin/<title>/` only after `SYSTEM.CNF` selects that title's serial-coded executable and all 11
tracked executable facts match. For Crash 3 this deliberately ignores the disc's bundled
`DRAGON/SPYRO.EXE`; presence is not selection.

After `tools/verify.py` refreshes the authoritative tree, a focused policy/provisioning rerun is:

```sh
ctest --test-dir scratch/build-clang --output-on-failure \
  -R '^(crash_cpp_policy|crash_title_provision_tests)$'
```

## Cross-check the first oracle window

The asset-gated target re-provisions the executable, runs the independent CPU's permanent 39-case
fixture, then compares symbolic crt0 decoding with execution at the first real call boundary:

```sh
PSXPORT_CRASH1_DISC=/path/to/Crash-Bandicoot-USA.chd \
  cmake --build scratch/build-clang --target crash1_oracle_boot_check

PSXPORT_CRASH2_DISC=/path/to/Crash-Bandicoot-2-USA.chd \
  cmake --build scratch/build-clang --target crash2_oracle_boot_check

PSXPORT_CRASH3_DISC=/path/to/Crash-Bandicoot-Warped-USA.chd \
  cmake --build scratch/build-clang --target crash3_oracle_boot_check
```

Each target verifies only that title's crt0 call boundary. Crash 2 and Crash 3 also compare all 15
typed runtime image facts against the executable and prove an altered fact disagrees. Neither target
claims a full oracle boot or a runnable PC port. CMake reserves independent
`generated/<title>/` namespaces; Crash 2 still has no generated substrate.

## Cross-check the resident calls before the first BIOS boundary

Once the verified executable exists under its serial-scoped cache, an asset-gated target emits the
resident substrate, builds its focused runner, compares the complete port-side register file against
the independent oracle, and proves the comparator's opposite answer:

```sh
cmake --build scratch/build-clang --target crash1_recomp_boundary_check -j16
cmake --build scratch/build-clang --target crash3_recomp_boundary_check -j16
```

The positive path compares `pc`, all 31 nonzero general-purpose registers, `lo`, and `hi` at calls
`0x80011A18`, `0x80011D88`, `0x8003E0C0`, `0x8001652C`, `0x8003F224`, `0x80042B1C`, `0x8004319C`,
and `0x8003E1F8` (34/34 each). All reference states come from psxport's canonical ordinal call
capture; Crash does not duplicate the oracle's MIPS call-tracking rules. The controls require the
real emitter to refuse an out-of-text seed and the same comparator to name a deliberately altered
eighth-boundary `a0`; a real oracle trace capped immediately before call eight must report only
`7/8` boundaries and refuse. Generated code remains gitignored and is never edited. This establishes
the resident entry path through the final executed `jal` before the independent CPU leaves mapped
text at `0xBFC00180`. The gate additionally executes the measured generated syscall wrapper on the
port side, checks `EnterCriticalSection` changes IRQ state `1 -> 0` and returns `1`, and refuses a
different execution-proven function as the wrapper. This is deliberately not reported as
port/oracle agreement after the syscall: the independent harness cannot yet validate CP0
Cause/EPC, model the BIOS syscall return, and resume at EPC+4. BIOS, hardware, and later boot are
still unverified. The emitter's 115 static seeds and 653 emitted candidates are discovery
denominators, not claims that every candidate is executable code.

The Crash 3 target uses only `SCUS-94244` artifacts and `titles/crash3/recomp_seeds.json`. Its eight
call targets are `0x800112B8`, `0x80011628`, game main `0x80048AA0`, `0x800154AC`, `0x8004BFBC`,
`0x8004F37C`, `0x8004F914`, and `0x80048C38`; each agrees 34/34. At step 75,963 the independent CPU
enters `0xBFC00180` after the last target's syscall. No post-syscall equality or rendered-frame claim
follows from this gate.

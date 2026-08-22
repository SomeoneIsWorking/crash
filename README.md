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
(`SCUS-94900`) and Crash 2 (`SCUS-94154`). Crash 2's symbolic crt0 decode agrees 6/6 with the independent
CPU oracle at its first real call, and its derived runtime's typed executable facts agree 15/15 with the
retail bytes. The clean Clang/CTest/real-data gates and both title regressions use recorded psxport
`ad5cf802`. Crash 1 has 653 emitted static candidates. Its
generated path agrees with the independent oracle on all 34
CPU-state fields at the first eight executed calls. Nine addresses in the candidate set have
execution provenance: eight generated bodies execute before the eighth observed target. The eighth
target is the real `addiu $a0, 1; syscall 0` wrapper for `EnterCriticalSection`; the controlled port
boundary executes that generated wrapper and proves the shipping HLE returns the prior IRQ state and
disables delivery. The independent CPU instead enters `0xBFC00180`, correctly, because its focused
oracle has no BIOS or syscall-return model. There is now a direct, title-owned `Crash1Runtime` seam
with no legacy `GameConfig`/`GameHooks` views, but still no runnable port, full oracle boot, native
producer, widescreen path, or interpolation path.

## Configure the framework scaffold

```sh
python3 tools/psxport_sync.py --auto
CC=clang CXX=clang++ cmake -S . -B scratch/build-clang \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
CCACHE_DIR=scratch/ccache cmake --build scratch/build-clang --target crash_scaffold
```

Run `ctest --test-dir scratch/build-clang --output-on-failure -R crash_cpp_policy` for the normal
first-party C++ gate. The shared framework checker applies this repository's tracked `clang-format`
and `clang-tidy` policy and the 1,200-line ownership cap without linting `external/psxport` or
generated code. The runtime owner, its focused test, and the boundary runner are the repository's
compile-backed first-party translation units.

## Provision a measured title

Disc images and extracted executables are never committed. After configuring the Clang build above,
provision the USA disc; the tool builds psxport's disc reader target when needed:

```sh
python3 tools/provision_title.py --title crash1 /path/to/Crash-Bandicoot-USA.chd
python3 tools/provision_title.py --title crash2 /path/to/Crash-Bandicoot-2-USA.chd
```

The disc argument may instead come from the title key (`PSXPORT_CRASH1_DISC` or
`PSXPORT_CRASH2_DISC`), `PSXPORT_DISC`, the same keys in the gitignored `.env`, or one `*.chd` drop-in
at the repository root, in that order. A configured missing path and multiple drop-ins are refused.
The shared implementation publishes into the matching `scratch/bin/<title>/` only after `SYSTEM.CNF`
selects that title's serial-coded executable and all 11 tracked executable facts match.

Run `ctest --test-dir scratch/build-clang --output-on-failure -R
'^(crash_cpp_policy|crash_title_provision_tests)$'` for the normal C++ policy and provisioning both-answer
tests.

## Cross-check the first oracle window

The asset-gated target re-provisions the executable, runs the independent CPU's permanent 39-case
fixture, then compares symbolic crt0 decoding with execution at the first real call boundary:

```sh
PSXPORT_CRASH1_DISC=/path/to/Crash-Bandicoot-USA.chd \
  cmake --build scratch/build-clang --target crash1_oracle_boot_check

PSXPORT_CRASH2_DISC=/path/to/Crash-Bandicoot-2-USA.chd \
  cmake --build scratch/build-clang --target crash2_oracle_boot_check
```

Each target verifies only that title's crt0 call boundary. Crash 2 also compares all 15 typed runtime
image facts against the executable and proves an altered fact disagrees. Neither target claims a full
oracle boot or a runnable PC port.

## Cross-check the resident calls before the first BIOS boundary

Once the verified executable exists under `scratch/bin/crash1/`, one asset-gated target emits the
resident substrate, builds its focused runner, compares the complete port-side register file against
the independent oracle, and proves the comparator's opposite answer:

```sh
cmake --build scratch/build-clang --target crash1_recomp_boundary_check -j16
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

# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

Current status: framework scaffold plus a verified Crash 1 North American disc/executable identity
and 653 emitted static candidates. The generated path agrees with the independent oracle on all 34
CPU-state fields at the first eight executed calls. Nine addresses in the candidate set have
execution provenance: eight generated bodies execute before the eighth observed target. Two oracle
instructions later, execution leaves mapped executable text at the BIOS exception vector. There is
now a direct, title-owned `Crash1Runtime` seam with no legacy `GameConfig`/`GameHooks` views, but
still no runnable port, full oracle boot, native producer, widescreen path, or
interpolation path.

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

## Provision Crash Bandicoot

Disc images and extracted executables are never committed. After configuring the Clang build above,
provision the USA disc; the tool builds psxport's disc reader target when needed:

```sh
python3 tools/provision_crash1.py /path/to/Crash-Bandicoot-USA.chd
```

The disc argument may instead come from `PSXPORT_CRASH1_DISC`, `PSXPORT_DISC`, the same keys in the
gitignored `.env`, or one `*.chd` drop-in at the repository root, in that order. A configured missing
path and multiple drop-ins are refused. The tool extracts into `scratch/bin/crash1/` only after
`SYSTEM.CNF` boots `SCUS_949.00` and all 11 tracked executable facts match.

Run `ctest --test-dir scratch/build-clang --output-on-failure -R
'^(crash_cpp_policy|crash1_provision_tests)$'` for the normal C++ policy and provisioning both-answer
tests.

## Cross-check the first oracle window

The asset-gated target re-provisions the executable, runs the independent CPU's permanent 39-case
fixture, then compares symbolic crt0 decoding with execution at the first real call boundary:

```sh
PSXPORT_CRASH1_DISC=/path/to/Crash-Bandicoot-USA.chd \
  cmake --build scratch/build-clang --target crash1_oracle_boot_check
```

This verifies the crt0 call boundary only. It does not claim a full oracle boot or a runnable PC port.

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
text at `0xBFC00180`; BIOS, hardware, and later boot are still unverified. The emitter's 115 static
seeds and 653 emitted candidates are discovery denominators, not claims that every candidate is
executable code.

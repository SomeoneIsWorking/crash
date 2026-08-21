# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

Current status: framework scaffold plus a verified Crash 1 North American disc/executable identity
and 653 emitted static candidates. The generated path agrees with the independent oracle on all 34
CPU-state fields at the first four executed calls. Reaching call four also proves that the third
target executed and returned through its measured caller continuation. Five addresses in the
candidate set have execution provenance so far. There is still no runnable port, full oracle boot,
game seam, native producer, widescreen path, or interpolation path.

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
generated code. The boundary runner is the repository's one compile-backed first-party translation
unit.

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

## Cross-check the first four recompiled boundaries

Once the verified executable exists under `scratch/bin/crash1/`, one asset-gated target emits the
resident substrate, builds its focused runner, compares the complete port-side register file against
the independent oracle, and proves the comparator's opposite answer:

```sh
cmake --build scratch/build-clang --target crash1_recomp_boundary_check -j16
```

The positive path compares `pc`, all 31 nonzero general-purpose registers, `lo`, and `hi` at calls
`0x80011A18`, `0x80011D88`, `0x8003E0C0`, and `0x8001652C` (34/34 each). The fourth boundary occurs
only after `0x8003E0C0` returns to `0x80011D98`, so the slice includes that leaf's generated body and
return path. All reference states come from psxport's canonical ordinal call capture; Crash does not
duplicate the oracle's MIPS call-tracking rules. The controls require the real emitter to refuse an
out-of-text seed and the same comparator to name a deliberately altered fourth-boundary `a0`; a real
oracle trace capped immediately before call four must report only `3/4` boundaries and refuse.
Generated code remains gitignored and is never edited. This establishes only the entry through call
four; later resident code, BIOS and hardware are still unverified. The emitter's 115 static seeds and
653 emitted candidates are discovery denominators, not claims that every candidate is executable
code.

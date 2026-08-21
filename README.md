# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

Current status: framework scaffold plus a verified Crash 1 North American disc/executable identity
and an independently executed, 6/6 cross-checked crt0 call boundary. There is still no generated
code, game seam, runnable port, full oracle boot, native producer, widescreen path, or interpolation
path.

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
generated code. The scaffold currently reports an explicit zero first-party translation units.

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

The asset-gated target re-provisions the executable, runs the independent CPU's permanent 22-case
fixture, then compares symbolic crt0 decoding with execution at the first real call boundary:

```sh
PSXPORT_CRASH1_DISC=/path/to/Crash-Bandicoot-USA.chd \
  cmake --build scratch/build-clang --target crash1_oracle_boot_check
```

This verifies the crt0 call boundary only. It does not claim a full oracle boot or a runnable PC port.

# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

Current status: framework scaffold plus a verified Crash 1 North American executable identity. There
is still no provisioned disc, generated code, game seam, runnable port, native producer, widescreen
path, or interpolation path.

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

Disc images and extracted executables are never committed. Provision them through a gitignored
`.env` or a drop-in file in the repository root once each title's region and executable are measured.

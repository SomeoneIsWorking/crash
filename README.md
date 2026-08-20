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

Disc images and extracted executables are never committed. Provision them through a gitignored
`.env` or a drop-in file in the repository root once each title's region and executable are measured.

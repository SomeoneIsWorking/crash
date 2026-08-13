# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

Current status: project scaffold only. No game executable, generated code, native producer,
widescreen path, or interpolation path is claimed yet.

## Configure the framework scaffold

```sh
git submodule update --init external/psxport
external/psxport/scripts/sync-submodules.sh
cmake -S . -B build
cmake --build build --target crash_scaffold
```

Disc images and extracted executables are never committed. Provision them through a gitignored
`.env` or a drop-in file in the repository root once each title's region and executable are measured.

# Crash trilogy port

This repository ports Crash Bandicoot 1, 2, and 3 on one shared engine lineage. Shared engine code
belongs in `game/`; title-specific seams, executable facts, and assets belong in
`titles/crash1/`, `titles/crash2/`, and `titles/crash3/`.

Read `external/psxport/CLAUDE.md` and `external/psxport/docs/workspace/PROTOCOL.md` before work.
Generated code is sacrosanct. Never commit discs, extracted executables, `generated/`, `.env`, or
machine-specific paths. Run artifacts go under `scratch/`, never `/tmp`.

All picture work is RE-driven. Widescreen and interpolation require PC-native graphics producers
reading game state; do not reconstruct pictures from GTE/OT/GP0 output. Each title must first reach a
faithful, measurable base before enhancements.

---
id: C018
kind: claim
status: holds
created: 2026-08-24
tags: launcher,product
depends: CMakeLists.txt
reconfirmed: 2026-08-24
verified_at: 2026-08-24 23:24:12
---

## Claim

Crash's zero-argument launcher selects the real crash1_port product through one frozen-uv interpreter; provisioning and generation use that interpreter, missing native dependencies name actionable install commands, and no launcher path invokes CTest, scaffold, oracle, or boundary diagnostics.

## Evidence

uv run --frozen python -B tests/test_run.py passed 7/7 hermetic positive/refusal cases on 2026-08-24; CTest crash_launcher_tests passed in the Clang product build.

## What would falsify it

if a real launcher command trace escapes the uv interpreter, invokes a test/diagnostic instead of crash1_port, silently proceeds without a required native dependency, or zero arguments cease to select crash1_port

## Re-confirmed 2026-08-24

Final frozen-uv launcher tests passed 7/7 directly and in authoritative Clang CTest; the normal verifier propagated the .venv interpreter to CMake and passed the clean psxport 9c2e3f1c pin check. Product/run.sh remained unrun as required.

## Re-confirmed 2026-08-24

Final frozen-uv launcher tests passed 7/7 directly and under authoritative Clang CTest; CMakeLists registers the same locked-interpreter test and the normal verifier passed recorded psxport 9c2e3f1c. Product/run.sh remained unrun.

## Re-confirmed 2026-08-24

The shipping configure command now explicitly selects the isolated `scratch/build-player` tree with `BUILD_TESTING=OFF`; CMake omits its test executables and registrations in that configuration. The hermetic launcher test proves both configure passes carry the option and still build only discdump plus crash1_port, with no CTest command.

## Re-confirmed 2026-08-24

At bb3a497, launcher tests passed 7/7 and assert both player configure passes include BUILD_TESTING=OFF, only discdump/crash1_port are built, and no CTest/diagnostic command executes. The real frozen prepare route correctly refused only because no disc asset was configured.

---
id: I008
kind: instrument
status: trusted
created: 2026-08-26
---

## Instrument

Clean psxport `99a42aa3` `oracle_trace` running verified `SCUS_949.00` with ordered
`--model-syscall-return 1:1`, `--model-bios-return B:0x56:0x8000F800`, and
`--model-main-ram-word 0x8000F818:VALUE` in one process-isolated Mednafen CPU.

## Validated by

With value `0x00000C80`, it applies the syscall model at step 62,083, B(56h) at step 62,088, and
captures local wrapper `0x8004323C` at step 62,165 with `v0=0x00000CB8`. The same real executable with
the deliberate opposite `0x00000C84` reaches the same wrapper and step with `v0=0x00000CBC`. The
four-byte endpoint shift proves the seed crosses the production memory seam and affects the real copy
loop; uniform output would have distrusted the instrument.

## Known failure modes

The current tracer stops at the first linked call after the models. Crash's first such call is local
wrapper `0x8004323C`, whose non-link `jr` tail-dispatches A(44h); therefore this instrument cannot claim
A(44h) state. It also logs the explicit seed and registers but does not dump the fourteen destination
words. Issue #8 owns exact post-model target capture and the subsequent RAM comparison.

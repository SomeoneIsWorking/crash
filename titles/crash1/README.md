# Crash Bandicoot

The selected target is the North American NTSC-U release, serial `SCUS-94900`, executable
`SCUS_949.00`. `executable.json` records the identity and PS-X EXE header measured from the real image;
`tools/verify_executable.py` compares that tracked record to the bytes.

`recomp_seeds.json` deliberately adds no guessed addresses: psxport roots discovery in these real
executable bytes. The current asset-gated gate expands 115 static candidate seeds into 666 emitted
candidates and proves the generated path agrees with the independent oracle on all 34 CPU-state
fields at each of the first eight executed calls, from `0x80011A18` through `0x8003E1F8`. Every
target is recovered from execution, not entered in the seed file.

This is an entry-through-first-BIOS-dispatch recompile boundary, not a playable port. The eighth target is
the measured `EnterCriticalSection` wrapper (`addiu $a0, 1; syscall 0`). The controlled port boundary
executes that generated wrapper and verifies the shipping HLE returns prior IRQ state `1`, disables
IRQ delivery, and rejects a different execution-proven function as the wrapper. The independent CPU
records Cause `0x20` and EPC `0x8003E1FC`; both CPUs pop the Status mode stack once, resume at
`0x8003E200`, and agree 34/34 at BIOS dispatch PC `0xB0`. Its `t1=0x56` and `ra=0x800431B8` identify
B(56h). The 18/18 gate also compares those three manifest-backed shipping facts to the independent
CPU and rejects an altered selector. The differential proof still stops before the B0 HLE call.

Retail disassembly at `0x8004319C..0x80043248` proves the next dependency: B(56h) returns a C0-table
pointer, Crash reads its `+0x18` slot, copies fourteen words from
`[0x80043204,0x8004323C)`, and tail-dispatches A(44h) with
`ra=0x800431E8`. The product composition now permits B(56h) through shipping HLE and stops at that
A(44h) pre-HLE boundary; its focused Clang build and 14/14 hermetic composition checks pass. No
product process was launched, so runtime reach and the post-B(56h) state remain unverified.

The 666-candidate emission count is not a code-correctness denominator: only the entry and eight
call targets have execution provenance so far. The boundary runner installs the direct, title-owned
`Crash1Runtime`; that runtime deliberately has no legacy configuration view or invented host behavior.
PSX-SPX identifies the consumed C(06h) ExceptionHandler slot as writable retail address `0x00000C80`.
Clean psxport `99a42aa3` publishes that value, and `bios_contract.json` plus the shipping-HLE consumer
CTest preserve Crash's side without a title-local override. The real ordered oracle applies the
selector-1 syscall return, B(56h), and C0 seed in one CPU and reaches `v0=0x00000CB8`, proving the
fourteen-word copy completed. It then captures local wrapper `0x8004323C`; exact target capture cannot
yet continue through that wrapper's non-link A(44h) tail dispatch. Later resident code, BIOS,
hardware, overlays, and gameplay remain unverified.

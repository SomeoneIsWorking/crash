# Crash

PC-native ports of the PlayStation Crash Bandicoot trilogy, built on
[psxport](https://github.com/SomeoneIsWorking/psxport).

The repository is one engine lineage with title-specific integration under `titles/`:

- `titles/crash1/`
- `titles/crash2/`
- `titles/crash3/`

The shipping contract covers all three games. Crash 2 and Crash 3 are not optional follow-ups, but
their presence in this repository must not be read as implementation status; `docs/project-state.md`
records factual capability coverage and `docs/re-frontier.md` the measured binary frontier.

Current status: a current-frontier Crash 1 product plus verified North American disc/executable identities for Crash 1
(`SCUS-94900`), Crash 2 (`SCUS-94154`), and Crash 3 (`SCUS-94244`). Crash 2 and Crash 3 each have a
symbolic crt0 decode agreeing 6/6 with the independent CPU oracle at the first real call, and each
derived runtime's typed executable/frame-contract facts agree 20/20 with its retail bytes and manifest.
The three distinct VSync bodies are fingerprinted from the real executables and bound only to
psxport's fatal native-loop trap; every title creates a host-owned driver, but all three currently
refuse frame stepping because no frame boundary is proven. Crash 3's `SYSTEM.CNF`
selection is explicit because its disc also contains the unrelated bootable-looking
`DRAGON/SPYRO.EXE`. The last landed clean Clang/CTest/real-data gates and all title regressions use
recorded psxport `8611d756`; the fatal-VSync contract is verified against the current shared
framework integration batch and still requires the operator's clean framework landing and pin bump.
Against current psxport `54af32cb`, Crash 1 has 666 emitted static candidates. Its generated path agrees with the independent oracle on all 34
CPU-state fields at the first eight executed calls. Nine addresses in the candidate set have
execution provenance: eight generated bodies execute before the eighth observed target. The eighth
target is the real `addiu $a0, 1; syscall 0` wrapper for `EnterCriticalSection`; the controlled port
boundary executes that generated wrapper and proves the shipping HLE returns the prior IRQ state and
disables delivery. The independent and shipping CPUs also agree on syscall Cause/EPC, resume at EPC+4,
and the following 34/34 BIOS B dispatch state; Crash 1 reaches B(56h), with `t1=0x56` and
`ra=0x800431B8`. Crash 3 independently emits 986 candidates from `SCUS_942.44`; its first eight
calls also agree 34/34, including tracked game main `0x80048AA0` and its own syscall wrapper
`0x80048C38`, then agrees 34/34 at its first post-return B0 dispatch. Retail Crash 1 disassembly proves
that B(56h) is followed by a C0 slot `+0x18` read, a fourteen-word copy, and A(44h) with
`ra=0x800431E8`. The real `crash1_port` product is now composed to permit B(56h) through shipping HLE
and stop at that A(44h) pre-HLE boundary. A focused Clang build and 14/14 hermetic composition checks
prove the shipping wiring; a serialized product execution is still required before claiming runtime
reach. PSX-SPX grounds the consumed C0 slot 6 as C(06h) ExceptionHandler at `0x00000C80`; clean
psxport `99a42aa3` and the Crash consumer CTest satisfy that contract. The real ordered oracle applies
the syscall and B(56h) returns, seeds the slot, and completes the fourteen-word copy, but stops at local
wrapper `0x8004323C` before its non-link A(44h) tail dispatch. Issue #8 owns exact post-model target
capture. There is still no independently compared A(44h) state, rendered frame, native producer,
widescreen path, interpolation path, or Crash 2/3 product.

## Run the current product

Install `uv`, CMake, Git, pkg-config, SDL3 development files, and a compatible C/C++ compiler. Then
provide the North American Crash Bandicoot disc either as the optional argument or through
`PSXPORT_CRASH1_DISC`, `PSXPORT_DISC`, the equivalent gitignored `.env` entries, or one repository-root
CHD:

```sh
./run.sh /path/to/Crash-Bandicoot-USA.chd
```

With a configured disc, zero arguments are the normal path:

```sh
./run.sh
```

The slim shell entry uses `uv run --frozen`; the locked interpreter performs psxport synchronization,
provisioning, substrate emission, CMake configuration, and product build before launching only
`scratch/bin/crash1_port`. Its isolated player tree sets `BUILD_TESTING=OFF`; it does not configure or
run CTest, `crash_scaffold`, or any diagnostic. To validate the
fresh-clone provisioning/build contract without starting the product, use `./run.sh --prepare-only`.
Missing native dependencies are refused with the exact Homebrew, APT, DNF, or supported Windows
install command. Caller-selected `CC`/`CXX` values pass through; the launcher prefers Clang when it is
available but does not whitelist or blacklist compiler identities.

The current product is composed to execute the retail program through the measured `EnterCriticalSection`
transition, allow B(56h) through shipping HLE, and stop before the following A(44h) HLE mutation. The
shipping/oracle boundary harness proves only the pre-B(56h) state 34/34; serialized product execution and
post-B(56h) equality remain outstanding. It is not playable and does not render a window or frame. Crash
2 and Crash 3 remain measured integrations without player products.

## Verify the project

```sh
python3 tools/psxport_sync.py --auto
python3 tools/verify.py
```

The verifier always configures and builds the authoritative `scratch/build-clang/` tree with Clang
before running its complete CTest graph and framework pin check. This ordering is required: invoking
`ctest` directly does not reconfigure CMake and can run a stale test graph after targets change. Raw
`ctest --test-dir scratch/build-clang` is only a focused rerun after `tools/verify.py` has refreshed
the tree. The shared framework checker applies this repository's tracked `clang-format` and
`clang-tidy` policy and the 1,200-line ownership cap without linting `external/psxport` or generated
code. Clang is the authoritative verification choice; top-level CMake does not reject another
compatible compiler selected for a player build.

## Provision a measured title

Disc images and extracted executables are never committed. After configuring the Clang build above,
provision the USA disc; the tool builds psxport's disc reader target when needed:

```sh
python3 tools/provision_title.py --title crash1 /path/to/Crash-Bandicoot-USA.chd
python3 tools/provision_title.py --title crash2 /path/to/Crash-Bandicoot-2-USA.chd
python3 tools/provision_title.py --title crash3 /path/to/Crash-Bandicoot-Warped-USA.chd
```

The disc argument may instead come from the title key (`PSXPORT_CRASH1_DISC` or
`PSXPORT_CRASH2_DISC` or `PSXPORT_CRASH3_DISC`), `PSXPORT_DISC`, the same keys in the gitignored
`.env`, or one `*.chd` drop-in at the repository root, in that order. A configured missing path and
multiple drop-ins are refused. The shared implementation publishes into the matching
`scratch/bin/<title>/` only after `SYSTEM.CNF` selects that title's serial-coded executable and all 12
tracked executable and VSync-body facts match. For Crash 3 this deliberately ignores the disc's bundled
`DRAGON/SPYRO.EXE`; presence is not selection.

After `tools/verify.py` refreshes the authoritative tree, a focused policy/provisioning rerun is:

```sh
ctest --test-dir scratch/build-clang --output-on-failure \
  -R '^(crash_cpp_policy|crash_title_provision_tests)$'
```

## Cross-check the first oracle window

The asset-gated target re-provisions the executable, runs the independent CPU's permanent 39-case
fixture, then compares symbolic crt0 decoding with execution at the first real call boundary:

```sh
PSXPORT_CRASH1_DISC=/path/to/Crash-Bandicoot-USA.chd \
  cmake --build scratch/build-clang --target crash1_oracle_boot_check

PSXPORT_CRASH2_DISC=/path/to/Crash-Bandicoot-2-USA.chd \
  cmake --build scratch/build-clang --target crash2_oracle_boot_check

PSXPORT_CRASH3_DISC=/path/to/Crash-Bandicoot-Warped-USA.chd \
  cmake --build scratch/build-clang --target crash3_oracle_boot_check
```

Each target verifies only that title's crt0 call boundary. Crash 2 and Crash 3 also compare all 20
typed runtime image and native-frame-contract facts against the executable and prove an altered fact
disagrees. Neither target
claims a full oracle boot. CMake reserves independent `generated/<title>/` namespaces. These oracle
targets are evidence tools and are never part of the player launch route.

## Cross-check the resident calls before the first BIOS boundary

Once the verified executable exists under its serial-scoped cache, an asset-gated target emits the
resident substrate, builds its focused runner, compares the complete port-side register file against
the independent oracle, and proves the comparator's opposite answer:

```sh
cmake --build scratch/build-clang --target crash1_recomp_boundary_check -j16
cmake --build scratch/build-clang --target crash2_recomp_boundary_check -j16
cmake --build scratch/build-clang --target crash3_recomp_boundary_check -j16
```

The positive path compares `pc`, all 31 nonzero general-purpose registers, `lo`, and `hi` at calls
`0x80011A18`, `0x80011D88`, `0x8003E0C0`, `0x8001652C`, `0x8003F224`, `0x80042B1C`, `0x8004319C`,
and `0x8003E1F8` (34/34 each). All reference states come from psxport's canonical ordinal call
capture; Crash does not duplicate the oracle's MIPS call-tracking rules. The controls require the
real emitter to refuse an out-of-text seed and the same comparator to name a deliberately altered
eighth-boundary `a0`; a real oracle trace capped immediately before call eight must report only
`7/8` boundaries and refuse. Generated code remains gitignored and is never edited. This establishes
the resident entry path through the syscall and its first post-return dispatch. The gate executes the
measured generated wrapper, checks `EnterCriticalSection` changes IRQ state `1 -> 0` and returns `1`,
validates Cause `0x20` and the title-specific EPC against the independent CPU, pops the Status mode
stack once, resumes at EPC+4, and compares the next dynamic dispatch 34/34. Crash 1 reaches PC `0xB0`
with `t1=0x56` and `ra=0x800431B8`, identifying BIOS B(56h). A different syscall target and wrong
selector both refuse before resume. The manifest-backed B(56h) PC, function, and return address agree
with the independent CPU, and an altered function refuses. Executing the B0 HLE, hardware, and later
boot remain unverified. The emitter's 115 static seeds and 666 emitted candidates are discovery
denominators, not claims that every candidate is executable code.

The Crash 3 target uses only `SCUS-94244` artifacts and `titles/crash3/recomp_seeds.json`. Its eight
call targets are `0x800112B8`, `0x80011628`, game main `0x80048AA0`, `0x800154AC`, `0x8004BFBC`,
`0x8004F37C`, `0x8004F914`, and `0x80048C38`; each agrees 34/34. At step 75,963 the independent CPU
enters `0xBFC00180` after the last target's syscall; Cause/EPC/resume and the following B0 dispatch now
agree 34/34. No executed B0 HLE or rendered-frame claim follows from this gate.

The Crash 2 target likewise uses only `SCUS-94154` artifacts and `titles/crash2/recomp_seeds.json`.
Its eight call targets are `0x8001144C`, `0x800117BC`, game main `0x80049BD4`, `0x80015614`,
`0x8004B1B8`, `0x8004EC30`, `0x8004F1F8`, and `0x80049D1C`; each agrees 34/34. Each title manifest
tracks its own game-main and first-syscall address plus call ordinal, and the 16-case gate mutates the
tracked syscall frontier to prove a named disagreement. This shared harness structure is not a Crash 2
or Crash 3 player implementation.

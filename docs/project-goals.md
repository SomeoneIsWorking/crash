# Project goals

Stable product intent for the Crash engine-lineage repository. Factual coverage belongs in
`docs/project-state.md`; atomic work belongs in `docs/issues/`.

## G001 — playable faithful Crash trilogy ports

Deliver native PC products for the North American releases of Crash Bandicoot, Crash Bandicoot 2,
and Crash Bandicoot 3 that execute each verified retail program through owned host integration.

Why it matters: the repository represents one engine lineage, so a Crash 1-only executable is not the
trilogy product.

Success conditions:

- each title has a serial-verified, asset-safe provisioning path and shipping executable;
- each product reaches sustained gameplay with working presentation, input, audio, saves, and timing;
- `run.sh` launches the current intended product from a fresh clone with documented native dependencies,
  `uv`, and user-supplied game data.

Constraints and non-goals:

- generated recompiler output is derived from user-supplied executables and is never hand-edited or
  committed;
- title addresses and behavior remain title-owned until direct cross-title evidence proves a shared
  engine owner;
- CTR and Crash Bash are separate engines and are not part of this repository's product.

Related state: S001, S002, S003, S004, S008, S009.

## G002 — owned widescreen, interpolation, and native rendering

Provide widescreen, presentation-time interpolation, and game-owned native rendering for all three
Crash titles on top of their faithful execution bases.

Why it matters: these enhancements require semantic access to camera, simulation, transform, and
graphics-submission state; post-projection reconstruction cannot deliver the intended fidelity.

Success conditions:

- each title widens view geometry through an owned camera/projection producer;
- authoritative previous/current simulation and transform state feeds presentation-time interpolation;
- game-owned producers submit native primitives without using GTE/OT/GP0 output as the source of truth;
- enhancement modes remain disabled during oracle comparison and have independent visual/runtime gates.

Constraints and non-goals:

- enhancements never substitute for an unimplemented faithful boot or gameplay stage;
- shared implementation is limited to behavior proven common across Crash 1, 2, and 3;
- native rendering is not required for non-Crash projects merely because this repository owns it.

Related state: S005, S006, S007.

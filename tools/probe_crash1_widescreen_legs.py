#!/usr/bin/env python3
"""Run Crash 1's widescreen in matched 4:3 and 16:9 legs and report the GUEST's own projection state.

WHY A TOOL AND NOT TWO COMMANDS. `PSXPORT_PRESENT_SINK` is process-wide, so a wide leg and a 4:3 leg
must each be captured in their OWN process; run them in one process and both legs come out the same
width and the comparison is not a measurement. `ASPECT_AUTO` (aspect=3) resolves against the SINK's
aspect, so headless it silently means 4:3 — which is why a wide run is proved by the guest's own
published projection words and by `render_width > native_width`, NEVER by a config value. This tool
writes the per-leg settings itself so the aspect cannot be left at its default, sizes the sink from
the measured native extent, and quotes the log TAIL of each leg, because `picture_announce` prints
only on CHANGE and a mid-run sample is not the steady state.

It never runs a product the caller did not start: this launches the product itself, refuses to start
while another instance holds the machine's single slot, and kills only the PIDs it captured.

THE DISC IS NOT OPTIONAL, AND A LEG WITHOUT ONE IS NOT A PICTURE MEASUREMENT. The previous revision
of this tool set no disc path at all, so every leg it produced ran with the framework's own "no disc
image ... The CD model will run with NO MEDIA" warning and `CdRead: LBA 16 unreadable ... 0
sector(s) delivered` in the log. That is Crash 1 without its own data, and the "no frame at all"
number those legs produced was a measurement of THIS TOOL, not of the title. This revision takes
`--disc`, passes it as `PSXPORT_CRASH1_DISC` (the per-title key the framework's DiscConfig reads),
and REFUSES to print a picture verdict for a leg whose log shows media was absent, so the same
mistake cannot be published a second time as a title result.

    tools/probe_crash1_widescreen_legs.py --disc "$DISC" --frames 400
    tools/probe_crash1_widescreen_legs.py --disc "$DISC" --frames 400 --bin build/agent-clang/crash1_port

Exit 0 means both legs ran WITH media and the guest-state evidence is present on both. 1 means a leg
did not reach its publication site, which is a finding to report rather than something to paper over.
2 means no leg could be started at all.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

# The measured native extent. NOT a framework default and NOT a literal in the owner: it is what the
# title's own GP1(0xC0) display publication produced on the real boot, and the 4:3 leg's
# `render_width` line is what has to agree with it.
NATIVE_WIDTH = 320
NATIVE_HEIGHT = 240
SINK_HEIGHT = 240

ASPECT_4X3 = 0
ASPECT_16X9 = 1

RUNNING_PRODUCT = re.compile(r"^.*/([A-Za-z0-9_.]*_port)$")


def other_product_running() -> str | None:
    """A running product, matched on the EXECUTABLE the process is running.

    Matching anywhere in the command line is wrong: a shell that merely mentions `megamanx4_port` in
    one of its arguments is not a running product, and refusing on it would report a busy machine
    that is idle.
    """
    for line in subprocess.run(["ps", "-eo", "pid,args"], capture_output=True, text=True,
                               check=False).stdout.splitlines()[1:]:
        pid, _, args = line.strip().partition(" ")
        executable = args.split(" ", 1)[0]
        if RUNNING_PRODUCT.match(executable) and "crash1_port" not in executable:
            return line.strip()
    return None


def sink_width(aspect: int) -> int:
    return NATIVE_WIDTH if aspect == ASPECT_4X3 else 428


def write_settings(path: pathlib.Path, aspect: int) -> None:
    path.write_text(f"aspect={aspect}\n", encoding="utf-8")


# The framework's own wording when it found no media, and the CD symptom that follows it. A leg whose
# log contains either of these ran Crash 1 without its data, so nothing it says about a picture is a
# statement about the title. These are the strings the refusal matches, quoted from a real
# media-less run (scratch/wide/leg_4x3.log) rather than guessed.
NO_MEDIA_MARKERS = (
    "The CD model will run with NO MEDIA",
    "CdRead: LBA 16 unreadable",
)


def media_present(log: pathlib.Path) -> tuple[bool, str]:
    """Whether the leg had media, and the marker that says it did not.

    A refusal here rather than a warning, because the previous revision of this tool reported a
    picture verdict for legs whose own log said the CD model had no media.
    """
    text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
    for marker in NO_MEDIA_MARKERS:
        if marker in text:
            return False, marker
    return True, ""


def run_leg(
    binary: pathlib.Path, workdir: pathlib.Path, aspect: int, frames: int, disc: str | None
) -> tuple[int, pathlib.Path]:
    tag = "4x3" if aspect == ASPECT_4X3 else "16x9"
    settings = workdir / f"settings_{tag}.ini"
    write_settings(settings, aspect)
    environment = dict(os.environ)
    environment.update(
        PSXPORT_VK_HEADLESS="1",
        PSXPORT_NOAUDIO="1",
        PSXPORT_NOPACE="1",
        PSXPORT_NATIVE_FRAMES=str(frames),
        PSXPORT_PRESENT_SINK=f"{sink_width(aspect)}x{SINK_HEIGHT}",
        PSXPORT_SETTINGS=str(settings),
        SDL_VIDEODRIVER="offscreen",
        SDL_AUDIODRIVER="dummy",
    )
    # The per-title key wins over PSXPORT_DISC in `DiscConfig::resolve_disc_path`, and it is the one
    # `GameConfig::discEnvVar` names for this title, so a workspace that provisions one disc per
    # title cannot have another title's media leak into this leg.
    if disc:
        environment["PSXPORT_CRASH1_DISC"] = disc
    # The `bios` debug channel is the only place the framework records WHICH call a guest made, from
    # where, and with which arguments -- the info-level "unimplemented BIOS A0:0x27" names the vector
    # and the function but not the call site, and the call site is what identifies what the guest
    # wanted. Kept opt-in via the caller's environment so the default run is not spammed.
    if os.environ.get("CRASH1_BIOS_TRACE"):
        environment["PSXPORT_DEBUG"] = "bios"
    log = workdir / f"leg_{tag}.log"
    with log.open("w", encoding="utf-8") as stream:
        completed = subprocess.run([str(binary)], cwd=ROOT, env=environment, stdout=stream,
                                   stderr=subprocess.STDOUT, check=False, timeout=900)
    return completed.returncode, log


def report(log: pathlib.Path) -> dict[str, str]:
    text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
    found: dict[str, str] = {}
    for line in text.splitlines():
        if "[crash1-frame:error]" in line or "lightrec execution fault" in line.lower():
            found["stop"] = line.split("]", 1)[-1].strip() if "]" in line else line.strip()
        if "[crash1-wide] guest projection init published" in line:
            found["init"] = line.split("[crash1-wide]", 1)[1].strip()
        elif "[crash1-wide] guest centre" in line:
            found["centre"] = line.split("[crash1-wide]", 1)[1].strip()
        elif "[wide]" in line and "native picture" in line:
            found["wide"] = line.split("[wide]", 1)[1].strip()
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin", type=pathlib.Path, default=ROOT / "build/agent-clang/crash1_port")
    parser.add_argument("--frames", type=int, default=400)
    parser.add_argument(
        "--disc",
        help="the user's Crash Bandicoot USA CHD. A leg without one is Crash 1 with no data, and "
        "this tool refuses to report a picture verdict for it rather than publishing a number "
        "measured against an empty CD",
    )
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "scratch/wide")
    args = parser.parse_args()

    binary = args.bin if args.bin.is_absolute() else ROOT / args.bin
    if not binary.is_file():
        print(f"probe_crash1_widescreen_legs: no product at {binary}; build it first", file=sys.stderr)
        return 2
    holder = other_product_running()
    if holder:
        print(f"probe_crash1_widescreen_legs: another product holds the machine's single slot, and this "
              f"tool never starts a second one or kills one it did not start:\n  {holder}",
              file=sys.stderr)
        return 2

    args.out.mkdir(parents=True, exist_ok=True)
    results: dict[str, dict[str, str]] = {}
    codes: dict[str, int] = {}
    for aspect in (ASPECT_4X3, ASPECT_16X9):
        tag = "4x3" if aspect == ASPECT_4X3 else "16x9"
        code, log = run_leg(binary, args.out, aspect, args.frames, args.disc)
        codes[tag] = code
        results[tag] = report(log)
        had_media, marker = media_present(log)
        print(f"== {tag}: sink {sink_width(aspect)}x{SINK_HEIGHT}, exit {code}, log {log}")
        print(f"   media: {'present' if had_media else f'ABSENT ({marker})'}")
        for key in ("stop", "init", "centre", "wide"):
            if key in results[tag]:
                print(f"   {key}: {results[tag][key]}")
        # The log TAIL, because `picture_announce` prints only on change and a mid-run sample is not
        # the steady state. Printed for both legs so a reader can see where each one stopped.
        tail = log.read_text(encoding="utf-8", errors="replace").splitlines()[-6:] if log.is_file() else []
        print("   --- log tail ---")
        for line in tail:
            print(f"   | {line}")

    ok = True
    for tag in ("4x3", "16x9"):
        had_media, marker = media_present(args.out / f"leg_{tag}.log")
        if not had_media:
            print(
                f"FAIL: the {tag} leg ran with NO MEDIA (the log says {marker!r}), so nothing it "
                "reports about a picture is a statement about Crash 1. Pass --disc <the user's CHD>.",
                file=sys.stderr,
            )
            ok = False
        if "stop" in results[tag]:
            print(
                f"FAIL: the {tag} leg left guest execution early: {results[tag]['stop']}",
                file=sys.stderr,
            )
            ok = False
        if "init" not in results[tag]:
            print(f"FAIL: the {tag} leg never reached the guest's own projection publication "
                  f"(exit {codes[tag]})", file=sys.stderr)
            ok = False
    if "centre" not in results["16x9"]:
        print("NOTE: the 16:9 leg published the retail centre but never the widened one, so the "
              "product stopped before its per-frame SetGeomOffset at 0x80017F00. That is a recorded "
              "gap in the run, not a widening that failed: the owner is pinned by "
              "tests/crash1_widescreen.cpp, which drives the recovered leaf directly.", file=sys.stderr)
        ok = False
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

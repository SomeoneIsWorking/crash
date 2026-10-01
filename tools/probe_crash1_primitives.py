#!/usr/bin/env python3
"""Measure what the Crash 1 guest ACTUALLY submits and whether a frame is ACTUALLY presented.

WHY THIS TOOL EXISTS, and what it falsifies. `docs/issues/0020` and `docs/project-state.md` S011
record the frontier as "the guest reaches its display wait and submits no primitives:
`[producers] run-end: OtAttr spans recorded 0 (overflow 0)`", and "Zero frames are presented". **Both
halves of that were read off one counter, and that counter is a dead tap for this title.**

`OtAttr` counts a guest store only when it lands inside a configured packet-pool window, and that window
comes from the LEGACY `GameConfig::packetPoolBase/Stride` or the live `packetPoolBasePtrs/EndPtrs`
(`runtime/psx/ot_attr.cpp` `pool_range_uncached`). Crash 1 is a DIRECT runtime — it implements
`GameRuntime` and never fills a legacy `GameConfig` — so `c->cfg` is null, the window is empty, and the
span count is ZERO BY CONSTRUCTION whatever the guest draws. The framework even has a warning for
exactly this case ("packet-pool attribution is STRUCTURALLY BLIND here, so an empty span table means
'not measured', NOT 'the guest submitted nothing'"), and it is UNREACHABLE for a null-config title:
`poolRangeMiss` runs only under `if (c->cfg != mPoolCfg)`, and `mPoolCfg` is initialised to `nullptr`, so
`nullptr != nullptr` is false and the resolution — with its warning — never happens. That is the
workspace's recurring instrument defect: a metric that reads a tap nothing writes returns a confident
answer about the wrong subject, and the zero it returns is the most believable possible output.

So this tool does NOT report the span count. It runs the same product, in the same process, in the same
run that prints the span line, and measures two INDEPENDENT things with denominators:

  A) the guest's own GP0 traffic, from `PSXPORT_PRIMDUMP` — every primitive the framework's rasterizer
     was offered, per frame, with the frame count as the denominator. A missing CSV is a FAILURE, never
     a zero: `primdump` writes no file when it saw nothing, so "no file" and "no prims" are different
     answers and only one of them is a picture of the guest.
  B) the presented frames, from `PSXPORT_DEBUG=fps60dump` + `PSXPORT_FPS60_DUMP_FROM` — one PNG per
     `FramePresenter::commit`, decoded here to a non-black pixel fraction, so "a frame was presented" and
     "the presented frame has a picture in it" are two numbers rather than one assertion.

The two live in ONE run on purpose. A span count of 0 next to a prim count of 0 would be consistent
with a game that draws nothing; a span count of 0 next to a prim count in the hundreds, in the same
process, is a measurement of the counter rather than of the game.

THE DISC IS NOT OPTIONAL, for the reason `probe_crash1_widescreen_legs.py` already established: a leg
with no media is Crash 1 without its data, and this tool REFUSES to print a picture verdict for one,
matching the framework's own no-media wording rather than guessing at it.

    tools/probe_crash1_primitives.py --disc "$DISC" --frames 400
    tools/probe_crash1_primitives.py --disc "$DISC" --frames 400 --bin build/agent-clang/crash1_port

It never runs ./run.sh, never starts a second product while one holds the machine's single slot, and
kills only PIDs it captured itself. Run artifacts go to one `--out` directory that it overwrites.

Exit 0 = media present, the guest submitted prims, and at least one presented frame carries a picture.
1 = a measurement was missing or a refusal fired. 2 = no leg could be started.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import struct
import subprocess
import sys
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[1]

# The framework's own wording when it found no media, quoted from a real media-less run
# (scratch/wide/leg_4x3.log). Matched, not guessed: a leg carrying either of these ran Crash 1
# without its data, so nothing it says about a picture is a statement about the title.
NO_MEDIA_MARKERS = (
    "The CD model will run with NO MEDIA",
    "CdRead: LBA 16 unreadable",
)

RUNNING_PRODUCT = re.compile(r"^.*/([A-Za-z0-9_.]*_port)$")

# The run-end telemetry, as one pattern so a line that changes shape is a REFUSAL and not a silent
# zero. `fallback_blocks` and `fallback_instructions` are the denominators for "Lightrec ran this".
TELEMETRY = re.compile(
    r"Lightrec run-end: translated_blocks=(\d+) executed_blocks=(\d+)"
    r" executed_instructions=(\d+) fallback_blocks=(\d+) fallback_instructions=(\d+)"
)
SPAN_LINE = re.compile(r"OtAttr spans recorded (\d+) \(overflow (\d+)\)")
PRIMDUMP_LINE = re.compile(r"wrote (\S+) — (\d+) prims over frames (\d+)\.\.(\d+)")
PRIMDUMP_EMPTY = re.compile(r"frames (\d+)\.\.(\d+) passed with ZERO prims offered")


class Refused(Exception):
    """The input cannot support the requested claim."""


# ── B: the presented PNGs ────────────────────────────────────────────────────────────────────────────

def decode_png_nonblack(data: bytes) -> tuple[int, int, float]:
    """(width, height, non-black fraction) of an 8-bit non-interlaced PNG.

    Only the subset the framework's `image_write_rgb24` emits is accepted — 8-bit truecolour with or
    without alpha, no interlacing — and anything else RAISES rather than guessing, because a decoder
    that silently returns zeros for a format it does not understand would report a black frame for a
    frame it never read. That is the same failure this tool exists to correct, one level down.
    """
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise Refused("not a PNG (bad signature)")
    pos = 8
    width = height = 0
    bit_depth = colour_type = interlace = -1
    idat = bytearray()
    try:
        while pos + 8 <= len(data):
            (length,) = struct.unpack_from(">I", data, pos)
            kind = data[pos + 4:pos + 8]
            body = data[pos + 8:pos + 8 + length]
            if len(body) != length:
                raise Refused(f"PNG chunk {kind!r} claims {length} bytes and carries {len(body)}")
            pos += 12 + length
            if kind == b"IHDR":
                width, height, bit_depth, colour_type, _, _, interlace = struct.unpack(">IIBBBBB", body)
            elif kind == b"IDAT":
                idat += body
            elif kind == b"IEND":
                break
    except (struct.error, IndexError) as exc:
        # A truncated capture is not a black capture. Raising `Refused` rather than letting struct
        # raise keeps "the tool could not read this frame" distinct from "this frame is empty".
        raise Refused(f"malformed PNG chunk structure: {exc}") from exc
    if bit_depth < 0:
        raise Refused("PNG carries no IHDR chunk")
    if interlace != 0:
        raise Refused("interlaced PNG, which this decoder does not read")
    if bit_depth != 8 or colour_type not in (2, 6):
        raise Refused(f"PNG colour type {colour_type} at bit depth {bit_depth}, which this decoder "
                      "does not read (only 8-bit RGB/RGBA)")
    channels = 3 if colour_type == 2 else 4
    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    if len(raw) != height * (stride + 1):
        raise Refused(f"PNG payload is {len(raw)} bytes for {height} rows of {stride}+1")
    previous = bytearray(stride)
    nonblack = 0
    for y in range(height):
        base = y * (stride + 1)
        filter_type = raw[base]
        line = bytearray(raw[base + 1:base + 1 + stride])
        if filter_type == 1:      # Sub
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif filter_type == 2:    # Up
            for i in range(stride):
                line[i] = (line[i] + previous[i]) & 0xFF
        elif filter_type == 3:    # Average
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + previous[i]) >> 1)) & 0xFF
        elif filter_type == 4:    # Paeth
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                up = previous[i]
                upleft = previous[i - channels] if i >= channels else 0
                estimate = left + up - upleft
                da, db, dc = abs(estimate - left), abs(estimate - up), abs(estimate - upleft)
                if da <= db and da <= dc:
                    predictor = left
                elif db <= dc:
                    predictor = up
                else:
                    predictor = upleft
                line[i] = (line[i] + predictor) & 0xFF
        elif filter_type != 0:
            raise Refused(f"PNG row filter {filter_type}, which this decoder does not read")
        for x in range(0, stride, channels):
            if line[x] or line[x + 1] or line[x + 2]:
                nonblack += 1
        previous = line
    total = width * height
    return width, height, (nonblack / total if total else 0.0)


# ── A: the guest's GP0 traffic ───────────────────────────────────────────────────────────────────────

def census_csv(path: pathlib.Path) -> dict[str, object]:
    """Per-frame primitive counts and the command-byte census, straight from the primdump CSV.

    The header is REQUIRED: a truncated or hand-edited file with no header is a refusal, because
    `csv.DictReader` would otherwise hand back rows keyed by column INDEX and every count below would
    still be a number.
    """
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        raise Refused(f"cannot read the primdump CSV at {path}: {exc}") from exc
    first = text.split("\n", 1)[0]
    for column in ("frame,", "kind,", "op,"):
        if column not in first:
            raise Refused(f"{path} has no `{column}` header column; refusing to count an unread file")
    per_frame: dict[int, int] = {}
    kinds: dict[str, int] = {}
    ops: dict[str, int] = {}
    rows = 0
    for line in text.split("\n")[1:]:
        if not line:
            continue
        fields = line.split(",")
        if len(fields) < 5:
            raise Refused(f"{path} row {rows + 1} has {len(fields)} fields, not a primitive record")
        try:
            frame = int(fields[0])
        except ValueError as exc:
            raise Refused(f"{path} row {rows + 1} has a non-numeric frame {fields[0]!r}") from exc
        per_frame[frame] = per_frame.get(frame, 0) + 1
        kinds[fields[2]] = kinds.get(fields[2], 0) + 1
        ops[fields[3]] = ops.get(fields[3], 0) + 1
        rows += 1
    if rows == 0:
        raise Refused(f"{path} has a header and no data rows")
    counts = sorted(per_frame.values())
    return {
        "rows": rows,
        "frames": len(per_frame),
        "first_frame": min(per_frame),
        "last_frame": max(per_frame),
        "min_per_frame": counts[0],
        "max_per_frame": counts[-1],
        "kinds": dict(sorted(kinds.items(), key=lambda kv: -kv[1])),
        "ops": dict(sorted(ops.items(), key=lambda kv: -kv[1])),
    }


# ── the run ──────────────────────────────────────────────────────────────────────────────────────────

def media_present(log: pathlib.Path) -> tuple[bool, str]:
    text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
    for marker in NO_MEDIA_MARKERS:
        if marker in text:
            return False, marker
    return True, ""


def other_product_running() -> str | None:
    """A running product, matched on the EXECUTABLE. Matching anywhere in the command line is wrong:
    a shell that merely names `crash1_port` in an argument is not a running product."""
    for line in subprocess.run(["ps", "-eo", "pid,args"], capture_output=True, text=True,
                               check=False).stdout.splitlines()[1:]:
        pid, _, args = line.strip().partition(" ")
        executable = args.split(" ", 1)[0]
        if RUNNING_PRODUCT.match(executable) and "crash1_port" not in executable:
            return line.strip()
    return None


def run_leg(binary: pathlib.Path, out: pathlib.Path, frames: int, disc: str,
            dump_from: int) -> tuple[int, pathlib.Path]:
    """One headless, silent, unpaced leg. Everything is on the ENVIRONMENT: the product is the
    product, and no argument turns this into a different code path. The disc is passed the way the
    framework's `DiscConfig::resolve_disc_path` reads it (per-title key first, so a workspace with one
    disc per title cannot leak another title's media into this leg) and is never written to a tracked
    file."""
    environment = dict(os.environ)
    environment.update(
        PSXPORT_VK_HEADLESS="1",
        PSXPORT_NOAUDIO="1",
        PSXPORT_NOPACE="1",
        PSXPORT_NATIVE_FRAMES=str(frames),
        PSXPORT_PRESENT_SINK="320x240",
        PSXPORT_CRASH1_DISC=disc,
        PSXPORT_PRIMDUMP=f"0:{frames}",
        PSXPORT_DEBUG="fps60dump",
        PSXPORT_FPS60_DUMP_FROM=str(dump_from),
        SDL_VIDEODRIVER="offscreen",
        SDL_AUDIODRIVER="dummy",
    )
    log = out / "leg.log"
    with log.open("w", encoding="utf-8") as stream:
        # `start_new_session` so the whole leg is one process group we can signal by PID alone; no
        # pkill, no pattern match on a shared binary name.
        completed = subprocess.Popen([str(binary)], cwd=ROOT, env=environment, stdout=stream,
                                     stderr=subprocess.STDOUT, start_new_session=True)
        try:
            code = completed.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            completed.kill()
            completed.wait()
            raise Refused(f"the leg did not finish within 1800s and was killed by PID {completed.pid}")
    return code, log


def parse_log(log: pathlib.Path) -> dict[str, object]:
    text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
    found: dict[str, object] = {}
    for line in text.splitlines():
        if match := TELEMETRY.search(line):
            found["telemetry"] = {k: int(v) for k, v in zip(
                ("translated_blocks", "executed_blocks", "executed_instructions",
                 "fallback_blocks", "fallback_instructions"), match.groups())}
        elif match := SPAN_LINE.search(line):
            found["spans"] = (int(match.group(1)), int(match.group(2)))
        elif match := PRIMDUMP_LINE.search(line):
            found["primdump"] = (match.group(1), int(match.group(2)), int(match.group(3)),
                                 int(match.group(4)))
        elif match := PRIMDUMP_EMPTY.search(line):
            found["primdump_empty"] = (int(match.group(1)), int(match.group(2)))
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bin", type=pathlib.Path, default=ROOT / "build/crash1_port")
    parser.add_argument("--frames", type=int, default=400)
    parser.add_argument("--dump-from", type=int, default=395,
                        help="the presented fence whose PNG this tool decodes")
    parser.add_argument("--disc", help="the user's Crash Bandicoot USA CHD, passed on the command line")
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "scratch/primitives")
    args = parser.parse_args()

    if not args.disc:
        print("REFUSED: pass --disc <the user's CHD>. A leg with no media is Crash 1 with no data, and "
              "no picture verdict may be reported for one.", file=sys.stderr)
        return 2
    binary = args.bin if args.bin.is_absolute() else ROOT / args.bin
    if not binary.is_file():
        print(f"REFUSED: no product at {binary}; build it first", file=sys.stderr)
        return 2
    if holder := other_product_running():
        print(f"REFUSED: another product holds the machine's single slot, and this tool never starts a "
              f"second one or kills one it did not start:\n  {holder}", file=sys.stderr)
        return 2

    out = args.out if args.out.is_absolute() else ROOT / args.out
    out.mkdir(parents=True, exist_ok=True)
    for stale in out.glob("*.log"):
        stale.unlink()

    try:
        code, log = run_leg(binary, out, args.frames, args.disc, args.dump_from)
    except Refused as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2

    had_media, marker = media_present(log)
    print(f"== leg: exit {code}, {args.frames} requested frames, media "
          f"{'present' if had_media else f'ABSENT ({marker})'}, log {log}")
    if not had_media:
        print(f"FAIL: the leg ran with NO MEDIA (the log says {marker!r}). Nothing this run produced is "
              "a statement about Crash 1's picture. Pass --disc <the user's CHD>.", file=sys.stderr)
        return 1

    failures: list[str] = []
    try:
        facts = parse_log(log)
    except Refused as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2
    print(f"   [scan] the log is {log.stat().st_size} byte(s) over {args.frames} requested frame(s)")

    if "telemetry" not in facts:
        failures.append("the run printed no Lightrec run-end telemetry, so it did not complete a frame "
                        "loop and nothing else in this leg is a measurement")
    else:
        t = facts["telemetry"]
        print(f"   telemetry: {t['executed_instructions']} executed instructions in "
              f"{t['executed_blocks']} executed blocks from {t['translated_blocks']} translated "
              f"block(s); {t['fallback_blocks']} of {t['executed_blocks']} executed blocks fell back")

    if "spans" in facts:
        recorded, overflow = facts["spans"]
        print(f"   OtAttr spans recorded {recorded} (overflow {overflow}) — REPORTED, NOT USED: the "
              "window behind this counter is the legacy GameConfig packet pool, which a direct runtime "
              "never fills, so it reads zero by construction")

    if "primdump_empty" in facts:
        failures.append(f"the framework's own primdump reported ZERO prims over frames "
                        f"{facts['primdump_empty'][0]}..{facts['primdump_empty'][1]}")
    elif "primdump" not in facts:
        failures.append("the primdump wrote no CSV and reported no zero, so the guest's GP0 traffic was "
                        "NOT measured — a missing file is not a zero")
    else:
        csv_path, prims, first, last = facts["primdump"]
        resolved = (csv_path if pathlib.Path(csv_path).is_absolute() else ROOT / csv_path)
        try:
            census = census_csv(resolved)
        except Refused as exc:
            failures.append(f"the primdump CSV is unreadable: {exc}")
        else:
            print(f"   guest GP0: {census['rows']} primitives over {census['frames']} frame(s) "
                  f"({census['first_frame']}..{census['last_frame']}); per frame "
                  f"{census['min_per_frame']}..{census['max_per_frame']} "
                  f"(mean {census['rows'] / census['frames']:.1f})")
            print(f"   kinds: {census['kinds']}")
            print(f"   GP0 command bytes: {census['ops']}")
            if census["rows"] == 0:
                failures.append("the CSV counted zero primitives")

    framedump = ROOT / "scratch" / "framedump"
    shots = sorted(framedump.glob("*.png")) if framedump.is_dir() else []
    print(f"   presented frames captured: {len(shots)} PNG(s) in {framedump}")
    if not shots:
        failures.append("no presented frame was captured, so PRESENTATION was not measured — the run "
                        "reaching a display wait is not the same claim as a frame being shown")
    else:
        decoded: list[tuple[str, int, int, float]] = []
        for shot in shots:
            try:
                width, height, nonblack = decode_png_nonblack(shot.read_bytes())
            except (Refused, zlib.error) as exc:
                failures.append(f"{shot.name} could not be decoded ({exc}); it is NOT evidence of a "
                                "black frame")
                break
            decoded.append((shot.name, width, height, nonblack))
        if decoded and not failures:
            name, width, height, nonblack = decoded[-1]
            painted = sum(1 for *_, fraction in decoded if fraction > 0.0)
            print(f"   {name}: {width}x{height}, {nonblack * 100:.1f}% non-black pixels; "
                  f"{painted} of {len(decoded)} captured frame(s) carry a picture")
            if painted == 0:
                failures.append(f"every one of the {len(decoded)} presented frame(s) is entirely black")

    for failure in failures:
        print(f"FAIL: {failure}", file=sys.stderr)
    if failures:
        return 1
    print("PASS: Crash 1 submits primitives and presents frames with a picture; the OtAttr span counter "
          "is not what measures either")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

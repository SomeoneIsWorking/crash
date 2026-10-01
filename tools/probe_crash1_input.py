#!/usr/bin/env python3
"""Drive Crash 1 with a held pad button and report whether the TITLE responds, in one run.

WHY THIS TOOL EXISTS. It answers three questions in one run that are ambiguous apart: does the host's
button reach Crash's BIOS auto-pad word at `0x80057054`, does the presented frame change as a result,
and does the present-frame counter advance over the same hold. A word that moves while the frame does
not is a title that READS input and IGNORES it, which is a different defect from a word that never
moves at all. The widened leaf `0x80042F8C` — the camera distance and camera centre publishers — does
not run in a live level, which is what this tool is used to re-check.

WHAT IT MEASURES, AND WHY ALL THREE IN ONE RUN. A single number here is a fact about this tool:

  (a) `0x80057054` — the BIOS pad word, read live through the framework's own control channel. The
      word MOVING proves the host's button reached the BIOS's own auto-pad publisher.
  (b) the presented frame changing — captured with `shot` before and after the hold and compared as
      a non-black pixel fraction AND a byte-difference count. A word that moves and a frame that does
      not is a title that READS input and IGNORES it, which is a different defect from a word that
      never moves at all.
  (c) the present-frame counter advancing over the same hold. Without it, (b) is ambiguous between
      "the frame changed because of the input" and "the frame changed because time passed".

A fourth reading is taken because the previous two issues were both wrong in the same way: the
`ens`/`scene`/`otattr` summary is printed so the reader can see the title's own state rather than
inferring it from pixels. If a future revision quotes one of these, the denominator is in the output.

The control channel is the framework's own (`runtime/psx/dbg_server.cpp`), opened by
`PSXPORT_DEBUG_SERVER=1` on loopback, and driven with `tools/dbgclient.py`'s `LiveClient` — the same
protocol object the framework ships, so this tool does not carry a second copy of the handshake. It
is part of the product, not a test backdoor: no argument of the product changes.

    tools/probe_crash1_input.py --disc "$DISC"
    tools/probe_crash1_input.py --disc "$DISC" --button start --hold-frames 90
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import socket
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "external/psxport" / "tools"))
sys.path.insert(0, str(ROOT / "external/psxport"))

from dbgclient import LiveClient  # noqa: E402  (path is set above, deliberately)

DEFAULT_PORT = 5959

# The framework's own wording when it found no media. Matched, not guessed: a leg carrying either
# of these ran Crash 1 without its data, and nothing it reports would be a statement about the title.
NO_MEDIA_MARKERS = (
    "The CD model will run with NO MEDIA",
    "CdRead: LBA 16 unreadable",
)

RUNNING_PRODUCT = re.compile(r"^.*/([A-Za-z0-9_.]*_port)$")

# The BIOS auto-pad word from `docs/issues/0012`. The address is the one the issue measured, and the
# low 16 bits are the button mask the BIOS's own publisher writes in the consumer-pad layout.
PAD_WORD = 0x80057054

# The control channel's `rw <addr> [n]` prints `ADDR: W0 W1 ...` and the `frame` command prints both
# present counters. Parsed from the SHAPE of the reply, not from a prose sentence, so a wording change
# reads as "unparsed" rather than as a value.
RW_LINE = re.compile(r"^([0-9A-Fa-f]{8}):((?: [0-9A-Fa-f]{8})+)", re.MULTILINE)
# `frame` prints `frame=<real> interp=<n> total=<t> paused=<p> disp=(x,y)`. The FIRST version of this
# tool grepped for `real`, which the reply does not contain, so `advanced` was always False - and the
# PASS branch printed "with the frame counter advancing" while the tool's own flag said otherwise. A
# verdict that can be printed next to a contradicting flag is not a verdict.
REPLY_FRAME = re.compile(r"\bframe=(\d+)\b")
# `paused=` is the framework's OWN view of its freeze flag, and it is the reading that separates "the
# title is paused and therefore is not updating its camera" from "the title is running and the camera
# path is broken". Those are opposite next steps and they look identical from a log line.
REPLY_PAUSED = re.compile(r"\bpaused=(\d+)\b")


class Refused(Exception):
    """The input cannot support the requested claim."""


def other_product_running() -> str | None:
    """A crash1_port this tool did not start. It never starts a second one and never kills one it
    does not own, so a stale instance has to be reported rather than reaped."""
    proc = pathlib.Path("/proc")
    for entry in proc.iterdir():
        if not entry.name.isdigit():
            continue
        try:
            exe = (entry / "cmdline").read_bytes().split(b"\0")[0].decode("utf-8", "replace")
        except OSError:
            continue
        if RUNNING_PRODUCT.match(exe) and "crash1" in exe:
            return f"  pid {entry.name}: {exe}"
    return None


def read_frame_state(client: LiveClient) -> tuple[int | None, int | None]:
    """(present frame, paused) from one `frame` reply, or (None, None) if neither parsed."""
    reply = client.send("frame") or ""
    frame = REPLY_FRAME.search(reply)
    paused = REPLY_PAUSED.search(reply)
    return (int(frame.group(1)) if frame else None, int(paused.group(1)) if paused else None)


def read_word(client: LiveClient, address: int) -> int | None:
    """Read one guest word through the control channel, or None if the reply carries no word.

    A missing word is None and NOT 0. "The channel did not answer" and "the word is zero" are
    different facts, and collapsing them is the mistake this workspace keeps making."""
    # `rw`, NOT `w32`. `w32 A V` is a WRITE, so reading a word with it would have answered with the
    # address it echoed and this tool would have reported "the word never moved" for a run in which
    # it moved every sample - a measurement of the wrong verb, which is the mistake this whole
    # workspace keeps meeting.
    reply = client.send(f"rw {address:#x} 1") or ""
    match = RW_LINE.search(reply)
    if not match:
        return None
    return int(match.group(2).split()[0], 16)


def nonblack_fraction(path: pathlib.Path) -> float | None:
    """Non-black fraction of a PPM (P6), or None if it is not one.

    The framework's `shot` writes PPM. A missing or short file is None, never 0.0 — the same
    discipline the PNG decoder in probe_crash1_primitives.py enforces."""
    try:
        data = path.read_bytes()
    except OSError:
        return None
    if not data.startswith(b"P6"):
        return None
    fields, pos = [], 2
    while len(fields) < 3:
        while pos < len(data) and data[pos : pos + 1].isspace():
            pos += 1
        if data[pos : pos + 1] == b"#":
            while pos < len(data) and data[pos] != 0x0A:
                pos += 1
            continue
        start = pos
        while pos < len(data) and not data[pos : pos + 1].isspace():
            pos += 1
        fields.append(int(data[start:pos]))
    pos += 1
    width, height, _maxval = fields
    pixels = data[pos : pos + width * height * 3]
    if len(pixels) != width * height * 3:
        return None
    lit = sum(1 for i in range(0, len(pixels), 3) if pixels[i] or pixels[i + 1] or pixels[i + 2])
    return lit / float(width * height)


def capture(client: LiveClient, path: pathlib.Path) -> tuple[float | None, int | None]:
    if path.exists():
        path.unlink()
    reply = client.send(f"shot {path}")
    if not path.is_file():
        raise Refused(f"the control channel reported no capture: {reply!r}")
    import hashlib
    return nonblack_fraction(path), hashlib.sha256(path.read_bytes()).hexdigest()[:12]


def run(binary: pathlib.Path, out: pathlib.Path, frames: int, disc: str, port: int,
        button: str, hold_frames: int, taps: int, tap_gap: float, aspect: int) -> int:
    settings = out / "settings.ini"
    settings.write_text(f"aspect={aspect}\n", encoding="utf-8")
    environment = dict(os.environ)
    environment.update(
        PSXPORT_VK_HEADLESS="1",
        PSXPORT_NOAUDIO="1",
        PSXPORT_NOPACE="1",
        PSXPORT_NATIVE_FRAMES=str(frames),
        PSXPORT_PRESENT_SINK="320x240",
        PSXPORT_CRASH1_DISC=disc,
        PSXPORT_DEBUG_SERVER=str(port),
        # The aspect is a SETTINGS file, not an env knob: `ASPECT_AUTO` resolves against the sink
        # and silently means 4:3 headless, so a wide leg has to be requested by the file. It matters
        # here because the widened centre's log line is printed ONLY when the latched plan is wide
        # AND the retail centre is non-zero, so a 4:3 leg cannot report a centre at all - and its
        # silence is not a measurement of the owner.
        PSXPORT_SETTINGS=str(settings),
        SDL_VIDEODRIVER="offscreen",
        SDL_AUDIODRIVER="dummy",
    )
    log = out / "input_leg.log"
    with log.open("w", encoding="utf-8") as stream:
        process = subprocess.Popen([str(binary)], cwd=ROOT, env=environment, stdout=stream,
                                   stderr=subprocess.STDOUT, start_new_session=True)
    try:
        client = connect(port, process.pid)
        try:
            return drive(client, out, button, hold_frames, taps, tap_gap)
        finally:
            try:
                client.close()
            except OSError:
                pass
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    del log


def connect(port: int, pid: int, timeout: float = 60.0) -> LiveClient:
    deadline = time.monotonic() + timeout
    last: Exception | None = None
    while time.monotonic() < deadline:
        try:
            client = LiveClient(port=port)
            client.send("frame")
            return client
        except (OSError, socket.error) as exc:
            last = exc
            time.sleep(0.5)
    raise Refused(f"the control channel on port {port} never answered (pid {pid}): {last}")


def drive(client: LiveClient, out: pathlib.Path, button: str, hold_frames: int, taps: int,
          tap_gap: float) -> int:
    print(f"== driving button {button!r} for {hold_frames} presented frame(s)")
    baseline_frame = REPLY_FRAME.search(client.send("frame") or "")
    before_word = read_word(client, PAD_WORD)
    before_frac, before_hash = capture(client, out / "before.ppm")
    print(f"   before: pad word 0x80057054 = "
          f"{'unread' if before_word is None else f'0x{before_word:08X}'}, "
          f"present frame {baseline_frame.group(1) if baseline_frame else 'unread'}, "
          f"frame non-black {before_frac}")

    client.send(f"press {button}")
    # Sample while held, so a word that moves and returns is still visible.
    samples = []
    for _ in range(max(1, hold_frames // 10)):
        time.sleep(0.4)
        samples.append(read_word(client, PAD_WORD))
    after_frame = REPLY_FRAME.search(client.send("frame") or "")
    client.send(f"release {button}")
    time.sleep(0.4)
    after_word = read_word(client, PAD_WORD)
    after_frac, after_hash = capture(client, out / "after.ppm")

    print(f"   during: pad word samples {[f'0x{w:08X}' if w is not None else 'unread' for w in samples]}")
    print(f"   after:  pad word 0x80057054 = "
          f"{'unread' if after_word is None else f'0x{after_word:08X}'}, "
          f"present frame {after_frame.group(1) if after_frame else 'unread'}, "
          f"frame non-black {after_frac}")
    # The title menu answers Start, and the menu's own Start is what leads toward a level. Tapping it
    # is how the run gets far enough to publish a camera, and the camera is what issue 0023 is
    # waiting on - so the taps exist to give that question a chance to be answered in the same run
    # rather than in a second one.
    for index in range(taps):
        client.send(f"tap {button}")
        time.sleep(tap_gap)
        word = read_word(client, PAD_WORD)
        reply = client.send("frame") or ""
        marker = REPLY_FRAME.search(reply)
        print(f"   tap {index + 1}/{taps}: pad word "
              f"{'unread' if word is None else f'0x{word:08X}'}, present frame "
              f"{marker.group(1) if marker else 'unread'}")
    final_frac, final_hash = capture(client, out / "final.ppm")
    final_frame, paused = read_frame_state(client)
    print(f"   after {taps} tap(s): frame non-black {final_frac} (sha {final_hash}), present frame "
          f"{final_frame}, paused {paused}")
    # A title-level pause is ANSWERED BY THE TITLE, so it is evidence about the guest and is released
    # through the pad. The framework's own `pause` command would freeze the run from outside and would
    # make the camera question unanswerable, which is the opposite of what this step is for.
    if paused == 1:
        client.send(f"press {button}")
        time.sleep(0.5)
        during_hold = read_word(client, PAD_WORD)
        client.send(f"release {button}")
        time.sleep(tap_gap)
        resumed_frame, still_paused = read_frame_state(client)
        print(f"   unpause: pad word while held "
              f"{'unread' if during_hold is None else f'0x{during_hold:08X}'}, present frame "
              f"{resumed_frame}, paused {still_paused}")
        settled_frac, settled_hash = capture(client, out / "settled.ppm")
        print(f"   after unpause: frame non-black {settled_frac} (sha {settled_hash})")
        if settled_hash is not None and settled_hash == final_hash:
            print("   NOTE: the frame is byte-identical after the unpause, so the level is not "
                  "animating and no camera question can be answered from this state.")

    for command in ("ens", "scene"):
        reply = client.send(command) or ""
        print(f"   --- {command} ---")
        for line in reply.splitlines()[:20]:
            print(f"   | {line}")

    moved = {w for w in samples if w is not None} - ({before_word} if before_word is not None else set())
    advanced = False
    if baseline_frame and after_frame:
        advanced = int(after_frame.group(1)) > int(baseline_frame.group(1))
    # A fixed-size PPM has a constant file length, so a SIZE comparison would have reported "the frame
    # did not change" for a frame whose every pixel changed. The content digest is the comparison.
    changed = before_hash is not None and after_hash is not None and before_hash != after_hash
    lit_delta = (None if before_frac is None or after_frac is None else after_frac - before_frac)

    print(f"== pad word moved during the hold : {sorted(f'0x{w:08X}' for w in moved) or 'NO'}")
    print(f"== present frame advanced         : {advanced}")
    print(f"== captured frame content changed : {changed} (sha {before_hash} -> {after_hash})")
    print(f"== non-black delta                : {lit_delta}")
    # Every branch states the three readings it actually has. A branch that cannot be reached is a
    # defect in the tool and says so, rather than falling through to a PASS.
    if not moved:
        print("FINDING: the host's button never reached the BIOS pad word 0x80057054. That is a "
              "publisher defect, not a title defect, and it is measured rather than inferred.")
        return 1
    if not changed:
        print("FINDING: the pad word moved and the captured frame content did not change. The title "
              "reads the word and does not act on it; the next question is which reader consumes it.")
        return 1
    if not advanced:
        print(f"UNUSABLE: the pad word moved and the frame changed, but the present-frame counter did "
              f"not parse (before {baseline_frame}, after {after_frame}), so 'the input caused this' is "
              f"not established. Fix the parse before quoting this run.")
        return 2
    print(f"PASS: the pad word moved to {sorted(f'0x{w:08X}' for w in moved)[0]}, the captured frame "
          f"content changed, and the present counter advanced "
          f"{baseline_frame.group(1)} -> {after_frame.group(1)} over the same interval.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--bin", type=pathlib.Path, default=ROOT / "build/crash1_port")
    parser.add_argument("--frames", type=int, default=900)
    parser.add_argument("--disc", help="the user's Crash Bandicoot USA CHD, passed on the command line")
    parser.add_argument("--button", default="start",
                        help="pad button to hold: start/select/x/o/triangle/square/up/down/left/right")
    parser.add_argument("--hold-frames", type=int, default=90)
    parser.add_argument("--taps", type=int, default=0,
                        help="tap the button this many times after the hold, to walk past a title menu")
    parser.add_argument("--tap-gap", type=float, default=2.0,
                        help="seconds between taps (the run is unpaced, so this is wall clock)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--aspect", type=int, default=0, choices=(0, 1),
                        help="0 = 4:3, 1 = 16:9. The widened centre is only reported in a wide leg.")
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "scratch/input")
    args = parser.parse_args()

    if not args.disc:
        print("REFUSED: pass --disc <the user's CHD>. A leg with no media is Crash 1 with no data, "
              "and no verdict about input may be reported for one.", file=sys.stderr)
        return 2
    binary = args.bin if args.bin.is_absolute() else ROOT / args.bin
    if not binary.is_file():
        print(f"REFUSED: no product at {binary}; build it first", file=sys.stderr)
        return 2
    if holder := other_product_running():
        print(f"REFUSED: another product holds the machine's single slot, and this tool never starts "
              f"a second one or kills one it did not start:\n{holder}", file=sys.stderr)
        return 2
    out = args.out if args.out.is_absolute() else ROOT / args.out
    out.mkdir(parents=True, exist_ok=True)
    try:
        code = run(binary, out, args.frames, args.disc, args.port, args.button, args.hold_frames,
                   args.taps, args.tap_gap, args.aspect)
    except Refused as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2
    text = (out / "input_leg.log").read_text(encoding="utf-8", errors="replace")
    # The widescreen owner's own line, read after the taps. `docs/issues/0023` is waiting on exactly
    # this: the widened leaf 0x80042F8C has exactly two callers, both camera code, and neither ran in
    # 400 un-driven frames. Reporting the line's presence is a fact; its absence is one too, as long
    # as the leaf's own invocation count is the thing being read rather than the log line alone.
    centres = [line for line in text.splitlines() if "guest centre" in line]
    print(f"== the widened leaf published a centre {len(centres)} time(s)"
          + (f"; first: {centres[0].split(']', 1)[-1].strip()}" if centres else ""))
    for marker in NO_MEDIA_MARKERS:
        if marker in text:
            print(f"FAIL: the leg ran with NO MEDIA ({marker!r} in the log). Nothing this run produced "
                  f"is a statement about Crash 1's input.", file=sys.stderr)
            return 1
    return code


if __name__ == "__main__":
    raise SystemExit(main())

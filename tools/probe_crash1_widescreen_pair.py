#!/usr/bin/env python3
"""Build Crash 1's 4:3-versus-16:9 widescreen picture pair from the real product.

WHY A TOOL AND NOT TWO COMMANDS. `PSXPORT_PRESENT_SINK` and the settings file are process-wide, so
each aspect must be captured in its OWN process. `ASPECT_AUTO` (`aspect=3`) resolves against the
SINK's aspect, so a headless run of it measures 4:3 while appearing to be wide — which is why nothing
here reads a config value as evidence. Each leg names its own tracked `.ini` through
`PSXPORT_SETTINGS`, and the witness quoted is the product's own `render_width` / `native_width` pair,
never `wide_engine`.

CRASH 1 HAS A THIRD, TITLE-SPECIFIC WITNESS, and it is the one that decides the question. This title
publishes its horizontal projection as the GTE screen offset OFX (cop2 control register 24), not as a
viewport rectangle, and the owner records what the GUEST's own registers hold after the guest ran:
`[crash1-wide] guest projection init published H 1000, OFX 0, OFY 0 ... host canvas N (native 320)`.
That line is the widening's real witness, and it appears in BOTH the old and the new owner.

THE PRODUCT'S OWN ABORTS ARE REPORTED BY NAME, NOT TIMED OUT. `crash1_port` refuses rather than
limping when its frame-loop contract or its projection guard is violated, and both refusals abort the
process. A tool that waited out its timeout would report "never reached frame N" for both legs and
lose the reason, which is the only part of this that is actually informative. So each named refusal
line is matched, quoted, and reported as a REFUSAL with the exit code it produced.

The tool launches the product itself, refuses to start while another product binary holds the
machine's single slot, and kills only PIDs it captured. Never `pkill`/`pgrep -f`.

    tools/probe_crash1_widescreen_pair.py --selftest
    tools/probe_crash1_widescreen_pair.py --disc "$DISC" --frame 400

`--disc`, or `$PSXPORT_DISC`, names the user's image. There is no default: a machine-specific path
baked into a tracked tool is exactly what must never ship, and a default is that path with extra
steps. The tool refuses by name and says which variable to set.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
FRAMEWORK = ROOT / "external" / "psxport"
SCRATCH = ROOT / "scratch" / "wproof"
# The user's disc image is RUNTIME INPUT and is named by the environment, never baked into a tracked
# file: a machine-specific absolute path in a tracked tool is the one thing that must never ship, and
# the product takes the same variable. No default, because a default is a machine path with extra
# steps — the tool refuses by name instead and says which variable to set.
DISC_ENV = "PSXPORT_DISC"

sys.path.insert(0, str(FRAMEWORK / "tools"))
sys.path.insert(0, str(FRAMEWORK / "tools" / "port"))

# The measured native display extent, used ONLY to size the sink. The evidence quoted is the product's
# own render_width and the guest's own published projection words.
NATIVE_WIDTH = 320
SINK_HEIGHT = 240

LEG_SETTINGS = {"4x3": "config/aspect_4x3.ini", "16x9": "config/aspect_16x9.ini"}
ASPECT_VALUES = {"4x3": 0, "16x9": 1}

# The keys `Mods::load` actually accepts, from `runtime/psx/mods.cpp`. It has NO comment support: it
# splits every line on the first `=` and compares the whole prefix as the key, so a documented settings
# file emits `unknown key "# ..." ignored — it configures nothing` for every comment line containing an
# `=`. This list is here so the selftest can assert the tracked files parse CLEANLY rather than
# asserting a comment convention the parser does not share.
SETTINGS_KEYS = frozenset({
    "aspect", "ires", "ires_auto", "face_order", "ssao", "light", "shadows",
    "shadow_strength", "fps60", "ssao_strength", "ssao_radius", "ssao_bias", "ssao_range",
    "light_dir", "light_ambient", "light_diffuse",
})

PRODUCT = re.compile(r"^.*/([A-Za-z0-9_.]*_port)$")
WIDE_LINE = re.compile(r"native picture: aspect=(\d+) wide_engine=(\d+) "
                       r"native_width=(\d+) render_width=(\d+)")
SHOT_LINE = re.compile(r"\[gpu_shot\] wrote (\S+) \((\d+)x(\d+)")
# The GUEST's own published projection words and the host canvas the plan latched. This is the
# title-specific witness; `[wide] native picture:` is printed before the guest publishes.
GUEST_PROJECTION = re.compile(
    r"guest projection init published H (\d+), OFX (-?\d+), OFY (-?\d+).*host canvas (\d+) "
    r"\(native (\d+)\)")

# Each entry is (regex, the reason it is a REFUSAL, the owner it names). Matching a line here is a
# refusal with a name attached; not matching one is not a pass, it is a leg that ran.
#
# THE CHANNEL IS MATCHED AS `crash1-frame` + an optional `:` SEVERITY SUFFIX, not as the bare name.
# The product logs refusals as `[crash1-frame:error]` and `[crash1-wide:error]`, and an earlier
# version of this table matched `[crash1-frame]` exactly — so it matched NOTHING on a real log and
# would have reported a clean leg for a product that had aborted. The tool's own selftest is what
# caught it, by feeding it the real line: a refusal matcher that cannot match the string it exists to
# match is worse than no matcher, because it manufactures the opposite answer.
REFUSALS = (
    (re.compile(r"\[crash1-frame[:\w]*\] frame (\d+) reached an unexpected boundary at (0x[0-9A-F]{8})"),
     "the title's frame-loop contract refused a display-field boundary whose provenance is not the "
     "one measured VSync site",
     "titles/crash1/core/crash1_frame_driver.cpp"),
    (re.compile(r"\[crash1-frame[:\w]*\] frame (\d+) left guest execution at (0x[0-9A-F]{8}) with (\w+)"),
     "the title's frame-loop contract refused a frame that ended somewhere other than its measured "
     "boundary",
     "titles/crash1/core/crash1_frame_driver.cpp"),
    (re.compile(r"\[crash1-wide[:\w]*\] the guest published OFX (-?\d+) but \$a0 was (-?\d+)"),
     "the widening owner's own guard refused the frame: it compared the published GTE register against "
     "$a0 read AFTER the retail leaf had shifted that register in place",
     "game/core/guest_projection_publication.cpp"),
    (re.compile(r"\[crash1-frame[:\w]*\] frame (\d+) returned without the measured one-or-two GpuUpdate"),
     "the frame completed without the measured one-or-two display waits",
     "titles/crash1/core/crash1_frame_driver.cpp"),
    (re.compile(r"\[crash1-frame[:\w]*\] frame (\d+) reached an unexpected boundary"),
     "the title's frame-loop contract refused a display-field boundary",
     "titles/crash1/core/crash1_frame_driver.cpp"),
)


def other_product_running() -> str | None:
    """A running product, matched on the EXECUTABLE, never anywhere in the command line."""
    for line in subprocess.run(["ps", "-eo", "pid,args"], capture_output=True, text=True,
                               check=False).stdout.splitlines()[1:]:
        _, _, args = line.strip().partition(" ")
        executable = args.split(" ", 1)[0]
        if PRODUCT.match(executable) and "crash1_port" not in executable:
            return line.strip()
    return None


def sink_width(aspect: int) -> int:
    return NATIVE_WIDTH if aspect == 0 else NATIVE_WIDTH * 16 // 12


def parse_wide_lines(log_text: str) -> list[dict]:
    """EVERY `[wide] native picture:` line, in order, with the source line it came from."""
    found = []
    for number, line in enumerate(log_text.splitlines(), start=1):
        found_match = WIDE_LINE.search(line)
        if found_match:
            found.append({
                "log_line": number,
                "aspect": int(found_match.group(1)),
                "wide_engine": int(found_match.group(2)),
                "native_width": int(found_match.group(3)),
                "render_width": int(found_match.group(4)),
                "text": line.split("] ", 1)[-1],
            })
    return found


def classify_wide(lines: list[dict]) -> dict:
    """Which `[wide]` line is the steady state, and did the product announce a wider picture.

    The LAST line is the post-latch steady state. `announced_wider` is what THIS source line says and
    it is deliberately kept separate from `guest_widened`, which is the guest's own published OFX:
    on this title the two answer different questions and the project state has recorded that the
    announce line alone is not sufficient.
    """
    if not lines:
        return {"announced_wider": None,
                "reason": "the product printed NO [wide] native picture line at all, so it never "
                          "announced a picture and there is nothing to read"}
    last = lines[-1]
    return {
        "steady_state_log_line": last["log_line"],
        "native_width": last["native_width"],
        "render_width": last["render_width"],
        "announced_wider": last["render_width"] > last["native_width"],
        "announced": len(lines),
    }


def parse_guest_projection(log_text: str) -> dict | None:
    """The GUEST's own published OFX/OFY/H and the host canvas the plan latched, or None.

    This is the title-specific witness and it is read out of the guest's coprocessor registers rather
    than out of any config value. Absence is reported as None, not as zero.
    """
    found = GUEST_PROJECTION.findall(log_text)
    if not found:
        return None
    h, ofx, ofy, canvas, native = found[-1]
    return {"H": int(h), "OFX": int(ofx), "OFY": int(ofy),
            "host_canvas": int(canvas), "native": int(native),
            "occurrences": len(found)}


def find_refusals(log_text: str) -> list[dict]:
    """Every named product refusal in the log, with the line and the owner it names.

    Reported by name and count so a reader can tell "the product refused for a measured reason" from
    "the tool found nothing to refuse about", which are not the same statement.
    """
    found = []
    for number, line in enumerate(log_text.splitlines(), start=1):
        for pattern, reason, owner in REFUSALS:
            match = pattern.search(line)
            if match:
                found.append({"log_line": number, "reason": reason, "owner": owner,
                              "detail": line.split("] ", 1)[-1]})
                break
    return found


def parse_telemetry(text: str) -> dict:
    """Guest-execution counters out of the debug endpoint's own `guest` reply.

    This is the SIMULATION-UNDISTURBED witness. Every counter rides through verbatim, including the
    fallback ones, so a fallback that did not happen is visible as 0 rather than absent.
    """
    counters = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", text)}
    if not counters:
        raise ValueError(f"the guest reply carried no counters: {text.strip()!r}")
    return counters


def run_leg(leg: str, frame: int, port: int, binary: pathlib.Path, timeout: int,
            disc: str) -> dict:
    settings = ROOT / LEG_SETTINGS[leg]
    if not settings.is_file():
        raise SystemExit(f"REFUSED: tracked settings {settings} is missing; the product would run on "
                         f"whatever untracked ini sits beside it. NOTHING WAS MEASURED.")
    aspect = ASPECT_VALUES[leg]
    SCRATCH.mkdir(parents=True, exist_ok=True)
    log = SCRATCH / f"leg_{leg}.log"
    capture = SCRATCH / "shots" / f"{leg}-f{frame}.ppm"
    capture.parent.mkdir(parents=True, exist_ok=True)

    from launch_environment import agent_environment

    busy = other_product_running()
    if busy:
        raise SystemExit(f"REFUSED: another product is already running ({busy}); the machine has one "
                         f"product slot. NOTHING WAS MEASURED.")

    environment = agent_environment(os.environ, settings=settings)
    environment.update({
        "PSXPORT_PRESENT_SINK": f"{sink_width(aspect)}x{SINK_HEIGHT}",
        "PSXPORT_LOG_FILE": str(log),
        "PSXPORT_DISC": disc,
        "PSXPORT_DEBUG_SERVER": str(port),
        "PSXPORT_WATCHDOG": "900",
        "PSXPORT_PRESENT_SHOT_AT": str(frame),
        "SDL_VIDEODRIVER": "offscreen",
        "SDL_AUDIODRIVER": "dummy",
        "VK_ICD_FILENAMES": os.environ.get("VK_ICD_FILENAMES",
                                           "/usr/share/vulkan/icd.d/radeon_icd.x86_64.json"),
    })
    print(f"[{leg}] settings={settings} aspect={aspect} sink={environment['PSXPORT_PRESENT_SINK']} "
          f"disc={disc}; target presented frame {frame}", flush=True)
    process = subprocess.Popen([str(binary)], cwd=ROOT, env=environment,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    result = {"leg": leg, "aspect": aspect, "settings": str(settings),
              "sink": environment["PSXPORT_PRESENT_SINK"], "pid": process.pid,
              "target_frame": frame, "capture": str(capture), "log": str(log),
              "reached_frame": None, "shot_reply": None, "shot_size": None,
              "telemetry": None, "exit": None, "note": ""}
    try:
        from dbgclient import LiveClient
        client = None
        deadline = time.time() + timeout
        while time.time() < deadline:
            if process.poll() is not None:
                result["note"] = (f"the product EXITED on its own (rc={process.returncode}) before "
                                  f"presented frame {frame}")
                break
            try:
                if client is None:
                    client = LiveClient(port=port, timeout=120.0)
                counters = client.frames()
                if counters["frame"] >= frame:
                    result["reached_frame"] = counters["frame"]
                    result["frames_split"] = counters
                    break
                time.sleep(2.0)
            except (OSError, RuntimeError):
                client = None
                time.sleep(2.0)
        else:
            result["note"] = f"NEVER reached presented frame {frame} within {timeout}s"
        if result["reached_frame"] is not None and client is not None:
            result["shot_reply"] = client.shot(str(capture)).strip()
            try:
                result["telemetry"] = parse_telemetry(client.send("guest"))
            except (OSError, RuntimeError, ValueError) as error:
                result["note"] = f"the guest telemetry could not be read: {error}"
            try:
                client.quit()
            except (OSError, RuntimeError):
                pass
    finally:
        try:
            process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            process.terminate()  # this tool's own child, by the captured PID
            process.wait(timeout=60)
        result["exit"] = process.returncode
    log_text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
    result["wide_lines"] = parse_wide_lines(log_text)
    result["width_verdict"] = classify_wide(result["wide_lines"])
    result["guest_projection"] = parse_guest_projection(log_text)
    result["refusals"] = find_refusals(log_text)
    shots = SHOT_LINE.findall(log_text)
    if shots:
        result["shot_size"] = f"{shots[-1][1]}x{shots[-1][2]}"
    (SCRATCH / f"leg_{leg}.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    return result


def report_leg(result: dict, out) -> None:
    out(f"[{result['leg']}] pid={result['pid']} exit={result['exit']} "
        f"reached presented frame {result['reached_frame']} of {result['target_frame']} asked")
    verdict = result["width_verdict"]
    out(f"[{result['leg']}] announced [wide] lines: {verdict.get('announced', 0)}")
    for line in result["wide_lines"]:
        out(f"    log:{line['log_line']}  {line['text']}")
    out(f"[{result['leg']}] announce steady state (log line {verdict.get('steady_state_log_line')}): "
        f"render_width={verdict.get('render_width')} native_width={verdict.get('native_width')} "
        f"-> announced wider is {verdict.get('announced_wider')}")
    if "reason" in verdict:
        out(f"[{result['leg']}] NO ANNOUNCE VERDICT: {verdict['reason']}")
    projection = result["guest_projection"]
    if projection:
        out(f"[{result['leg']}] GUEST's own published projection (the title's real witness, "
            f"{projection['occurrences']} occurrence(s) in the log): H {projection['H']}, "
            f"OFX {projection['OFX']}, OFY {projection['OFY']} -> host canvas "
            f"{projection['host_canvas']} (native {projection['native']})")
    else:
        out(f"[{result['leg']}] the guest published NO projection words in this run, so there is no "
            f"guest-side witness at all — that is an absence, not a zero")
    out(f"[{result['leg']}] named product refusals: {len(result['refusals'])}")
    for refusal in result["refusals"]:
        out(f"    log:{refusal['log_line']}  {refusal['detail']}")
        out(f"      -> {refusal['reason']}")
        out(f"      -> owner: {refusal['owner']}")
    out(f"[{result['leg']}] guest-resolution readback: {result['shot_reply']} "
        f"({result['shot_size']})")
    if result["telemetry"]:
        keys = ("calls", "translated_blocks", "executed_blocks", "executed_instructions",
                "host_dispatches", "cache_hits", "cache_misses", "invalidations", "faults")
        out(f"[{result['leg']}] guest telemetry: "
            + " ".join(f"{key}={result['telemetry'].get(key)}" for key in keys))
    if result["note"]:
        out(f"[{result['leg']}] NOTE: {result['note']}")


def selftest(out=print) -> int:
    """Prove the receipt logic reports BOTH answers, then exit.

    The refusal matcher is the important half. A leg that finds no named refusal must NOT read as a
    pass, and a leg that finds one must report it with its owner rather than as a timeout — so the
    fixtures are a clean log, a log carrying each of the real refusal lines, and a log carrying a line
    that only LOOKS like one.
    """
    checks = 0
    failures = []

    clean = ("[wide] native picture: aspect=0 wide_engine=0 native_width=320 render_width=320\n"
             "[crash1-wide] guest projection init published H 1000, OFX 0, OFY 0 — retail; "
             "host canvas 320 (native 320)\n")
    checks += 1
    if find_refusals(clean):
        failures.append("a clean log was reported as carrying a refusal")
    checks += 1
    projection = parse_guest_projection(clean)
    if projection is None or projection["host_canvas"] != 320 or projection["OFX"] != 0:
        failures.append(f"the clean fixture's guest projection read {projection}, expected OFX 0 / 320")
    checks += 1
    if parse_guest_projection("[boot] nothing published here\n") is not None:
        failures.append("a log with no published projection reported one; absence must be None, "
                        "never a zero")

    wide_log = ("[wide] native picture: aspect=1 wide_engine=0 native_width=320 render_width=320\n"
                "[crash1-wide] guest projection init published H 1000, OFX 0, OFY 0; "
                "host canvas 428 (native 320)\n")
    verdict = classify_wide(parse_wide_lines(wide_log))
    checks += 1
    if verdict["announced_wider"] is not False:
        failures.append("the announce line read a wider picture for render_width == native_width")
    checks += 1
    if "announced_wider" not in verdict:
        failures.append("the announce verdict is not separated from the guest-side witness, so the two "
                        "different questions this title asks would be reported as one")

    for label, line in (
            ("frame contract",
             "[crash1-frame:error] frame 0 reached an unexpected boundary at 0x800170FC with ra=0x800170FC"),
            ("projection guard",
             "[crash1-wide:error] the guest published OFX 86 but $a0 was 5636096; the plan and the "
             "guest's own leaf disagree, so the frame would not be the widening it claims")):
        found = find_refusals(line)
        checks += 1
        if len(found) != 1:
            failures.append(f"the {label} refusal line matched {len(found)} refusals, expected 1")
        elif not found[0]["owner"].startswith(("titles/", "game/")):
            failures.append(f"the {label} refusal named no owner: {found[0]['owner']!r}")

    # A line that only LOOKS like a refusal must not be counted as one, and neither must the bare
    # channel name: these are the two ways a matcher over-reports.
    checks += 1
    if find_refusals("[cfg] PSXPORT_FPS60 = false [default]\n"):
        failures.append("an unrelated line was matched as a product refusal")
    checks += 1
    if find_refusals("[crash1-wide] guest widescreen installed: SetGeomOffset 0x80042F8C\n"):
        failures.append("the title's ordinary install announcement was matched as a refusal")

    for leg, path in LEG_SETTINGS.items():
        resolved = ROOT / path
        checks += 1
        if not resolved.is_file():
            failures.append(f"tracked settings {path} named by the {leg} leg is missing")
            continue
        checks += 1
        if f"aspect={ASPECT_VALUES[leg]}" not in resolved.read_text(encoding="utf-8"):
            failures.append(f"{path} does not pin aspect={ASPECT_VALUES[leg]}")
        # The product must be able to read every line of its own control file: `Mods::load` has no
        # comment support, so a documented settings file makes the product log `unknown key` warnings
        # that read like misconfiguration.
        for number, line in enumerate(resolved.read_text(encoding="utf-8").splitlines(), start=1):
            if not line.strip():
                continue
            if "=" not in line:
                failures.append(f"{path}:{number} is {line!r}, which `Mods::load` silently skips "
                                f"(no `=`); a control file should have nothing it cannot express")
                continue
            if line.split("=", 1)[0] not in SETTINGS_KEYS:
                failures.append(f"{path}:{number} has key {line.split('=', 1)[0]!r}, which "
                                f"`Mods::load` does not accept; the product will log `unknown key` "
                                f"and configure nothing")

    for failure in failures:
        out(f"crash1 widescreen-pair selftest: FAIL — {failure}")
    if failures:
        out(f"crash1 widescreen-pair selftest: {checks - len(failures)}/{checks} => FAIL")
        return 1
    out(f"crash1 widescreen-pair selftest: {checks}/{checks} => PASS (a clean log carries no refusal "
        f"and its guest projection reads OFX 0, a log with no published projection reports absence as "
        f"None rather than zero, the announce verdict is kept separate from the guest-side witness, "
        f"both real refusal lines are matched once each with the owner that raised them, an unrelated "
        f"line and the title's own install announcement are not matched, both tracked settings files "
        f"pin different aspects, and every line of both files is a key `Mods::load` actually accepts)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--selftest", action="store_true", help="prove the receipt logic, then exit")
    parser.add_argument("--leg", choices=sorted(LEG_SETTINGS), help="run ONE aspect leg")
    parser.add_argument("--frame", type=int, default=400, help="presented frame to capture at")
    parser.add_argument("--port", type=int, default=5961, help="debug endpoint port")
    parser.add_argument("--timeout", type=int, default=600, help="seconds to wait for the frame")
    parser.add_argument("--binary", default="build/ci/crash1_port")
    parser.add_argument("--disc", help=f"the user's disc image; defaults to ${DISC_ENV}")
    args = parser.parse_args()
    if args.selftest:
        return selftest()
    disc = args.disc or os.environ.get(DISC_ENV)
    if not disc:
        print(f"REFUSED: no disc image. Pass --disc PATH or set {DISC_ENV}. The user's image is "
              f"runtime input and is deliberately not baked into a tracked tool. NOTHING WAS "
              f"MEASURED.", flush=True)
        return 2
    if not pathlib.Path(disc).is_file():
        print(f"REFUSED: disc image {disc} is not a file. NOTHING WAS MEASURED.", flush=True)
        return 2
    binary = ROOT / args.binary
    if not binary.is_file():
        print(f"REFUSED: {binary} does not exist. NOTHING WAS MEASURED.", flush=True)
        return 2

    legs = [args.leg] if args.leg else ["4x3", "16x9"]
    results = []
    for index, leg in enumerate(legs):
        results.append(run_leg(leg, args.frame, args.port + index, binary, args.timeout, disc))
        report_leg(results[-1], print)
    if len(results) != 2:
        return 1
    refused = [r["leg"] for r in results if r["refusals"] or not pathlib.Path(r["capture"]).is_file()]
    if refused:
        print(f"REFUSED: the {', '.join(refused)} leg(s) produced no capture because the product itself "
              f"refused, with the named owner quoted above. NOTHING WAS COMPARED, and this is not a "
              f"pass — a widening with no picture behind it is a mechanism, not a capability.", flush=True)
        return 2
    import widescreen_pair
    return widescreen_pair.report(results[0]["capture"], results[1]["capture"])


if __name__ == "__main__":
    raise SystemExit(main())

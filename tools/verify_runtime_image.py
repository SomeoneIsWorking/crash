#!/usr/bin/env python3
"""Compare a title runtime's GuestProgramImage with the real EXE and shipping crt0 decoder."""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import verify_executable


class Refused(RuntimeError):
    """The supplied tools or data cannot support a runtime-image comparison."""


def run(command: list[str]) -> str:
    try:
        result = subprocess.run(
            command, cwd=ROOT, text=True, capture_output=True, check=False
        )
    except OSError as exc:
        raise Refused(f"could not execute {command[0]}: {exc}") from exc
    if result.returncode:
        detail = (result.stderr or result.stdout).strip()
        raise Refused(f"{command[0]} exited {result.returncode}: {detail}")
    return result.stdout + result.stderr


def capture(pattern: str, text: str, label: str, group: int = 1) -> int:
    match = re.search(pattern, text, re.MULTILINE)
    if match is None:
        raise Refused(f"crt0 extractor reported no parseable {label}")
    return int(match.group(group), 0)


def parse_runtime_facts(text: str) -> dict[str, int]:
    facts: dict[str, int] = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        name, value = line.split("=", 1)
        try:
            facts[name] = int(value, 0)
        except ValueError as exc:
            raise Refused(
                f"runtime fact {name!r} is not an integer: {value!r}"
            ) from exc
    if "runtime facts: 20 field(s)" not in text:
        raise Refused("runtime facts tool omitted its 20-field denominator")
    return facts


def expected_facts(
    manifest: dict[str, object], extractor_output: str
) -> dict[str, int]:
    want = verify_executable.expected(manifest)
    runtime = manifest.get("runtime", {})
    if not isinstance(runtime, dict):
        raise Refused("manifest runtime facts must be an object")
    game_main = int(str(runtime.get("game_main_entry", "0")), 0)
    vsync_begin, vsync_end, _ = verify_executable.vsync_range(manifest)
    text_begin = int(want["text_address"])
    text_end = text_begin + int(want["text_size"])
    return {
        "bss_begin": capture(r"bss \[(0x[0-9A-Fa-f]+),", extractor_output, "bss begin"),
        "bss_end": capture(
            r"bss \[0x[0-9A-Fa-f]+,(0x[0-9A-Fa-f]+)\)", extractor_output, "bss end"
        ),
        "stack_top_word": capture(
            r"stackTopBase\s+(0x[0-9A-Fa-f]+)", extractor_output, "stack-top word"
        ),
        "stack_reserve_word": capture(
            r"stackTopBase2\s+(0x[0-9A-Fa-f]+)", extractor_output, "stack-reserve word"
        ),
        "heap_base": capture(
            r"heap base (0x[0-9A-Fa-f]+)", extractor_output, "heap base"
        ),
        "heap_size_store": capture(
            r"heapSizePtr\s+(0x[0-9A-Fa-f]+)", extractor_output, "heap-size store"
        ),
        "heap_base_store": capture(
            r"heapBasePtr\s+(0x[0-9A-Fa-f]+)", extractor_output, "heap-base store"
        ),
        "global_pointer": capture(
            r"\| gp=(0x[0-9A-Fa-f]+)", extractor_output, "global pointer"
        ),
        "libc_init": capture(
            r"\| libcInit (0x[0-9A-Fa-f]+)", extractor_output, "libcInit"
        ),
        "game_main": game_main,
        "crt0_entry": int(want["entry"]),
        "resident_begin": text_begin & 0x1FFFFFFF,
        "resident_end": text_end & 0x1FFFFFFF,
        "stack_bias_declared": 1,
        "stack_bias": capture(r"\bbias (-?[0-9]+)", extractor_output, "stack bias"),
        "vsync_begin": vsync_begin,
        "vsync_end": vsync_end,
        "platform_vsync": vsync_begin,
        "platform_window_begin": vsync_begin,
        "platform_window_end": vsync_end,
    }


def compare(expected: dict[str, int], actual: dict[str, int]) -> list[str]:
    missing = sorted(expected.keys() - actual.keys())
    extra = sorted(actual.keys() - expected.keys())
    failures = [f"missing runtime fact {name}" for name in missing]
    failures.extend(f"unexpected runtime fact {name}" for name in extra)
    failures.extend(
        f"{name}: executable says 0x{value & 0xFFFFFFFF:08X}, runtime ships 0x{actual[name] & 0xFFFFFFFF:08X}"
        for name, value in expected.items()
        if name in actual and actual[name] != value
    )
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", required=True, help="directory below titles/")
    parser.add_argument(
        "--exe", type=pathlib.Path, required=True, help="verified PS-X EXE"
    )
    parser.add_argument("--extractor", required=True, help="built psxport crt0_extract")
    parser.add_argument(
        "--runtime-facts", required=True, help="built title runtime facts tool"
    )
    parser.add_argument(
        "--selftest", action="store_true", help="also prove one altered fact disagrees"
    )
    args = parser.parse_args()

    try:
        manifest = verify_executable.load_manifest(
            ROOT / "titles" / args.title / "executable.json"
        )
        identity_failures = verify_executable.check(manifest, args.exe, False)
        if identity_failures:
            raise Refused(
                f"executable identity disagrees on {len(identity_failures)} tracked fact(s)"
            )
        expected = expected_facts(manifest, run([args.extractor, str(args.exe)]))
        actual = parse_runtime_facts(run([args.runtime_facts]))
        failures = compare(expected, actual)
        if failures:
            for failure in failures:
                print(f"MISMATCH: {failure}")
            return 1
        print(
            f"MATCH: {len(expected)}/{len(expected)} GuestProgramImage facts agree with the real executable"
        )
        if expected["game_main"]:
            print(f"game_main=0x{expected['game_main']:08X} is a tracked executable-call boundary")
        else:
            print("game_main=0 is an explicit unmeasured frontier, not a guessed address")

        if args.selftest:
            altered = dict(actual)
            altered["global_pointer"] ^= 4
            control = compare(expected, altered)
            if len(control) != 1 or not control[0].startswith("global_pointer:"):
                print(
                    "FAIL: altered-runtime control did not produce one named disagreement"
                )
                return 1
            print("PASS: altered-runtime control produced 1 named disagreement")
        return 0
    except (OSError, Refused) as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

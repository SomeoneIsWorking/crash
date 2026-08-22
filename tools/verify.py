#!/usr/bin/env python3
"""Configure, build, and test Crash's authoritative Clang verification tree."""

from __future__ import annotations

import argparse
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_BUILD = ROOT / "scratch" / "build-clang"


class Refused(RuntimeError):
    """A required verifier command could not be executed."""


def run(command: list[str], *, environment: dict[str, str] | None = None) -> int:
    print("+ " + " ".join(command), flush=True)
    try:
        result = subprocess.run(
            command,
            cwd=ROOT,
            env=environment,
            check=False,
        )
    except OSError as exc:
        raise Refused(f"could not execute {command[0]}: {exc}") from exc
    return result.returncode


def verify(build: pathlib.Path, jobs: int) -> int:
    environment = dict(os.environ)
    environment["CC"] = "clang"
    environment["CXX"] = "clang++"
    configure = [
        "cmake",
        "-S",
        str(ROOT),
        "-B",
        str(build),
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
    ]
    commands = (
        (configure, environment),
        (["cmake", "--build", str(build), "--parallel", str(jobs)], None),
        (["ctest", "--test-dir", str(build), "--output-on-failure"], None),
        ([sys.executable, str(ROOT / "tools" / "psxport_sync.py"), "--check"], None),
    )
    for command, command_environment in commands:
        result = run(command, environment=command_environment)
        if result:
            return 1
    print(f"PASS: authoritative verifier completed in {build.relative_to(ROOT)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--jobs",
        type=int,
        default=max(1, os.cpu_count() or 1),
        help="parallel build jobs (default: host CPU count)",
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be at least 1")

    try:
        return verify(DEFAULT_BUILD, args.jobs)
    except Refused as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

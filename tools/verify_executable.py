#!/usr/bin/env python3
"""Compare a title's tracked executable identity with a real PS-X EXE.

Exit 0 means every declared fact matched, exit 1 means the image contradicted the manifest, and exit
2 means no valid comparison was possible. This is an identity/header gate, not a boot gate.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
import pathlib
import sys
import tempfile
from dataclasses import dataclass

ROOT = pathlib.Path(__file__).resolve().parent.parent
PSXPORT = pathlib.Path(os.environ.get("PSXPORT_DIR", ROOT / "external" / "psxport"))
RECOMP_TOOLS = PSXPORT / "tools" / "recomp"
sys.path.insert(0, str(RECOMP_TOOLS))
try:
    import psexe
except ImportError as exc:
    raise SystemExit(
        f"REFUSED: cannot import psxport's PS-X EXE loader from {RECOMP_TOOLS}; "
        "run tools/psxport_sync.py --auto or set PSXPORT_DIR"
    ) from exc


class Refused(Exception):
    """The input cannot support the requested identity claim."""


@dataclass(frozen=True)
class Measurement:
    name: str
    size: int
    sha1: str
    sha256: str
    entry: int
    gp: int
    text_address: int
    text_size: int
    stack_address: int
    stack_offset: int
    markers: tuple[str, ...]


def parse_hex(value: object, field: str) -> int:
    if not isinstance(value, str):
        raise Refused(f"manifest field {field} must be a hexadecimal string")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise Refused(f"manifest field {field} is not hexadecimal: {value!r}") from exc


def load_manifest(path: pathlib.Path) -> dict[str, object]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise Refused(f"cannot read manifest {path}: {exc}") from exc
    required = {
        "title",
        "region",
        "serial",
        "executable",
        "file_size",
        "sha1",
        "sha256",
        "header",
        "region_markers",
    }
    missing = sorted(required - manifest.keys())
    if missing:
        raise Refused(f"manifest {path} is missing {', '.join(missing)}")
    return manifest


def measure(path: pathlib.Path, markers: list[str]) -> Measurement:
    try:
        data = path.read_bytes()
        image = psexe.load(str(path))
    except (OSError, ValueError) as exc:
        raise Refused(f"cannot read a valid executable from {path}: {exc}") from exc
    if image.text_size == 0 or not image.load <= image.entry < image.text_end:
        raise Refused(
            f"invalid PS-X EXE range: entry=0x{image.entry:08X}, "
            f"text=[0x{image.load:08X},0x{image.text_end:08X})"
        )
    found = tuple(marker for marker in markers if marker.encode("ascii") in data)
    return Measurement(
        path.name,
        len(data),
        hashlib.sha1(data).hexdigest(),
        hashlib.sha256(data).hexdigest(),
        image.entry,
        image.gp,
        image.load,
        image.text_size,
        image.sp_base,
        image.sp_off,
        found,
    )


def expected(manifest: dict[str, object]) -> dict[str, object]:
    header = manifest["header"]
    markers = manifest["region_markers"]
    if not isinstance(header, dict):
        raise Refused("manifest field header must be an object")
    if (
        not isinstance(markers, list)
        or not markers
        or not all(isinstance(item, str) for item in markers)
    ):
        raise Refused("manifest field region_markers must be a non-empty string list")
    return {
        "name": manifest["executable"],
        "size": manifest["file_size"],
        "sha1": manifest["sha1"],
        "sha256": manifest["sha256"],
        "entry": parse_hex(header.get("entry"), "header.entry"),
        "gp": parse_hex(header.get("gp"), "header.gp"),
        "text_address": parse_hex(header.get("text_address"), "header.text_address"),
        "text_size": parse_hex(header.get("text_size"), "header.text_size"),
        "stack_address": parse_hex(header.get("stack_address"), "header.stack_address"),
        "stack_offset": parse_hex(header.get("stack_offset"), "header.stack_offset"),
        "markers": tuple(markers),
    }


def check(
    manifest: dict[str, object], path: pathlib.Path, verbose: bool = True
) -> list[str]:
    markers = manifest["region_markers"]
    if not isinstance(markers, list):
        raise Refused("manifest field region_markers must be a list")
    measured = measure(path, markers)
    want = expected(manifest)
    actual = vars(measured)
    failures = [
        f"{field}: manifest={value!r}, executable={actual[field]!r}"
        for field, value in want.items()
        if value != actual[field]
    ]
    if verbose:
        print(
            f"measured {measured.name}: {measured.size} bytes, sha1={measured.sha1}, "
            f"sha256={measured.sha256}"
        )
        print(
            f"header: entry=0x{measured.entry:08X}, gp=0x{measured.gp:08X}, "
            f"text=[0x{measured.text_address:08X},"
            f"0x{measured.text_address + measured.text_size:08X}) ({measured.text_size} bytes), "
            f"stack=0x{measured.stack_address + measured.stack_offset:08X}"
        )
        print(f"region markers: {len(measured.markers)}/{len(markers)} found")
        if failures:
            for failure in failures:
                print(f"MISMATCH: {failure}")
        else:
            print(
                f"MATCH: 11/11 facts agree for {manifest['title']} ({manifest['region']})"
            )
        print(
            "blind spot: identity/header only; this does not prove disc provenance, boot, gameplay, "
            "overlays, or any native implementation"
        )
    return failures


def selftest(manifest: dict[str, object], executable: pathlib.Path) -> bool:
    results: list[tuple[str, bool]] = []
    results.append(
        (
            "retail image matches shipping manifest",
            not check(manifest, executable, False),
        )
    )

    wrong = copy.deepcopy(manifest)
    wrong["sha1"] = "0" * 40
    results.append(
        ("wrong shipping hash is rejected", bool(check(wrong, executable, False)))
    )

    scratch = ROOT / "scratch"
    scratch.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="verify-executable-", dir=scratch
    ) as directory:
        fixture = pathlib.Path(directory) / executable.name
        data = bytearray(executable.read_bytes())
        data[-1] ^= 1
        fixture.write_bytes(data)
        results.append(
            ("mutated executable is rejected", bool(check(manifest, fixture, False)))
        )

        malformed = pathlib.Path(directory) / "malformed.exe"
        malformed.write_bytes(b"not a PS-X EXE")
        try:
            check(manifest, malformed, False)
            results.append(("malformed executable is refused", False))
        except Refused:
            results.append(("malformed executable is refused", True))

    for name, passed in results:
        print(f"{'PASS' if passed else 'FAIL'}: {name}")
    print(f"selftest: {sum(passed for _, passed in results)}/{len(results)} cases")
    return all(passed for _, passed in results)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--title", default="crash1", help="directory below titles/ (default: crash1)"
    )
    parser.add_argument("--exe", type=pathlib.Path, help="path to the real executable")
    actions = parser.add_mutually_exclusive_group(required=True)
    actions.add_argument(
        "--check", action="store_true", help="compare executable to manifest"
    )
    actions.add_argument(
        "--selftest", action="store_true", help="exercise match, mismatch, refusal"
    )
    args = parser.parse_args()

    manifest_path = ROOT / "titles" / args.title / "executable.json"
    try:
        manifest = load_manifest(manifest_path)
        executable = args.exe or ROOT / "scratch" / "bin" / args.title / str(
            manifest["executable"]
        )
        if args.selftest:
            return 0 if selftest(manifest, executable) else 1
        return 1 if check(manifest, executable) else 0
    except (OSError, Refused) as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

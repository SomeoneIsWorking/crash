#!/usr/bin/env python3
"""Provision Crash Bandicoot's USA boot executable from a user-supplied CHD.

Disc resolution is CLI > environment > .env > one repository-root CHD. Configured paths are
authoritative: a missing path is refused instead of falling through to a different disc. Copyrighted
inputs and extracted files remain under gitignored paths.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import types
from collections.abc import Callable, Mapping
from dataclasses import dataclass

ROOT = pathlib.Path(__file__).resolve().parent.parent
ENV_KEYS = ("PSXPORT_CRASH1_DISC", "PSXPORT_DISC")
MANIFEST = ROOT / "titles" / "crash1" / "executable.json"
OUTPUT_DIR = ROOT / "scratch" / "bin" / "crash1"


class Refused(Exception):
    """The available input cannot support a Crash 1 provisioning claim."""


@dataclass(frozen=True)
class ResolvedDisc:
    path: pathlib.Path
    source: str


def _path_from(value: str, root: pathlib.Path) -> pathlib.Path:
    path = pathlib.Path(value).expanduser()
    return path if path.is_absolute() else root / path


def _validate_disc(value: str, source: str, root: pathlib.Path) -> ResolvedDisc:
    path = _path_from(value, root)
    if not path.is_file():
        raise Refused(f"{source} names {path}, which is not a file")
    return ResolvedDisc(path.resolve(), source)


def _dotenv_values(path: pathlib.Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise Refused(f"cannot read {path}: {exc}") from exc

    values: dict[str, str] = {}
    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        key, value = stripped.split("=", 1)
        key = key.strip()
        if key not in ENV_KEYS:
            continue
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        values[key] = value
    return values


def resolve_disc(
    argument: str | None,
    *,
    root: pathlib.Path = ROOT,
    environ: Mapping[str, str] = os.environ,
) -> ResolvedDisc:
    """Resolve one disc without falling through an invalid configured source."""
    if argument is not None:
        return _validate_disc(argument, "CLI argument", root)

    for key in ENV_KEYS:
        if environ.get(key):
            return _validate_disc(environ[key], f"${key}", root)

    dotenv = _dotenv_values(root / ".env")
    for key in ENV_KEYS:
        if dotenv.get(key):
            return _validate_disc(dotenv[key], f".env ({key})", root)

    dropins = sorted(root.glob("*.chd"), key=lambda item: item.name.casefold())
    if len(dropins) == 1:
        return _validate_disc(str(dropins[0]), "repository-root *.chd drop-in", root)
    if len(dropins) > 1:
        names = ", ".join(item.name for item in dropins)
        raise Refused(f"multiple repository-root CHDs are ambiguous: {names}")
    raise Refused(
        "no disc image; pass one as an argument, set PSXPORT_CRASH1_DISC or PSXPORT_DISC, "
        "put either key in .env, or place one *.chd in the repository root"
    )


def find_discdump(
    override: str | None,
    *,
    root: pathlib.Path = ROOT,
    environ: Mapping[str, str] = os.environ,
) -> pathlib.Path:
    configured = override or environ.get("PSXPORT_DISCDUMP")
    if configured:
        path = _path_from(configured, root)
        if not path.is_file() or not os.access(path, os.X_OK):
            raise Refused(f"configured discdump is not executable: {path}")
        return path.resolve()

    build_dirs = (root / "scratch" / "build-clang", root / "build")
    build_dir = next((path for path in build_dirs if (path / "CMakeCache.txt").is_file()), None)
    if build_dir is None:
        raise Refused(
            "no configured Crash build; configure scratch/build-clang with the README's Clang "
            "command before provisioning"
        )

    result = subprocess.run(
        [
            "cmake",
            "--build",
            str(build_dir),
            "--target",
            "discdump",
            "-j",
            str(os.cpu_count() or 4),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise Refused(f"could not build psxport discdump: {detail}")

    base = build_dir / "psxport_build" / "tools" / "discdump"
    for candidate in (base, base.with_suffix(".exe")):
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return candidate.resolve()
    raise Refused(f"discdump target built but produced no executable below {base.parent}")


def parse_boot_target(system_cnf: pathlib.Path) -> str:
    try:
        text = system_cnf.read_text(encoding="ascii", errors="replace")
    except OSError as exc:
        raise Refused(f"cannot read extracted {system_cnf.name}: {exc}") from exc
    match = re.search(
        r"^\s*BOOT\s*=\s*cdrom:\\+([^;\r\n]+)(?:;1)?\s*$", text, re.IGNORECASE | re.MULTILINE
    )
    if not match:
        raise Refused("SYSTEM.CNF has no supported BOOT = cdrom:\\...;1 target")
    return pathlib.PureWindowsPath(match.group(1).strip()).name


def extract_boot(discdump: pathlib.Path, disc: pathlib.Path, output: pathlib.Path) -> None:
    result = subprocess.run(
        [str(discdump), str(disc), str(output)], capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise Refused(f"discdump failed with exit {result.returncode}: {detail}")


def _identity_manifest() -> tuple[types.ModuleType, dict[str, object]]:
    sys.path.insert(0, str(ROOT / "tools"))
    import verify_executable

    return verify_executable, verify_executable.load_manifest(MANIFEST)


def expected_executable() -> str:
    _, manifest = _identity_manifest()
    name = manifest.get("executable")
    if not isinstance(name, str) or not name:
        raise Refused("executable manifest has no usable executable name")
    return name


def verify_identity(executable: pathlib.Path) -> list[str]:
    verify_executable, manifest = _identity_manifest()
    return verify_executable.check(manifest, executable)


def provision(
    disc: pathlib.Path,
    discdump: pathlib.Path,
    *,
    output_dir: pathlib.Path = OUTPUT_DIR,
    extract: Callable[[pathlib.Path, pathlib.Path, pathlib.Path], None] = extract_boot,
    identity_check: Callable[[pathlib.Path], list[str]] = verify_identity,
) -> pathlib.Path:
    """Extract, validate, then publish the boot executable and SYSTEM.CNF."""
    output_dir.mkdir(parents=True, exist_ok=True)
    staging_parent = output_dir.parent
    with tempfile.TemporaryDirectory(prefix="crash1-provision-", dir=staging_parent) as temporary:
        staging = pathlib.Path(temporary)
        extract(discdump, disc, staging)
        system_cnf = staging / "SYSTEM.CNF"
        boot_target = parse_boot_target(system_cnf)
        executable_name = expected_executable()
        if boot_target.casefold() != executable_name.casefold():
            raise Refused(f"SYSTEM.CNF boots {boot_target!r}, expected {executable_name!r}")

        executable = staging / executable_name
        if not executable.is_file():
            raise Refused(f"discdump did not extract {executable_name}")
        failures = identity_check(executable)
        if failures:
            raise Refused(f"executable identity disagrees on {len(failures)} tracked fact(s)")

        destination_exe = output_dir / executable.name
        destination_cnf = output_dir / system_cnf.name
        os.replace(executable, destination_exe)
        os.replace(system_cnf, destination_cnf)
    return destination_exe


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("disc", nargs="?", help="Crash Bandicoot USA CHD")
    parser.add_argument("--discdump", help="executable psxport discdump override")
    parser.add_argument(
        "--resolve-only", action="store_true", help="print the selected disc without extracting"
    )
    args = parser.parse_args()

    try:
        resolved = resolve_disc(args.disc)
        print(f"[disc] {resolved.source}: {resolved.path}", file=sys.stderr)
        if args.resolve_only:
            print(resolved.path)
            return 0
        discdump = find_discdump(args.discdump)
        executable = provision(resolved.path, discdump)
        print("MATCH: SYSTEM.CNF boot target and executable identity agree")
        print(f"provisioned {executable.relative_to(ROOT)}")
        print(
            "blind spot: this proves the selected disc's boot executable identity, not boot, "
            "gameplay, overlays, or native implementation"
        )
        return 0
    except (OSError, Refused) as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

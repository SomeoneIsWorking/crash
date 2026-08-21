#!/usr/bin/env python3
"""Emit and compare Crash 1's resident recompile at its first executed call boundary.

The port side executes psxport's generated C from the retail USA executable. The reference side
executes the same input bytes in the independent Mednafen CPU oracle. The symbolic crt0 decoder
selects the expected first-call target, but it does not supply either side's register values.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass

ROOT = pathlib.Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))
import verify_executable

PSXPORT = pathlib.Path(os.environ.get("PSXPORT_DIR", ROOT / "external" / "psxport"))
EMITTER = PSXPORT / "tools" / "recomp" / "emit.py"
MANIFEST = ROOT / "titles" / "crash1" / "executable.json"
SEEDS = ROOT / "titles" / "crash1" / "recomp_seeds.json"
DEFAULT_EXE = ROOT / "scratch" / "bin" / "crash1" / "SCUS_949.00"
DEFAULT_GENERATED = ROOT / "generated" / "crash1"
DEFAULT_BUILD = ROOT / "build"
RAW = ROOT / "scratch" / "raw" / "crash1-recomp"
COUNT_RE = re.compile(
    r"\[func\] functions: (?P<seeds>\d+) seeds -> (?P<functions>\d+) recompiled"
)
LIBC_INIT_RE = re.compile(r"^\s+libcInit\s+(0x[0-9A-Fa-f]+)\s*$", re.MULTILINE)
STATE_HEADER_RE = re.compile(
    r"^# (?P<tag>[A-Z-]+)-REGS(?: step=\d+)? pc=0x(?P<pc>[0-9A-Fa-f]+)$"
)
STATE_REG_RE = re.compile(
    r"^# (?P<tag>[A-Z-]+)-REG (?P<name>[a-z0-9]+)=0x(?P<value>[0-9A-Fa-f]+)$"
)
REGISTER_NAMES = (
    "at",
    "v0",
    "v1",
    "a0",
    "a1",
    "a2",
    "a3",
    "t0",
    "t1",
    "t2",
    "t3",
    "t4",
    "t5",
    "t6",
    "t7",
    "s0",
    "s1",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "t8",
    "t9",
    "k0",
    "k1",
    "gp",
    "sp",
    "fp",
    "ra",
    "lo",
    "hi",
)


class Refused(RuntimeError):
    """The requested evidence cannot support a comparison."""


@dataclass(frozen=True)
class Emission:
    seeds: int
    functions: int
    version: str


@dataclass(frozen=True)
class State:
    pc: int
    registers: dict[str, int]


def run(
    command: list[str], *, environment: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            command,
            cwd=ROOT,
            env=environment,
            text=True,
            capture_output=True,
            check=False,
        )
    except OSError as exc:
        raise Refused(f"could not execute {command[0]}: {exc}") from exc


def require_executable(path: pathlib.Path) -> verify_executable.Measurement:
    manifest = verify_executable.load_manifest(MANIFEST)
    failures = verify_executable.check(manifest, path, False)
    if failures:
        raise Refused(
            "retail executable contradicts the tracked identity:\n"
            + "\n".join(failures)
        )
    return verify_executable.measure(path, list(manifest["region_markers"]))


def invoke_emitter(
    executable: pathlib.Path, seeds: pathlib.Path, output: pathlib.Path
) -> subprocess.CompletedProcess[str]:
    output.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ)
    environment["PSXPORT_SHARDS"] = "1"
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    environment.pop("PSXPORT_USE_GHIDRA", None)
    return run(
        [
            sys.executable,
            "-B",
            str(EMITTER),
            str(executable),
            str(output / "main.c"),
            "--seeds",
            str(seeds),
        ],
        environment=environment,
    )


def emit(
    executable: pathlib.Path, seeds: pathlib.Path, output: pathlib.Path
) -> Emission:
    identity = require_executable(executable)
    result = invoke_emitter(executable, seeds, output)
    combined = result.stdout + result.stderr
    if result.returncode:
        raise Refused(
            f"emitter refused with exit {result.returncode}:\n{combined.rstrip()}"
        )
    count = COUNT_RE.search(combined)
    if count is None:
        raise Refused("emitter succeeded without a seed/function denominator")
    seed_count = int(count.group("seeds"))
    function_count = int(count.group("functions"))
    if seed_count <= 0 or function_count < seed_count:
        raise Refused(
            f"invalid discovery denominator: {seed_count} seeds -> {function_count} functions"
        )

    try:
        declarations = (output / "rec_decls.h").read_text(encoding="utf-8")
        sources = (output / "rec_sources.cmake").read_text(encoding="utf-8")
        overlays = (output / "overlay_table.c").read_text(encoding="utf-8")
        version = (output / ".recomp_version").read_text(encoding="utf-8").strip()
    except OSError as exc:
        raise Refused(f"emitter omitted a required generated interface: {exc}") from exc
    required_declarations = (
        f"void func_{identity.entry:08X}(Core*);",
        "void main_dispatch(Core* c, uint32_t addr);",
        "void shard_set_override(uint32_t addr, OverrideFn fn);",
    )
    missing = [
        declaration
        for declaration in required_declarations
        if declaration not in declarations
    ]
    if missing:
        raise Refused("generated declarations omit " + ", ".join(missing))
    for source in ("overlay_table.c", "shard_0.c", "shard_disp.c"):
        if source not in sources:
            raise Refused(f"generated source manifest omits {source}")
    if "const int g_rec_overlay_count = 0;" not in overlays:
        raise Refused(
            "generated overlay table does not report zero configured overlays"
        )
    if not version:
        raise Refused("generated substrate has an empty version stamp")
    return Emission(seed_count, function_count, version)


def tool_path(build: pathlib.Path, relative: str) -> pathlib.Path:
    path = build / "psxport_build" / relative
    if not path.is_file():
        raise Refused(f"required built tool is absent: {path}")
    return path


def symbolic_boundary(executable: pathlib.Path, build: pathlib.Path) -> int:
    extractor = tool_path(build, "tools/crt0_extract")
    result = run([str(extractor), str(executable)])
    if result.returncode:
        raise Refused(
            f"crt0_extract refused with exit {result.returncode}:\n{result.stderr.rstrip()}"
        )
    match = LIBC_INIT_RE.search(result.stdout)
    if match is None:
        raise Refused("crt0_extract reported no unique libcInit boundary")
    return int(match.group(1), 16)


def parse_state(text: str, tag: str) -> State:
    pc: int | None = None
    registers: dict[str, int] = {}
    for line in text.splitlines():
        header = STATE_HEADER_RE.match(line)
        if header and header.group("tag") == tag:
            if pc is not None:
                raise Refused(f"trace has more than one {tag} register block")
            pc = int(header.group("pc"), 16)
        register = STATE_REG_RE.match(line)
        if register and register.group("tag") == tag:
            name = register.group("name")
            if name in registers:
                raise Refused(f"trace repeats {tag} register {name}")
            registers[name] = int(register.group("value"), 16)
    if pc is None:
        raise Refused(f"trace has no {tag} register block")
    missing = sorted(set(REGISTER_NAMES) - registers.keys())
    extra = sorted(registers.keys() - set(REGISTER_NAMES))
    if missing or extra:
        raise Refused(
            f"{tag} register denominator changed: missing={missing}, extra={extra}"
        )
    return State(pc, registers)


def compare_states(reference: State, port: State) -> list[str]:
    mismatches = []
    if reference.pc != port.pc:
        mismatches.append(f"pc: oracle=0x{reference.pc:08X}, port=0x{port.pc:08X}")
    for name in REGISTER_NAMES:
        if reference.registers[name] != port.registers[name]:
            mismatches.append(
                f"{name}: oracle=0x{reference.registers[name]:08X}, port=0x{port.registers[name]:08X}"
            )
    return mismatches


def capture_states(
    executable: pathlib.Path, build: pathlib.Path, runner: pathlib.Path, steps: int
) -> tuple[int, State, State]:
    identity = require_executable(executable)
    boundary = symbolic_boundary(executable, build)
    oracle = tool_path(build, "tools/oracle/oracle_trace")
    if not runner.is_file():
        raise Refused(f"port boundary runner is absent: {runner}")
    RAW.mkdir(parents=True, exist_ok=True)
    oracle_trace = RAW / "oracle-call.txt"
    oracle_result = run(
        [
            str(oracle),
            str(executable),
            "--steps",
            str(steps),
            "--capture-first-call",
            "--summary-only",
            "--out",
            str(oracle_trace),
        ]
    )
    if oracle_result.returncode:
        raise Refused(
            f"oracle_trace refused with exit {oracle_result.returncode}:\n{oracle_result.stderr.rstrip()}"
        )
    reference = parse_state(oracle_trace.read_text(encoding="utf-8"), "CALL-BOUNDARY")
    if reference.pc != boundary:
        raise Refused(
            f"independent oracle first call 0x{reference.pc:08X} disagrees with symbolic boundary 0x{boundary:08X}"
        )

    port_result = run(
        [str(runner), str(executable), f"0x{identity.entry:08X}", f"0x{boundary:08X}"]
    )
    (RAW / "port-call.txt").write_text(port_result.stdout, encoding="utf-8")
    if port_result.returncode:
        raise Refused(
            f"port runner refused with exit {port_result.returncode}:\n{port_result.stderr.rstrip()}"
        )
    port = parse_state(port_result.stdout, "PORT-CALL-BOUNDARY")
    return boundary, reference, port


def check_comparison(
    executable: pathlib.Path, build: pathlib.Path, runner: pathlib.Path, steps: int
) -> tuple[State, State]:
    boundary, reference, port = capture_states(executable, build, runner, steps)
    mismatches = compare_states(reference, port)
    if mismatches:
        raise Refused("port/oracle boundary disagreement:\n" + "\n".join(mismatches))
    total = 1 + len(REGISTER_NAMES)
    print(f"PASS: {total}/{total} state fields agree at first call 0x{boundary:08X}")
    print(f"traces: {RAW / 'oracle-call.txt'} and {RAW / 'port-call.txt'}")
    return reference, port


def selftest(
    executable: pathlib.Path, build: pathlib.Path, runner: pathlib.Path, steps: int
) -> bool:
    RAW.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="bootstrap-", dir=RAW) as temporary:
        measurement = emit(executable, SEEDS, pathlib.Path(temporary))
    print(
        "PASS positive emission: "
        f"{measurement.seeds} binary-rooted seeds -> {measurement.functions} functions; "
        f"0 overlays; version {measurement.version}"
    )

    with tempfile.TemporaryDirectory(prefix="refusal-", dir=RAW) as temporary:
        directory = pathlib.Path(temporary)
        bad_seeds = directory / "outside-text.json"
        bad_seeds.write_text('{"main": ["0x90000000"]}\n', encoding="utf-8")
        result = invoke_emitter(executable, bad_seeds, directory / "generated")
        combined = result.stdout + result.stderr
        if result.returncode == 0 or "seed(s) outside the module text" not in combined:
            raise Refused("out-of-text seed was not refused by the production emitter")
    print("PASS negative emission: out-of-text seed refused before generation")

    reference, port = check_comparison(executable, build, runner, steps)
    altered_registers = dict(port.registers)
    altered_registers["a0"] ^= 1
    mismatches = compare_states(reference, State(port.pc, altered_registers))
    if len(mismatches) != 1 or not mismatches[0].startswith("a0:"):
        raise Refused(
            "production comparator did not detect the opposite-answer a0 mutation"
        )
    print(
        "PASS negative comparison: one altered port register produced one named mismatch"
    )
    print("SELFTEST 4/4")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=pathlib.Path, default=DEFAULT_EXE)
    parser.add_argument("--build", type=pathlib.Path, default=DEFAULT_BUILD)
    parser.add_argument("--runner", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, default=DEFAULT_GENERATED)
    parser.add_argument("--steps", type=int, default=100000)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--emit", action="store_true")
    action.add_argument("--compare", action="store_true")
    action.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    runner = args.runner or args.build / "crash1_recomp_boundary"
    try:
        if args.emit:
            measurement = emit(args.exe, SEEDS, args.output)
            print(
                f"PASS: emitted {measurement.seeds} binary-rooted seeds -> "
                f"{measurement.functions} functions; 0 overlays; version {measurement.version}"
            )
        elif args.selftest:
            selftest(args.exe, args.build, runner, args.steps)
        else:
            check_comparison(args.exe, args.build, runner, args.steps)
        return 0
    except (OSError, Refused, verify_executable.Refused) as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

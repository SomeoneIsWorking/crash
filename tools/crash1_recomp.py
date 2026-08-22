#!/usr/bin/env python3
"""Emit and compare Crash 1's resident recompile through its first BIOS boundary.

The port side executes psxport's generated C from the retail USA executable. The reference side
executes the same input bytes in the independent Mednafen CPU oracle. The symbolic crt0 decoder
selects only the expected first-call target. Canonical ordinal capture in oracle_trace independently
selects each actual call target and supplies every reference register file. The first eight calls
cover the resident entry path through the last executed ``jal`` before the independent CPU leaves
mapped executable text for the BIOS exception vector.
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
    r"^# (?P<tag>[A-Z-]+)-REGS(?: step=(?P<step>\d+))? pc=0x(?P<pc>[0-9A-Fa-f]+)$"
)
STATE_REG_RE = re.compile(
    r"^# (?P<tag>[A-Z-]+)-REG (?P<name>[a-z0-9]+)=0x(?P<value>[0-9A-Fa-f]+)$"
)
TEXT_EXIT_RE = re.compile(
    r"^# LEFT THE MAPPED TEXT at step (?P<step>\d+): "
    r"pc=0x(?P<pc>[0-9A-Fa-f]+) is outside "
)
ENTER_CRITICAL_RE = re.compile(
    r"^# PORT-ENTER-CRITICAL boundary=0x(?P<boundary>[0-9A-Fa-f]+) "
    r"selector=0x(?P<selector>[0-9A-Fa-f]+)$",
    re.MULTILINE,
)
ENTER_CRITICAL_V0_RE = re.compile(
    r"^# PORT-ENTER-CRITICAL v0=0x(?P<v0>[0-9A-Fa-f]+)$", re.MULTILINE
)
ENTER_CRITICAL_IRQ_RE = re.compile(
    r"^# PORT-ENTER-CRITICAL irq-before=(?P<before>\d+) irq-after=(?P<after>\d+)$",
    re.MULTILINE,
)
ENTER_CRITICAL_STATUS_RE = re.compile(
    r"^# PORT-ENTER-CRITICAL status-before=0x(?P<before>[0-9A-Fa-f]+) "
    r"status-after=0x(?P<after>[0-9A-Fa-f]+)$",
    re.MULTILINE,
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
CALL_ORDINALS = tuple(range(1, 9))


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


@dataclass(frozen=True)
class CallBoundary:
    ordinal: int
    step: int
    state: State


@dataclass(frozen=True)
class EnterCriticalBoundary:
    address: int
    selector: int
    return_value: int
    irq_before: int
    irq_after: int
    status_before: int
    status_after: int


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


def parse_call_boundary(text: str, ordinal: int) -> CallBoundary:
    state = parse_state(text, "CALL-BOUNDARY")
    headers = [
        match
        for line in text.splitlines()
        if (match := STATE_HEADER_RE.match(line))
        and match.group("tag") == "CALL-BOUNDARY"
    ]
    if len(headers) != 1 or headers[0].group("step") is None:
        raise Refused(
            f"oracle call {ordinal} has no unique canonical step-bearing boundary block"
        )
    return CallBoundary(ordinal, int(headers[0].group("step")), state)


def parse_text_exit(text: str) -> tuple[int, int]:
    matches = [match for line in text.splitlines() if (match := TEXT_EXIT_RE.match(line))]
    if len(matches) != 1:
        raise Refused(f"oracle trace has {len(matches)} mapped-text exit boundaries, expected 1")
    return int(matches[0].group("step")), int(matches[0].group("pc"), 16)


def parse_enter_critical(text: str) -> EnterCriticalBoundary:
    header = ENTER_CRITICAL_RE.search(text)
    result = ENTER_CRITICAL_V0_RE.search(text)
    irq = ENTER_CRITICAL_IRQ_RE.search(text)
    status = ENTER_CRITICAL_STATUS_RE.search(text)
    if header is None or result is None or irq is None or status is None:
        raise Refused("port runner omitted the complete EnterCriticalSection boundary block")
    return EnterCriticalBoundary(
        int(header.group("boundary"), 16),
        int(header.group("selector"), 16),
        int(result.group("v0"), 16),
        int(irq.group("before")),
        int(irq.group("after")),
        int(status.group("before"), 16),
        int(status.group("after"), 16),
    )


def require_unique_call_target(observed_targets: set[int], call: CallBoundary) -> None:
    if call.state.pc in observed_targets:
        raise Refused(
            f"oracle call {call.ordinal} repeats target 0x{call.state.pc:08X}; "
            "the target-override runner cannot distinguish repeated call ordinals"
        )
    observed_targets.add(call.state.pc)


def capture_port_state(
    executable: pathlib.Path,
    runner: pathlib.Path,
    entry: int,
    boundary: int,
    ordinal: int,
) -> State:
    port_result = run(
        [str(runner), str(executable), f"0x{entry:08X}", f"0x{boundary:08X}"]
    )
    port_trace = RAW / f"port-call-{ordinal}.txt"
    port_trace.write_text(port_result.stdout, encoding="utf-8")
    if port_result.returncode:
        raise Refused(
            f"port runner refused at call {ordinal} with exit {port_result.returncode}:\n"
            f"{port_result.stderr.rstrip()}"
        )
    return parse_state(port_result.stdout, "PORT-CALL-BOUNDARY")


def capture_enter_critical(
    executable: pathlib.Path, runner: pathlib.Path, entry: int, boundary: int
) -> EnterCriticalBoundary:
    result = run(
        [
            str(runner),
            str(executable),
            f"0x{entry:08X}",
            f"0x{boundary:08X}",
            "--execute-enter-critical",
        ]
    )
    trace = RAW / "port-enter-critical.txt"
    trace.write_text(result.stdout, encoding="utf-8")
    if result.returncode:
        raise Refused(
            "port runner refused the measured EnterCriticalSection boundary with exit "
            f"{result.returncode}:\n{result.stderr.rstrip()}"
        )
    return parse_enter_critical(result.stdout)


def capture_states(
    executable: pathlib.Path, build: pathlib.Path, runner: pathlib.Path, steps: int
) -> list[tuple[CallBoundary, State]]:
    identity = require_executable(executable)
    first_symbolic_boundary = symbolic_boundary(executable, build)
    oracle = tool_path(build, "tools/oracle/oracle_trace")
    if not runner.is_file():
        raise Refused(f"port boundary runner is absent: {runner}")
    RAW.mkdir(parents=True, exist_ok=True)
    comparisons = []
    observed_targets: set[int] = set()
    for ordinal in CALL_ORDINALS:
        oracle_trace = RAW / f"oracle-call-{ordinal}.txt"
        result = run(
            [
                str(oracle),
                str(executable),
                "--steps",
                str(steps),
                "--capture-call",
                str(ordinal),
                "--summary-only",
                "--out",
                str(oracle_trace),
            ]
        )
        if result.returncode:
            raise Refused(
                f"oracle_trace call {ordinal} refused with exit {result.returncode}:\n"
                f"{result.stderr.rstrip()}"
            )
        call = parse_call_boundary(oracle_trace.read_text(encoding="utf-8"), ordinal)
        require_unique_call_target(observed_targets, call)
        if ordinal == 1 and call.state.pc != first_symbolic_boundary:
            raise Refused(
                f"independent oracle first call 0x{call.state.pc:08X} disagrees with symbolic "
                f"boundary 0x{first_symbolic_boundary:08X}"
            )
        port = capture_port_state(
            executable, runner, identity.entry, call.state.pc, call.ordinal
        )
        comparisons.append((call, port))
    return comparisons


def check_comparison(
    executable: pathlib.Path, build: pathlib.Path, runner: pathlib.Path, steps: int
) -> list[tuple[CallBoundary, State]]:
    comparisons = capture_states(executable, build, runner, steps)
    total = 1 + len(REGISTER_NAMES)
    for call, port in comparisons:
        mismatches = compare_states(call.state, port)
        if mismatches:
            raise Refused(
                f"port/oracle disagreement at call {call.ordinal} step {call.step}:\n"
                + "\n".join(mismatches)
            )
        print(
            f"PASS: call {call.ordinal} at step {call.step}, target 0x{call.state.pc:08X}: "
            f"{total}/{total} state fields agree"
        )
    executed_bodies = len(CALL_ORDINALS)
    executable_addresses = executed_bodies + 1
    print(
        f"PASS execution denominator: {executable_addresses} executable-proven addresses "
        f"among the emitted candidates ({executed_bodies} generated bodies executed, then "
        "1 observed call target)"
    )
    last_ordinal = CALL_ORDINALS[-1]
    print(
        f"traces: {RAW / f'oracle-call-{last_ordinal}.txt'} and "
        f"{RAW / f'port-call-{last_ordinal}.txt'}"
    )
    return comparisons


def check_enter_critical_boundary(
    executable: pathlib.Path,
    runner: pathlib.Path,
    entry: int,
    calls: list[tuple[CallBoundary, State]],
) -> None:
    last_call = calls[-1][0]
    oracle_trace = (RAW / f"oracle-call-{last_call.ordinal}.txt").read_text(
        encoding="utf-8"
    )
    exit_step, exit_pc = parse_text_exit(oracle_trace)
    if exit_pc != 0xBFC00180 or exit_step != last_call.step + 2:
        raise Refused(
            "oracle did not enter the boot exception vector exactly two instructions after "
            f"call {last_call.ordinal}: call-step={last_call.step}, "
            f"exit-step={exit_step}, exit-pc=0x{exit_pc:08X}"
        )
    print(
        "PASS oracle syscall boundary: call 8 executes addiu-a0-1/syscall-0, then "
        f"vectors to 0x{exit_pc:08X} at step {exit_step}"
    )

    port = capture_enter_critical(executable, runner, entry, last_call.state.pc)
    if port.address != last_call.state.pc:
        raise Refused(
            f"port captured syscall wrapper 0x{port.address:08X}, expected oracle call target "
            f"0x{last_call.state.pc:08X}"
        )
    expected_status = port.status_before & ~1
    if (
        port.selector != 1
        or port.return_value != 1
        or port.irq_before != 1
        or port.irq_after != 0
        or port.status_after != expected_status
    ):
        raise Refused(
            "port EnterCriticalSection semantics disagree: "
            f"selector={port.selector}, v0={port.return_value}, "
            f"irq={port.irq_before}->{port.irq_after}, "
            f"status=0x{port.status_before:08X}->0x{port.status_after:08X}"
        )
    print(
        "PASS port syscall boundary: generated wrapper selected EnterCriticalSection; "
        "shipping HLE returned prior IRQ=1 and disabled IRQ delivery"
    )

    wrong_boundary = calls[-2][0].state.pc
    negative = run(
        [
            str(runner),
            str(executable),
            f"0x{entry:08X}",
            f"0x{wrong_boundary:08X}",
            "--execute-enter-critical",
        ]
    )
    if (
        negative.returncode != 2
        or "is not Crash 1's measured addiu-a0-1/syscall-0 wrapper"
        not in negative.stderr
    ):
        raise Refused(
            "port syscall discriminator accepted a different execution-proven function "
            f"0x{wrong_boundary:08X}"
        )
    print(
        "PASS negative syscall boundary: a different execution-proven function was refused "
        "before EnterCriticalSection execution"
    )


def selftest(
    executable: pathlib.Path, build: pathlib.Path, runner: pathlib.Path, steps: int
) -> bool:
    RAW.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="bootstrap-", dir=RAW) as temporary:
        measurement = emit(executable, SEEDS, pathlib.Path(temporary))
    print(
        "PASS positive emission: "
        f"{measurement.seeds} static candidate seeds -> {measurement.functions} emitted candidates; "
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

    comparisons = check_comparison(executable, build, runner, steps)
    last_call, last_port = comparisons[-1]
    check_enter_critical_boundary(
        executable, runner, require_executable(executable).entry, comparisons
    )

    before_last_trace = RAW / "oracle-before-last-call.txt"
    before_last_result = run(
        [
            str(tool_path(build, "tools/oracle/oracle_trace")),
            str(executable),
            "--steps",
            str(last_call.step),
            "--capture-call",
            str(last_call.ordinal),
            "--summary-only",
            "--out",
            str(before_last_trace),
        ]
    )
    if before_last_result.returncode != 2:
        raise Refused(
            f"short real-oracle control did not refuse at missing call {last_call.ordinal}: "
            f"exit={before_last_result.returncode}, stderr={before_last_result.stderr.rstrip()}"
        )
    short_trace = before_last_trace.read_text(encoding="utf-8")
    reached = last_call.ordinal - 1
    if (
        f"reached {reached} of {last_call.ordinal} requested executed jal call boundary/boundaries"
        not in before_last_result.stderr
        or "CALL-BOUNDARY-REGS" in short_trace
    ):
        raise Refused(
            f"short oracle trace did not report a clean {reached}/{last_call.ordinal} denominator "
            "with no boundary block"
        )
    print(
        f"PASS negative call denominator: trace ending before call {last_call.ordinal} "
        f"refused {reached}/{last_call.ordinal}"
    )

    try:
        require_unique_call_target({last_call.state.pc}, last_call)
    except Refused as exc:
        if "cannot distinguish repeated call ordinals" not in str(exc):
            raise Refused("repeated-target refusal named the wrong cause") from exc
    else:
        raise Refused(
            "target-override runner accepted an ambiguous repeated call target"
        )
    print(
        "PASS negative runner boundary: repeated call target refused as ordinal-ambiguous"
    )

    altered_registers = dict(last_port.registers)
    altered_registers["a0"] ^= 1
    mismatches = compare_states(last_call.state, State(last_port.pc, altered_registers))
    if len(mismatches) != 1 or not mismatches[0].startswith("a0:"):
        raise Refused(
            "production comparator did not detect the opposite-answer a0 mutation"
        )
    print(
        "PASS negative comparison: one altered port register produced one named mismatch"
    )
    print("SELFTEST 12/12")
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
                f"PASS: emitted {measurement.seeds} static candidate seeds -> "
                f"{measurement.functions} emitted candidates; 0 overlays; version {measurement.version}"
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

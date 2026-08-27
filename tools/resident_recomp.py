#!/usr/bin/env python3
"""Emit and compare a Crash title's resident recompile through its first BIOS boundary.

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
DEFAULT_BUILD = ROOT / "build"
COUNT_RE = re.compile(
    r"\[func\] functions: (?P<seeds>\d+) seeds -> (?P<functions>\d+) recompiled"
)
LIBC_INIT_RE = re.compile(r"^\s+libcInit\s+(0x[0-9A-Fa-f]+)\s*$", re.MULTILINE)
STATE_HEADER_RE = re.compile(
    r"^# (?P<tag>[A-Z0-9_-]+)-REGS(?: step=(?P<step>\d+))? pc=0x(?P<pc>[0-9A-Fa-f]+)$"
)
STATE_REG_RE = re.compile(
    r"^# (?P<tag>[A-Z0-9_-]+)-REG (?P<name>[a-z0-9]+)=0x(?P<value>[0-9A-Fa-f]+)$"
)
TEXT_EXIT_RE = re.compile(
    r"^# LEFT THE MAPPED TEXT at step (?P<step>\d+): "
    r"pc=0x(?P<pc>[0-9A-Fa-f]+) is outside "
)
SYSCALL_EXCEPTION_RE = re.compile(
    r"^# SYSCALL-EXCEPTION vector=0x(?P<vector>[0-9A-Fa-f]+) "
    r"selector=0x(?P<selector>[0-9A-Fa-f]+) status=0x(?P<status>[0-9A-Fa-f]+) "
    r"cause=0x(?P<cause>[0-9A-Fa-f]+) epc=0x(?P<epc>[0-9A-Fa-f]+) "
    r"step=(?P<step>\d+)$",
    re.MULTILINE,
)
MODELED_SYSCALL_RETURN_RE = re.compile(
    r"^# MODELED-SYSCALL-RETURN selector=0x(?P<selector>[0-9A-Fa-f]+) "
    r"resume=0x(?P<resume>[0-9A-Fa-f]+) v0=0x(?P<v0>[0-9A-Fa-f]+) "
    r"v1=0x(?P<v1>[0-9A-Fa-f]+) status=0x(?P<status>[0-9A-Fa-f]+) "
    r"cause=0x(?P<cause>[0-9A-Fa-f]+) epc=0x(?P<epc>[0-9A-Fa-f]+) "
    r"step=(?P<step>\d+)$",
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


class Refused(RuntimeError):
    """The requested evidence cannot support a comparison."""


@dataclass(frozen=True)
class TitleSpec:
    key: str
    codeword: str
    executable_name: str
    call_ordinals: tuple[int, ...] = tuple(range(1, 9))

    @property
    def manifest(self) -> pathlib.Path:
        return ROOT / "titles" / self.key / "executable.json"

    @property
    def seeds(self) -> pathlib.Path:
        return ROOT / "titles" / self.key / "recomp_seeds.json"

    @property
    def default_executable(self) -> pathlib.Path:
        return ROOT / "scratch" / "bin" / self.key / self.executable_name

    @property
    def default_generated(self) -> pathlib.Path:
        return ROOT / "generated" / self.key

    @property
    def raw(self) -> pathlib.Path:
        return ROOT / "scratch" / "raw" / f"{self.codeword.lower()}-recomp"


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
    cause_before: int
    cause_after: int
    epc_before: int
    epc_after: int


@dataclass(frozen=True)
class SyscallException:
    vector: int
    selector: int
    status: int
    cause: int
    epc: int
    step: int


@dataclass(frozen=True)
class ModeledSyscallReturn:
    selector: int
    resume: int
    v0: int
    v1: int
    status: int
    cause: int
    epc: int
    step: int
    next_call: CallBoundary


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


def require_executable(
    path: pathlib.Path, spec: TitleSpec
) -> verify_executable.Measurement:
    manifest = verify_executable.load_manifest(spec.manifest)
    failures = verify_executable.check(manifest, path, False)
    if failures:
        raise Refused(
            "retail executable contradicts the tracked identity:\n"
            + "\n".join(failures)
        )
    return verify_executable.measure_manifest(path, manifest)


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
    executable: pathlib.Path, seeds: pathlib.Path, output: pathlib.Path, spec: TitleSpec
) -> Emission:
    identity = require_executable(executable, spec)
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
    matches = [
        match for line in text.splitlines() if (match := TEXT_EXIT_RE.match(line))
    ]
    if len(matches) != 1:
        raise Refused(
            f"oracle trace has {len(matches)} mapped-text exit boundaries, expected 1"
        )
    return int(matches[0].group("step")), int(matches[0].group("pc"), 16)


def parse_modeled_syscall_return(
    text: str,
) -> tuple[SyscallException, ModeledSyscallReturn]:
    exceptions = list(SYSCALL_EXCEPTION_RE.finditer(text))
    returns = list(MODELED_SYSCALL_RETURN_RE.finditer(text))
    if len(exceptions) != 1 or len(returns) != 1:
        raise Refused(
            "oracle modeled syscall trace must contain one exception and one return record"
        )
    exception_match = exceptions[0]
    return_match = returns[0]
    exception = SyscallException(
        int(exception_match.group("vector"), 16),
        int(exception_match.group("selector"), 16),
        int(exception_match.group("status"), 16),
        int(exception_match.group("cause"), 16),
        int(exception_match.group("epc"), 16),
        int(exception_match.group("step")),
    )
    post_state = parse_state(text, "POST-RETURN-CALL-BOUNDARY")
    post_headers = [
        match
        for line in text.splitlines()
        if (match := STATE_HEADER_RE.match(line))
        and match.group("tag") == "POST-RETURN-CALL-BOUNDARY"
    ]
    if len(post_headers) != 1 or post_headers[0].group("step") is None:
        raise Refused(
            "oracle modeled syscall trace has no unique post-return call step"
        )
    modeled = ModeledSyscallReturn(
        int(return_match.group("selector"), 16),
        int(return_match.group("resume"), 16),
        int(return_match.group("v0"), 16),
        int(return_match.group("v1"), 16),
        int(return_match.group("status"), 16),
        int(return_match.group("cause"), 16),
        int(return_match.group("epc"), 16),
        int(return_match.group("step")),
        CallBoundary(
            1,
            int(post_headers[0].group("step")),
            post_state,
        ),
    )
    return exception, modeled


def parse_enter_critical(text: str, spec: TitleSpec) -> EnterCriticalBoundary:
    marker = re.escape(spec.codeword)
    header = re.search(
        rf"^# {marker}-PORT-ENTER-CRITICAL boundary=0x(?P<boundary>[0-9A-Fa-f]+) "
        rf"selector=0x(?P<selector>[0-9A-Fa-f]+)$",
        text,
        re.MULTILINE,
    )
    result = re.search(
        rf"^# {marker}-PORT-ENTER-CRITICAL v0=0x(?P<v0>[0-9A-Fa-f]+)$",
        text,
        re.MULTILINE,
    )
    irq = re.search(
        rf"^# {marker}-PORT-ENTER-CRITICAL irq-before=(?P<before>\d+) "
        rf"irq-after=(?P<after>\d+)$",
        text,
        re.MULTILINE,
    )
    status = re.search(
        rf"^# {marker}-PORT-ENTER-CRITICAL status-before=0x(?P<before>[0-9A-Fa-f]+) "
        rf"status-after=0x(?P<after>[0-9A-Fa-f]+)$",
        text,
        re.MULTILINE,
    )
    cause = re.search(
        rf"^# {marker}-PORT-ENTER-CRITICAL cause-before=0x(?P<before>[0-9A-Fa-f]+) "
        rf"cause-after=0x(?P<after>[0-9A-Fa-f]+)$",
        text,
        re.MULTILINE,
    )
    epc = re.search(
        rf"^# {marker}-PORT-ENTER-CRITICAL epc-before=0x(?P<before>[0-9A-Fa-f]+) "
        rf"epc-after=0x(?P<after>[0-9A-Fa-f]+)$",
        text,
        re.MULTILINE,
    )
    if (
        header is None
        or result is None
        or irq is None
        or status is None
        or cause is None
        or epc is None
    ):
        raise Refused(
            "port runner omitted the complete EnterCriticalSection boundary block"
        )
    return EnterCriticalBoundary(
        int(header.group("boundary"), 16),
        int(header.group("selector"), 16),
        int(result.group("v0"), 16),
        int(irq.group("before")),
        int(irq.group("after")),
        int(status.group("before"), 16),
        int(status.group("after"), 16),
        int(cause.group("before"), 16),
        int(cause.group("after"), 16),
        int(epc.group("before"), 16),
        int(epc.group("after"), 16),
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
    spec: TitleSpec,
) -> State:
    port_result = run(
        [str(runner), str(executable), f"0x{entry:08X}", f"0x{boundary:08X}"]
    )
    port_trace = spec.raw / f"port-call-{ordinal}.txt"
    port_trace.write_text(port_result.stdout, encoding="utf-8")
    if port_result.returncode:
        raise Refused(
            f"port runner refused at call {ordinal} with exit {port_result.returncode}:\n"
            f"{port_result.stderr.rstrip()}"
        )
    return parse_state(port_result.stdout, f"{spec.codeword}-PORT-CALL-BOUNDARY")


def capture_enter_critical(
    executable: pathlib.Path,
    runner: pathlib.Path,
    entry: int,
    boundary: int,
    spec: TitleSpec,
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
    trace = spec.raw / "port-enter-critical.txt"
    trace.write_text(result.stdout, encoding="utf-8")
    if result.returncode:
        raise Refused(
            "port runner refused the measured EnterCriticalSection boundary with exit "
            f"{result.returncode}:\n{result.stderr.rstrip()}"
        )
    return parse_enter_critical(result.stdout, spec)


def capture_oracle_after_enter_critical(
    executable: pathlib.Path,
    build: pathlib.Path,
    steps: int,
    ordinal: int,
    spec: TitleSpec,
) -> tuple[SyscallException, ModeledSyscallReturn]:
    trace = spec.raw / "oracle-after-enter-critical.txt"
    result = run(
        [
            str(tool_path(build, "tools/oracle/oracle_trace")),
            str(executable),
            "--steps",
            str(steps),
            "--capture-call",
            str(ordinal),
            "--model-syscall-return",
            "1:1",
            "--summary-only",
            "--out",
            str(trace),
        ]
    )
    if result.returncode:
        raise Refused(
            "oracle_trace refused the modeled EnterCriticalSection return with exit "
            f"{result.returncode}:\n{result.stderr.rstrip()}"
        )
    return parse_modeled_syscall_return(trace.read_text(encoding="utf-8"))


def capture_port_after_enter_critical(
    executable: pathlib.Path,
    runner: pathlib.Path,
    entry: int,
    syscall_boundary: int,
    next_boundary: int,
    spec: TitleSpec,
) -> tuple[EnterCriticalBoundary, State]:
    result = run(
        [
            str(runner),
            str(executable),
            f"0x{entry:08X}",
            f"0x{syscall_boundary:08X}",
            "--resume-enter-critical-to",
            f"0x{next_boundary:08X}",
        ]
    )
    trace = spec.raw / "port-after-enter-critical.txt"
    trace.write_text(result.stdout, encoding="utf-8")
    if result.returncode:
        raise Refused(
            "port runner refused the post-EnterCriticalSection boundary with exit "
            f"{result.returncode}:\n{result.stderr.rstrip()}"
        )
    return (
        parse_enter_critical(result.stdout, spec),
        parse_state(result.stdout, f"{spec.codeword}-PORT-CALL-BOUNDARY"),
    )


def capture_states(
    executable: pathlib.Path,
    build: pathlib.Path,
    runner: pathlib.Path,
    steps: int,
    spec: TitleSpec,
) -> list[tuple[CallBoundary, State]]:
    identity = require_executable(executable, spec)
    first_symbolic_boundary = symbolic_boundary(executable, build)
    oracle = tool_path(build, "tools/oracle/oracle_trace")
    if not runner.is_file():
        raise Refused(f"port boundary runner is absent: {runner}")
    spec.raw.mkdir(parents=True, exist_ok=True)
    comparisons = []
    observed_targets: set[int] = set()
    for ordinal in spec.call_ordinals:
        oracle_trace = spec.raw / f"oracle-call-{ordinal}.txt"
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
            executable, runner, identity.entry, call.state.pc, call.ordinal, spec
        )
        comparisons.append((call, port))
    return comparisons


def require_tracked_call(
    comparisons: list[tuple[CallBoundary, State]],
    runtime: dict[str, object],
    *,
    entry_key: str,
    ordinal_key: str,
    label: str,
    codeword: str,
) -> None:
    entry = int(str(runtime.get(entry_key, "0")), 0)
    ordinal = int(runtime.get(ordinal_key, 0))
    if not entry and not ordinal:
        return
    if not entry or not ordinal:
        raise Refused(
            f"tracked {label} must declare both {entry_key} and {ordinal_key}"
        )
    matches = [call for call, _ in comparisons if call.ordinal == ordinal]
    if len(matches) != 1 or matches[0].state.pc != entry:
        observed = matches[0].state.pc if len(matches) == 1 else 0
        raise Refused(
            f"tracked {label} call {ordinal} at 0x{entry:08X} "
            f"disagrees with oracle target 0x{observed:08X}"
        )
    print(
        f"PASS {codeword}: {label} call {ordinal} reached tracked entry 0x{entry:08X}"
    )


def require_tracked_bios_dispatch(
    runtime: dict[str, object], modeled: ModeledSyscallReturn, spec: TitleSpec
) -> None:
    keys = (
        "first_bios_dispatch_address",
        "first_bios_dispatch_function",
        "first_bios_dispatch_return_address",
    )
    try:
        tracked = tuple(int(str(runtime[key]), 0) for key in keys)
    except (KeyError, TypeError, ValueError) as exc:
        raise Refused(
            "tracked first BIOS dispatch must declare address, function, and return address"
        ) from exc
    observed = (
        modeled.next_call.state.pc,
        modeled.next_call.state.registers["t1"],
        modeled.next_call.state.registers["ra"],
    )
    if tracked != observed:
        raise Refused(
            "tracked first BIOS dispatch disagrees with the independent CPU: "
            f"tracked pc=0x{tracked[0]:08X} function=0x{tracked[1]:08X} ra=0x{tracked[2]:08X}; "
            f"oracle pc=0x{observed[0]:08X} function=0x{observed[1]:08X} ra=0x{observed[2]:08X}"
        )
    print(
        f"PASS {spec.codeword}: tracked first BIOS dispatch agrees at "
        f"pc=0x{tracked[0]:08X}, function=0x{tracked[1]:02X}, ra=0x{tracked[2]:08X}"
    )


def check_comparison(
    executable: pathlib.Path,
    build: pathlib.Path,
    runner: pathlib.Path,
    steps: int,
    spec: TitleSpec,
) -> list[tuple[CallBoundary, State]]:
    comparisons = capture_states(executable, build, runner, steps, spec)
    total = 1 + len(REGISTER_NAMES)
    for call, port in comparisons:
        mismatches = compare_states(call.state, port)
        if mismatches:
            raise Refused(
                f"port/oracle disagreement at call {call.ordinal} step {call.step}:\n"
                + "\n".join(mismatches)
            )
        print(
            f"PASS {spec.codeword}: call {call.ordinal} at step {call.step}, "
            f"target 0x{call.state.pc:08X}: "
            f"{total}/{total} state fields agree"
        )
    manifest = verify_executable.load_manifest(spec.manifest)
    runtime = manifest.get("runtime", {})
    if not isinstance(runtime, dict):
        raise Refused("manifest runtime facts must be an object")
    require_tracked_call(
        comparisons,
        runtime,
        entry_key="game_main_entry",
        ordinal_key="game_main_call_ordinal",
        label="game-main",
        codeword=spec.codeword,
    )
    require_tracked_call(
        comparisons,
        runtime,
        entry_key="first_syscall_entry",
        ordinal_key="first_syscall_call_ordinal",
        label="first-syscall",
        codeword=spec.codeword,
    )
    executed_bodies = len(spec.call_ordinals)
    executable_addresses = executed_bodies + 1
    print(
        f"PASS execution denominator: {executable_addresses} executable-proven addresses "
        f"among the emitted candidates ({executed_bodies} generated bodies executed, then "
        "1 observed call target)"
    )
    last_ordinal = spec.call_ordinals[-1]
    print(
        f"traces: {spec.raw / f'oracle-call-{last_ordinal}.txt'} and "
        f"{spec.raw / f'port-call-{last_ordinal}.txt'}"
    )
    return comparisons


def check_enter_critical_boundary(
    executable: pathlib.Path,
    build: pathlib.Path,
    runner: pathlib.Path,
    entry: int,
    steps: int,
    calls: list[tuple[CallBoundary, State]],
    spec: TitleSpec,
) -> None:
    last_call = calls[-1][0]
    oracle_trace = (spec.raw / f"oracle-call-{last_call.ordinal}.txt").read_text(
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

    port = capture_enter_critical(executable, runner, entry, last_call.state.pc, spec)
    if port.address != last_call.state.pc:
        raise Refused(
            f"port captured syscall wrapper 0x{port.address:08X}, expected oracle call target "
            f"0x{last_call.state.pc:08X}"
        )
    expected_status = port.status_before & ~1
    expected_cause = (port.cause_before & 0x0000FF00) | 0x20
    expected_epc = port.address + 4
    if (
        port.selector != 1
        or port.return_value != 1
        or port.irq_before != 1
        or port.irq_after != 0
        or port.status_after != expected_status
        or port.cause_after != expected_cause
        or port.epc_after != expected_epc
    ):
        raise Refused(
            "port EnterCriticalSection semantics disagree: "
            f"selector={port.selector}, v0={port.return_value}, "
            f"irq={port.irq_before}->{port.irq_after}, "
            f"status=0x{port.status_before:08X}->0x{port.status_after:08X}, "
            f"cause=0x{port.cause_before:08X}->0x{port.cause_after:08X}, "
            f"epc=0x{port.epc_before:08X}->0x{port.epc_after:08X}"
        )
    print(
        "PASS port syscall boundary: generated wrapper selected EnterCriticalSection; "
        "shipping HLE retained syscall Cause/EPC, returned prior IRQ=1, and disabled IRQ delivery"
    )

    exception, modeled = capture_oracle_after_enter_critical(
        executable, build, steps, last_call.ordinal, spec
    )
    expected_status = (exception.status & ~0x0F) | ((exception.status >> 2) & 0x0F)
    if (
        exception.vector != 0xBFC00180
        or exception.selector != 1
        or ((exception.cause >> 2) & 0x1F) != 8
        or (exception.cause >> 31) != 0
        or exception.epc != last_call.state.pc + 4
        or exception.step != exit_step
        or modeled.selector != 1
        or modeled.v0 != 1
        or modeled.resume != exception.epc + 4
        or modeled.status != expected_status
        or modeled.cause != exception.cause
        or modeled.epc != exception.epc
        or modeled.step != exception.step
    ):
        raise Refused(
            "oracle modeled syscall return contradicts the measured exception: "
            f"vector=0x{exception.vector:08X}, selector={exception.selector}, "
            f"status=0x{exception.status:08X}->0x{modeled.status:08X}, "
            f"cause=0x{exception.cause:08X}->0x{modeled.cause:08X}, "
            f"epc=0x{exception.epc:08X}->0x{modeled.epc:08X}, "
            f"resume=0x{modeled.resume:08X}"
        )
    if port.cause_after != exception.cause or port.epc_after != exception.epc:
        raise Refused(
            "shipping exception record disagrees with the independent CPU: "
            f"cause oracle=0x{exception.cause:08X} port=0x{port.cause_after:08X}, "
            f"epc oracle=0x{exception.epc:08X} port=0x{port.epc_after:08X}"
        )
    print(
        "PASS independent syscall return: Cause/EPC agree exactly; one Status mode-stack "
        f"pop resumes at 0x{modeled.resume:08X}"
    )

    manifest = verify_executable.load_manifest(spec.manifest)
    runtime = manifest.get("runtime", {})
    if not isinstance(runtime, dict):
        raise Refused("manifest runtime facts must be an object")
    require_tracked_bios_dispatch(runtime, modeled, spec)
    altered_runtime = dict(runtime)
    altered_runtime["first_bios_dispatch_function"] = "0x00000057"
    try:
        require_tracked_bios_dispatch(altered_runtime, modeled, spec)
    except Refused as exc:
        if "disagrees with the independent CPU" not in str(exc):
            raise Refused(
                "altered tracked BIOS dispatch named the wrong cause"
            ) from exc
    else:
        raise Refused(
            "tracked first BIOS dispatch accepted an altered function selector"
        )
    print(
        "PASS negative tracked BIOS dispatch: altered function selector disagreed by name"
    )

    resumed_transition, resumed_port = capture_port_after_enter_critical(
        executable,
        runner,
        entry,
        last_call.state.pc,
        modeled.next_call.state.pc,
        spec,
    )
    mismatches = compare_states(modeled.next_call.state, resumed_port)
    if mismatches:
        raise Refused(
            "port/oracle disagreement at the first post-syscall call boundary:\n"
            + "\n".join(mismatches)
        )
    if (
        resumed_transition.cause_after != exception.cause
        or resumed_transition.epc_after != exception.epc
    ):
        raise Refused(
            "resumed shipping run did not retain the proven syscall Cause/EPC record"
        )
    print(
        "PASS post-syscall boundary: independent and shipping CPUs agree "
        f"{1 + len(REGISTER_NAMES)}/{1 + len(REGISTER_NAMES)} at call target "
        f"0x{modeled.next_call.state.pc:08X}"
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
        or f"is not {spec.codeword}'s measured addiu-a0-1/syscall-0 wrapper"
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

    wrong_selector_trace = spec.raw / "oracle-wrong-syscall-selector.txt"
    wrong_selector = run(
        [
            str(tool_path(build, "tools/oracle/oracle_trace")),
            str(executable),
            "--steps",
            str(steps),
            "--model-syscall-return",
            "2:1",
            "--summary-only",
            "--out",
            str(wrong_selector_trace),
        ]
    )
    wrong_text = (
        wrong_selector_trace.read_text(encoding="utf-8")
        if wrong_selector_trace.is_file()
        else ""
    )
    if wrong_selector.returncode != 2 or "# MODELED-SYSCALL-RETURN" in wrong_text:
        raise Refused("real oracle accepted the wrong EnterCriticalSection selector")
    print(
        "PASS negative independent syscall return: wrong selector refused before resume"
    )


def selftest(
    executable: pathlib.Path,
    build: pathlib.Path,
    runner: pathlib.Path,
    steps: int,
    spec: TitleSpec,
) -> bool:
    spec.raw.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="bootstrap-", dir=spec.raw) as temporary:
        measurement = emit(executable, spec.seeds, pathlib.Path(temporary), spec)
    print(
        "PASS positive emission: "
        f"{measurement.seeds} static candidate seeds -> {measurement.functions} emitted candidates; "
        f"0 overlays; version {measurement.version}"
    )

    with tempfile.TemporaryDirectory(prefix="refusal-", dir=spec.raw) as temporary:
        directory = pathlib.Path(temporary)
        bad_seeds = directory / "outside-text.json"
        bad_seeds.write_text('{"main": ["0x90000000"]}\n', encoding="utf-8")
        result = invoke_emitter(executable, bad_seeds, directory / "generated")
        combined = result.stdout + result.stderr
        if result.returncode == 0 or "seed(s) outside the module text" not in combined:
            raise Refused("out-of-text seed was not refused by the production emitter")
    print("PASS negative emission: out-of-text seed refused before generation")

    comparisons = check_comparison(executable, build, runner, steps, spec)
    last_call, last_port = comparisons[-1]
    manifest = verify_executable.load_manifest(spec.manifest)
    runtime = manifest.get("runtime", {})
    if not isinstance(runtime, dict):
        raise Refused("manifest runtime facts must be an object")
    altered_runtime = dict(runtime)
    altered_runtime["first_syscall_entry"] = f"0x{last_call.state.pc ^ 4:08X}"
    try:
        require_tracked_call(
            comparisons,
            altered_runtime,
            entry_key="first_syscall_entry",
            ordinal_key="first_syscall_call_ordinal",
            label="first-syscall",
            codeword=spec.codeword,
        )
    except Refused as exc:
        if "disagrees with oracle target" not in str(exc):
            raise Refused("altered tracked frontier named the wrong cause") from exc
    else:
        raise Refused("tracked first-syscall comparison accepted an altered entry")
    print(
        "PASS negative tracked frontier: altered first-syscall entry disagreed by name"
    )

    check_enter_critical_boundary(
        executable,
        build,
        runner,
        require_executable(executable, spec).entry,
        steps,
        comparisons,
        spec,
    )

    before_last_trace = spec.raw / "oracle-before-last-call.txt"
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
    print("SELFTEST 18/18")
    return True


def main(spec: TitleSpec) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=pathlib.Path, default=spec.default_executable)
    parser.add_argument("--build", type=pathlib.Path, default=DEFAULT_BUILD)
    parser.add_argument("--runner", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, default=spec.default_generated)
    parser.add_argument("--steps", type=int, default=100000)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--emit", action="store_true")
    action.add_argument("--compare", action="store_true")
    action.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    runner = args.runner or args.build / f"{spec.key}_recomp_boundary"
    try:
        if args.emit:
            measurement = emit(args.exe, spec.seeds, args.output, spec)
            print(
                f"PASS: emitted {measurement.seeds} static candidate seeds -> "
                f"{measurement.functions} emitted candidates; 0 overlays; version {measurement.version}"
            )
        elif args.selftest:
            selftest(args.exe, args.build, runner, args.steps, spec)
        else:
            check_comparison(args.exe, args.build, runner, args.steps, spec)
        return 0
    except (OSError, Refused, verify_executable.Refused) as exc:
        print(f"REFUSED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(
        "resident_recomp.py is a shared implementation; use a serial-scoped title entry point"
    )

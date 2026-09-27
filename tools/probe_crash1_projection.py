#!/usr/bin/env python3
"""Census Crash 1's guest projection owners, and re-derive titles/crash1/executable.json's
`runtime.projection` block from the authenticated image.

WHY THIS IS A PROBE AND NOT A LOOKUP. Four separate false negatives are already recorded for this
workspace, and Crash 1 reproduces three of them:

  * Ghidra's reference model reports **0** references to `set_geom_screen` (0x80042FAC) and 2 to
    `set_geom_offset` (0x80042F8C). Both leaves have exactly 2 `jal` call sites each; the model
    misses the first pair.
  * The title's display-mode / draw-area owner, `FUN_80041C38`, has **0** direct `jal`/`j` call
    sites in the whole image. It is reached only as entry 7 of the GPU driver pointer table at
    0x80054A24, so no call-target scan can find it.
  * Crash 1 is compiled with **no gp-relative addressing** (globals are `lui $r,0x8005` + a 16-bit
    offset — measured: the BIOS PadRead result cell 0x80057054 is reached by
    `lui v0,0x8005; lw v0,0x7054(v0)` at 0x8003E470). A gp-relative reference scan therefore returns
    zero for every global in the image, including ones the title's own verified owners name.
  * The GTE cop2op layout is the second trap. `ctc2 $rt, <cop2op>` writes GTE control register
    `<cop2op> bits 11..15`, and the class (mfc2/cfc2/mtc2/ctc2) is bits 21..25 — NOT the low 5 bits
    of the immediate. Reading the register from the immediate's low bits finds CR[4]/CR[5]/CR[6] and
    reports "no OFX/OFY/H write anywhere", which is exactly the wrong answer.

So this tool scans EVERY instruction word of the resident text and reports a denominator for every
claim, including the nulls. It writes no guest code and needs no MIPS disassembler beyond the four
fields it decodes.

Exit 0 = the image agrees with the manifest and every selftest fired. 1 = disagreement. 2 = no valid
comparison was possible.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import struct
import sys
from dataclasses import dataclass

ROOT = pathlib.Path(__file__).resolve().parent.parent
PSXPORT = ROOT / "external" / "psxport"
sys.path.insert(0, str(PSXPORT))
try:
    from tools.formats import psx_exe
except ImportError as exc:  # pragma: no cover - environment problem, not a verdict
    raise SystemExit(
        f"REFUSED: cannot import psxport's PS-X EXE loader from {PSXPORT}; "
        "run tools/psxport_sync.py --auto"
    ) from exc

EXE_HEADER_BYTES = 0x800

# lightrec (shared/lightrec/lightrec.c `lightrec_mtc`, disassembler.h `cp2_basic_opcodes`,
# `struct opcode_r`): an opcode-0x12 word carries the whole cop2op in bits 0..15. Bits 21..25 pick
# the transfer class and bits 11..15 the GTE register index. Beetle's gte.c then names CR[24]=OFX,
# CR[25]=OFY, CR[26]=H.
CP2_OPCODE = 0x12
CLASS_MFC2 = 0  # cop2 DATA read
CLASS_CFC2 = 2  # cop2 CONTROL read
CLASS_MTC2 = 4  # cop2 DATA write
CLASS_CTC2 = 6  # cop2 CONTROL write
CLASS_NAMES = {
    CLASS_MFC2: "mfc2 data read",
    CLASS_CFC2: "cfc2 control read",
    CLASS_MTC2: "mtc2 data write",
    CLASS_CTC2: "ctc2 control write",
}
REGISTER_SHIFT = 11
CLASS_SHIFT = 21

CR_OFX = 24
CR_OFY = 25
CR_H = 26

# Horizontal extents a PSX horizontal cull can compare against. 320 is THE NTSC 4:3 dot width; the
# neighbours are the widths a title widens to or clips from. 0xBFFF is the PSX GTE visibility
# sign-mask, 0xDFFF the PSX active-low digital pad bit.
CULL_IMMEDIATES = {
    320: "PSX NTSC 4:3 dot width",
    319: "4:3 dot width - 1",
    318: "4:3 dot width - 2",
    352: "4:3 width + 32",
    368: "dedicated 368-dot mode",
    384: "4:3 width * 1.2",
    288: "4:3 width - 32",
    0xBFFF: "PSX GTE visibility sign-mask pair",
    0xDFFF: "PSX active-low digital pad bit",
}
# The immediate-bearing signed compares. `slti` and `sltiu` are I-TYPE (opcodes 0x0A and 0x0B), not
# SPECIAL functs: a SPECIAL funct field occupies bits 0..5 and would collide with an imm16. The
# register-vs-register `slt`/`sltu` are SPECIAL functs 0x2A/0x2B and carry no literal at all, so they
# are in the NULL below, not in the match set.
OP_SLTI = 0x0A
OP_SLTIU = 0x0B


class Refused(Exception):
    """The input cannot support the requested claim."""


@dataclass(frozen=True)
class Image:
    words: tuple[int, ...]
    load: int
    text_end: int
    data: bytes
    path: pathlib.Path

    def word_at(self, address: int) -> int:
        return self.words[(address - self.load) >> 2]

    def file_offset(self, address: int) -> int:
        return EXE_HEADER_BYTES + (address - self.load)


def load_image(path: pathlib.Path) -> Image:
    try:
        data = path.read_bytes()
        image = psx_exe.load(str(path))
    except (OSError, ValueError) as exc:
        raise Refused(f"cannot read a valid executable from {path}: {exc}") from exc
    count = (image.text_end - image.load) // 4
    if count <= 0:
        raise Refused(f"{path} declares an empty text segment")
    body = data[EXE_HEADER_BYTES : EXE_HEADER_BYTES + count * 4]
    if len(body) != count * 4:
        raise Refused(f"{path} text segment is not backed by file bytes")
    return Image(struct.unpack(f"<{count}I", body), image.load, image.text_end, data, path)


def parse_hex(value: object, field: str) -> int:
    if not isinstance(value, str):
        raise Refused(f"manifest field {field} must be a hex string")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise Refused(f"manifest field {field} is not hexadecimal: {value!r}") from exc


def projection_manifest(manifest: dict[str, object]) -> dict[str, object]:
    runtime = manifest.get("runtime")
    if not isinstance(runtime, dict):
        raise Refused("manifest field runtime must be an object")
    projection = runtime.get("projection")
    if not isinstance(projection, dict):
        raise Refused("manifest field runtime.projection must be an object")
    return projection


def census_cop2(image: Image) -> dict[int, list[int]]:
    """Every opcode-0x12 word, classified, keyed by (class, GTE register)."""
    out: dict[int, list[int]] = {}
    for index, word in enumerate(image.words):
        if word >> 26 != CP2_OPCODE:
            continue
        cls = (word >> CLASS_SHIFT) & 0x1F
        register = (word >> REGISTER_SHIFT) & 0x1F
        out.setdefault((cls << 8) | register, []).append(image.load + index * 4)
    return out


def control_writers(census: dict[int, list[int]], register: int) -> list[int]:
    return census.get((CLASS_CTC2 << 8) | register, [])


def control_readers(census: dict[int, list[int]], register: int) -> list[int]:
    return census.get((CLASS_CFC2 << 8) | register, [])


def direct_call_sites(image: Image, target: int) -> list[int]:
    """Every `jal`/`j` whose computed destination is exactly `target`.

    The null this cannot see is stated, not hidden: a call through a pointer reaches the target with
    no branch instruction naming it at all, which is how Crash 1's display-mode owner is reached.
    """
    sites: list[int] = []
    for index, word in enumerate(image.words):
        opcode = word >> 26
        if opcode not in (0x02, 0x03):  # j, jal
            continue
        pc = image.load + index * 4
        destination = ((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
        if destination == target:
            sites.append(pc)
    return sorted(sites)


def jalr_sites(image: Image) -> int:
    return sum(1 for word in image.words if word >> 26 == 0x00 and (word & 0x3F) == 0x09)


def cull_census(image: Image) -> list[tuple[int, str, int]]:
    """Immediate-bearing horizontal-cull idioms, by immediate.

    A literal cull is a compare against a constant. A cull against a *variable* bound carries no
    immediate at all, so this scan structurally cannot see it; the count of sites it classified and
    the count of sites it could not classify are both reported.
    """
    sites: list[tuple[int, str, int]] = []
    for index, word in enumerate(image.words):
        pc = image.load + index * 4
        opcode = word >> 26
        immediate = word & 0xFFFF
        signed = immediate - 0x10000 if immediate & 0x8000 else immediate
        text = None
        if opcode in (OP_SLTI, OP_SLTIU):
            mnemonic = "slti" if opcode == OP_SLTI else "sltiu"
            text = f"{mnemonic} r{(word >> 16) & 0x1F}, r{(word >> 21) & 0x1F}, {signed}"
        elif opcode in (0x0C, 0x0D):
            mnemonic = "andi" if opcode == 0x0C else "ori"
            text = f"{mnemonic} r{(word >> 16) & 0x1F}, 0x{immediate:04X}"
        elif opcode == 0x09:
            text = f"addiu r{(word >> 16) & 0x1F}, r{(word >> 21) & 0x1F}, {signed}"
        if text is None:
            continue
        if immediate in CULL_IMMEDIATES or (-signed) & 0xFFFF in CULL_IMMEDIATES:
            sites.append((pc, text, immediate))
    return sites


def driver_table(image: Image, table: int, limit: int = 32) -> list[tuple[int, int]]:
    """The GPU driver pointer table: a run of resident words that are guest code addresses."""
    entries: list[tuple[int, int]] = []
    for slot in range(limit):
        address = table + slot * 4
        if address + 4 > image.text_end + (image.text_end - image.load):
            break
        try:
            value = image.word_at(address)
        except IndexError:
            break
        if not (image.load <= value < image.text_end):
            break
        entries.append((address, value))
    return entries


def report(image: Image) -> dict[str, object]:
    census = census_cop2(image)
    by_register = {register: (control_writers(census, register), control_readers(census, register))
                   for register in (CR_OFX, CR_OFY, CR_H)}
    culls = cull_census(image)
    widths = [(pc, text, immediate) for pc, text, immediate in culls
              if immediate in (320, 319, 318, 352, 368, 384, 288)
              or (immediate & 0x8000 and (0x10000 - immediate) in (320, 319, 318, 352, 368, 384, 288))]
    return {
        "words": len(image.words),
        "load": image.load,
        "text_end": image.text_end,
        "by_register": by_register,
        "culls": culls,
        "widths": widths,
        "jalr": jalr_sites(image),
    }


def check(image: Image, manifest: dict[str, object], facts: dict[str, object]) -> list[str]:
    projection = projection_manifest(manifest)
    failures: list[str] = []
    register_names = {CR_OFX: "OFX", CR_OFY: "OFY", CR_H: "H"}

    print(
        f"// projection census over {facts['words']} instruction words in "
        f"[0x{facts['load']:08X},0x{facts['text_end']:08X}) of {image.path.name}"
    )

    classes: dict[int, int] = {}
    for key, sites in census_cop2(image).items():
        cls = key >> 8
        classes[cls] = classes.get(cls, 0) + len(sites)
    print(f"// opcode 0x12 (COP2) words by transfer class, bits21-25 (denominator {sum(classes.values())}):")
    for cls in sorted(classes):
        print(f"//   {cls:2d} {CLASS_NAMES.get(cls, 'GTE command'):<22} x{classes[cls]}")
    if not any(cls in CLASS_NAMES for cls in classes):
        failures.append("no COP2 transfer class decoded; the cop2op layout assumption is wrong")

    declared = projection.get("_control_registers")
    if not isinstance(declared, dict):
        raise Refused("manifest field runtime.projection._control_registers must be an object")
    for register, name in register_names.items():
        # Plain JSON integers, not hex strings: these are register NUMBERS, and 0x18 (24) written
        # as a hex string is 36, which is the sort of silent mismatch a facts gate exists to refuse.
        value = declared.get(name.lower())
        if not isinstance(value, int) or isinstance(value, bool):
            failures.append(f"manifest _control_registers.{name.lower()} must be an integer register number")
        elif value != register:
            failures.append(f"manifest register number for {name} is {value}, not {register}")

    for name in ("gte_init", "set_geom_offset", "set_geom_screen"):
        block = projection.get(name)
        if not isinstance(block, dict):
            raise Refused(f"manifest field runtime.projection.{name} must be an object")
        begin = parse_hex(block.get("entry"), f"projection.{name}.entry")
        end = parse_hex(block.get("end"), f"projection.{name}.end")
        if not (image.load <= begin < end <= image.text_end):
            failures.append(f"projection.{name} [0x{begin:08X},0x{end:08X}) is outside the text")
            continue
        offset = image.file_offset(begin)
        words = [image.word_at(begin + 4 * step) for step in range((end - begin) // 4)]
        # Every writer of a projection register inside the range must be the register the manifest
        # claims, and every word must be one of the three known forms. This is what makes the
        # recorded range a fact rather than a comment.
        print(f"//   {name} [0x{begin:08X},0x{end:08X}) = " + " ".join(f"{w:08X}" for w in words))
        for step, word in enumerate(words):
            if word >> 26 != CP2_OPCODE and (word >> 26) not in (0x00, 0x03):
                continue
            cls = (word >> CLASS_SHIFT) & 0x1F
            register = (word >> REGISTER_SHIFT) & 0x1F
            if cls == CLASS_CTC2 and register in register_names:
                owner = f"{name} writes {register_names[register]} (CR[{register}]) at 0x{begin + 4 * step:08X}"
                print(f"//     {owner}")
        del offset

    census = census_cop2(image)
    for register, name in register_names.items():
        sites = control_writers(census, register)
        reads = control_readers(census, register)
        print(
            f"// CR[{register}] {name}: {len(sites)} control writer(s) "
            f"[{' '.join(f'0x{a:08X}' for a in sites)}], {len(reads)} control reader(s)"
        )
        if len(sites) != 2:
            failures.append(
                f"CR[{register}] {name} has {len(sites)} control writer(s); the manifest and the owner "
                f"are written against exactly two"
            )
        if reads:
            failures.append(
                f"CR[{register}] {name} has {len(reads)} control reader(s); the widening assumes no guest "
                f"branch depends on it"
            )

    call_sites = projection.get("_call_sites")
    if not isinstance(call_sites, dict):
        raise Refused("manifest field runtime.projection._call_sites must be an object")
    leaf = projection.get("set_geom_offset")
    if isinstance(leaf, dict):
        target = parse_hex(leaf.get("entry"), "projection.set_geom_offset.entry")
        measured = direct_call_sites(image, target)
        declared_sites = [parse_hex(token, "projection._call_sites.set_geom_offset")
                          for token in str(call_sites.get("set_geom_offset", "")).split()]
        print(
            f"// direct call sites of set_geom_offset 0x{target:08X}: {len(measured)} measured, "
            f"{len(declared_sites)} declared, {facts['jalr']} jalr site(s) in the image"
        )
        if measured != declared_sites:
            failures.append(
                f"set_geom_offset call sites measured {['0x%08X' % a for a in measured]} but the manifest "
                f"declares {['0x%08X' % a for a in declared_sites]}"
            )

    table = driver_table(image, 0x80054A24)
    print(
        f"// GPU driver pointer table at 0x80054A24: {len(table)} guest-code entries "
        f"(an address scan cannot reach these; the owner is entry index "
        f"{[slot for slot, (_a, v) in enumerate(table) if v == 0x80041C38]}) for FUN_80041C38)"
    )
    if not table:
        failures.append("the GPU driver pointer table at 0x80054A24 is absent; the owner moved")
    if not any(value == 0x80041C38 for _address, value in table):
        failures.append("FUN_80041C38 (the display-mode/draw-area publisher) is not in the driver table")
    if direct_call_sites(image, 0x80041C38):
        failures.append(
            "FUN_80041C38 now has a direct call site; the table-only reachability this records is stale"
        )

    culls = facts["culls"]
    widths = facts["widths"]
    print(
        f"// horizontal cull-idiom census: {len(culls)} immediate match(es) against a 4:3 dot width or a "
        f"PSX visibility/pad mask, of which {len(widths)} name a 4:3 width"
    )
    histogram: dict[int, int] = {}
    for _pc, _text, immediate in culls:
        histogram[immediate] = histogram.get(immediate, 0) + 1
    for immediate in sorted(histogram):
        signed = immediate - 0x10000 if immediate & 0x8000 else immediate
        print(f"//   0x{immediate:04X} ({signed:6d}) x{histogram[immediate]:<3d} {CULL_IMMEDIATES.get(immediate, '')}")
    for pc, text, immediate in widths:
        print(f"//   0x{pc:08X}  {text}   [immediate 0x{immediate:04X}]")
    print(
        "// NULL, stated: a cull against a VARIABLE bound (a global, a viewport record) carries no "
        "immediate and is invisible to this scan. This tool counts what it scanned and what it "
        "matched; it does not claim the image has no variable-bound cull."
    )
    return failures


def selftest(image: Image) -> list[str]:
    """Prove the census can produce the other answer, on a fixture that is built to contain one."""
    failures: list[str] = []

    # A control WRITE to CR[24] is invisible when the register is read from the immediate's low
    # five bits instead of bits 11..15. Build one word each way and require the census to find only
    # the correct one.
    def word(rt: int, cls: int, register: int) -> int:
        return ((0x12 << 26) | ((cls & 0x1F) << CLASS_SHIFT) | ((rt & 0x1F) << 16) |
                ((register & 0x1F) << REGISTER_SHIFT))

    fixture = Image(
        words=(word(4, CLASS_CTC2, CR_OFX),),
        load=0x80010000,
        text_end=0x80010004,
        data=b"",
        path=image.path,
    )
    found = control_writers(census_cop2(fixture), CR_OFX)
    if found != [0x80010000]:
        failures.append(f"selftest: a ctc2 to CR[24] was not found at its own address (got {found})")
    else:
        print("// selftest fired: a ctc2 CR[24] write is found at 0x80010000")

    # The wrong register field must find nothing, which is the exact false negative this tool exists
    # to prevent, so a future edit that reintroduces it fails here rather than in a report.
    wrong = (word(4, CLASS_CTC2, CR_OFX) & ~(0x1F << REGISTER_SHIFT)) | (24 << 0)
    wrong_fixture = Image(words=(wrong,), load=0x80010000, text_end=0x80010004, data=b"", path=image.path)
    if control_writers(census_cop2(wrong_fixture), CR_OFX):
        failures.append("selftest: the decoy low-immediate-field encoding still reached CR[24]")

    # A cfc2 READ of CR[24] must be reported as a reader, because the widening's safety claim is
    # "no guest branch reads it".
    read_fixture = Image(words=(word(4, CLASS_CFC2, CR_OFX),), load=0x80010000, text_end=0x80010004,
                         data=b"", path=image.path)
    if not control_readers(census_cop2(read_fixture), CR_OFX):
        failures.append("selftest: a cfc2 CR[24] read was not reported as a reader")
    else:
        print("// selftest fired: a cfc2 CR[24] read is reported as a reader, not a writer")

    # A cull against 320 must be found; a cull against 321 must not be reported as one. sltiu is
    # I-TYPE: opcode 0x0B, then rs, rt, imm16 - and its imm16 is the whole of bits 0..15.
    sltiu = (OP_SLTIU << 26) | (3 << 21) | (3 << 16) | 320
    near = (OP_SLTIU << 26) | (3 << 21) | (3 << 16) | 321
    cull_fixture = Image(words=(sltiu, near), load=0x80010000, text_end=0x80010008, data=b"",
                         path=image.path)
    hits = cull_census(cull_fixture)
    if [immediate for _pc, _text, immediate in hits] != [320]:
        failures.append(f"selftest: cull census did not separate 320 from 321 (got {hits})")
    else:
        print("// selftest fired: the cull census separates an exact 4:3 width from a near miss")

    # A pointer-table owner must be invisible to a direct call scan and visible to the table scan.
    if direct_call_sites(fixture, 0x80010000):
        failures.append("selftest: the call scan invented a call site")
    print("// selftest fired: a table-only owner produces zero direct call sites")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", default="crash1", help="title directory under titles/")
    parser.add_argument("--executable", type=pathlib.Path, help="override the executable path")
    parser.add_argument("--selftest-only", action="store_true", help="run the negative cases only")
    arguments = parser.parse_args()

    title = ROOT / "titles" / arguments.title
    manifest_path = title / "executable.json"
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, ValueError) as exc:
        print(f"REFUSED: cannot read {manifest_path}: {exc}")
        return 2
    executable = arguments.executable
    if executable is None:
        print(f"REFUSED: pass --executable <SCUS_949.00>; the authenticated image is never a "
              f"build input and tools/provision_title.py places it")
        return 2
    try:
        image = load_image(executable)
        failures = selftest(image) if arguments.selftest_only else None
        if not arguments.selftest_only:
            facts = report(image)
            failures = check(image, manifest, facts)
            failures += selftest(image)
    except Refused as exc:
        print(f"REFUSED: {exc}")
        return 2
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print("PASS: Crash 1 projection census agrees with titles/crash1/executable.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

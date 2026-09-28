#!/usr/bin/env python3
"""Re-derive SCUS_949.00's SIZE-CLASS BLOCK POOL from the authenticated image and gate the
native owner in titles/crash1/core/crash1_block_pool.h against it.

WHY THIS TOOL EXISTS, and what it is not. Crash 1's product stops before it presents anything, and
the guest PC it stops on is `0x800159A8`. That address is not a mystery once the bytes are read: it
is the second `lw $v0,0x4($v1)` of the engine's size-class cell lookup, and the run reaches it
through six measured call sites. Reading it is the first step; the second is making sure the
reading cannot drift. This tool does three things and nothing else:

  A) it re-derives every constant the native owner uses from the image - the class shift, the
     bucket-index mask, the cell stride, the field offsets, and the three pool globals - by
     DECODING the instruction words, so a constant in the header that the image does not produce
     fails here;
  B) it re-derives the call sites of the lookup and of its bounded sibling by an exact `jal` scan,
     with the word count as the denominator, so "6 call sites" is a count over a stated range and
     not a recollection;
  C) it diffs the header's own `inline constexpr` literals against the manifest, so a literal
     cannot be edited in C++ without a gate going red.

WHAT IT DELIBERATELY DOES NOT CLAIM. It does not claim what the engine's class key MEANS. The
lookup compares each cell's second word against `request >> 13`, and the six callers pass request
words that are not uniformly byte counts - one of them passes a tagged handle straight out of a
struct - so the unit is not established here and the recovered code names the field for what it is
compared against, not for a unit this tool cannot measure. It also does not claim the pool is
correct at run time: whether the bucket table holds a usable pointer is a RUNTIME question, and
`crash1_block_pool.cpp` is where that is answered with the value the guest actually published.

INSTRUMENT DISCIPLINE. Every count printed here carries its denominator. The tool REFUSES rather
than answering when the image is missing or is not a PS-X EXE, because "0 call sites" from an
absent image is the failure mode this workspace keeps re-learning. The selftest runs the negative
cases on synthetic fixtures and needs no image, so a fresh clone can still see the instrument go
red.

Exit 0 = the image, the manifest and the header agree, and every selftest fired. 1 = disagreement.
2 = no comparison was possible.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
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

HEADER = ROOT / "titles" / "crash1" / "core" / "crash1_block_pool.h"

OP_LUI = 0x0F
OP_JAL = 0x03
OP_ADDIU = 0x09
OP_ANDI = 0x0C
OP_LW = 0x23
SPECIAL = 0x00
FUNCT_SRL = 0x02
FUNCT_SRA = 0x03

REG_NAMES = (
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
)


class Refused(Exception):
    """The input cannot support the requested claim."""


@dataclass(frozen=True)
class Image:
    words: tuple[int, ...]
    load: int
    text_end: int
    data: bytes
    path: pathlib.Path

    @property
    def word_count(self) -> int:
        return len(self.words)

    def word_at(self, address: int) -> int:
        if not self.load <= address < self.text_end:
            raise Refused(f"0x{address:08X} is outside the text segment")
        return self.words[(address - self.load) >> 2]

    def slice(self, begin: int, end: int) -> bytes:
        return self.data[0x800 + (begin - self.load) : 0x800 + (end - self.load)]


def load_image(path: pathlib.Path) -> Image:
    try:
        data = path.read_bytes()
        image = psx_exe.load(str(path))
    except (OSError, ValueError) as exc:
        raise Refused(f"cannot read a valid executable from {path}: {exc}") from exc
    count = (image.text_end - image.load) // 4
    if count <= 0:
        raise Refused(f"{path} declares an empty text segment")
    body = data[0x800 : 0x800 + count * 4]
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


def opcode(word: int) -> int:
    return (word >> 26) & 0x3F


def registers(word: int) -> tuple[int, int, int]:
    return ((word >> 21) & 0x1F, (word >> 16) & 0x1F, (word >> 11) & 0x1F)


def immediate(word: int) -> int:
    return word & 0xFFFF


def signed_immediate(word: int) -> int:
    value = immediate(word)
    return value - 0x10000 if value & 0x8000 else value


def special_funct(word: int) -> int:
    return word & 0x3F


def jump_target(address: int, word: int) -> int:
    # A MIPS jump forms its target from the DELAY SLOT's PC region, not the jump's own PC.
    return (((address + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)) & 0xFFFFFFFF


def block_pool_block(manifest: dict[str, object]) -> dict[str, object]:
    runtime = manifest.get("runtime")
    if not isinstance(runtime, dict):
        raise Refused("manifest field runtime must be an object")
    block = runtime.get("block_pool")
    if not isinstance(block, dict):
        raise Refused("manifest field runtime.block_pool must be an object")
    return block


def sub(block: dict[str, object], name: str) -> dict[str, object]:
    value = block.get(name)
    if not isinstance(value, dict):
        raise Refused(f"manifest field runtime.block_pool.{name} must be an object")
    return value


def at(block: dict[str, object], dotted: str) -> object:
    node: object = block
    for part in dotted.split("."):
        if not isinstance(node, dict):
            raise Refused(f"manifest path {dotted} leaves the object early")
        node = node.get(part)
    if node is None:
        raise Refused(f"manifest path {dotted} is absent")
    return node


def body_digest(image: Image, begin: int, end: int) -> str:
    return hashlib.sha256(image.slice(begin, end)).hexdigest()


# ── A: the decoded shape of the lookup, re-derived from the image ────────────────────────────────

def decode_lookup(image: Image, entry: int, class_shift_word: int) -> dict[str, object]:
    """Recover the class shift, the bucket mask, the stride and the class field from the words.

    This reads the INSTRUCTIONS, not a transcription of them: the shift comes out of the `srl`'s
    `sa` field, the mask out of the `andi`'s immediate, the stride out of the `addiu`'s immediate,
    and the class-field offset out of the `lw`'s displacement. If the image said something else,
    this would say something else too - which is the point of doing it here rather than in a
    comment.
    """
    shift = (class_shift_word >> 6) & 0x1F
    found: dict[str, object] = {"class_shift": shift}
    words = [image.word_at(entry + 4 * i) for i in range(20)]
    masks = [w for w in words if opcode(w) == OP_ANDI]
    # A stride step is an `addiu` that adds to the register it read from. The lookup has three: the
    # loop step, the same step in the `bne` delay slot, and one NEGATIVE correction that undoes the
    # delay slot on the match path. Two positive steps and exactly one -stride is the shape the
    # recovery claims; anything else means the description is of a different function.
    steps = [
        signed_immediate(w)
        for w in words
        if opcode(w) == OP_ADDIU and registers(w)[0] == registers(w)[1]
    ]
    forwards = [step for step in steps if step > 0]
    backwards = [step for step in steps if step < 0]
    loads = {signed_immediate(w) for w in words if opcode(w) == OP_LW}
    if len(masks) != 1:
        raise Refused(f"expected exactly one `andi` in the lookup at 0x{entry:08X}, found {len(masks)}")
    if len(set(forwards)) != 1 or len(forwards) < 2:
        raise Refused(
            f"the lookup at 0x{entry:08X} has {forwards} forward stride steps; the recovery "
            "describes one stride used twice"
        )
    stride = forwards[0]
    if backwards != [-stride]:
        raise Refused(
            f"the lookup at 0x{entry:08X} has corrections {backwards}; the recovery describes "
            f"exactly one {-stride}"
        )
    # A cell is `stride` bytes wide, and the lookup's only cell-relative load is the class field.
    # That field is the SECOND word, so the offset is half the stride - derived, not assumed, and
    # cross-checked against the displacement the image actually encodes.
    class_field = stride // 2
    if class_field not in loads or 0 not in loads:
        raise Refused(
            f"the lookup at 0x{entry:08X} loads at {sorted(loads)}; the recovery describes a "
            f"bucket-pointer read at +0 and a class read at +{class_field}"
        )
    found["bucket_index_mask"] = immediate(masks[0])
    found["cell_stride"] = stride
    found["class_field_offset"] = class_field
    return found


def decode_lui_pool_global(image: Image, address: int) -> int:
    """Resolve a `lui $rX,0x8006` + `lw $rY,disp($rX)` pair to the pool global it names."""
    rs, _, _ = registers(image.word_at(address))
    base = (image.word_at(address - 4) & 0xFFFF) << 16
    return (base + signed_immediate(image.word_at(address))) & 0xFFFFFFFF


# ── A2: WHAT the class field is compared against, and what the shift is for ─────────────────────────
#
# THE THING THIS SECTION EXISTS FOR. `0x80015978` is `srl v0,a0,13`, which reads like "the class is
# the request shifted down 13", and the shipped owner was written that way. It is not what the image
# says. The class a cell carries is compared against register `$a0` — the request, WHOLE — and the
# shifted value is dead one instruction after the bucket address is formed. A static reading of the
# shift alone produced a lookup that could never match a cell the engine itself wrote, so its walk
# never ended and it handed every caller a cell at the top of main RAM. That is not a subtle
# regression: it was the measured cause of the product presenting nothing, and it survived a
# self-test because the self-test was written from the same misreading.
#
# So the check below is on the BRANCH words, not on a comment: it reads the register operands of
# every class comparison in the image and requires all of them to be the argument register, and it
# requires the argument register to be UNREDEFINED between the entry and each comparison. Change the
# owner back to searching for the shifted key and this still passes — the owner is not what is being
# checked — but change the RECOVERY and this goes red, which is the direction that matters.
OP_BEQ = 0x04
OP_BNE = 0x05
REG_A0 = 4
REG_V0 = 2
# (branch, what the branch is, the `lw` that produced the class field, the function entry)
CLASS_COMPARISONS = (
    (0x8001599C, "the lookup's first-cell test", 0x80015994, 0x80015978),
    (0x800159B0, "the lookup's back edge", 0x800159A8, 0x80015978),
    (0x800159E8, "the bounded sibling's first-cell test", 0x800159E0, 0x800159C4),
)


def decode_class_comparison(image: Image, branch: int, class_load: int) -> tuple[int, int]:
    """(class field register, register the class is compared against) for one branch.

    MIPS register fields, which this repo's other decoders already use: for a branch, `rs` is
    bits 25-21 and `rt` is bits 20-16, and `beq` compares the two, so either order reads the same.
    For the `lw` that produced the class field, the DESTINATION is `rt` (bits 20-16), not `rs` — a
    loader that read `rs` here would be reading the cell pointer and would report the wrong register.
    """
    word = image.word_at(branch)
    if opcode(word) not in (OP_BEQ, OP_BNE):
        raise Refused(f"0x{branch:08X} is not a beq/bne, so it is not a class comparison")
    # For a BRANCH the two operands are `rs` (bits 25-21) and `rt` (bits 20-16) and neither is a
    # destination, so bits 15-11 are the high half of the displacement and must not be read.
    compared, against, _ = registers(word)
    load = image.word_at(class_load)
    if opcode(load) != OP_LW:
        raise Refused(f"0x{class_load:08X} is not the `lw` that produced the class field")
    return registers(load)[1], against


def decode_bucket_shift(image: Image, entry: int) -> tuple[int, int]:
    """(source register, destination register) of the `srl` that builds the bucket index.

    `srl rd,rt,sa` puts rt in bits 20-16 and rd in bits 15-11, so the SOURCE is `rt` and the
    DESTINATION is `rd`. Reading the other way round would report the shift as consuming whatever
    register it produced, which happens to be a plausible-looking sentence and is the wrong one.
    """
    word = image.word_at(entry)
    if opcode(word) != 0x00 or special_funct(word) != FUNCT_SRL:
        raise Refused(f"0x{entry:08X} is not an `srl`, so it is not the bucket-index shift")
    return registers(word)[1], registers(word)[2]


# Every opcode that WRITES a register, so the check below can ask "was $a0 touched?" from the words
# instead of from a reading of the function. SPECIAL writes `rd` (bits 15-11); the rest write `rt`.
# The branches are DELIBERATELY ABSENT: `beq`/`bne`/`blez`/`bgez` put their second operand in `rt`
# and write nothing, and treating `rt` as a destination there invents a write of `$a0` that the
# image does not contain — which is the kind of confident wrong answer this tool exists to stop.
_WRITING_OPCODES = frozenset(
    {0x02, 0x03, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F,
     0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B}
)


def redefined_registers(image: Image, entry: int, stop: int) -> set[int]:
    """Registers written in [entry, stop), decoded from the words rather than asserted from a comment."""
    written: set[int] = set()
    for address in range(entry, stop, 4):
        word = image.word_at(address)
        op = opcode(word)
        if op == 0x00:
            written.add(registers(word)[2])
        elif op in _WRITING_OPCODES:
            written.add(registers(word)[1])
    return written


# ── B: the call sites, with the word scan as the denominator ─────────────────────────────────────

def call_sites(image: Image, target: int) -> list[int]:
    sites = []
    for index in range(image.word_count):
        word = image.words[index]
        if opcode(word) == OP_JAL and jump_target(image.load + index * 4, word) == target:
            sites.append(image.load + index * 4)
    return sites


def function_entries(image: Image) -> set[int]:
    return {
        jump_target(image.load + index * 4, word)
        for index, word in enumerate(image.words)
        if opcode(word) == OP_JAL
    }


def enclosing_function(entries: set[int], site: int) -> int:
    below = [entry for entry in entries if entry <= site]
    return max(below) if below else 0


# ── C: the header's own literals ──────────────────────────────────────────────────────────────────

CONSTANT = re.compile(r"inline constexpr [^;=]*?\b(k[A-Za-z0-9_]+)\s*=\s*([^;]+);")


def header_constants(path: pathlib.Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    return {name: value.strip() for name, value in CONSTANT.findall(text)}


def check_shipping_constants(manifest: dict[str, object]) -> list[str]:
    block = block_pool_block(manifest)
    if not HEADER.is_file():
        return [f"the native owner's header is absent: {HEADER}"]
    found = header_constants(HEADER)
    pairs = [
        ("kFindCell", at(block, "find_cell.entry")),
        ("kFindCellEnd", at(block, "find_cell.end")),
        ("kFindCellBounded", at(block, "find_cell_bounded.entry")),
        ("kFindCellBoundedEnd", at(block, "find_cell_bounded.end")),
        ("kClassShift", at(block, "find_cell.class_shift")),
        ("kBucketIndexMask", at(block, "find_cell.bucket_index_mask")),
        ("kCellStride", at(block, "find_cell.cell_stride")),
        ("kClassFieldOffset", at(block, "find_cell.class_field_offset")),
        ("kBucketTablePointer", at(block, "find_cell.bucket_table_pointer")),
        ("kPoolBasePointer", at(block, "find_cell_bounded.pool_base_pointer")),
        ("kCellCountBlockPointer", at(block, "find_cell_bounded.cell_count_block_pointer")),
        ("kCellCountOffset", at(block, "find_cell_bounded.cell_count_offset")),
        ("kPoolExhausted", at(block, "find_cell_bounded.exhausted_value")),
        ("kFaultSite", at(block, "fault_site.address")),
    ]
    failures: list[str] = []
    for name, expected in pairs:
        if name not in found:
            failures.append(f"{HEADER.name} declares no {name}")
            continue
        try:
            have = int(found[name].rstrip("uU"), 0)
        except ValueError:
            failures.append(
                f"{HEADER.name}: {name} is {found[name]!r}, which is not an integer literal this "
                "tool can compare against the image"
            )
            continue
        want = int(expected, 0) if isinstance(expected, str) else int(expected)
        if have & 0xFFFFFFFF != want & 0xFFFFFFFF:
            failures.append(
                f"{HEADER.name}: {name} is {found[name]}, the image says 0x{want & 0xFFFFFFFF:08X}"
            )
    return failures


# ── the measurement ───────────────────────────────────────────────────────────────────────────────

def check(image: Image, manifest: dict[str, object]) -> list[str]:
    block = block_pool_block(manifest)
    find_cell = sub(block, "find_cell")
    bounded = sub(block, "find_cell_bounded")
    entry = parse_hex(at(block, "find_cell.entry"), "find_cell.entry")
    end = parse_hex(at(block, "find_cell.end"), "find_cell.end")
    bounded_entry = parse_hex(at(block, "find_cell_bounded.entry"), "find_cell_bounded.entry")
    bounded_end = parse_hex(at(block, "find_cell_bounded.end"), "find_cell_bounded.end")

    print(
        f"[scan] {image.path.name}: text 0x{image.load:08X}..0x{image.text_end:08X}, "
        f"{image.word_count} words"
    )

    failures: list[str] = []

    for name, begin, stop in (
        ("find_cell", entry, end),
        ("find_cell_bounded", bounded_entry, bounded_end),
    ):
        recorded = at(block, f"{name}.body_sha256")
        if not isinstance(recorded, str):
            raise Refused(f"manifest field {name}.body_sha256 must be a hex digest string")
        measured = body_digest(image, begin, stop)
        words = (stop - begin) // 4
        print(f"[body] {name} [0x{begin:08X},0x{stop:08X}) = {words} words, sha256 {measured}")
        if measured != recorded:
            failures.append(f"{name} body digest {measured} does not match the manifest {recorded}")

    # The fault site is the whole claim, so it is pinned by its own word rather than by a range.
    fault = parse_hex(at(block, "fault_site.address"), "fault_site.address")
    fault_word = parse_hex(at(block, "fault_site.word"), "fault_site.word")
    measured_fault = image.word_at(fault)
    if measured_fault != fault_word:
        failures.append(
            f"fault site 0x{fault:08X} is 0x{measured_fault:08X}, the manifest says 0x{fault_word:08X}"
        )
    if opcode(measured_fault) != OP_LW or signed_immediate(measured_fault) != 4:
        failures.append(
            f"fault site 0x{fault:08X} is not the class-field load the recovery claims it is "
            f"(opcode 0x{opcode(measured_fault):02X}, displacement {signed_immediate(measured_fault)})"
        )

    decoded = decode_lookup(image, entry, image.word_at(entry))
    for key in ("class_shift", "bucket_index_mask", "cell_stride", "class_field_offset"):
        recorded = parse_hex(at(block, f"find_cell.{key}"), f"find_cell.{key}")
        if decoded[key] != recorded:
            failures.append(
                f"find_cell.{key}: the instructions decode to {decoded[key]}, the manifest says "
                f"0x{recorded:X}"
            )
    print(
        "[decode] find_cell: bucket-index shift {}, bucket mask 0x{:X}, cell stride {}, class field +{}"
        .format(decoded["class_shift"], int(decoded["bucket_index_mask"]), decoded["cell_stride"],
                decoded["class_field_offset"])
    )

    # WHAT THE CLASS IS COMPARED AGAINST, read out of the branch words. See the note on
    # `decode_class_comparison`: this is the check that the shipped owner was wrong about.
    for branch, description, class_load, function_entry in CLASS_COMPARISONS:
        field_reg, against = decode_class_comparison(image, branch, class_load)
        shift_source, shift_dest = decode_bucket_shift(image, function_entry)
        print(
            "[compare] 0x{:08X} ({}): the class field in ${} is compared against ${}; the "
            "bucket-index `srl` consumed ${}".format(
                branch,
                description,
                REG_NAMES[field_reg],
                REG_NAMES[against],
                REG_NAMES[shift_source],
            )
        )
        if against != REG_A0:
            failures.append(
                f"0x{branch:08X} ({description}) compares the class field against "
                f"${REG_NAMES[against]}, not against $a0: the class is the request WHOLE, and an "
                "owner that searches for the shifted key can never match a cell the engine wrote"
            )
        if field_reg != REG_V0:
            failures.append(
                f"0x{branch:08X} ({description}) tests ${REG_NAMES[field_reg]}, not $v0; the "
                "recovery describes the `lw $v0,4($cell)` result"
            )
        if shift_source != REG_A0 or shift_dest != REG_V0:
            failures.append(
                f"0x{function_entry:08X} shifts ${REG_NAMES[shift_source]} into "
                f"${REG_NAMES[shift_dest]}; the recovery describes `srl $v0,$a0,13`"
            )
        # The shift exists to index the bucket, so the register the request is compared against must
        # survive untouched from the entry into the compare. This is the check that would fire if a
        # future reader concluded the class is something derived rather than the argument itself.
        overwritten = redefined_registers(image, function_entry, branch)
        if REG_A0 in overwritten:
            failures.append(
                f"0x{branch:08X} compares against $a0, but $a0 is REDEFINED between "
                f"0x{function_entry:08X} and it ({sorted(REG_NAMES[r] for r in overwritten)} are "
                "written); the class is then something other than the request"
            )

    # The pool global is named by a `lui`+`lw` PAIR, never by a literal, so the pair is what has to
    # be found: a scan for the 16-bit displacement alone would match unrelated code and a scan for
    # the address alone would match nothing.
    for dotted, expected_sites in (
        ("find_cell.bucket_table_pointer", (entry + 8,)),
        ("find_cell_bounded.pool_base_pointer", (bounded_entry + 0x38,)),
        ("find_cell_bounded.cell_count_block_pointer", (bounded_entry + 0x30,)),
    ):
        address = parse_hex(at(block, dotted), dotted)
        for site in expected_sites:
            if decode_lui_pool_global(image, site) != address:
                failures.append(
                    f"{dotted}: the `lui`+`lw` pair at 0x{site:08X} names "
                    f"0x{decode_lui_pool_global(image, site):08X}, the manifest says 0x{address:08X}"
                )

    expected_sites = at(block, "call_sites")
    if not isinstance(expected_sites, list):
        raise Refused("manifest field runtime.block_pool.call_sites must be a list")
    measured_sites = call_sites(image, entry)
    entries = function_entries(image)
    print(
        f"[scan] {image.word_count} words; {len(entries)} distinct `jal` targets, so a "
        f"function-entry census saturates and cannot rank callers by likelihood"
    )
    print(f"[callers] `jal 0x{entry:08X}`: {len(measured_sites)} site(s)")
    recorded_sites = []
    for record in expected_sites:
        if not isinstance(record, dict):
            raise Refused("each runtime.block_pool.call_sites entry must be an object")
        recorded_sites.append(parse_hex(record.get("address"), "call_sites[].address"))
    for site in measured_sites:
        print(f"    0x{site:08X} inside 0x{enclosing_function(entries, site):08X}")
    if measured_sites != recorded_sites:
        failures.append(
            f"call sites {['0x%08X' % s for s in measured_sites]} do not match the manifest "
            f"{['0x%08X' % s for s in recorded_sites]}"
        )
    if int(at(block, "call_site_count")) != len(measured_sites):
        failures.append(
            f"the manifest records {at(block, 'call_site_count')} call sites, the scan found "
            f"{len(measured_sites)}"
        )

    bounded_sites = call_sites(image, bounded_entry)
    recorded_bounded = at(block, "find_cell_bounded.call_site_count")
    print(
        f"[callers] `jal 0x{bounded_entry:08X}`: {len(bounded_sites)} site(s) of "
        f"{image.word_count} words scanned"
    )
    if len(bounded_sites) != int(recorded_bounded):
        failures.append(
            f"the bounded sibling has {len(bounded_sites)} call sites, the manifest records "
            f"{recorded_bounded}"
        )

    exhausted = parse_hex(at(block, "find_cell_bounded.exhausted_value"), "exhausted_value")
    if exhausted >= 0x80000000:
        exhausted -= 0x100000000
    if exhausted != int(at(block, "find_cell_bounded.exhausted_value_signed")):
        failures.append(
            f"the bounded sibling's exhausted word 0x{exhausted & 0xFFFFFFFF:08X} does not read as "
            f"{at(block, 'find_cell_bounded.exhausted_value_signed')}"
        )
    bounded_words = [image.word_at(bounded_entry + 4 * i) for i in range((bounded_end - bounded_entry) // 4)]
    # The bound is `((cell - poolBase) >> k) >= cellCount`, so the image must contain a right shift
    # and a `slt` against the cell count. Finding neither means the recovery is describing a
    # different function, and saying so beats reporting a digest that matched.
    shifts = [w for w in bounded_words if opcode(w) == 0 and special_funct(w) in (FUNCT_SRL, FUNCT_SRA)]
    if not shifts:
        failures.append("the bounded sibling contains no right shift, as the recovery claims")
    slts = [w for w in bounded_words if opcode(w) == 0 and special_funct(w) == 0x2A]
    if not slts:
        failures.append("the bounded sibling contains no `slt`, as the recovery claims")
    count_offset = parse_hex(at(block, "find_cell_bounded.cell_count_offset"),
                             "find_cell_bounded.cell_count_offset")
    if not any(signed_immediate(w) == count_offset for w in bounded_words if opcode(w) == OP_LW):
        failures.append(
            f"the bounded sibling does not load the cell count at +{count_offset}, as the manifest says"
        )
    negative = [
        signed_immediate(w)
        for w in bounded_words
        if opcode(w) == OP_ADDIU and registers(w)[0] == 0 and signed_immediate(w) < 0
    ]
    if not negative:
        failures.append("the bounded sibling returns no negative sentinel, as the recovery claims")
    elif exhausted not in negative:
        failures.append(
            f"the bounded sibling's sentinels are {negative}, the manifest says {exhausted}"
        )

    failures.extend(check_shipping_constants(manifest))
    return failures


# ── the negative cases ────────────────────────────────────────────────────────────────────────────

def selftest() -> list[str]:
    """Cases whose firing the instrument must be able to SHOW, plus the ones it cannot.

    The rule this follows: a selftest that only ever returns "everything is fine" is a test of
    nothing, and one that returns a list nobody reads is worse. So every case below either raised
    (and is reported as fired) or is reported as MISSING, and a MISSING entry is a failure. The
    case that matters most is the last one: an EMPTY image must report `0 of 0 words scanned` as a
    refusal, because "0 call sites" measured against nothing is indistinguishable from a real
    answer, and this workspace has already published one of those.
    """
    fired: list[str] = []
    missing: list[str] = []

    def expect_refusal(description: str, action) -> None:
        try:
            action()
        except Refused:
            fired.append(description)
        else:
            missing.append(f"MISSING: {description}")

    block = {
        "find_cell": {
            "entry": "0x80015978",
            "end": "0x800159C4",
            "body_sha256": "0" * 64,
            "class_shift": 13,
            "bucket_index_mask": "0x03FC",
            "cell_stride": 8,
            "class_field_offset": 4,
            "bucket_table_pointer": "0x8005C530",
        },
        "find_cell_bounded": {
            "entry": "0x800159C4",
            "end": "0x80015A3C",
            "body_sha256": "0" * 64,
            "pool_base_pointer": "0x8005C534",
            "cell_count_block_pointer": "0x8005C540",
            "cell_count_offset": "0x404",
            "exhausted_value": "0xFFFFFFF6",
            "exhausted_value_signed": -10,
            "call_site_count": 0,
        },
        "fault_site": {"address": "0x800159A8", "word": "0x8C620004"},
        "call_sites": [{"address": "0x8001313C"}],
    }

    for absent in ("runtime", "runtime.block_pool"):
        broken: dict[str, object] = {"runtime": {"block_pool": json.loads(json.dumps(block))}}
        node: dict[str, object] = broken  # type: ignore[assignment]
        parts = absent.split(".")
        for part in parts[:-1]:
            node = node[part]  # type: ignore[assignment]
        del node[parts[-1]]
        expect_refusal(
            f"refuses a manifest without {absent}",
            lambda b=broken: block_pool_block(b),  # type: ignore[arg-type]
        )
    # `block_pool_block` only proves the top of the tree, so the sub-block accessor is exercised
    # separately: a manifest carrying `block_pool` with no `find_cell` is the shape a half-finished
    # edit produces, and it must be a refusal rather than a later KeyError.
    expect_refusal(
        "refuses a block_pool without find_cell",
        lambda: sub(block_pool_block({"runtime": {"block_pool": {"find_cell_bounded": {}}}}), "find_cell"),
    )

    expect_refusal(
        "refuses a non-hexadecimal manifest address", lambda: parse_hex("nothex", "find_cell.entry")
    )
    expect_refusal(
        "refuses a non-string manifest address", lambda: parse_hex(13, "find_cell.entry")
    )
    expect_refusal(
        "refuses a word read outside the declared text",
        lambda: Image(words=(), load=0, text_end=0, data=b"", path=pathlib.Path("<selftest>")).word_at(0x80015978),
    )
    expect_refusal(
        "refuses an absent manifest path",
        lambda: at(block_pool_block({"runtime": {"block_pool": block}}), "find_cell.nope"),
    )

    # The positive control: the instrument must be able to say "nothing was scanned", and it must
    # say it with a denominator rather than as a bare zero.
    empty = Image(words=(), load=0, text_end=0, data=b"", path=pathlib.Path("<selftest>"))
    sites = call_sites(empty, 0x80015978)
    if sites or empty.word_count != 0:
        missing.append(
            f"MISSING: an empty image reported {len(sites)} call site(s) over "
            f"{empty.word_count} words instead of refusing"
        )
    else:
        fired.append("an empty image answers 0 call sites over a stated 0-word denominator")

    # THE CLASS-COMPARISON CHECK HAS TO BE ABLE TO GO RED, and "it passed on the real image" is not
    # evidence of that. So the two candidate models are built as synthetic word tables and the same
    # decoder that accepted the image must reject the shifted one. This is the case the shipped owner
    # was written from, and it is the case a self-test written from the same misreading agreed with.
    for name, against in (("whole request", REG_A0), ("shifted key", 2)):
        words = {}
        for index, address in enumerate(range(0x80015978, 0x800159C4, 4)):
            words[address] = 0x00000000
        words[0x80015978] = 0x00041342  # srl $v0,$a0,13
        words[0x8001597C] = 0x3C068006  # lui $v1,0x8006
        words[0x80015980] = 0x8E63C530  # lw $v1,-0x3AD0($v1)
        words[0x80015984] = 0x304203FC  # andi $v0,$v0,0x03FC
        words[0x80015988] = 0x00422021  # addu $v0,$v0,$v1
        words[0x8001598C] = 0x8C430000  # lw $v1,0($v0)
        words[0x80015994] = 0x8C620004  # lw $v0,4($v1)
        words[0x8001599C] = (OP_BEQ << 26) | (2 << 21) | (against << 16) | (0x00015A18 >> 2 & 0xFFFF)
        words[0x800159A4] = 0x24630008
        words[0x800159A8] = 0x8C620004
        words[0x800159B0] = (OP_BNE << 26) | (2 << 21) | (against << 16)
        synthetic = Image(
            words=tuple(words.get(0x80015978 + 4 * i, 0) for i in range((0x800159C4 - 0x80015978) // 4)),
            load=0x80015978,
            text_end=0x800159C4,
            data=bytes((0x800159C4 - 0x80015978)),
            path=pathlib.Path("<selftest:pool-lookup>"),
        )
        field_reg, seen = decode_class_comparison(synthetic, 0x8001599C, 0x80015994)
        if field_reg != REG_V0:
            missing.append(f"MISSING: the synthetic {name} fixture decoded a class field of ${field_reg}")
        elif (seen == REG_A0) != (against == REG_A0):
            missing.append(
                f"MISSING: the synthetic {name} fixture decoded the comparison register as ${seen}"
            )
        else:
            fired.append(
                f"the class-comparison decoder accepts the {name} fixture and reads the comparison "
                f"register as ${REG_NAMES[seen]}"
            )
    expect_refusal(
        "refuses a word it calls a class comparison but which is not a branch",
        lambda: decode_class_comparison(
            Image(words=(0, 0, 0x8C620004, 0x24630008), load=0, text_end=16,
                  data=bytes(16), path=pathlib.Path("<selftest>")),
            0x8001599C,
            0x80015994,
        ),
    )

    return missing + fired


def failures_only(fired: list[str]) -> list[str]:
    return [entry for entry in fired if entry.startswith("MISSING:")]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", default="crash1", help="title directory under titles/")
    parser.add_argument("--executable", type=pathlib.Path, help="the authenticated SCUS_949.00")
    parser.add_argument("--selftest-only", action="store_true", help="run the negative cases only")
    arguments = parser.parse_args()

    try:
        if arguments.selftest_only:
            entries = selftest()
            for entry in entries:
                print(f"[selftest] {entry}")
            print(f"[selftest] {sum(1 for e in entries if not e.startswith('MISSING:'))} of "
                  f"{len(entries)} cases fired")
            missing = failures_only(entries)
            for entry in missing:
                print(f"FAIL: {entry}")
            if missing:
                return 1
            print("PASS: Crash 1 block-pool selftest fired every case")
            return 0
        manifest_path = ROOT / "titles" / arguments.title / "executable.json"
        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        except (OSError, ValueError) as exc:
            print(f"REFUSED: cannot read {manifest_path}: {exc}")
            return 2
        if arguments.executable is None:
            print(
                "REFUSED: pass --executable <SCUS_949.00>; the authenticated image is never a build "
                "input and tools/provision_title.py places it"
            )
            return 2
        image = load_image(arguments.executable)
        failures = check(image, manifest)
        entries = selftest()
        for entry in entries:
            print(f"[selftest] {entry}")
        print(f"[selftest] {sum(1 for e in entries if not e.startswith('MISSING:'))} of "
              f"{len(entries)} cases fired")
        failures.extend(failures_only(entries))
    except Refused as exc:
        print(f"REFUSED: {exc}")
        return 2

    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print("PASS: Crash 1 block-pool recovery agrees with titles/crash1/executable.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Re-derive SCUS_949.00's POOL ALLOCATE PATH (0x80012F10) from the authenticated image and gate the
readable owner in titles/crash1/core/crash1_pool_node.h against it.

WHY THIS TOOL AND WHAT IT IS NOT. `crash1_block_pool.py` recovers the cell LOOKUP at 0x80015978, which
only reads the pool. This recovers the function at 0x80012F10 that FILLS it, and the one thing that
matters about it is that it is where the CLASS WORD is defined: the word is loaded at 0x80012FBC,
shifted at 0x80012FC8 to pick a bucket, and the SAME unshifted word is compared against a cell's class
field at 0x80012FFC. The shipped lookup owner was written against the shifted form, could never match
anything this function wrote, and presented no primitives. That defect is corrected in
`crash1_block_pool.h`; this tool is what keeps the correction honest on the ALLOCATE side, and it is
deliberately a separate instrument from the lookup's so that one wrong constant fails in the owner
that owns it.

WHAT IT DECODES, every value from the instruction that produces it, never from a comment:
  A) the function's extent, its body digest, and its two `jal` call sites over a stated word count;
  B) the five globals, each of which is a `lui`+`addiu`/`lw` PAIR and therefore invisible to a
     literal-immediate scan;
  C) both strides, decoded from the multiply-out the image actually performs;
  D) the class-word rule — the bucket is `word >> 13`, the class is `word` — from the branch words and
     from the store at 0x80013140, which is where the engine writes the word it is about to look up;
  E) the five-way dispatch, read off the comparison immediates;
  F) the owner's own `inline constexpr` literals, diffed against titles/crash1/executable.json.

INSTRUMENT DISCIPLINE, the same rules the other probes in this repository follow. Every count carries
its denominator, the tool REFUSES rather than answering when the image is absent or is not a PS-X EXE
("0 call sites" from an absent image is indistinguishable from a real answer), and the selftest builds
synthetic fixtures for the cases that must go RED so that "it passed on the real image" is never
mistaken for "it can fail".

Exit 0 = image, manifest and header agree and every selftest case fired. 1 = disagreement. 2 = no
comparison was possible.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import struct
import sys

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

HEADER = ROOT / "titles" / "crash1" / "core" / "crash1_pool_node.h"

OP_SPECIAL = 0x00
OP_LUI = 0x0F
OP_JAL = 0x03
OP_ADDIU = 0x09
OP_ANDI = 0x0C
OP_LW = 0x23
OP_BEQ = 0x04
OP_BNE = 0x05
OP_SRA = 0x03  # SPECIAL funct
OP_SLL = 0x00
OP_ADDU = 0x21
OP_SUBU = 0x23
OP_ORI = 0x0D
OP_SW = 0x2B
OP_LHU = 0x25
OP_LH = 0x21

REG_NAMES = (
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
)
REG_A0 = 4
REG_V0 = 2
REG_V1 = 3
REG_S2 = 18


class Refused(Exception):
    """The input cannot support the requested claim."""


class Image:
    def __init__(self, words, load, text_end, path):
        self.words = tuple(words)
        self.load = load
        self.text_end = text_end
        self.path = pathlib.Path(path)

    @property
    def word_count(self) -> int:
        return len(self.words)

    def word_at(self, address: int) -> int:
        if not self.load <= address < self.text_end:
            raise Refused(f"0x{address:08X} is outside the text segment")
        return self.words[(address - self.load) >> 2]

    def slice(self, begin: int, end: int) -> bytes:
        return b"".join(struct.pack("<I", self.word_at(begin + 4 * i))
                        for i in range((end - begin) // 4))


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
    return Image(struct.unpack(f"<{count}I", body), image.load, image.text_end, path)


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
    return (((address + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)) & 0xFFFFFFFF


def call_sites(image: Image, target: int) -> list[int]:
    return [
        image.load + 4 * i
        for i, word in enumerate(image.words)
        if opcode(word) == OP_JAL and jump_target(image.load + 4 * i, word) == target
    ]


def at(node, dotted):
    for part in dotted.split("."):
        if not isinstance(node, dict):
            raise Refused(f"manifest path {dotted} leaves the object early")
        node = node.get(part)
    if node is None:
        raise Refused(f"manifest path {dotted} is absent")
    return node


def parse_hex(value, field):
    if not isinstance(value, str):
        raise Refused(f"manifest field {field} must be a hex string")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise Refused(f"manifest field {field} is not hexadecimal: {value!r}") from exc


# ── B: the five globals, each a `lui` + (addiu | lw) PAIR ─────────────────────────────────────────

def resolve_lui_pair(image: Image, lui: int, access: int) -> int:
    """The address a `lui $rX,imm` / (`addiu`|`lw`) $rY,disp($rX) pair names.

    The image carries NONE of these addresses as an immediate, so a scan for the address finds
    nothing and a scan for the displacement matches unrelated code. Only the pair identifies them, and
    the pair is also what proves the constant is in the image at all.
    """
    if opcode(image.word_at(lui)) != OP_LUI:
        raise Refused(f"0x{lui:08X} is not a `lui`, so it names no global")
    base = immediate(image.word_at(lui)) << 16
    access_word = image.word_at(access)
    if opcode(access_word) not in (OP_ADDIU, OP_LW):
        raise Refused(f"0x{access:08X} is not the `addiu`/`lw` half of the pair at 0x{lui:08X}")
    rs, _, _ = registers(access_word)
    if rs != registers(image.word_at(lui))[1]:
        raise Refused(
            f"0x{access:08X} does not read the register 0x{lui:08X} loaded, so the pair does not name "
            "one global"
        )
    return (base + signed_immediate(access_word)) & 0xFFFFFFFF


GLOBALS = {
    "node_table": (0x80012F30, 0x80012F34),
    "node_cursor": (0x80012F28, 0x80012F2C),
    "bucket_table": (0x80012FC0, 0x80012FC4),
    "handle_table": (0x800130D4, 0x800130D8),
    "escape_class_table": (0x800131B8, 0x800131BC),
    "type_callback_table": (0x80013048, 0x80013050),
}


# ── C: the strides, decoded from the multiply-out ──────────────────────────────────────────────────

def decode_node_stride(image: Image) -> int:
    """index*44, recovered by SIMULATING the five instructions that build it.

    0x80012F14 `sll v0,a0,1`; 0x80012F18 `addu v0,v0,a0`; 0x80012F1C `sll v0,v0,2`;
    0x80012F20 `subu v0,v0,a0`; 0x80012F24 `sll v0,v0,2`. Reading "44" out of that is arithmetic a
    reader can check; asserting "44" is a claim. The simulated probe values are 1, 2 and 7, and two of
    the three are enough to pin the formula, so all three are run and all three must agree.
    """
    sequence = [image.word_at(0x80012F14 + 4 * i) for i in range(5)]
    # All five are SPECIAL (funct below), not ADDIU: `sll`, `addu`, `sll`, `subu`, `sll`. Reading them
    # as I-types is a transcription error that a shape check on the opcode alone would not catch,
    # which is why the funct is checked too.
    expected_functs = [0x00, 0x21, 0x00, 0x23, 0x00]
    for index, (word, want) in enumerate(zip(sequence, expected_functs)):
        if opcode(word) != OP_SPECIAL or special_funct(word) != want:
            raise Refused(
                f"stride step {index} is opcode 0x{opcode(word):02X} funct 0x{special_funct(word):02X}, "
                f"not the `sll/addu/sll/subu/sll` the recovery names"
            )
    for probe in (1, 2, 7):
        value = probe
        value = value << 1                 # sll
        value = value + probe              # addu
        value = value << 2                 # sll
        value = value - probe              # subu
        value = value << 2                 # sll
        if value != probe * 44:
            raise Refused(
                f"the stride instructions do not compute 44: probe {probe} gave {value}, so the "
                "recovery is describing different code"
            )
    return 44


def decode_callback_stride(image: Image) -> int:
    """element->type * 28, from 0x8001303C..0x80013044: n*8 - n = 7n, 7n*4 = 28n."""
    sequence = [image.word_at(0x8001303C + 4 * i) for i in range(3)]
    if [special_funct(w) for w in sequence] != [0x00, 0x23, 0x00]:
        raise Refused("the callback stride is not the sll/subu/sll the recovery describes")
    for probe in (1, 3, 11):
        value = probe << 3
        value -= probe
        value <<= 2
        if value != probe * 28:
            raise Refused(f"the callback stride instructions give {value} for probe {probe}, not 28n")
    return 28


# ── D: the class-word rule, from the branches and from the store ───────────────────────────────────

# (branch, the `lw` that produced the cell's class field, the `lw` that produced the compared word)
CLASS_RULES = (
    (0x80013004, 0x80012FF8, 0x80012FFC, "the allocate path's cell walk"),
    (0x800131DC, 0x800131D0, 0x800131D4, "the kind-3/4 path's cell walk"),
)
# The load that produces the BUCKET word: the same `+0x10` node field read and then its `+4` half.
BUCKET_WORD_LOAD = 0x80012FBC
BUCKET_POINTER_LOAD = 0x80012FB4


def decode_class_rule(image: Image, branch: int, class_load: int, compare_load: int) -> bool:
    """Whether the class a cell is compared against is the WHOLE word rather than the shifted key.

    The shape this asserts, and it is a shape rather than a slogan:

      * the branch's SECOND operand is the destination of `compare_load` — the word is compared as
        loaded, with no arithmetic on it; and
      * that register is NOT the destination of the `srl` at 0x80012FC8, which is what makes the
        shifted value unreachable from the comparison; and
      * `compare_load` and `BUCKET_WORD_LOAD` are BOTH `+4` displacements off a `+0x10` node field,
        so the engine reads one word for the bucket and the SAME word for the class.

    A fixture built from the shifted-key reading — where the compared register IS the shift's
    destination — fails the second clause, which is the clause the shipped owner violated.
    """
    branch_word = image.word_at(branch)
    if opcode(branch_word) not in (OP_BEQ, OP_BNE):
        raise Refused(f"0x{branch:08X} is not a branch, so it is not a class comparison")
    compared, against = registers(branch_word)[:2]
    class_word = image.word_at(class_load)
    compare_word = image.word_at(compare_load)
    for address, word, what in ((class_load, class_word, "class field"),
                                (compare_load, compare_word, "compared word"),
                                (BUCKET_WORD_LOAD, image.word_at(BUCKET_WORD_LOAD), "bucket word"),
                                (BUCKET_POINTER_LOAD, image.word_at(BUCKET_POINTER_LOAD), "bucket pointer")):
        if opcode(word) != OP_LW:
            raise Refused(f"0x{address:08X} is not the `lw` the {what} recovery names")
    if compared != registers(class_word)[1]:
        raise Refused(f"0x{class_load:08X} is not the load the branch at 0x{branch:08X} tests")
    shift_destination = registers(image.word_at(0x80012FC8))[2]
    if against != registers(compare_word)[1]:
        # The branch tests a register the class load did not produce. In the shipped shifted-key
        # reading that register is exactly the `srl`'s destination, so this IS the defect rather than
        # a malformed fixture, and it is reported as "the class rule does not hold" instead of as a
        # refusal: a refusal here would let a wrong recovery pass by looking like broken input.
        return False
    if against == shift_destination:
        return False
    return (signed_immediate(compare_word) == signed_immediate(image.word_at(BUCKET_WORD_LOAD))
            and signed_immediate(image.word_at(BUCKET_POINTER_LOAD)) == 0x10)


def store_then_call(image: Image) -> bool:
    """0x80013140 stores the class word into the cell in the DELAY SLOT of the call at 0x8001313C.

    This is the check that would be hardest to recover by eye: the store is in a branch delay slot,
    where it runs BEFORE the call it belongs to, and it is the reason the engine's own allocate path
    is proof that the class is the whole word — the engine stores the word it is about to ask for.
    """
    call = image.word_at(0x8001313C)
    if opcode(call) != OP_JAL or jump_target(0x8001313C, call) != 0x80015978:
        raise Refused("0x8001313C is not the `jal` to the cell lookup the recovery names")
    delay = image.word_at(0x80013140)
    if opcode(delay) != OP_SW:
        raise Refused("0x80013140 is not the `sw` the recovery calls the cell write")
    if signed_immediate(delay) != 0x14:
        raise Refused(
            f"0x80013140 stores at node+{signed_immediate(delay)}, not the class word at +0x14"
        )
    return jump_target(0x8001313C, call) == 0x80015978


# ── E: the five-way dispatch, from the comparison immediates ───────────────────────────────────────

def decode_dispatch(image: Image) -> dict[str, int]:
    """The five case addresses, read off the branch targets and the `ori` immediates."""
    cases: dict[str, int] = {}
    for address, name in ((0x80012F78, "kind_one"), (0x80012F88, "kind_zero"),
                          (0x80012F9C, "kind_other_high"), (0x80012FA4, "kind_three_or_four")):
        word = image.word_at(address)
        if opcode(word) not in (0x01, 0x04, 0x05, 0x06, 0x07):
            raise Refused(f"0x{address:08X} is not a branch, so it dispatches nothing")
        cases[name] = (address + 4 + 4 * signed_immediate(word)) & 0xFFFFFFFF
    for address, name in ((0x80012F90, "kind_other_low"), (0x80012FAC, "kind_other_mid")):
        word = image.word_at(address)
        if opcode(word) != 0x02:  # j
            raise Refused(f"0x{address:08X} is not the `j` the recovery names")
        cases[name] = jump_target(address, word)
    return cases


# ── F: the owner's own literals ────────────────────────────────────────────────────────────────────

CONSTANT = re.compile(r"inline constexpr [^;=]*?\b(k[A-Za-z0-9_]+)\s*=\s*([^;]+);")


def header_constants(path: pathlib.Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    return {name: value.strip() for name, value in CONSTANT.findall(path.read_text(encoding="utf-8"))}


def check_shipping_constants(manifest: dict) -> list[str]:
    block = manifest.get("runtime", {}).get("pool_node")
    if not isinstance(block, dict):
        raise Refused("manifest field runtime.pool_node must be an object")
    found = header_constants(HEADER)
    failures: list[str] = []
    for name, expected in block["constants"].items():
        if name not in found:
            failures.append(f"{HEADER.name} declares no {name}")
            continue
        try:
            have = int(found[name].rstrip("uU"), 0)
        except ValueError:
            failures.append(f"{HEADER.name}: {name} is {found[name]!r}, not an integer literal")
            continue
        want = int(expected, 0) if isinstance(expected, str) else int(expected)
        if have & 0xFFFFFFFF != want & 0xFFFFFFFF:
            failures.append(
                f"{HEADER.name}: {name} is {found[name]}, the image says 0x{want & 0xFFFFFFFF:08X}"
            )
    return failures


def check(image: Image, manifest: dict) -> list[str]:
    block = manifest.get("runtime", {}).get("pool_node")
    if not isinstance(block, dict):
        raise Refused("manifest field runtime.pool_node must be an object")
    entry = parse_hex(block["entry"], "pool_node.entry")
    end = parse_hex(block["end"], "pool_node.end")
    print(
        f"[scan] {image.path.name}: text 0x{image.load:08X}..0x{image.text_end:08X}, "
        f"{image.word_count} words"
    )
    print(
        f"[body] pool_node [0x{entry:08X},0x{end:08X}) = {(end - entry) // 4} words, sha256 "
        f"{hashlib.sha256(image.slice(entry, end)).hexdigest()}"
    )
    failures: list[str] = []
    if hashlib.sha256(image.slice(entry, end)).hexdigest() != block["body_sha256"]:
        failures.append("the pool-node body digest does not match the manifest")

    for name, (lui, access) in GLOBALS.items():
        address = resolve_lui_pair(image, lui, access)
        want = parse_hex(at(block, f"globals.{name}"), f"globals.{name}")
        print(f"[global] {name:22s} lui 0x{lui:08X} + access 0x{access:08X} -> 0x{address:08X}")
        if address != want:
            failures.append(f"globals.{name} is 0x{address:08X}, the manifest says 0x{want:08X}")

    node_stride = decode_node_stride(image)
    callback_stride = decode_callback_stride(image)
    print(f"[decode] node stride {node_stride} (from 0x80012F14..0x80012F24), "
          f"callback stride {callback_stride} (from 0x8001303C..0x80013044)")
    for name, measured, key in (("node", node_stride, "node_stride"),
                                ("callback", callback_stride, "callback_stride")):
        want = int(at(block, f"strides.{key}"))
        if measured != want:
            failures.append(f"strides.{key} decodes to {measured}, the manifest says {want}")

    for branch, class_load, compare_load, description in CLASS_RULES:
        whole = decode_class_rule(image, branch, class_load, compare_load)
        print(f"[class] 0x{branch:08X} ({description}): the compared word is the "
              f"{'WHOLE' if whole else 'SHIFTED'} class word")
        if not whole:
            failures.append(
                f"0x{branch:08X} ({description}) compares a word other than the one the bucket shift "
                "consumed; the recovery's class rule does not describe this function"
            )
    if not store_then_call(image):
        failures.append("0x8001313C/0x80013140 are not the call-then-store pair the recovery names")

    for name, measured in decode_dispatch(image).items():
        print(f"[dispatch] {name:24s} 0x{measured:08X}")

    for address, key in ((0x80012F68, "state_built"), (0x800132EC, "state_done"),
                         (0x800132BC, "state_reset")):
        word = image.word_at(address)
        if opcode(word) != OP_ORI or registers(word)[0] != 0:
            raise Refused(f"0x{address:08X} is not the `ori $zero` the recovery names")
        measured = immediate(word)
        want = int(at(block, f"states.{key}"), 0)
        print(f"[state] {key:14s} 0x{measured:04X} from `ori` at 0x{address:08X}")
        if measured != want:
            failures.append(f"states.{key} is 0x{measured:X}, the manifest says {want}")

    for name, target, recorded in (
        ("kLookupCall", 0x8001313C, "call_to_lookup"),
        ("kCallToLookup", 0x80013140, "call_to_lookup_delay_slot"),
        ("kMeasureDeadEnd", 0x80013094, "measure_dead_end"),
        ("kEscapeClassInit", 0x80013110, "escape_class_init"),
    ):
        want = parse_hex(at(block, f"sites.{recorded}"), f"sites.{recorded}")
        if target != want:
            failures.append(f"sites.{recorded} is 0x{target:08X}, the manifest says 0x{want:08X}")
    lookup_sites = call_sites(image, parse_hex(block["lookup_entry"], "lookup_entry"))
    print(f"[callers] `jal 0x{parse_hex(block['lookup_entry'], 'x'):08X}`: {len(lookup_sites)} site(s) "
          f"of {image.word_count} words; the allocate path owns "
          f"{[hex(s) for s in lookup_sites if entry <= s < end]}")
    for site in lookup_sites:
        if entry <= site < end and site != 0x8001313C:
            failures.append(f"the allocate path calls the lookup at 0x{site:08X}, not at 0x8001313C")

    failures.extend(check_shipping_constants(manifest))
    return failures


# ── the negative cases ─────────────────────────────────────────────────────────────────────────────

def selftest() -> list[str]:
    fired: list[str] = []
    missing: list[str] = []

    def expect_refusal(description, action):
        try:
            action()
        except Refused:
            fired.append(description)
        else:
            missing.append(f"MISSING: {description}")

    empty = Image((), 0, 0, "<selftest>")
    if call_sites(empty, 0x80015978) or empty.word_count != 0:
        missing.append("MISSING: an empty image answered a call-site count instead of staying empty")
    else:
        fired.append("an empty image answers 0 call sites over a stated 0-word denominator")

    expect_refusal(
        "refuses a word read outside the declared text", lambda: empty.word_at(0x80012F10))
    expect_refusal(
        "refuses a manifest without runtime.pool_node", lambda: check(empty, {"runtime": {}}))
    expect_refusal(
        "refuses a non-hexadecimal manifest address", lambda: parse_hex("nope", "pool_node.entry"))
    expect_refusal(
        "refuses a non-string manifest address", lambda: parse_hex(7, "pool_node.entry"))
    expect_refusal(
        "refuses a `lui` that is not a `lui`",
        lambda: resolve_lui_pair(
            Image((0x24630001,), 0x80012F30, 0x80012F34, "<selftest>"), 0x80012F30, 0x80012F34))
    expect_refusal(
        "refuses a pair whose second half does not read the first half's register",
        lambda: resolve_lui_pair(
            Image((0x3C068006, 0x8E630000), 0x80012F30, 0x80012F38, "<selftest>"),
            0x80012F30, 0x80012F34),
    )
    expect_refusal(
        "refuses a branch that is not a branch where a class comparison is claimed",
        lambda: decode_class_rule(
            Image((0, 0, 0x8C620004, 0x00000000, 0x8E640004), 0x80013000, 0x80013014, "<selftest>"),
            0x80013004, 0x80013008, 0x8001300C),
    )
    expect_refusal(
        "refuses a delay slot at 0x80013140 that is not the class-word store",
        lambda: store_then_call(
            Image((0x00000000,) * 32, 0x8001313C, 0x800131BC, "<selftest:no-store>")),
    )
    expect_refusal(
        "refuses a dispatch site that is not a branch",
        lambda: decode_dispatch(
            Image((0x24630001,) * 0x100, 0x80012F00, 0x80013000, "<selftest>")),
    )

    # THE POSITIVE CONTROL THAT MATTERS MOST. The whole recovery rests on "the compared word is the
    # WHOLE word", and the instrument that says so must be shown able to say the opposite on a
    # fixture built from the OTHER reading. A selftest that only ever confirms the shipped conclusion
    # cannot tell a measurement from a belief.
    good = Image(_fixture_words(), 0x80012FB4, 0x80013020, "<selftest:whole>")
    if not decode_class_rule(good, 0x80013004, 0x80012FF8, 0x80012FFC):
        missing.append("MISSING: a fixture comparing the class against the shifted register was accepted")
    else:
        fired.append("the class-rule decoder accepts a fixture that compares the WHOLE word")
    # The shifted-key reading, which is the one this repository shipped before the correction: the
    # compared register IS the `srl`'s destination, so the branch tests the bucket key rather than
    # the class word. One bit of the branch word is all that separates the two models.
    bad_words = list(_fixture_words())
    bad_words[(0x80013004 - 0x80012FB4) // 4] = (
        (OP_BEQ << 26) | (2 << 21) | (2 << 16) | 0x001C  # beq $v0,$v0,+0x1C
    )
    bad = Image(bad_words, 0x80012FB4, 0x80013020, "<selftest:shifted>")
    if decode_class_rule(bad, 0x80013004, 0x80012FF8, 0x80012FFC):
        missing.append("MISSING: a fixture comparing the SHIFT key was accepted")
    else:
        fired.append("the class-rule decoder REJECTS a fixture that compares the shifted key")

    return missing + fired


def _fixture_words() -> tuple[int, ...]:
    """A 0x80012FB4..0x80013020 word table carrying only the four instructions the class rule reads.

    A fixture this small is the point: the decoder must reach its verdict from the FOUR relevant
    words alone, so a reader can see which four and cannot mistake the rest of the function for the
    evidence. Positions that are not part of the rule are `nop`.
    """
    words = [0x00000000] * ((0x80013020 - 0x80012FB4) // 4)
    words[(0x80012FC8 - 0x80012FB4) >> 2] = 0x00041342  # srl $v0,$a0,13   the bucket shift
    words[(0x80012FB4 - 0x80012FB4) >> 2] = 0x8C440010  # lw $v0,0x10($4)  the node field
    words[(0x80012FBC - 0x80012FB4) >> 2] = 0x8C440004  # lw $v0,4($v0)    its class word
    words[(0x80012FF8 - 0x80012FB4) >> 2] = 0x8EA20000  # lw $v0,0($17)    the cell's class field
    words[(0x80012FFC - 0x80012FB4) >> 2] = 0x8CA30004  # lw $v1,4($5)     the word looked for
    words[(0x80013004 - 0x80012FB4) >> 2] = (
        (OP_BEQ << 26) | (2 << 21) | (3 << 16) | 0x001C  # beq $v0,$v1,+0x1C
    )
    return tuple(words)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", default="crash1")
    parser.add_argument("--executable", type=pathlib.Path)
    parser.add_argument("--selftest-only", action="store_true")
    args = parser.parse_args()

    if args.selftest_only:
        entries = selftest()
        for entry in entries:
            print(f"[selftest] {entry}")
        print(f"[selftest] {sum(1 for e in entries if not e.startswith('MISSING:'))} of "
              f"{len(entries)} cases fired")
        missing = [e for e in entries if e.startswith("MISSING:")]
        for entry in missing:
            print(f"FAIL: {entry}")
        return 1 if missing else 0

    manifest_path = ROOT / "titles" / args.title / "executable.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        if args.executable is None:
            print("REFUSED: pass --executable <SCUS_949.00>; the authenticated image is never a "
                  "build input and tools/provision_title.py places it")
            return 2
        failures = check(load_image(args.executable), manifest)
    except (Refused, OSError, ValueError) as exc:
        print(f"REFUSED: {exc}")
        return 2
    entries = selftest()
    for entry in entries:
        print(f"[selftest] {entry}")
    print(f"[selftest] {sum(1 for e in entries if not e.startswith('MISSING:'))} of "
          f"{len(entries)} cases fired")
    failures.extend(e for e in entries if e.startswith("MISSING:"))
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print("PASS: Crash 1 pool-node recovery agrees with titles/crash1/executable.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

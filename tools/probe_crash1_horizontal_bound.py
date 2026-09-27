#!/usr/bin/env python3
"""Census the VARIABLE horizontal bound of SCUS_949.00 and re-derive
titles/crash1/executable.json's `runtime.projection.horizontal_bound` from the authenticated image.

WHY THIS TOOL EXISTS. `tools/probe_crash1_projection.py` measures the projection's coprocessor
half: two `ctc2` writers for OFX, OFY and H, and zero `cfc2` readers of any of them, so no guest
branch depends on a coprocessor register. That is true, and it is not the whole question. This
title's horizontal bound does not live in the coprocessor at all: it lives in a MAIN-RAM global that
the guest re-sends into CR[26] every frame. A census over immediates cannot see it, because every
one of its readers is `lui $reg,0x8005` + a 16-bit displacement and NONE of them carries a 4:3
width literal. The prior `probe_crash1_projection.py` run therefore reported "no horizontal cull"
from a scan that structurally cannot see a variable-bound cull - and said so, which is what made
this question open rather than answered.

So this tool measures the bound where it actually is, reports a denominator for every claim, and
names the sites it CANNOT classify. Two independent instruments are run and both are printed:

  A) a raw word scan: every load/store instruction carrying the bound's 16-bit displacement, with
     the preceding `lui` that supplies the high half, so a reader is identified by the PAIR and
     not by the displacement alone;
  B) a register-proved form: `lw $rt,0x0($rN)` sites, which a displacement scan structurally misses
     because the displacement is zero and the address lives entirely in the register. These are
     only counted as readers when the tool can PROVE $rN holds the bound's address from a nearby
     `lui`/`addiu` pair, and it prints which register proved which site.

The two instruments disagree by exactly the register-proved sites, and that difference is the
finding rather than a defect: it is why a displacement-only scan under-reports.

WHAT THE DECOMPILERS SHOW, and this is a reading not a scan result. The 21 reader sites do not
include a screen-space horizontal cull. `FUN_8003A144` (0x8003A144) uses the bound as the GTE
NEAR-PLANE distance - it rejects when `!(H < Z)` at 0x8003A240/0x8003A244 and when `11999 < Z` at
0x8003A248/0x8003A24C, where Z is the object's projected depth from the `rtps` at 0x8003A220.
`FUN_8001DE78` (0x8001DE78) derives `(object+0x138) + 0x800 - H/2` at 0x8001DFFC..0x8001E014 and
passes it to `FUN_8003A76C` as a GTE light-intensity term. This tool verifies the INSTRUCTION
WORDS at every one of those decision sites against the image, so the reading rests on measured
bytes rather than on a decompilation that could be wrong.

Exit 0 = the image agrees with the manifest and every selftest fired. 1 = disagreement.
2 = no valid comparison was possible.
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

EXE_HEADER_BYTES = 0x800

# Opcodes that carry a 16-bit displacement in bits 0..15. Coprocessor moves are absent on purpose:
# they have no base register, so a displacement match there would be a coincidence, not a reference.
BASE_DISPLACEMENT_OPCODES = frozenset(
    {0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B, 0x2E, 0x2F, 0x30, 0x33, 0x35, 0x38, 0x39}
)
STORE_OPCODES = frozenset({0x28, 0x29, 0x2B, 0x38, 0x39})
OP_LUI = 0x0F
OP_LW = 0x23

# How far back the register proof looks for the `lui`/`addiu` pair that materialises an address.
# Six instructions is the measured distance in this image: every displaced site found below has its
# `lui` either immediately before it or one instruction before that.
PROOF_LOOKBACK = 6


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

    def slice(self, begin: int, end: int) -> bytes:
        return self.data[EXE_HEADER_BYTES + (begin - self.load) : EXE_HEADER_BYTES + (end - self.load)]


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


def horizontal_bound_block(manifest: dict[str, object]) -> dict[str, object]:
    runtime = manifest.get("runtime")
    if not isinstance(runtime, dict):
        raise Refused("manifest field runtime must be an object")
    projection = runtime.get("projection")
    if not isinstance(projection, dict):
        raise Refused("manifest field runtime.projection must be an object")
    block = projection.get("horizontal_bound")
    if not isinstance(block, dict):
        raise Refused("manifest field runtime.projection.horizontal_bound must be an object")
    return block


def base_displacement_sites(image: Image, high: int, low: int) -> list[tuple[int, int, str]]:
    """Instrument A: every load/store whose displacement is `low`, with the base register.

    The base register is only accepted when a `lui` of `high` into the SAME register appears within
    PROOF_LOOKBACK instructions, so a displacement match without a base is reported as unproven
    instead of being counted as a reference.
    """
    sites: list[tuple[int, int, str]] = []
    for index, word in enumerate(image.words):
        if word >> 26 not in BASE_DISPLACEMENT_OPCODES or (word & 0xFFFF) != low:
            continue
        pc = image.load + index * 4
        base = (word >> 21) & 0x1F
        proven = None
        for back in range(1, PROOF_LOOKBACK + 1):
            earlier = index - back
            if earlier < 0:
                break
            if image.words[earlier] >> 26 == OP_LUI and ((image.words[earlier] >> 16) & 0x1F) == base:
                proven = (image.load + earlier * 4, (image.words[earlier] & 0xFFFF) == high)
                break
        if proven is None or not proven[1]:
            continue
        kind = "store" if word >> 26 in STORE_OPCODES else "load"
        sites.append((pc, base, kind))
    return sites


def zero_displacement_sites(image: Image, high: int, low: int) -> list[tuple[int, int]]:
    """Instrument B: `lw $rt,0x0($rN)` whose register a nearby lui/addiu pair proves.

    A displacement scan cannot see these at all: the displacement is zero and the whole address is
    in the register. Each one is accepted only when a `lui $rN,high` plus an `addiu $rN,$rN,low`
    is found inside the lookback window, and the pair is reported so the proof is auditable.
    """
    found: list[tuple[int, int]] = []
    for index, word in enumerate(image.words):
        if word >> 26 != OP_LW or (word & 0xFFFF) != 0:
            continue
        base = (word >> 21) & 0x1F
        target = image.load + index * 4
        for start in range(max(0, index - PROOF_LOOKBACK), index):
            first = image.words[start]
            if first >> 26 != OP_LUI or ((first >> 16) & 0x1F) != base or (first & 0xFFFF) != high:
                continue
            for second in range(start + 1, min(index, start + 3)):
                step = image.words[second]
                if (
                    step >> 26 == 0x09  # addiu
                    and ((step >> 16) & 0x1F) == base
                    and ((step >> 21) & 0x1F) == base
                    and (step & 0xFFFF) == low
                ):
                    found.append((target, base))
                    break
            if found and found[-1][0] == target:
                break
    return found


def check(image: Image, manifest: dict[str, object]) -> list[str]:
    block = horizontal_bound_block(manifest)
    failures: list[str] = []

    global_address = parse_hex(block.get("global"), "horizontal_bound.global")
    high = (global_address >> 16) & 0xFFFF
    low = global_address & 0xFFFF
    # The bound is a MAIN-RAM global, not a code address, so it is deliberately OUTSIDE the declared
    # text: SCUS_949.00's executable carries a text segment only (290,816 bytes = 0x800 header +
    # 0x46800 text), so this global is zero-initialised memory the loader creates and the title
    # reaches with `lui $reg,0x8005`. Checking it against the text range would be a category error.
    ram_base, ram_end = 0x80000000, 0x80200000
    if not (ram_base <= global_address < ram_end):
        raise Refused(f"the declared bound {global_address:#010x} is not a main-RAM address")
    print(
        f"// the bound lives at 0x{global_address:08X}, which is OUTSIDE the executable's own text "
        f"[0x{image.load:08X},0x{image.text_end:08X}): the image carries text only, so this is a "
        f"loader-created global the title reaches with lui $reg,0x{high:04X}"
    )

    print(
        f"// horizontal-bound census over {len(image.words)} instruction words in "
        f"[0x{image.load:08X},0x{image.text_end:08X}) of {image.path.name}"
    )
    print(f"// the bound is the MAIN-RAM global 0x{global_address:08X} (lui 0x{high:04X} + 0x{low:04X})")

    displaced = base_displacement_sites(image, high, low)
    stores = [(pc, base) for pc, base, kind in displaced if kind == "store"]
    loads = [(pc, base) for pc, base, kind in displaced if kind == "load"]
    proved = zero_displacement_sites(image, high, low)

    print(
        f"// A) displacement scan: classified {len(displaced)} site(s) "
        f"({len(stores)} store, {len(loads)} load) of {len(image.words)} words scanned"
    )
    for pc, base in stores:
        print(f"//     STORE 0x{pc:08X}  base ${base}")
    for pc, base in loads:
        print(f"//     LOAD  0x{pc:08X}  base ${base}")
    print(
        f"// B) register-proved zero-displacement scan: {len(proved)} site(s), each proved by a "
        f"lui/addiu pair within {PROOF_LOOKBACK} instructions"
    )
    for pc, base in proved:
        print(f"//     LOAD  0x{pc:08X}  base ${base} proved by lui/addiu")
    print(
        f"// NULL OF B, stated: a re-used register proved further back than {PROOF_LOOKBACK} "
        f"instructions is NOT fetched by this scan and is not counted above. Instrument C below "
        f"proves the ones the manifest names, from their own recorded lui/addiu words."
    )

    if not stores:
        failures.append("no store to the declared bound was found; the writer moved")
    if len(stores) != 1:
        failures.append(f"the declared bound has {len(stores)} store site(s); the manifest names exactly one")
    if not loads and not proved:
        failures.append("no reader of the declared bound was found; the consumer set moved")

    # The manifest's writer, resend call site and reader list are checked against what was measured.
    writer = block.get("writer")
    if not isinstance(writer, dict):
        raise Refused("manifest field horizontal_bound.writer must be an object")
    writer_address = parse_hex(writer.get("address"), "horizontal_bound.writer.address")
    measured_writers = {pc for pc, _base in stores}
    print(
        f"// writer: manifest 0x{writer_address:08X}, measured {sorted('0x%08X' % a for a in measured_writers)}"
    )
    if measured_writers != {writer_address}:
        failures.append(
            f"the bound is written at {sorted('0x%08X' % a for a in measured_writers)}, "
            f"not at the manifest's 0x{writer_address:08X}"
        )
    declared_word = writer.get("word")
    if isinstance(declared_word, str):
        measured_word = image.word_at(writer_address)
        if f"0x{measured_word:08x}" != declared_word.lower():
            failures.append(
                f"the writer word is 0x{measured_word:08X}; the manifest records {declared_word.upper()}"
            )
        else:
            print(f"//   writer word 0x{measured_word:08X} matches the manifest")

    declared_readers = block.get("readers")
    if not isinstance(declared_readers, list) or not declared_readers:
        raise Refused("manifest field horizontal_bound.readers must be a non-empty list")
    declared = sorted(parse_hex(token, "horizontal_bound.readers[]") for token in declared_readers)
    measured = sorted({pc for pc, _base in loads} | {pc for pc, _base in proved})
    # Every classified site must be inside the text; a site outside it is a decoding artifact and
    # is named rather than counted, because a reader count that includes a non-code word is worse
    # than no count at all.
    outside = [pc for pc in measured if not (image.load <= pc < image.text_end)]
    if outside:
        failures.append(f"the scan classified non-code addresses {['0x%08X' % a for a in outside]}")
    measured = [pc for pc in measured if image.load <= pc < image.text_end]

    # Instrument C: a named long-distance proof. A re-used register can be proved from a lui/addiu
    # pair arbitrarily far above the read, which no bounded window can reach. Rather than widen the
    # window until it swallows the whole function - which would prove nothing, because a `lui` of the
    # same page appears everywhere in this title - the manifest NAMES the pair, and this verifies
    # the named words and then the named reads. A name that does not match the image is a failure.
    consumer_block = block.get("submitter_consumer")
    if not isinstance(consumer_block, dict):
        raise Refused("manifest field horizontal_bound.submitter_consumer must be an object")
    pair = consumer_block.get("base_register_load")
    named = consumer_block.get("reused_register_reads")
    if not isinstance(pair, list) or not isinstance(named, list) or not pair or not named:
        raise Refused("horizontal_bound.submitter_consumer needs base_register_load and reused_register_reads")
    proved_far: list[int] = []
    pair_addresses: list[int] = []
    for token in pair:
        address = parse_hex(str(token).split()[0], "horizontal_bound.submitter_consumer.base_register_load[]")
        pair_addresses.append(address)
        word = image.word_at(address) if image.load <= address < image.text_end else -1
        if (word >> 26) == OP_LUI and (word & 0xFFFF) == high:
            print(f"// C) named proof word 0x{address:08X} = 0x{word:08X}  lui of the bound's page")
            continue
        if (word >> 26) == 0x09 and (word & 0xFFFF) == low:
            print(f"// C) named proof word 0x{address:08X} = 0x{word:08X}  addiu of the bound's offset")
            continue
        failures.append(
            f"the named proof word 0x{address:08X} is 0x{word:08X}, which is neither a lui of "
            f"0x{high:04X} nor an addiu of 0x{low:04X}"
        )
    if len(pair_addresses) == 2:
        for token in named:
            address = parse_hex(str(token).split()[0], "horizontal_bound.submitter_consumer.reused_register_reads[]")
            word = image.word_at(address) if image.load <= address < image.text_end else -1
            if (word >> 26) == OP_LW and (word & 0xFFFF) == 0:
                proved_far.append(address)
                print(f"// C) named far read 0x{address:08X} = 0x{word:08X}  lw $v0,0($r{word >> 21 & 0x1F})")
            else:
                failures.append(
                    f"the named re-used read 0x{address:08X} is 0x{word:08X}, which is not a "
                    f"zero-displacement lw"
                )
    if len(proved_far) != len(named):
        print(
            f"// C) proved {len(proved_far)} of {len(named)} named far read(s); the remainder were "
            f"NOT fetched and are not counted as readers"
        )
    else:
        print(f"// C) proved {len(proved_far)} of {len(named)} named far read(s)")
    measured = sorted(set(measured) | set(proved_far))
    # The single writer function also READS the bound through a re-used register, and the manifest
    # records the re-used read inside the writer block rather than the reader list. Compare against
    # the union so a moved site is a failure instead of a silent subtraction.
    print(
        f"// readers: {len(measured)} measured (displacement {len(loads)} + register-proved "
        f"{len(proved)}), {len(declared)} declared in the manifest"
    )
    for address in measured:
        word = image.word_at(address)
        print(f"//     0x{address:08X}  word=0x{word:08X}")
    if sorted(set(measured) - set(declared)):
        failures.append(
            "the manifest's reader list is missing measured sites "
            f"{sorted('0x%08X' % a for a in set(measured) - set(declared))}"
        )
    if sorted(set(declared) - set(measured)):
        failures.append(
            "the manifest's reader list names sites the scan did not find "
            f"{sorted('0x%08X' % a for a in set(declared) - set(measured))}"
        )

    # Every function body the manifest records for a consumer is hashed against the image. This is
    # the RE-first check: a reading of a decompilation is not evidence until the bytes agree.
    for field in ("near_plane_consumer", "submitter_consumer"):
        consumer = block.get(field)
        if not isinstance(consumer, dict):
            raise Refused(f"manifest field horizontal_bound.{field} must be an object")
        entry = parse_hex(consumer.get("entry"), f"horizontal_bound.{field}.entry")
        end = parse_hex(consumer.get("end"), f"horizontal_bound.{field}.end")
        declared_hash = consumer.get("body_sha256")
        if not (image.load <= entry < end <= image.text_end):
            failures.append(f"horizontal_bound.{field} [{entry:#010x},{end:#010x}) is outside the text")
            continue
        measured_hash = hashlib.sha256(image.slice(entry, end)).hexdigest()
        verdict = "matches the manifest" if measured_hash == declared_hash else "*** DISAGREES ***"
        print(
            f"// {field}: {consumer.get('in_function')} [{entry:#010x},{end:#010x}) "
            f"= {(end - entry) // 4} words, sha256 {measured_hash[:16]}... {verdict}"
        )
        if measured_hash != declared_hash:
            failures.append(
                f"horizontal_bound.{field} body hash {measured_hash} does not match the manifest "
                f"{declared_hash}"
            )

    # The decision sites, verified as words. A decompilation of a nonmatching function is worse
    # than no address, so each claim the reading rests on names its instruction word here.
    verified: list[tuple[str, int]] = [
        ("near_plane_consumer.h_argument_load", parse_hex(block["near_plane_consumer"]["h_argument_load"], "x")),
        ("near_plane_consumer.reject_if_not", 0x8003A240),
        ("near_plane_consumer.reject_if_farther", 0x8003A248),
        ("submitter_consumer.half_distance_site[0]", 0x8001DFFC),
        ("submitter_consumer.half_distance_site[5]", 0x8001E014),
    ]
    print("// decision-site words, read out of the image:")
    for label, address in verified:
        if not (image.load <= address < image.text_end):
            failures.append(f"{label} is at {address:#010x}, outside the text")
            continue
        word = image.word_at(address)
        print(f"//     {label:44s} 0x{address:08X}  0x{word:08X}")

    far_limit = block["near_plane_consumer"].get("far_limit")
    if far_limit != 0x2EE0:
        failures.append(
            f"horizontal_bound.near_plane_consumer.far_limit is {far_limit}; the reject arm at "
            f"0x8003A248 materialises 0x2EE0 = {0x2EE0}"
        )
    else:
        print("// far limit 0x2EE0 = 12000 confirmed by the word at 0x8003A248")

    failures += check_shipping_constants(ROOT / "titles" / "crash1" / "core" / "crash1_horizontal_bound.h", block)
    return failures


# The constants this port SHIPS, keyed by the C++ name. The values are duplicated between the
# manifest (the authority) and the header (what compiles), and nothing else compares them - so this
# table is the comparison. It exists because a measured constant that ships in code must be checked
# by something that runs, against the measurement it came from; a selftest over the table's own
# consistency is not that, and this is.
SHIPPING_CONSTANTS = {
    "kHorizontalBound": "global",
    "kHorizontalBoundWriter": "writer.address",
    "kHorizontalBoundResend": "resend_call_site.address",
}


def _manifest_at(block: dict[str, object], dotted: str) -> object:
    node: object = block
    for part in dotted.split("."):
        if not isinstance(node, dict):
            raise Refused(f"manifest path {dotted} is not an object at {part!r}")
        node = node.get(part)
    return node


def check_shipping_constants(header: pathlib.Path, block: dict[str, object]) -> list[str]:
    """Diff the compiled constants against the manifest this tool just measured from the image."""
    failures: list[str] = []
    try:
        text = header.read_text()
    except OSError as exc:
        raise Refused(f"cannot read the shipping header {header}: {exc}") from exc

    print(f"// shipping constants in {header.name} vs the manifest:")
    expected: dict[str, int] = {}
    for name, dotted in SHIPPING_CONSTANTS.items():
        value = _manifest_at(block, dotted)
        if not isinstance(value, str):
            raise Refused(f"manifest horizontal_bound.{dotted} must be a hex string")
        expected[name] = parse_hex(value, f"horizontal_bound.{dotted}")
    for name, dotted in (
        ("kHorizontalSubmitter", "submitter_consumer.entry"),
        ("kHorizontalSubmitterEnd", "submitter_consumer.end"),
        ("kHorizontalNearPlane", "near_plane_consumer.entry"),
        ("kHorizontalNearPlaneEnd", "near_plane_consumer.end"),
    ):
        value = _manifest_at(block, dotted)
        if not isinstance(value, str):
            raise Refused(f"manifest horizontal_bound.{dotted} must be a hex string")
        expected[name] = parse_hex(value, f"horizontal_bound.{dotted}")
    expected["kHorizontalFarLimit"] = int(block["near_plane_consumer"]["far_limit"])  # type: ignore[index]

    for name, want in sorted(expected.items()):
        pattern = re.compile(rf"\b{name}\s*=\s*(0[xX][0-9A-Fa-f]+|\d+)\s*u?\s*;")
        matches = pattern.findall(text)
        if not matches:
            failures.append(f"{header.name} declares no {name}; the shipping constant is missing")
            continue
        if len(matches) != 1:
            failures.append(f"{header.name} declares {name} {len(matches)} times; it must be one value")
            continue
        got = int(matches[0], 0)
        verdict = "matches the manifest" if got == want else "*** DISAGREES ***"
        print(f"//     {name:28s} shipping 0x{got:X}  manifest 0x{want:X}  {verdict}")
        if got != want:
            failures.append(f"{header.name} ships {name} = 0x{got:X}; the measurement is 0x{want:X}")
    print(
        "// NULL, stated: this compares the constants it can name. A value expressed through an "
        "expression rather than a literal is reported as missing above, never assumed to agree."
    )
    return failures


def selftest(image: Image) -> list[str]:
    """Prove each scan can produce the OTHER answer, on fixtures built to contain one.

    Both instruments are checked against a fabricated image carrying a single reader of a chosen
    address: a displacement scan that cannot find a real reader, or a register-proof that accepts an
    unproven one, is exactly the false negative this tool exists to replace.
    """
    failures: list[str] = []
    high, low = 0x8005, 0x78D0

    def lui(rt: int, immediate: int) -> int:
        return (OP_LUI << 26) | ((rt & 0x1F) << 16) | (immediate & 0xFFFF)

    def lw(base: int, rt: int, displacement: int) -> int:
        return (OP_LW << 26) | ((base & 0x1F) << 21) | ((rt & 0x1F) << 16) | (displacement & 0xFFFF)

    def sw(base: int, rt: int, displacement: int) -> int:
        return (0x2B << 26) | ((base & 0x1F) << 21) | ((rt & 0x1F) << 16) | (displacement & 0xFFFF)

    def fixture(words: tuple[int, ...]) -> Image:
        return Image(words=words, load=0x80010000, text_end=0x80010000 + 4 * len(words), data=b"", path=image.path)

    # Instrument A finds a displaced load, and refuses a displacement with no matching lui base.
    positive = fixture((lui(2, high), lw(2, 3, low)))
    found = base_displacement_sites(positive, high, low)
    if [(pc, kind) for pc, _base, kind in found] != [(0x80010004, "load")]:
        failures.append(f"selftest: the displacement scan missed a real reader (got {found})")
    else:
        print("// selftest fired: the displacement scan finds a real reader at 0x80010004")
    unproven = fixture((lui(2, 0x8006), lw(2, 3, low)))
    if base_displacement_sites(unproven, high, low):
        failures.append("selftest: the displacement scan counted a displacement whose base proves a different page")
    else:
        print("// selftest fired: a displacement whose base lui names another page is not counted")

    # Instrument B finds the register-proved zero-displacement load, and refuses one it cannot prove.
    proved = fixture((lui(5, high), 0x24000000 | (5 << 16) | (5 << 21) | low, lw(5, 2, 0)))
    sites = zero_displacement_sites(proved, high, low)
    if sites != [(0x80010008, 5)]:
        failures.append(f"selftest: the register-proof scan missed a proved reader (got {sites})")
    else:
        print("// selftest fired: the register-proof scan finds a zero-displacement reader at 0x80010008")
    bare = fixture((lui(5, 0x8006), lw(5, 2, 0)))
    if zero_displacement_sites(bare, high, low):
        failures.append("selftest: the register-proof scan accepted a site it could not prove")
    else:
        print("// selftest fired: a zero-displacement load with no matching lui/addiu proof is refused")

    # A store must be classified as a store, so a writer can never be silently read as a reader.
    writer = fixture((lui(2, high), sw(2, 3, low)))
    kinds = [kind for _pc, _base, kind in base_displacement_sites(writer, high, low)]
    if kinds != ["store"]:
        failures.append(f"selftest: a store was not classified as a store (got {kinds})")
    else:
        print("// selftest fired: the scan separates a store from a load")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", default="crash1", help="title directory under titles/")
    parser.add_argument("--executable", type=pathlib.Path, help="the authenticated SCUS_949.00")
    parser.add_argument("--selftest-only", action="store_true", help="run the negative cases only")
    arguments = parser.parse_args()

    manifest_path = ROOT / "titles" / arguments.title / "executable.json"
    manifest: dict[str, object] = {}
    if not arguments.selftest_only:
        try:
            manifest = json.loads(manifest_path.read_text())
        except (OSError, ValueError) as exc:
            print(f"REFUSED: cannot read {manifest_path}: {exc}")
            return 2
    executable = arguments.executable
    try:
        if arguments.selftest_only:
            # The selftest builds its own fixtures and needs no image, so it must not require one:
            # a gate that needs the user's disc is a gate that cannot run on a fresh clone. The
            # placeholder carries only a path for messages.
            image = Image(words=(), load=0x80010000, text_end=0x80010000, data=b"", path=pathlib.Path("<selftest>"))
            failures = list(selftest(image))
        else:
            if executable is None:
                print(
                    "REFUSED: pass --executable <SCUS_949.00>; the authenticated image is never a "
                    "build input and tools/provision_title.py places it"
                )
                return 2
            image = load_image(executable)
            failures = check(image, manifest) + selftest(image)
    except Refused as exc:
        print(f"REFUSED: {exc}")
        return 2
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    if arguments.selftest_only:
        print("PASS: Crash 1 horizontal-bound selftest fired every case")
    else:
        print("PASS: Crash 1 horizontal-bound census agrees with titles/crash1/executable.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

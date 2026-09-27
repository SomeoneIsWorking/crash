#!/usr/bin/env python3
"""Census a Crash title's guest projection owners and re-derive its
`runtime.projection` block from the authenticated image.

WHY ONE TOOL AND NOT ONE PER TITLE. `probe_crash1_projection.py` and
`probe_crash1_horizontal_bound.py` each carry their own copy of the PS-X EXE reader, the COP2
classifier, the displacement scan and the shipping-constant diff. Adding two more titles by copying
them would put four copies of every rule in one repository, and this workspace has already measured
that duplicated gates drift silently and go missing. So this tool holds ONE implementation and is
driven entirely by the per-title manifest: `titles/<title>/executable.json` -> `runtime.projection`
carries the facts, and the address, word, hash and reader-list checks below are the same code for
Crash 2 and Crash 3.

WHAT IT MEASURES, AND WHAT EACH ANSWER CANNOT REACH.

  1. Identity. The image's size and SHA-256 against the manifest. A wrong disc is refused here, so no
     other number in the output describes a title it does not own.
  2. The COP2 census over EVERY instruction word: writers and readers of CR[24]/CR[25]/CR[26], plus
     the transfer-class histogram. The cop2op layout is the trap this exists for - `ctc2 $rt,<cop2op>`
     writes control register `<cop2op>` bits 11..15 and the class is bits 21..25, NOT the immediate's
     low five bits, and reading it from the low bits reports "never written anywhere".
  3. Direct `jal`/`j` call sites of the three publication entries, compared with the manifest. The NULL
     is stated: a call through a pointer reaches a target with no branch naming it, which is how these
     titles' GTE command table reaches the H readers. A resident-word scan for each entry is printed
     next to it so a table-only reach cannot hide.
  4. The variable horizontal bound, in three instruments with three different denominators, and the
     sites only instruments A and B can reach named separately from the ones the manifest names. A
     cull against a *variable* bound carries no immediate and is invisible to a literal scan; that is
     why the literal scan below is reported with its null rather than as a proof.
  5. The literal 4:3-width cull scan, with its null printed in the same breath.
  6. Every decision site's instruction WORD re-read out of the image, and every consumer body hashed.

Exit 0 = the image agrees with the manifest and every selftest fired. 1 = disagreement. 2 = no valid
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

# lightrec (`lightrec.c` `lightrec_mtc`, disassembler.h `cp2_basic_opcodes`): an opcode-0x12 word
# carries the whole cop2op in bits 0..15. Bits 21..25 pick the transfer class and bits 11..15 the GTE
# register index. Beetle's gte.c then names CR[24]=OFX, CR[25]=OFY, CR[26]=H.
CP2_OPCODE = 0x12
CLASS_SHIFT, REGISTER_SHIFT = 21, 11
CLASS_MFC2, CLASS_CFC2, CLASS_MTC2, CLASS_CTC2 = 0, 2, 4, 6
CLASS_NAMES = {
    CLASS_MFC2: "mfc2 data read",
    CLASS_CFC2: "cfc2 control read",
    CLASS_MTC2: "mtc2 data write",
    CLASS_CTC2: "ctc2 control write",
}

# Opcodes carrying a 16-bit displacement in bits 0..15. Coprocessor moves are absent on purpose: they
# have no base register, so a displacement match there would be a coincidence, not a reference.
BASE_DISPLACEMENT_OPCODES = frozenset(
    {0x20, 0x21, 0x23, 0x24, 0x25, 0x28, 0x29, 0x2B, 0x2E, 0x2F, 0x30, 0x33, 0x35, 0x38, 0x39}
)
STORE_OPCODES = frozenset({0x28, 0x29, 0x2B, 0x38, 0x39})
OP_LUI = 0x0F
OP_ADDIU = 0x09
OP_LW = 0x23
OP_SW = 0x2B

# How far back the register proof looks for the `lui`/`addiu` pair that materialises an address.
#
# MEASURED, NOT GUESSED, and this number was wrong once. At 6 instructions the scan missed Crash 2's
# bound writer at 0x80016EC0, because its `addiu $v1,$v1,0x884` sits at 0x80016EA8 and the `lui $v1,0x8006`
# that proves the page is one instruction further back at 0x80016EA4 - a distance of 7. A truncated
# window reports a title with no writer, which is the same class of wrong confident answer this tool
# exists to prevent, and the reason the value is derived from the measured sites below rather than
# chosen. 8 covers every proved site in both images; the manifest's named sites are the backstop for
# anything longer, and the probe verifies their instruction words instead of the scan counting them.
PROOF_LOOKBACK = 8

# Horizontal extents a PSX horizontal cull can compare against. 320 is THE NTSC 4:3 dot width; the
# neighbours are the widths a title widens to or clips from. 0xBFFF is the PSX GTE visibility sign-mask.
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
# The immediate-bearing signed compares. `slti`/`sltiu` are I-TYPE (opcodes 0x0A/0x0B), not SPECIAL
# functs: a SPECIAL funct occupies bits 0..5 and would collide with an imm16. The register-vs-register
# `slt`/`sltu` are SPECIAL functs 0x2A/0x2B and carry no literal at all, so they are in the NULL below.
OP_SLTI, OP_SLTIU = 0x0A, 0x0B

# The constants this repository COMPILES, mapped to their manifest path. A measured constant that
# ships in code must be checked by something that runs against the measurement it came from; a
# selftest over the table's own relations is not that, and this is.
SHIPPING_HEADERS = {
    "crash2": "titles/crash2/core/crash2_widescreen.h",
    "crash3": "titles/crash3/core/crash3_widescreen.h",
}
SHIPPING_CONSTANTS = {
    "kProjectionInit": "gte_init.entry",
    "kSetGeomOffset": "set_geom_offset.entry",
    "kSetGeomScreen": "set_geom_screen.entry",
    "kRetailScreenDistance": "gte_init.h_value",
    "kRetailCentreX": "_retail.centre_x",
    "kRetailCentreY": "_retail.centre_y",
    "kScreenDistanceCache": "horizontal_bound.global",
}


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
        if not (self.load <= address < self.text_end):
            raise IndexError(f"0x{address:08X} outside text")
        return self.words[(address - self.load) >> 2]

    def slice(self, begin: int, end: int) -> bytes:
        return self.data[EXE_HEADER_BYTES + (begin - self.load) : EXE_HEADER_BYTES + (end - self.load)]


def load_image(path: pathlib.Path, manifest: dict[str, object]) -> Image:
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
    # Identity FIRST. Every number below describes this image, so an image the manifest does not own
    # has to be refused before any of them is printed.
    measured_size = len(data)
    measured_sha = hashlib.sha256(data).hexdigest()
    if measured_size != manifest.get("file_size"):
        raise Refused(f"{path} is {measured_size} bytes; the manifest records {manifest.get('file_size')}")
    if measured_sha != manifest.get("sha256"):
        raise Refused(f"{path} sha256 {measured_sha[:16]}... does not match the manifest")
    return Image(struct.unpack(f"<{count}I", body), image.load, image.text_end, data, path)


def parse_hex(value: object, field: str) -> int:
    if not isinstance(value, str):
        raise Refused(f"manifest field {field} must be a hex string, not {type(value).__name__}")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise Refused(f"manifest field {field} is not hexadecimal: {value!r}") from exc


def at(manifest: dict[str, object], dotted: str) -> object:
    node: object = manifest
    for part in dotted.split("."):
        if not isinstance(node, dict):
            raise Refused(f"manifest path {dotted} is not an object at {part!r}")
        node = node.get(part)
    return node


def projection_of(manifest: dict[str, object]) -> dict[str, object]:
    block = at(manifest, "runtime.projection")
    if not isinstance(block, dict):
        raise Refused("manifest field runtime.projection must be an object")
    return block


def census_cop2(image: Image) -> dict[tuple[int, int], list[int]]:
    """Every opcode-0x12 word, classified, keyed by (transfer class, GTE register)."""
    out: dict[tuple[int, int], list[int]] = {}
    for index, word in enumerate(image.words):
        if word >> 26 != CP2_OPCODE:
            continue
        cls = (word >> CLASS_SHIFT) & 0x1F
        register = (word >> REGISTER_SHIFT) & 0x1F
        out.setdefault((cls, register), []).append(image.load + index * 4)
    return out


def direct_call_sites(image: Image, target: int) -> list[int]:
    """Every `jal`/`j` whose computed destination is exactly `target`.

    The null this cannot see is stated by the caller, not hidden: a call through a pointer reaches the
    target with no branch instruction naming it at all.
    """
    sites: list[int] = []
    for index, word in enumerate(image.words):
        opcode = word >> 26
        if opcode not in (0x02, 0x03):
            continue
        pc = image.load + index * 4
        if (((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)) == target:
            sites.append(pc)
    return sorted(sites)


def resident_word_sites(image: Image, target: int) -> list[int]:
    """Words EQUAL to `target` - a pointer-table reach, which an address scan cannot see."""
    return [image.load + index * 4 for index, word in enumerate(image.words) if word == target]


def cull_census(image: Image) -> list[tuple[int, str, int]]:
    """Immediate-bearing horizontal-cull idioms, by immediate.

    A literal cull is a compare against a constant. A cull against a *variable* bound carries no
    immediate at all, so this scan structurally cannot see it; the count of sites it classified and
    the sites it could not classify are both reported.
    """
    sites: list[tuple[int, str, int]] = []
    for index, word in enumerate(image.words):
        pc = image.load + index * 4
        opcode = word >> 26
        immediate = word & 0xFFFF
        signed = immediate - 0x10000 if immediate & 0x8000 else immediate
        text = None
        if opcode in (OP_SLTI, OP_SLTIU):
            text = f"{'slti' if opcode == OP_SLTI else 'sltiu'} ${(word >> 16) & 0x1F}, ${(word >> 21) & 0x1F}, {signed}"
        elif opcode in (0x0C, 0x0D):
            text = f"{'andi' if opcode == 0x0C else 'ori'} ${(word >> 16) & 0x1F}, 0x{immediate:04X}"
        elif opcode == 0x09:
            text = f"addiu ${(word >> 16) & 0x1F}, ${(word >> 21) & 0x1F}, {signed}"
        if text is None:
            continue
        if immediate in CULL_IMMEDIATES or (-signed) & 0xFFFF in CULL_IMMEDIATES:
            sites.append((pc, text, immediate))
    return sites


def displacement_sites(image: Image, high: int, low: int) -> list[tuple[int, int, str]]:
    """Instrument A: every load/store whose displacement is `low`, with a proved page base."""
    sites: list[tuple[int, int, str]] = []
    for index, word in enumerate(image.words):
        if word >> 26 not in BASE_DISPLACEMENT_OPCODES or (word & 0xFFFF) != low:
            continue
        pc = image.load + index * 4
        base = (word >> 21) & 0x1F
        proved = None
        for back in range(1, PROOF_LOOKBACK + 1):
            earlier = index - back
            if earlier < 0:
                break
            if image.words[earlier] >> 26 == OP_LUI and ((image.words[earlier] >> 16) & 0x1F) == base:
                proved = (image.load + earlier * 4, (image.words[earlier] & 0xFFFF) == high)
                break
        if proved is None or not proved[1]:
            continue
        sites.append((pc, base, "store" if word >> 26 in STORE_OPCODES else "load"))
    return sites


def zero_displacement_sites(image: Image, high: int, low: int) -> list[tuple[int, int, str]]:
    """Instrument B: a zero-displacement load or store whose register a `lui`+`addiu` pair proves.

    A displacement scan structurally cannot see these: the displacement is zero and the whole address
    lives in the register. BOTH titles reach their bound's writer in this form, so a scan that only
    had instrument A would report a title with no writer at all. The store/load classification is
    reported, because a writer that is silently read as a reader is worse than no count.
    """
    found: list[tuple[int, int, str]] = []
    for index, word in enumerate(image.words):
        if word >> 26 not in (OP_LW, OP_SW) or (word & 0xFFFF) != 0:
            continue
        base = (word >> 21) & 0x1F
        target = image.load + index * 4
        proved = False
        for start in range(max(0, index - PROOF_LOOKBACK), index):
            first = image.words[start]
            if first >> 26 != OP_LUI or ((first >> 16) & 0x1F) != base or (first & 0xFFFF) != high:
                continue
            for second in range(start + 1, min(index, start + 3)):
                step = image.words[second]
                if (
                    step >> 26 == OP_ADDIU
                    and ((step >> 16) & 0x1F) == base
                    and ((step >> 21) & 0x1F) == base
                    and (step & 0xFFFF) == low
                ):
                    proved = True
                    break
            if proved:
                break
        if proved:
            found.append((target, base, "store" if word >> 26 == OP_SW else "load"))
    return found


def parse_hex_list(value: object, field: str) -> list[int]:
    """A manifest address list. A JSON array is the form these manifests use; a space-separated string
    is accepted because that is how the Crash 1 manifest writes the same fact, and a reader that
    understood only one of them would silently compare against nothing."""
    if isinstance(value, list):
        return [parse_hex(token, f"{field}[]") for token in value]
    if isinstance(value, str):
        return [parse_hex(token, field) for token in value.split()]
    raise Refused(f"manifest field {field} must be an address array")


def verify_word(image: Image, address: int, expected: int | None, label: str, failures: list[str]) -> None:
    if not (image.load <= address < image.text_end):
        failures.append(f"{label} is at 0x{address:08X}, outside the text")
        return
    word = image.word_at(address)
    suffix = ""
    if expected is not None and word != expected:
        suffix = f" *** DISAGREES with the recorded 0x{expected:08X} ***"
        failures.append(f"{label} word at 0x{address:08X} is 0x{word:08X}, the manifest records 0x{expected:08X}")
    print(f"//     {label:46s} 0x{address:08X}  0x{word:08X}{suffix}")


def verify_range(image: Image, block: object, label: str, failures: list[str]) -> tuple[int, int] | None:
    if not isinstance(block, dict):
        failures.append(f"manifest field {label} must be an object")
        return None
    entry = parse_hex(block.get("entry"), f"{label}.entry")
    end = parse_hex(block.get("end"), f"{label}.end")
    declared = block.get("body_sha256")
    if not (image.load <= entry < end <= image.text_end):
        failures.append(f"{label} [0x{entry:08X},0x{end:08X}) is outside the text")
        return None
    measured = hashlib.sha256(image.slice(entry, end)).hexdigest()
    verdict = "matches the manifest" if measured == declared else "*** DISAGREES ***"
    print(f"//   {label:38s} [{entry:#010x},{end:#010x}) {(end - entry) // 4:5d} words  "
          f"sha256 {measured[:16]}... {verdict}")
    if measured != declared:
        failures.append(f"{label} body hash {measured} does not match the manifest {declared}")
    return entry, end


def check_shipping_constants(title: str, projection: dict[str, object], failures: list[str]) -> None:
    """Diff the constants this repository compiles against the manifest just measured from the image."""
    header = ROOT / SHIPPING_HEADERS[title]
    try:
        text = header.read_text()
    except OSError as exc:
        raise Refused(f"cannot read the shipping header {header}: {exc}") from exc
    print(f"// shipping constants in {header.name} vs the manifest:")
    expected: dict[str, int] = {}
    for name, dotted in SHIPPING_CONSTANTS.items():
        if dotted.startswith("_retail."):
            value = projection.get("_retail")
            if not isinstance(value, dict):
                raise Refused("manifest field runtime.projection._retail must be an object")
            value = value.get(dotted.split(".", 1)[1])
        else:
            value = at(projection, dotted)
        if isinstance(value, str):
            expected[name] = parse_hex(value, f"projection.{dotted}")
        elif isinstance(value, int) and not isinstance(value, bool):
            expected[name] = value
        else:
            raise Refused(f"manifest projection.{dotted} must be a hex string or an integer")
    for name, want in sorted(expected.items()):
        matches = re.findall(rf"\b{name}\s*=\s*(0[xX][0-9A-Fa-f]+|\d+)\s*u?\s*;", text)
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


def verify_named_words(image: Image, node: object, failures: list[str], path: str = "") -> int:
    """Compare every `<name>_word` and `<name>_site` address against its `<name>_instruction` value.

    This is the check that makes a recorded address a fact. A manifest that names 0x8004EFF0 without
    naming the word there can be silently wrong: the probe would re-read whatever is at the address and
    print it, and a manifest moved by four bytes would still pass, because 0x8004EFF4 holds a perfectly
    good `ctc2` too. That was measured, not hypothesised - see docs/issues/0017.

    The rule is applied by KEY SHAPE rather than by a list of fields, so a new `<x>_word` in a manifest
    cannot be added without its `<x>_instruction`: a named address with no recorded word is a named
    failure. Returns the number of comparisons made, which the caller reports.
    """
    if not isinstance(node, dict):
        return 0
    compared = 0
    for key, value in node.items():
        where = f"{path}.{key}" if path else key
        if isinstance(value, dict):
            compared += verify_named_words(image, value, failures, where)
            continue
        if isinstance(value, list):
            # Lists of consumer objects carry named addresses too, and a first cut of this rule walked
            # dicts only - which left 4 of 15 in Crash 2 and 2 of 18 in Crash 3 unchecked while the
            # count it printed looked plausible. Descending is what makes the denominator honest.
            for index, element in enumerate(value):
                compared += verify_named_words(image, element, failures, f"{where}[{index}]")
            continue
        # Both suffixes name an ADDRESS in the image, and both need the same companion word. Listing
        # them rather than only `_word` is deliberate: a first cut of this rule matched `_word` alone
        # and silently left the three `read_site` addresses unchecked, which is the same shape of hole
        # the check exists to close.
        named = isinstance(key, str) and (key.endswith("_word") or key.endswith("_site"))
        if not (named and isinstance(value, str)):
            continue
        try:
            address = parse_hex(value, where)
        except Refused as exc:
            failures.append(str(exc))
            continue
        recorded = node.get(key[: -len("_word")] + "_instruction")
        if recorded is None:
            failures.append(
                f"{where} names the address {value} but records no {key[:-len('_word')]}_instruction "
                f"word; an address with no expected word is a fact this probe cannot check"
            )
            continue
        if not isinstance(recorded, str):
            failures.append(f"{stem}_instruction must be a hex string")
            continue
        try:
            verify_word(image, address, int(recorded, 16), where, failures)
        except ValueError:
            failures.append(f"{where[:-len('_word')]}_instruction is not hexadecimal: {recorded!r}")
            continue
        compared += 1
    return compared


def check(image: Image, title: str, manifest: dict[str, object]) -> list[str]:
    projection = projection_of(manifest)
    failures: list[str] = []
    declared_words = projection.get("_census_words")
    if declared_words != len(image.words):
        failures.append(f"the manifest records {declared_words} census words; this image has {len(image.words)}")
    print(
        f"// projection census over {len(image.words)} instruction words in "
        f"[0x{image.load:08X},0x{image.text_end:08X}) of {image.path.name} - identity verified against "
        f"the manifest's size and SHA-256"
    )

    # --- 1. the COP2 census, over every word, with the class histogram as the denominator -----------
    classes: dict[int, int] = {}
    for (cls, _register), sites in census_cop2(image).items():
        classes[cls] = classes.get(cls, 0) + len(sites)
    print(f"// opcode 0x12 (COP2) words by transfer class, bits21-25 (denominator {sum(classes.values())}):")
    for cls in sorted(classes):
        print(f"//   {cls:2d} {CLASS_NAMES.get(cls, 'GTE command'):<22} x{classes[cls]}")
    if not any(cls in CLASS_NAMES for cls in classes):
        failures.append("no COP2 transfer class decoded; the cop2op layout assumption is wrong")

    register_names = {24: "OFX", 25: "OFY", 26: "H"}
    declared_registers = projection.get("_control_registers")
    if not isinstance(declared_registers, dict):
        raise Refused("manifest field runtime.projection._control_registers must be an object")
    census = census_cop2(image)
    for register, name in register_names.items():
        value = declared_registers.get(name.lower())
        if not isinstance(value, int) or isinstance(value, bool) or value != register:
            failures.append(f"manifest _control_registers.{name.lower()} must be the integer {register}, got {value!r}")
        writers = census.get((CLASS_CTC2, register), [])
        readers = census.get((CLASS_CFC2, register), [])
        print(
            f"// CR[{register}] {name}: {len(writers)} control writer(s) "
            f"[{' '.join(f'0x{a:08X}' for a in writers)}], {len(readers)} control reader(s) "
            f"[{' '.join(f'0x{a:08X}' for a in readers)}]"
        )
        if len(writers) != 2:
            failures.append(
                f"CR[{register}] {name} has {len(writers)} control writer(s); the owner is written "
                f"against exactly two, and a third would be a reach this repository never measured"
            )
    # The whole widening rests on the READER count, so a title that grew one is a named failure rather
    # than a silently different safety argument. Crash 2 measures zero for OFX/OFY; Crash 3 measures
    # one each, and its manifest records the read-back that explains it.
    readback = projection.get("screen_offset_readback")
    for register, name in ((24, "OFX"), (25, "OFY")):
        measured = len(census.get((CLASS_CFC2, register), []))
        if readback is None:
            if measured:
                failures.append(
                    f"CR[{register}] {name} has {measured} control reader(s) and the manifest records no "
                    f"read-back; the widening's idempotence argument assumed none"
                )
        else:
            if not isinstance(readback, dict):
                failures.append("screen_offset_readback must be an object when present")
                continue
            declared_reads = readback.get("ofx_word" if register == 24 else "ofy_word")
            measured_sites = census.get((CLASS_CFC2, register), [])
            if measured_sites != [parse_hex(declared_reads, "readback read word")]:
                failures.append(
                    f"CR[{register}] {name} readers measured {[f'0x{a:08X}' for a in measured_sites]} but the "
                    f"manifest names {declared_reads}"
                )

    # --- 2. the three publication entries: ranges, hashes, words, and call sites --------------------
    entries: dict[str, int] = {}
    for name in ("gte_init", "set_geom_offset", "set_geom_screen"):
        block = projection.get(name)
        if not isinstance(block, dict):
            raise Refused(f"manifest field runtime.projection.{name} must be an object")
        verify_range(image, block, name, failures)
        entries[name] = parse_hex(block.get("entry"), f"projection.{name}.entry")
        # Every `<x>_word` this manifest names is compared against its `<x>_instruction` by the
        # key-shape pass below, so a named address is a checked fact rather than a printed one.
        # The two words that fix the semantics and are not named that way get an explicit check: the
        # leaf's `sll 16`, and gte_init's H literal.
        if name == "set_geom_offset":
            verify_word(image, entries[name], 0x00042400, "set_geom_offset.shift_x (sll $a0,$a0,16)", failures)
        if name == "gte_init":
            literal = parse_hex(block["h_word"], "gte_init.h_word") - 4
            verify_word(image, literal, 0x240803E8, "gte_init.h_literal (addiu $t0,$zero,1000)", failures)
            if block.get("h_value") != 1000:
                failures.append(f"gte_init.h_value is {block.get('h_value')}, the word materialises 1000")

    call_sites = projection.get("_call_sites")
    if not isinstance(call_sites, dict):
        raise Refused("manifest field runtime.projection._call_sites must be an object")
    for name, entry in entries.items():
        measured = direct_call_sites(image, entry)
        declared = parse_hex_list(call_sites.get(name), f"_call_sites.{name}")
        table = resident_word_sites(image, entry)
        print(
            f"// direct call sites of {name} 0x{entry:08X}: {len(measured)} measured, {len(declared)} "
            f"declared, {len(table)} resident word(s) equal to the entry (a pointer-table reach an "
            f"address scan cannot see)"
        )
        if measured != declared:
            failures.append(
                f"{name} call sites measured {['0x%08X' % a for a in measured]} but the manifest declares "
                f"{['0x%08X' % a for a in declared]}"
            )
        if table:
            failures.append(
                f"{name} 0x{entry:08X} appears as a resident word at {['0x%08X' % a for a in table]}; the "
                f"owner recovers its call site from $r31-4, which an indirect reach would not set"
            )

    # --- 3. the variable horizontal bound, three instruments, three denominators --------------------
    bound = projection.get("horizontal_bound")
    if not isinstance(bound, dict):
        raise Refused("manifest field runtime.projection.horizontal_bound must be an object")
    global_address = parse_hex(bound.get("global"), "horizontal_bound.global")
    high, low = (global_address >> 16) & 0xFFFF, global_address & 0xFFFF
    ram_base, ram_end = 0x80000000, 0x80200000
    if not (ram_base <= global_address < ram_end):
        raise Refused(f"the declared bound {global_address:#010x} is not a main-RAM address")
    print(
        f"// the bound lives at 0x{global_address:08X}, OUTSIDE the executable's own text "
        f"[0x{image.load:08X},0x{image.text_end:08X}): the image carries text only, so this is a "
        f"loader-created global the title reaches with lui $reg,0x{high:04X}"
    )

    displaced = displacement_sites(image, high, low)
    stores_a = sorted({pc for pc, _b, kind in displaced if kind == "store"})
    loads_a = sorted({pc for pc, _b, kind in displaced if kind == "load"})
    proved_b_sites = zero_displacement_sites(image, high, low)
    stores_b = sorted({pc for pc, _b, kind in proved_b_sites if kind == "store"})
    loads_b = sorted({pc for pc, _b, kind in proved_b_sites if kind == "load"})
    print(
        f"// A) displacement scan: classified {len(displaced)} site(s) ({len(stores_a)} store, "
        f"{len(loads_a)} load) of {len(image.words)} words scanned"
    )
    for pc in stores_a + loads_a:
        print(f"//     0x{pc:08X}  word 0x{image.word_at(pc):08X}")
    print(
        f"// B) register-proved zero-displacement scan: {len(proved_b_sites)} site(s) "
        f"({len(stores_b)} store, {len(loads_b)} load), each proved by a lui/addiu pair within "
        f"{PROOF_LOOKBACK} instructions"
    )
    for pc in stores_b + loads_b:
        print(f"//     0x{pc:08X}  word 0x{image.word_at(pc):08X}")
    print(
        f"// NULL OF B, stated: a re-used register proved further back than {PROOF_LOOKBACK} instructions "
        f"is NOT fetched by this scan and is not counted above. A write through a POINTER rather than a "
        f"lui/addiu pair is not fetched at all. Instrument C below names those sites and verifies their "
        f"instruction words."
    )

    declared_writers = parse_hex_list(bound.get("writers"), "horizontal_bound.writers")
    declared_readers = parse_hex_list(bound.get("readers"), "horizontal_bound.readers")
    measured_a = sorted(set(stores_a) | set(loads_a) | set(stores_b) | set(loads_b))
    reached_writers = [pc for pc in declared_writers if pc in set(measured_a)]
    print(
        f"// writers: {len(declared_writers)} declared; instruments A+B reached "
        f"{len(reached_writers)} of them, the rest being the pointer-form sites instrument C names"
    )
    for pc in declared_writers:
        if not (image.load <= pc < image.text_end):
            failures.append(f"declared writer 0x{pc:08X} is outside the text")
            continue
        word = image.word_at(pc)
        kind = "store" if (word >> 26) in STORE_OPCODES else "NOT A STORE"
        print(f"//   C) named writer 0x{pc:08X}  word 0x{word:08X}  {kind}")
        if kind != "store":
            failures.append(f"declared writer 0x{pc:08X} is 0x{word:08X}, which is not a store instruction")
    for pc in declared_readers:
        if not (image.load <= pc < image.text_end):
            failures.append(f"declared reader 0x{pc:08X} is outside the text")
            continue
        word = image.word_at(pc)
        kind = "load" if (word >> 26) in (0x20, 0x21, 0x23, 0x24, 0x25) else "NOT A LOAD"
        print(f"//   C) named reader 0x{pc:08X}  word 0x{word:08X}  {kind}")
        if kind != "load":
            failures.append(f"declared reader 0x{pc:08X} is 0x{word:08X}, which is not a load instruction")
    missed = sorted(set(declared_writers) | set(declared_readers))
    unaccounted = [pc for pc in missed if pc not in measured_a]
    print(
        f"// instrument A+B reached {len([pc for pc in missed if pc in measured_a])} of the "
        f"{len(missed)} sites instruments C names; the remaining {len(unaccounted)} are the pointer-form "
        f"writes a displacement scan structurally cannot see: {['0x%08X' % a for a in unaccounted]}"
    )
    spurious = sorted(set(measured_a) - set(missed))
    if spurious:
        failures.append(
            f"instruments A+B found sites the manifest does not name: {['0x%08X' % a for a in spurious]}"
        )

    # --- 4. the consumers that turn the bound into a decision, hashed and their words re-read --------
    near_plane = bound.get("near_plane_consumer")
    if not isinstance(near_plane, dict):
        raise Refused("manifest field horizontal_bound.near_plane_consumer must be an object")
    verify_range(image, near_plane, "near_plane_consumer", failures)
    verify_word(
        image,
        parse_hex(near_plane.get("far_limit_word"), "near_plane_consumer.far_limit_word"),
        None,
        "near_plane_consumer.far_limit address",
        failures,
    )
    far_word = image.word_at(parse_hex(near_plane["far_limit_word"], "far_limit_word"))
    if (far_word & 0xFFFF) != near_plane.get("far_limit"):
        failures.append(
            f"the far-limit word materialises {far_word & 0xFFFF}, the manifest records "
            f"{near_plane.get('far_limit')}"
        )
    print(
        f"// the near-plane consumer rejects a vertex when NOT (H < Z < {near_plane.get('far_limit')}), so "
        f"the bound IS the GTE near plane in this title; a widening holds it fixed"
    )

    hud = bound.get("hud_consumer")
    if isinstance(hud, dict):
        verify_range(image, hud, "hud_consumer", failures)

    for field in ("light_consumers", "translation_vector_consumers"):
        consumers = bound.get(field)
        if not isinstance(consumers, list):
            continue
        for index, consumer in enumerate(consumers):
            if not isinstance(consumer, dict):
                failures.append(f"horizontal_bound.{field}[{index}] must be an object")
                continue
            if "entry" in consumer and "end" in consumer and "body_sha256" in consumer:
                verify_range(image, consumer, f"{field}[{index}]", failures)

    # --- 5. the read-back, which is the one place Crash 3 differs from its siblings -----------------
    if isinstance(readback, dict):
        verify_range(image, readback, "screen_offset_readback", failures)
        sites = direct_call_sites(image, parse_hex(readback["entry"], "readback.entry"))
        declared_sites = parse_hex_list(readback.get("call_sites"), "readback.call_sites")
        print(f"// read-back call sites: {len(sites)} measured, {len(declared_sites)} declared")
        if sites != declared_sites:
            failures.append(f"read-back call sites measured {['0x%08X' % a for a in sites]}, declared {declared_sites}")
        if isinstance(readback.get("consumer"), dict):
            verify_range(image, readback["consumer"], "readback.consumer", failures)
        widenable = projection.get("widenable")
        pass_through = widenable.get("pass_through_call_sites") if isinstance(widenable, dict) else None
        if not pass_through:
            failures.append(
                "the title reads the published centre back but declares no pass-through call site; "
                "widening that path would compound the margin"
            )
        else:
            for site in parse_hex_list(pass_through, "widenable.pass_through_call_sites"):
                republish = parse_hex_list(readback.get("republish_call_sites"),
                                           "readback.republish_call_sites")
                if site not in declared_sites + republish:
                    failures.append(
                        f"pass-through call site 0x{site:08X} is not one of the measured republication sites"
                    )
    else:
        # No read-back: then NO call site may be a pass-through, because a pass-through with nothing
        # to pass through is a silent hole in the widening.
        widenable = projection.get("widenable")
        if isinstance(widenable, dict) and widenable.get("pass_through_call_sites"):
            failures.append("the title declares a pass-through call site but the census found no read-back")

    # --- 6. the draw area, and the literal cull scan with its null ---------------------------------
    draw_area = projection.get("draw_area")
    if isinstance(draw_area, dict):
        lui = parse_hex(draw_area.get("lui_word"), "draw_area.lui_word")
        store = parse_hex(draw_area.get("store_word"), "draw_area.store_word")
        verify_word(image, lui, 0x3C03E100, "draw_area.lui (GP1 0xE1 draw-area command)", failures)
        verify_word(image, store, 0xACA20000, "draw_area.store", failures)
        print(
            f"// the guest writes GP1 0xE1 exactly once, as (GPUSTAT & 0x3FFF) | 0xE1001000, which is origin "
            f"(0,4) with width and height zero - the WHOLE display area, so this title has no second, "
            f"narrower clip rectangle for a widening to move"
        )

    # The key-shape word check, over the WHOLE projection block, with the number of comparisons as its
    # denominator. A `<x>_word` with no `<x>_instruction` is a failure, so a new field cannot skip it.
    word_comparisons = verify_named_words(image, projection, failures, "runtime.projection")
    print(f"// named instruction words compared against the image: {word_comparisons}")

    culls = cull_census(image)
    histogram: dict[int, int] = {}
    for _pc, _text, immediate in culls:
        histogram[immediate] = histogram.get(immediate, 0) + 1
    print(
        f"// literal horizontal-cull census: {len(culls)} immediate match(es) against a 4:3 dot width or a "
        f"PSX visibility/pad mask, of {len(image.words)} words scanned"
    )
    for immediate in sorted(histogram):
        signed = immediate - 0x10000 if immediate & 0x8000 else immediate
        print(f"//   0x{immediate:04X} ({signed:6d}) x{histogram[immediate]:<3d} {CULL_IMMEDIATES.get(immediate, '')}")
    for pc, text, immediate in culls:
        if immediate in (320, 319, 318, 352, 368, 384, 288) or (
            immediate & 0x8000 and (0x10000 - immediate) in (320, 319, 318, 352, 368, 384, 288)
        ):
            print(f"//   0x{pc:08X}  {text}   [immediate 0x{immediate:04X}]")
    print(
        "// NULL, stated: a cull against a VARIABLE bound (a global, a viewport record) carries no "
        "immediate and is invisible to this scan. It counts what it scanned and what it matched; it does "
        "NOT claim the image contains no variable-bound cull. The variable-bound census above is what "
        "looks for those."
    )

    check_shipping_constants(title, projection, failures)
    return failures


def selftest() -> list[str]:
    """Prove each instrument can produce the OTHER answer, on fixtures built to contain one.

    A scan that has only ever printed one result is not trusted, and a wrong confident answer is worse
    than none. Each case below is the input that would have to exist for the answer to be the other
    one, asserted as a POSITIVE expectation so it runs on every build and needs no restore.
    """
    failures: list[str] = []
    high, low = 0x8006, 0x0884

    def image(words: tuple[int, ...]) -> Image:
        return Image(words=words, load=0x80010000, text_end=0x80010000 + 4 * len(words), data=b"",
                     path=pathlib.Path("<selftest>"))

    def lui(rt: int, immediate: int) -> int:
        return (OP_LUI << 26) | ((rt & 0x1F) << 16) | (immediate & 0xFFFF)

    def lw(base: int, rt: int, displacement: int) -> int:
        return (OP_LW << 26) | ((base & 0x1F) << 21) | ((rt & 0x1F) << 16) | (displacement & 0xFFFF)

    def sw(base: int, rt: int, displacement: int) -> int:
        return (OP_SW << 26) | ((base & 0x1F) << 21) | ((rt & 0x1F) << 16) | (displacement & 0xFFFF)

    # A `ctc2` to CR[24] must be found at its own address, and the decoy encoding - the register in
    # the immediate's low five bits, which is the mistake this tool exists to prevent - must not.
    def ctc2(rt: int, cls: int, register: int) -> int:
        return ((CP2_OPCODE << 26) | ((cls & 0x1F) << CLASS_SHIFT) | ((rt & 0x1F) << 16)
                | ((register & 0x1F) << REGISTER_SHIFT))

    positive = image((ctc2(4, CLASS_CTC2, 24),))
    found = census_cop2(positive).get((CLASS_CTC2, 24), [])
    if found != [0x80010000]:
        failures.append(f"selftest: a ctc2 to CR[24] was not found at its own address (got {found})")
    else:
        print("// selftest fired: a ctc2 CR[24] write is found at 0x80010000")
    decoy = (ctc2(4, CLASS_CTC2, 24) & ~(0x1F << REGISTER_SHIFT)) | (24 << 0)
    if census_cop2(image((decoy,))).get((CLASS_CTC2, 24), []):
        failures.append("selftest: the decoy low-immediate-field encoding still reached CR[24]")
    else:
        print("// selftest fired: the decoy low-immediate-field encoding reaches no control register")

    # A `cfc2` read must be reported as a READER, because the widening's whole safety argument is the
    # reader count.
    if not census_cop2(image((ctc2(14, CLASS_CFC2, 26),))).get((CLASS_CFC2, 26), []):
        failures.append("selftest: a cfc2 CR[26] read was not reported as a reader")
    else:
        print("// selftest fired: a cfc2 CR[26] read is reported as a reader, not a writer")

    # Instrument A finds a displaced load, and refuses a displacement whose base proves another page.
    if [(pc, kind) for pc, _b, kind in displacement_sites(image((lui(4, high), lw(4, 2, low))), high, low)] != [
        (0x80010004, "load")
    ]:
        failures.append("selftest: the displacement scan missed a real reader")
    else:
        print("// selftest fired: the displacement scan finds a real reader at 0x80010004")
    if displacement_sites(image((lui(4, 0x8007), lw(4, 2, low))), high, low):
        failures.append("selftest: the displacement scan counted a displacement whose base proves another page")
    else:
        print("// selftest fired: a displacement whose base lui names another page is not counted")

    # Instrument B finds the zero-displacement access a displacement scan structurally CANNOT see -
    # which is the form both titles write their bound in, so a tool without it would report a title
    # with no writer at all.
    proved = (lui(3, high), (OP_ADDIU << 26) | (3 << 16) | (3 << 21) | low, sw(3, 6, 0))
    if sorted((pc, kind) for pc, _b, kind in zero_displacement_sites(image(proved), high, low)) != [
        (0x80010008, "store")
    ]:
        failures.append("selftest: the register-proof scan missed the zero-displacement write")
    else:
        print("// selftest fired: the register-proof scan finds the zero-displacement WRITE at 0x80010008")
    if zero_displacement_sites(image((lui(3, 0x8007), lw(3, 2, 0))), high, low):
        failures.append("selftest: the register-proof scan accepted a site it could not prove")
    else:
        print("// selftest fired: a zero-displacement access with no matching lui/addiu proof is refused")

    # A POINTER-FORM access - a store through a register whose value came from a struct pointer or a
    # caller's argument, with no `lui` of the bound's page anywhere near - must be a MISS of BOTH
    # instruments. This is the shape Crash 3 writes its bound in (`sw $a2, 0xC4($v1)` at 0x80017A94),
    # so a tool that could not see it would report that title as having no writer at all, which is why
    # the manifest NAMES those sites and the probe verifies their words instead of counting them.
    pointer_form = (lui(3, 0x8007), (OP_ADDIU << 26) | (3 << 16) | (3 << 21) | 0xC4, sw(3, 6, 0xC4))
    if zero_displacement_sites(image(pointer_form), high, low):
        failures.append("selftest: the register-proof scan claimed a pointer-form access it cannot prove")
    if displacement_sites(image(pointer_form), high, 0xC4):
        failures.append("selftest: the displacement scan claimed a pointer-form access it cannot prove")
    if not pointer_form[2] == sw(3, 6, 0xC4):
        failures.append("selftest: the pointer-form fixture is not the store it claims to be")
    else:
        print(
            "// selftest fired: a pointer-form store is a MISS of both scans, which is why the manifest "
            "names those sites instead of the scan counting them"
        )

    # A cull against 320 must be found; a cull against 321 must not be reported as one.
    sltiu = (OP_SLTIU << 26) | (3 << 21) | (3 << 16) | 320
    near = (OP_SLTIU << 26) | (3 << 21) | (3 << 16) | 321
    if [immediate for _pc, _text, immediate in cull_census(image((sltiu, near)))] != [320]:
        failures.append("selftest: the cull census did not separate 320 from 321")
    else:
        print("// selftest fired: the cull census separates an exact 4:3 width from a near miss")

    # A pointer-table reach must be invisible to a call scan and visible to the resident-word scan.
    # The word is literally equal to the target, which is what a pointer table holds.
    target = 0x80020000
    if direct_call_sites(image((sltiu, target)), target):
        failures.append("selftest: the call scan invented a call site")
    if resident_word_sites(image((sltiu, target)), target) != [0x80010004]:
        failures.append("selftest: the resident-word scan missed a pointer-table reach")
    else:
        print("// selftest fired: a pointer-table reach produces zero direct call sites and one word match")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--title", default="crash2", help="title directory under titles/")
    parser.add_argument("--executable", type=pathlib.Path, help="the authenticated executable")
    parser.add_argument("--selftest-only", action="store_true", help="run the negative cases only")
    arguments = parser.parse_args()

    if arguments.selftest_only:
        failures = selftest()
        for failure in failures:
            print(f"FAIL: {failure}")
        if failures:
            return 1
        print(f"PASS: {arguments.title} projection-probe selftest fired every case")
        return 0

    if arguments.title not in SHIPPING_HEADERS:
        print(f"REFUSED: --title {arguments.title} has no shipping owner to diff the constants against")
        return 2
    manifest_path = ROOT / "titles" / arguments.title / "executable.json"
    try:
        manifest = json.loads(manifest_path.read_text())
    except (OSError, ValueError) as exc:
        print(f"REFUSED: cannot read {manifest_path}: {exc}")
        return 2
    if arguments.executable is None:
        print(
            "REFUSED: pass --executable <the title's SCUS_*.BIN>; the authenticated image is never a "
            "build input and tools/provision_title.py places it"
        )
        return 2
    try:
        image = load_image(arguments.executable, manifest)
        failures = check(image, arguments.title, manifest) + selftest()
    except Refused as exc:
        print(f"REFUSED: {exc}")
        return 2
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print(f"PASS: {arguments.title} projection census agrees with titles/{arguments.title}/executable.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

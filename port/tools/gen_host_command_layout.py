#!/usr/bin/env python3
"""Generate the host's layouts of the game's command stream words.

The console reads a command stream (a fighter's or an item's script, a color
overlay) through bit-fields over big-endian 32-bit words, which MWCC allocates
from the most significant bit.  The host converts each word of a stream to
native order when it loads the archive, so a word holds the same integer on
both; what still differs is where the compiler puts a bit-field, since GCC and
Clang allocate from the least significant bit on a little-endian target.

This tool reads the declarations in src/melee/lb/types.h, places every field
where MWCC does, and writes host declarations that put each field on the same
bits: the fields of a word in reverse order, with unnamed padding where the
console leaves bits unused.  A pointer in a stream (the target of a subroutine
or a goto) becomes the signed distance from its word to the target, which
keeps every command word four bytes wide.

It also writes a C check that packs values into words the way the console lays
them out and reads them back through the host declarations.

    port/tools/gen_host_command_layout.py          # rewrite both outputs
    port/tools/gen_host_command_layout.py --check  # fail if they are stale
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TYPES_H = ROOT / "src/melee/lb/types.h"
HEADER = ROOT / "port/src/game/host_command_layout.h"
CHECK = ROOT / "port/tests/command_layout_check.c"

WORD = 32

SIZES = {
    "u8": 8, "s8": 8, "char": 8,
    "u16": 16, "s16": 16,
    "u32": 32, "s32": 32, "int": 32, "enum_t": 32,
}
SIGNED = {"s8", "s16", "s32", "int", "enum_t"}

MEMBER = re.compile(
    r"^(?P<type>[A-Za-z_][A-Za-z0-9_]*)\s*(?P<pointer>\*)?\s*"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*"
    r"(?:\[(?P<count>[^\]]+)\])?\s*(?::\s*(?P<width>\d+))?\s*;$"
)


@dataclass
class Member:
    type: str
    name: str
    width: int | None  # bit-field width, or None for a plain member
    count: int | None  # array length
    pointer: bool


@dataclass
class Placed:
    member: Member
    bit: int  # console position, counted from the most significant bit
    bits: int


@dataclass
class Aggregate:
    kind: str  # "struct" or "union"
    name: str
    members: list  # Member, or (name, Aggregate) for a nested struct


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def parse_members(body: str) -> list:
    members: list = []
    body = strip_comments(body)
    position = 0
    while True:
        nested = re.compile(r"struct\s*\{").search(body, position)
        semicolon = body.find(";", position)
        if semicolon < 0:
            break
        if nested is not None and nested.start() < semicolon:
            depth = 0
            index = nested.end() - 1
            while True:
                if body[index] == "{":
                    depth += 1
                elif body[index] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                index += 1
            inner = parse_members(body[nested.end():index])
            tail = re.compile(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*;").match(body, index + 1)
            if tail is None:
                raise SystemExit("an anonymous struct member has no name")
            members.append((tail.group(1), Aggregate("struct", "", inner)))
            position = tail.end()
            continue
        declaration = " ".join(body[position:semicolon + 1].split())
        position = semicolon + 1
        if not declaration.strip(" ;"):
            continue
        match = MEMBER.match(declaration)
        if match is None:
            raise SystemExit(f"cannot read the member `{declaration}`")
        members.append(Member(
            type=match.group("type"),
            name=match.group("name"),
            width=int(match.group("width")) if match.group("width") else None,
            count=int(match.group("count"), 0) if match.group("count") else None,
            pointer=match.group("pointer") is not None,
        ))
    return members


def find_aggregates(text: str, first: str, stop: str) -> list[Aggregate]:
    start = text.index(first)
    end = text.index(stop, start)
    region = text[start:end]
    found = []
    for match in re.finditer(r"^(struct|union) ([A-Za-z_][A-Za-z0-9_]*) \{(.*?)^\};",
                             region, flags=re.S | re.M):
        found.append(Aggregate(match.group(1), match.group(2),
                               parse_members(match.group(3))))
    return found


def expand(member: Member) -> list[Member]:
    """GXColor is four bytes named r, g, b and a."""
    if member.type == "GXColor" and not member.pointer and member.count is None:
        return [Member("u8", channel, None, None, False) for channel in "rgba"]
    return [member]


def place(members: list[Member]) -> tuple[list[Placed], int]:
    """Lay the members out as MWCC does on a big-endian target: each
    bit-field in a storage unit of its declared type, from the most
    significant bit, sharing the unit with what precedes it when it fits."""
    placed = []
    cursor = 0
    align = 8
    for member in members:
        if member.pointer:
            size = WORD
        elif member.type in SIZES:
            size = SIZES[member.type]
        else:
            raise SystemExit(f"no size for the type `{member.type}`")
        align = max(align, size)
        if member.width is not None:
            unit = cursor - cursor % size
            if cursor + member.width > unit + size:
                cursor = unit + size
            placed.append(Placed(member, cursor, member.width))
            cursor += member.width
        else:
            cursor = -(-cursor // size) * size
            bits = size * (member.count or 1)
            placed.append(Placed(member, cursor, bits))
            cursor += bits
    total = -(-cursor // align) * align
    return placed, total


def host_word_count(total_bits: int) -> int:
    return -(-total_bits // WORD)


def host_declarations(placed: list[Placed], total_bits: int, indent: str) -> list[str]:
    lines = []
    words = host_word_count(total_bits)
    for word in range(words):
        low = word * WORD
        high = low + WORD
        entries = []
        whole_word = None
        for item in placed:
            member = item.member
            end = item.bit + item.bits
            if end <= low or item.bit >= high:
                continue
            if member.count is not None:
                if member.type not in ("u8", "s8", "char") or item.bit % WORD:
                    raise SystemExit(f"cannot lay out the array `{member.name}`")
                covered = (end - item.bit) // WORD * WORD
                if item.bit == low:
                    lines.append(f"{indent}{member.type} {member.name}[{covered // 8}];")
                # The bytes of a trailing partial word are padding here.
                if low < item.bit + covered:
                    whole_word = True
                continue
            if item.bit < low or end > high:
                raise SystemExit(f"`{member.name}` crosses a word")
            if member.pointer:
                if item.bits != WORD:
                    raise SystemExit(f"the pointer `{member.name}` is not a word")
                whole_word = f"{indent}s32 rel; /* {member.name}: target - &rel */"
                continue
            if member.width is None and item.bits == WORD:
                whole_word = f"{indent}{member.type} {member.name};"
                continue
            lsb = high - end
            base = "s32" if member.type in SIGNED else "u32"
            entries.append((lsb, item.bits, f"{base} {member.name}"))
        if whole_word is True:
            continue
        if whole_word is not None:
            if entries:
                raise SystemExit("a word mixes a plain member with bit-fields")
            lines.append(whole_word)
            continue
        entries.sort()
        cursor = 0
        for lsb, bits, declaration in entries:
            if lsb < cursor:
                raise SystemExit(f"`{declaration}` overlaps another field")
            if lsb > cursor:
                lines.append(f"{indent}u32 : {lsb - cursor};")
            lines.append(f"{indent}{declaration} : {bits};")
            cursor = lsb + bits
        if cursor < WORD:
            lines.append(f"{indent}u32 : {WORD - cursor};")
    return lines


def emit_aggregate(aggregate: Aggregate, cases: list, label: str) -> list[str]:
    lines = [f"{aggregate.kind} {aggregate.name} {{"]
    if aggregate.kind == "struct":
        members = [m for member in aggregate.members for m in expand(member)]
        placed, total = place(members)
        lines += host_declarations(placed, total, "    ")
        lines.append("};")
        lines.append(f"_Static_assert(sizeof(struct {aggregate.name}) == "
                     f"{host_word_count(total) * 4}, \"{aggregate.name}\");")
        add_cases(cases, placed, f"((const struct {aggregate.name}*) (const void*) words)",
                  label or aggregate.name)
        return lines
    for member in aggregate.members:
        if isinstance(member, tuple):
            name, inner = member
            fields = inner.members
        else:
            name, fields = member.name, [member]
        fields = [m for field in fields for m in expand(field)]
        placed, total = place(fields)
        if host_word_count(total) != 1:
            raise SystemExit(f"the union member `{name}` is not one word")
        lines.append("    struct {")
        lines += host_declarations(placed, total, "        ")
        lines.append(f"    }} {name};")
        add_cases(cases, placed,
                  f"((const union {aggregate.name}*) (const void*) words)->{name}",
                  f"{aggregate.name}.{name}")
    lines.append("};")
    return lines


def add_cases(cases: list, placed: list[Placed], access: str, label: str) -> None:
    for item in placed:
        member = item.member
        if member.pointer or member.count is not None:
            continue
        word = item.bit // WORD
        lsb = WORD - (item.bit - word * WORD) - item.bits
        separator = "." if access.endswith(")") is False else "->"
        cases.append((f"{label}.{member.name}", word, lsb, item.bits,
                      int(member.type in SIGNED), f"{access}{separator}{member.name}"))


def generate() -> tuple[str, str]:
    text = TYPES_H.read_text()
    overlay = find_aggregates(text, "union ColorOverlay_x8_t {", "ASSERT_SIZE(union ColorOverlay_x8_t")
    commands = find_aggregates(text, "struct Command_00 {", "union CmdUnion {")
    cases: list = []
    header = [
        "/* Generated by port/tools/gen_host_command_layout.py from",
        " * src/melee/lb/types.h; do not edit.",
        " *",
        " * The host converts the words of a command stream to native order, and",
        " * these declarations put every field on the bits MWCC gives it: the",
        " * fields of each word in reverse order, with unnamed padding where the",
        " * console leaves bits unused.  A pointer in a stream is `rel`, the",
        " * distance from its own word to the target. */",
        "",
        "#ifndef MELEE_HOST_COMMAND_LAYOUT_H",
        "#define MELEE_HOST_COMMAND_LAYOUT_H",
        "",
    ]
    for aggregate in overlay + commands:
        header += emit_aggregate(aggregate, cases, "")
        header.append("")
    header.append("#endif")
    check = [
        "/* Generated by port/tools/gen_host_command_layout.py; do not edit.",
        " *",
        " * Packs values into words where MWCC lays each field out and reads them",
        " * back through the host declarations of port/src/game/host_command_layout.h. */",
        "",
        "#include <melee/lb/types.h>",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "#include <stdio.h>",
        "",
        "typedef int64_t (*CommandFieldGetter)(const uint32_t* words);",
        "",
        "typedef struct CommandFieldCase {",
        "    const char* name;",
        "    unsigned word;",
        "    unsigned lsb;",
        "    unsigned width;",
        "    int is_signed;",
        "    CommandFieldGetter get;",
        "} CommandFieldCase;",
        "",
    ]
    for index, case in enumerate(cases):
        check.append(f"static int64_t get_{index}(const uint32_t* words)")
        check.append("{")
        check.append(f"    return {case[5]};")
        check.append("}")
        check.append("")
    check.append("static const CommandFieldCase cases[] = {")
    for index, (name, word, lsb, width, is_signed, _) in enumerate(cases):
        check.append(f"    {{ \"{name}\", {word}, {lsb}, {width}, {is_signed}, get_{index} }},")
    check.append("};")
    check += [
        "",
        "int melee_host_command_layout_failures(char* message, size_t size);",
        "",
        "int melee_host_command_layout_failures(char* message, size_t size)",
        "{",
        "    static const uint32_t values[] = {",
        "        0x00000000U, 0xFFFFFFFFU, 0x55555555U, 0xAAAAAAAAU, 0x89ABCDEFU,",
        "    };",
        "    static const uint32_t backgrounds[] = { 0x00000000U, 0xFFFFFFFFU };",
        "    int failures = 0;",
        "    size_t c;",
        "    size_t v;",
        "    size_t b;",
        "",
        "    for (c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {",
        "        const CommandFieldCase* const field = &cases[c];",
        "        const uint32_t mask = field->width == 32",
        "            ? 0xFFFFFFFFU : (uint32_t) ((1U << field->width) - 1U);",
        "        for (v = 0; v < sizeof(values) / sizeof(values[0]); v++) {",
        "            for (b = 0; b < sizeof(backgrounds) / sizeof(backgrounds[0]); b++) {",
        "                uint32_t words[4];",
        "                const uint32_t value = values[v] & mask;",
        "                int64_t expected = (int64_t) value;",
        "                int64_t got;",
        "                words[0] = words[1] = words[2] = words[3] = backgrounds[b];",
        "                words[field->word] &= ~(mask << field->lsb);",
        "                words[field->word] |= value << field->lsb;",
        "                if (field->is_signed && ((value >> (field->width - 1)) & 1U)) {",
        "                    expected -= (int64_t) 1 << field->width;",
        "                }",
        "                got = field->get(words);",
        "                if (got != expected) {",
        "                    if (failures == 0 && message != NULL && size != 0) {",
        "                        snprintf(message, size,",
        "                                 \"%s read %lld where the console reads %lld\",",
        "                                 field->name, (long long) got,",
        "                                 (long long) expected);",
        "                    }",
        "                    failures += 1;",
        "                }",
        "            }",
        "        }",
        "    }",
        "    return failures;",
        "}",
        "",
        "int melee_host_command_layout_cases(void);",
        "",
        "int melee_host_command_layout_cases(void)",
        "{",
        "    return (int) (sizeof(cases) / sizeof(cases[0]));",
        "}",
    ]
    return "\n".join(header) + "\n", "\n".join(check) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="fail if the outputs differ from what would be written")
    args = parser.parse_args()
    header, check = generate()
    if args.check:
        stale = [str(path.relative_to(ROOT)) for path, text in
                 ((HEADER, header), (CHECK, check))
                 if not path.exists() or path.read_text() != text]
        if stale:
            print("stale, rerun port/tools/gen_host_command_layout.py: " + ", ".join(stale))
            return 1
        return 0
    HEADER.write_text(header)
    CHECK.write_text(check)
    return 0


if __name__ == "__main__":
    sys.exit(main())

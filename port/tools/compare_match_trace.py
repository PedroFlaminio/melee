#!/usr/bin/env python3
"""Compare two canonical match traces written by melee-pc --run-modes TRACE=.

Each line is one drawn frame: the frame number, then `key=value` fields, with
`P1`..`P4` opening each fighter's fields.  Floats are the eight hex digits of
their IEEE bits, so the comparison is exact.  The traces must cover the same
frames and fighters; the first field that differs is reported with its frame,
entity and both values.  --ignore leaves a field out, to look past a
difference already understood.  Exit status: 0 identical, 1 different, 2
unusable.
"""

import argparse
import struct
import sys

FLOAT_FIELDS = {"anim", "x", "y", "vx", "vy", "facing", "percent"}


def read_trace(path, ignored):
    frames = []
    with open(path, encoding="ascii") as stream:
        for number, line in enumerate(stream, 1):
            tokens = line.split()
            if not tokens:
                continue
            try:
                frame = int(tokens[0])
            except ValueError:
                raise SystemExit(f"{path}:{number}: no frame number")
            fields = []
            entity = "frame"
            for token in tokens[1:]:
                if "=" not in token:
                    entity = token
                    fields.append((entity, None, None))
                    continue
                key, value = token.split("=", 1)
                if key not in ignored:
                    fields.append((entity, key, value))
            frames.append((frame, fields))
    return frames


def shown(key, value):
    if key in FLOAT_FIELDS and value is not None and len(value) == 8:
        return f"{value} ({struct.unpack('>f', bytes.fromhex(value))[0]!r})"
    return str(value)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("expected")
    parser.add_argument("actual")
    parser.add_argument("--ignore", action="append", default=[],
                        metavar="FIELD", help="a field to leave out")
    args = parser.parse_args()
    expected = read_trace(args.expected, set(args.ignore))
    actual = read_trace(args.actual, set(args.ignore))
    if not expected:
        print(f"{args.expected}: empty trace")
        return 2
    if [frame for frame, _ in expected] != [frame for frame, _ in actual]:
        print(f"frames differ: {len(expected)} frames "
              f"({expected[0][0]}-{expected[-1][0]}) against {len(actual)}"
              + (f" ({actual[0][0]}-{actual[-1][0]})" if actual else ""))
        return 2
    for (frame, want), (_, got) in zip(expected, actual):
        for index in range(max(len(want), len(got))):
            left = want[index] if index < len(want) else None
            right = got[index] if index < len(got) else None
            if left == right:
                continue
            if left is None or right is None or left[:2] != right[:2]:
                print(f"frame {frame}: fields differ: {left} against {right}")
                return 1
            entity, key, _ = left
            print(f"frame {frame} {entity} {key}: {shown(key, left[2])} "
                  f"against {shown(key, right[2])}")
            return 1
    print(f"{len(expected)} frames identical "
          f"({expected[0][0]}-{expected[-1][0]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

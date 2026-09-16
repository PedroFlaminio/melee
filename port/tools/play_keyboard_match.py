#!/usr/bin/env python3
"""Play a whole VS match on the keyboard, in the --play window.

play_keyboard_probe.py presses one key in the middle of a match.  This one
plays the route the melee-host-vs-match-asset test plays, but with pad 1's
side of it coming from real key events sent to the window: START on the
title, the main menu, the character and stage selects, the fight, the pause
and quit, and the results screen back to character select.  Pad 2 stays
scripted, because the window's keyboard is pad 1 only.

Keys land on frames, not on the wall clock: the route prints `rules frame N`
two frames before each press, and the press goes out when that line arrives,
so a machine that draws slower than 60 Hz moves the whole schedule with it.

    DISPLAY=:1 python3 port/tools/play_keyboard_match.py

It prints the scene the route reached and what the fighter was doing, and
fails if the route did not get from the title to the results and back.
"""

import argparse
import os
import queue
import re
import shutil
import subprocess
import sys
import threading
import time

# Pad 1's half of the route, as (frame, key, frames held).  The frames are
# the ones melee-host-vs-match-asset uses for the same presses.
PRESSES = [
    (120, "Return", 3),    # START on the title
    (160, "KP_2", 3),      # the main menu's cursor, down to VS Mode
    (200, "j", 3),         # A: into VS Mode
    (240, "j", 3),         # A: VS Melee
    (300, "w", 16),        # character select: up to pad 1's HMN button
    (320, "j", 3),         # A: open the port
    (330, "d", 8),         # right, towards Fox
    (345, "w", 10),        # up, onto Fox
    (360, "j", 3),         # A: take Fox
    (380, "Return", 3),    # START: to stage select
    (420, "a", 2),         # left
    (425, "w", 15),        # up, onto Hyrule Temple
    (445, "j", 3),         # A: start the match
    (660, "d", 40),        # the fight: run right
    (705, "j", 3),         # and jab
    (760, "Return", 3),    # pause
    (790, "h", 20),        # L, held with R and A for the quit
    (795, "l", 15),
    (800, "j", 10),
    (805, "Return", 5),    # START over L+R+A: quit the match
    (1060, "j", 3),        # results: past the winner
    (1200, "Return", 3),   # START: pad 1 is ready
    (1320, "k", 60),       # B, held: character select back to the main menu
]

# Pad 2's half, which the script keeps.
PAD2 = [
    "300-315:SY=127@2", "320:A@2", "330-333:SX=-127@2",
    "345-354:SY=127@2", "360:A@2", "1200:START@2",
]

# What the route reports: the fighter during the fight, and the scene of
# every state it ran.
DIAGNOSTICS = ["700:FIGHTERS", "705:ACTION", "745:ACTION", "750:FIGHTERS",
               "1500:STOP"]


def reader_thread(stream, lines, sink):
    for line in stream:
        lines.append(line)
        sink.put(line)
    sink.put(None)


def find_window(pid, timeout_s):
    """By process: an earlier run's window keeps its title in the X server's
    list for a while, and sending to a dead id is a BadWindow."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        found = subprocess.run(
            ["xdotool", "search", "--pid", str(pid), "--onlyvisible",
             "--name", "Melee PC"],
            capture_output=True, text=True)
        ids = [line for line in found.stdout.split() if line]
        if ids:
            return ids[-1]
        time.sleep(0.2)
    return None


def key(window, action, name):
    return subprocess.run(["xdotool", action, "--window", window, name],
                          capture_output=True, text=True).returncode == 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary",
                        default="build/host-release/port/melee-pc")
    parser.add_argument("--root", default="assets-local")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--shot", metavar="PATH",
                        help="grab the window itself at --shot-frame, which "
                             "is the only way to see what the keyboard is "
                             "playing: BMP entries need the hidden presenter")
    parser.add_argument("--shot-frame", type=int, default=700)
    args = parser.parse_args()

    if shutil.which("xdotool") is None:
        print("xdotool is not installed", file=sys.stderr)
        return 2
    if os.environ.get("DISPLAY") is None:
        print("no DISPLAY to open the window on", file=sys.stderr)
        return 2

    # A marker one frame before each press and one on the frame it is
    # released, so both go out on the frame the route is on rather than on a
    # guess at the wall clock.
    # The keyup goes out after the last frame the key is meant to be down
    # has been drawn, so a press of N frames is released on frame + N - 1.
    markers = sorted({max(frame - 1, 1) for frame, _, _ in PRESSES} |
                     {frame + hold - 1 for frame, _, hold in PRESSES} |
                     ({args.shot_frame} if args.shot else set()))
    route = [f"{frame}:RULES" for frame in markers] + PAD2 + DIAGNOSTICS
    command = [args.binary]
    if shutil.which("stdbuf") is not None:
        command = ["stdbuf", "-oL", args.binary]
    environment = dict(os.environ, SDL_VIDEODRIVER="x11", LANG="C", LC_ALL="C")
    game = subprocess.Popen(command + ["--play", args.root] + route,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, bufsize=1, env=environment)
    lines = []
    sink = queue.Queue()
    thread = threading.Thread(target=reader_thread,
                              args=(game.stdout, lines, sink), daemon=True)
    thread.start()
    window = find_window(game.pid, 30.0)
    if window is None:
        game.kill()
        print("the play window never appeared", file=sys.stderr)
        return 1

    held = []  # (release_at, key), released as the schedule passes them
    pending = list(PRESSES)
    deadline = time.monotonic() + 180.0
    while pending and time.monotonic() < deadline:
        try:
            line = sink.get(timeout=deadline - time.monotonic())
        except queue.Empty:
            break
        if line is None:
            break
        marker = re.match(r"rules frame (\d+):", line)
        if marker is None:
            continue
        frame = int(marker.group(1))
        if args.shot and frame == args.shot_frame:
            subprocess.run(["import", "-window", window, args.shot],
                           capture_output=True, text=True)
            print(f"frame {frame}: window grabbed to {args.shot}")
        for release_at, name in list(held):
            if frame >= release_at:
                key(window, "keyup", name)
                held.remove((release_at, name))
        for press in list(pending):
            at, name, hold = press
            if frame + 1 < at:
                continue
            pending.remove(press)
            if not key(window, "keydown", name):
                game.kill()
                print(f"the {name} press did not reach the window",
                      file=sys.stderr)
                return 1
            held.append((at + hold - 1, name))
            if not args.quiet:
                print(f"frame {frame}: {name} for {hold} frames")
    for _, name in held:
        key(window, "keyup", name)
    try:
        game.wait(timeout=120)
    except subprocess.TimeoutExpired:
        game.kill()
    thread.join(timeout=5.0)
    output = "".join(lines)

    scenes = re.findall(r"scene (0x[0-9a-f]+) from frame (\d+)", output)
    for scene, frame in scenes:
        print(f"scene {scene} from frame {frame}")
    for frame, x in re.findall(r"fighters frame (\d+): P1=(-?[\d.]+)", output):
        print(f"frame {frame}: P1 x={x}")
    for frame, motion in re.findall(r"action frame (\d+): P1=(-?\d+)", output):
        print(f"frame {frame}: P1 motion {motion}")
    if pending:
        print(f"{len(pending)} presses never went out: "
              f"{[name for _, name, _ in pending]}", file=sys.stderr)
        print("the last lines the game printed:", file=sys.stderr)
        for line in lines[-15:]:
            print("  " + line.rstrip(), file=sys.stderr)
        return 1
    reached = [scene for scene, _ in scenes]
    wanted = ["0x00", "0x01", "0x08", "0x09", "0x02", "0x05", "0x08"]
    if reached[:len(wanted)] != wanted:
        print(f"the keyboard did not carry the route: {reached}",
              file=sys.stderr)
        print("the last lines the game printed:", file=sys.stderr)
        for line in lines[-12:]:
            print("  " + line.rstrip(), file=sys.stderr)
        return 1
    print("the keyboard took the route from the title to the results "
          "and back to character select")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Drive a --play window with real key events and read the fighter back.

The scripted routes reach the game through the same PADRead the window fills,
so they never exercise the window's own keyboard.  This probe does: it opens
the play window on X11, waits for the match to start, and sends a key press to
that window alone with xdotool's --window form, which goes to the game through
SDL's event queue and nowhere else.  FIGHTERS entries before and after the
press say whether the fighter moved, and ACTION says what it was doing.

Run it twice, with --press and with --no-press: with the key the fighter runs
and its x moves, without it the fighter stays where it spawned.  That
difference is the keyboard path.

    python3 port/tools/play_keyboard_probe.py --press
    python3 port/tools/play_keyboard_probe.py --no-press

Needs an X server (DISPLAY), xdotool, and assets-local.  The window is drawn
at 60 frames a second of wall clock, so the wait before the press is in
seconds; a loaded machine slows the route down and moves the frames the press
lands on.
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

ROUTE = [
    # Title to the main menu, then two pads through character and stage
    # select, as the vs-match route does.
    "120:START", "160:DOWN", "200:A", "240:A",
    "300-315:SY=127", "300-315:SY=127@2", "320:A", "320:A@2",
    "330-337:SX=127", "330-333:SX=-127@2",
    "345-354:SY=127", "345-354:SY=127@2", "360:A", "360:A@2",
    "380:START", "420-421:SX=-127", "425-439:SY=127", "445:A",
    # The match: nothing scripted on pad 1 from here, so what the fighter
    # does is what the window's keyboard said.
    "700:FIGHTERS", "705:ACTION", "795:ACTION", "800:FIGHTERS", "900:STOP",
]


def reader_thread(stream, lines, sink):
    for line in stream:
        lines.append(line)
        sink.put(line)
    sink.put(None)


def wait_for(sink, pattern, timeout_s):
    """The route prints each scene as it starts, which is a better clock than
    the wall: a loaded machine draws the same frames later."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            line = sink.get(timeout=deadline - time.monotonic())
        except queue.Empty:
            return None
        if line is None:
            return None
        found = re.search(pattern, line)
        if found:
            return found
    return None


def send_key(pid, action, key):
    """The window is resolved again for each event: SDL can replace it, and a
    send to an id that is gone is a BadWindow the game never sees."""
    for _ in range(5):
        window = find_window(pid, 5.0)
        if window is None:
            print(f"no window for pid {pid} to {action} on", file=sys.stderr)
            continue
        sent = subprocess.run(["xdotool", action, "--window", window, key],
                              capture_output=True, text=True)
        if sent.returncode == 0:
            return True
        print(f"xdotool {action} on {window}: {sent.stderr.strip()}",
              file=sys.stderr)
        time.sleep(0.2)
    return False


def find_window(pid, timeout_s):
    """By process, not by name: an earlier run's window keeps its title in the
    X server's list for a while, and sending to a dead id is a BadWindow."""
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary",
                        default="build/host-release/port/melee-pc")
    parser.add_argument("--root", default="assets-local")
    parser.add_argument("--key", default="d",
                        help="the key held during the match; d is right on "
                             "the keyboard's main stick")
    parser.add_argument("--press", action=argparse.BooleanOptionalAction,
                        default=True)
    parser.add_argument("--press-frame", type=int, default=700,
                        help="the route frame the press aims at; the match "
                             "takes input after the GO at frame 655")
    parser.add_argument("--hold", type=float, default=1.5)
    args = parser.parse_args()

    if shutil.which("xdotool") is None:
        print("xdotool is not installed", file=sys.stderr)
        return 2
    if os.environ.get("DISPLAY") is None:
        print("no DISPLAY to open the window on", file=sys.stderr)
        return 2

    environment = dict(os.environ, SDL_VIDEODRIVER="x11", LANG="C", LC_ALL="C")
    # The game's stdout into a pipe is block buffered, so the scene lines
    # this probe waits on would only arrive at the end; stdbuf gives them
    # back a line at a time.
    command = [args.binary]
    if shutil.which("stdbuf") is not None:
        command = ["stdbuf", "-oL", args.binary]
    game = subprocess.Popen(command + ["--play", args.root] + ROUTE,
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
    print(f"window {window}")
    if args.press:
        started = wait_for(sink, r"scene 0x02 from frame (\d+)", 60.0)
        if started is None:
            game.kill()
            print("the match never started", file=sys.stderr)
            return 1
        frames_to_wait = args.press_frame - int(started.group(1))
        time.sleep(max(frames_to_wait, 0) / 60.0)
        if not send_key(game.pid, "keydown", args.key):
            game.kill()
            print("the key press did not reach the window", file=sys.stderr)
            return 1
        time.sleep(args.hold)
        send_key(game.pid, "keyup", args.key)
    game.wait()
    thread.join(timeout=5.0)
    output = "".join(lines)

    positions = re.findall(r"fighters frame (\d+): P1=(-?[\d.]+)", output)
    actions = re.findall(r"action frame (\d+): P1=(-?\d+)", output)
    for frame, x in positions:
        print(f"frame {frame}: P1 x={x}")
    for frame, motion in actions:
        print(f"frame {frame}: P1 motion {motion}")
    if len(positions) < 2:
        print("the route did not reach the match", file=sys.stderr)
        return 1
    moved = abs(float(positions[-1][1]) - float(positions[0][1]))
    # A stick key walks the fighter; a button leaves it where it is and
    # changes what it is doing, so either counts as the key arriving.
    motions = [motion for _, motion in actions]
    acted = moved >= 1.0 or (len(motions) >= 2 and motions[0] != motions[-1])
    print(f"moved {moved:.2f} units and went through motions "
          f"{','.join(motions)} with the key "
          f"{'held' if args.press else 'left alone'}")
    if args.press and not acted:
        print("the key did not reach the game", file=sys.stderr)
        return 1
    if not args.press and acted:
        print("the fighter acted with no key held", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

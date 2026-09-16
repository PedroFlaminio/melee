#!/usr/bin/env python3
"""Play random CPU matches headlessly, restart after a crash, log what broke.

`melee-pc --run-modes` takes a scripted pad route, so a whole VS match runs
without a window.  This tool writes one random route per match - the character
each port takes, the rules, how long the fight lasts - runs it, reads the
diagnostics back and appends one JSON line per match to a results file.  A run
that crashes, hangs or never reaches the end of its route is written to the log
with the command that reproduces it, and the next match starts anyway.

    python3 port/tools/random_cpu_matches.py --matches 20 --jobs 4
    python3 port/tools/random_cpu_matches.py --characters fox,mario,link
    python3 port/tools/random_cpu_matches.py --players 4 --until-end

The route is deterministic: the host's random seed does not come from the wall
clock, so the same route always plays the same match, and the command the log
prints replays a crash exactly.  Variety comes from the route itself.

What the routes can reach was measured against this build's menus:

  - Character select shows the 14 starters, two rows of seven.  A port reaches
    an icon by holding right for COLUMN frames and up for ROW frames from its
    own toggle box, and each port's box sits 12 frames further right than the
    one before it.  Ten of the characters load; Peach, Donkey Kong, Kirby and
    Zelda stop the host while their data is read, so they are only used when
    --characters asks for them.
  - Hyrule Temple is the only stage that loads - every other square stops in
    the stage translator - so every match is played there.
  - The rules menu's first two rows work: the match kind and its count.  The
    item and extra-rules submenus stop with a name, so they are left alone.
    Time, Stock and Bonus matches run; a Coin match stops on its first coin
    (item.c:576), so --kinds draws Time and Stock unless told otherwise.

The routes that break are the point: they are written to the log with the
command that replays them, and the tool carries on with the next match.
"""

import argparse
import collections
import concurrent.futures as futures
import json
import os
import random
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))))

Character = collections.namedtuple("Character", "kind column row loads")

# Character select, measured icon by icon: the column is how many frames the
# stick is held right from port 1's toggle box, the row how many frames up.
COLUMN_FRAMES = (6, 12, 18, 22, 26, 34, 40)
ROW_FRAMES = {"top": 16, "bottom": 11}
PORT_COLUMN_SHIFT = -12

# `loads` is False for the four whose data the host cannot translate yet: the
# match dies while it is read, before anyone moves.
CHARACTERS = {
    "mario": Character(8, 0, "top", True),
    "pikachu": Character(13, 1, "top", True),
    "bowser": Character(5, 2, "top", True),
    "peach": Character(12, 3, "top", False),
    "yoshi": Character(17, 4, "top", True),
    "donkey": Character(1, 5, "top", False),
    "captain": Character(0, 6, "top", True),
    "fox": Character(2, 0, "bottom", True),
    "ness": Character(11, 1, "bottom", True),
    "iceclimbers": Character(14, 2, "bottom", True),
    "kirby": Character(4, 3, "bottom", False),
    "samus": Character(16, 4, "bottom", True),
    "zelda": Character(18, 5, "bottom", False),
    "link": Character(6, 6, "bottom", True),
}
BY_KIND = {entry.kind: name for name, entry in CHARACTERS.items()}
LOADABLE = [name for name, entry in CHARACTERS.items() if entry.loads]
# The three whose items the host translates; the other loadable characters
# stop the match as soon as a special of theirs puts one on the stage.
STABLE = ["fox", "mario", "link"]

# A run that played its match is "ok"; "early" is a match that ended on
# its own before the route quit it, which is the game working, not the
# host breaking.  Every other status goes to the log.
OK_STATUSES = ("ok", "early")

# The scenes the route walks through, to say where a run died in words.  A run
# that dies with the stage select as its last scene died either there or in the
# match it is loading, because the match names itself only once it is up.
SCENE_NAMES = {
    "0x00": "the title", "0x01": "the main menu", "0x02": "the match",
    "0x03": "sudden death", "0x05": "the results screen",
    "0x08": "character select",
    "0x09": "the stage select, or the match it is loading",
    "0x18": "the opening movie", "0x27": "the prize notice",
    "0x29": "the challenger",
}

STAGE_KIND = 14  # Hyrule Temple
MATCH_KINDS = ("time", "stock", "coin", "bonus")

# The frames each phase of the route lands on.  Character select opens on
# frame 243 and the phases after it are placed from there.
TOGGLE_UP = "300-315:SY=127"
OPEN_PORT = 320
MAKE_CPU = 326
MOVE_ACROSS = 332
MOVE_UP = 386
TAKE_ICON = 424
RULES_CORNER = 430
RULES_OPEN = 495
RULES_FIRST_PRESS = 520
RULES_PRESS_GAP = 20
STAGE_TO_MATCH = 88  # frames between the stage's A press and the first drawn
                     # frame of the match
PAUSE_TO_RESULTS = 28
RESULTS_PAST_WINNER = 280
RESULTS_READY = 420
RESULTS_LEAVE = 520


def character_moves(port, character):
    """The stick holds that carry `port`'s cursor onto `character`'s icon."""
    across = COLUMN_FRAMES[character.column] + PORT_COLUMN_SHIFT * (port - 1)
    return across, ROW_FRAMES[character.row]


def select_entries(picks):
    """Open every port as a CPU and take one icon with each."""
    entries = []
    for port, name in enumerate(picks, start=1):
        at = "" if port == 1 else f"@{port}"
        across, up = character_moves(port, CHARACTERS[name])
        entries.append(f"{TOGGLE_UP}{at}")
        entries.append(f"{OPEN_PORT}:A{at}")  # N/A -> HMN
        entries.append(f"{MAKE_CPU}:A{at}")   # HMN -> CPU
        if across:
            stick = 127 if across > 0 else -127
            entries.append(
                f"{MOVE_ACROSS}-{MOVE_ACROSS + abs(across) - 1}:SX={stick}{at}")
        entries.append(f"{MOVE_UP}-{MOVE_UP + up - 1}:SY=127{at}")
        entries.append(f"{TAKE_ICON}:A{at}")
    return entries


def rules_entries(kind, steps):
    """Set the match kind and the count under it, then return to the doors.

    Pad 1 drives its cursor into the corner, which clamps it, and back under
    the rules button, so the icon it came from does not matter.  RIGHT on the
    first row walks Time, Stock, Coin and Bonus; the row below holds that
    kind's count, which LEFT and RIGHT step one at a time.
    """
    entries = [f"{RULES_CORNER}-{RULES_CORNER + 39}:SX=-127",
               f"{RULES_CORNER}-{RULES_CORNER + 39}:SY=127",
               f"{RULES_CORNER + 41}-{RULES_CORNER + 58}:SX=127",
               f"{RULES_OPEN}:A"]
    frame = RULES_FIRST_PRESS
    for _ in range(MATCH_KINDS.index(kind)):
        entries.append(f"{frame}:RIGHT")
        frame += RULES_PRESS_GAP
    if steps:
        entries.append(f"{frame}:DOWN")
        frame += RULES_PRESS_GAP
        for _ in range(abs(steps)):
            entries.append(f"{frame}:{'RIGHT' if steps > 0 else 'LEFT'}")
            frame += RULES_PRESS_GAP
    entries.append(f"{frame}:RULES")  # prints the three back
    frame += RULES_PRESS_GAP
    entries.append(f"{frame}:B")      # back to character select
    return entries, frame + 60


def stage_entries(start):
    """Leave character select and take Hyrule Temple on the stage screen."""
    entries = [f"{start}:START",
               f"{start + 40}-{start + 41}:SX=-127",
               f"{start + 45}-{start + 59}:SY=127",
               f"{start + 65}:A"]
    return entries, start + 65 + STAGE_TO_MATCH


def match_entries(first, config):
    """The fight itself: the clock, the samples, and how the match ends."""
    entries = []
    if config["clock"] and config["kind"] == "time":
        entries.append(f"{first + 120}:CLOCK={config['clock']}")
    last = first + config["frames"]
    for frame in range(first + 150, last, config["sample_every"]):
        entries.append(f"{frame}:FIGHTERS")
        entries.append(f"{frame}:RESULT")
    if config["until_end"]:
        # Nothing quits the match; it ends on its own clock or its last stock,
        # and the route stops once the budget is spent.  STOP leaves the host
        # on the frame it lands on, before anything else that frame asks for,
        # so the last look at the match goes one frame earlier.
        last -= 1
    if config["trace"]:
        entries.append(f"{first + 1}-{last}:TRACE={config['trace']}")
    elif config["final_trace"]:
        entries.append(f"{last - 3}-{last}:TRACE={config['final_trace']}")
    if config["until_end"]:
        entries.append(f"{last}:RESULT")
        entries.append(f"{last + 1}:STOP")
        return entries
    # Pause and quit with L+R+A+START, which the match takes as no contest,
    # then walk the results screen back to character select and the menu.
    entries.append(f"{last}:START")
    entries.append(f"{last + 20}-{last + 35}:L+R+A")
    entries.append(f"{last + 25}-{last + 35}:START")
    results = last + PAUSE_TO_RESULTS
    entries.append(f"{results + RESULTS_PAST_WINNER}:START")
    for port in range(1, config["players"] + 1):
        at = "" if port == 1 else f"@{port}"
        entries.append(f"{results + RESULTS_READY}:START{at}")
    entries.append(f"{results + RESULTS_LEAVE}-{results + RESULTS_LEAVE + 200}:B")
    entries.append(f"{results + RESULTS_LEAVE + 60}:RESULT")
    return entries


def build_route(config):
    """The whole pad script for one match, in run-modes' FRAME:INPUT form."""
    entries = ["120:START", "160:DOWN", "200:A", "240:A"]
    entries += select_entries(config["picks"])
    if config["rules"]:
        rules, next_frame = rules_entries(config["kind"], config["steps"])
        entries += rules
    else:
        next_frame = TAKE_ICON + 16
    stage, first_frame = stage_entries(next_frame)
    entries += stage
    entries += match_entries(first_frame, config)
    return entries, first_frame


def draw_config(rng, args, index):
    """Roll one match: who plays it, under which rules, for how long."""
    players = args.players or rng.choice([2, 2, 2, 3, 4])
    picks = [rng.choice(args.roster) for _ in range(players)]
    kind = rng.choice(args.kinds) if args.rules else "time"
    steps = 0
    if args.rules:
        # The count under the match kind: minutes for a Time match, stocks for
        # a Stock one, starting from the defaults of two minutes and three
        # stocks.  Coin and Bonus matches keep their own default.
        if kind == "time":
            # A match that is quit at a frame cannot be allowed to run out of
            # clock first, or the quit lands on the results screen instead.
            floor = 1
            if not args.until_end:
                floor = max(1, -(-(args.match_frames + 900) // 3600))
            steps = rng.randint(max(-1, floor - 2), 3)
        elif kind == "stock":
            steps = rng.randint(-2, 2)
    config = {
        "index": index,
        "players": players,
        "picks": picks,
        "rules": args.rules,
        "kind": kind,
        "steps": steps,
        "clock": args.clock,
        "frames": args.match_frames,
        "sample_every": args.sample_every,
        "until_end": args.until_end,
        "trace": None,
        "final_trace": None,
    }
    return config


SEEN_FILE = re.compile(r"/[^\s:]*/(?=[\w.-]+\.(?:c|cpp|h|hpp):\d+)")

# What a broken run says about itself, most telling first: the assertion that
# ends a failed archive load says less than the symbol it could not find, so
# the symbol wins when both are there.
CRASH_MARKS = (
    ("OS panic", "no host translation", "cannot translate",
     "Cannot find symbol", "Memory Empty", "unported"),
    ("HSD assertion", "assertion failed", "Segmentation", "did not run",
     "stopped: ", "could not be", "timed out"),
)


def crash_signature(text):
    """The line the host died on, with its absolute paths cut back to names."""
    lines = text.strip().splitlines()
    for marks in CRASH_MARKS:
        for line in reversed(lines):
            if any(mark in line for mark in marks):
                return SEEN_FILE.sub("", line.strip())[:200]
    return ""


def parse_output(text):
    """The diagnostics the route asked for, as far as the run got."""
    report = {}
    selection = re.search(r"vs selection: stage (\d+)((?: \d+=-?\d+)*)", text)
    if selection is not None:
        report["stage"] = int(selection.group(1))
        report["selected"] = [int(part.split("=")[1])
                              for part in selection.group(2).split()]
    scenes = re.search(r"^scenes: (.+)$", text, re.MULTILINE)
    if scenes is not None:
        report["scenes"] = scenes.group(1)
    rules = re.findall(r"rules frame \d+: mode (\d+) time (\d+) stock (\d+)",
                       text)
    if rules:
        mode, limit, stocks = rules[-1]
        report["rules"] = {"mode": int(mode), "time": int(limit),
                           "stock": int(stocks)}
    results = re.findall(r"result frame (\d+): outcome (\d+) winners (\d+) "
                         r"first (\d+) stocks P1=(-?\d+) P2=(-?\d+)", text)
    if results:
        frame, outcome, winners, first, p1, p2 = results[-1]
        report["result"] = {"frame": int(frame), "outcome": int(outcome),
                            "winners": int(winners), "first": int(first),
                            "stocks": [int(p1), int(p2)]}
    entered = re.findall(r"scene (0x[0-9a-f]+) from frame (\d+)", text)
    if entered:
        report["last_scene"] = entered[-1][0]
    match_scene = re.search(r"scene 0x02 from frame (\d+)", text)
    if match_scene is not None:
        report["match_from"] = int(match_scene.group(1))
    after = re.findall(r"scene 0x0[35] from frame (\d+)", text)
    if after:
        report["match_until"] = int(after[0])
    samples = re.findall(r"fighters frame (\d+): (.+)", text)
    if samples:
        report["samples"] = len(samples)
        report["last_positions"] = samples[-1][1].strip()
    # The last frame any diagnostic reported, which is how far a run that died
    # in the middle of a match got.
    reached = re.findall(r"(?:from )?frame (\d+)", text)
    if reached:
        report["reached_frame"] = max(int(frame) for frame in reached)
    return report


def parse_trace_tail(path):
    """The last traced frame: what each fighter was doing when it was taken."""
    try:
        with open(path, encoding="ascii", errors="replace") as stream:
            lines = stream.read().strip().splitlines()
    except OSError:
        return None
    if not lines:
        return None
    fighters = {}
    current = None
    for token in lines[-1].split():
        if re.fullmatch(r"P[1-4]", token):
            current = token
            fighters[current] = {}
        elif current is not None and "=" in token:
            key, value = token.split("=", 1)
            if key in ("percent", "x", "y"):
                fighters[current][key] = _float_bits(value)
            elif key == "stocks":
                fighters[current][key] = int(value)
    return fighters or None


def _float_bits(text):
    """The traces write floats as the eight hex digits of their bits."""
    try:
        return round(struct.unpack(">f", bytes.fromhex(text))[0], 3)
    except (ValueError, struct.error):
        return None


def host_command(args, route):
    """The host's stdout into a pipe is block buffered, and a run that dies
    takes the buffer with it, so the lines before the crash are the ones worth
    keeping: stdbuf hands them over a line at a time."""
    command = [args.binary, "--run-modes", args.root, "0", "3"] + route
    return (["stdbuf", "-oL"] + command) if args.line_buffered else command


def run_match(config, args):
    """One melee-pc run.  Never raises: a broken run is a result too."""
    route, first_frame = build_route(config)
    command = host_command(args, route)
    environment = dict(os.environ, LANG="C", LC_ALL="C")
    if args.no_audio:
        environment["MELEE_HOST_AUDIO"] = "0"
    scratch = None
    # With --until-end nobody knows which frame the match ends on, so the four
    # frame trace would read a results screen or a sudden death rather than the
    # fight; there the RESULT sample says what happened instead.
    if config["trace"] is None and not args.no_trace and not config["until_end"]:
        handle, scratch = tempfile.mkstemp(prefix="melee-trace-", suffix=".txt")
        os.close(handle)
        config["final_trace"] = scratch
        route, first_frame = build_route(config)
        command = host_command(args, route)
    started = time.monotonic()
    try:
        done = subprocess.run(command, capture_output=True, text=True,
                              timeout=args.timeout, env=environment,
                              cwd=args.cwd)
        code, text = done.returncode, done.stdout + done.stderr
    except subprocess.TimeoutExpired as expired:
        code, text = None, _expired_text(expired)
    except OSError as error:
        code, text = None, f"the host could not be started: {error}"
    seconds = round(time.monotonic() - started, 2)

    record = {
        "match": config["index"],
        "when": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "seconds": seconds,
        "players": config["players"],
        "picks": config["picks"],
        "kind": config["kind"],
        "count_steps": config["steps"],
        "match_frames": config["frames"],
        "first_frame": first_frame,
        "exit": code,
        "route": route,
    }
    record.update(parse_output(text))
    if scratch is not None:
        fighters = parse_trace_tail(scratch)
        if fighters:
            record["final"] = fighters
        os.unlink(scratch)
    elif config["trace"]:
        record["trace"] = config["trace"]
        fighters = parse_trace_tail(config["trace"])
        if fighters:
            record["final"] = fighters

    if code is None:
        record["status"] = "timeout" if "timed out" in text else "unstarted"
    elif code < 0:
        record["signal"] = signal.Signals(-code).name
        # SIGTERM and friends are not the host falling over: something outside
        # this tool took the process, and the match says nothing about the
        # port either way.
        record["status"] = ("killed"
                            if record["signal"] in ("SIGTERM", "SIGKILL",
                                                    "SIGHUP", "SIGINT")
                            else "signal")
    elif code != 0:
        record["status"] = "failed"
    elif config["until_end"]:
        record["status"] = "ok" if "stopped at frame" in text else "short"
    elif "selected" not in record:
        # A match that ran out of clock or stocks before the route quit it
        # leaves the rest of the route pressing buttons at the wrong screens.
        # That is the match ending, not the host breaking.
        ended = record.get("match_until")
        record["status"] = ("early" if ended and ended < first_frame +
                            config["frames"] else "short")
    elif record.get("selected") != [CHARACTERS[name].kind
                                    for name in config["picks"]]:
        # The cursor took an icon nobody asked for, so the measured moves no
        # longer match this build's character select.
        record["status"] = "misselected"
        record["selected_names"] = [BY_KIND.get(kind, str(kind))
                                    for kind in record["selected"]]
    else:
        record["status"] = "ok"
    if record["status"] != "ok":
        # A run that dies without a word still groups with the others that
        # died the same way, so the fallback names the signal, not the last
        # frame a diagnostic happened to print.
        scene = record.get("last_scene")
        scene = (f"{scene} ({SCENE_NAMES.get(scene, 'a scene of its own')})"
                 if scene else "no scene at all")
        if record["status"] == "killed":
            record["signature"] = (f"{record['signal']}: the run was killed "
                                   f"from outside, in {scene}")
        else:
            record["signature"] = (
                crash_signature(text) or
                (f"{record['signal']} with no message from the host, "
                 f"in {scene}" if record["status"] == "signal"
                 else _last_lines(text, 1)))
        record["output"] = _last_lines(text, args.log_lines)
    return record


def _expired_text(expired):
    parts = ["the run timed out"]
    for stream in (expired.stdout, expired.stderr):
        if stream:
            parts.append(stream if isinstance(stream, str)
                         else stream.decode("utf-8", "replace"))
    return "\n".join(parts)


def _last_lines(text, count):
    return "\n".join(text.strip().splitlines()[-count:])


def quote(command):
    return " ".join(part if re.fullmatch(r"[\w./=:@+-]+", part)
                    else "'" + part.replace("'", "'\\''") + "'"
                    for part in command)


def write_failure(log, record, args):
    """Everything needed to understand and replay one broken run."""
    command = [args.binary, "--run-modes", args.root, "0", "3"] + record["route"]
    picks = " vs ".join(record["picks"])
    log.write(f"\n{'=' * 78}\n")
    log.write(f"match {record['match']} at {record['when']}: "
              f"{record['status']}\n")
    log.write(f"  fighters : {picks} on stage {STAGE_KIND} "
              f"({record['kind']} match, {record['match_frames']} frames)\n")
    log.write(f"  exit     : {record.get('signal') or record['exit']}"
              f" after {record['seconds']}s\n")
    if record.get("reached_frame"):
        log.write(f"  reached  : frame {record['reached_frame']} "
                  f"(the match starts on {record['first_frame']})\n")
    if record.get("selected_names"):
        log.write(f"  took     : {' vs '.join(record['selected_names'])}\n")
    if record.get("signature"):
        log.write(f"  stopped  : {record['signature']}\n")
    if record.get("scenes"):
        log.write(f"  scenes   : {record['scenes']}\n")
    log.write(f"  replay   : cd {args.cwd} && {quote(command)}\n")
    if record.get("output"):
        log.write("  output   |\n")
        for line in record["output"].splitlines():
            log.write(f"    {line}\n")
    log.flush()


def summarise(records, stream):
    if not records:
        stream.write("no match was played\n")
        return
    counts = collections.Counter(record["status"] for record in records)
    played = counts["ok"] + counts["early"]
    stream.write(f"\n{len(records)} matches run: " +
                 ", ".join(f"{count} {status}"
                           for status, count in counts.most_common()) + "\n")
    seconds = sum(record["seconds"] for record in records)
    stream.write(f"{seconds:.0f}s of host time, "
                 f"{seconds / len(records):.1f}s a match\n")
    broken = [record for record in records
              if record["status"] not in OK_STATUSES]
    if broken:
        stream.write("\nwhat stopped the runs:\n")
        for signature, count in collections.Counter(
                record.get("signature", "") for record in broken).most_common():
            stream.write(f"  {count:4d}  {signature}\n")
    # One fighter is in more broken runs than another simply by being drawn
    # more often, so the count each was in is next to it.
    seen, blamed = collections.Counter(), collections.Counter()
    for record in records:
        seen.update(set(record["picks"]))
        if record["status"] not in OK_STATUSES:
            blamed.update(set(record["picks"]))
    stream.write("\nfighters, and the matches of theirs that broke:\n")
    for name, count in sorted(seen.items(),
                              key=lambda pair: (-blamed[pair[0]] / pair[1],
                                                -pair[1])):
        stream.write(f"  {name:<12} {blamed[name]:3d} of {count:3d}\n")
    if played:
        stream.write(f"\n{played} match(es) played to the end\n")


def roster_from(text):
    if text in ("loadable", ""):
        return list(LOADABLE)
    if text == "all":
        return list(CHARACTERS)
    if text == "stable":
        return list(STABLE)
    names = []
    for part in text.split(","):
        name = part.strip().lower()
        if name not in CHARACTERS:
            raise SystemExit(f"{name}: not a character on this select screen; "
                             f"pick from {', '.join(sorted(CHARACTERS))}")
        names.append(name)
    if not names:
        raise SystemExit("--characters needs at least one name")
    return names


def main():
    parser = argparse.ArgumentParser(
        description="play random CPU matches, log the runs that break",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="characters: " + ", ".join(sorted(CHARACTERS)) +
               "\nsets: loadable (default), stable, all")
    parser.add_argument("--binary", default="build/host-release/port/melee-pc")
    parser.add_argument("--root", default="assets-local",
                        help="the extracted disc the host reads (default: "
                             "assets-local)")
    parser.add_argument("--cwd", default=ROOT_DIR,
                        help="where to run from (default: the repository)")
    parser.add_argument("--matches", type=int, default=0,
                        help="how many to play; 0 plays until interrupted")
    parser.add_argument("--jobs", type=int, default=1,
                        help="matches to run at the same time")
    parser.add_argument("--seed", type=int,
                        help="seed for the choices, to repeat a session")
    parser.add_argument("--players", type=int, choices=(2, 3, 4),
                        help="ports in every match (default: random, mostly 2)")
    parser.add_argument("--characters", default="loadable",
                        help="a set or a comma separated list of names")
    parser.add_argument("--kinds", default="time,stock",
                        help="match kinds to draw from: " +
                             ",".join(MATCH_KINDS))
    parser.add_argument("--no-rules", dest="rules", action="store_false",
                        help="leave the rules menu alone (two minute Time)")
    parser.add_argument("--match-frames", type=int, default=1800,
                        help="drawn frames of fighting before the match is "
                             "quit, or the budget with --until-end "
                             "(default: 1800, half a minute)")
    parser.add_argument("--until-end", action="store_true",
                        help="let the match end on its own clock or stocks "
                             "instead of quitting it; the route stops at the "
                             "budget, so nothing is read back from the menus")
    parser.add_argument("--clock", type=int,
                        help="seconds to put on a Time match's clock once the "
                             "HUD is up, to end it sooner")
    parser.add_argument("--sample-every", type=int, default=600,
                        help="frames between the position and result samples")
    parser.add_argument("--trace-dir",
                        help="write a full per-frame match trace per run here")
    parser.add_argument("--no-trace", action="store_true",
                        help="skip even the four frame trace that reads the "
                             "damage and stocks back")
    parser.add_argument("--no-audio", action="store_true",
                        help="run with MELEE_HOST_AUDIO=0")
    parser.add_argument("--timeout", type=float, default=900.0,
                        help="seconds before a run is killed (default: 900)")
    parser.add_argument("--log", default="cpu-match-crashes.log",
                        help="where the broken runs are written")
    parser.add_argument("--results", default="cpu-matches.jsonl",
                        help="one JSON line a match, broken or not")
    parser.add_argument("--log-lines", type=int, default=25,
                        help="lines of output kept for a broken run")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args()

    args.roster = roster_from(args.characters)
    args.kinds = [kind.strip() for kind in args.kinds.split(",") if kind.strip()]
    for kind in args.kinds:
        if kind not in MATCH_KINDS:
            raise SystemExit(f"{kind}: not a match kind; pick from "
                             f"{', '.join(MATCH_KINDS)}")
    if not os.path.isabs(args.binary):
        args.binary = os.path.join(args.cwd, args.binary)
    if not os.path.exists(args.binary):
        raise SystemExit(f"{args.binary}: no host binary; build it with\n"
                         f"  cmake --build --preset host-release")
    if not os.path.isdir(os.path.join(args.cwd, args.root)) and \
            not os.path.isdir(args.root):
        raise SystemExit(f"{args.root}: no extracted disc to read")
    if args.trace_dir:
        os.makedirs(args.trace_dir, exist_ok=True)
    args.line_buffered = shutil.which("stdbuf") is not None

    seed = args.seed if args.seed is not None else random.randrange(1 << 30)
    rng = random.Random(seed)
    if not args.quiet:
        print(f"seed {seed}, roster: {', '.join(sorted(args.roster))}")
        print(f"logging broken runs to {args.log}, "
              f"every match to {args.results}")

    records = []
    stop = False

    def interrupted(*_):
        nonlocal stop
        if stop:  # a second one does not wait for the running matches
            signal.signal(signal.SIGINT, signal.SIG_DFL)
            raise KeyboardInterrupt
        stop = True
        print("\nfinishing the matches already running; "
              "interrupt again to drop them\n", file=sys.stderr)

    signal.signal(signal.SIGINT, interrupted)

    log = open(args.log, "a", encoding="utf-8")
    results = open(args.results, "a", encoding="utf-8")
    log.write(f"\n# session {time.strftime('%Y-%m-%dT%H:%M:%S')} seed {seed} "
              f"roster {','.join(sorted(args.roster))}\n")
    log.flush()

    def issue(index):
        config = draw_config(rng, args, index)
        if args.trace_dir:
            config["trace"] = os.path.join(args.trace_dir,
                                           f"match-{index:05d}.txt")
        return config

    def finish(record):
        records.append(record)
        results.write(json.dumps(record) + "\n")
        results.flush()
        if record["status"] not in OK_STATUSES:
            write_failure(log, record, args)
        if not args.quiet:
            picks = " vs ".join(record["picks"])
            note = ""
            if record["status"] != "ok":
                note = record.get("signature", "")[:90]
            elif record.get("final"):
                note = " ".join(
                    f"{port} {state.get('percent', 0):.0f}%"
                    f"/{state.get('stocks', '?')}"
                    for port, state in sorted(record["final"].items()))
            elif record.get("result"):
                result = record["result"]
                note = (f"outcome {result['outcome']}, "
                        f"{result['winners']} winner(s), stocks " +
                        "-".join(str(count) for count in result["stocks"]))
            print(f"[{record['match']:5d}] {record['status']:<11} "
                  f"{record['seconds']:6.1f}s  {picks:<28} {note}")

    try:
        with futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            running = {}
            index = 0
            while not stop:
                while len(running) < args.jobs and not stop:
                    if args.matches and index >= args.matches:
                        break
                    index += 1
                    config = issue(index)
                    running[pool.submit(run_match, config, args)] = config
                if not running:
                    break
                done, _ = futures.wait(
                    running, return_when=futures.FIRST_COMPLETED)
                for future in done:
                    running.pop(future)
                    finish(future.result())
            for future in futures.as_completed(list(running)):
                finish(future.result())
    except KeyboardInterrupt:
        print("dropped the matches still running", file=sys.stderr)
    finally:
        summarise(records, sys.stdout)
        summarise(records, log)
        log.close()
        results.close()
    # Broken runs are what this tool is for, so they are not a failure of the
    # tool.  Nothing playable at all is: the build, the disc or the routes are
    # not what this expects.
    return 0 if any(record["status"] in OK_STATUSES
                    for record in records) else 1


if __name__ == "__main__":
    sys.exit(main())

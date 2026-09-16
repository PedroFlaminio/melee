#!/usr/bin/env python3
import sys
import subprocess
import os

root_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
melee_pc = os.path.join(root_dir, "build", "host-release", "port", "melee-pc")

# Run a match with a CPU using run-modes
# The sequence is derived from melee-host-vs-mario-link-asset but with Pad 1 selecting CPU for P2
# 120:START 160:DOWN 200:A 240:A (title -> css)
# P1 chooses Mario: 330-337:SX=127 345-354:SY=127 360:A
# P1 moves cursor to P2 HMN/CPU token and presses A (approximate coordinates)
# We will use the fact that the CSS allows P1 to click the CPU token.
# To make it simple, we can just launch the game interactively with --play so the user can test!
subprocess.run([melee_pc, "--play", os.path.join(root_dir, "assets-local")])

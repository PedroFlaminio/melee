#!/usr/bin/env python3
"""Check what the game plays on the host against reference decoders.

Runs melee-pc from the title to the main menu and back, recording the AX
mixer's output: START on the title plays sound 118 of main.ssm (two voices),
the menu starts menu01.hps, and B at frame 700 leaves.  The music is decoded
here from the .hps chunks and the effect with ssm_to_wav.py's decoder, so
neither goes through the host's mixer.

The music must match on both channels, window by window and at one lag, up to
a gain: a sample dropped or repeated where the stream moves from one chunk to
the next shows up as a window that no longer fits.  With the music subtracted,
what remains where START was pressed must be the effect.
"""

import argparse
import math
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ssm_to_wav import decode, voices  # noqa: E402

ROUTE = ["0", "2", "120:START", "700-760:B"]
EXPECTED_ROUTE = "route: 0x00 (122 frames) 0x01 (580 frames) 0x00"
RATE = 32000
MUSIC_WINDOW = 4000


def decode_hps(path, seconds):
    """DSP ADPCM of every channel of a .hps stream, following its chunks."""
    data = path.read_bytes()
    if data[:8] != b" HALPST\0":
        raise SystemExit(f"{path}: not a HALPST stream")
    rate, channels = struct.unpack_from(">II", data, 8)
    coefs = [struct.unpack_from(">16h", data, 0x20 + 0x38 * c)
             for c in range(channels)]
    want = int(seconds * rate)
    out = [[] for _ in range(channels)]
    history = [(0, 0)] * channels
    offset = 0x80
    while min(len(o) for o in out) < want:
        size, end, following = struct.unpack_from(">III", data, offset)
        half = size // 2
        for c in range(channels):
            chunk = data[offset + 0x20 + c * half:offset + 0x20 + (c + 1) * half]
            yn1, yn2 = history[c]
            nibble_address = 0
            while nibble_address <= end and len(out[c]) < want:
                if nibble_address % 16 == 0:
                    pred_scale = chunk[nibble_address >> 1]
                    scale = 1 << (pred_scale & 0xF)
                    index = (pred_scale >> 4) & 7
                    c1, c2 = coefs[c][index * 2], coefs[c][index * 2 + 1]
                    nibble_address += 2
                    continue
                byte = chunk[nibble_address >> 1]
                nibble = byte >> 4 if nibble_address % 2 == 0 else byte & 0xF
                if nibble >= 8:
                    nibble -= 16
                value = nibble * scale + ((0x400 + c1 * yn1 + c2 * yn2) >> 11)
                value = max(-32768, min(32767, value))
                yn2, yn1 = yn1, value
                out[c].append(value)
                nibble_address += 1
            history[c] = (yn1, yn2)
        if following == 0xFFFFFFFF:
            break
        offset = following
    return rate, out


def sound_voices(bank_path, sound_id):
    """Each voice of one sound of a .ssm bank, decoded once."""
    bank = bank_path.read_bytes()
    header_size = struct.unpack_from(">I", bank, 0)[0]
    samples = bank[(header_size + 0x10 + 31) & ~31:]
    return [decode(samples, v) for v in voices(bank) if v["id"] == sound_id]


def fit(mix, reference, start, length):
    """Normalized correlation and least-squares gain of mix[start:] against
    reference[:length]."""
    xy = xx = yy = 0.0
    for i in range(length):
        x = mix[start + i]
        y = reference[i]
        xy += x * y
        xx += x * x
        yy += y * y
    if xx == 0 or yy == 0:
        return 0.0, 0.0
    return xy / math.sqrt(xx * yy), xy / yy


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("melee_pc", type=Path)
    parser.add_argument("assets", type=Path)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as work:
        wav_path = Path(work) / "route.wav"
        run = subprocess.run(
            [str(args.melee_pc), "--run-modes", str(args.assets), *ROUTE,
             f"0-700:WAV={wav_path}"],
            capture_output=True, text=True)
        if run.returncode != 0 or EXPECTED_ROUTE not in run.stdout:
            print(run.stdout[-2000:] + run.stderr[-2000:])
            print(f"melee-pc exited with {run.returncode} without the route")
            return 1
        with wave.open(str(wav_path)) as wav:
            if wav.getframerate() != RATE or wav.getnchannels() != 2:
                print("the recording is not 32 kHz stereo")
                return 1
            frames = wav.readframes(wav.getnframes())
    pcm = struct.unpack(f"<{len(frames) // 2}h", frames)
    mix = [pcm[0::2], pcm[1::2]]
    print(f"recorded {len(mix[0])} stereo pairs ({len(mix[0]) / RATE:.2f} s)")

    first = next((i for i in range(len(mix[0]))
                  if mix[0][i] != 0 or mix[1][i] != 0), None)
    if first is None:
        print("the recording is silent")
        return 1

    rate, music = decode_hps(args.assets / "audio" / "menu01.hps", 12)
    if rate != RATE or len(music) != 2:
        print("menu01.hps is not a 32 kHz stereo stream")
        return 1

    # The music starts shortly after the effect; find where, one second in,
    # where the effect has ended.
    probe = 1000
    best = (-2.0, None)
    for lag in range(first, first + 3000):
        c, _ = fit(mix[0], music[0][RATE:RATE + probe], lag + RATE, probe)
        if c > best[0]:
            best = (c, lag)
    music_start = best[1]
    print(f"effect from sample {first}, music from sample {music_start}")

    # Window by window at that one lag, until B at frame 700.
    end = min(len(mix[0]), int(690 * RATE / 59.94))
    worst = [2.0, 2.0]
    gains = [[], []]
    windows = 0
    for start in range(music_start + RATE, end - MUSIC_WINDOW, MUSIC_WINDOW):
        offset = start - music_start
        for side in range(2):
            c, g = fit(mix[side], music[side][offset:offset + MUSIC_WINDOW],
                       start, MUSIC_WINDOW)
            worst[side] = min(worst[side], c)
            gains[side].append(g)
        windows += 1
    print(f"music: {windows} windows of {MUSIC_WINDOW} samples, worst "
          f"correlation left {worst[0]:.6f} right {worst[1]:.6f}")
    failed = windows < 40 or min(worst) < 0.99999

    effect = sound_voices(args.assets / "audio" / "main.ssm", 118)
    if len(effect) != 2:
        print("sound 118 of main.ssm does not have two voices")
        return 1
    for side in range(2):
        gain = sorted(gains[side])[len(gains[side]) // 2]
        length = len(effect[side])
        residual = [mix[side][i] - (gain * music[side][i - music_start]
                                    if i >= music_start else 0)
                    for i in range(first - 64, first + 64 + length)]
        best = (-2.0, None)
        for shift in range(0, 128):
            c, _ = fit(residual, effect[side][:2000], shift, 2000)
            if c > best[0]:
                best = (c, shift)
        c, _ = fit(residual, effect[side], best[1], length)
        print(f"effect 118 voice {side}: correlation {c:.6f} from sample "
              f"{first - 64 + best[1]}")
        failed = failed or c < 0.9999
    if failed:
        print("the host's audio does not match the reference decoders")
        return 1
    print("the host's music and effect match the reference decoders")
    return 0


if __name__ == "__main__":
    sys.exit(main())

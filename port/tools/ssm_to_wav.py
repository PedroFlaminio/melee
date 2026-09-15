#!/usr/bin/env python3
"""Decode the ADPCM voices of a Melee .ssm sound bank into WAV files.

A bank starts with four big-endian words: the header size, the size of the
sample data, the number of sounds and the first sound id.  Each sound follows
as its voice count and sample rate, then one 0x40-byte block per voice holding
the AXPBADDR (loop flag, format, loop, end and current addresses in nibbles
from the start of the sample data), the AXPBADPCM (eight coefficient pairs,
gain, predictor and scale, history) and the AXPBADPCMLOOP.  The sample data
starts at the header size plus 0x10, rounded up to 32 bytes, and is what the
synth copies to ARAM.

This is a reference for the host's AX mixer: it decodes each voice from its
current address through its end address, once, and prints what it measured.
"""

import argparse
import struct
import sys
import wave
from pathlib import Path


def voices(bank):
    header_size, data_size, count, first_id = struct.unpack(">4I", bank[:16])
    data = (header_size + 0x10 + 31) & ~31
    offset = 0x10
    for sound in range(count):
        voice_count, rate = struct.unpack(">II", bank[offset:offset + 8])
        for index in range(voice_count):
            block = bank[offset + 8 + index * 0x40:offset + 8 + (index + 1) * 0x40]
            loop_flag, fmt, loop_hi, loop_lo, end_hi, end_lo, cur_hi, cur_lo = \
                struct.unpack(">8H", block[:16])
            coefs = struct.unpack(">16h", block[16:48])
            _gain, pred_scale, yn1, yn2 = struct.unpack(">Hhhh", block[48:56])
            yield {
                "id": first_id + sound, "voice": index, "rate": rate,
                "loop": loop_flag, "format": fmt,
                "loop_address": (loop_hi << 16) | loop_lo,
                "end": (end_hi << 16) | end_lo,
                "current": (cur_hi << 16) | cur_lo,
                "coefs": coefs, "pred_scale": pred_scale & 0xFF,
                "yn1": yn1, "yn2": yn2,
            }
        offset += 8 + voice_count * 0x40
    return data, data_size


def decode(samples, voice):
    """DSP ADPCM from the current nibble through the end nibble, inclusive.
    Nibble addresses that are multiples of 16 hold a frame's predictor and
    scale byte, which the decoder reads instead of a sample."""
    out = []
    coefs = voice["coefs"]
    pred_scale = voice["pred_scale"]
    yn1, yn2 = voice["yn1"], voice["yn2"]
    address = voice["current"]
    end = voice["end"]
    while address <= end:
        if address % 16 == 0:
            pred_scale = samples[address // 2]
            address += 2
            continue
        byte = samples[address // 2]
        nibble = (byte >> 4) if address % 2 == 0 else (byte & 0xF)
        if nibble >= 8:
            nibble -= 16
        scale = 1 << (pred_scale & 0xF)
        index = (pred_scale >> 4) & 0x7
        c1, c2 = coefs[index * 2], coefs[index * 2 + 1]
        value = nibble * scale + ((0x400 + c1 * yn1 + c2 * yn2) >> 11)
        value = max(-32768, min(32767, value))
        yn2, yn1 = yn1, value
        out.append(value)
        address += 1
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bank", type=Path)
    parser.add_argument("--out", type=Path, help="directory for WAV files")
    parser.add_argument("--first", type=int, default=0,
                        help="first voice to decode")
    parser.add_argument("--count", type=int, default=1 << 30,
                        help="voices to decode")
    args = parser.parse_args()
    bank = args.bank.read_bytes()
    generator = voices(bank)
    listed = []
    try:
        while True:
            listed.append(next(generator))
    except StopIteration as stop:
        data, data_size = stop.value
    samples = bank[data:data + data_size]
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)
    for voice in listed[args.first:args.first + args.count]:
        if voice["format"] != 0:
            print(f"id 0x{voice['id']:x} voice {voice['voice']}: "
                  f"format {voice['format']} is not ADPCM")
            continue
        pcm = decode(samples, voice)
        peak = max((abs(s) for s in pcm), default=0)
        rms = (sum(s * s for s in pcm) / len(pcm)) ** 0.5 if pcm else 0.0
        clipped = sum(1 for s in pcm if s in (32767, -32768))
        step = (sum(abs(b - a) for a, b in zip(pcm, pcm[1:])) / (len(pcm) - 1)
                if len(pcm) > 1 else 0.0)
        print(f"id 0x{voice['id']:x} voice {voice['voice']}: "
              f"{len(pcm)} samples at {voice['rate']} Hz "
              f"({len(pcm) / voice['rate']:.3f} s), loop {voice['loop']}, "
              f"peak {peak}, rms {rms:.0f}, clipped {clipped}, "
              f"mean step {step:.0f}")
        if args.out:
            path = args.out / f"{args.bank.stem}_{voice['id']:04x}_{voice['voice']}.wav"
            with wave.open(str(path), "wb") as wav:
                wav.setnchannels(1)
                wav.setsampwidth(2)
                wav.setframerate(voice["rate"])
                wav.writeframes(struct.pack(f"<{len(pcm)}h", *pcm))
    return 0


if __name__ == "__main__":
    sys.exit(main())

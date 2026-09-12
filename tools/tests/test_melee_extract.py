from __future__ import annotations

import hashlib
import struct
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import melee_extract  # noqa: E402


def make_test_disc(path: Path) -> bytes:
    image = bytearray(0x1000)
    image[0:6] = b"GALE01"
    struct.pack_into(">I", image, 0x1C, melee_extract.GAMECUBE_MAGIC)

    dol_offset = 0x500
    struct.pack_into(">I", image, 0x420, dol_offset)
    struct.pack_into(">I", image, dol_offset + 0x00, 0x100)
    struct.pack_into(">I", image, dol_offset + 0x90, 4)
    image[dol_offset + 0x100 : dol_offset + 0x104] = b"DOL!"

    fst_offset = 0x800
    file_offset = 0x900
    file_data = b"hello"
    names = b"hello.dat\0"
    fst = bytearray(24 + len(names))
    struct.pack_into(">III", fst, 0, 0x01000000, 0, 2)
    struct.pack_into(">III", fst, 12, 0, file_offset, len(file_data))
    fst[24:] = names
    struct.pack_into(">I", image, 0x424, fst_offset)
    struct.pack_into(">I", image, 0x428, len(fst))
    image[fst_offset : fst_offset + len(fst)] = fst
    image[file_offset : file_offset + len(file_data)] = file_data
    path.write_bytes(image)
    return bytes(image[dol_offset : dol_offset + 0x104])


class GameCubeDiscTest(unittest.TestCase):
    def test_reads_header_dol_and_fst(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "test.iso"
            dol = make_test_disc(image)
            disc = melee_extract.GameCubeDisc(image)

            self.assertEqual(disc.game_id, "GALE01")
            self.assertEqual(disc.dol_size, 0x104)
            self.assertEqual(disc.dol_sha1, hashlib.sha1(dol).hexdigest())
            self.assertEqual(len(disc.entries), 1)
            self.assertEqual(disc.entries[0].entry_number, 1)
            self.assertEqual(disc.entries[0].path, "hello.dat")
            self.assertEqual(disc.entries[0].offset, 0x900)
            self.assertFalse(disc.supported)

    def test_extracts_files_and_refuses_implicit_overwrite(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "test.iso"
            make_test_disc(image)
            disc = melee_extract.GameCubeDisc(image)
            disc.supported = True
            destination = root / "resources"

            manifest = disc.extract(destination)
            self.assertEqual((destination / "hello.dat").read_bytes(), b"hello")
            self.assertEqual(manifest["files"][0]["entry_number"], 1)
            self.assertTrue((destination / "manifest.json").is_file())
            index = (destination / "dvd-index.bin").read_bytes()
            self.assertEqual(index[:8], melee_extract.DVD_INDEX_MAGIC)
            self.assertTrue((destination / "sys/main.dol").is_file())

            with self.assertRaises(melee_extract.DiscError):
                disc.extract(destination)

    def test_rejects_non_gamecube_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "test.iso"
            image.write_bytes(bytes(melee_extract.DISC_HEADER_SIZE))
            with self.assertRaises(melee_extract.DiscError):
                melee_extract.GameCubeDisc(image)

    def test_output_path_cannot_escape_destination(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "test.iso"
            make_test_disc(image)
            disc = melee_extract.GameCubeDisc(image)
            with self.assertRaises(melee_extract.DiscError):
                disc._output_path(root / "out", "../escape")


if __name__ == "__main__":
    unittest.main()

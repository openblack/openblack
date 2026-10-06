"""Runs Black & White's ambience volume code (BW1W120 runblack.exe) under Unicorn on a synthetic island and
records the results, to check openblack's SoundMap and AtmosAudio against.

Real code: GSoundMap::Update with CalculateRadiusPointAndDistance, UpdateFromMap, AddAtmosType,
AtmosMapTypeInfo::Add, Terrain::GetAtmosType, LH3DIsland::GetAltitude and the volume calculation (fn_0071DD60);
GAudio::ProcessAtmosBanks; fn_005E2240 (alignment); LH3DSky SetTime/Time2SkyType. Stubbed: GGame::GetCamera,
GDebug::SetMessage and the LHAudio imports.

Regenerate a scenario (Python 3 with unicorn installed):
    python emu_soundmap.py <seed> <queries> ../scenarios/volumes_<seed>.json
"""
import json
import os
import random
import struct
import sys

from emu_common import Emulator

# Version 1.20 binaries, the executable decrypted
EXE = os.environ.get("BW1_EXE", "runblack.exe")

SOUND_MAP_UPDATE = 0x71D6F0
PROCESS_ATMOS_BANKS = 0x428FE0
ALIGNMENT = 0x5E2240
SKY_SET_TIME = 0x86A2C0
GET_CAMERA = 0x54C180
DEBUG_SET_MESSAGE = 0x511DA0

G_GAME = 0xD0195C
G_GLOBAL_AUDIO = 0xCD3B20
CAMERA_POSITION = 0xEA1DB8
INDEX_BLOCK = 0xE9C964
PTR_BLOCKS = 0xE9C564
SKY_TYPE = 0xFA26BC
SKY_TIMES = 0xFA2694  # dayFull, duskEnd, duskStart, nightFull
SOUND_INFO = 0xD9A908
IMPORT_SET_BANK_VOLUME = 0x8A9788
IMPORT_SET_GROUP = 0x8A978C

RETAIL_SOUND_INFO = [40.0, 70.0, 20.0, 50.0, 20.0, 120.0, 250.0, 200.0, 1500.0, 44444.0, 0.0, 0.0, 0.0, 0.0, 0.0]
CELL_COUNT = 17 * 17


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def make_island(rng):
    lookup = bytearray(32 * 32)
    blocks = []
    for bx in range(9, 15):
        for bz in range(9, 15):
            if rng.random() < 0.25:
                continue
            cells = bytearray(CELL_COUNT * 8)
            base = rng.choice([0, 2, 5, 40, 120])
            for i in range(CELL_COUNT):
                altitude = max(0, min(255, base + rng.randint(-6, 30)))
                if rng.random() < 0.15:
                    altitude = rng.randint(0, 6)
                properties = rng.randint(0, 255)
                flags = rng.randint(0, 255)
                if rng.random() < 0.3:
                    flags = (flags & ~0x3C) | (rng.randint(0, 9) << 2)
                struct.pack_into("<BBBBBBBB", cells, i * 8, 0, 0, 0, 0, altitude, 0, properties, flags)
            blocks.append(cells.hex())
            lookup[bx * 32 + bz] = len(blocks)
    return lookup, blocks


def make_queries(rng, count):
    queries = []
    for _ in range(count):
        if rng.random() < 0.85:
            x = rng.uniform(1300.0, 2700.0)
            z = rng.uniform(1300.0, 2700.0)
        else:
            x = rng.uniform(-300.0, 5500.0)
            z = rng.uniform(-300.0, 5500.0)
        y = rng.choice([rng.uniform(0.0, 150.0), rng.uniform(0.0, 400.0), rng.uniform(0.0, 400.0),
                        rng.uniform(0.0, 2000.0), rng.uniform(1000.0, 50000.0)])
        weather = [rng.randint(-128, 127) if rng.random() < 0.2 else rng.randint(0, 60) for _ in range(4)]
        if rng.random() < 0.4:
            weather = [0, 0, 0, 0]
        sky = rng.choice([0.0, 1.0, 2.0, rng.uniform(0.0, 2.0)])
        queries.append({"x": bits(f32(x)), "y": bits(f32(y)), "z": bits(f32(z)), "weather": weather,
                        "sky": bits(f32(sky))})
    return queries


def run_sound_map(emu, scenario):
    lookup = bytes.fromhex(scenario["lookup"])
    emu.write(INDEX_BLOCK, lookup)
    for index, cells in enumerate(scenario["blocks"], start=1):
        block = emu.alloc(CELL_COUNT * 8)
        emu.write(block, bytes.fromhex(cells))
        emu.w32(PTR_BLOCKS + index * 4, block)
    for i, value in enumerate(scenario["soundInfo"]):
        emu.w32(SOUND_INFO + i * 4, value)

    game = emu.alloc(0x260000)
    emu.w32(G_GAME, game)
    emu.w32(game + 0x59B8 + 0x28, 8)
    emu.w32(game + 0x59B8 + 0x2C, 0x2000)
    emu.w32(game + 0x59B8 + 0x30, 0x2000)
    camera = emu.alloc(0x200)
    emu.w32(game + 0x2502C0, camera)
    sound_map = emu.alloc(0x110)

    results = []
    for query in scenario["queries"]:
        emu.w32(CAMERA_POSITION + 0, query["x"])
        emu.w32(CAMERA_POSITION + 4, query["y"])
        emu.w32(CAMERA_POSITION + 8, query["z"])
        emu.write(camera + 0x80, bytes([0, query["weather"][0] & 0xFF, query["weather"][1] & 0xFF, 0,
                                        query["weather"][2] & 0xFF, query["weather"][3] & 0xFF, 0, 0]))
        emu.w32(SKY_TYPE, query["sky"])
        emu.call(SOUND_MAP_UPDATE, ecx=sound_map)
        results.append({
            "volumes": [emu.u32(sound_map + 0xB4 + 4 * i) for i in range(14)],
            "heightAboveLand": emu.u32(sound_map + 0x108),
        })
    return results


def run_alignment(emu, values):
    audio = emu.alloc(0x300)
    emu.w32(G_GLOBAL_AUDIO, audio)
    results = []
    for value in values:
        argument = bits(f32((f32(value) + 1.0) * 0.5))
        emu.call(ALIGNMENT, [argument])
        results.append(emu.u32(audio + 0x190))
    return results


def run_sky(emu, cases):
    results = []
    for case in cases:
        for i, value in enumerate(case["times"]):
            emu.w32(SKY_TIMES + i * 4, value)
        emu.call(SKY_SET_TIME, [case["time"]])
        results.append(emu.u32(SKY_TYPE))
    return results


def run_banks(emu, steps):
    sent = []
    emu.w32(IMPORT_SET_BANK_VOLUME, 0x0FF00010)
    emu.w32(IMPORT_SET_GROUP, 0x0FF00020)
    emu.uc.mem_map(0x0FF00000, 0x1000)
    emu.stub(0x0FF00010, lambda e: sent.append(e.arg(1)), 8)
    emu.stub(0x0FF00020, lambda e: 0, 8)
    audio = emu.alloc(0x300)
    emu.w32(audio + 0x14, emu.alloc(0x100))
    for i in range(14):
        emu.w32(audio + 0x194 + 4 * i, 0x100 + i)
    results = []
    for step in steps:
        for i in range(14):
            emu.w32(audio + 0x1CC + 4 * i, step["targets"][i])
            emu.w32(audio + 0x204 + 4 * i, step["current"][i])
        sent.clear()
        emu.call(PROCESS_ATMOS_BANKS, ecx=audio)
        results.append({"current": [emu.u32(audio + 0x204 + 4 * i) for i in range(14)],
                        "sent": [v - (1 << 32) if v & 0x80000000 else v for v in sent]})
    return results


def main():
    seed = int(sys.argv[1])
    query_count = int(sys.argv[2])
    out = sys.argv[3]
    rng = random.Random(seed)

    lookup, blocks = make_island(rng)
    sound_info = RETAIL_SOUND_INFO if seed % 2 else [rng.uniform(5.0, 90.0), rng.uniform(5.0, 120.0),
                                                     rng.uniform(5.0, 40.0), rng.uniform(41.0, 90.0), 20.0,
                                                     rng.uniform(50.0, 150.0), rng.uniform(151.0, 400.0),
                                                     rng.uniform(100.0, 400.0), rng.uniform(401.0, 3000.0),
                                                     rng.uniform(3001.0, 60000.0), 0.0, 0.0, 0.0, 0.0, 0.0]
    scenario = {
        "seed": seed,
        "lookup": lookup.hex(),
        "blocks": blocks,
        "soundInfo": [bits(f32(v)) for v in sound_info],
        "queries": make_queries(rng, query_count),
    }

    alignments = [rng.uniform(-1.5, 1.5) for _ in range(200)] + [-1.0, -0.6, -0.2, 0.0, 0.2, 1.0]
    scenario["alignments"] = [bits(f32(v)) for v in alignments]
    sky_cases = []
    for _ in range(300):
        times = sorted(f32(rng.uniform(0.0, 12.0)) for _ in range(4))
        times = [times[3], times[2], times[1], times[0]]  # dayFull, duskEnd, duskStart, nightFull
        if rng.random() < 0.5:
            times = [8.25, 7.5, 7.0, 4.5]
        sky_cases.append({"times": [bits(f32(t)) for t in times], "time": bits(f32(rng.uniform(0.0, 24.0)))})
    scenario["sky"] = sky_cases
    bank_steps = []
    for _ in range(300):
        def level():
            return rng.choice([0.0, 1.0, 0.1, 0.8, rng.uniform(0.0, 1.0), rng.uniform(-0.2, 1.2)])
        bank_steps.append({"targets": [bits(f32(level())) for _ in range(14)],
                           "current": [bits(f32(level())) for _ in range(14)]})
    scenario["bankSteps"] = bank_steps

    emu = Emulator(EXE)
    emu.stub(GET_CAMERA, lambda e: e.u32(e.ecx() + 0x2502C0))
    emu.stub(DEBUG_SET_MESSAGE, lambda e: 0)
    scenario["expected"] = {
        "soundMap": run_sound_map(emu, scenario),
        "alignments": run_alignment(emu, [struct.unpack("<f", struct.pack("<I", b))[0] for b in scenario["alignments"]]),
        "sky": run_sky(emu, sky_cases),
        "bankSteps": run_banks(emu, bank_steps),
    }
    with open(out, "w", newline="\n") as f:
        json.dump(scenario, f, separators=(",", ":"))
    print(f"seed {seed}: {len(scenario['queries'])} sound map queries")


if __name__ == "__main__":
    main()

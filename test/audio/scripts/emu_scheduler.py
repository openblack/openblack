"""Runs LHAudioDLL's atmosphere scheduler (BW1W120 LHaudiodllR.dll) on a synthetic scenario under Unicorn and
records the voice trace, to check openblack's AtmosPlayer against.

Real code: fn_10001000 (manager), fn_10001840 (create), fn_10001610 (bank registration), fn_100011B0 (queue insert),
fn_10001170 (pop), LHAtmosProcess, LHAtmosSetBankVolume, LHAtmosSetGroup, LHAtmosGetBankVolume, the
LH_SamplePlayOptions constructor. Stubbed: CRT rand/srand/time/new/delete/sprintf and the sample functions that
talk to QMixer (LHSamplePlay, LHSampleSetVolume, LHSampleIsPlaying, LHSampleStop, sample slot range), which
reproduce the parts of LHSamplePlay the scheduler can observe: slot fields, header overrides and the pitch
deviation's random draw.

Regenerate a scenario (Python 3 with unicorn installed):
    python emu_scheduler.py <seed> <turns> ../scenarios/scheduler_<seed>.json
"""
import json
import os
import random
import struct
import sys

from emu_common import Emulator, MsvcRand

# Version 1.20 binaries, the executable decrypted
DLL = os.environ.get("BW1_AUDIO_DLL", "LHaudiodllR.dll")

CREATE_MANAGER = 0x10001000
CREATE = 0x10001840
REGISTER = 0x10001610
PROCESS = 0x100018B0
SET_BANK_VOLUME = 0x10001FC0
SET_GROUP = 0x10002060

TIME = 0x1001E80D
SRAND = 0x1001E7DE
RAND = 0x1001E7EB
NEW = 0x1001E3DB
DELETE = 0x1001E3D0
SPRINTF = 0x1001EB70
SAMPLE_PLAY = 0x100113B0
SAMPLE_SET_VOLUME = 0x10013400
SAMPLE_IS_PLAYING = 0x10014070
SAMPLE_STOP = 0x10012DF0
SAMPLE_FIRST = 0x10014130
SAMPLE_LAST = 0x10014150

SLOT_SIZE = 0x90
SLOT_COUNT = 40000
HEADER_SIZE = 0x280


def f32_bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def make_header(index, rng):
    h = bytearray(HEADER_SIZE)
    sample_id = index + 1
    if rng.random() < 0.05:
        sample_id = 0
    struct.pack_into("<i", h, 0x104, sample_id)
    struct.pack_into("<h", h, 0x118, rng.randint(0, 3))
    struct.pack_into("<H", h, 0x11A, rng.choice([0, 0, 1, 2]))
    struct.pack_into("<I", h, 0x128, rng.choice([22050, 11025, 44100]))
    flags = 0
    for bit in (0x1, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400):
        if rng.random() < 0.4:
            flags |= bit
    struct.pack_into("<I", h, 0x244, flags)
    struct.pack_into("<i", h, 0x248, rng.choice([-1, 0, 0, 2, 3, 6]))
    struct.pack_into("<H", h, 0x25C, rng.randint(0, 140))
    struct.pack_into("<I", h, 0x260, rng.choice([0, 15, 30, 40, 80, 100, 160]))
    struct.pack_into("<I", h, 0x264, rng.choice([0, 0, 5, 15, 30]))
    struct.pack_into("<f", h, 0x268, rng.choice([1.0, 300.0, 2.5]))
    struct.pack_into("<f", h, 0x26C, rng.choice([9999.0, 1500.0, 40.0]))
    struct.pack_into("<f", h, 0x270, rng.choice([0.3, 1.0, 2.0, 3.0, 4.0]))
    struct.pack_into("<H", h, 0x274, rng.choice([0, 1, 2, 3]))
    struct.pack_into("<I", h, 0x278, 1)
    interval = rng.choice([-1, 0] + [rng.randint(1, 160) for _ in range(6)])
    struct.pack_into("<i", h, 0x27C, interval)
    return bytes(h)


def make_scenario(seed, turns):
    rng = random.Random(seed)
    banks = []
    for b in range(rng.randint(4, 8)):
        headers = [make_header(i, rng) for i in range(rng.randint(1, 30))]
        atmos_count = 0 if rng.random() < 0.1 else len(headers)
        banks.append({"name": f"bank{b}.sad", "atmosCount": atmos_count, "headers": [h.hex() for h in headers]})
    script = []
    volumes = [rng.randint(0, 127) for _ in banks]
    group = 1
    paused = False
    for turn in range(turns):
        if rng.random() < 0.01:
            group = 3 - group
        if rng.random() < 0.005:
            paused = not paused
        for b in range(len(banks)):
            r = rng.random()
            if r < 0.01:
                volumes[b] = 0
            elif r < 0.02:
                volumes[b] = rng.choice([-5, 140, 127])
            else:
                volumes[b] = max(-3, min(130, volumes[b] + rng.randint(-6, 6)))
        script.append({"group": group, "volumes": list(volumes), "active": not paused})
    return {"seed": seed, "time": 1000000 + seed, "banks": banks, "script": script}


def duration(bank_index, sample_id):
    """Turns a one-shot plays for, shared with the C++ test."""
    return 1 + ((sample_id * 7 + bank_index * 3) % 13)


def run(scenario):
    emu = Emulator(DLL)
    rand = MsvcRand()
    clock = scenario["time"]
    trace = []
    state = {"turn": 0, "next_voice": 1}

    system = emu.alloc(0x100)
    slots = emu.alloc(SLOT_SIZE * SLOT_COUNT)
    emu.w32(system + 0x4, 1)
    emu.w32(system + 0x8, 1)
    emu.w32(system + 0xC, 1)
    emu.w32(system + 0x10, 0)
    emu.w32(system + 0x14, 1)
    # Samples get a fresh slot each and slots are never reused: which slot a sample lands in is resource handling,
    # the port only has to reproduce what is heard
    emu.w32(system + 0x50, slots)
    emu.w32(system + 0x54, slots - SLOT_SIZE)
    next_slot = [slots]
    slot_info = {}
    bank_index = {}
    play_seeded = [False]

    emu.stub(TIME, lambda e: clock)
    emu.stub(SRAND, lambda e: rand.seed(e.arg(0)))
    emu.stub(RAND, lambda e: rand.next())
    emu.stub(NEW, lambda e: e.alloc(e.arg(0)))
    emu.stub(DELETE, lambda e: 0)
    emu.stub(SPRINTF, lambda e: 0)
    emu.stub(SAMPLE_FIRST, lambda e: slots)
    emu.stub(SAMPLE_LAST, lambda e: e.u32(system + 0x54))

    def is_active(slot):
        if emu.u32(slot + 0x8C) == 0:
            return False
        info = slot_info[slot]
        return info["loop"] or state["turn"] < info["end"]

    def sample_play(e):
        if not play_seeded[0]:
            rand.seed(clock)
            play_seeded[0] = True
        options = e.arg(0)
        atmos = e.u32(options + 0x0)
        bank = e.u32(options + 0x4)
        is_3d = e.u32(options + 0x8)
        opt_flags = e.u32(options + 0x1C)
        number = e.u32(options + 0x24)
        header = e.u32(bank + 0x14) + (number - 1) * HEADER_SIZE
        mask = e.u32(header + 0x244)
        sample_id = e.i32(header + 0x104)

        def pick(bit, header_offset, option_offset, signed=False):
            if mask & bit and not opt_flags & bit:
                return e.i32(header + header_offset) if signed else e.u32(header + header_offset)
            return e.i32(options + option_offset) if signed else e.u32(options + option_offset)

        # fn_10011020: instances already playing, by sample or by voice group (header 0x118)
        play_type = e.u32(header + 0x274) & 0xFFFF if mask & 0x400 and not opt_flags & 0x400 else e.u32(options + 0x50)
        group = e.u32(header + 0x118) & 0xFFFF

        def find_playing(same_sample, same_group):
            for s in range(slots, next_slot[0], SLOT_SIZE):
                if e.u32(s + 0x4) != bank or not is_active(s):
                    continue
                if (same_sample and slot_info[s]["sample"] == sample_id) or                         (same_group and group != 0 and slot_info[s]["group"] == group):
                    return s
            return None

        if play_type == 2:
            playing = find_playing(True, True)
            if playing is not None:
                return playing
        elif play_type == 3:
            playing = find_playing(True, False) or find_playing(False, True)
            if playing is not None:
                trace.append({"turn": state["turn"], "event": "stop", "voice": slot_info[playing]["voice"]})
                e.w32(playing + 0x8C, 0)

        # fn_10011420
        volume = e.u16(header + 0x25C) if mask & 0x20 and not opt_flags & 0x20 else e.u32(options + 0x28)
        volume = min(volume, 127)
        loop_count = pick(0x40, 0x248, 0x4C, signed=True)
        min_distance = pick(0x80, 0x268, 0x54)
        max_distance = pick(0x100, 0x26C, 0x58)
        scale = pick(0x200, 0x270, 0x5C)
        pitch = pick(0x1, 0x260, 0x48)
        if pitch == 0:
            pitch = 100
        deviation = ((pitch * e.u32(header + 0x264)) & 0xFFFFFFFF) // 100
        pitch = (pitch - deviation + ((rand.next() * deviation * 2) & 0xFFFFFFFF) // 0x7FFF) & 0xFFFFFFFF
        if pitch == 0:
            pitch = 100

        slot = next_slot[0]
        next_slot[0] += SLOT_SIZE
        e.w32(system + 0x54, slot)
        e.w32(slot + 0x0, atmos)
        e.w32(slot + 0x4, bank)
        e.w32(slot + 0x38, volume)
        e.w32(slot + 0x8C, 1)
        voice = state["next_voice"]
        state["next_voice"] += 1
        slot_info[slot] = {
            "voice": voice,
            "loop": loop_count < 0,
            "end": state["turn"] + duration(bank_index[bank], sample_id),
            "sample": sample_id,
            "group": group,
        }
        trace.append({
            "turn": state["turn"], "event": "play", "voice": voice, "bank": bank_index[bank], "sample": sample_id,
            "volume": volume, "pitch": pitch, "loop": loop_count, "positional": bool(is_3d),
            "x": e.u32(options + 0x30), "y": e.u32(options + 0x34), "z": e.u32(options + 0x38),
            "min": min_distance, "max": max_distance, "scale": scale,
        })
        return slot

    def sample_set_volume(e):
        slot = e.arg(0)
        volume = struct.unpack("<i", struct.pack("<I", e.arg(1)))[0]
        volume = max(0, min(127, volume))
        if slot == 0 or not is_active(slot) or e.u32(slot + 0x38) == volume:
            return 0
        e.w32(slot + 0x38, volume)
        trace.append({"turn": state["turn"], "event": "volume", "voice": slot_info[slot]["voice"], "volume": volume})
        return slot

    def sample_is_playing(e):
        slot = e.arg(0)
        return 1 if slot and is_active(slot) else 0

    def sample_stop(e):
        slot = e.arg(0)
        if slot and is_active(slot):
            trace.append({"turn": state["turn"], "event": "stop", "voice": slot_info[slot]["voice"]})
        if slot:
            e.w32(slot + 0x8C, 0)
        return slot

    emu.stub(SAMPLE_PLAY, sample_play, 4)
    emu.stub(SAMPLE_SET_VOLUME, sample_set_volume, 8)
    emu.stub(SAMPLE_IS_PLAYING, sample_is_playing, 4)
    emu.stub(SAMPLE_STOP, sample_stop, 4)

    emu.call(CREATE_MANAGER)
    emu.call(CREATE, ecx=system)

    bank_objects = []
    for index, bank in enumerate(scenario["banks"]):
        headers = b"".join(bytes.fromhex(h) for h in bank["headers"])
        bank_object = emu.alloc(0x140)
        header_array = emu.alloc(len(headers))
        emu.write(header_array, headers)
        emu.w32(bank_object + 0x8, len(bank["headers"]))
        emu.w32(bank_object + 0xC, bank["atmosCount"])
        emu.w32(bank_object + 0x14, header_array)
        bank_index[bank_object] = index
        bank_objects.append(bank_object)
        if bank["atmosCount"]:
            emu.call(REGISTER, [bank_object], ecx=system)

    for turn, step in enumerate(scenario["script"]):
        state["turn"] = turn
        for index, bank_object in enumerate(bank_objects):
            emu.call(SET_GROUP, [bank_object, step["group"]], ecx=system)
            emu.call(SET_BANK_VOLUME, [bank_object, step["volumes"][index] & 0xFFFFFFFF], ecx=system)
        emu.call(PROCESS, [1 if step["active"] else 0], ecx=system)
    return trace


def main():
    seed = int(sys.argv[1])
    turns = int(sys.argv[2])
    out = sys.argv[3]
    scenario = make_scenario(seed, turns)
    trace = run(scenario)
    scenario["expected"] = trace
    with open(out, "w", newline="\n") as f:
        json.dump(scenario, f, separators=(",", ":"))
    plays = sum(1 for t in trace if t["event"] == "play")
    print(f"seed {seed}: {len(trace)} events, {plays} plays")


if __name__ == "__main__":
    main()

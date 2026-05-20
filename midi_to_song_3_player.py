#!/usr/bin/env python3
"""
midi_to_3motors.py – Convert MIDI to 3‑motor Step arrays and auto‑patch songs.h
Usage: python midi_to_3motors.py <file.mid> [--raw] ["Song Name"]
"""

import mido
import os
import re
import sys
import math
from collections import defaultdict

# ===== CONFIG =====
TIME_SLICE_MS = 10
BPM_FALLBACK = 120
USE_RAW_DURATION = False
SRC_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "src")
SONGS_DIR = os.path.join(SRC_DIR, "songs")
SONGS_H = os.path.join(SRC_DIR, "songs.h")

DURATION_MACROS = {
    "MS_64": 47,
    "MS_32": 94,
    "MS_16": 187,
    "MS_Q": 375,
    "MS_DQ": 562,
    "MS_H": 750,
    "MS_DH": 1125,
    "MS_W": 1500,
}

NOTE_NAMES = ['C', 'CS', 'D', 'DS', 'E', 'F', 'FS', 'G', 'GS', 'A', 'AS', 'B']

def midi_note_to_name(note):
    if note == 0:
        return "NOTE_REST"
    octave = (note // 12) - 1
    name = NOTE_NAMES[note % 12]
    return f"NOTE_{name}{octave}"

def ms_to_duration_macro(ms):
    best_name = "MS_Q"
    best_diff = float('inf')
    for name, val in DURATION_MACROS.items():
        diff = abs(ms - val)
        if diff < best_diff:
            best_diff = diff
            best_name = name
    return best_name, DURATION_MACROS[best_name]

# ----------------------------------------------------------------------
# MIDI parsing (same as before)
# ----------------------------------------------------------------------
def parse_midi_events(mid_path):
    mid = mido.MidiFile(mid_path)
    ticks_per_beat = mid.ticks_per_beat
    tempo = mido.bpm2tempo(BPM_FALLBACK)

    all_events = []
    for track in mid.tracks:
        abs_tick = 0
        for msg in track:
            abs_tick += msg.time
            all_events.append((abs_tick, msg))
    all_events.sort(key=lambda x: x[0])

    tick_to_sec = {}
    current_tempo = tempo
    last_tick = 0
    last_sec = 0.0
    for tick, msg in all_events:
        delta_sec = mido.tick2second(tick - last_tick, ticks_per_beat, current_tempo)
        last_sec += delta_sec
        last_tick = tick
        tick_to_sec[tick] = last_sec
        if msg.is_meta and msg.type == 'set_tempo':
            current_tempo = msg.tempo

    active = {}
    notes = []
    for tick, msg in all_events:
        if msg.is_meta:
            continue
        t = tick_to_sec.get(tick, 0.0)
        if msg.type == 'note_on' and msg.velocity > 0:
            active[msg.note] = t
        elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
            if msg.note in active:
                start = active.pop(msg.note)
                dur = t - start
                if dur > 0.01:
                    notes.append((start, start + dur, msg.note))
    return notes

# ----------------------------------------------------------------------
# Voice selection
# ----------------------------------------------------------------------
class MelodyTracker:
    def __init__(self):
        self.prev_note = None
    def select(self, notes):
        if not notes:
            return 0
        notes = sorted(notes)
        if self.prev_note is None:
            self.prev_note = notes[-1]
            return self.prev_note
        best = min(notes, key=lambda n: abs(n - self.prev_note))
        self.prev_note = best
        return best

def select_bass(notes):
    return min(notes) if notes else 0

def select_harmony(notes, melody_note, bass_note):
    candidates = [n for n in notes if n not in (melody_note, bass_note)]
    if not candidates:
        return 0
    return max(candidates)

def convert_midi_to_3motors(midi_path, use_raw_duration=False):
    print("Parsing MIDI...")
    notes = parse_midi_events(midi_path)
    if not notes:
        print("Error: No note events found.")
        sys.exit(1)
    print(f"  {len(notes)} note events")

    melody_tracker = MelodyTracker()

    def combined_selector(active_set):
        if not active_set:
            return (0, 0, 0)
        notes_list = sorted(active_set)
        bass = notes_list[0]
        melody = melody_tracker.select(notes_list)
        harmony = select_harmony(notes_list, melody, bass)
        return (melody, harmony, bass)

    events = []
    for start, end, note in notes:
        events.append((start, note, 1))
        events.append((end, note, -1))
    events.sort(key=lambda x: x[0])

    active_notes = set()
    prev_time = events[0][0]
    melody_seq, harmony_seq, bass_seq = [], [], []

    for time, note, typ in events:
        if time > prev_time + 0.001:
            melody, harmony, bass = combined_selector(active_notes)
            duration_sec = time - prev_time
            if duration_sec > 0.01:
                duration_ms = duration_sec * 1000
                if use_raw_duration:
                    melody_seq.append((melody, int(duration_ms)))
                    harmony_seq.append((harmony, int(duration_ms)))
                    bass_seq.append((bass, int(duration_ms)))
                else:
                    macro, _ = ms_to_duration_macro(duration_ms)
                    melody_seq.append((melody, macro))
                    harmony_seq.append((harmony, macro))
                    bass_seq.append((bass, macro))
        if typ == 1:
            active_notes.add(note)
        else:
            active_notes.discard(note)
        prev_time = time

    def merge_seq(seq):
        if not seq:
            return []
        merged = []
        for note, dur in seq:
            if merged and merged[-1][0] == note:
                if use_raw_duration:
                    merged[-1] = (note, merged[-1][1] + dur)
                else:
                    merged.append((note, dur))
            else:
                merged.append((note, dur))
        return merged

    return merge_seq(melody_seq), merge_seq(harmony_seq), merge_seq(bass_seq)

# ----------------------------------------------------------------------
# Export header file
# ----------------------------------------------------------------------
def export_header(melody, harmony, bass, song_name, var_prefix, out_path, use_raw_duration):
    lines = [
        f"// Auto-generated by midi_to_3motors.py",
        f"// Song: {song_name}",
        f"#pragma once",
        f'#include "../notes.h"',
        f'#include "../player.h"',
        f"",
    ]
    if use_raw_duration:
        lines.append("// Raw duration in milliseconds")
    else:
        lines.append("// Duration uses standard macros")

    for name, seq in [("melody", melody), ("harmony", harmony), ("bass", bass)]:
        lines.append(f"// {name.upper()}")
        lines.append(f"static const Step {var_prefix}_{name}[] = {{")
        for note, dur in seq:
            if note == 0:
                note_macro = "NOTE_REST"
            else:
                note_macro = midi_note_to_name(note)
            lines.append(f"    {{{note_macro}, {dur}}},")
        lines.append("};")
        lines.append("")

    lines.append(f"#define {var_prefix.upper()}_MELODY_LEN (sizeof({var_prefix}_melody) / sizeof(Step))")
    lines.append(f"#define {var_prefix.upper()}_HARMONY_LEN (sizeof({var_prefix}_harmony) / sizeof(Step))")
    lines.append(f"#define {var_prefix.upper()}_BASS_LEN (sizeof({var_prefix}_bass) / sizeof(Step))")

    with open(out_path, "w") as f:
        f.write("\n".join(lines))
    print(f"Exported: {out_path}")

# ----------------------------------------------------------------------
# Patch songs.h – add #include and Song3 entry
# ----------------------------------------------------------------------
def patch_songs_h(var_prefix, song_name, h_filename):
    if not os.path.exists(SONGS_H):
        print(f"Warning: {SONGS_H} not found, skipping patch.")
        return

    with open(SONGS_H, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Add #include if missing
    include_line = f'#include "songs/{h_filename}"'
    if include_line not in content:
        # Insert after last existing #include "songs/..."
        include_pattern = r'^#include "songs/.*\.h"'
        matches = list(re.finditer(include_pattern, content, re.MULTILINE))
        if matches:
            pos = matches[-1].end()
            content = content[:pos] + "\n" + include_line + content[pos:]
        else:
            # No includes yet, insert after any initial comments/guards? Simpler: insert after #define guards
            guard_match = re.search(r'#define SONGS_H', content)
            if guard_match:
                pos = guard_match.end()
                content = content[:pos] + "\n\n" + include_line + content[pos:]
            else:
                content = include_line + "\n" + content
        print(f"  Added include: {include_line}")
    else:
        print(f"  Include already present: {include_line}")

    # 2. Generate song entry
    entry_lines = [
        "    {",
        f'        .name = "{song_name}",',
        f'        .melody = {var_prefix}_melody,',
        f'        .melody_len = {var_prefix.upper()}_MELODY_LEN,',
        f'        .harmony = {var_prefix}_harmony,',
        f'        .harmony_len = {var_prefix.upper()}_HARMONY_LEN,',
        f'        .bass = {var_prefix}_bass,',
        f'        .bass_len = {var_prefix.upper()}_BASS_LEN,',
        "    },"
    ]
    entry = "\n".join(entry_lines)

    # Check if already in song_list
    if var_prefix in content:
        print(f"  Song entry for {var_prefix} already exists, skipping.")
        return

    # Find song_list array
    array_pattern = r'(static const Song3 song_list\[\]\s*=\s*\{)(.*?)(\};)'
    match = re.search(array_pattern, content, re.DOTALL)
    if not match:
        print(f"Warning: Could not find 'static const Song3 song_list[]' in {SONGS_H}")
        print("Please add the entry manually:\n", entry)
        return

    before = content[:match.start(2)+1]  # up to the opening brace content
    inside = match.group(2)
    after = content[match.start(3):]     # "};"

    # Insert new entry before the last '};'
    # Find the last '}' inside the array (the closing brace of the array content)
    # We'll just prepend the new entry at the end of the existing list, adding a comma to the previous entry if needed.
    # Simpler: remove trailing whitespace and add entry before the closing '};'
    # But we must ensure the previous entry has a comma.
    # We'll locate the position of the last '}' that belongs to the array (not the outer).
    # A safe way: replace the content inside the braces.
    inside_stripped = inside.rstrip()
    if inside_stripped and not inside_stripped.endswith(','):
        inside_stripped += ','
    new_inside = inside_stripped + "\n" + entry + "\n"
    new_content = before + new_inside + after

    with open(SONGS_H, "w", encoding="utf-8") as f:
        f.write(new_content)
    print(f"  Added song entry: {song_name}")

# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert MIDI to 3-motor Step arrays and patch songs.h")
    parser.add_argument("midi_file", help="Input .mid file")
    parser.add_argument("song_name", nargs="?", default=None, help="Display name for song")
    parser.add_argument("--raw", action="store_true", help="Output raw duration in ms instead of macros")
    args = parser.parse_args()

    midi_path = args.midi_file
    if not os.path.exists(midi_path):
        print(f"Error: {midi_path} not found")
        sys.exit(1)

    midi_basename = os.path.splitext(os.path.basename(midi_path))[0]
    song_name = args.song_name if args.song_name else midi_basename.replace("-", " ").replace("_", " ").title()
    var_prefix = "song_" + re.sub(r'[^a-z0-9]', '_', midi_basename.lower()).strip('_')
    h_filename = f"{midi_basename}.h"
    out_path = os.path.join(SONGS_DIR, h_filename)
    os.makedirs(SONGS_DIR, exist_ok=True)

    print(f"Song: {song_name}")
    print(f"Var prefix: {var_prefix}")
    print(f"Raw duration: {args.raw}")

    melody, harmony, bass = convert_midi_to_3motors(midi_path, use_raw_duration=args.raw)
    print(f"Melody steps: {len(melody)}, Harmony: {len(harmony)}, Bass: {len(bass)}")

    export_header(melody, harmony, bass, song_name, var_prefix, out_path, args.raw)
    patch_songs_h(var_prefix, song_name, h_filename)

    print("\nDone. The new song is ready and has been added to songs.h.")
    print("Press the user button (PD0) to cycle through songs.")
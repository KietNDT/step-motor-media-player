#!/usr/bin/env python3
"""
midi_to_3motors.py v2 – improved voice separation for STM32 stepper music
Usage: python midi_to_3motors.py <file.mid> [--raw] ["Song Name"]
"""

import mido
import os
import re
import sys
import math
from collections import defaultdict

# ===== CONFIG =====
TIME_SLICE_MS = 10          # 10ms for better timing (was 20)
BPM_FALLBACK = 120
USE_RAW_DURATION = False    # set True to output raw ms, False to use macros
SRC_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "src")
SONGS_DIR = os.path.join(SRC_DIR, "songs")

# Duration macros (only used if USE_RAW_DURATION == False)
DURATION_MACROS = {
    "MS_32": 94,   # 32nd note
    "MS_16": 187,
    "MS_8":  375,  # eighth
    "MS_Q":  375,  # quarter (same as eighth for simplicity, adjust as needed)
    "MS_H":  750,
    "MS_W": 1500,
}
# Better to map to your exact macros from project summary:
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
# MIDI parsing – same as before, returns list of (start_sec, end_sec, note)
# ----------------------------------------------------------------------
def parse_midi_events(mid_path):
    mid = mido.MidiFile(mid_path)
    ticks_per_beat = mid.ticks_per_beat
    tempo = mido.bpm2tempo(BPM_FALLBACK)
    
    # Merge all tracks into a single list with absolute ticks
    all_events = []
    for track in mid.tracks:
        abs_tick = 0
        for msg in track:
            abs_tick += msg.time
            all_events.append((abs_tick, msg))
    
    # Sort by absolute tick
    all_events.sort(key=lambda x: x[0])
    
    # Build tick -> time mapping with tempo changes
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
    
    # Extract note on/off events
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
                if dur > 0.01:  # ignore very short notes
                    notes.append((start, start + dur, msg.note))
    return notes


# ----------------------------------------------------------------------
# Improved voice selectors
# ----------------------------------------------------------------------
class MelodyTracker:
    def __init__(self):
        self.prev_note = None
    def select(self, notes):
        if not notes:
            return 0
        notes = sorted(notes)
        if self.prev_note is None:
            self.prev_note = notes[-1]   # start with highest
            return self.prev_note
        # Choose note closest to previous melody
        best = min(notes, key=lambda n: abs(n - self.prev_note))
        self.prev_note = best
        return best

def select_bass(notes):
    return min(notes) if notes else 0

def select_harmony(notes, melody_note, bass_note):
    """Harmony = highest remaining note that is not melody or bass"""
    candidates = [n for n in notes if n not in (melody_note, bass_note)]
    if not candidates:
        return 0
    # Option: pick highest (often sounds better than median)
    return max(candidates)

# ----------------------------------------------------------------------
# Build sequence for a voice using exact note segments (no slicing)
# This is more accurate than 10ms slices.
# ----------------------------------------------------------------------
def build_voice_sequence_exact(notes, voice_selector_func, use_raw_duration):
    """
    notes: list of (start, end, midi_note) from MIDI
    voice_selector_func: function that takes a list of simultaneous notes and returns the selected note for this voice.
    Returns list of (note, duration_ms or macro_name)
    """
    # First, create timeline of note changes using event merging
    # Collect all start/end events
    events = []  # (time_sec, note, type: +1 for start, -1 for end)
    for start, end, note in notes:
        events.append((start, note, 1))
        events.append((end, note, -1))
    events.sort(key=lambda x: x[0])
    
    # Sweep through events to know which notes are active at any interval
    active_notes = set()
    prev_time = events[0][0]
    sequence = []
    
    for time, note, typ in events:
        if time > prev_time + 0.001:  # ignore micro-gaps
            # Determine selected note for the interval [prev_time, time)
            selected = voice_selector_func(active_notes) if active_notes else 0
            duration_sec = time - prev_time
            if duration_sec > 0.01:  # ignore very short gaps
                duration_ms = duration_sec * 1000
                if use_raw_duration:
                    sequence.append((selected, int(duration_ms)))
                else:
                    macro_name, _ = ms_to_duration_macro(duration_ms)
                    sequence.append((selected, macro_name))
        # Update active notes
        if typ == 1:
            active_notes.add(note)
        else:
            active_notes.discard(note)
        prev_time = time
    
    # Merge consecutive same notes
    merged = []
    for note, dur in sequence:
        if merged and merged[-1][0] == note:
            # combine durations
            if use_raw_duration:
                merged[-1] = (note, merged[-1][1] + dur)
            else:
                # For macros, cannot simply add; we need to recalc total ms
                # Simpler: keep as separate steps; merging macros is messy
                merged.append((note, dur))
        else:
            merged.append((note, dur))
    return merged

# ----------------------------------------------------------------------
# Main conversion
# ----------------------------------------------------------------------
def convert_midi_to_3motors(midi_path, use_raw_duration=False):
    print("Parsing MIDI...")
    notes = parse_midi_events(midi_path)
    # After parse_midi_events, before building timeline:
    if not notes:
        print("Error: No note events found in MIDI file.")
        sys.exit(1)

    print(f"  {len(notes)} note events")
    
    # Create voice selectors
    melody_tracker = MelodyTracker()
    
    # Helper that selects melody using tracker, and passes melody/bass to harmony
    # We need a combined approach: for each time interval, we need all three voices simultaneously
    # The exact event method above calls voice_selector_func per interval.
    # We'll create a combined selector that returns (melody, harmony, bass) for each interval.
    def combined_selector(active_set):
        if not active_set:
            return (0, 0, 0)
        notes_list = sorted(active_set)
        bass = notes_list[0]
        melody = melody_tracker.select(notes_list)
        harmony = select_harmony(notes_list, melody, bass)
        return (melody, harmony, bass)
    
    # Build timeline of (time, (m,h,b)) using exact events
    events = []
    for start, end, note in notes:
        events.append((start, note, 1))
        events.append((end, note, -1))
    events.sort(key=lambda x: x[0])
    
    active_notes = set()
    prev_time = events[0][0]
    melody_seq = []
    harmony_seq = []
    bass_seq = []
    
    for time, note, typ in events:
        if time > prev_time + 0.001:
            # Get selection for interval
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
    
    # Merge consecutive same notes for each voice
    def merge_seq(seq):
        if not seq:
            return []
        merged = []
        for note, dur in seq:
            if merged and merged[-1][0] == note:
                if use_raw_duration:
                    merged[-1] = (note, merged[-1][1] + dur)
                else:
                    merged.append((note, dur))  # keep separate for macros
            else:
                merged.append((note, dur))
        return merged
    
    melody_seq = merge_seq(melody_seq)
    harmony_seq = merge_seq(harmony_seq)
    bass_seq = merge_seq(bass_seq)
    
    return melody_seq, harmony_seq, bass_seq

# ----------------------------------------------------------------------
# Export to C header
# ----------------------------------------------------------------------
def export_header(melody, harmony, bass, song_name, var_prefix, out_path, use_raw_duration):
    lines = [
        f"// Auto-generated by midi_to_3motors.py v2",
        f"// Song: {song_name}",
        f"#pragma once",
        f'#include "../notes.h"',
        f'#include "../player.h"   // for Step typedef',
        f"",
    ]
    if use_raw_duration:
        lines.append("// Note: duration is in milliseconds (raw)")
        lines.append("// You need to modify Step struct to have uint16_t dur_ms")
    else:
        lines.append("// Duration uses standard macros (MS_Q, MS_H, etc.)")
    
    for name, seq in [("melody", melody), ("harmony", harmony), ("bass", bass)]:
        lines.append(f"// {name.upper()}")
        lines.append(f"static const Step {var_prefix}_{name}[] = {{")
        for note, dur in seq:
            if note == 0:
                note_macro = "NOTE_REST"
            else:
                note_macro = midi_note_to_name(note)
            if use_raw_duration:
                lines.append(f"    {{{note_macro}, {dur}}},")
            else:
                lines.append(f"    {{{note_macro}, {dur}}},")
        lines.append("};")
        lines.append("")
    
    # Optional length defines
    lines.append(f"#define {var_prefix.upper()}_MELODY_LEN (sizeof({var_prefix}_melody) / sizeof(Step))")
    lines.append(f"#define {var_prefix.upper()}_HARMONY_LEN (sizeof({var_prefix}_harmony) / sizeof(Step))")
    lines.append(f"#define {var_prefix.upper()}_BASS_LEN (sizeof({var_prefix}_bass) / sizeof(Step))")
    
    with open(out_path, "w") as f:
        f.write("\n".join(lines))
    print(f"Exported: {out_path}")

# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert MIDI to 3-motor Step arrays")
    parser.add_argument("midi_file", help="Input .mid file")
    parser.add_argument("song_name", nargs="?", default=None, help="Display name for song")
    parser.add_argument("--raw", action="store_true", help="Output raw duration in ms instead of macros")
    parser.add_argument("--slice-ms", type=int, default=10, help="Time slice resolution (ms) – not used in exact mode, kept for compatibility")
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
    print("\nDone. Include this file and use the three arrays.")
    print(f"Example:\n  #include \"songs/{h_filename}\"\n  player_play_3ch({var_prefix}_melody, {var_prefix}_harmony, {var_prefix}_bass);")
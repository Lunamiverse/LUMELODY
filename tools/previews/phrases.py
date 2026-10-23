"""Original demo phrases used to audition presets.

Every phrase is written for this project (no transcription of existing songs).
Times are in beats; render() converts to seconds with the phrase tempo.
"""
import random

def chord(t, d, notes, vel=0.7, roll=0.0, rng=None):
    out = []
    for i, n in enumerate(notes):
        v = vel + (rng.uniform(-0.05, 0.05) if rng else 0)
        out.append((t + i * roll, d - i * roll, n, max(0.05, min(1.0, v))))
    return out

def seq(items, vel=0.75, rng=None, legato=1.0):
    """items: (start_beat, dur_beat, note[, vel])"""
    out = []
    for it in items:
        s, d, n = it[0], it[1], it[2]
        v = it[3] if len(it) > 3 else vel
        if rng: v += rng.uniform(-0.06, 0.06)
        out.append((s, d * legato, n, max(0.05, min(1.0, v))))
    return out

def P(bpm, notes, desc):
    return {"bpm": bpm, "notes": notes, "desc": desc}

def build():
    r = random.Random(7)
    ph = {}

    # --- R&B keys: Ebmaj9 - Cm9 - Abmaj9 - Bb13sus -> Ebmaj9, with a top-line answer
    n = []
    n += chord(0, 2, [51, 58, 62, 65, 67], .62, .012, r)
    n += chord(2, 2, [48, 55, 58, 62, 63], .58, .012, r)
    n += chord(4, 2, [44, 51, 55, 58, 60], .60, .012, r)
    n += chord(6, 2, [46, 53, 56, 60, 63], .57, .012, r)
    n += chord(8, 3, [51, 58, 62, 65, 70], .55, .02, r)
    n += seq([(.5, .5, 79), (1, .5, 77), (1.5, 1, 75), (2.75, .25, 74), (3, 1, 75),
              (4.5, .5, 72), (5, .5, 74), (5.5, 1, 75), (6.5, .5, 77), (7, 1.5, 79), (8.5, 2.5, 82, .5)], .66, r)
    ph["keys_rnb"] = P(72, n, "R&B progression in Eb (maj9 colours) with a top melody")

    # --- dark trap keys: Cm - Abmaj7 - Fm9 - G7b9
    n = []
    n += chord(0, 4, [48, 55, 60, 63, 67], .66, .015, r)
    n += chord(4, 4, [44, 51, 55, 60, 63], .62, .015, r)
    n += chord(8, 4, [41, 48, 51, 55, 60], .6, .015, r)
    n += chord(12, 4, [43, 50, 53, 56, 59], .64, .015, r)
    n += seq([(1, .5, 75), (1.5, .5, 74), (2, 1.5, 72), (5, .5, 75), (5.5, .5, 79), (6, 2, 77),
              (9, .5, 75), (9.5, .5, 74), (10, 1.5, 72), (13, .5, 71), (13.5, 2.5, 74)], .7, r)
    ph["keys_dark"] = P(140, n, "Dark minor progression (Cm-Ab-Fm-G) at 140, half-time feel")

    # --- bossa / Brazilian-inspired comping: Dm9 - G13 - Cmaj9 - A7b13
    n = []
    comp = [0, 1.5, 3]  # syncopated hits per 4 beats
    voic = [[50, 57, 60, 64, 65], [43, 53, 57, 59, 64], [48, 52, 55, 59, 62], [45, 55, 61, 65, 67]]
    for i, v in enumerate(voic):
        for c in comp:
            n += chord(i * 4 + c, 1.2 if c != 3 else .9, v[1:], .55 + (.08 if c == 0 else 0), .006, r)
        n += seq([(i * 4, 1.8, v[0], .6)], rng=r)
    n += seq([(.5, .5, 77), (1, .5, 76), (2, 1, 74), (4.5, .5, 77), (5, 1, 76), (6.5, 1, 74),
              (8.5, .5, 76), (9, .5, 74), (9.5, 1, 71), (12.5, .5, 73), (13, 2, 72)], .62, r)
    ph["keys_bossa"] = P(118, n, "Bossa-inspired syncopated comping in C with a light melody")

    # --- reggaeton pluck riff over Am - F - C - G (3+3+2 phrasing)
    pos = [0, .75, 1.5, 2, 2.75, 3.5]
    arps = {"Am": [69, 76, 72, 69, 76, 71], "F": [65, 72, 69, 65, 72, 67], "C": [64, 72, 67, 64, 71, 67], "G": [62, 71, 67, 62, 74, 71]}
    n = []
    for b, ch in enumerate(["Am", "F", "C", "G"]):
        for p, note in zip(pos, arps[ch]):
            n += seq([(b * 4 + p, .5, note, .78 if p in (0, 2) else .66)], rng=r)
    ph["pluck_reggaeton"] = P(94, n, "Syncopated 3-3-2 pluck riff, Am-F-C-G at 94 BPM")

    # --- dancehall: playful offbeat riff in D minor
    n = []
    riff = [(0, .5, 74), (.75, .25, 72), (1, .5, 74), (1.5, .5, 77), (2.5, .5, 76), (3, .5, 74), (3.5, .5, 69)]
    riff2 = [(0, .5, 70), (.75, .25, 69), (1, .5, 70), (1.5, .5, 74), (2.5, .5, 72), (3, .5, 70), (3.5, .5, 72)]
    for b, rf in enumerate([riff, riff2, riff, [(0, .5, 72), (.75, .25, 74), (1, .5, 76), (1.5, 1, 77), (3, 1, 74)]]):
        n += seq([(b * 4 + s, d, k) for s, d, k in rf], .74, r)
    ph["pluck_dancehall"] = P(100, n, "Playful offbeat riff in D minor at 100 BPM")

    # --- dembow: bouncy repetitive motif at 124
    n = []
    motif = [(0, .25, 67), (.5, .25, 70), (1, .25, 74), (1.5, .25, 70), (1.75, .25, 72), (2.5, .25, 79), (3, .25, 74), (3.5, .25, 72)]
    for b in range(4):
        shift = [0, 0, -2, 3][b]
        n += seq([(b * 4 + s, d, k + shift, .85 if s in (0, 1, 2.5) else .7) for s, d, k in motif], rng=r)
    ph["pluck_dembow"] = P(124, n, "Bouncy, punchy motif in G minor at 124 BPM")

    # --- afrobeats: pentatonic syncopation over F - Gm - Bb - C
    n = []
    pat = [(0, .5, 72), (.75, .25, 69), (1.25, .5, 72), (2, .25, 74), (2.5, .5, 77), (3.25, .5, 74)]
    for b, sh in enumerate([0, 2, -2, 0]):
        n += seq([(b * 4 + s, d, k + sh, .72) for s, d, k in pat], rng=r)
    n += chord(0, 4, [53, 57, 60], .45, 0, r) + chord(4, 4, [55, 58, 62], .45, 0, r) + chord(8, 4, [58, 62, 65], .45, 0, r) + chord(12, 4, [60, 64, 67], .45, 0, r)
    ph["pluck_afro"] = P(106, n, "Syncopated pentatonic riff with soft chords, F major, 106 BPM")

    # --- trap bell melody (triplet feel) in C minor
    n = seq([(0, .66, 72), (.67, .66, 75), (1.33, .67, 79), (2, 1, 77), (3, .5, 75), (3.5, .5, 74),
             (4, .66, 72), (4.67, .66, 75), (5.33, .67, 80), (6, 1.5, 79), (7.5, .5, 77),
             (8, .66, 72), (8.67, .66, 75), (9.33, .67, 79), (10, 1, 82), (11, .5, 80), (11.5, .5, 79),
             (12, 1, 77), (13, 1, 75), (14, 2, 74)], .72, r)
    ph["bell_trap"] = P(140, n, "Triplet bell melody in C minor at 140")

    # --- glassy reggaeton/latin pop bell line with space for delay
    n = seq([(0, .5, 76), (.75, .5, 79), (1.5, .5, 81), (2.5, 1, 79), (4, .5, 77), (4.75, .5, 76), (5.5, .5, 74), (6.5, 1.5, 76),
             (8, .5, 76), (8.75, .5, 79), (9.5, .5, 84), (10.5, 1, 83), (12, .5, 81), (12.75, .5, 79), (13.5, 2, 81)], .72, r)
    ph["bell_latin"] = P(96, n, "Spacious bell line in A minor, 96 BPM")

    # --- expressive mono lead with slides (overlapping notes = legato/glide)
    n = seq([(0, 1.1, 67), (1, .6, 70), (1.5, 1.6, 72), (3, .6, 75), (3.5, 1.1, 74), (4.5, .6, 72), (5, 2.1, 70),
             (7, .6, 67), (7.5, 1.1, 70), (8.5, .6, 72), (9, 1.6, 77), (10.5, .6, 75), (11, 2.6, 74)], .74, r)
    ph["lead_mono"] = P(92, n, "Legato G-minor line with slides at 92 BPM")

    # --- bright pop hook
    n = seq([(0, .5, 76), (.5, .5, 76), (1, .5, 79), (1.5, 1, 81), (3, .5, 79), (3.5, .5, 76),
             (4, .5, 74), (4.5, .5, 76), (5, .5, 79), (5.5, 1.5, 76), (8, .5, 76), (8.5, .5, 76), (9, .5, 79), (9.5, 1, 84),
             (11, .5, 83), (11.5, .5, 81), (12, .5, 79), (12.5, .5, 81), (13, 2, 79)], .76, r)
    ph["lead_pop"] = P(118, n, "Catchy major-key hook at 118 BPM")

    # --- slow cinematic / ethereal lead
    n = seq([(0, 2.1, 69), (2, 1.1, 72), (3, 1.1, 71), (4, 3.1, 67), (7, 1.1, 64), (8, 2.1, 69), (10, 1.1, 76), (11, 3, 74)], .66, r)
    ph["lead_slow"] = P(80, n, "Slow, open melody for ethereal/cinematic leads")

    # --- pads: Fmaj9 - Am7 - Dm9 - Bbmaj7#11 (wide voicings)
    n = []
    n += chord(0, 4, [41, 53, 57, 60, 64, 67], .62, 0, r)
    n += chord(4, 4, [45, 52, 55, 60, 64, 67], .6, 0, r)
    n += chord(8, 4, [38, 50, 53, 57, 60, 64], .62, 0, r)
    n += chord(12, 4.5, [46, 53, 57, 62, 64, 69], .58, 0, r)
    ph["pad"] = P(76, n, "Four wide chords, 76 BPM, held for the full bar")

    n = []
    n += chord(0, 4, [36, 48, 51, 55, 58], .62, 0, r)
    n += chord(4, 4, [32, 48, 51, 56, 60], .6, 0, r)
    n += chord(8, 4, [29, 44, 48, 51, 56], .62, 0, r)
    n += chord(12, 4.5, [31, 47, 50, 55, 62], .58, 0, r)
    ph["pad_dark"] = P(70, n, "Dark minor pad progression in C, 70 BPM")

    # --- string-like layer: legato voices moving under a melody
    n = []
    n += chord(0, 4, [50, 57, 62, 65], .55, .03, r) + chord(4, 4, [46, 58, 62, 65], .55, .03, r)
    n += chord(8, 4, [48, 55, 60, 64], .6, .03, r) + chord(12, 4.5, [45, 57, 61, 64], .62, .03, r)
    n += seq([(0, 2, 74), (2, 2, 72), (4, 3, 70), (7, 1, 69), (8, 2, 67), (10, 2, 72), (12, 4.5, 73)], .62, r)
    ph["strings"] = P(72, n, "Emotional D-minor string-style voicings with a top line")

    # --- guitar-style pluck pattern (arpeggiated, R&B/reggaeton)
    n = []
    shapes = [[52, 59, 64, 67, 71], [48, 55, 60, 64, 67], [45, 52, 57, 60, 64], [47, 54, 59, 62, 66]]
    for b, sh in enumerate(shapes):
        order = [0, 2, 3, 4, 3, 2, 1, 3]
        for i, o in enumerate(order):
            n += seq([(b * 4 + i * .5, 1.2, sh[o], .7 if i % 2 == 0 else .58)], rng=r)
    ph["guitar_arp"] = P(88, n, "Arpeggiated guitar-style figure, Em-C-Am-Bm, 88 BPM")

    n = []
    pat = [0, .5, .75, 1.5, 2, 2.75, 3, 3.5]
    roots = [57, 55, 53, 52]
    for b, rt in enumerate(roots):
        for i, p in enumerate(pat):
            nn = rt + [0, 7, 12, 7, 0, 12, 10, 7][i]
            n += seq([(b * 4 + p, .22, nn, .75 if i in (0, 4) else .6)], rng=r)
    ph["guitar_muted"] = P(104, n, "Rhythmic muted-guitar style pattern, 104 BPM")

    # --- basses
    n = seq([(0, 1.4, 36), (1.5, .4, 36), (2, 1.1, 36), (3, 1.0, 43), (4, 1.5, 39), (5.5, .5, 39), (6, 2.1, 41),
             (8, 1.4, 36), (9.5, .4, 36), (10, 1.1, 36), (11, 1.1, 46), (12, 1.6, 44), (13.5, 2.5, 43)], .85, r)
    ph["bass_808"] = P(140, n, "808 pattern with slides in C minor, 140 BPM")

    n = []
    for b, rt in enumerate([31, 31, 34, 29]):
        n += seq([(b * 4 + 0, .6, rt), (b * 4 + .75, .4, rt), (b * 4 + 1.5, .45, rt + 12, .7), (b * 4 + 2, .6, rt), (b * 4 + 3, .4, rt + 3, .75), (b * 4 + 3.5, .4, rt + 5, .7)], .85, r)
    ph["bass_dembow"] = P(124, n, "Punchy dembow/reggaeton bass in G minor, 124 BPM")

    n = seq([(0, 1.5, 39), (1.5, .5, 46), (2, 1, 48), (3, .9, 51), (4, 1.5, 36), (5.5, .5, 43), (6, 1, 46), (7, .9, 48),
             (8, 1.5, 44), (9.5, .5, 51), (10, 1, 53), (11, .9, 51), (12, 1.5, 46), (13.5, .5, 48), (14, 2, 51)], .75, r)
    ph["bass_melodic"] = P(76, n, "Melodic R&B bass line in Eb, 76 BPM")

    n = []
    for b, rt in enumerate([33, 33, 29, 31]):
        n += seq([(b * 4 + x, .3, rt + o) for x, o in [(.5, 0), (1.5, 0), (1.75, 12), (2.5, 0), (3.5, 0), (3.75, 10)]], .8, r)
    ph["bass_house"] = P(124, n, "Offbeat plucky bass, 124 BPM")

    n = seq([(0, 1.0, 38), (1.5, .5, 38), (2, .75, 41), (3, 1.0, 43), (4, .75, 38), (5, .5, 36), (5.5, .5, 38), (6, 2, 41),
             (8, 1.0, 38), (9.5, .5, 38), (10, .75, 45), (11, 1.0, 43), (12, 2, 41), (14, 2, 36)], .82, r)
    ph["bass_dancehall"] = P(98, n, "Deep, round dancehall bass line in D minor, 98 BPM")

    # --- textures
    n = seq([(0, 6, 60, .6), (2, 6, 67, .5), (4, 8, 74, .45), (8, 6, 63, .55), (10, 6, 70, .5)], rng=r)
    ph["texture"] = P(80, n, "Sparse overlapping long notes for textures")

    n = []
    for i in range(16):
        n += seq([(i * .25, .3, 60 + [0, 3, 7, 10, 12, 15, 19, 22][i % 8] + (12 if i >= 8 else 0), .5 + i * .025)], rng=r)
    n += chord(4, 4, [72, 79, 84, 87], .6, 0, r)
    ph["rise"] = P(120, n, "Rising sparkle arpeggio then a held chord")

    n = []
    arp = [72, 75, 79, 84, 79, 75, 82, 79]
    for b in range(4):
        for i, k in enumerate(arp):
            n += seq([(b * 4 + i * .5, .45, k + [0, 0, -4, -2][b], .72)], rng=r)
    ph["glitch"] = P(130, n, "Steady arpeggio so glitch/stutter effects are audible, 130 BPM")

    return ph

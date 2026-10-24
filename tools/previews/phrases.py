"""Original demo phrases used to audition presets (round 2: darker, minor-key, record-ready).

Every phrase is written for this project. Production styles referenced (trapsoul, underwater R&B,
drill, dark reggaeton, dancehall-pop, bounce house) are genres, not songs: nothing is transcribed.
Times are in beats. `roots` drives the optional 808 in the demo beat; `groove` picks the drum pattern.
"""
import random

def chord(t, d, notes, vel=0.7, roll=0.0, rng=None):
    out = []
    for i, n in enumerate(notes):
        v = vel + (rng.uniform(-0.05, 0.05) if rng else 0)
        out.append((t + i * roll, d - i * roll, n, max(0.05, min(1.0, v))))
    return out

def seq(items, vel=0.75, rng=None):
    out = []
    for it in items:
        s, d, n = it[0], it[1], it[2]
        v = it[3] if len(it) > 3 else vel
        if rng: v += rng.uniform(-0.06, 0.06)
        out.append((s, d, n, max(0.05, min(1.0, v))))
    return out

def P(bpm, notes, desc, groove, roots, swing=0.0):
    return {"bpm": bpm, "notes": notes, "desc": desc, "groove": groove, "roots": roots, "swing": swing}

def swung(t, amount):
    """Delay off-beat 16ths by `amount` of a 16th (0..0.5)."""
    six = round(t * 4, 6)
    if abs(six - round(six)) < 1e-6 and int(round(six)) % 2 == 1:
        return t + amount * 0.25
    return t

def build():
    r = random.Random(11)
    ph = {}

    # --- trapsoul / underwater R&B: Fm9 - Dbmaj7(#11) - Bbm9 - C7(b9) at 68 BPM
    n = []
    n += chord(0, 4, [41, 48, 51, 55, 56, 60], .58, .02, r)
    n += chord(4, 4, [37, 48, 53, 55, 60], .55, .02, r)
    n += chord(8, 4, [34, 49, 53, 56, 60], .57, .02, r)
    n += chord(12, 4, [36, 46, 52, 55, 61], .6, .02, r)
    n += seq([(1.5, .5, 72), (2, 1, 75), (3.5, .5, 72), (5, .5, 77), (5.5, 1.5, 75), (7.5, .5, 72),
              (9.5, .5, 73), (10, 1.5, 72), (13, .5, 70), (13.5, .5, 67), (14, 2, 68)], .6, r)
    ph["trapsoul"] = P(68, n, "Moody Fm9 progression with a sparse top line, 68 BPM",
                       "trapsoul", [(0, 29), (4, 25), (8, 34), (12, 36)])

    # --- drill bells: G minor with a flat-2 colour, staccato, 142 BPM (half-time feel)
    mel = [(0, .5, 79), (.75, .5, 79), (1.5, .5, 80), (2, .5, 79), (3, .5, 74), (3.5, .5, 75),
           (4, .5, 79), (4.75, .5, 79), (5.5, .5, 80), (6, .5, 82), (7, 1, 80),
           (8, .5, 79), (8.75, .5, 79), (9.5, .5, 80), (10, .5, 79), (11, .5, 74), (11.5, .5, 75),
           (12, .5, 77), (12.75, .5, 75), (13.5, .5, 74), (14, 2, 71)]
    n = seq(mel, .78, r)
    n += seq([(s, d, k - 12, .45) for s, d, k in [(0, 4, 79), (4, 4, 80), (8, 4, 79), (12, 4, 77)]], rng=r)
    ph["drill_bell"] = P(142, n, "Staccato drill bell line in G minor (flat-2 colour), 142 BPM",
                         "drill", [(0, 31), (2.5, 31), (4, 32), (7, 34), (8, 31), (10.5, 31), (12, 29), (14, 26)])

    # --- dark reggaeton: Am - F - Dm - E (harmonic minor), 3-3-2 riff at 92
    pos = [0, .75, 1.5, 2, 2.75, 3.5]
    arps = {"Am": [69, 72, 76, 69, 72, 71], "F": [65, 69, 72, 65, 69, 67], "Dm": [62, 65, 69, 62, 65, 64], "E": [64, 68, 71, 64, 68, 71]}
    n = []
    for b, ch in enumerate(["Am", "F", "Dm", "E"]):
        for p, note in zip(pos, arps[ch]):
            n += seq([(b * 4 + p, .45, note, .8 if p in (0, 2) else .66)], rng=r)
    ph["reggaeton_dark"] = P(92, n, "Dark 3-3-2 riff, Am-F-Dm-E (harmonic minor), 92 BPM",
                             "reggaeton", [(0, 33), (4, 29), (8, 26), (12, 28)])

    # --- dark reggaeton lead (mono, slides)
    n = seq([(0, .8, 76), (.75, .8, 77), (1.5, 1.6, 76), (3, .6, 72), (3.5, 1.1, 74),
             (4.5, .6, 72), (5, 1.6, 69), (7, .5, 68), (7.5, .6, 69), (8, .8, 76), (8.75, .8, 77),
             (9.5, 1.1, 79), (10.5, .6, 77), (11, 1.1, 76), (12, .6, 74), (12.5, .6, 72), (13, 3, 71)], .76, r)
    ph["reggaeton_lead"] = P(92, n, "Sliding minor lead over the dark reggaeton changes, 92 BPM",
                             "reggaeton", [(0, 33), (4, 29), (8, 26), (12, 28)])

    # --- dancehall-pop, dark: C# minor offbeat riff at 98
    n = []
    riff = [(.5, .5, 73), (1, .25, 76), (1.5, .5, 73), (2.5, .5, 71), (3, .5, 69), (3.5, .5, 68)]
    for b, sh in enumerate([0, 0, -3, -1]):
        n += seq([(b * 4 + s, d, k + sh, .75) for s, d, k in riff], rng=r)
    ph["dancehall_dark"] = P(98, n, "Dark offbeat dancehall-pop riff in C# minor, 98 BPM",
                             "dancehall", [(0, 37), (4, 37), (8, 33), (12, 35)])

    # --- dembow, dark and hard: G minor motif at 124
    n = []
    motif = [(0, .25, 67), (.5, .25, 70), (.75, .25, 67), (1.5, .25, 74), (2, .25, 75), (2.5, .25, 74), (3, .25, 70), (3.5, .25, 69)]
    for b in range(4):
        sh = [0, 0, -4, -2][b]
        n += seq([(b * 4 + s, d, k + sh, .86 if s in (0, 1.5, 2.5) else .7) for s, d, k in motif], rng=r)
    ph["dembow_dark"] = P(124, n, "Hard, dark dembow motif in G minor, 124 BPM",
                          "dembow", [(0, 31), (4, 31), (8, 27), (12, 29)])

    # --- bounce house: swung minor-9 chord stabs, Dm9 - Gm9 - Bbmaj7 - A7(b9) at 118
    n = []
    stabs = [0, .75, 1.5, 2.5, 3.25]
    voic = [[50, 57, 60, 64, 65], [55, 58, 62, 65, 69], [46, 57, 62, 65, 69], [45, 55, 58, 61, 64]]
    for b, v in enumerate(voic):
        for s in stabs:
            n += chord(swung(b * 4 + s, .35), .35, v, .72 if s in (0, 2.5) else .6, 0, r)
    ph["bounce_house"] = P(118, n, "Swung minor-9 chord stabs for bounce/house, 118 BPM",
                           "bounce", [(0, 38), (4, 43), (8, 34), (12, 33)], swing=.35)

    # --- bounce lead / squelch line
    n = []
    line = [(0, .25, 62), (.5, .25, 65), (.75, .5, 69), (1.5, .25, 67), (2, .25, 65), (2.5, .5, 62), (3.25, .5, 60)]
    for b, sh in enumerate([0, 5, -4, -5]):
        n += seq([(swung(b * 4 + s, .35), d, k + sh, .75) for s, d, k in line], rng=r)
    ph["bounce_lead"] = P(118, n, "Swung, rubbery minor line, 118 BPM", "bounce",
                          [(0, 38), (4, 43), (8, 34), (12, 33)], swing=.35)

    # --- dark trap lead / flute (slides, minor, legato overlaps)
    n = seq([(0, 1.6, 77), (1.5, .6, 75), (2, 1.1, 73), (3, .6, 72), (3.5, 2.1, 70),
             (6, .6, 68), (6.5, 1.6, 70), (8, 1.6, 77), (9.5, .6, 80), (10, 1.1, 77), (11, .6, 75),
             (11.5, 1.1, 73), (12.5, .6, 72), (13, 3, 73)], .72, r)
    ph["trap_lead"] = P(140, n, "Haunting B-flat minor lead with slides, 140 BPM (half-time)",
                        "trap", [(0, 34), (4, 30), (8, 37), (12, 32)])

    # --- dark pad progression: C#m9 - Amaj7 - F#m9 - G#7sus
    n = []
    n += chord(0, 4, [37, 49, 52, 56, 59, 63], .6, 0, r)
    n += chord(4, 4, [33, 49, 52, 56, 57, 61], .58, 0, r)
    n += chord(8, 4, [30, 45, 49, 52, 56, 57], .6, 0, r)
    n += chord(12, 4.5, [32, 49, 51, 54, 56, 61], .58, 0, r)
    ph["pad_dark"] = P(70, n, "Dark, emotional C#m9 progression held for full bars, 70 BPM",
                       "trapsoul", [(0, 37), (4, 33), (8, 30), (12, 32)])

    # --- cinematic strings / choir / brass: D minor with a phrygian turn
    n = []
    n += chord(0, 4, [38, 50, 57, 62, 65], .55, .03, r) + chord(4, 4, [39, 51, 58, 63, 67], .6, .03, r)
    n += chord(8, 4, [34, 50, 58, 62, 65], .58, .03, r) + chord(12, 4.5, [33, 49, 57, 61, 64], .65, .03, r)
    n += seq([(0, 2, 74), (2, 2, 77), (4, 3, 75), (7, 1, 74), (8, 2, 74), (10, 2, 70), (12, 4.5, 73)], .62, r)
    ph["cinematic"] = P(70, n, "D minor with an Eb (phrygian) turn, slow and heavy, 70 BPM",
                        "trap", [(0, 38), (4, 39), (8, 34), (12, 33)])

    # --- dark brass stabs
    n = []
    for b, v in enumerate([[50, 57, 62, 65], [51, 58, 63, 67], [46, 58, 62, 65], [45, 57, 61, 64]]):
        n += chord(b * 4, .9, v, .82, 0, r) + chord(b * 4 + 1.5, .4, v, .7, 0, r) + chord(b * 4 + 2.5, 1.2, v, .78, 0, r)
    ph["brass"] = P(140, n, "Hard minor brass stabs, 140 BPM (half-time)", "trap",
                    [(0, 38), (4, 39), (8, 34), (12, 33)])

    # --- dark guitar-style arpeggio (Em(add9) - Cmaj7 - Am9 - B7)
    n = []
    shapes = [[40, 47, 54, 55, 59], [36, 43, 52, 55, 59], [33, 45, 52, 55, 59], [35, 47, 51, 54, 57]]
    for b, sh in enumerate(shapes):
        for i, o in enumerate([0, 2, 3, 4, 3, 2, 1, 3]):
            n += seq([(b * 4 + i * .5, 1.2, sh[o] + 12, .7 if i % 2 == 0 else .56)], rng=r)
    ph["guitar_dark"] = P(76, n, "Moody arpeggio, Em-C-Am-B, 76 BPM", "trapsoul",
                          [(0, 28), (4, 36), (8, 33), (12, 35)])

    # --- muted-guitar style rhythm (A minor, 98 BPM)
    n = []
    pat = [0, .5, .75, 1.5, 2, 2.75, 3, 3.5]
    for b, rt in enumerate([57, 53, 55, 52]):
        for i, x in enumerate(pat):
            n += seq([(b * 4 + x, .22, rt + [0, 7, 12, 7, 0, 12, 10, 7][i], .78 if i in (0, 4) else .6)], rng=r)
    ph["guitar_muted"] = P(98, n, "Rhythmic muted-guitar pattern, Am-F-G-Em, 98 BPM", "dancehall",
                           [(0, 33), (4, 29), (8, 31), (12, 28)])

    # --- cowbell / phonk-style melody (F# minor)
    n = seq([(0, .5, 78), (.5, .5, 78), (1, .5, 81), (1.5, .5, 78), (2, .5, 76), (2.5, .5, 78), (3, 1, 73),
             (4, .5, 78), (4.5, .5, 78), (5, .5, 81), (5.5, .5, 83), (6, .5, 81), (6.5, .5, 78), (7, 1, 76),
             (8, .5, 78), (8.5, .5, 78), (9, .5, 81), (9.5, .5, 78), (10, .5, 76), (10.5, .5, 78), (11, 1, 73),
             (12, .5, 74), (12.5, .5, 73), (13, .5, 71), (13.5, .5, 69), (14, 2, 66)], .8, r)
    ph["phonk"] = P(130, n, "Driving F#-minor cowbell melody, 130 BPM", "phonk",
                    [(0, 30), (4, 30), (8, 30), (12, 26)])

    # --- basses
    n = seq([(0, 1.4, 31), (1.5, .4, 31), (2, 1.1, 31), (3, 1.0, 38), (4, 1.5, 34), (5.5, .5, 34), (6, 2.1, 32),
             (8, 1.4, 31), (9.5, .4, 31), (10, 1.1, 31), (11, 1.1, 41), (12, 1.6, 39), (13.5, 2.5, 38)], .88, r)
    ph["bass_808"] = P(142, n, "808 line with slides in G minor, 142 BPM", "drill_no808", [])

    n = seq([(0, 1.0, 34), (1, .5, 34), (1.5, 1.6, 46), (3, 1.1, 41), (4, 2, 37), (6, 2, 36),
             (8, 1.0, 34), (9, .5, 34), (9.5, 1.6, 46), (11, 1.1, 44), (12, 4, 41)], .86, r)
    ph["bass_trap"] = P(140, n, "Long trap 808 with octave slides in B-flat minor, 140 BPM", "trap_no808", [])

    n = []
    for b, rt in enumerate([31, 31, 27, 29]):
        n += seq([(b * 4 + 0, .6, rt), (b * 4 + .75, .4, rt), (b * 4 + 1.5, .45, rt + 12, .7),
                  (b * 4 + 2, .6, rt), (b * 4 + 3, .4, rt + 3, .75), (b * 4 + 3.5, .4, rt - 2, .7)], .86, r)
    ph["bass_dembow"] = P(124, n, "Punchy dembow/reggaeton bass in G minor, 124 BPM", "dembow_no808", [])

    n = seq([(0, 1.5, 29), (1.5, .5, 36), (2, 1, 39), (3, .9, 41), (4, 1.5, 25), (5.5, .5, 32), (6, 1, 37), (7, .9, 36),
             (8, 1.5, 34), (9.5, .5, 41), (10, 1, 44), (11, .9, 41), (12, 1.5, 36), (13.5, .5, 37), (14, 2, 36)], .78, r)
    ph["bass_rnb"] = P(68, n, "Melodic trapsoul bass in F minor, 68 BPM", "trapsoul_no808", [])

    n = []
    for b, rt in enumerate([38, 43, 34, 33]):
        n += seq([(swung(b * 4 + x, .35), .3, rt + o) for x, o in [(0, 0), (.75, 0), (1.5, 12), (2.5, 0), (3, 10), (3.5, 7)]], .82, r)
    ph["bass_bounce"] = P(118, n, "Swung squelch bass line, 118 BPM", "bounce_no808", [], swing=.35)

    n = seq([(0, 1.0, 37), (1.5, .5, 37), (2, .75, 40), (3, 1.0, 42), (4, .75, 37), (5, .5, 35), (5.5, .5, 37), (6, 2, 33),
             (8, 1.0, 37), (9.5, .5, 37), (10, .75, 44), (11, 1.0, 42), (12, 2, 40), (14, 2, 35)], .84, r)
    ph["bass_dancehall"] = P(98, n, "Deep dancehall bass line in C# minor, 98 BPM", "dancehall_no808", [])

    # --- textures
    n = seq([(0, 6, 61, .6), (2, 6, 68, .5), (4, 8, 73, .45), (8, 6, 64, .55), (10, 6, 71, .5)], rng=r)
    ph["texture"] = P(70, n, "Sparse overlapping C#-minor notes for textures", "trapsoul", [(0, 37), (8, 33)])

    n = []
    arp = [72, 75, 79, 82, 79, 75, 80, 79]
    for b in range(4):
        for i, k in enumerate(arp):
            n += seq([(b * 4 + i * .5, .45, k + [0, 0, -4, -1][b], .72)], rng=r)
    ph["glitch"] = P(140, n, "Minor arpeggio so stutter/glitch effects are audible, 140 BPM", "trap", [(0, 36), (8, 32), (12, 35)])


    # ================= TRIPPY section phrases (original; dark, hypnotic, psychedelic trap / rage / cloud rap)
    # --- psych lead: E minor with a Phrygian flat-2, long notes that slide into each other, 150 BPM half-time
    n = seq([(0, 2.2, 76), (2, 1.2, 77), (3, 1.1, 76), (4, 2.6, 71), (6.5, 1.6, 74), (8, 2.2, 79), (10, 1.1, 77),
             (11, 1.1, 76), (12, 1.6, 74), (13.5, 2.5, 71)], .7, r)
    ph["psych_lead"] = P(150, n, "Hypnotic E-minor lead with a flat-2 and long slides, 150 BPM (half-time)",
                         "trap", [(0, 28), (4, 24), (8, 21), (12, 23)])
    # --- psych chords: Em(add9) - Cmaj7(#11) - Am9 - B7(b9), long and washed, 72 BPM feel
    n = []
    n += chord(0, 4, [40, 52, 55, 59, 66], .55, .03, r)
    n += chord(4, 4, [36, 52, 55, 59, 66], .55, .03, r)
    n += chord(8, 4, [33, 52, 55, 59, 60], .55, .03, r)
    n += chord(12, 4, [35, 51, 54, 57, 60], .58, .03, r)
    ph["psych_pad"] = P(75, n, "Washed Em(add9) - Cmaj7(#11) - Am9 - B7(b9) progression, 75 BPM",
                        "trapsoul", [(0, 28), (4, 24), (8, 21), (12, 23)])
    # --- psych arp: a hypnotic broken-chord pattern over Em - C - Am - B, 150 BPM
    n = []
    for b, ch in enumerate([[64, 67, 71, 74], [64, 67, 71, 72], [64, 69, 72, 76], [63, 66, 71, 75]]):
        for i, k in enumerate([0, 2, 1, 3, 2, 1, 3, 2]):
            n += seq([(b * 4 + i * .5, .42, ch[k], .72 if i % 4 == 0 else .58)], rng=r)
    ph["psych_arp"] = P(150, n, "Hypnotic broken-chord arp, Em - C - Am - B, 150 BPM (half-time)",
                        "trap", [(0, 28), (4, 24), (8, 21), (12, 23)])
    # --- psych 808: long notes, big overlapping slides down and up, E minor, 150 BPM
    n = seq([(0, 1.6, 28), (1.5, 1, 40), (2.4, 1.2, 28), (4, 1.4, 24), (5.25, 1, 31), (6.2, 1.6, 26),
             (8, 1.6, 33), (9.5, .9, 36), (10.3, 1.5, 33), (12, 1.4, 35), (13.3, .8, 38), (14, 2, 28)], .9, r)
    ph["psych_808"] = P(150, n, "Long E-minor 808 line with big slides, 150 BPM", "trap",
                        [(0, 28), (4, 24), (8, 21), (12, 23)])
    return ph

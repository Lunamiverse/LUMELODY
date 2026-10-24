#!/usr/bin/env python3
"""LUMELODY sound libraries: drums, hi-hats, percussion, effects and textures for
NEW JAZZ, DISCO + EDM + CLUB, REGGAETON and DEMBOW DOMINICANO.

Every sound is ORIGINAL SYNTHESIS made here (physical/modal models, filtered noise, FM); no recordings or third-party
samples are used. The tamboras and the güira are physical models of those instruments, played in Dominican patterns:
convincing, but they are not recordings of a tamborero (the library accepts real recordings later with the same ids).

Patterns are note lists that point at one-shots, so they play at any tempo and export as MIDI (GM drum notes) or audio.
Output: web/samples/<id>.flac + web/samples/manifest.json  (run: python3 tools/samples/make_samples.py)
"""
import json, os, subprocess, sys, wave
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from dsp import *   # noqa

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "web", "samples")
ITEMS, ONESHOTS = [], {}

def S(lib, cat, sid, name, desc, fn, note=None, hidden=False):
    ITEMS.append({"kind": "oneshot", "lib": lib, "cat": cat, "id": sid, "name": name, "desc": desc, "fn": fn, "note": note, "hidden": hidden})

def LOOP(lib, cat, sid, name, desc, fn, bpm=None, beats=None):
    ITEMS.append({"kind": "loop", "lib": lib, "cat": cat, "id": sid, "name": name, "desc": desc, "fn": fn, "bpm": bpm, "beats": beats})

def PAT(lib, cat, sid, name, desc, bpm, beats, events, swing=0.0):
    ITEMS.append({"kind": "pattern", "lib": lib, "cat": cat, "id": sid, "name": name, "desc": desc, "bpm": bpm, "beats": beats,
                  "events": [{"s": s, "b": round(b, 4), "v": round(v, 3)} for s, b, v in events], "swing": swing})

def bar_events(spec, bars=1):
    """spec: {sample_id: [(beat, vel), ...]} for one bar (4 beats) -> event list over `bars` bars."""
    ev = []
    for b in range(bars):
        for sid, hits in spec.items():
            for beat, vel in hits:
                ev.append((sid, b * 4 + beat, vel))
    return ev

# =========================================================================================== building blocks
def kick(f_top=160, f_low=50, tau=.03, decay=.32, click=.4, drive=2.0, seed=1, sub_tail=0.0, lpf=8000):
    n = int((decay * 5 + sub_tail) * SR)
    body = sweep_sine(n, f_top, f_low, tau) * env_ad(n, .0008, decay + sub_tail)
    clk = hp(noise(n, seed), 2500) * env_ad(n, .0002, .0035) * click
    x = lp(sat(body + clk, drive), lpf)
    return x

def snare(tone=190, decay=.16, snap=.9, body=.5, seed=2, bp_lo=1200, bp_hi=8000):
    n = int(.6 * SR)
    nz = bp(noise(n, seed), bp_lo, bp_hi) * env_ad(n, .0005, decay) * snap
    tn = (np.sin(2 * np.pi * tone * t(.6)) + .5 * np.sin(2 * np.pi * tone * 1.6 * t(.6))) * env_ad(n, .0005, .05) * body
    return sat(nz + tn, 1.4)

def clap(spread_ms=(0, 9, 18, 27), decay=.14, seed=3, lo=900, hi=6000):
    n = int(.7 * SR); x = np.zeros(n); nz = noise(n, seed)
    for k, off in enumerate(spread_ms):
        o = int(off * SR / 1000); m = n - o
        x[o:] += nz[:m] * env_ad(m, .0004, .006) * (1 - .1 * k)
    x += nz * env_ad(n, .004 + spread_ms[-1] / 1000, decay) * .6
    return bp(x, lo, hi)

def hat_metal(n, f0=310, seed=4):
    """808-style metallic cluster of square waves."""
    ratios = [1, 1.47, 1.71, 1.94, 2.53, 2.66]
    x = np.zeros(n); tt = np.arange(n) / SR
    rr = rng(seed)
    for r in ratios:
        x += np.sign(np.sin(2 * np.pi * f0 * r * tt + rr.uniform(0, 6.28)))
    return x / len(ratios)

def hat(decay=.03, f0=310, tone=.6, seed=4, lo=7000, hi=16000, n_secs=None):
    n = int((n_secs or decay * 8 + .02) * SR)
    x = hat_metal(n, f0, seed) * tone + noise(n, seed) * (1 - tone)
    return bp(hp(x, lo), lo, hi) * env_ad(n, .0004, decay)

def wood(f0=900, decay=.04, seed=5, ratios=(1, 2.57, 4.2, 6.1), click=.4):
    n = int(.4 * SR)
    x = modal(n, f0, ratios, [1, .45, .25, .1], [decay, decay * .6, decay * .4, decay * .25], rng(seed))
    x += hp(noise(n, seed), 3000) * env_ad(n, .0001, .002) * click
    return x

def membrane(f0, decay, strike=.5, seed=6, damp=1.0, mods=(1, 1.59, 2.14, 2.30, 2.65, 2.92, 3.16, 3.50), pitch_drop=.06):
    """circular membrane (Bessel mode ratios) with a strike transient and a small pitch drop as the skin relaxes."""
    n = int(max(.25, decay * 6) * SR)
    tt = np.arange(n) / SR
    out = np.zeros(n); rr = rng(seed)
    amps = [1, .62, .45, .38, .3, .22, .16, .1]
    for k, (r, a) in enumerate(zip(mods, amps)):
        f = f0 * r * (1 + pitch_drop * np.exp(-tt / .015))
        de = decay * damp / (1 + .55 * k)
        out += a * (1 if k == 0 else strike + .2) * np.sin(2 * np.pi * np.cumsum(f) / SR + rr.uniform(0, 6.28)) * np.exp(-tt / de)
    return out

def scrape(n, rate_hz=90, lo=2500, hi=11000, seed=7, ridges=.85):
    """a stick dragged over ridges (güira, güiro): a train of tiny clicks through a metal / wood resonance."""
    tt = np.arange(n) / SR
    ph = np.cumsum(np.full(n, rate_hz / SR)); ticks = np.diff(np.floor(ph), prepend=0) > 0
    imp = ticks.astype(float) * (1 + .3 * rng(seed).standard_normal(n))
    x = np.convolve(imp, np.exp(-np.arange(int(.003 * SR)) / (.0008 * SR)), mode="same")
    x = ridges * x + (1 - ridges) * noise(n, seed) * .3
    return bp(x, lo, hi)

def formant_voice(dur, f0, vowels, seed=8, breath=.15):
    """a short synthesized 'voice' (pulse source through three formant resonances): vocal-style FX, not words."""
    n = int(dur * SR); tt = np.arange(n) / SR
    src = np.zeros(n); ph = np.cumsum(np.full(n, f0 / SR) * (1 + .03 * np.sin(2 * np.pi * 5 * tt)))
    src = (np.mod(ph, 1) < .12).astype(float) - .12
    src += breath * noise(n, seed) * .3
    F = {"a": (800, 1200, 2500), "e": (500, 1900, 2600), "i": (300, 2300, 3000), "o": (450, 800, 2600), "u": (325, 700, 2500)}
    out = np.zeros(n)
    seg = n // len(vowels)
    for k, v in enumerate(vowels):
        a, b = k * seg, n if k == len(vowels) - 1 else (k + 1) * seg
        part = np.zeros(n); part[a:b] = src[a:b]
        for j, f in enumerate(F[v]):
            out += bp(part, f * .85, f * 1.15, 2) * (1, .6, .3)[j]
    return out * env_adsr(n, .015, .1, .8, .06, dur - .08)

def chord_saw(dur, notes, detune=.006, seed=9):
    n = int(dur * SR); tt = np.arange(n) / SR; x = np.zeros(n); rr = rng(seed)
    for m in notes:
        f = 440 * 2 ** ((m - 69) / 12)
        for d in (-detune, 0, detune):
            ph = np.mod(f * (1 + d) * tt + rr.uniform(), 1)
            x += 2 * ph - 1
    return x / (len(notes) * 3)

# =========================================================================================== NEW JAZZ
NJ = "New Jazz"
S(NJ, "Drums", "nj-dust-kick", "Dust Kick", "Soft, round kick with a felt-like attack and a warm saturated body: sits under jazzy chords.",
  lambda: finish(stereo(lp(kick(130, 52, .035, .3, .15, 2.2, 11), 4500))), note=36)
S(NJ, "Drums", "nj-knock-kick", "Knock Kick", "Short, tight kick with a woody knock on top, for busy underground patterns.",
  lambda: finish(stereo(kick(190, 58, .018, .18, .7, 2.6, 12) + wood(320, .02, 13, click=.2) * .25)), note=36)
S(NJ, "Drums", "nj-brushed-snare", "Brushed Snare", "Brush-like snare: a soft swish of noise with a quiet head tone and a short room.",
  lambda: finish(reverb(stereo(snare(210, .12, .7, .22, 14, 900, 6000) * env_ad(int(.6 * SR), .006, .2), .3, 14), .5, .2, 15)), note=38)
S(NJ, "Drums", "nj-rim-knock", "Rim Knock", "Dry stick-on-rim click with a hollow wooden tone.",
  lambda: finish(stereo(wood(780, .03, 16, (1, 2.3, 3.9), .6) + bp(noise(int(.4 * SR), 16), 1500, 6000) * env_ad(int(.4 * SR), .0002, .006) * .4)), note=37)
PAT(NJ, "Drums", "nj-pattern-laidback", "Laid-Back Pattern", "Swung, behind-the-beat new-jazz groove: kick, brushed snare and rim knocks, 84 BPM.", 84, 4,
    bar_events({"nj-dust-kick": [(0, .95), (1.75, .65), (2.5, .8)], "nj-brushed-snare": [(1, .8), (3, .85)], "nj-rim-knock": [(2.75, .4), (3.75, .35)],
                "nj-hat-tight": [(i * .5, .55 if i % 2 == 0 else .38) for i in range(8)]}), swing=.3)
S(NJ, "Hi-Hats", "nj-hat-tight", "Tight Hat", "Crisp, short closed hat with a dark top, for intricate patterns.",
  lambda: finish(stereo(lp(hat(.022, 320, .55, 21), 11000), .2, 21, .3)), note=42)
S(NJ, "Hi-Hats", "nj-hat-open", "Smoky Open Hat", "Dark open hat that breathes out slowly.",
  lambda: finish(stereo(lp(hat(.16, 300, .5, 22, 6000, 14000), 9000), .2, 22, .3)), note=46)
S(NJ, "Hi-Hats", "nj-hat-pedal", "Pedal Chick", "Soft foot-pedal 'chick': short, closed and low in the mix.",
  lambda: finish(stereo(bp(hat(.012, 280, .4, 23, 4000, 11000), 4000, 10000) * .8)), note=44)
PAT(NJ, "Hi-Hats", "nj-hat-roll", "Triplet Roll", "Hat pattern with triplet rolls and ghost notes for melodic trap and new jazz, 84 BPM.", 84, 4,
    bar_events({"nj-hat-tight": [(i * .5, .6 if i % 2 == 0 else .4) for i in range(6)] + [(3, .45), (3 + 1/6, .4), (3 + 2/6, .45), (3.5, .5), (3.5 + 1/6, .42), (3.5 + 2/6, .5)]}), swing=.15)
PAT(NJ, "Hi-Hats", "nj-hat-shuffle", "Shuffle Hats", "Swung 8ths with an open hat on the 'and' of 4, 84 BPM.", 84, 4,
    bar_events({"nj-hat-tight": [(i * .5, .55 if i % 2 == 0 else .4) for i in range(7)], "nj-hat-open": [(3.5, .5)], "nj-hat-pedal": [(1, .35), (3, .35)]}), swing=.35)
S(NJ, "Percussion", "nj-finger-snap", "Finger Snap", "Close, dry finger snap with a quick room.",
  lambda: finish(reverb(stereo(bp(noise(int(.3 * SR), 31), 1200, 5500) * env_ad(int(.3 * SR), .0003, .02) + wood(1900, .01, 31, click=.3) * .3), .35, .15, 31)), note=39)
S(NJ, "Percussion", "nj-woodblock", "Night Woodblock", "Hollow, pitched woodblock: a soft accent for off-beats.",
  lambda: finish(stereo(wood(1150, .05, 32, (1, 2.76, 5.4), .25))), note=76)
S(NJ, "Percussion", "nj-shaker", "Dust Shaker", "Gentle, grainy shaker stroke.",
  lambda: finish(stereo(bp(noise(int(.25 * SR), 33), 4500, 12000) * env_ad(int(.25 * SR), .015, .035), .3, 33, .4)), note=82)
S(NJ, "Percussion", "nj-metal-ping", "Metal Ping", "Small, bright metal ping with a long, quiet ring: a futuristic accent.",
  lambda: finish(reverb(stereo(modal(int(1.6 * SR), 1320, (1, 2.32, 4.25, 6.63), (1, .4, .2, .1), (.6, .35, .2, .1), rng(34))), 1.6, .3, 34)), note=81)
S(NJ, "Percussion", "nj-box-slap", "Box Slap", "Cajón-style slap: a low thump with a snare buzz on top.",
  lambda: finish(stereo(membrane(95, .07, .3, 35) * .9 + bp(noise(int(.4 * SR), 35), 1800, 7000) * env_ad(int(.4 * SR), .001, .06) * .35)), note=40)
S(NJ, "Effects", "nj-tape-stop", "Tape Stop", "A held Dm9 chord that slows to a halt like a stopped tape.",
  lambda: finish(reverb(stereo(tape_stop()), .8, .2, 41)))
S(NJ, "Effects", "nj-reverse-cymbal", "Reverse Cymbal", "A dark cymbal played backwards: swells up into the downbeat.",
  lambda: finish(stereo(reverse_cymbal(2.0, 42), .3, 42, .5), fi=.05, fo=.004))
S(NJ, "Effects", "nj-spinback", "Vinyl Spinback", "A record pulled back by hand: a quick rising-falling rewind sweep.",
  lambda: finish(stereo(spinback(43), .2, 43, .3)))
S(NJ, "Effects", "nj-space-riser", "Space Riser", "Filtered noise and a rising tone that climb for two bars at 84 BPM.",
  lambda: finish(riser(5.7, 44, tone=True, top=7000), fi=.3, fo=.05))
S(NJ, "Effects", "nj-sub-drop", "Sub Drop", "A deep sine falling an octave and a half: the drop under a transition.",
  lambda: finish(stereo(sweep_sine(int(2.2 * SR), 110, 32, .45) * env_ad(int(2.2 * SR), .002, .7)), fo=.1))
LOOP(NJ, "Textures", "nj-vinyl-crackle", "Vinyl Crackle", "Warm surface noise with soft pops, loopable.", lambda: finish(vinyl(8.0, 51), rms_db=-30, peak_db=-6, tail=False, fi=.05, fo=.05))
LOOP(NJ, "Textures", "nj-rain-room", "Rain Room", "Rain on a window heard from inside: soft, dense, loopable.", lambda: finish(rain(8.0, 52), rms_db=-28, peak_db=-6, tail=False, fi=.2, fo=.2))
LOOP(NJ, "Textures", "nj-granular-night", "Granular Night", "Scattered glassy grains in a dark space: a slowly shifting bed.", lambda: finish(grains(8.0, 53, [62, 65, 69, 72, 76]), rms_db=-26, peak_db=-4, tail=False, fi=.3, fo=.3))
LOOP(NJ, "Textures", "nj-tape-air", "Tape Air", "Soft tape hiss with a gentle wow: adds 'air' and age to a mix.", lambda: finish(tape_air(8.0, 54), rms_db=-34, peak_db=-10, tail=False, fi=.2, fo=.2))
LOOP(NJ, "Textures", "nj-midnight-hum", "Midnight Hum", "Low, warm, slowly beating drone on D: a bed under intros.", lambda: finish(drone(8.0, 38, 55), rms_db=-24, peak_db=-4, tail=False, fi=.4, fo=.4))

# =========================================================================================== DISCO + EDM + CLUB
DC = "Disco + EDM + Club"
S(DC, "Drums", "dc-floor-kick", "Four-Floor Kick", "Punchy, tight house kick: firm click, round 50 Hz body, short tail.",
  lambda: finish(stereo(kick(200, 50, .022, .26, .6, 2.4, 61))), note=36)
S(DC, "Drums", "dc-bigroom-kick", "Big Room Kick", "Long, distorted festival kick with a pitched tail.",
  lambda: finish(stereo(kick(320, 52, .03, .45, .8, 4.0, 62, .15))), note=36)
S(DC, "Drums", "dc-disco-snare", "Disco Snare", "Bright, open snare with a short plate: classic dance snare.",
  lambda: finish(reverb(stereo(snare(200, .15, 1, .55, 63)), .9, .22, 63, 9000)), note=38)
S(DC, "Drums", "dc-club-clap", "Club Clap", "Wide, layered clap with a short room, for 2 and 4.",
  lambda: finish(reverb(stereo(clap((0, 8, 17, 25), .13, 64), .4, 64, .6), .7, .25, 64)), note=39)
PAT(DC, "Drums", "dc-disco-house-groove", "Disco House Groove", "Four-on-the-floor with claps on 2 and 4, offbeat open hats and 16th shakers, 124 BPM.", 124, 4,
    bar_events({"dc-floor-kick": [(b, 1) for b in range(4)], "dc-club-clap": [(1, .85), (3, .9)], "dc-hat-open": [(b + .5, .55) for b in range(4)],
                "dc-hat-closed": [(i * .25, .32) for i in range(16) if i % 2], "dc-shaker": [(i * .25, .3 if i % 2 else .22) for i in range(16)]}))
S(DC, "Hi-Hats", "dc-hat-closed", "Sparkle Hat", "Bright, crisp closed hat that sparkles at the top.",
  lambda: finish(stereo(hat(.028, 340, .65, 71, 7500, 17000), .2, 71, .3)), note=42)
S(DC, "Hi-Hats", "dc-hat-open", "Offbeat Open Hat", "Classic open hat for the offbeats, with a clean decay.",
  lambda: finish(stereo(hat(.11, 330, .6, 72, 6500, 16000), .2, 72, .3)), note=46)
S(DC, "Hi-Hats", "dc-hat-sizzle", "Sizzle Hat", "Long, sizzling hat with a ride-like shimmer.",
  lambda: finish(stereo(hat(.32, 360, .7, 73, 6000, 16000), .3, 73, .5)), note=51)
PAT(DC, "Hi-Hats", "dc-hat-16-groove", "16th Groove", "Dynamic 16th-note hats with accents and an open hat on every 'and', 124 BPM.", 124, 4,
    bar_events({"dc-hat-closed": [(i * .25, (.7, .35, .5, .35)[i % 4]) for i in range(16) if i % 4 != 2], "dc-hat-open": [(b + .5, .55) for b in range(4)]}))
PAT(DC, "Hi-Hats", "dc-hat-build", "Build Hats", "Hats that double from 8ths to 16ths to 32nds over two bars: a build-up, 128 BPM.", 128, 8,
    [("dc-hat-closed", i * .5, .45 + .02 * i) for i in range(8)] + [("dc-hat-closed", 4 + i * .25, .5 + .01 * i) for i in range(8)] + [("dc-hat-closed", 6 + i * .125, .55 + .02 * i) for i in range(16)])
S(DC, "Percussion", "dc-tambourine", "Tambourine", "Bright tambourine shake: jingles over a soft skin tap.",
  lambda: finish(stereo(tambourine(81), .3, 81, .4)), note=54)
S(DC, "Percussion", "dc-shaker", "Groove Shaker", "Even, crisp shaker for 16th-note movement.",
  lambda: finish(stereo(bp(noise(int(.15 * SR), 82), 5000, 13000) * env_ad(int(.15 * SR), .008, .025), .3, 82, .3)), note=82)
S(DC, "Percussion", "dc-conga", "Conga Open", "Open, ringing high conga for disco percussion grooves.",
  lambda: finish(stereo(membrane(330, .13, .5, 83, pitch_drop=.04))), note=63)
S(DC, "Percussion", "dc-cowbell", "Dance Cowbell", "Two-tone electronic cowbell, short and bright.",
  lambda: finish(stereo(cowbell(540, 800, .08, 84))), note=56)
S(DC, "Percussion", "dc-clave", "Clave", "Hard, bright wooden clave click.",
  lambda: finish(stereo(wood(2500, .03, 85, (1, 2.1, 3.3), .3))), note=75)
S(DC, "Effects", "dc-uplifter", "Uplifter", "Eight-bar white-noise uplifter with a rising filter and a lifting tone, 124 BPM.",
  lambda: finish(riser(15.5, 91, tone=True, top=12000, wide=True), fi=.5, fo=.05))
S(DC, "Effects", "dc-downlifter", "Downlifter", "Falling noise sweep after a drop, two bars.",
  lambda: finish(downlifter(3.9, 92), fo=.3))
S(DC, "Effects", "dc-drop-impact", "Drop Impact", "Boom + crash + sub for the first beat of a drop.",
  lambda: finish(impact(93, sub=48)))
S(DC, "Effects", "dc-filter-sweep", "Filter Sweep", "An Am7 supersaw chord opening from dark to bright over two bars.",
  lambda: finish(reverb(stereo(lp_sweep(chord_saw(3.9, [57, 60, 64, 67], .008, 94), 200, 12000)), 1.5, .2, 94), fi=.05, fo=.2))
PAT(DC, "Effects", "dc-snare-build", "Snare Roll Build", "Snare roll from quarters to 32nds over four bars, rising in level: the classic build, 128 BPM.", 128, 16,
    [("dc-disco-snare", i * 1.0, .45) for i in range(4)] + [("dc-disco-snare", 4 + i * .5, .5 + .01 * i) for i in range(8)]
    + [("dc-disco-snare", 8 + i * .25, .55 + .01 * i) for i in range(16)] + [("dc-disco-snare", 12 + i * .125, .6 + .012 * i) for i in range(32)])
LOOP(DC, "Textures", "dc-shimmer-air", "Shimmer Air", "Bright, shimmering high air with sparkling partials.", lambda: finish(shimmer(8.0, 101), rms_db=-28, peak_db=-6, tail=False, fi=.3, fo=.3))
LOOP(DC, "Textures", "dc-analog-warmth", "Analog Warmth", "Warm, chorused analog drone on A: glue under chords.", lambda: finish(analog_drone(8.0, 45, 102), rms_db=-24, peak_db=-4, tail=False, fi=.4, fo=.4))
LOOP(DC, "Textures", "dc-neon-buzz", "Neon Buzz", "A neon sign's hum and flicker: futuristic room tone.", lambda: finish(neon(8.0, 103), rms_db=-30, peak_db=-8, tail=False, fi=.2, fo=.2))
LOOP(DC, "Textures", "dc-club-room", "Club Room Tone", "A muffled big room heard from the bar: low thumps and air, at 124 BPM.", lambda: finish(club_room(7.74, 104), rms_db=-28, peak_db=-6, tail=False, fi=.2, fo=.2), bpm=124, beats=16)
LOOP(DC, "Textures", "dc-glitter", "Glitter Sparkles", "Random tiny bell sparkles across the stereo field.", lambda: finish(grains(8.0, 105, [84, 88, 91, 96, 100], dens=14, short=True), rms_db=-30, peak_db=-6, tail=False, fi=.2, fo=.2))

# =========================================================================================== REGGAETON
RG = "Reggaetón"
S(RG, "Drums", "rg-dembow-kick", "Dembow Kick", "Punchy, short reggaetón kick: tight low end that leaves room for the 808.",
  lambda: finish(stereo(kick(170, 55, .02, .22, .55, 2.6, 111))), note=36)
S(RG, "Drums", "rg-perreo-snare", "Perreo Snare", "Crisp, snappy reggaetón snare with a bright crack.",
  lambda: finish(reverb(stereo(snare(230, .1, 1, .45, 112, 1500, 9000)), .4, .12, 112)), note=38)
S(RG, "Drums", "rg-rimshot", "Rimshot", "Hard rimshot click with a ringing tone: the classic dembow accent.",
  lambda: finish(stereo(wood(820, .04, 113, (1, 2.4, 4.1), .5) * .8 + snare(400, .03, .4, .2, 113, 2500, 9000) * .3)), note=37)
PAT(RG, "Drums", "rg-classic-dembow", "Classic Dembow", "The core reggaetón rhythm: kick on every beat, snare on the 3-3-2 'boom-ch-boom-chick', 94 BPM.", 94, 4,
    bar_events({"rg-dembow-kick": [(b, 1 if b % 2 == 0 else .9) for b in range(4)], "rg-perreo-snare": [(.75, .85), (1.5, .8), (2.75, .85), (3.5, .8)],
                "rg-hat-closed": [(i * .5, .45) for i in range(8)]}))
PAT(RG, "Drums", "rg-dark-perreo", "Dark Perreo", "Heavier, sparser dembow with rimshots and a fill in the last beat, 92 BPM.", 92, 8,
    bar_events({"rg-dembow-kick": [(0, 1), (1, .85), (2, 1), (3, .85)], "rg-rimshot": [(.75, .8), (1.5, .75), (2.75, .8)], "rg-perreo-snare": [(3.5, .85)],
                "rg-hat-closed": [(i * .5, .4) for i in range(8)]}, 2) + [("rg-perreo-snare", 7.75, .6), ("rg-perreo-snare", 7.875, .7)])
S(RG, "Hi-Hats", "rg-hat-closed", "Perreo Hat", "Clean, tight closed hat for reggaetón grooves.",
  lambda: finish(stereo(hat(.025, 330, .6, 121, 7500, 16000), .2, 121, .3)), note=42)
S(RG, "Hi-Hats", "rg-hat-open", "Open Hat Suave", "Smooth open hat for the 'and' of the beat.",
  lambda: finish(stereo(hat(.12, 320, .55, 122, 6500, 15000), .2, 122, .3)), note=46)
PAT(RG, "Hi-Hats", "rg-hat-16", "16ths Perreo", "Steady 16th hats with accents that follow the dembow, 94 BPM.", 94, 4,
    bar_events({"rg-hat-closed": [(i * .25, .55 if i in (0, 3, 6, 8, 11, 14) else .3) for i in range(16)]}))
PAT(RG, "Hi-Hats", "rg-hat-rolls", "Hat Rolls", "Hats with fast 32nd rolls leading into each half-bar, 94 BPM.", 94, 4,
    bar_events({"rg-hat-closed": [(i * .5, .45) for i in range(8)] + [(1.75 + k * .0625, .3 + .06 * k) for k in range(4)] + [(3.5 + k * .0625, .3 + .06 * k) for k in range(8)], "rg-hat-open": [(3, .45)]}))
S(RG, "Hi-Hats", "rg-hat-shaker", "Shaker Hat", "A hat made of shaker grains: softer, for old-school grooves.",
  lambda: finish(stereo(bp(noise(int(.2 * SR), 125), 5500, 14000) * env_ad(int(.2 * SR), .004, .03) + hat(.02, 300, .5, 125) * .4, .3, 125, .3)), note=70)
S(RG, "Percussion", "rg-bongo-hi", "Bongo Macho", "Bright high bongo slap.",
  lambda: finish(stereo(membrane(420, .07, .7, 131, pitch_drop=.05))), note=60)
S(RG, "Percussion", "rg-conga-low", "Conga Tumba", "Deep, round low conga (tumba).",
  lambda: finish(stereo(membrane(190, .16, .45, 132, pitch_drop=.04))), note=64)
S(RG, "Percussion", "rg-timbal", "Timbal", "Metal-shelled timbal hit with a bright ring.",
  lambda: finish(stereo(membrane(380, .1, .6, 133) * .7 + modal(int(.6 * SR), 1650, (1, 1.5, 2.3), (.5, .3, .2), (.2, .12, .08), rng(133)) * .35)), note=65)
S(RG, "Percussion", "rg-guiro", "Güiro", "Scraped güiro stroke: a gourd with ridges, long-short.",
  lambda: finish(stereo(guiro(134), .2, 134, .3)), note=73)
S(RG, "Percussion", "rg-clave", "Clave Perreo", "Dry, high clave accent.",
  lambda: finish(stereo(wood(2300, .025, 135, (1, 2.2, 3.4), .35))), note=75)
S(RG, "Effects", "rg-riser", "Riser Perreo", "A two-bar riser: filtered noise climbing to the drop, 94 BPM.",
  lambda: finish(riser(5.1, 141, tone=False, top=9000), fi=.3, fo=.05))
S(RG, "Effects", "rg-reverse-crash", "Reverse Crash", "Bright crash played backwards into the downbeat.",
  lambda: finish(stereo(reverse_cymbal(2.6, 142, bright=True), .3, 142, .5), fi=.05, fo=.004))
S(RG, "Effects", "rg-sub-drop", "Sub Drop", "Short sub drop with a little grit, for the space before the chorus.",
  lambda: finish(stereo(sat(sweep_sine(int(1.6 * SR), 90, 35, .3) * env_ad(int(1.6 * SR), .002, .5), 1.6)), fo=.1))
S(RG, "Effects", "rg-laser", "Laser Zap", "Classic falling 'laser' zap.",
  lambda: finish(reverb(stereo(sweep_sine(int(.5 * SR), 2600, 180, .07) * env_ad(int(.5 * SR), .001, .12)), .7, .2, 144)))
S(RG, "Effects", "rg-whoosh", "Whoosh", "Fast stereo whoosh that pans across.",
  lambda: finish(whoosh(1.2, 145)))
LOOP(RG, "Textures", "rg-noche", "Noche Ambience", "Warm night air with soft insect chirps, loopable.", lambda: finish(night(8.0, 151), rms_db=-30, peak_db=-6, tail=False, fi=.3, fo=.3))
LOOP(RG, "Textures", "rg-dark-drone", "Dark Drone", "Low, dark, slowly filtered drone on B.", lambda: finish(drone(8.0, 35, 152, dark=True), rms_db=-24, peak_db=-4, tail=False, fi=.4, fo=.4))
LOOP(RG, "Textures", "rg-vinyl-old", "Old-School Vinyl", "Thin, crackly vinyl with a band-limited hiss: early-2000s feel.", lambda: finish(bp(vinyl(8.0, 153, pops=1.6), 300, 7000), rms_db=-32, peak_db=-6, tail=False, fi=.05, fo=.05))
LOOP(RG, "Textures", "rg-rain", "Lluvia", "Heavy tropical rain bed.", lambda: finish(rain(8.0, 154, heavy=True), rms_db=-27, peak_db=-6, tail=False, fi=.2, fo=.2))
LOOP(RG, "Textures", "rg-futuro-glitch", "Futuro Glitch", "Stuttering digital texture for experimental reggaetón, at 94 BPM.", lambda: finish(glitch(5.1, 155, 94), rms_db=-28, peak_db=-6, tail=False, fi=.05, fo=.05), bpm=94, beats=8)

# =========================================================================================== DEMBOW DOMINICANO
DD = "Dembow Dominicano"
# --- tamboras: two-headed Dominican drum. One head is played with the open hand ("mano"), the other with a stick
#     ("palito"), and the stick also strikes the wooden shell ("cáscara"). Physical model, not a recording.
S(DD, "Drums", "dd-tambora-palito", "Tambora Palito", "Tambora: stick on the head, open — the bright, ringing accent (physical model).",
  lambda: finish(stereo(tambora_palito(201), .15, 201, .2)), note=45)
S(DD, "Drums", "dd-tambora-mano", "Tambora Mano", "Tambora: open hand on the goat-skin head — a deeper, warmer tone (physical model).",
  lambda: finish(stereo(tambora_mano(202), .15, 202, .2)), note=43)
S(DD, "Drums", "dd-tambora-tapao", "Tambora Tapao", "Tambora: muted hand slap, short and dry (physical model).",
  lambda: finish(stereo(tambora_tapao(203))), note=41)
S(DD, "Drums", "dd-tambora-cascara", "Tambora Cáscara", "Tambora: stick on the wooden shell — the clicking timekeeper (physical model).",
  lambda: finish(stereo(tambora_cascara(204), .1, 204, .1)), note=48)
S(DD, "Drums", "dd-tambora-redoble", "Tambora Redoble", "A tambora roll (redoble) that accelerates into the downbeat.",
  lambda: finish(stereo(tambora_roll(205), .15, 205, .2), fi=.002, fo=.03), note=47)
S(DD, "Drums", "dd-kick-calle", "Kick Calle", "Hard, punchy dembow kick with a distorted knock.",
  lambda: finish(stereo(kick(230, 56, .016, .2, .9, 3.6, 206))), note=36)
S(DD, "Drums", "dd-snare-dembow", "Snare Dembow", "Sharp, cracking snare for fast Dominican dembow.",
  lambda: finish(reverb(stereo(snare(250, .09, 1.1, .4, 207, 1800, 10000)), .35, .1, 207)), note=38)
S(DD, "Drums", "dd-clap-calle", "Clap Calle", "Tight, aggressive clap.", lambda: finish(stereo(clap((0, 6, 12), .09, 208, 1100, 7000), .3, 208, .4)), note=39)
S(DD, "Drums", "dd-rim-dembow", "Rim Dembow", "Bright rimshot for the dembow off-beats.", lambda: finish(stereo(wood(900, .035, 209, (1, 2.4, 4.0), .55))), note=37)
PAT(DD, "Drums", "dd-tambora-jaleo", "Tambora Jaleo", "Tambora pattern: cáscara clicks keep time, the palito accents the 'and' of 2 and the 4, the mano answers — a merengue-rooted figure adapted to dembow, 122 BPM.", 122, 4,
    bar_events({"dd-tambora-cascara": [(0, .7), (.5, .55), (1, .65), (1.5, .55), (2, .7), (2.5, .55), (3, .65), (3.5, .55)],
                "dd-tambora-palito": [(1.75, .9), (3, .95), (3.75, .8)], "dd-tambora-mano": [(0, .8), (2, .75), (2.75, .6)], "dd-tambora-tapao": [(1.25, .55), (3.25, .5)]}))
PAT(DD, "Drums", "dd-dembow-dominicano", "Dembow Dominicano", "Fast Dominican dembow: hard four-on-the-floor kick, snare/rim on the dembow, tambora accents and güira, 124 BPM.", 124, 8,
    bar_events({"dd-kick-calle": [(b, 1) for b in range(4)], "dd-snare-dembow": [(.75, .85), (1.5, .8), (2.75, .85), (3.5, .8)], "dd-clap-calle": [(1, .55), (3, .6)],
                "dd-tambora-palito": [(1.75, .7), (3.75, .75)], "dd-tambora-cascara": [(i * .5 + .25, .4) for i in range(8)], "dd-hat-fast": [(i * .25, .42 if i % 2 else .3) for i in range(16)],
                "dd-guira-corto": [(i * .5, .55 if i % 2 == 0 else .4) for i in range(8)]}, 2) + [("dd-tambora-redoble", 7.0, .8)])
PAT(DD, "Drums", "dd-dembow-calle", "Dembow Calle", "Underground street dembow: kick and rimshots, rolling hats, tambora tapao ghost notes, 120 BPM.", 120, 4,
    bar_events({"dd-kick-calle": [(0, 1), (1, .9), (2, 1), (2.5, .7), (3, .9)], "dd-rim-dembow": [(.75, .8), (1.5, .75), (2.75, .8), (3.5, .75)],
                "dd-tambora-tapao": [(.25, .4), (1.25, .45), (2.25, .4), (3.25, .45)], "dd-hat-fast": [(i * .25, .45 if i % 4 == 0 else .3) for i in range(16)]}))
S(DD, "Hi-Hats", "dd-hat-fast", "Hat Rápido", "Tight, bright closed hat that can run fast 16ths.",
  lambda: finish(stereo(hat(.02, 350, .65, 211, 8000, 17000), .2, 211, .3)), note=42)
S(DD, "Hi-Hats", "dd-hat-open", "Hat Abierto", "Short, bright open hat for dembow off-beats.",
  lambda: finish(stereo(hat(.09, 340, .6, 212, 7000, 16000), .2, 212, .3)), note=46)
PAT(DD, "Hi-Hats", "dd-hat-rolls", "Rolls Dembow", "16th hats with 32nd and triplet rolls, the energy of fast dembow, 124 BPM.", 124, 4,
    bar_events({"dd-hat-fast": [(i * .25, .45 if i % 2 == 0 else .3) for i in range(12)] + [(3 + k * .125, .35 + .05 * k) for k in range(8)]}))
PAT(DD, "Hi-Hats", "dd-hat-galope", "Hats Galope", "Galloping hats (16th + two 32nds) with an open hat on the 'and' of 4, 122 BPM.", 122, 4,
    bar_events({"dd-hat-fast": [(b + o, v) for b in range(4) for o, v in ((0, .5), (.25, .3), (.375, .35))] + [(b + .5, .4) for b in range(4)], "dd-hat-open": [(3.5, .5)]}))
S(DD, "Hi-Hats", "dd-hat-crunch", "Hat Crunch", "Crushed, lo-fi hat with a gritty edge for underground dembow.",
  lambda: finish(stereo(np.round(hat(.03, 300, .5, 215, 5000, 14000) * 12) / 12, .2, 215, .3)), note=44)
# --- güira: a metal scraper played with a comb-like stick — the other half of the Dominican rhythm section
S(DD, "Percussion", "dd-guira-largo", "Güira Raspado", "Güira: a long scrape down the metal (physical model).",
  lambda: finish(stereo(guira(.32, 221), .25, 221, .3)), note=69)
S(DD, "Percussion", "dd-guira-corto", "Güira Corto", "Güira: a short, crisp stroke for 8th-note patterns (physical model).",
  lambda: finish(stereo(guira(.09, 222), .25, 222, .3)), note=82)
PAT(DD, "Percussion", "dd-guira-pattern", "Güira Merengue", "Güira pattern: long scrape on the beat, short strokes on the 16ths, a merengue-rooted figure for dembow, 122 BPM.", 122, 4,
    bar_events({"dd-guira-largo": [(0, .8), (2, .8)], "dd-guira-corto": [(.5, .55), (.75, .45), (1, .65), (1.5, .55), (1.75, .45), (2.5, .55), (2.75, .45), (3, .65), (3.5, .55), (3.75, .5)]}))
S(DD, "Percussion", "dd-campana", "Campana", "Bright, cutting cowbell (campana) for dembow accents.",
  lambda: finish(stereo(cowbell(640, 960, .1, 224))), note=56)
S(DD, "Percussion", "dd-jaleo-claps", "Palmas Jaleo", "A group of hand claps, a little loose, like a crowd keeping time.",
  lambda: finish(reverb(stereo(group_claps(225), .4, 225, .5), .5, .18, 225)), note=39)
S(DD, "Percussion", "dd-maracas", "Maracas", "Bright maracas shake.", lambda: finish(stereo(bp(noise(int(.12 * SR), 226), 4000, 12000) * env_ad(int(.12 * SR), .006, .022), .3, 226, .3)), note=70)
S(DD, "Effects", "dd-sirena", "Sirena", "Dembow siren: a rising-falling alarm tone with a wobble.",
  lambda: finish(reverb(stereo(siren(2.4, 231)), .6, .2, 231)))
S(DD, "Effects", "dd-vocal-ey", "Vocal 'Ey!'", "A short synthesized shout-like 'ey!' (formant model, not a recorded voice).",
  lambda: finish(reverb(stereo(formant_voice(.38, 240, ["e", "i"], 232)), .5, .18, 232)))
S(DD, "Effects", "dd-impact-calle", "Impacto Calle", "Hard impact: distorted boom, crash and sub.",
  lambda: finish(impact(233, sub=52, dirty=True)))
S(DD, "Effects", "dd-riser", "Subida", "Fast one-bar riser into the drop, 124 BPM.",
  lambda: finish(riser(1.95, 234, tone=True, top=10000), fi=.1, fo=.03))
S(DD, "Effects", "dd-rewind", "Rewind DJ", "DJ rewind: the track pulled back fast.", lambda: finish(stereo(spinback(235, longer=True), .2, 235, .3)))
LOOP(DD, "Textures", "dd-calle", "Calle Ambience", "A distant street at night: traffic rumble, far-off murmur, loopable.", lambda: finish(street(8.0, 241), rms_db=-30, peak_db=-6, tail=False, fi=.3, fo=.3))
LOOP(DD, "Textures", "dd-hum", "Bajo Hum", "Gritty low hum on A for dark dembow intros.", lambda: finish(drone(8.0, 33, 242, dark=True, grit=True), rms_db=-24, peak_db=-4, tail=False, fi=.4, fo=.4))
LOOP(DD, "Textures", "dd-playa", "Playa", "Waves on the shore: slow swells of noise, loopable.", lambda: finish(waves(8.0, 243), rms_db=-28, peak_db=-5, tail=False, fi=.3, fo=.3))
LOOP(DD, "Textures", "dd-radio", "Radio Lo-Fi", "A tiny old radio's crackle and tuning hiss.", lambda: finish(radio(8.0, 244), rms_db=-30, peak_db=-6, tail=False, fi=.05, fo=.05))
LOOP(DD, "Textures", "dd-fiesta-air", "Aire de Fiesta", "Bright, airy party ambience with a soft pulse at 122 BPM.", lambda: finish(party_air(7.87, 245), rms_db=-28, peak_db=-6, tail=False, fi=.2, fo=.2), bpm=122, beats=16)

# =========================================================================================== instrument models + FX
def tambora_palito(seed):
    n = int(.9 * SR)
    head = membrane(158, .16, .85, seed, pitch_drop=.08)                    # stick: bright upper modes
    stick = hp(noise(n, seed), 1800) * env_ad(n, .0001, .004) * .5            # wood on skin
    shell = modal(n, 760, (1, 2.6, 4.3), (.25, .12, .06), (.05, .03, .02), rng(seed))   # the shell answers
    return sat(head[:n] + stick + shell, 1.3)

def tambora_mano(seed):
    n = int(1.0 * SR)
    head = membrane(118, .22, .25, seed, pitch_drop=.05)                     # hand: fundamental-heavy, soft attack
    slap = bp(noise(n, seed), 300, 2500) * env_ad(n, .0012, .008) * .3
    return head[:n] * env_ad(n, .0015, 9) + slap

def tambora_tapao(seed):
    n = int(.35 * SR)
    head = membrane(132, .045, .55, seed, damp=.6, pitch_drop=.03)
    slap = bp(noise(n, seed), 800, 5000) * env_ad(n, .0005, .012) * .55
    return head[:n] + slap

def tambora_cascara(seed):
    n = int(.25 * SR)
    return wood(980, .025, seed, (1, 2.71, 4.4, 6.8), .7) [:n]

def tambora_roll(seed):
    total = .9; n = int(total * SR); out = np.zeros(n + int(.5 * SR))
    times = [total * (1 - (1 - k / 14) ** 1.6) for k in range(14)]
    for k, at in enumerate(times):
        hit = tambora_palito(seed + k) if k % 2 == 0 else tambora_tapao(seed + k) * .8
        place(out, hit * (.45 + .55 * k / 13), at)
    return out

def guira(dur, seed):
    n = int((dur + .15) * SR)
    x = scrape(n, 140 + 40 * rng(seed).random(), 3000, 12000, seed, .8)
    x = x * env_adsr(n, .004, dur * .5, .7, .04, dur)
    ring = modal(n, 4200, (1, 1.37, 1.91, 2.6), (.3, .2, .15, .1), (.08, .06, .05, .04), rng(seed)) * env_ad(n, .002, dur)
    return x + ring * .15

def guiro(seed):
    n = int(.45 * SR); x = np.zeros(n)
    a = scrape(int(.22 * SR), 60, 1500, 7000, seed, .9) * np.linspace(.6, 1, int(.22 * SR))
    b = scrape(int(.08 * SR), 75, 1500, 7000, seed + 1, .9)
    place(x, a, 0); place(x, b, .3)
    return x

def tambourine(seed):
    n = int(.45 * SR)
    jingles = sum(modal(n, f, (1, 1.48, 2.1), (.5, .3, .2), (.09, .06, .04), rng(seed + i)) for i, f in enumerate((6200, 7400, 8100, 9300, 10500)))
    shake = bp(noise(n, seed), 5000, 14000) * env_ad(n, .008, .06)
    skin = membrane(260, .04, .3, seed) [:n] * .3
    return jingles * env_ad(n, .002, .12) * .3 + shake * .6 + skin

def cowbell(f1, f2, decay, seed):
    n = int(.5 * SR); tt = np.arange(n) / SR
    x = np.sign(np.sin(2 * np.pi * f1 * tt)) + np.sign(np.sin(2 * np.pi * f2 * tt))
    return bp(x, f1 * .8, f2 * 3) * (env_ad(n, .0005, .012) * .6 + env_ad(n, .0005, decay) * .5)

def group_claps(seed):
    n = int(.6 * SR); out = np.zeros(n); r = rng(seed)
    for k in range(5):
        place(out, clap((0, 5 + r.integers(0, 4), 11 + r.integers(0, 4)), .06 + .03 * r.random(), seed + k, 800 + 300 * r.random(), 5500), .002 + .012 * r.random(), .6 + .3 * r.random())
    return out

def riser(dur, seed, tone=False, top=9000, wide=False):
    n = int(dur * SR); tt = np.arange(n) / SR; p = tt / dur
    nz = noise(n, seed)
    out = np.zeros(n); blocks = 64; L = n // blocks
    for k in range(blocks):
        a, b = k * L, n if k == blocks - 1 else (k + 1) * L
        f = 300 * (top / 300) ** (k / (blocks - 1))
        out[a:b] = bp(nz[max(0, a - 2048):b], f * .6, min(f * 1.6, 18000))[-(b - a):]
    out *= p ** 1.6
    if tone: out += np.sin(2 * np.pi * np.cumsum(110 * 2 ** (p * 3)) / SR) * p ** 2 * .25
    st = stereo(out, .5 if wide else .2, seed, .7)
    return reverb(st, 1.5, .25, seed)

def downlifter(dur, seed):
    n = int(dur * SR); p = np.arange(n) / n
    nz = noise(n, seed); out = np.zeros(n); blocks = 48; L = n // blocks
    for k in range(blocks):
        a, b = k * L, n if k == blocks - 1 else (k + 1) * L
        f = 12000 * (200 / 12000) ** (k / (blocks - 1))
        out[a:b] = bp(nz[max(0, a - 2048):b], f * .6, min(f * 1.6, 18000))[-(b - a):]
    out *= (1 - p) ** 1.3
    return reverb(stereo(out, .3, seed, .6), 1.8, .3, seed)

def impact(seed, sub=50, dirty=False):
    n = int(3.0 * SR)
    boom = sweep_sine(n, 180, sub, .06) * env_ad(n, .001, .6)
    crash = hp(noise(n, seed), 3000) * env_ad(n, .001, .7) * .35
    hit = kick(260, sub, .02, .3, 1, 3, seed)[:n] * .6
    x = boom + crash + np.pad(hit, (0, max(0, n - len(hit))))[:n]
    if dirty: x = sat(x, 3)
    return reverb(stereo(x, .3, seed, .4), 2.5, .3, seed)

def reverse_cymbal(dur, seed, bright=False):
    n = int(dur * SR)
    cym = hat(dur / 3.2, 370 if bright else 330, .5, seed, 4000 if bright else 3000, 16000, n_secs=dur)
    cym = reverb(stereo(cym[:n], .3, seed, .5), 1.2, .3, seed)[:n]
    return cym[::-1].copy()

def tape_stop():
    dur = 1.6; n = int(dur * SR); tt = np.arange(n) / SR
    speed = np.clip(1 - (tt / dur) ** 1.4, 0, 1)
    x = np.zeros(n)
    for m in (50, 60, 64, 65, 69):
        f = 440 * 2 ** ((m - 69) / 12)
        x += np.sin(2 * np.pi * np.cumsum(f * speed) / SR) * (1 + .3 * np.sin(2 * np.pi * np.cumsum(f * 2 * speed) / SR))
    return lp(x / 5, 4000) * speed ** .5

def spinback(seed, longer=False):
    dur = 1.1 if longer else .7; n = int(dur * SR); tt = np.arange(n) / SR
    rate = np.sin(np.pi * tt / dur) * (14 if longer else 10)
    src = lp(noise(n, seed), 3000) * .3 + np.sin(2 * np.pi * np.cumsum(180 * rate) / SR)
    return bp(src, 200, 6000) * np.sin(np.pi * tt / dur) ** .7

def whoosh(dur, seed):
    n = int(dur * SR); p = np.arange(n) / n
    x = bp(noise(n, seed), 400, 6000) * np.sin(np.pi * p) ** 2
    L, R = x * (1 - p), x * p
    return reverb(np.stack([L, R], 1), .8, .2, seed)

def siren(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR
    f = 700 + 500 * np.sin(2 * np.pi * .9 * tt) + 30 * np.sin(2 * np.pi * 7 * tt)
    x = np.sin(2 * np.pi * np.cumsum(f) / SR) + .3 * np.sign(np.sin(2 * np.pi * np.cumsum(f) / SR))
    return lp(x, 5000) * env_adsr(n, .05, .3, .9, .2, dur - .25) * .6

def lp_sweep(x, f0, f1):
    n = len(x); out = np.zeros(n); blocks = 64; L = n // blocks
    for k in range(blocks):
        a, b = k * L, n if k == blocks - 1 else (k + 1) * L
        f = f0 * (f1 / f0) ** (k / (blocks - 1))
        out[a:b] = lp(x[max(0, a - 2048):b], f, 4)[-(b - a):]
    return out

# --- textures (loopable beds: their own fades are short; they start and end quietly)
def vinyl(dur, seed, pops=1.0):
    n = int(dur * SR); r = rng(seed)
    hiss = lp(hp(noise(n, seed), 600), 6000) * .06
    clicks = np.zeros(n); idx = r.integers(0, n, int(dur * 9 * pops)); clicks[idx] = r.standard_normal(len(idx)) * (r.random(len(idx)) ** 3) * 2
    clicks = bp(clicks, 800, 8000)
    rumble = lp(noise(n, seed + 1), 60) * .2
    return stereo(hiss + clicks + rumble, .1, seed, .2)

def rain(dur, seed, heavy=False):
    n = int(dur * SR); r = rng(seed)
    bed = lp(hp(noise(n, seed), 400), 9000) * (.25 if heavy else .12)
    drops = np.zeros(n); idx = r.integers(0, n, int(dur * (900 if heavy else 400))); drops[idx] = r.random(len(idx)) ** 2
    drops = bp(np.convolve(drops, np.exp(-np.arange(200) / 25), mode="same"), 1500, 9000)
    x = bed + drops * .5
    return reverb(stereo(x, .4, seed, .9), 1.0, .3, seed)

def grains(dur, seed, notes, dens=6, short=False):
    n = int(dur * SR); out = np.zeros((n, 2)); r = rng(seed)
    for k in range(int(dur * dens)):
        m = r.choice(notes); f = 440 * 2 ** ((m - 69) / 12)
        g = modal(int(.9 * SR), f, (1, 2.01, 3.02), (1, .3, .1), (.12 if short else .4, .06, .03), r) * env_ad(int(.9 * SR), .01 if not short else .001, 9)
        pan = r.uniform(-.8, .8)
        place(out, np.stack([g * (1 - pan) / 2, g * (1 + pan) / 2], 1), r.uniform(0, dur - .3), .3 + .4 * r.random())
    return reverb(out, 3.0, .45, seed)

def tape_air(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR
    x = lp(hp(noise(n, seed), 2000), 12000) * (1 + .15 * np.sin(2 * np.pi * .7 * tt))
    return stereo(x * .1, .6, seed, .5)

def drone(dur, midi, seed, dark=False, grit=False):
    n = int(dur * SR); tt = np.arange(n) / SR; f = 440 * 2 ** ((midi - 69) / 12)
    x = sum(np.sin(2 * np.pi * f * h * (1 + d) * tt) / h for h in (1, 2, 3, 4) for d in (-.002, .002))
    x = lp(x, 900 if dark else 2500)
    if grit: x = sat(x * 1.5, 2) + lp(noise(n, seed), 300) * .05
    x *= 1 + .2 * np.sin(2 * np.pi * .15 * tt)
    return stereo(x, .3, seed, 1.2)

def shimmer(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR; x = np.zeros(n); r = rng(seed)
    for m in (88, 93, 95, 100, 105):
        f = 440 * 2 ** ((m - 69) / 12)
        x += np.sin(2 * np.pi * f * tt + r.uniform(0, 6)) * (.5 + .5 * np.sin(2 * np.pi * r.uniform(.1, .4) * tt + r.uniform(0, 6)))
    x = x * .08 + lp(hp(noise(n, seed), 6000), 14000) * .05
    return reverb(stereo(x, .6, seed, 1.0), 3.0, .4, seed)

def analog_drone(dur, midi, seed):
    n = int(dur * SR); tt = np.arange(n) / SR; f = 440 * 2 ** ((midi - 69) / 12); x = np.zeros(n)
    for d in (-.004, 0, .004):
        x += 2 * np.mod(f * (1 + d) * tt, 1) - 1
    x = lp(x / 3, 1200)
    return reverb(stereo(x, .5, seed, 7), 2.0, .3, seed)

def neon(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR
    hum = sum(np.sin(2 * np.pi * 60 * h * tt) / h for h in (1, 2, 3, 5)) * .3
    buzz = bp(noise(n, seed), 2000, 8000) * .08
    flick = np.repeat(rng(seed).random(int(dur * 20)) > .93, int(SR / 20))[:n] * .4 + .6
    return stereo((hum + buzz) * np.pad(flick, (0, n - len(flick)), constant_values=1), .2, seed, .3)

def club_room(dur, seed):
    n = int(dur * SR); out = np.zeros(n); beat = 60 / 124
    k = lp(kick(160, 50, .02, .25, .2, 2, seed), 300)
    for b in range(int(dur / beat) + 1): place(out, k * .6, b * beat)
    air = lp(noise(n, seed), 1500) * .05
    return reverb(stereo(lp(out, 400) + air, .3, seed, 1), 1.8, .5, seed)

def night(dur, seed):
    n = int(dur * SR); r = rng(seed); out = np.zeros(n)
    air = lp(noise(n, seed), 2000) * .04
    for k in range(int(dur * 3)):
        f = r.uniform(3500, 5500); m = int(.06 * SR)
        ch = np.sin(2 * np.pi * f * np.arange(m) / SR) * env_ad(m, .005, .015) * (np.sin(2 * np.pi * 45 * np.arange(m) / SR) > 0)
        place(out, ch * .15, r.uniform(0, dur - .1))
    return reverb(stereo(out + air, .5, seed, .8), 1.5, .3, seed)

def glitch(dur, seed, bpm):
    n = int(dur * SR); out = np.zeros(n); r = rng(seed); step = 60 / bpm / 4
    src = bp(noise(int(.2 * SR), seed), 300, 9000)
    tone = np.sin(2 * np.pi * 220 * np.arange(int(.2 * SR)) / SR)
    for k in range(int(dur / step)):
        if r.random() < .45:
            piece = (src if r.random() < .5 else tone)[: int(step * SR * r.choice([.25, .5, 1]))]
            piece = np.round(piece * r.choice([4, 8, 16])) / r.choice([4, 8, 16])
            place(out, piece * env_ad(len(piece), .001, .05) * .5, k * step)
    return stereo(out, .4, seed, .4)

def street(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR
    traffic = lp(noise(n, seed), 250) * (.5 + .3 * np.sin(2 * np.pi * .07 * tt)) * .3
    murmur = bp(noise(n, seed + 1), 300, 1200) * (.5 + .5 * np.sin(2 * np.pi * 2.3 * tt) * np.sin(2 * np.pi * .3 * tt)) * .05
    horn = np.zeros(n); place(horn, np.sin(2 * np.pi * 420 * np.arange(int(.4 * SR)) / SR) * env_adsr(int(.4 * SR), .02, .1, .8, .05, .3) * .03, 5.1)
    return reverb(stereo(traffic + murmur + lp(horn, 2000), .4, seed, 1), 2.2, .35, seed)

def waves(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR
    swell = (np.sin(2 * np.pi * tt / 4) * .5 + .5) ** 2
    nz = noise(n, seed)
    x = (lp(nz, 600) * (1 - swell) + lp(nz, 3600) * swell) * (.15 + .5 * swell)
    return stereo(x, .5, seed, 1.5)

def radio(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR
    hiss = bp(noise(n, seed), 600, 4000) * .08
    tune = np.sin(2 * np.pi * np.cumsum(900 + 300 * np.sin(2 * np.pi * .2 * tt)) / SR) * .02 * (np.sin(2 * np.pi * .13 * tt) > .7)
    return stereo(bp(vinyl(dur, seed, 2)[:, 0] * .5 + hiss + tune, 400, 4500), .0)

def party_air(dur, seed):
    n = int(dur * SR); tt = np.arange(n) / SR; beat = 60 / 122
    pulse = (1 + np.cos(2 * np.pi * tt / beat)) / 2
    x = lp(hp(noise(n, seed), 1500), 9000) * (.05 + .05 * pulse)
    return reverb(stereo(x, .5, seed, .8), 1.5, .3, seed)

# =========================================================================================== render
def write_flac(path, x):
    tmp = path.replace(".flac", ".tmp.wav")
    with wave.open(tmp, "wb") as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype("<i2").tobytes())
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", tmp, "-compression_level", "8", path], check=True)
    os.remove(tmp)

def qa(x):
    pk = float(np.abs(x).max()); rms = float(np.sqrt(np.mean(x ** 2)))
    d = np.abs(np.diff(x, axis=0)).max()
    dc = float(np.abs(x.mean(axis=0)).max())
    first, last = float(np.abs(x[:2]).max()), float(np.abs(x[-16:]).max())   # starts from silence, ends in silence (no clicks)
    return {"peak_db": round(float(20 * np.log10(pk + 1e-12)), 2), "rms_db": round(float(20 * np.log10(rms + 1e-12)), 2), "max_step": round(float(d), 3),
            "dc": round(dc, 5), "start": round(first, 4), "end": round(last, 4), "clipped": int((np.abs(x) > .999).sum())}

def main():
    os.makedirs(OUT, exist_ok=True)
    only = sys.argv[1] if len(sys.argv) > 1 else ""
    manifest = {"libraries": ["New Jazz", "Disco + EDM + Club", "Reggaetón", "Dembow Dominicano"],
                "categories": ["Drums", "Hi-Hats", "Percussion", "Effects", "Textures"], "items": []}
    problems = []
    for it in ITEMS:
        doc = {k: v for k, v in it.items() if k != "fn" and v is not None}
        if it["kind"] in ("oneshot", "loop"):
            path = os.path.join(OUT, it["id"] + ".flac")
            if not only or only in it["id"] or not os.path.exists(path):
                x = it["fn"]()
                q = qa(x)
                if q["clipped"] or q["dc"] > .002 or q["start"] > .01 or q["end"] > .005: problems.append((it["id"], q))
                write_flac(path, x)
                doc["qa"] = q; doc["seconds"] = round(len(x) / SR, 3)
            else:
                prev = next((p for p in json.load(open(os.path.join(OUT, "manifest.json")))["items"] if p["id"] == it["id"]), {}) if os.path.exists(os.path.join(OUT, "manifest.json")) else {}
                doc["qa"] = prev.get("qa"); doc["seconds"] = prev.get("seconds")
            doc["file"] = it["id"] + ".flac"
        manifest["items"].append(doc)
        print(f"{it['lib'][:10]:<10} {it['cat']:<10} {it['kind']:<8} {it['name']:<24} {doc.get('qa', '')}")
    # checks: every pattern points at existing one-shots; every library/category has at least five sounds
    ids = {i["id"] for i in manifest["items"] if i["kind"] != "pattern"}
    for i in manifest["items"]:
        if i["kind"] == "pattern":
            missing = {e["s"] for e in i["events"]} - ids
            assert not missing, (i["id"], missing)
    from collections import Counter
    c = Counter((i["lib"], i["cat"]) for i in manifest["items"] if not i.get("hidden"))
    for lib in manifest["libraries"]:
        for cat in manifest["categories"]:
            assert c[(lib, cat)] >= 5, (lib, cat, c[(lib, cat)])
    json.dump(manifest, open(os.path.join(OUT, "manifest.json"), "w"), indent=1, ensure_ascii=False)
    print("\n", len(manifest["items"]), "items;", dict(Counter(i["kind"] for i in manifest["items"])))
    print("QA problems:", problems or "none")

if __name__ == "__main__":
    main()

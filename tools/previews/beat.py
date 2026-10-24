"""Synthesized demo drum beds so presets can be auditioned in a record context.

These drums exist only to give the previews a groove. They are original synthesis, simple by design,
and are NOT the plugin's drum library (that needs its own sampled/recorded design pass).
"""
import numpy as np
from scipy.signal import butter, sosfilt

SR = 44100
rng = np.random.default_rng(5)

def _env(n, decay_s, attack_s=0.0005):
    t = np.arange(n) / SR
    a = np.clip(t / attack_s, 0, 1)
    return a * np.exp(-t / decay_s)

def _bp(x, lo, hi):
    return sosfilt(butter(2, [lo, hi], btype="band", fs=SR, output="sos"), x)

def _hp(x, f):
    return sosfilt(butter(2, f, btype="high", fs=SR, output="sos"), x)

def _lp(x, f):
    return sosfilt(butter(2, f, btype="low", fs=SR, output="sos"), x)

def kick(vel=1.0, punch=1.0):
    n = int(.45 * SR); t = np.arange(n) / SR
    f = 46 + 110 * np.exp(-t / .035)
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * _env(n, .22)
    click = _hp(rng.standard_normal(n), 2500) * _env(n, .004) * .35 * punch
    x = np.tanh((body + click) * 2.2) * vel
    return np.stack([x, x], 1)

def clap(vel=1.0):
    n = int(.35 * SR); x = np.zeros(n); noise = rng.standard_normal(n)
    for off in (0, .009, .019):
        o = int(off * SR); m = n - o
        x[o:] += noise[:m] * _env(m, .008) * .7
    x += noise * _env(n, .11) * .45
    x = _bp(x, 900, 5200)
    snare_tone = np.sin(2 * np.pi * 185 * np.arange(n) / SR) * _env(n, .05) * .35
    y = np.tanh((x + snare_tone) * 1.6) * vel
    L = y; R = np.concatenate([np.zeros(12), y[:-12]])   # tiny spread
    return np.stack([L, R], 1)

def rim(vel=1.0):
    n = int(.08 * SR); t = np.arange(n) / SR
    x = (np.sin(2 * np.pi * 820 * t) * .6 + _bp(rng.standard_normal(n), 1500, 4500) * .6) * _env(n, .012)
    x = np.tanh(x * 2) * vel
    return np.stack([x * .9, x], 1)

_hat_f = [205.3, 304.4, 369.6, 522.7, 540.0, 800.0]
def hat(vel=1.0, open_=False):
    n = int((.35 if open_ else .06) * SR); t = np.arange(n) / SR
    x = sum(np.sign(np.sin(2 * np.pi * f * 1.9 * t + i)) for i, f in enumerate(_hat_f))
    x = _lp(_hp(_bp(x, 6000, 14000), 7000), 12500) * _env(n, .12 if open_ else .022) * .33 * vel
    pan = rng.uniform(-.15, .15)
    return np.stack([x * (1 - pan), x * (1 + pan)], 1)

def shaker(vel=1.0):
    n = int(.07 * SR)
    x = _bp(rng.standard_normal(n), 4500, 11000) * _env(n, .02, .008) * .5 * vel
    return np.stack([x * .8, x], 1)

def sub808(note, dur_s, vel=1.0):
    n = int(dur_s * SR); t = np.arange(n) / SR
    f0 = 440 * 2 ** ((note - 69) / 12)
    f = f0 * (1 + 1.2 * np.exp(-t / .02))
    x = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.minimum(1, np.exp(-(t - .05).clip(0) / 1.4))
    rel = min(n, int(.03 * SR)); x[-rel:] *= np.linspace(1, 0, rel)
    x = np.tanh(x * 1.8) * .8 * vel
    return np.stack([x, x], 1)

# patterns: list of (beat, instrument, velocity) for one 4-beat bar
def _pattern(groove, bar):
    p = []
    last = bar == 3
    if groove == "trapsoul":
        p += [(0, "kick", 1), (1.75, "kick", .8), (2.5, "kick", .85)]
        p += [(1, "clap", .9), (3, "clap", .95)]
        for i in range(8): p.append((i * .5, "hat", .5 if i % 2 else .62))
        if last: p += [(3.5, "hat", .45), (3.625, "hat", .4), (3.75, "hat", .45), (3.875, "hat", .5)]
    elif groove in ("trap", "drill"):
        if groove == "trap":
            p += [(0, "kick", 1), (1.5, "kick", .85), (3.25, "kick", .7)]
            p += [(2, "clap", 1)]
            for i in range(8): p.append((i * .5, "hat", .55 if i % 2 else .68))
            if bar % 2 == 1: p += [(3.25, "hat", .45), (3.375, "hat", .45), (3.75, "hat", .5), (3.833, "hat", .45), (3.917, "hat", .5)]
        else:
            p += [(0, "kick", 1), (2.75, "kick", .8)]
            p += [(2, "clap", 1), (3.75 if bar % 2 else 3.5, "clap", .6)]
            for b in [0, .5, 1, 1.333, 1.667, 2, 2.5, 3, 3.5]: p.append((b, "hat", .6))
    elif groove in ("reggaeton", "dembow"):
        for b in range(4): p.append((b, "kick", 1 if b % 2 == 0 else .9))
        for b in (.75, 1.5, 2.75, 3.5): p.append((b, "clap" if groove == "reggaeton" else "rim", .8))
        if groove == "dembow":
            for b in (1, 3): p.append((b, "clap", .7))
            for i in range(16): p.append((i * .25, "hat", .55 if i % 2 else .42))
            p.append((3.5, "openhat", .5))
        else:
            for i in range(8): p.append((i * .5, "hat", .5))
            for i in range(16): p.append((i * .25, "shaker", .35 + .15 * (i % 2)))
    elif groove == "dancehall":
        p += [(0, "kick", 1), (.75, "kick", .85), (2, "kick", .95), (2.75, "kick", .8)]
        p += [(1, "clap", .85), (3, "clap", .9), (1.5, "rim", .5), (3.5, "rim", .55)]
        for i in range(8): p.append((i * .5, "hat", .45 if i % 2 == 0 else .6))
    elif groove == "bounce":
        for b in range(4): p.append((b, "kick", 1))
        p += [(1, "clap", .85), (3, "clap", .9)]
        for b in range(4): p.append((b + .5, "openhat", .45))
        for i in range(16):
            if i % 4: p.append((i * .25, "shaker", .4 if i % 2 else .3))
    elif groove == "disco":
        for b in range(4): p.append((b, "kick", 1))
        p += [(1, "clap", .85), (3, "clap", .9)]
        for b in range(4): p.append((b + .5, "openhat", .5))
        for i in range(16): p.append((i * .25, "shaker", .34 if i % 2 else .26))
    elif groove == "newjazz":
        p += [(0, "kick", .95), (1.75, "kick", .7), (2.5, "kick", .8)]
        p += [(1, "clap", .7), (3, "clap", .75), (3.75, "rim", .4)]
        for i in range(8): p.append((i * .5, "hat", .38 if i % 2 else .5))
    elif groove == "phonk":
        p += [(0, "kick", 1), (.75, "kick", .7), (2, "kick", .9), (2.5, "kick", .7)]
        p += [(1, "clap", .95), (3, "clap", .95)]
        for i in range(8): p.append((i * .5, "hat", .6))
        p.append((3.5, "openhat", .5))
    return p

_INST = {"kick": kick, "clap": clap, "rim": rim, "hat": hat, "shaker": shaker,
         "openhat": lambda v: hat(v, True)}

def _swing(b, amount):
    six = round(b * 4, 6)
    if amount and abs(six - round(six)) < 1e-6 and int(round(six)) % 2 == 1:
        return b + amount * .25
    return b

def render(groove, bpm, n_samples, roots, swing=0.0, bars=4):
    out = np.zeros((n_samples, 2))
    use808 = not groove.endswith("_no808")
    g = groove.replace("_no808", "")
    spb = 60 / bpm
    def put(x, t):
        i = int(t * SR)
        if i >= n_samples: return
        m = min(len(x), n_samples - i); out[i:i + m] += x[:m]
    for bar in range(bars):
        for b, inst, v in _pattern(g, bar):
            t = (bar * 4 + _swing(b, swing)) * spb + rng.normal(0, .002)
            put(_INST[inst](v * rng.uniform(.88, 1.0)), max(0, t))
    if use808 and roots:
        r = sorted(roots)
        for i, (b, note) in enumerate(r):
            end = r[i + 1][0] if i + 1 < len(r) else bars * 4
            put(sub808(note, (end - b) * spb, .9), b * spb)
    return out

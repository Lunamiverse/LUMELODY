#!/usr/bin/env python3
"""Write preset JSON files, render audition phrases through the C++ engine, check and loudness-match them.

usage: python3 tools/previews/build_previews.py [name-filter]
outputs: presets/*.json, previews/wav/*.wav, previews/mp3/*.mp3, previews/report.json
"""
import json, os, re, subprocess, sys, wave
import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.dirname(__file__))
from preset_defs import PRESETS  # noqa: E402
from phrases import build  # noqa: E402
import beat  # noqa: E402

RENDER = os.path.join(ROOT, "build", "render_preset")
TARGET_LUFS = -16.0
TP_CEIL = -1.0

def slug(s):
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")

def flatten(p):
    flat = {}
    for i, layer in enumerate(p["layers"]):
        for k, v in layer.items():
            flat[f"l{i+1}_{k}"] = v
    flat.update(p["voice"])
    flat.update(p["fx"])
    return flat

def read_wav(path):
    with wave.open(path, "rb") as w:
        n, sr, sw = w.getnframes(), w.getframerate(), w.getsampwidth()
        if sw == 2:
            return np.frombuffer(w.readframes(n), dtype="<i2").astype(np.float32).reshape(-1, 2) / 32768.0, sr
        raw = np.frombuffer(w.readframes(n), dtype=np.uint8).reshape(-1, 3)
    v = (raw[:, 0].astype(np.int32) | (raw[:, 1].astype(np.int32) << 8) | (raw[:, 2].astype(np.int32) << 16))
    v = np.where(v >= 1 << 23, v - (1 << 24), v).astype(np.float32) / 8388608.0
    return v.reshape(-1, 2), sr

def analyze(x, sr):
    mono = x.mean(axis=1)
    spec = np.abs(np.fft.rfft(mono * np.hanning(len(mono))))
    f = np.fft.rfftfreq(len(mono), 1 / sr)
    p = spec ** 2 + 1e-20
    tot = p.sum()
    band = lambda a, b: float(p[(f >= a) & (f < b)].sum() / tot)
    side = (x[:, 0] - x[:, 1]) * .5
    rms = lambda s: float(np.sqrt(np.mean(s ** 2)) + 1e-12)
    # click detector: isolated sample-level spikes in the 2nd difference, far above the surrounding 10 ms
    d2 = np.abs(np.diff(mono, 2))
    w = int(sr * .01)
    loc = np.sqrt(np.convolve(d2 ** 2, np.ones(w) / w, mode="same")) + 1e-9
    clicks = int(np.sum((d2 > loc * 9) & (d2 > 2e-3)))
    return {
        "centroid_hz": round(float((f * p).sum() / tot)),
        "sub_<60": round(band(0, 60), 3), "low_60-250": round(band(60, 250), 3), "mid_250-2k": round(band(250, 2000), 3),
        "presence_2k-6k": round(band(2000, 6000), 3), "air_>6k": round(band(6000, 22050), 3),
        "stereo_side_ratio": round(rms(side) / rms(mono), 3), "dc": round(float(abs(mono.mean())), 5), "clicks": clicks,
    }

def peak_limit(y, sr, ceil):
    """Offline look-ahead peak limiter (transparent on transients: 2 ms window, smooth gain)."""
    from scipy.ndimage import minimum_filter1d, uniform_filter1d
    peak = np.abs(y).max(axis=1)
    g = np.minimum(1.0, ceil / np.maximum(peak, 1e-9))
    w = max(8, int(sr * .002))
    g = uniform_filter1d(minimum_filter1d(g, 2 * w + 1), w)
    g = np.minimum(g, np.minimum(1.0, ceil / np.maximum(peak, 1e-9)))
    return y * g[:, None], round(float(-20 * np.log10(g.min())), 2)

def loudness(path):
    r = subprocess.run(["ffmpeg", "-hide_banner", "-nostats", "-i", path, "-af",
                        f"loudnorm=I={TARGET_LUFS}:TP={TP_CEIL}:LRA=11:print_format=json", "-f", "null", "-"],
                       capture_output=True, text=True)
    m = re.search(r"\{[^{}]*\"input_i\"[^{}]*\}", r.stderr, re.S)
    return json.loads(m.group(0))

def main():
    flt = sys.argv[1].lower() if len(sys.argv) > 1 else ""
    phrases = build()
    for d in ("presets", "previews/wav", "previews/mp3", "build/tmp"):
        os.makedirs(os.path.join(ROOT, d), exist_ok=True)
    report_path = os.path.join(ROOT, "previews", "report.json")
    report = json.load(open(report_path)) if os.path.exists(report_path) else {}
    if not flt:   # full rebuild: drop files from presets that no longer exist
        for d in ("presets", "previews/mp3", "previews/wav"):
            for f in os.listdir(os.path.join(ROOT, d)):
                os.remove(os.path.join(ROOT, d, f))
        report = {}
    names = [p["name"] for p in PRESETS]
    assert len(names) == len(set(names)), "duplicate preset names"

    for p in PRESETS:
        s = slug(p["name"])
        if flt and flt not in s:
            continue
        ph = phrases[p["preview_phrase"]]
        flat = flatten(p)
        tmp = os.path.join(ROOT, "build", "tmp")
        pf, nf = os.path.join(tmp, s + ".params"), os.path.join(tmp, s + ".notes")
        with open(pf, "w") as f:
            for k, v in flat.items():
                f.write(f"{k} {float(v)}\n")
        spb = 60.0 / ph["bpm"]
        with open(nf, "w") as f:
            for st, du, n, vel in ph["notes"]:
                f.write(f"{st*spb:.5f} {max(du,0.05)*spb:.5f} {n} {vel:.3f}\n")
        raw = os.path.join(tmp, s + ".raw.wav")
        out = subprocess.run([RENDER, pf, nf, raw, str(ph["bpm"]), "3.5"], capture_output=True, text=True, check=True).stdout.strip()
        x, sr = read_wav(raw)
        # trim trailing silence
        env = np.abs(x).max(axis=1)
        last = np.nonzero(env > 10 ** (-70 / 20))[0]
        if len(last):
            x = x[: min(len(x), last[-1] + int(.2 * sr))]
        stats = analyze(x, sr)
        ln = loudness(raw)
        gain_db = TARGET_LUFS - float(ln["input_i"])
        tp = float(ln["input_tp"]) + gain_db
        limit_db = 0.0
        if tp > TP_CEIL + 4.0:      # very peaky sounds: limit at most 4 dB, then back off the rest
            gain_db -= tp - (TP_CEIL + 4.0)
        y = x * (10 ** (gain_db / 20))
        y, limit_db = peak_limit(y, sr, 10 ** ((TP_CEIL - 0.3) / 20))
        fade = int(.05 * sr)
        y[-fade:] *= np.linspace(1, 0, fade)[:, None]
        wav_out = os.path.join(ROOT, "previews", "wav", s + ".wav")
        with wave.open(wav_out, "wb") as w:
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(sr)
            w.writeframes((np.clip(y, -1, 1) * 32767).astype("<i2").tobytes())
        mp3 = os.path.join(ROOT, "previews", "mp3", s + ".mp3")
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", wav_out, "-codec:a", "libmp3lame", "-b:a", "192k", mp3], check=True)
        # in-context version: same preview over a demo drum bed (+808 unless the preset is the bass)
        bed = beat.render(ph["groove"], ph["bpm"], len(y), ph["roots"], ph.get("swing", 0.0))
        rms = lambda a: float(np.sqrt(np.mean(a ** 2)) + 1e-9)
        bed *= (rms(y) * 10 ** (-2.5 / 20)) / rms(bed)
        mix, _ = peak_limit(y + bed, sr, 10 ** ((TP_CEIL - 0.3) / 20))
        bwav = os.path.join(tmp, s + ".beat.wav")
        with wave.open(bwav, "wb") as w:
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(sr)
            w.writeframes((np.clip(mix, -1, 1) * 32767).astype("<i2").tobytes())
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", bwav, "-codec:a", "libmp3lame", "-b:a", "160k",
                        os.path.join(ROOT, "previews", "mp3", s + "-beat.mp3")], check=True)
        # store the measured trim inside the preset so the plugin plays every preset at a matched level
        doc = dict(p)
        doc["fx"] = dict(p["fx"])
        doc["loudness_trim_db"] = round(gain_db, 2)
        doc["id"] = s
        with open(os.path.join(ROOT, "presets", s + ".json"), "w") as f:
            json.dump(doc, f, indent=2)
        # calibrated playing level for the plugin/web engine (about -18 LUFS on the demo phrase, headroom for effects)
        pk = float(re.search(r"peak=([\d.]+)", out).group(1))
        live = gain_db + (20 * np.log10(0.98 / pk) if pk > 0.98 else 0) - 2.0
        open(os.path.join(tmp, s + ".gain"), "w").write(f"{live:.2f}\n")
        doc["out_gain_db"] = round(float(live), 2)
        with open(os.path.join(ROOT, "presets", s + ".json"), "w") as f:
            json.dump(doc, f, indent=2)
        report[s] = {"name": p["name"], "engine": out, "input_lufs": float(ln["input_i"]), "trim_db": round(gain_db, 2),
                     "final_tp": round(float(ln["input_tp"]) + gain_db, 2), "preview_limit_db": limit_db, "secs": round(len(y) / sr, 2), **stats}
        print(f"{p['name']:<18} {out:<30} lufs={float(ln['input_i']):6.1f} trim={gain_db:+5.1f} cen={stats['centroid_hz']:5d} "
              f"sub={stats['sub_<60']:.2f} air={stats['air_>6k']:.3f} side={stats['stereo_side_ratio']:.2f} clicks={stats['clicks']}")
    json.dump(report, open(report_path, "w"), indent=1)

if __name__ == "__main__":
    main()

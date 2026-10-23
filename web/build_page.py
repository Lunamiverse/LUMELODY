#!/usr/bin/env python3
"""Assemble the playable LUMI web instrument: page + wasm engine + worklet + demo audio, into an output folder."""
import json, os, re, shutil, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "previews"))
from preset_defs import PRESETS  # noqa: E402
from build_previews import flatten, slug  # noqa: E402

out = sys.argv[1]
os.makedirs(os.path.join(out, "audio"), exist_ok=True)
report = json.load(open(os.path.join(ROOT, "previews", "report.json")))
LT = {1: "Analog", 2: "FM", 3: "String model", 4: "Modal mallet", 5: "Noise", 6: "Formant voice", 7: "Grain cloud"}
data = []
for p in PRESETS:
    s = slug(p["name"]); r = report[s]
    peak = float(re.search(r"peak=([\d.]+)", r["engine"]).group(1))
    # trim was measured on the renderer's output, which was scaled down when the raw engine peaked above 0.98
    gain = r["trim_db"] + (20 * __import__("math").log10(0.98 / peak) if peak > 0.98 else 0) - 2.0   # engine applies it
    lo, hi = p["range"]
    base = max(12, 12 * ((((lo + hi) // 2) - 12) // 12))
    flat = flatten(p); flat["out_gain_db"] = round(gain, 2)
    text = "\n".join(f"{k} {float(v)}" for k, v in flat.items())
    data.append({"id": s, "name": p["name"], "cat": p["category"], "genres": p["genres"], "desc": p["description"],
                 "use": p["use"], "range": p["range"], "engine": " + ".join(dict.fromkeys(LT[l["type"]] for l in p["layers"])),
                 "params": text, "gainDb": 0, "baseNote": base})
    for suf in ("", "-beat"):
        shutil.copy(os.path.join(ROOT, "previews", "mp3", s + suf + ".mp3"), os.path.join(out, "audio", s + suf + ".mp3"))
html = open(os.path.join(ROOT, "web", "instrument_template.html")).read().replace("__PRESETS__", json.dumps(data))
open(os.path.join(out, "index.html"), "w").write(html)
shutil.copy(os.path.join(ROOT, "build", "lumi.wasm"), os.path.join(out, "lumi.wasm"))
shutil.copy(os.path.join(ROOT, "web", "lumi-worklet.js"), os.path.join(out, "lumi-worklet.js"))
print(len(data), "presets")

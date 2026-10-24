#!/usr/bin/env python3
"""Assemble the playable LUMI web instrument: page + wasm engine + worklet + demo audio, into an output folder."""
import json, os, re, shutil, sys
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "previews"))
from preset_defs import PRESETS  # noqa: E402
from build_previews import flatten, slug  # noqa: E402

LT = {1: "Analog", 2: "FM", 3: "String model", 4: "Modal mallet", 5: "Noise", 6: "Formant voice", 7: "Grain cloud"}
def preset_data():
    """Every sound as the page and the plugin use it: flattened engine parameters with the calibrated level."""
    report = json.load(open(os.path.join(ROOT, "previews", "report.json")))
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
                     "params": text, "gainDb": 0, "baseNote": base, **({"kind": p["kind"]} if p.get("kind") else {})})
    return data

def template():
    """The instrument page source with the LUMI tools (Chord Progression Finder, library, export) inlined."""
    html = open(os.path.join(ROOT, "web", "instrument_template.html")).read()
    assert "/*__PROGRESSIONS__*/" in html
    tools = "\n".join(open(os.path.join(ROOT, "web", f)).read() for f in ("tools_core.js", "tools_ui.js", "tools_pads.js", "tools_sampler.js", "tools_libs.js", "tools_arp.js", "tools_suggest.js"))
    manifest = open(os.path.join(ROOT, "web", "samples", "manifest.json")).read()
    m = json.loads(manifest)
    for it in m["items"]:   # the page serves the samples as WAV (lossless: the sources are 16-bit); the plugin embeds the FLAC files
        if it.get("file"): it["file"] = it["file"].replace(".flac", ".wav")
    tools = "const LIB_MANIFEST = " + json.dumps(m, separators=(",", ":")) + ";\n" + tools
    return html.replace("/*__PROGRESSIONS__*/", tools)

def schema_json():
    import subprocess
    schema = subprocess.run([os.path.join(ROOT, "build", "schema_dump")], capture_output=True, text=True, check=True).stdout.strip()
    json.loads(schema)
    return schema

if __name__ == "__main__":
    out = sys.argv[1]
    os.makedirs(os.path.join(out, "audio"), exist_ok=True)
    data = preset_data()
    for d in data:
        for suf in ("", "-beat"):
            shutil.copy(os.path.join(ROOT, "previews", "mp3", d["id"] + suf + ".mp3"), os.path.join(out, "audio", d["id"] + suf + ".mp3"))
    schema = schema_json()
    html = template().replace("__PRESETS__", json.dumps(data)).replace("__SCHEMA__", schema)
    open(os.path.join(out, "index.html"), "w").write(html)
    shutil.copy(os.path.join(ROOT, "build", "lumi.wasm"), os.path.join(out, "lumi.wasm"))
    shutil.copy(os.path.join(ROOT, "web", "lumi-worklet.js"), os.path.join(out, "lumi-worklet.js"))
    os.makedirs(os.path.join(out, "samples"), exist_ok=True)
    for f in os.listdir(os.path.join(ROOT, "web", "samples")):
        if f.endswith(".flac"):
            import subprocess
            subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", os.path.join(ROOT, "web", "samples", f), "-c:a", "pcm_s16le", os.path.join(out, "samples", f[:-5] + ".wav")], check=True)
    print(len(data), "presets")

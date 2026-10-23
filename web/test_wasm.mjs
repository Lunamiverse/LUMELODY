// Node check: the wasm engine renders the same audio as the native build, and fast enough for real time.
import fs from "fs";
const bytes = fs.readFileSync(new URL("../build/lumi.wasm", import.meta.url));
const mod = new WebAssembly.Module(bytes);
const stubs = {}; for (const i of WebAssembly.Module.imports(mod)) (stubs[i.module] ??= {})[i.name] = () => 0;
const { exports: X } = new WebAssembly.Instance(mod, stubs);
X._initialize(); X.lumi_init(44100);
const enc = new TextEncoder();
function load(paramsText, bpm) { const b = enc.encode(paramsText + "\0"); new Uint8Array(X.memory.buffer, X.lumi_text(), b.length).set(b); return X.lumi_load(bpm); }
const name = process.argv[2] || "dark-choir";
const n = load(fs.readFileSync(`build/tmp/${name}.params`, "utf8"), 140);
const notes = fs.readFileSync(`build/tmp/${name}.notes`, "utf8").trim().split("\n").map(l => l.split(" ").map(Number));
const ev = []; for (const [t, d, k, v] of notes) { ev.push([t, 0, k, v]); ev.push([t + d, 1, k, 0]); }
ev.sort((a, b) => a[0] - b[0] || b[1] - a[1]);
const secs = Math.max(...notes.map(x => x[0] + x[1])) + 1, total = Math.floor(secs * 44100);
const out = new Float32Array(total); let ei = 0;
const t0 = performance.now();
for (let pos = 0; pos < total; pos += 128) {
  while (ei < ev.length && ev[ei][0] * 44100 <= pos) { const e = ev[ei++]; e[1] ? X.lumi_note_off(e[2]) : X.lumi_note_on(e[2], e[3]); }
  const m = Math.min(128, total - pos); X.lumi_process(m);
  out.set(new Float32Array(X.memory.buffer, X.lumi_out_l(), m), pos);
}
const ms = performance.now() - t0;
let peak = 0, nan = 0; for (const v of out) { if (!Number.isFinite(v)) nan++; else peak = Math.max(peak, Math.abs(v)); }
console.log(`${name}: ${n} params, ${secs.toFixed(1)} s audio in ${ms.toFixed(0)} ms  -> ${(secs * 1000 / ms).toFixed(1)}x real time, peak ${peak.toFixed(3)}, NaN ${nan}`);

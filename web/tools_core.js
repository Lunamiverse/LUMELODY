// ================================================================ LUMI tools: harmony, player, universal export
// Shared by the Chord Progression Finder and the progression library (and later melody / bass / arp tools).

// ---------------------------------------------------------------- harmony
const HARMONY = (() => {
  const PC = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };
  const QUAL = { "": [0, 4, 7], m: [0, 3, 7], dim: [0, 3, 6], m7b5: [0, 3, 6, 10], "7": [0, 4, 7, 10], "9": [0, 4, 7, 10, 14],
                 maj7: [0, 4, 7, 11], maj9: [0, 4, 7, 11, 14], m7: [0, 3, 7, 10], m9: [0, 3, 7, 10, 14], "13": [0, 4, 10, 14, 21],
                 add9: [0, 4, 7, 14], sus4: [0, 5, 7], sus2: [0, 2, 7] };
  const SHARP = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"], FLAT = ["C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"];
  const pcOf = s => { const m = /^([A-G])([#b]?)$/.exec(s.trim()); if (!m) throw new Error("note " + s); return (PC[m[1]] + (m[2] === "#" ? 1 : m[2] === "b" ? -1 : 0) + 12) % 12; };
  const flatsFor = (tonic, mode) => mode === "minor" ? [2, 7, 0, 5, 10, 3, 8].includes(tonic) : [5, 10, 3, 8, 1, 6].includes(tonic);
  const name = (pc, flats) => (flats ? FLAT : SHARP)[((pc % 12) + 12) % 12];
  function parse(sym) {
    const [main, slash] = sym.split("/");
    const m = /^([A-G][#b]?)(.*)$/.exec(main);
    if (!m || !(m[2] in QUAL)) throw new Error("chord " + sym);
    const root = pcOf(m[1]), iv = QUAL[m[2]];
    return { sym, root, q: m[2], iv, bass: slash ? pcOf(slash) : root, third: iv.includes(3) ? 3 : iv.includes(5) && !iv.includes(4) ? 5 : 4,
             seventh: iv.includes(10) ? 10 : iv.includes(11) ? 11 : null };
  }
  // left hand: the chord's bass around G2–F#3 (spread: plus its 5th or 7th); right hand: a close voicing led smoothly
  function voice(chords, opt = {}) {
    let center = 64;
    return chords.map(c => {
      const lh = 43 + ((c.bass - 7 + 12) % 12);
      let lhNotes = [lh], upIv = c.iv.length >= 4 ? c.iv.filter(x => x !== 0) : c.iv;
      if (opt.spread && c.iv.length >= 4) {   // shell in the left hand (root + 7th or 5th), colour tones on top
        const shell = c.seventh != null ? c.seventh : 7;
        lhNotes = [lh, lh + shell];
        upIv = c.iv.filter(x => x !== 0 && x !== shell);
        if (upIv.length < 3 && c.iv.includes(7) && shell !== 7) upIv = upIv.concat([7]);
      }
      const pcs = [...new Set(upIv.map(x => (c.root + x) % 12))];
      let best = null;
      const floor = Math.max(...lhNotes) + (opt.spread ? 3 : 5);
      for (let r = 0; r < pcs.length; r++) {
        const order = pcs.slice(r).concat(pcs.slice(0, r));
        for (let start = 52; start <= 68; start++) {
          if (start % 12 !== order[0]) continue;
          const notes = [start]; for (let i = 1; i < order.length; i++) { let n = notes[i - 1] + 1; while (n % 12 !== order[i]) n++; notes.push(n); }
          if (notes[0] < floor || notes[notes.length - 1] > 81) continue;
          const avg = notes.reduce((a, b) => a + b, 0) / notes.length, score = Math.abs(avg - center);
          if (!best || score < best.score) best = { notes, score, avg };
        }
      }
      if (!best) best = { notes: pcs.map(pc => 60 + ((pc - 0 + 12) % 12)).sort((a, b) => a - b), avg: 64 };
      center = best.avg * .7 + 64 * .3;
      return { lh, lhNotes, up: best.notes, chord: c };
    });
  }
  // one-note change (hold a chord, move a single voice): the top voice steps to the next chord tone or the 9th
  function moved(v) {
    const top = v.up[v.up.length - 1], ok = new Set(v.chord.iv.map(x => (v.chord.root + x) % 12).concat([(v.chord.root + 2) % 12]));
    for (let n = top + 1; n <= top + 5; n++) if (ok.has(n % 12)) return v.up.slice(0, -1).concat([n]);
    return v.up;
  }
  // chords (+ optional bass bars) -> note events in beats; blocks: [{sym, dur}] with dur in beats
  function chordEvents(blocks, opt = {}) {
    const chords = blocks.map(b => parse(b.sym)), vs = voice(chords, opt);
    const ev = []; let t = 0;
    vs.forEach((v, i) => {
      const dur = blocks[i].dur, half = opt.move && dur >= 8 ? dur / 2 : dur;
      v.lhNotes.forEach((n, k) => ev.push({ t: t + k * .01, d: dur - .05, n, vel: .66 - k * .06, track: 0, blk: i }));
      v.up.forEach((n, k) => ev.push({ t: t + .015 + k * .012, d: (k === v.up.length - 1 ? half : dur) - .05, n, vel: .57 + .05 * Math.sin(i * 3 + k), track: 0, blk: i }));
      if (half < dur) { const mv = moved(v); ev.push({ t: t + half, d: half - .05, n: mv[mv.length - 1], vel: .55, track: 0, blk: i }); }
      t += dur;
    });
    return { ev, beats: t, chords, voicings: vs };
  }
  return { QUAL, pcOf, name, flatsFor, parse, voice, moved, chordEvents };
})();

// ---------------------------------------------------------------- audition player (one at a time, loops)
const LUMI_PLAY = (() => {
  let cur = null, ctr = 0;
  const listeners = new Set();
  const emit = () => listeners.forEach(f => f(cur ? cur.key : null, cur ? cur.blk : -1));
  function stop() {
    if (!cur) return;
    clearInterval(cur.iv); for (const t of cur.timers) clearTimeout(t);
    for (const id of cur.on) noteOff(id);
    cur = null; emit();
  }
  // score: {ev:[{t,d,n,vel,track,blk}], beats} or a function returning one (read again every loop, so overdubs play);
  // opt: {bpm, tracks:Set|null, loop:true, delay: seconds before beat 1 (count-in)}
  function start(key, scoreIn, opt) {
    stop();
    if (!ctx) startFromGesture();
    const getScore = typeof scoreIn === "function" ? scoreIn : () => scoreIn, score = getScore();
    const spb = 60 / opt.bpm, loopLen = score.beats * spb, p = { key, iv: 0, timers: [], on: new Set(), t0: performance.now() / 1000 + .08 + (opt.delay || 0), next: 0, blk: -1, spb, beats: score.beats };
    cur = p;
    const starts = [...new Set(score.ev.filter(e => e.blk != null).map(e => e.blk + ":" + Math.floor(e.t * 1000)))];
    const tick = () => {
      if (cur !== p) return;
      const now = performance.now() / 1000;
      while (p.t0 + p.next * loopLen < now + .25) {
        if (p.next > 0 && opt.loop === false) { p.timers.push(setTimeout(() => { if (cur === p) stop(); }, Math.max(0, (p.t0 + loopLen - now) * 1000) + 200)); p.next = 1e9; break; }
        const base = p.t0 + p.next * loopLen, score = getScore();
        const firstOfBlock = new Map();
        for (const e of score.ev) if (e.blk != null && (!firstOfBlock.has(e.blk) || e.t < firstOfBlock.get(e.blk))) firstOfBlock.set(e.blk, e.t);
        for (const [blk, t] of firstOfBlock) p.timers.push(setTimeout(() => { if (cur === p) { p.blk = blk; emit(); } }, Math.max(0, (base + t * spb - now) * 1000)));
        for (const e of score.ev) {
          if (opt.tracks && !opt.tracks.has(e.track)) continue;
          const at = base + e.t * spb, id = "lp" + (++ctr);
          p.timers.push(setTimeout(() => { if (cur !== p) return; p.on.add(id); noteOn(id, e.n, e.vel); }, Math.max(0, (at - now) * 1000)));
          p.timers.push(setTimeout(() => { if (cur !== p) return; p.on.delete(id); noteOff(id); }, Math.max(0, (at + e.d * spb - now) * 1000)));
        }
        p.next++;
        if (p.timers.length > 6000) p.timers.splice(0, 3000);
      }
    };
    tick(); p.iv = setInterval(tick, 60);
    void starts; emit();
  }
  function once(notes, vel = .62, ms = 1500) {   // a single chord
    if (!ctx) startFromGesture();
    notes.forEach((n, j) => { const id = "lo" + (++ctr); setTimeout(() => noteOn(id, n, vel), j * 14); setTimeout(() => noteOff(id), ms); });
  }
  // where the loop is now, in beats (negative during a count-in)
  const clock = () => cur ? { key: cur.key, beat: (performance.now() / 1000 - cur.t0) / cur.spb, beats: cur.beats, spb: cur.spb, t0: cur.t0 } : null;
  return { start, stop, once, clock, playing: () => cur && cur.key, onChange: f => listeners.add(f) };
})();

// ---------------------------------------------------------------- universal export: MIDI, WAV, drag to DAW
const LUMI_EXPORT = (() => {
  const PPQ = 480;
  const vlq = n => { const b = [n & 127]; while ((n >>= 7)) b.unshift((n & 127) | 128); return b; };
  const str = s => [...new TextEncoder().encode(s)];
  // tracks: [{name, ev:[{t,d,n,vel}]}] in beats -> standard MIDI file, format 1 (tempo + one track each)
  function midi(tracks, bpm) {
    const us = Math.round(60e6 / bpm);
    const out = [0x4d, 0x54, 0x68, 0x64, 0, 0, 0, 6, 0, 1, 0, tracks.length, PPQ >> 8, PPQ & 255];
    tracks.forEach((tr, ti) => {
      const list = [], ch = (tr.channel || 0) & 15;
      for (const e of tr.ev) { const a = Math.max(0, Math.round(e.t * PPQ)), b = Math.max(a + 1, Math.round((e.t + e.d) * PPQ)); list.push([a, 1, 0x90 | ch, e.n, Math.max(1, Math.min(127, Math.round(e.vel * 127)))], [b, 0, 0x80 | ch, e.n, 0]); }
      list.sort((x, y) => x[0] - y[0] || x[1] - y[1]);
      const body = [0, 0xff, 3, ...vlq(str(tr.name).length), ...str(tr.name)];
      if (ti === 0) body.push(0, 0xff, 0x51, 3, (us >> 16) & 255, (us >> 8) & 255, us & 255, 0, 0xff, 0x58, 4, 4, 2, 24, 8);
      let t = 0; for (const [at, , st, n, v] of list) { body.push(...vlq(at - t), st, n, v); t = at; }
      body.push(0, 0xff, 0x2f, 0);
      out.push(0x4d, 0x54, 0x72, 0x6b, ...[24, 16, 8, 0].map(s => (body.length >> s) & 255), ...body);
    });
    return new Uint8Array(out);
  }
  const b64 = bytes => { let s = ""; for (let i = 0; i < bytes.length; i += 8192) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 8192)); return btoa(s); };
  const safe = s => s.replace(/[\\/:*?"<>|]+/g, "-").replace(/\s+/g, " ").trim();
  function download(name, bytes, mime) {
    try {
      const url = URL.createObjectURL(new Blob([bytes], { type: mime }));
      const a = el("a", { href: url, download: name }); document.body.append(a); a.click(); a.remove();
      setTimeout(() => URL.revokeObjectURL(url), 8000);
      return true;
    } catch (e) { return false; }
  }
  const exported = () => window.dispatchEvent(new CustomEvent("lumi:milestone", { detail: "exported" }));
  function saveMidi(name, bytes) {
    exported();
    name = safe(name) + ".mid";
    if (PLUGIN) { send({ type: "saveFile", name, b64: b64(bytes) }); return; }
    toast(download(name, bytes, "audio/midi") ? "MIDI saved: drag it into FL Studio's piano roll" : "This page can't save files here; the plugin can.");
  }
  // the current effects as the engine's state string (so an audio render matches what you hear)
  const fxString = () => SCHEMA.map((f, e) => `${f.id}.on=${fxState[e].on ? 1 : 0};` + f.params.map((p, i) => `${f.id}.${p.id}=${fxState[e].v[i]};`).join("")).join("");
  // offline render of a score with the selected sound and effects (browser: the same WASM engine in an OfflineAudioContext)
  let wasmBytes = null;
  async function renderWav(score, bpm, tracks) {
    const sr = 44100, spb = 60 / bpm, tail = 4, secs = score.beats * spb + tail;
    const OAC = window.OfflineAudioContext || window.webkitOfflineAudioContext;
    if (!OAC) throw new Error("offline audio unavailable");
    const oc = new OAC(2, Math.ceil(secs * sr), sr);
    if (!wasmBytes) wasmBytes = await fetch("lumi.wasm").then(r => r.arrayBuffer());
    await oc.audioWorklet.addModule("lumi-worklet.js");
    const node = new AudioWorkletNode(oc, "lumi-engine", { numberOfInputs: 0, outputChannelCount: [2] });
    node.connect(oc.destination);
    const wait = type => new Promise(res => { const h = e => { if (e.data.type === type) { node.port.removeEventListener("message", h); res(e.data); } }; node.port.addEventListener("message", h); node.port.start(); });
    const ready = wait("ready"); node.port.postMessage({ type: "module", bytes: wasmBytes.slice(0) }); await ready;
    const p = PRESETS[cur], te = new TextEncoder();
    const loaded = wait("loaded"); node.port.postMessage({ type: "load", id: p.id, bytes: te.encode(p.params + "\0"), bpm, state: te.encode(fxString() + "\0") }); await loaded;
    node.port.postMessage({ type: "tempo", bpm });
    const seq = [];
    for (const e of score.ev) { if (tracks && !tracks.has(e.track)) continue; seq.push({ f: Math.round(e.t * spb * sr), on: 1, note: e.n, vel: e.vel }, { f: Math.round((e.t + e.d) * spb * sr), on: 0, note: e.n }); }
    seq.sort((a, b) => a.f - b.f || a.on - b.on);
    if (typeof SAMPLER !== "undefined") await SAMPLER.applyToNode(node, wait);   // the sampler / layer mode takes part in the render
    const ok = wait("seqok"); node.port.postMessage({ type: "seq", events: seq }); await ok;
    const buf = await oc.startRendering();
    return wav(buf.getChannelData(0), buf.getChannelData(1), sr);
  }
  function wav(L, R, sr) {   // 24-bit stereo PCM, peak-safe
    let pk = 0; for (let i = 0; i < L.length; i++) pk = Math.max(pk, Math.abs(L[i]), Math.abs(R[i]));
    const g = pk > .98 ? .98 / pk : 1, n = L.length, data = n * 6, b = new DataView(new ArrayBuffer(44 + data));
    const w = (o, s) => { for (let i = 0; i < s.length; i++) b.setUint8(o + i, s.charCodeAt(i)); };
    w(0, "RIFF"); b.setUint32(4, 36 + data, true); w(8, "WAVE"); w(12, "fmt "); b.setUint32(16, 16, true); b.setUint16(20, 1, true); b.setUint16(22, 2, true);
    b.setUint32(24, sr, true); b.setUint32(28, sr * 6, true); b.setUint16(32, 6, true); b.setUint16(34, 24, true); w(36, "data"); b.setUint32(40, data, true);
    let o = 44;
    for (let i = 0; i < n; i++) for (const x of [L[i], R[i]]) { let v = Math.round(Math.max(-1, Math.min(.9999999, x * g)) * 8388607); if (v < 0) v += 16777216; b.setUint8(o++, v & 255); b.setUint8(o++, (v >> 8) & 255); b.setUint8(o++, (v >> 16) & 255); }
    return new Uint8Array(b.buffer);
  }
  function saveWav(name, bytes) { exported(); name = safe(name) + ".wav"; if (PLUGIN) { send({ type: "saveFile", name, b64: b64(bytes) }); return; } toast(download(name, bytes, "audio/wav") ? "Audio saved (WAV)" : "This page can't save files here; the plugin can."); }
  async function saveAudio(name, score, bpm, tracks) {
    name = safe(name) + ".wav";
    if (PLUGIN) { send({ type: "renderAudio", name, bpm, ev: score.ev.filter(e => !tracks || tracks.has(e.track)).map(e => [e.t, e.d, e.n, e.vel]), beats: score.beats, drag: false }); return; }
    toast("Rendering audio…");
    try { const bytes = await renderWav(score, bpm, tracks); toast(download(name, bytes, "audio/wav") ? "Audio saved (WAV, with the current sound and effects)" : "This page can't save files here; the plugin can."); }
    catch (e) { toast("Audio export isn't available in this browser: " + (e && e.message || e)); }
  }
  // DRAG TO DAW: in the plugin the file is dragged natively (FL Studio accepts it in the piano roll / playlist);
  // in a browser, Chrome can drag the file out (DownloadURL); a click always saves it as a fallback
  function dragHandle(node, getItem) {
    node.setAttribute("draggable", PLUGIN ? "false" : "true");
    let url = null, moved = false, downAt = null;
    node.addEventListener("pointerdown", e => {
      moved = false; downAt = [e.clientX, e.clientY];
      if (PLUGIN) {
        const it = getItem(); if (!it) return;
        e.preventDefault(); exported();
        if (it.native) send(it.native);
        else if (it.audio) send({ type: "renderAudio", name: safe(it.name) + ".wav", bpm: it.bpm, ev: it.score.ev.filter(x => !it.tracks || it.tracks.has(x.track)).map(x => [x.t, x.d, x.n, x.vel]), beats: it.score.beats, drag: true });
        else send({ type: "dragFile", name: safe(it.name) + ".mid", b64: b64(it.bytes) });
      }
    });
    node.addEventListener("pointermove", e => { if (downAt && Math.hypot(e.clientX - downAt[0], e.clientY - downAt[1]) > 5) moved = true; });
    node.addEventListener("dragstart", e => {
      const it = getItem(); if (!it || it.audio || it.lazy) { e.preventDefault(); return; }
      if (url) URL.revokeObjectURL(url);
      url = URL.createObjectURL(new Blob([it.bytes], { type: "audio/midi" }));
      exported(); try { e.dataTransfer.setData("DownloadURL", `audio/midi:${safe(it.name)}.mid:${url}`); e.dataTransfer.setData("text/plain", safe(it.name) + ".mid"); e.dataTransfer.effectAllowed = "copy"; } catch (_) {}
    });
    node.addEventListener("click", () => {
      downAt = null; if (moved) return;
      const it = getItem(); if (!it) return;
      if (PLUGIN) return;   // the plugin already handled the press
      if (it.lazy) it.get().then(r => r && r.bytes && saveWav(r.name, r.bytes));
      else if (it.audio) saveAudio(it.name, it.score, it.bpm, it.tracks); else saveMidi(it.name, it.bytes);
    });
  }
  return { midi, saveMidi, saveAudio, saveWav, wavBytes: wav, renderWav, dragHandle, fxString, b64 };
})();

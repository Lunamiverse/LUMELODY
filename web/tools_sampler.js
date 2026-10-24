// ================================================================ SAMPLER: My Samples, sample editor, YouTube import, layer mode
// The same engine code (engine/LumiSampler.h, LumiAnalysis.h) runs in the browser (WASM) and in the plugin (native).
// Browser: files are decoded here, kept only on this device (IndexedDB), analysed by a second engine instance on the
// main thread and played by the AudioWorklet. Plugin: the native side decodes, analyses, keeps file paths and plays.
const SAMPLER = (() => {
  const MAX_FILE_BYTES = 300 * 1024 * 1024, MAX_SECONDS = 600, MAX_SAMPLER_SECONDS = 300, PEAK_BINS = 1400;
  const KEYN = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"];
  const noteName = n => KEYN[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 1);
  const fmtSecs = s => s >= 60 ? `${Math.floor(s / 60)}:${String(Math.round(s % 60)).padStart(2, "0")}` : `${s.toFixed(s < 10 ? 2 : 1)} s`;
  const DEF = { stretch: 1, reverse: 0, revA: 0, revB: 1, start: 0, end: 1, loopA: 0, loopB: 1, loop: 0, mode: 0, root: 60, tune: 0, sliceBase: 60, slices: [],
                attack: .002, decay: .3, sustain: 1, release: .08, gainDb: 0, pan: 0, velSens: .7, oneShot: 0, polyphony: 16 };
  const toText = s => Object.keys(DEF).filter(k => k !== "slices").map(k => `${k}=${+s[k]};`).join("") + "slices=" + s.slices.map(x => +x.toFixed(6)).join(",") + ";";
  const REBUILD = ["stretch", "reverse", "revA", "revB"];

  let cur = null;                         // the sample loaded in the sampler: {id, name, sr, channels, frames, seconds, an, path}
  let S = Object.assign({}, DEF);         // its settings
  let layer = Object.assign({ mode: 0, piano: 1, sampler: .7 }, store.get("lumi-layer-v1", {}));
  let undo = [], redo = [], peaks = null, ab = null, tool = "trim", lib = [], query = "", favOnly = false;
  const decoded = new Map();              // id -> {L, R, sr, channels} (small cache: the same file is never decoded twice in a row)
  const remember = (id, a) => { decoded.delete(id); decoded.set(id, a); while (decoded.size > 3) decoded.delete(decoded.keys().next().value); };

  // ---------------------------------------------------------------- native bridge (plugin) with request ids
  let rid = 0; const waiting = new Map();
  function req(type, payload = {}) { return new Promise((res, rej) => { const id = ++rid; waiting.set(id, { res, rej }); send(Object.assign({ type, rid: id }, payload)); setTimeout(() => { if (waiting.has(id)) { waiting.delete(id); rej(new Error("the plugin did not answer")); } }, 120000); }); }
  function onNative(m) { if (m.rid && waiting.has(m.rid)) { const w = waiting.get(m.rid); waiting.delete(m.rid); m.error ? w.rej(new Error(m.error)) : w.res(m); return true; } return false; }

  // ---------------------------------------------------------------- browser: storage (IndexedDB), decoding, analysis engine
  let dbp = null;
  function db() {
    if (!dbp) dbp = new Promise((res, rej) => { const r = indexedDB.open("lumelody", 1); r.onupgradeneeded = () => { const d = r.result; d.createObjectStore("samples", { keyPath: "id" }); d.createObjectStore("presets", { keyPath: "name" }); }; r.onsuccess = () => res(r.result); r.onerror = () => rej(r.error); });
    return dbp;
  }
  async function idb(storeName, mode, fn) { const d = await db(); return new Promise((res, rej) => { const t = d.transaction(storeName, mode), st = t.objectStore(storeName); const r = fn(st); t.oncomplete = () => res(r && r.result); t.onerror = () => rej(t.error); t.onabort = () => rej(t.error || new Error("storage full or blocked")); }); }
  let TWp = null;
  function tools() {   // a second engine instance on the main thread: analysis and waveforms never touch the audio thread
    if (!TWp) TWp = (async () => {
      const bytes = await fetch("lumi.wasm").then(r => { if (!r.ok) throw new Error("engine file missing"); return r.arrayBuffer(); });
      const mod = await WebAssembly.compile(bytes), stubs = {};
      for (const i of WebAssembly.Module.imports(mod)) (stubs[i.module] ??= {})[i.name] = () => 0;
      const X = (await WebAssembly.instantiate(mod, stubs)).exports; X._initialize(); return X;
    })();
    return TWp;
  }
  async function toolsLoad(a) {
    const X = await tools(), n = a.L.length, pl = X.lumi_alloc(n), pr = X.lumi_alloc(n);
    new Float32Array(X.memory.buffer, pl, n).set(a.L); new Float32Array(X.memory.buffer, pr, n).set(a.R);
    X.lumi_sample_set(pl, pr, n, a.sr, a.channels); X.lumi_free(pl); X.lumi_free(pr); toolsLoaded = a;
    return X;
  }
  const writeText = (X, t) => { const b = new TextEncoder().encode(t + "\0"); new Uint8Array(X.memory.buffer, X.lumi_text(), b.length).set(b); };
  let toolsLoaded = null;
  async function toolsFor(a) { const X = await tools(); return toolsLoaded === a ? X : toolsLoad(a); }
  async function analyse(a) {
    const X = await toolsFor(a), n = X.lumi_analyse(), o = Array.from(new Float32Array(X.memory.buffer, X.lumi_out(), n));
    return { bpm: o[0], bpmConf: o[1], key: o[2], minor: o[3] > .5, keyConf: o[4], root: o[5], cents: o[6], rootConf: o[7], silS: o[8], silE: o[9], seconds: o[10], onsets: o.slice(12, 12 + o[11]) };
  }
  async function peaksOf(a, settings, bins = PEAK_BINS) {
    const X = await toolsFor(a); writeText(X, toText(settings || DEF));
    const n = X.lumi_peaks(bins, settings ? 1 : 0); return Array.from(new Float32Array(X.memory.buffer, X.lumi_out(), n));
  }
  async function processedOf(a, settings) {
    const X = await toolsFor(a); writeText(X, toText(settings));
    const n = X.lumi_processed(0), L = new Float32Array(X.memory.buffer, X.lumi_out(), n).slice();
    writeText(X, toText(settings)); X.lumi_processed(1); const R = new Float32Array(X.memory.buffer, X.lumi_out(), n).slice();
    return { L, R };
  }
  function readFile(file, onProgress) {   // real progress while the file is read from disk
    return new Promise((res, rej) => { const fr = new FileReader(); fr.onprogress = e => e.lengthComputable && onProgress(e.loaded / e.total); fr.onload = () => res(fr.result); fr.onerror = () => rej(new Error("the file could not be read (moved, locked or no permission)")); fr.readAsArrayBuffer(file); });
  }
  async function decode(bytes) {
    const OAC = window.OfflineAudioContext || window.webkitOfflineAudioContext;
    const oc = new OAC(2, 1, 44100);
    let buf;
    try { buf = await oc.decodeAudioData(bytes.slice(0)); }
    catch (e) { throw new Error("this file is not audio this browser can decode, or it is damaged"); }
    if (!buf || buf.length < 2) throw new Error("the file contains no audio");
    if (buf.duration > MAX_SECONDS) throw new Error(`the audio is ${Math.round(buf.duration / 60)} minutes long; LUMELODY imports up to ${MAX_SECONDS / 60} minutes`);
    const L = buf.getChannelData(0).slice(), R = buf.numberOfChannels > 1 ? buf.getChannelData(1).slice() : L;
    return { L, R, sr: buf.sampleRate, channels: Math.min(2, buf.numberOfChannels) };
  }
  async function hash(bytes) { try { const h = await crypto.subtle.digest("SHA-256", bytes); return Array.from(new Uint8Array(h).slice(0, 12), b => b.toString(16).padStart(2, "0")).join(""); } catch (e) { return "f" + bytes.byteLength + "-" + Date.now(); } }
  const AUDIO_EXT = /\.(wav|wave|aif|aiff|flac|mp3|ogg|oga|opus|m4a|aac|webm|caf)$/i;

  // ---------------------------------------------------------------- import (file picker, drag and drop, folders)
  async function importFiles(files) {
    files = [...files];
    if (!files.length) return;
    let ok = 0, last = null;
    for (const [i, f] of files.entries()) {
      try { last = await importOne(f, p => progress(`${f.name}: ${p}`, (i + .5) / files.length)); ok++; }
      catch (e) { progress(null); note(`${f.name}: ${e.message}`, true); }
    }
    progress(null);
    if (ok) { await loadLib(); renderLib(); if (files.length === 1 && last) note(last.dupe ? `${last.name} is already in your samples` : `${last.name} imported`); else note(`${ok} of ${files.length} files imported`); }
    return last;
  }
  async function importOne(f, prog) {
    if (PLUGIN) {
      if (f.size > 64 * 1024 * 1024) throw new Error("dropped files over 64 MB: use import file… so the plugin reads it from disk");
      prog("reading"); const bytes = new Uint8Array(await readFile(f, x => prog(`reading ${Math.round(x * 100)}%`)));
      prog("decoding"); return await req("smpImportData", { name: f.name, b64: LUMI_EXPORT.b64(bytes) });
    }
    if (!f.size) throw new Error("the file is empty");
    if (f.size > MAX_FILE_BYTES) throw new Error(`the file is ${Math.round(f.size / 1048576)} MB; the limit is ${MAX_FILE_BYTES / 1048576} MB`);
    if (f.type && !/^audio\/|^video\/(mp4|webm|ogg)/.test(f.type) && !AUDIO_EXT.test(f.name)) throw new Error("not an audio file");
    const bytes = await readFile(f, x => prog(`reading ${Math.round(x * 100)}%`));
    const id = await hash(bytes);
    const existing = await idb("samples", "readonly", st => st.get(id));
    if (existing) return Object.assign(existing, { dupe: true });
    prog("decoding"); const a = await decode(bytes);
    prog("analysing"); const an = await analyse(a), pk = await peaksOf(a, null, 160);
    const entry = { id, name: f.name.replace(/\.[^.]+$/, ""), file: f.name, type: f.type, size: f.size, added: Date.now(), fav: false, bytes,
                    seconds: a.L.length / a.sr, sr: a.sr, channels: a.channels, peaks: pk, an: slimAn(an), folder: f.webkitRelativePath ? f.webkitRelativePath.split("/").slice(0, -1).join("/") : "" };
    prog("saving");
    await idb("samples", "readwrite", st => st.put(entry));
    remember(id, a);
    return entry;
  }
  const slimAn = an => ({ bpm: an.bpm, bpmConf: an.bpmConf, key: an.key, minor: an.minor, keyConf: an.keyConf, root: an.root, cents: an.cents, rootConf: an.rootConf, onsets: an.onsets, silS: an.silS, silE: an.silE });
  async function openFolder() {
    if (PLUGIN) { progress("choosing a folder…"); try { const r = await req("smpFolder"); progress(null); folderFiles = r.files || []; renderLib(); note(folderFiles.length ? `${folderFiles.length} audio files in ${r.folder}` : "no audio files in that folder"); } catch (e) { progress(null); note(e.message, true); } return; }
    if (window.showDirectoryPicker) {
      try {
        const dir = await window.showDirectoryPicker({ mode: "read" }), files = [];
        async function walk(h, path, depth) { for await (const [name, e] of h.entries()) { if (files.length > 2000) return; if (e.kind === "file" && AUDIO_EXT.test(name)) files.push(Object.assign(await e.getFile(), {})); else if (e.kind === "directory" && depth < 4) await walk(e, path + name + "/", depth + 1); } }
        progress("reading the folder…"); await walk(dir, "", 0); progress(null);
        folderFiles = files.map(f => ({ name: f.name, size: f.size, file: f })); renderLib();
        note(files.length ? `${files.length} audio files: preview them, then import the ones you want` : "no audio files in that folder");
      } catch (e) { progress(null); if (e.name !== "AbortError") note("this browser blocked folder access: use import file or drag files here", true); }
    } else $("#smFolderIn").click();
  }
  let folderFiles = [];

  // ---------------------------------------------------------------- library list
  async function loadLib() {
    try {
      if (PLUGIN) { const r = await req("libLoad"); lib = r.entries || []; }
      else lib = (await idb("samples", "readonly", st => st.getAll())) || [];
      lib.sort((a, b) => (b.fav - a.fav) || (b.added - a.added));
    } catch (e) { lib = []; note("your sample library could not be opened: " + e.message, true); }
  }
  const keyLabel = an => an && an.key >= 0 ? `${KEYN[an.key]}${an.minor ? "m" : ""}` : "";
  const sure = c => c >= .55 ? "" : "?";
  function miniWave(pk) {
    const c = el("canvas", { className: "smwave" }); c.width = 160; c.height = 26;
    const g = c.getContext("2d"); g.fillStyle = "rgba(255,255,255,.75)";
    if (pk) for (let i = 0; i < pk.length / 2; i++) { const lo = pk[i * 2], hi = pk[i * 2 + 1]; const y0 = 13 - hi * 12, y1 = 13 - lo * 12; g.fillRect(i, y0, 1, Math.max(1, y1 - y0)); }
    return c;
  }
  function renderLib() {
    const list = $("#smList"); if (!list) return;
    const q = query.trim().toLowerCase();
    const rows = lib.filter(e => (!favOnly || e.fav) && (!q || (e.name + " " + (e.folder || "") + " " + keyLabel(e.an)).toLowerCase().includes(q)));
    const items = rows.map(e => {
      const star = el("button", { className: "smstar" + (e.fav ? " on" : ""), type: "button", textContent: e.fav ? "★" : "☆", title: e.fav ? "Remove from favourites" : "Add to favourites" });
      star.onclick = async () => { e.fav = !e.fav; await saveEntry(e); renderLib(); };
      const meta = [fmtSecs(e.seconds || 0), e.an && e.an.bpm && e.an.bpmConf > .25 ? `${Math.round(e.an.bpm)} bpm${sure(e.an.bpmConf)}` : "", keyLabel(e.an) ? keyLabel(e.an) + sure(e.an.keyConf) : "", e.missing ? "file not found" : ""].filter(Boolean).join(" · ");
      const play = el("button", { className: "btn", type: "button", textContent: "▶", title: "Preview" }); play.onclick = () => preview(e, play);
      const use = el("button", { className: "btn hot", type: "button", textContent: cur && cur.id === e.id ? "loaded" : "use", title: "Load into the sampler (plays from MIDI, the keys and the pads)" }); use.onclick = () => useEntry(e);
      const del = el("button", { className: "btn", type: "button", textContent: "×", title: PLUGIN ? "Remove from the list (the file stays on disk)" : "Remove from LUMELODY's library (your original file is not touched)" });
      del.onclick = async () => { await removeEntry(e); };
      const row = el("div", { className: "smrow" + (cur && cur.id === e.id ? " cur" : "") }, [star, el("div", { className: "smname" }, [el("b", { textContent: e.name }), el("small", { textContent: meta })]), miniWave(e.peaks), play, use, del]);
      return row;
    });
    const folder = folderFiles.length ? [el("div", { className: "smfolder" }, [el("span", { className: "dim", textContent: "folder: " + folderFiles.length + " files (not imported yet)" })]), ...folderFiles.slice(0, 300).map(ff => {
      const p = el("button", { className: "btn", type: "button", textContent: "▶" }); p.onclick = () => previewFile(ff, p);
      const imp = el("button", { className: "btn", type: "button", textContent: "import" }); imp.onclick = async () => { if (PLUGIN) { try { progress("importing…"); await req("smpImportPath", { path: ff.path }); progress(null); await loadLib(); renderLib(); note(ff.name + " imported"); } catch (e) { progress(null); note(e.message, true); } } else importFiles([ff.file]); };
      return el("div", { className: "smrow f" }, [el("div", { className: "smname" }, [el("b", { textContent: ff.name }), el("small", { textContent: ff.size ? (ff.size / 1048576).toFixed(1) + " MB" : "" })]), p, imp]);
    })] : [];
    list.replaceChildren(...(items.length || folder.length ? items.concat(folder) : [el("p", { className: "smempty", textContent: lib.length ? "nothing matches the search" : "no samples yet: import a file, open a folder, or drag audio files here" })]));
    $("#smCount").textContent = `${lib.length} sample${lib.length === 1 ? "" : "s"}`;
  }
  async function saveEntry(e) { try { if (PLUGIN) await req("libSave", { entries: lib.map(x => Object.assign({}, x)) }); else await idb("samples", "readwrite", st => st.put(e)); } catch (err) { note("could not save: " + err.message, true); } }
  async function removeEntry(e) {
    try { if (PLUGIN) { lib = lib.filter(x => x.id !== e.id); await req("libSave", { entries: lib }); } else await idb("samples", "readwrite", st => st.delete(e.id)); decoded.delete(e.id); await loadLib(); renderLib(); note(e.name + " removed"); }
    catch (err) { note("could not remove: " + err.message, true); }
  }

  // ---------------------------------------------------------------- preview (independent of the sampler)
  let pv = null;
  function stopPreview() { if (pv) { try { pv.src && pv.src.stop(); } catch (e) {} if (PLUGIN) send({ type: "smpPreview", on: false }); if (pv.btn) pv.btn.textContent = "▶"; pv = null; } }
  async function audioOf(e) {
    if (decoded.has(e.id)) { const a = decoded.get(e.id); remember(e.id, a); return a; }
    const full = await idb("samples", "readonly", st => st.get(e.id));
    if (!full || !full.bytes) throw new Error("the stored audio is missing");
    const a = await decode(full.bytes); remember(e.id, a); return a;
  }
  async function playBuffer(a, btn) {
    if (!ctx) await startAudio();
    if (!ctx || ctx.plugin || !ctx.createBufferSource) return;
    const b = ctx.createBuffer(2, a.L.length, a.sr); b.copyToChannel(a.L, 0); b.copyToChannel(a.R, 1);
    const src = ctx.createBufferSource(); src.buffer = b; src.connect(ctx.destination); src.start();
    pv = { src, btn }; btn.textContent = "■"; src.onended = () => { if (pv && pv.src === src) { btn.textContent = "▶"; pv = null; } };
  }
  async function preview(e, btn) {
    if (pv && pv.btn === btn) { stopPreview(); return; }
    stopPreview();
    try {
      if (PLUGIN) { await req("smpPreview", { on: true, path: e.path, id: e.id }); pv = { btn }; btn.textContent = "■"; return; }
      await playBuffer(await audioOf(e), btn);
    } catch (err) { note(err.message, true); }
  }
  async function previewFile(ff, btn) {
    if (pv && pv.btn === btn) { stopPreview(); return; }
    stopPreview();
    try { if (PLUGIN) { await req("smpPreview", { on: true, path: ff.path }); pv = { btn }; btn.textContent = "■"; return; } await playBuffer(await decode(await ff.file.arrayBuffer()), btn); }
    catch (err) { note(err.message, true); }
  }

  // ---------------------------------------------------------------- load into the sampler
  async function useEntry(e, settings) {
    stopPreview();
    try {
      progress("loading " + e.name + "…");
      let info;
      if (PLUGIN) info = await req("smpUse", { id: e.id, path: e.path, settings: settings ? toText(settings) : "" });
      else {
        const a = await audioOf(e);
        if (a.L.length / a.sr > MAX_SAMPLER_SECONDS) throw new Error(`the sampler plays up to ${MAX_SAMPLER_SECONDS / 60} minutes; trim the file first`);
        if (!ctx) await startAudio();
        info = { frames: a.L.length, sr: a.sr, channels: a.channels };
        cur = { id: e.id, name: e.name, sr: a.sr, channels: a.channels, frames: a.L.length, seconds: a.L.length / a.sr, an: e.an, audio: a };
      }
      if (PLUGIN) cur = { id: e.id, name: e.name, path: e.path, sr: info.sr, channels: info.channels, frames: info.frames, seconds: info.frames / info.sr, an: info.an || e.an };
      S = Object.assign({}, DEF, settings || {});
      if (!settings) {   // sensible start: root note from the analysis when it is confident, trimmed leading silence
        if (cur.an && cur.an.root > 0 && cur.an.rootConf > .5) S.root = Math.round(cur.an.root);
        if (cur.an && cur.an.silS > 0 && cur.frames) S.start = Math.min(.5, cur.an.silS / cur.frames);
      }
      undo = []; redo = []; ab = null;
      await pushToEngine(true, true);
      if (layer.mode === 0) setLayer(1);
      progress(null);
      saveCurrent();
      tab("edit"); renderLib();
      note(`${e.name} is in the sampler: play the keys, the pads or your MIDI`);
    } catch (err) { progress(null); note(err.message, true); }
  }
  async function pushToEngine(rebuild, withAudio) {
    if (!cur) return;
    const text = toText(ab ? Object.assign({}, S, { reverse: ab.reverse }) : S);
    if (PLUGIN) { const r = await req("smpSettings", { bytes: text, rebuild: !!rebuild, bins: PEAK_BINS }); if (r.peaks) peaks = r.peaks; }
    else {
      if (withAudio) { const a = cur.audio; send({ type: "sample", id: cur.id, L: a.L.slice(), R: a.R.slice(), sr: a.sr, channels: a.channels, settings: new TextEncoder().encode(text + "\0") }); }
      else send({ type: "sampler", bytes: new TextEncoder().encode(text + "\0"), rebuild: !!rebuild });
      if (rebuild || !peaks) peaks = await peaksOf(cur.audio, ab ? Object.assign({}, S, { reverse: ab.reverse }) : S);
    }
    drawWave();
  }
  function setLayer(mode, piano, samp) {
    layer.mode = mode; if (piano != null) layer.piano = piano; if (samp != null) layer.sampler = samp;
    send({ type: "layer", mode: layer.mode, piano: layer.piano, sampler: layer.sampler });
    store.set("lumi-layer-v1", layer); renderLayer();
  }
  function saveCurrent() { if (!PLUGIN && cur) store.set("lumi-sampler-current-v1", { id: cur.id, settings: S }); }

  // ---------------------------------------------------------------- edits (undo / redo, non-destructive)
  function edit(fn, rebuildKeys) {
    if (!cur) return;
    const before = JSON.stringify(S);
    const old = Object.assign({}, S);
    fn(S); sanitize(S);
    if (JSON.stringify(S) === before) return;
    undo.push(before); if (undo.length > 200) undo.shift(); redo = [];
    const rebuild = REBUILD.some(k => old[k] !== S[k]) || rebuildKeys;
    pushToEngine(rebuild); saveCurrent(); renderEdit();
  }
  function sanitize(s) {
    for (const k of ["start", "end", "loopA", "loopB", "revA", "revB"]) s[k] = Math.max(0, Math.min(1, +s[k] || 0));
    if (s.end < s.start) [s.start, s.end] = [s.end, s.start];
    if (s.end - s.start < .0005) s.end = Math.min(1, s.start + .0005);
    if (s.loopB < s.loopA) [s.loopA, s.loopB] = [s.loopB, s.loopA];
    if (s.revB < s.revA) [s.revA, s.revB] = [s.revB, s.revA];
    s.slices = [...new Set(s.slices.filter(x => x > 0 && x < 1).map(x => +x.toFixed(6)))].sort((a, b) => a - b).slice(0, 63);
    s.stretch = Math.max(.25, Math.min(4, +s.stretch || 1)); s.root = Math.max(0, Math.min(127, Math.round(s.root))); s.tune = Math.max(-24, Math.min(24, +s.tune || 0));
  }
  const doUndo = () => { if (!undo.length) return; redo.push(JSON.stringify(S)); const prev = JSON.parse(undo.pop()); const rb = REBUILD.some(k => prev[k] !== S[k]); S = prev; pushToEngine(rb); saveCurrent(); renderEdit(); };
  const doRedo = () => { if (!redo.length) return; undo.push(JSON.stringify(S)); const nx = JSON.parse(redo.pop()); const rb = REBUILD.some(k => nx[k] !== S[k]); S = nx; pushToEngine(rb); saveCurrent(); renderEdit(); };
  // whole-sample reverse keeps the same audio selected: every marker is mirrored
  function setReverse(mode) {
    edit(s => {
      const was = s.reverse === 1, will = mode === 1;
      if (was !== will) {
        const m = x => 1 - x;
        [s.start, s.end] = [m(s.end), m(s.start)]; [s.loopA, s.loopB] = [m(s.loopB), m(s.loopA)];
        s.slices = s.slices.map(m);
      }
      if (mode === 2 && s.revB - s.revA > .999) { s.revA = s.start; s.revB = s.end; }   // region defaults to the trimmed part
      s.reverse = mode;
    });
  }
  async function toggleAB() {   // hear the other version without changing the edit
    if (!cur) return;
    if (ab) { ab = null; await pushToEngine(true); }
    else { ab = { reverse: S.reverse ? 0 : 1 }; await pushToEngine(true); }
    renderEdit();
  }

  // ---------------------------------------------------------------- waveform + markers
  function drawWave() {
    const c = $("#smWave"); if (!c) return;
    const r = c.getBoundingClientRect(), d = Math.min(2, window.devicePixelRatio || 1);
    c.width = Math.max(10, Math.round(r.width * d)); c.height = Math.max(10, Math.round(r.height * d));
    const g = c.getContext("2d"), W = c.width, Hh = c.height, mid = Hh / 2;
    g.clearRect(0, 0, W, Hh);
    if (!cur || !peaks) { g.fillStyle = "rgba(255,255,255,.4)"; g.font = `${11 * d}px ui-monospace, monospace`; g.textAlign = "center"; g.fillText(cur ? "drawing…" : "load a sample from my samples", W / 2, mid); return; }
    const x = f => f * W;
    if (S.loop) { g.fillStyle = "rgba(120,220,255,.12)"; g.fillRect(x(S.loopA), 0, x(S.loopB) - x(S.loopA), Hh); }
    if (S.reverse === 2) { g.fillStyle = "rgba(255,80,190,.14)"; g.fillRect(x(S.revA), 0, x(S.revB) - x(S.revA), Hh); }
    g.fillStyle = "rgba(0,0,0,.55)"; g.fillRect(0, 0, x(S.start), Hh); g.fillRect(x(S.end), 0, W - x(S.end), Hh);
    const bins = peaks.length / 2;
    g.fillStyle = "#fff";
    for (let px = 0; px < W; px++) {
      const b0 = Math.floor(px / W * bins), b1 = Math.max(b0 + 1, Math.floor((px + 1) / W * bins));
      let lo = 1, hi = -1; for (let b = b0; b < b1 && b < bins; b++) { lo = Math.min(lo, peaks[b * 2]); hi = Math.max(hi, peaks[b * 2 + 1]); }
      if (hi < lo) continue;
      const inside = px >= x(S.start) && px <= x(S.end);
      g.globalAlpha = inside ? .92 : .3;
      g.fillRect(px, mid - hi * mid * .92, 1, Math.max(1, (hi - lo) * mid * .92));
    }
    g.globalAlpha = 1;
    const line = (f, col, w = 1.5) => { g.fillStyle = col; g.fillRect(Math.round(x(f) - w * d / 2), 0, w * d, Hh); };
    S.slices.forEach((s, i) => { line(s, "rgba(255,255,255,.55)", 1); g.fillStyle = "rgba(255,255,255,.7)"; g.font = `${9 * d}px ui-monospace, monospace`; g.fillText(String(i + 2), x(s) + 3 * d, 11 * d); });
    if (S.loop) { line(S.loopA, "#78dcff"); line(S.loopB, "#78dcff"); }
    line(S.start, "#fff", 2); line(S.end, "#fff", 2);
    // playhead of the sounding note(s) is not tracked sample-accurately here; the keys light up from the real notes
  }
  function waveSetup() {
    const c = $("#smWave"); let drag = null;
    const fAt = e => { const r = c.getBoundingClientRect(); return Math.max(0, Math.min(1, (e.clientX - r.left) / r.width)); };
    const near = (f, v) => Math.abs(f - v) < 8 / c.getBoundingClientRect().width;
    c.addEventListener("pointerdown", e => {
      if (!cur) return; e.preventDefault(); try { c.setPointerCapture(e.pointerId); } catch (_) {}
      const f = fAt(e), snap = JSON.stringify(S);
      if (tool === "trim") drag = { key: near(f, S.start) || Math.abs(f - S.start) < Math.abs(f - S.end) ? "start" : "end", snap };
      else if (tool === "loop") { if (near(f, S.loopA)) drag = { key: "loopA", snap }; else if (near(f, S.loopB)) drag = { key: "loopB", snap }; else { S.loop = 1; S.loopA = S.loopB = f; drag = { key: "loopB", from: f, snap }; } }
      else if (tool === "region") { if (S.reverse !== 2) { S.reverse = 2; } S.revA = S.revB = f; drag = { key: "revB", from: f, snap, rebuild: true }; }
      else if (tool === "slice") {
        const k = S.slices.findIndex(s => near(f, s));
        if (k >= 0) drag = { key: "slice", k, snap }; else { S.slices.push(f); sanitize(S); drag = { key: "slice", k: S.slices.indexOf(+f.toFixed(6)), snap }; }
      }
      drawWave();
    });
    c.addEventListener("pointermove", e => {
      if (!drag) return; const f = fAt(e);
      if (drag.key === "slice") { if (drag.k >= 0) S.slices[drag.k] = f; }
      else if (drag.from != null) { const a = Math.min(drag.from, f), b = Math.max(drag.from, f); if (drag.key === "revB") { S.revA = a; S.revB = b; } else { S.loopA = a; S.loopB = b; } }
      else S[drag.key] = f;
      drawWave();
    });
    const end = () => {
      if (!drag) return;
      const snap = drag.snap, rb = drag.rebuild; drag = null;
      sanitize(S);
      if (JSON.stringify(S) !== snap) { undo.push(snap); redo = []; pushToEngine(rb || JSON.parse(snap).reverse !== S.reverse); saveCurrent(); renderEdit(); }
    };
    c.addEventListener("pointerup", end); c.addEventListener("pointercancel", end);
    c.addEventListener("dblclick", e => { if (tool !== "slice" || !cur) return; const f = fAt(e), k = S.slices.findIndex(s => near(f, s)); if (k >= 0) edit(s => s.slices.splice(k, 1)); });
    window.addEventListener("resize", () => { if (!$("#smp").hidden) drawWave(); });
  }

  // ---------------------------------------------------------------- presets (instrument configurations)
  async function savePreset() {
    if (!cur) return;
    const name = ($("#smPresetName").value || cur.name).trim().slice(0, 60);
    const p = { name, sampleId: cur.id, path: cur.path || "", sampleName: cur.name, settings: toText(S), layer: Object.assign({}, layer), saved: Date.now() };
    try { if (PLUGIN) await req("smpPresetSave", { preset: p }); else await idb("presets", "readwrite", st => st.put(p)); note(`preset "${name}" saved`); renderPresets(); }
    catch (e) { note("could not save the preset: " + e.message, true); }
  }
  async function listPresets() { try { return PLUGIN ? ((await req("smpPresetList")).presets || []) : ((await idb("presets", "readonly", st => st.getAll())) || []); } catch (e) { return []; } }
  async function renderPresets() {
    const sel = $("#smPresets"); if (!sel) return;
    const ps = await listPresets();
    sel.replaceChildren(el("option", { value: "", textContent: ps.length ? "load preset…" : "no presets yet" }), ...ps.sort((a, b) => b.saved - a.saved).map(p => el("option", { value: p.name, textContent: p.name + " · " + p.sampleName })));
  }
  async function loadPreset_(name) {
    const p = (await listPresets()).find(x => x.name === name); if (!p) return;
    const e = lib.find(x => x.id === p.sampleId) || (p.path ? { id: p.sampleId, name: p.sampleName, path: p.path } : null);
    if (!e) { note(`the sample "${p.sampleName}" is no longer in your library`, true); return; }
    const parsed = parseText(p.settings);
    await useEntry(e, parsed);
    if (p.layer) setLayer(p.layer.mode, p.layer.piano, p.layer.sampler);
    note(`preset "${p.name}" loaded`);
  }
  function parseText(t) {
    const s = Object.assign({}, DEF);
    for (const kv of t.split(";")) { const [k, v] = kv.split("="); if (!k || v == null) continue; if (k === "slices") s.slices = v ? v.split(",").map(Number) : []; else if (k in DEF) s[k] = +v; }
    return s;
  }

  // ---------------------------------------------------------------- export the edited sample
  async function exportAudio(drag) {
    if (!cur) return;
    const name = `${cur.name} (edited)`;
    if (PLUGIN) { send({ type: "smpExport", name: name + ".wav", drag: !!drag }); return; }
    try { const { L, R } = await processedOf(cur.audio, S); const a = Math.floor(S.start * L.length), b = Math.ceil(S.end * L.length); const bytes = LUMI_EXPORT.wavBytes(L.subarray(a, b), R.subarray(a, b), cur.sr); return { name, bytes }; }
    catch (e) { note(e.message, true); }
  }

  // ---------------------------------------------------------------- YouTube: honest URL handling
  function parseYouTube(raw) {
    let u; const s = (raw || "").trim();
    if (!s) return { ok: false, reason: "paste a YouTube link" };
    try { u = new URL(/^https?:\/\//i.test(s) ? s : "https://" + s); } catch (e) { return { ok: false, reason: "that is not a valid link" }; }
    const host = u.hostname.replace(/^www\.|^m\./, "");
    const okHost = ["youtube.com", "music.youtube.com", "youtu.be", "youtube-nocookie.com"].includes(host);
    if (!okHost) return { ok: false, reason: "that link is not from YouTube" };
    let id = null, kind = "video";
    if (host === "youtu.be") id = u.pathname.slice(1).split("/")[0];
    else if (u.pathname === "/watch") id = u.searchParams.get("v");
    else { const m = /^\/(shorts|embed|live|v)\/([^/?#]+)/.exec(u.pathname); if (m) { id = m[2]; kind = m[1] === "shorts" ? "short" : m[1] === "live" ? "live" : "video"; } }
    if (!id && u.searchParams.get("list")) return { ok: false, reason: "that is a playlist link; paste the link of one video" };
    if (!id || !/^[A-Za-z0-9_-]{11}$/.test(id)) return { ok: false, reason: "no video found in that link" };
    return { ok: true, id, kind, canonical: "https://www.youtube.com/watch?v=" + id };
  }
  async function youtubeCheck() {
    const out = $("#ytResult"), p = parseYouTube($("#ytUrl").value);
    if (!p.ok) { out.replaceChildren(el("p", { className: "yterr", textContent: p.reason })); return p; }
    out.replaceChildren(el("p", { className: "dim", textContent: `video id ${p.id} · looking up the title…` }));
    let meta = null, why = "";
    try {
      if (PLUGIN) meta = await req("ytMeta", { url: p.canonical });
      else {
        const ctl = new AbortController(); const t = setTimeout(() => ctl.abort(), 7000);
        const r = await fetch("https://www.youtube.com/oembed?format=json&url=" + encodeURIComponent(p.canonical), { signal: ctl.signal }); clearTimeout(t);
        if (r.status === 401 || r.status === 403) why = "the owner does not allow this video to be shown outside YouTube, or it is private";
        else if (r.status === 404 || r.status === 400) why = "the video is unavailable (removed, private or the id is wrong)";
        else if (!r.ok) why = "YouTube answered with an error (" + r.status + ")";
        else meta = await r.json();
      }
    } catch (e) { why = PLUGIN ? e.message : "the title could not be looked up from here (network or browser blocked the request)"; }
    const info = meta && meta.title ? [el("div", { className: "ytmeta" }, [meta.thumbnail_url || meta.thumb ? el("img", { src: meta.thumbnail_url || meta.thumb, alt: "", referrerPolicy: "no-referrer" }) : null, el("div", {}, [el("b", { textContent: meta.title }), el("small", { textContent: meta.author_name || meta.author || "" })])])]
                                     : [el("p", { className: "dim", textContent: "video id " + p.id + (why ? " · " + why : "") })];
    out.replaceChildren(...info,
      el("p", { className: "ytwhy", textContent: "No audio was imported. YouTube does not offer an authorised way for apps to download a video's audio, and its terms do not allow it, so LUMELODY does not download from YouTube." }),
      el("p", { className: "ytwhy", textContent: "If you own this audio or have permission to use it, import your own file of it. For free music you may use, YouTube Studio's Audio Library lets you download tracks legally." }),
      (() => { const r = el("div", { className: "frow" }); const b1 = el("button", { className: "btn hot", type: "button", textContent: "import an audio file I own" }); b1.onclick = () => $("#smFileIn").click();
        const b2 = el("button", { className: "btn", type: "button", textContent: "open YouTube Audio Library" }); b2.onclick = () => { if (PLUGIN) send({ type: "openUrl", url: "https://studio.youtube.com/channel/UC/music" }); else window.open("https://studio.youtube.com/channel/UC/music", "_blank", "noopener"); };
        r.append(b1, b2); return r; })());
    return Object.assign({}, p, { meta, why });
  }

  // ---------------------------------------------------------------- UI
  const $s = q => $("#smp " + q);
  function note(t, err) { const n = $s("#smNote"); if (!n) return; n.textContent = t; n.classList.toggle("err", !!err); clearTimeout(note.t); note.t = setTimeout(() => { n.textContent = ""; n.classList.remove("err"); }, err ? 9000 : 4500); }
  function progress(t, frac) { const p = $s("#smProg"); if (!p) return; p.hidden = !t; if (t) { p.querySelector("span").textContent = t; p.querySelector("i").style.width = frac != null ? Math.round(frac * 100) + "%" : "100%"; } }
  function tab(t) {
    $s("#smLibPane").hidden = t !== "lib"; $s("#smEditPane").hidden = t !== "edit"; $s("#smYtPane").hidden = t !== "yt";
    document.querySelectorAll("#smTabs [data-t]").forEach(b => b.setAttribute("aria-pressed", b.dataset.t === t ? "true" : "false"));
    if (t === "edit") { renderEdit(); requestAnimationFrame(drawWave); }
    if (t === "lib") renderLib();
  }
  function renderLayer() { document.querySelectorAll("#smLayer [data-m]").forEach(b => b.setAttribute("aria-pressed", +b.dataset.m === layer.mode ? "true" : "false")); const pl = $s("#smPianoLvl"), sl = $s("#smSampLvl"); if (pl) { pl.value = layer.piano; sl.value = layer.sampler; } }
  function renderEdit() {
    const has = !!cur;
    $s("#smEditEmpty").hidden = has; $s("#smEditor").hidden = !has;
    if (!has) return;
    const an = cur.an || {};
    $s("#smTitle").textContent = cur.name;
    $s("#smInfo").textContent = [fmtSecs(cur.seconds), `${cur.channels === 1 ? "mono" : "stereo"} ${Math.round(cur.sr / 100) / 10} kHz`,
      an.bpm && an.bpmConf > .25 ? `≈ ${an.bpm.toFixed(1)} bpm (${an.bpmConf >= .55 ? "likely" : "unsure"})` : "no clear tempo",
      an.key >= 0 ? `≈ ${KEYN[an.key]} ${an.minor ? "minor" : "major"} (${an.keyConf >= .55 ? "likely" : "unsure"})` : "",
      an.root > 0 ? `root ≈ ${noteName(Math.round(an.root))}${an.rootConf >= .55 ? "" : " (unsure)"}` : "no clear pitch"].filter(Boolean).join(" · ");
    document.querySelectorAll("#smTools [data-tool]").forEach(b => b.setAttribute("aria-pressed", b.dataset.tool === tool ? "true" : "false"));
    $s("#smMode").value = String(S.mode); $s("#smRoot").value = S.root; $s("#smRootName").textContent = noteName(S.root);
    $s("#smTune").value = S.tune; $s("#smStretch").value = S.stretch; $s("#smStretchV").textContent = "×" + (+S.stretch).toFixed(2);
    $s("#smRev").value = String(S.reverse); $s("#smAB").textContent = ab ? (ab.reverse ? "hearing: reversed" : "hearing: original") : "a/b";
    $s("#smAB").setAttribute("aria-pressed", ab ? "true" : "false"); $s("#smAB").disabled = false;
    $s("#smLoop").setAttribute("aria-pressed", S.loop ? "true" : "false"); $s("#smOne").setAttribute("aria-pressed", S.oneShot ? "true" : "false");
    $s("#smAtk").value = S.attack; $s("#smRel").value = S.release; $s("#smGain").value = S.gainDb;
    $s("#smUndo").disabled = !undo.length; $s("#smRedo").disabled = !redo.length;
    const n = S.slices.length + 1;
    $s("#smSliceInfo").textContent = S.mode === 1 ? `${n} slice${n > 1 ? "s" : ""} on ${noteName(S.sliceBase)}–${noteName(S.sliceBase + n - 1)}` : `${n} slice${n > 1 ? "s" : ""}`;
    $s("#smFit").disabled = !(an.bpm > 0);
    drawWave();
  }
  function init() {
    const smp = $("#smp");
    // import: file picker, folder, drag and drop anywhere on the panel
    $s("#smImport").onclick = () => PLUGIN ? (async () => { try { progress("choose a file…"); const r = await req("smpImport"); progress(null); if (r.cancelled) return; await loadLib(); renderLib(); note(r.dupe ? `${r.name} is already in your samples` : `${r.name} imported`); } catch (e) { progress(null); note(e.message, true); } })() : $("#smFileIn").click();
    $("#smFileIn").onchange = e => { const f = e.target.files; if (f && f.length) importFiles(f); e.target.value = ""; };
    $("#smFolderIn").onchange = e => { const f = [...(e.target.files || [])].filter(x => AUDIO_EXT.test(x.name)); folderFiles = f.map(x => ({ name: x.name, size: x.size, file: x })); renderLib(); note(f.length ? `${f.length} audio files: preview them, then import the ones you want` : "no audio files in that folder"); e.target.value = ""; };
    $s("#smFolder").onclick = openFolder;
    smp.addEventListener("dragover", e => { if (e.dataTransfer && [...e.dataTransfer.types].includes("Files")) { e.preventDefault(); smp.classList.add("drop"); } });
    smp.addEventListener("dragleave", e => { if (!smp.contains(e.relatedTarget)) smp.classList.remove("drop"); });
    smp.addEventListener("drop", e => { e.preventDefault(); smp.classList.remove("drop"); const f = e.dataTransfer && e.dataTransfer.files; if (f && f.length) { tab("lib"); importFiles(f); } });
    $s("#smSearch").oninput = e => { query = e.target.value; renderLib(); };
    $s("#smFav").onclick = () => { favOnly = !favOnly; $s("#smFav").setAttribute("aria-pressed", favOnly ? "true" : "false"); renderLib(); };
    document.querySelectorAll("#smTabs [data-t]").forEach(b => b.onclick = () => tab(b.dataset.t));
    document.querySelectorAll("#smLayer [data-m]").forEach(b => b.onclick = () => setLayer(+b.dataset.m));
    $s("#smPianoLvl").oninput = e => setLayer(layer.mode, +e.target.value, null); $s("#smSampLvl").oninput = e => setLayer(layer.mode, null, +e.target.value);
    $s("#smClose").onclick = () => open(false);
    // editor controls
    document.querySelectorAll("#smTools [data-tool]").forEach(b => b.onclick = () => { tool = b.dataset.tool; renderEdit(); });
    $s("#smMode").onchange = e => edit(s => { s.mode = +e.target.value; if (s.mode === 1 && !s.slices.length) s.slices = autoSlices(); });
    $s("#smRoot").onchange = e => edit(s => s.root = +e.target.value);
    $s("#smRootAuto").onclick = () => { const an = cur && cur.an; if (an && an.root > 0) edit(s => s.root = Math.round(an.root)); else note("no clear pitch was found in this sample", true); };
    $s("#smTune").onchange = e => edit(s => s.tune = +e.target.value);
    $s("#smStretch").onchange = e => edit(s => s.stretch = +e.target.value);
    $s("#smFit").onclick = () => { const an = cur && cur.an; const bpm = typeof FINDER !== "undefined" ? FINDER.state.bpm : 120; if (an && an.bpm > 0) edit(s => s.stretch = an.bpm / bpm); note(`stretched from ≈${an.bpm.toFixed(1)} to ${bpm} bpm (set the project tempo in chord blocks)`); };
    $s("#smRev").onchange = e => setReverse(+e.target.value);
    $s("#smAB").onclick = toggleAB;
    $s("#smLoop").onclick = () => edit(s => { s.loop = s.loop ? 0 : 1; if (s.loop && s.loopB - s.loopA > .999) { s.loopA = s.start + (s.end - s.start) * .25; s.loopB = s.end; } });
    $s("#smOne").onclick = () => edit(s => s.oneShot = s.oneShot ? 0 : 1);
    $s("#smAtk").onchange = e => edit(s => s.attack = +e.target.value); $s("#smRel").onchange = e => edit(s => s.release = +e.target.value); $s("#smGain").onchange = e => edit(s => s.gainDb = +e.target.value);
    $s("#smSliceAuto").onclick = () => edit(s => { s.slices = autoSlices(); });
    document.querySelectorAll("#smp [data-div]").forEach(b => b.onclick = () => edit(s => { const n = +b.dataset.div; s.slices = []; for (let i = 1; i < n; i++) s.slices.push(s.start + (s.end - s.start) * i / n); }));
    $s("#smSliceClear").onclick = () => edit(s => { s.slices = []; });
    $s("#smToPads").onclick = () => { if (!cur) return; edit(s => { s.mode = 1; if (!s.slices.length) s.slices = autoSlices(); }); if (layer.mode === 0) setLayer(1); if (typeof PADS !== "undefined") PADS.useSlices(S.sliceBase, S.slices.length + 1); WORKSPACE.open(true); WORKSPACE.tab("pads"); };
    $s("#smUndo").onclick = doUndo; $s("#smRedo").onclick = doRedo;
    $s("#smSavePreset").onclick = savePreset; $s("#smPresets").onchange = e => { if (e.target.value) loadPreset_(e.target.value); e.target.value = ""; };
    $s("#smExport").onclick = async () => { const r = await exportAudio(false); if (r && r.bytes) LUMI_EXPORT.saveWav(r.name, r.bytes); };
    LUMI_EXPORT.dragHandle($s("#smDrag"), () => cur ? { lazy: true, name: cur.name + " (edited)", get: () => exportAudio(true), native: { type: "smpExport", name: cur.name + " (edited).wav", drag: true } } : null);
    $s("#smEject").onclick = () => { if (!cur) return; cur = null; peaks = null; send({ type: PLUGIN ? "smpClear" : "sampleClear" }); if (!PLUGIN) store.set("lumi-sampler-current-v1", null); setLayer(0); renderEdit(); renderLib(); };
    // youtube
    $s("#ytCheck").onclick = youtubeCheck; $s("#ytUrl").onkeydown = e => { if (e.key === "Enter") youtubeCheck(); };
    // keyboard: undo / redo while the sampler is open
    document.addEventListener("keydown", e => { if ($("#smp").hidden || (e.target.closest && e.target.closest("input, select, textarea"))) return; if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") { e.preventDefault(); e.shiftKey ? doRedo() : doUndo(); } });
    $("#smpBtn").onclick = () => open($("#smp").hidden);
    waveSetup(); renderLayer();
    // restore the last session (browser) — the plugin restores from the project instead
    if (!PLUGIN) (async () => {
      const last = store.get("lumi-sampler-current-v1", null);
      await loadLib();
      if (last && last.id) { const e = lib.find(x => x.id === last.id); if (e) { const keepMode = layer.mode; await useEntry(e, Object.assign({}, DEF, last.settings)); setLayer(keepMode); } }
      else if (layer.mode !== 0) setLayer(0);
    })();
  }
  function autoSlices() {
    const an = cur && cur.an; if (!an || !an.onsets || !cur.frames) return [];
    const fr = an.onsets.map(f => f / cur.frames).filter(x => x > S.start + .002 && x < S.end - .002);
    let pts = fr.slice(0, 31);
    if (S.reverse === 1) pts = pts.map(x => 1 - x);
    return pts.length ? pts : [];
  }
  function open(on) {
    const p = $("#smp");
    if (on) { if (!$("#prog").hidden) WORKSPACE.open(false); if (typeof LIBS !== "undefined" && !$("#libs").hidden) LIBS.open(false); const det = $("#detail"); p.style.height = Math.max(420, $("#rack").offsetHeight + det.offsetHeight + 14) + "px"; }
    p.hidden = !on; $("#rack").style.visibility = on ? "hidden" : ""; $("#detail").style.display = on ? "none" : "";
    $("#smpBtn").setAttribute("aria-pressed", on ? "true" : "false");
    if (on) { loadLib().then(renderLib); renderPresets(); tab(cur ? "edit" : "lib"); } else stopPreview();
  }
  // native messages that are not replies (e.g. the project reopened with a sample)
  function onPluginEvent(m) {
    if (onNative(m)) return true;
    if (m.type === "smpState") {   // the plugin restored a sample from the project
      if (!m.loaded) { cur = null; peaks = null; if (m.error) note(m.error, true); renderEdit(); return true; }
      cur = { id: m.id, name: m.name, path: m.path, sr: m.sr, channels: m.channels, frames: m.frames, seconds: m.frames / m.sr, an: m.an };
      S = parseText(m.settings || ""); peaks = m.peaks || null; if (m.layer) { layer = m.layer; renderLayer(); }
      renderEdit(); return true;
    }
    if (m.type === "smpPreviewEnd") { if (pv && pv.btn) pv.btn.textContent = "▶"; pv = null; return true; }
    return false;
  }
  // audio export (browser OfflineAudioContext): the sampler takes part when it is loaded
  async function applyToNode(node, wait) {
    if (PLUGIN || !cur || !cur.audio) { node.port.postMessage({ type: "layer", mode: 0, piano: 1, sampler: .7 }); return; }
    const done = wait("sampleok");
    node.port.postMessage({ type: "sample", id: cur.id, L: cur.audio.L.slice(), R: cur.audio.R.slice(), sr: cur.audio.sr, channels: cur.audio.channels, settings: new TextEncoder().encode(toText(S) + "\0") });
    await done;
    node.port.postMessage({ type: "layer", mode: layer.mode, piano: layer.piano, sampler: layer.sampler });
  }
  return { init, open, tab, parseYouTube, youtubeCheck, importFiles, useEntry, loadLib, lib: () => lib, state: () => S, current: () => cur, layer: () => layer, setLayer,
           edit, setReverse, toggleAB, undo: doUndo, redo: doRedo, toText, parseText, peaksOf, processedOf, decode, onPluginEvent, applyToNode, savePreset, loadPreset: loadPreset_, listPresets, exportAudio, peaks: () => peaks };
})();

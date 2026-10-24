// ================================================================ SOUND LIBRARIES: New Jazz, Disco + EDM + Club Bangers, Reggaetón, Dembow Dominicano
// Nine categories each. Drums / hi-hats / percussion / effects / textures are real audio samples (web/samples/*.flac);
// pads / bass / strings / keys are playable LUMELODY instruments. Patterns are note lists over the one-shots: they play
// at any tempo, layer together, and export as MIDI (GM drum notes) or rendered audio.
const LIBS = (() => {
  const M = typeof LIB_MANIFEST !== "undefined" ? LIB_MANIFEST : { libraries: [], items: [] };
  const LIBNAMES = { "Disco + EDM + Club": "Disco + EDM + Club Bangers" };
  const CATS = ["Drums", "Pads", "Hi-Hats", "Bass", "Percussion", "Effects", "Strings", "Textures", "Keys"];
  const INSTR = { Pads: 1, Bass: 1, Strings: 1, Keys: 1 };
  const byId = new Map(M.items.map(i => [i.id, i]));
  const S = Object.assign({ lib: M.libraries[0] || "New Jazz", cat: "Drums", q: "", favOnly: false, bpm: 0, open: false, vol: .9 }, store.get("lumi-libs-v1", {}));
  const favs = new Set(store.get("lumi-libs-favs-v1", []));
  const save = () => store.set("lumi-libs-v1", { lib: S.lib, cat: S.cat, favOnly: S.favOnly, bpm: S.bpm, open: !$("#libs").hidden, vol: S.vol });
  let groove = [];   // [{id, vol}] pattern layers that play together

  // ---------------------------------------------------------------- audio (browser): decode once, play through Web Audio
  const buffers = new Map();
  async function buf(id) {
    if (buffers.has(id)) return buffers.get(id);
    const it = byId.get(id); if (!it || !it.file) throw new Error("missing sample " + id);
    if (!ctx) await startAudio();
    const p = fetch("samples/" + it.file).then(r => { if (!r.ok) throw new Error("sample file missing: " + it.file); return r.arrayBuffer(); }).then(b => (ctx.decodeAudioData ? ctx : new OfflineAudioContext(2, 1, 44100)).decodeAudioData(b));
    buffers.set(id, p);
    try { return await p; } catch (e) { buffers.delete(id); throw e; }
  }
  let master = null, live = [];
  function out() { if (!master && ctx && ctx.createGain) { master = ctx.createGain(); master.connect(ctx.destination); } if (master) master.gain.value = S.vol; return master; }
  function stopAll() {
    for (const s of live) { try { s.stop(); } catch (e) {} }
    live = []; clearInterval(seqTimer); seqTimer = 0; playing = null;
    if (PLUGIN) send({ type: "libStop" });
    renderPlayState();
  }
  let playing = null, seqTimer = 0;
  async function playOne(id) {
    stopAll();
    const it = byId.get(id);
    if (PLUGIN) { send({ type: "libPlay", id, loop: it.kind === "loop", vol: S.vol }); playing = { id }; renderPlayState(); return; }
    const b = await buf(id), src = ctx.createBufferSource(); src.buffer = b; src.loop = it.kind === "loop"; src.connect(out()); src.start();
    live.push(src); playing = { id }; src.onended = () => { if (playing && playing.id === id && !src.loop) { playing = null; renderPlayState(); } };
    renderPlayState();
  }
  // patterns: schedule one-shots ahead of time on the audio clock; several layers loop together
  const bpmOf = ids => S.bpm || (byId.get(ids[0]) || {}).bpm || 120;
  function swingAt(b, sw) { const six = Math.round(b * 4 * 1e4) / 1e4; return sw && Math.abs(six - Math.round(six)) < 1e-4 && Math.round(six) % 2 === 1 ? b + sw * .25 : b; }
  async function playPatterns(layers, key) {
    stopAll();
    const ids = layers.map(l => l.id), bpm = bpmOf(ids);
    if (PLUGIN) { send({ type: "libPattern", layers: layers.map(l => ({ events: byId.get(l.id).events, beats: byId.get(l.id).beats, swing: byId.get(l.id).swing || 0, vol: l.vol })), bpm, loop: true, vol: S.vol }); playing = { key }; renderPlayState(); return; }
    const need = [...new Set(layers.flatMap(l => byId.get(l.id).events.map(e => e.s)))];
    await Promise.all(need.map(buf));
    const spb = 60 / bpm, t0 = ctx.currentTime + .08, len = Math.max(...layers.map(l => byId.get(l.id).beats));
    const next = layers.map(() => 0);
    const tick = () => {
      const now = ctx.currentTime;
      layers.forEach((l, li) => {
        const p = byId.get(l.id), loopSecs = p.beats * spb;
        while (t0 + next[li] * loopSecs < now + .3) {
          const base = t0 + next[li] * loopSecs;
          for (const e of p.events) {
            const at = base + swingAt(e.b, p.swing) * spb; if (at < now - .01) continue;
            buffers.get(e.s).then(b => { const src = ctx.createBufferSource(), g = ctx.createGain(); src.buffer = b; g.gain.value = e.v * l.vol; src.connect(g); g.connect(out()); src.start(at); live.push(src); src.onended = () => { const k = live.indexOf(src); if (k >= 0) live.splice(k, 1); }; });
          }
          next[li]++;
        }
      });
    };
    tick(); seqTimer = setInterval(tick, 80); playing = { key, t0, bpm, len };
    renderPlayState();
  }
  // render patterns (one cycle of the longest layer) to audio
  async function renderPatterns(layers, cycles = 1) {
    const ids = layers.map(l => l.id), bpm = bpmOf(ids), spb = 60 / bpm, len = Math.max(...layers.map(l => byId.get(l.id).beats));
    const need = [...new Set(layers.flatMap(l => byId.get(l.id).events.map(e => e.s)))];
    const bufs = new Map(await Promise.all(need.map(async id => [id, await buf(id)])));
    const secs = len * cycles * spb + 2.5, oc = new (window.OfflineAudioContext || window.webkitOfflineAudioContext)(2, Math.ceil(secs * 44100), 44100);
    for (const l of layers) {
      const p = byId.get(l.id);
      for (let c = 0; c < Math.round(len * cycles / p.beats); c++) for (const e of p.events) {
        const src = oc.createBufferSource(), g = oc.createGain(); src.buffer = bufs.get(e.s); g.gain.value = e.v * l.vol; src.connect(g); g.connect(oc.destination);
        src.start((c * p.beats + swingAt(e.b, p.swing)) * spb);
      }
    }
    const r = await oc.startRendering();
    return LUMI_EXPORT.wavBytes(r.getChannelData(0), r.getChannelData(1), 44100);
  }
  function patternMidi(layers) {
    const ids = layers.map(l => l.id), bpm = bpmOf(ids), len = Math.max(...layers.map(l => byId.get(l.id).beats));
    const tracks = layers.map(l => {
      const p = byId.get(l.id), ev = [];
      for (let c = 0; c < Math.round(len / p.beats); c++) for (const e of p.events) { const o = byId.get(e.s); ev.push({ t: c * p.beats + swingAt(e.b, p.swing), d: .12, n: o.note || 36, vel: e.v * l.vol }); }
      return { name: p.name + " (GM drums)", ev, channel: 9 };
    });
    return LUMI_EXPORT.midi(tracks, bpm);
  }
  async function sampleWav(id) {   // the sample's own audio, exactly as stored (decoded losslessly from FLAC)
    const b = await buf(id); return LUMI_EXPORT.wavBytes(b.getChannelData(0), b.numberOfChannels > 1 ? b.getChannelData(1) : b.getChannelData(0), b.sampleRate);
  }

  // ---------------------------------------------------------------- UI
  const presetsOf = (lib, kind) => PRESETS.filter(p => p.cat === lib && p.kind === kind);
  function itemsFor() {
    const q = S.q.trim().toLowerCase();
    let list;
    if (INSTR[S.cat]) list = presetsOf(S.lib, S.cat).map(p => ({ kind: "instrument", id: "preset:" + p.id, name: p.name, desc: p.desc, preset: p }));
    else list = M.items.filter(i => i.lib === S.lib && i.cat === S.cat && !i.hidden);
    if (q) list = (INSTR[S.cat] ? list : M.items.filter(i => i.lib === S.lib && !i.hidden).concat(PRESETS.filter(p => p.cat === S.lib).map(p => ({ kind: "instrument", id: "preset:" + p.id, name: p.name, desc: p.desc, preset: p }))))
      .filter(i => (i.name + " " + (i.desc || "") + " " + (i.cat || i.preset && i.preset.kind || "")).toLowerCase().includes(q));
    if (S.favOnly) list = list.filter(i => favs.has(i.id));
    return list;
  }
  function render() {
    document.querySelectorAll("#libTabs2 [data-lib]").forEach(b => b.setAttribute("aria-pressed", b.dataset.lib === S.lib ? "true" : "false"));
    document.querySelectorAll("#libCats [data-cat]").forEach(b => b.setAttribute("aria-pressed", b.dataset.cat === S.cat ? "true" : "false"));
    $("#libFav").setAttribute("aria-pressed", S.favOnly ? "true" : "false");
    $("#libBpm").value = S.bpm || ""; $("#libBpm").placeholder = "auto"; $("#libVol").value = S.vol;
    const list = itemsFor();
    $("#libItems").replaceChildren(...(list.length ? list.map(card) : [el("p", { className: "smempty", textContent: S.favOnly ? "no favourites here yet: tap ☆ on a sound" : "nothing matches" })]));
    renderGroove(); renderPlayState(); warm(list);
  }
  // decode what is on screen while the person looks at it, so the first preview starts at once (idle time, one at a time)
  let warmQ = [];
  function warm(list) {
    const ids = [...new Set(list.flatMap(i => i.kind === "pattern" ? i.events.map(e => e.s) : i.kind === "instrument" ? [] : [i.id]))].filter(id => !buffers.has(id) && !warmed.has(id));
    warmQ = ids;
    if (PLUGIN) { if (ids.length) send({ type: "libWarm", ids }); ids.forEach(id => warmed.add(id)); return; }
    const step = () => { const id = warmQ.shift(); if (!id || $("#libs").hidden || !ctx) return; buf(id).catch(() => {}).then(() => idle(step)); };
    for (let k = 0; k < 4; k++) idle(step);   // four at a time: enough to hide the download, light on the CPU
  }
  const warmed = new Set();
  const idle = f => (window.requestIdleCallback ? requestIdleCallback(f, { timeout: 300 }) : setTimeout(f, 30));
  const kindLabel = i => i.kind === "instrument" ? "instrument" : i.kind === "pattern" ? `pattern · ${i.bpm} bpm · MIDI + audio` : i.kind === "loop" ? "loop" : "one-shot";
  function card(i) {
    const star = el("button", { className: "smstar" + (favs.has(i.id) ? " on" : ""), type: "button", textContent: favs.has(i.id) ? "★" : "☆", title: "Favourite" });
    star.onclick = () => { favs.has(i.id) ? favs.delete(i.id) : favs.add(i.id); store.set("lumi-libs-favs-v1", [...favs]); render(); };
    const play = el("button", { className: "btn lplay", type: "button", textContent: "▶", title: i.kind === "instrument" ? "Hear the demo" : "Preview" });
    play.dataset.id = i.id;
    // starts on press (not on release) so the sound comes as soon as the finger lands; Enter / Space still use click
    let pressed = false;
    play.onpointerdown = e => { if (e.button) return; pressed = true; togglePlay(i); };
    play.onclick = () => { if (pressed) { pressed = false; return; } togglePlay(i); };
    if (i.kind === "instrument" && !PLUGIN) play.onpointerenter = () => { if (!demoCache.has(i.preset.id)) { const a = new Audio(); a.preload = "auto"; a.src = "audio/" + i.preset.id + ".mp3"; demoCache.set(i.preset.id, a); } };
    const acts = [play];
    if (i.kind === "instrument") {
      const use = el("button", { className: "btn hot", type: "button", textContent: PRESETS[cur] && "preset:" + PRESETS[cur].id === i.id ? "playing on keys" : "play it", title: "Load it on the keys, the pads and MIDI" });
      use.onclick = () => { const k = PRESETS.indexOf(i.preset); if (k >= 0) { loadPreset(k); if (typeof SAMPLER !== "undefined" && SAMPLER.layer().mode === 1) SAMPLER.setLayer(0); render(); toast("Sound: " + i.name); } };
      acts.push(use);
    } else {
      if (i.kind === "pattern") {
        const inG = groove.some(g => g.id === i.id);
        const add = el("button", { className: "btn" + (inG ? " hot" : ""), type: "button", textContent: inG ? "in groove" : "+ groove", title: "Layer it with other patterns" });
        add.onclick = () => { if (inG) groove = groove.filter(g => g.id !== i.id); else groove.push({ id: i.id, vol: .9 }); if (groove.length >= 2) window.dispatchEvent(new CustomEvent("lumi:milestone", { detail: "groove" })); render(); if (playing && playing.key === "groove") playPatterns(groove, "groove"); };
        acts.push(add);
        const dm = el("button", { className: "btn drag", type: "button", textContent: "midi ↗", title: "Drag the pattern as MIDI (GM drum notes). Click to save." });
        LUMI_EXPORT.dragHandle(dm, () => ({ name: `LUMELODY ${i.name} ${bpmOf([i.id])} bpm`, bytes: patternMidi([{ id: i.id, vol: 1 }]) }));
        acts.push(dm);
      }
      const dw = el("button", { className: "btn drag", type: "button", textContent: "wav ↗", title: "Drag the audio into FL Studio. Click to save." });
      LUMI_EXPORT.dragHandle(dw, () => i.kind === "pattern"
        ? { lazy: true, name: `LUMELODY ${i.name} ${bpmOf([i.id])} bpm`, get: async () => ({ name: `LUMELODY ${i.name} ${bpmOf([i.id])} bpm`, bytes: await renderPatterns([{ id: i.id, vol: 1 }]) }), native: { type: "libExport", layers: [{ events: i.events, beats: i.beats, swing: i.swing || 0, vol: 1 }], bpm: bpmOf([i.id]), name: `LUMELODY ${i.name} ${bpmOf([i.id])} bpm.wav`, drag: true } }
        : { lazy: true, name: "LUMELODY " + i.name, get: async () => ({ name: "LUMELODY " + i.name, bytes: await sampleWav(i.id) }), native: { type: "libExport", id: i.id, name: `LUMELODY ${i.name}.wav`, drag: true } });
      acts.push(dw);
    }
    const c = el("div", { className: "lcard" + (playing && (playing.id === i.id) ? " on" : "") }, [
      el("div", { className: "lhead" }, [star, el("b", { textContent: i.name }), el("small", { className: "ltag", textContent: kindLabel(i) })]),
      el("p", { className: "ldesc", textContent: i.desc || "" }),
      el("div", { className: "lacts" }, acts)]);
    c.dataset.id = i.id;
    return c;
  }
  let demo = null; const demoCache = new Map();
  async function togglePlay(i) {
    if (playing && playing.id === i.id) { stopAll(); if (demo) { demo.pause(); demo = null; } return; }
    if (demo) { demo.pause(); demo = null; }
    try {
      if (i.kind === "instrument") {
        stopAll();
        if (PLUGIN) { toast("Load it with “play it”, then play the keys or your piano roll"); return; }
        demo = demoCache.get(i.preset.id) || new Audio("audio/" + i.preset.id + ".mp3"); demo.currentTime = 0; demo.volume = Math.min(1, S.vol); demo.play().catch(() => toast("the demo could not play here"));
        playing = { id: i.id }; demo.onended = () => { playing = null; renderPlayState(); }; renderPlayState(); return;
      }
      if (i.kind === "pattern") { await playPatterns([{ id: i.id, vol: 1 }], i.id); playing.id = i.id; renderPlayState(); return; }
      await playOne(i.id);
    } catch (e) { toast(e.message); }
  }
  function renderPlayState() {
    document.querySelectorAll("#libItems .lplay").forEach(b => { const on = playing && playing.id === b.dataset.id; b.textContent = on ? "■" : "▶"; b.closest(".lcard").classList.toggle("on", !!on); });
    const gp = $("#grvPlay"); if (gp) gp.textContent = playing && playing.key === "groove" ? "stop" : "play groove";
  }
  function renderGroove() {
    const g = $("#grvLayers"); if (!g) return;
    $("#libGroove").hidden = !groove.length;
    g.replaceChildren(...groove.map(l => {
      const p = byId.get(l.id), v = el("input", { type: "range", min: 0, max: 1.2, step: .01, value: l.vol, title: "Layer volume" });
      v.oninput = () => { l.vol = +v.value; };
      v.onchange = () => { if (playing && playing.key === "groove") playPatterns(groove, "groove"); };
      const x = el("button", { className: "chip", type: "button", textContent: "×", title: "Remove layer" }); x.onclick = () => { groove = groove.filter(q => q !== l); render(); if (playing && playing.key === "groove") (groove.length ? playPatterns(groove, "groove") : stopAll()); };
      return el("span", { className: "glayer" }, [el("b", { textContent: p.name }), v, x]);
    }));
  }
  // the moon becomes a disco ball while the Disco + EDM + Club Bangers library is open
  function mood() { if (typeof WORKSPACE !== "undefined") WORKSPACE.mood(); }
  const discoActive = () => !$("#libs").hidden && S.lib === "Disco + EDM + Club";
  function open(on) {
    const p = $("#libs");
    if (on) { if (!$("#prog").hidden) WORKSPACE.open(false); if (!$("#smp").hidden) SAMPLER.open(false); p.style.height = Math.max(420, $("#rack").offsetHeight + $("#detail").offsetHeight + 14) + "px"; }
    p.hidden = !on; $("#rack").style.visibility = on ? "hidden" : ""; $("#detail").style.display = on ? "none" : "";
    $("#libsBtn").setAttribute("aria-pressed", on ? "true" : "false");
    if (on) render(); else { stopAll(); if (demo) { demo.pause(); demo = null; } }
    save(); mood();
  }
  function setLib(l) { stopAll(); S.lib = l; save(); render(); mood(); }
  function init() {
    $("#libTabs2").replaceChildren(...M.libraries.map(l => { const b = el("button", { className: "chip", type: "button", textContent: LIBNAMES[l] || l }); b.dataset.lib = l; b.onclick = () => setLib(l); return b; }));
    $("#libCats").replaceChildren(...CATS.map(c => { const b = el("button", { className: "chip", type: "button", textContent: c.toLowerCase() }); b.dataset.cat = c; b.onclick = () => { S.cat = c; save(); render(); }; return b; }));
    $("#libSearch").oninput = e => { S.q = e.target.value; render(); };
    $("#libFav").onclick = () => { S.favOnly = !S.favOnly; save(); render(); };
    $("#libBpm").onchange = e => { const v = +e.target.value; S.bpm = v ? Math.max(60, Math.min(200, v)) : 0; e.target.value = S.bpm || ""; save(); if (playing && playing.key) { const k = playing.key; if (k === "groove") playPatterns(groove, "groove"); else { const it = byId.get(k); if (it) playPatterns([{ id: k, vol: 1 }], k).then(() => { playing.id = k; renderPlayState(); }); } } };
    $("#libVol").oninput = e => { S.vol = +e.target.value; if (master) master.gain.value = S.vol; if (demo) demo.volume = Math.min(1, S.vol); if (PLUGIN) send({ type: "libVol", vol: S.vol }); };
    $("#libVol").onchange = save;
    $("#libStop").onclick = () => { stopAll(); if (demo) { demo.pause(); demo = null; } };
    $("#libClose").onclick = () => open(false);
    $("#grvPlay").onclick = () => { if (playing && playing.key === "groove") stopAll(); else if (groove.length) playPatterns(groove, "groove"); };
    $("#grvClear").onclick = () => { groove = []; stopAll(); render(); };
    LUMI_EXPORT.dragHandle($("#grvMidi"), () => groove.length ? { name: `LUMELODY groove ${bpmOf(groove.map(g => g.id))} bpm`, bytes: patternMidi(groove) } : null);
    LUMI_EXPORT.dragHandle($("#grvWav"), () => groove.length ? { lazy: true, name: `LUMELODY groove ${bpmOf(groove.map(g => g.id))} bpm`, get: async () => ({ name: `LUMELODY groove ${bpmOf(groove.map(g => g.id))} bpm`, bytes: await renderPatterns(groove) }),
      native: { type: "libExport", layers: groove.map(l => ({ events: byId.get(l.id).events, beats: byId.get(l.id).beats, swing: byId.get(l.id).swing || 0, vol: l.vol })), bpm: bpmOf(groove.map(g => g.id)), name: `LUMELODY groove ${bpmOf(groove.map(g => g.id))} bpm.wav`, drag: true } } : null);
    $("#libsBtn").onclick = () => open($("#libs").hidden);
    if (S.open) setTimeout(() => open(true), 0);   // reopening the plugin/page comes back to the same library (and moon)
  }
  function onPluginEvent(m) {
    if (m.type === "libEnd") { if (playing) { playing = null; renderPlayState(); } return true; }
    if (m.type === "libError") { playing = null; renderPlayState(); toast(m.error || "could not play that sound"); return true; }
    return false;
  }
  return { init, open, onPluginEvent, setLib, render, discoActive, state: S, manifest: M, renderPatterns, patternMidi, sampleWav, playPatterns, stopAll, groove: () => groove, byId };
})();

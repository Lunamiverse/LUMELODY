// ================================================================ ARPEGGIATOR (fourth mode of the Chord Progression Finder)
// Notes come from the chord blocks, from a Neon Pads take, from the keys / MIDI keyboard you hold, or are generated.
// Every step is computed from (settings, seed): what plays is exactly what exports, and a loop repeats identically.
const ARP = (() => {
  const H = HARMONY;
  const RATES = [["1/4", 1], ["1/8", .5], ["1/8T", 1 / 3], ["1/16", .25], ["1/16T", 1 / 6], ["1/32", .125]];
  const PATTERNS = [["up", "up"], ["down", "down"], ["updown", "up-down"], ["random", "random"], ["played", "as played"], ["octave", "octave jumps"], ["rhythm", "rhythmic"], ["custom", "custom"]];
  const SOURCES = [["blocks", "chord blocks"], ["pads", "neon pads take"], ["live", "keys & midi"], ["gen", "generate"]];
  const GENRES = {   // starting points, not rules
    rnb:       { name: "R&B",        pattern: "updown", rate: "1/16",  octaves: 2, gate: .62, swing: .22, velVar: .35, complexity: 2, bpm: 82,  sound: ["after-hours-ep", "velvet-rhodes", "underwater-keys"] },
    trap:      { name: "Trap",       pattern: "rhythm", rate: "1/16T", octaves: 2, gate: .4,  swing: 0,   velVar: .3,  complexity: 3, bpm: 140, sound: ["melt-bell", "drill-bell", "lucid-arp"] },
    disco:     { name: "Disco",      pattern: "octave", rate: "1/8",   octaves: 1, gate: .5,  swing: .08, velVar: .2,  complexity: 2, bpm: 120, sound: ["disco-rhodes", "velvet-stabs", "lucid-arp"] },
    club:      { name: "Club / EDM", pattern: "up",     rate: "1/16",  octaves: 3, gate: .34, swing: 0,   velVar: .15, complexity: 2, bpm: 126, sound: ["lucid-arp", "2000s-synth-lead", "obsidian-pluck"] },
    jazz:      { name: "New Jazz",   pattern: "played", rate: "1/8T",  octaves: 2, gate: .72, swing: .35, velVar: .45, complexity: 3, bpm: 84,  sound: ["glass-keys", "velvet-rhodes", "late-set-piano"] },
    reggaeton: { name: "Reggaetón",  pattern: "custom", rate: "1/16",  octaves: 2, gate: .45, swing: 0,   velVar: .25, complexity: 2, bpm: 94,  sound: ["perreo-pluck", "calle-keys", "barrio-bell"], grid: "3-3-2" },
    dembow:    { name: "Dembow",     pattern: "rhythm", rate: "1/16",  octaves: 2, gate: .38, swing: 0,   velVar: .3,  complexity: 4, bpm: 118, sound: ["barrio-bell", "perreo-pluck", "romantico-pluck"] },
    ambient:   { name: "Ambient",    pattern: "up",     rate: "1/8",   octaves: 3, gate: 1,   swing: 0,   velVar: .2,  complexity: 1, bpm: 72,  sound: ["blue-hour-pad", "glass-keys", "melt-bell"] },
  };
  const GEN = {   // roman templates for "generate"
    rnb: ["ii7 V7 Imaj7 vi7", "Imaj7 vi7 ii7 V7", "IVmaj7 iii7 ii7 Imaj7"], trap: ["i bVI bIII bVII", "i bVI iv V", "i iv bVI V"],
    disco: ["i7 iv7 bVII bIIImaj7", "ii7 V7 Imaj7 vi7"], club: ["vi IV I V", "i bVI bIII bVII"], jazz: ["ii9 V13 Imaj9 VI7", "iim9 bII7 Imaj9 vi9"],
    reggaeton: ["i bVI bIII bVII", "i iv bVII bIII"], dembow: ["i bVII bVI bVII", "i bVI bVII i"], ambient: ["Imaj7 IVmaj7", "Iadd9 vi7 IVmaj7 Iadd9"],
  };
  const DEF = { mode: "beginner", source: "blocks", genre: "rnb", pattern: "updown", rate: "1/16", octaves: 2, gate: .62, swing: .22, velVar: .35, complexity: 2,
                bars: 0, latch: true, bpm: 82, seed: 1, grid: null, keep: { notes: true, rhythm: false, sound: false }, gen: { tonic: 9, style: "rnb", t: 0 } };
  const S = Object.assign(JSON.parse(JSON.stringify(DEF)), store.get("lumi-arp-v1", {}));
  const favs = store.get("lumi-arp-favs-v1", []);
  let before = null, showing = "now";   // compare: the settings before the last surprise / regenerate / preset
  const save = () => store.set("lumi-arp-v1", S);
  const SETTINGS = ["genre", "pattern", "rate", "octaves", "gate", "swing", "velVar", "complexity", "bars", "bpm", "seed", "grid", "gen"];
  const snap = () => JSON.parse(JSON.stringify(Object.fromEntries(SETTINGS.map(k => [k, S[k]]))));
  const hr = (a, b, c) => { let x = (Math.imul(a | 0, 374761393) + Math.imul(b | 0, 668265263) + Math.imul(c | 0, 1442695041)) | 0; x = Math.imul(x ^ (x >>> 13), 1274126177); x ^= x >>> 16; return (x >>> 0) / 4294967296; };

  // ---------------------------------------------------------------- input: a timeline of chords [{notes, t, d}] (beats)
  const live = { held: new Map(), order: [], latched: [] };
  function genChords() {
    const tpl = GEN[S.gen.style] || GEN.rnb, line = tpl[S.gen.t % tpl.length];
    const OFF = { I: 0, II: 2, III: 4, IV: 5, V: 7, VI: 9, VII: 11 };
    return line.split(" ").map(tok => {
      const m = /^(b?)(I{1,3}|IV|V|VI{0,2}|VII|i{1,3}|iv|v|vi{0,2}|vii)(.*)$/.exec(tok);
      const minor = m[2] === m[2].toLowerCase(), root = (S.gen.tonic + OFF[m[2].toUpperCase()] - (m[1] ? 1 : 0) + 12) % 12;
      let q = m[3]; if (minor && !q) q = "m"; else if (minor && /^[79]$/.test(q)) q = "m" + q; else if (minor && q.startsWith("m") === false && q) q = "m" + q.replace(/^m/, "");
      if (!(q in H.QUAL)) q = minor ? "m" : "";
      return H.name(root, false) + q;
    });
  }
  function source() {   // -> {chords:[{notes, t, d}], beats, label}
    if (S.source === "live") {
      const notes = live.held.size ? live.order.filter(n => [...live.held.values()].includes(n)) : (S.latch ? live.latched : []);
      return { chords: [{ notes, t: 0, d: 4 }], beats: 4, label: notes.length ? notes.map(n => H.name(n % 12, false) + (Math.floor(n / 12) - 1)).join(" ") : "hold notes on the keys or a MIDI keyboard", live: true };
    }
    let blocks = null;
    if (S.source === "blocks") blocks = (FINDER.state.blocks || []).map(b => ({ sym: b.sym, dur: b.dur }));
    if (S.source === "gen") blocks = genChords().map(sym => ({ sym, dur: 4 }));
    if (blocks) {
      if (!blocks.length) return { chords: [], beats: 0, label: "no chord blocks yet: make some in chord blocks, or choose generate" };
      const sc = H.chordEvents(blocks, {}); let t = 0;
      const chords = sc.voicings.map((v, i) => { const c = { notes: v.lhNotes.slice(0, 1).concat(v.up), t, d: blocks[i].dur, sym: blocks[i].sym }; t += blocks[i].dur; return c; });
      return { chords, beats: t, label: blocks.map(b => b.sym).join("  ·  ") };
    }
    // neon pads take: group the recorded notes that start together into chords
    const sc = PADS.score ? PADS.score() : null, ev = sc && sc.ev ? sc.ev.slice().sort((a, b) => a.t - b.t) : [];
    if (!ev.length) return { chords: [], beats: 0, label: "no neon pads take yet: record one in neon pads" };
    const chords = [];
    for (const e of ev) { const last = chords[chords.length - 1]; if (last && e.t - last.t < .12) { if (!last.notes.includes(e.n)) last.notes.push(e.n); } else chords.push({ notes: [e.n], t: e.t, d: 0 }); }
    chords.forEach((c, i) => c.d = (i + 1 < chords.length ? chords[i + 1].t : Math.max(sc.beats, c.t + 1)) - c.t);
    return { chords, beats: Math.max(sc.beats, 1), label: `${chords.length} chords / notes from your take` };
  }

  // ---------------------------------------------------------------- the pattern engine
  const rateOf = () => (RATES.find(r => r[0] === S.rate) || RATES[3])[1];
  const GRID_ROWS = 6;
  function defaultGrid(kind) {
    const g = Array.from({ length: 16 }, () => -1);
    if (kind === "3-3-2") [0, 3, 6, 8, 11, 14].forEach((s, i) => g[s] = [0, 1, 2, 1, 0, 2][i]);
    else for (let s = 0; s < 16; s++) g[s] = s % 4 === 3 ? -1 : [0, 1, 2, 3][s % 4] ;
    return g;
  }
  function rhythmMask(seed) {   // 16 steps; the downbeat always plays; ties lengthen notes
    const m = []; for (let s = 0; s < 16; s++) { const p = s === 0 ? 1 : s % 4 === 0 ? .85 : s % 2 === 0 ? .6 : .45; m.push(hr(seed, s, 77) < p ? 1 : 0); }
    return m;
  }
  function orderNotes(notes, played) {
    const base = played ? notes.slice() : notes.slice().sort((a, b) => a - b), out = [];
    for (let o = 0; o < S.octaves; o++) for (const n of base) out.push(n + 12 * o);
    return out.filter(n => n >= 21 && n <= 108);
  }
  function stepNote(list, k, s, seed) {   // k = steps since the chord started
    const L = list.length; if (!L) return null;
    switch (S.pattern) {
      case "down": return list[L - 1 - (k % L)];
      case "updown": { if (L < 2) return list[0]; const p = 2 * L - 2, i = k % p; return list[i < L ? i : p - i]; }
      case "random": return list[Math.floor(hr(seed, s, 11) * L)];
      case "octave": { const base = list.filter((_, i) => i < Math.max(1, L / S.octaves)); const n = base[Math.floor(k / 2) % base.length]; return k % 2 ? Math.min(108, n + 12) : n; }
      default: return list[k % L];   // up, as played, rhythmic
    }
  }
  // events for the beat range [b0, b1) of the loop
  function eventsIn(src, b0, b1, seedIn) {
    const seed = seedIn != null ? seedIn : S.seed, r = rateOf(), ev = [], loopLen = loopBeats(src);
    if (!src.chords.length || !loopLen) return ev;
    const mask = S.pattern === "rhythm" ? rhythmMask(seed) : null, grid = S.pattern === "custom" ? (S.grid || defaultGrid()) : null;
    const s0 = Math.ceil(b0 / r - 1e-6), s1 = Math.ceil(b1 / r - 1e-6);
    for (let s = s0; s < s1; s++) {
      const t = s * r, tl = ((t % loopLen) + loopLen) % loopLen, srcT = src.beats ? tl % src.beats : 0;
      const ci = src.chords.findIndex(c => srcT >= c.t - 1e-6 && srcT < c.t + c.d - 1e-6); if (ci < 0) continue;
      const c = src.chords[ci], list = orderNotes(c.notes, S.pattern === "played"); if (!list.length) continue;
      const k = Math.round((srcT - c.t) / r), step16 = ((s % 16) + 16) % 16;
      let n, d = Math.max(.03, r * S.gate), vel = .7;
      if (mask) { if (!mask[step16]) continue; let tie = 1; while (tie < 4 && !mask[(step16 + tie) % 16] && hr(seed, s + tie, 33) < .5) tie++; d = Math.max(.03, r * (tie - 1 + S.gate)); }
      if (grid) { const g = grid[step16]; if (g == null || g < 0) continue; n = list[g % list.length] + (g >= list.length && list.length < GRID_ROWS ? 12 : 0); }
      else n = stepNote(list, k, s, seed);
      if (n == null) continue;
      // complexity: accents, octave leaps, rests, ratchets, neighbour tones; all from the seed
      const cx = S.complexity;
      if (cx >= 4 && hr(seed, s, 5) < .08 * (cx - 3) && k > 0) continue;
      if (cx >= 3 && hr(seed, s, 6) < .1 * (cx - 2)) n = Math.min(108, n + 12);
      if (cx >= 5 && hr(seed, s, 7) < .12) n = list[(list.indexOf(n) + 1) % list.length] || n;
      const onBeat = Math.abs(t - Math.round(t)) < 1e-6;
      vel = .66 + (onBeat ? .08 + .03 * cx : 0) + (hr(seed, s, 9) * 2 - 1) * .3 * S.velVar;
      const sw = s % 2 ? S.swing * r * .5 : 0;
      const rat = cx >= 4 && hr(seed, s, 8) < .06 * (cx - 3);
      if (rat) { ev.push({ t: t + sw, d: Math.max(.03, d / 2 - .01), n, vel: Math.max(.1, Math.min(1, vel)), track: 0 }); ev.push({ t: t + sw + r / 2, d: Math.max(.03, d / 2 - .01), n, vel: Math.max(.1, Math.min(1, vel * .8)), track: 0 }); }
      else ev.push({ t: t + sw, d: Math.min(d, r * 4), n, vel: Math.max(.1, Math.min(1, vel)), track: 0 });
    }
    return ev;
  }
  const loopBeats = src => S.bars ? S.bars * 4 : Math.max(4, Math.ceil((src.beats || 4) / 4) * 4);
  function score(seed) { const src = source(), L = loopBeats(src); return { ev: eventsIn(src, 0, L, seed), beats: L, src }; }

  // ---------------------------------------------------------------- playback: scheduled one beat at a time, so edits are heard on the next beat
  let beatN = 0, firstCall = false;
  function chunk() {
    if (firstCall) { firstCall = false; return { ev: [], beats: 1 }; }
    const src = source(), L = loopBeats(src), b = beatN % L; beatN++;
    playhead = b;
    return { ev: eventsIn(src, b, b + 1).map(e => Object.assign({}, e, { t: e.t - b })), beats: 1 };
  }
  let playhead = 0;
  function play(on) {
    if (on === undefined) on = LUMI_PLAY.playing() !== "arp";
    if (!on) { LUMI_PLAY.stop(); sync(); return; }
    beatN = 0; firstCall = true;
    LUMI_PLAY.start("arp", chunk, { bpm: S.bpm, loop: true });
    sync();
  }
  function reset() { if (S.source === "live") { live.latched = []; live.held.clear(); live.order = []; } if (LUMI_PLAY.playing() === "arp") play(true); render(); }
  // keys / MIDI: while the arpeggiator listens, the notes you hold feed it instead of sounding directly
  const listening = () => !$("#arpPane").hidden && !$("#prog").hidden && S.source === "live";
  function capture(id, note, on) {
    if (!listening() || /^l[po]/.test(String(id))) return false;
    if (on) {
      if (!live.held.size) live.latched = [];
      live.held.set(id, note); if (!live.order.includes(note)) live.order.push(note);
      if (S.latch && !live.latched.includes(note)) live.latched.push(note);
      if (LUMI_PLAY.playing() !== "arp") play(true);
    } else {
      live.held.delete(id);
      if (!live.held.size) live.order = live.order.filter(n => live.latched.includes(n));
    }
    renderSrc();
    return true;
  }

  // ---------------------------------------------------------------- actions
  function remember() { before = snap(); showing = "now"; }
  function applySettings(o) { for (const k of SETTINGS) if (k in o) S[k] = JSON.parse(JSON.stringify(o[k])); }
  function useSound(ids) {
    const i = PRESETS.findIndex(p => ids.includes(p.id)); if (i >= 0 && i !== cur) loadPreset(i);
  }
  function genre(id, opt = {}) {
    const g = GENRES[id]; if (!g) return;
    if (!opt.noRemember) remember();
    S.genre = id;
    if (!(opt.keepRhythm && S.keep.rhythm)) { S.pattern = g.pattern; S.rate = g.rate; S.gate = g.gate; S.swing = g.swing; S.grid = g.grid ? defaultGrid(g.grid) : S.grid; }
    S.octaves = g.octaves; S.velVar = g.velVar; S.complexity = g.complexity; S.bpm = g.bpm;
    if (S.source === "gen" && !(opt.keepNotes && S.keep.notes)) S.gen.style = id;
    if (!S.keep.sound || !opt.keepSound) useSound(g.sound);
    save(); render(); if (LUMI_PLAY.playing() === "arp") play(true);
  }
  function surprise() {
    remember();
    const ids = Object.keys(GENRES), r = k => hr(Date.now() & 0xffff, k, 3);
    const g = ids[Math.floor(r(1) * ids.length)];
    genre(g, { noRemember: true, keepRhythm: true, keepNotes: true, keepSound: true });
    if (!S.keep.rhythm) { S.pattern = PATTERNS[Math.floor(r(2) * (PATTERNS.length - 1))][0]; S.rate = ["1/8", "1/16", "1/16", "1/16T", "1/8T"][Math.floor(r(4) * 5)]; S.gate = Math.round((.3 + r(5) * .6) * 100) / 100; S.swing = r(6) < .5 ? 0 : Math.round(r(7) * 40) / 100; }
    S.complexity = 1 + Math.floor(r(8) * 5); S.seed = 1 + Math.floor(r(9) * 99999);
    if (S.source === "gen" && !S.keep.notes) { S.gen.tonic = Math.floor(r(10) * 12); S.gen.t = Math.floor(r(11) * 3); }
    save(); render(); play(true);
  }
  function regenerate() {
    remember();
    S.seed = 1 + Math.floor(hr(S.seed, Date.now() & 0xffff, 1) * 99999);
    if (S.source === "gen" && !S.keep.notes) { S.gen.t++; }
    save(); render(); if (LUMI_PLAY.playing() === "arp") play(true);
  }
  function compare() {
    if (!before) { toast("nothing to compare yet: try a style, surprise me or regenerate"); return; }
    const now = snap(); applySettings(before); before = now; showing = showing === "now" ? "before" : "now";
    save(); render();
  }
  const fileName = () => `LUMELODY arp - ${GENRES[S.genre] ? GENRES[S.genre].name : "custom"} ${S.pattern} ${S.rate.replace("/", "-")} - ${S.bpm} bpm`;
  const midiBytes = () => { const sc = score(); return sc.ev.length ? LUMI_EXPORT.midi([{ name: "LUMELODY arpeggio", ev: sc.ev }], S.bpm) : null; };

  // ---------------------------------------------------------------- UI
  const $a = s => $("#arpPane " + s);
  function seg(node, items, curV, set) {
    node.replaceChildren(...items.map(([v, t]) => { const b = el("button", { type: "button", textContent: t }); b.setAttribute("aria-pressed", String(v) === String(curV) ? "true" : "false"); b.onclick = () => set(v); return b; }));
  }
  function setVal(k, v) { remember(); S[k] = v; save(); render(); }
  function renderSrc() { const src = source(); $a("#arpSrcInfo").textContent = src.label; drawRoll(); }
  function render() {
    const P = $("#arpPane"); if (!P) return;
    P.classList.toggle("beginner", S.mode === "beginner");
    seg($a("#arpMode"), [["beginner", "beginner"], ["advanced", "advanced"]], S.mode, v => { S.mode = v; save(); render(); });
    seg($a("#arpSrc"), SOURCES, S.source, v => { S.source = v; save(); render(); if (LUMI_PLAY.playing() === "arp") play(true); });
    $a("#arpGenRow").hidden = S.source !== "gen";
    $a("#arpGenKey").value = String(S.gen.tonic);
    $a("#arpGenres").replaceChildren(...Object.entries(GENRES).map(([id, g]) => { const b = el("button", { className: "chip", type: "button", textContent: g.name }); b.setAttribute("aria-pressed", S.genre === id ? "true" : "false"); b.onclick = () => genre(id, { keepRhythm: true, keepNotes: true, keepSound: true }); return b; }));
    seg($a("#arpPat"), PATTERNS, S.pattern, v => { setVal("pattern", v); if (v === "custom" && !S.grid) { S.grid = defaultGrid(); save(); render(); } });
    seg($a("#arpRate"), RATES.map(r => [r[0], r[0]]), S.rate, v => setVal("rate", v));
    seg($a("#arpOct"), [[1, "1"], [2, "2"], [3, "3"], [4, "4"]], S.octaves, v => setVal("octaves", +v));
    seg($a("#arpLen"), [[0, "source"], [1, "1 bar"], [2, "2"], [4, "4"], [8, "8"]], S.bars, v => setVal("bars", +v));
    for (const [id, k, f] of [["#arpGate", "gate", v => Math.round(v * 100) + "%"], ["#arpSwing", "swing", v => Math.round(v * 100) + "%"], ["#arpVel", "velVar", v => Math.round(v * 100) + "%"], ["#arpCx", "complexity", v => String(v)]]) {
      $a(id).value = S[k]; $a(id + "V").textContent = f(S[k]);
    }
    $a("#arpBpm").value = S.bpm;
    $a("#arpLatch").setAttribute("aria-pressed", S.latch ? "true" : "false"); $a("#arpLatch").hidden = S.source !== "live";
    for (const k of ["notes", "rhythm", "sound"]) $a(`[data-keep="${k}"]`).setAttribute("aria-pressed", S.keep[k] ? "true" : "false");
    $a("#arpCmp").textContent = before ? (showing === "now" ? "compare: hear before" : "compare: back to new") : "compare";
    $a("#arpCmp").disabled = !before;
    $a("#arpGridWrap").hidden = S.pattern !== "custom";
    if (S.pattern === "custom") drawGrid();
    $a("#arpFavs").replaceChildren(el("option", { value: "", textContent: favs.length ? "favourites…" : "no favourites yet" }), ...favs.map((f, i) => el("option", { value: String(i), textContent: f.name })));
    const snd = $a("#arpSound"); if (snd.options.length !== PRESETS.length) snd.replaceChildren(...PRESETS.map((p, i) => el("option", { value: String(i), textContent: p.name })));
    snd.value = String(cur);
    renderSrc(); sync();
  }
  function drawGrid() {
    const g = S.grid || defaultGrid(), w = $a("#arpGrid");
    w.replaceChildren(...Array.from({ length: GRID_ROWS }, (_, row) => el("div", { className: "agrow" }, [el("span", { className: "dim", textContent: "note " + (GRID_ROWS - row) }),
      ...Array.from({ length: 16 }, (_, s) => { const r = GRID_ROWS - 1 - row, c = el("button", { type: "button", className: "agcell" + (g[s] === r ? " on" : "") + (s % 4 === 0 ? " beat" : ""), title: `step ${s + 1}, note ${r + 1}` });
        c.onclick = () => { remember(); const gg = (S.grid || defaultGrid()).slice(); gg[s] = gg[s] === r ? -1 : r; S.grid = gg; save(); drawGrid(); drawRoll(); }; return c; })])));
  }
  function drawRoll() {
    const cv = $a("#arpRoll"); if (!cv || cv.offsetParent === null) return;
    const dpr = Math.min(2, window.devicePixelRatio || 1), W = cv.clientWidth, Hh = cv.clientHeight;
    cv.width = W * dpr; cv.height = Hh * dpr; const g = cv.getContext("2d"); g.scale(dpr, dpr); g.clearRect(0, 0, W, Hh);
    const sc = score(), ev = sc.ev; if (!ev.length) { g.fillStyle = "rgba(255,255,255,.4)"; g.font = "11px system-ui"; g.textAlign = "center"; g.fillText(sc.src.label, W / 2, Hh / 2); return; }
    const lo = Math.min(...ev.map(e => e.n)) - 1, hi = Math.max(...ev.map(e => e.n)) + 1, xb = W / sc.beats, yn = Hh / Math.max(6, hi - lo + 1);
    g.strokeStyle = "rgba(255,255,255,.08)"; for (let b = 0; b <= sc.beats; b++) { g.beginPath(); g.moveTo(b * xb, 0); g.lineTo(b * xb, Hh); g.lineWidth = b % 4 ? .5 : 1; g.stroke(); }
    for (const e of ev) { g.fillStyle = `rgba(255,255,255,${.35 + .6 * e.vel})`; g.fillRect(e.t * xb, Hh - (e.n - lo + 1) * yn, Math.max(1.5, e.d * xb - 1), Math.max(2, yn - 1)); }
    if (LUMI_PLAY.playing() === "arp") { g.fillStyle = "#fff"; g.fillRect(playhead * xb, 0, 1.5, Hh); }
  }
  function sync() { const on = LUMI_PLAY.playing() === "arp"; const b = $a("#arpPlay"); if (b) { b.textContent = on ? "stop" : "play"; b.setAttribute("aria-pressed", on ? "true" : "false"); } }
  function init() {
    const P = $("#arpPane"); if (!P) return;
    $a("#arpGenKey").replaceChildren(...Array.from({ length: 12 }, (_, i) => el("option", { value: String(i), textContent: H.name(i, false) + (/rnb|jazz|disco|ambient/.test(S.gen.style) ? " major" : " minor") })));
    $a("#arpGenKey").onchange = e => { remember(); S.gen.tonic = +e.target.value; save(); render(); };
    $a("#arpGenNew").onclick = () => { remember(); S.gen.t++; save(); render(); };
    $a("#arpPlay").onclick = () => play();
    $a("#arpReset").onclick = reset;
    $a("#arpSurprise").onclick = surprise;
    $a("#arpRegen").onclick = regenerate;
    $a("#arpCmp").onclick = compare;
    $a("#arpLatch").onclick = () => { S.latch = !S.latch; if (!S.latch) live.latched = []; save(); render(); };
    for (const [id, k, int] of [["#arpGate", "gate"], ["#arpSwing", "swing"], ["#arpVel", "velVar"], ["#arpCx", "complexity", 1]]) {
      $a(id).oninput = e => { S[k] = int ? Math.round(+e.target.value) : +e.target.value; $a(id + "V").textContent = int ? String(S[k]) : Math.round(S[k] * 100) + "%"; drawRoll(); };
      $a(id).onchange = () => { save(); };
      $a(id).onpointerdown = () => remember();
    }
    $a("#arpBpm").onchange = e => { S.bpm = Math.max(40, Math.min(240, +e.target.value || S.bpm)); e.target.value = S.bpm; save(); if (LUMI_PLAY.playing() === "arp") play(true); };
    P.querySelectorAll("[data-keep]").forEach(b => b.onclick = () => { S.keep[b.dataset.keep] = !S.keep[b.dataset.keep]; save(); render(); });
    $a("#arpSound").onchange = e => loadPreset(+e.target.value);
    $a("#arpFavSave").onclick = () => { const name = ($a("#arpFavName").value || "").trim() || `${GENRES[S.genre] ? GENRES[S.genre].name : "arp"} ${S.pattern} ${favs.length + 1}`; favs.push({ name, s: snap(), sound: PRESETS[cur] && PRESETS[cur].id }); store.set("lumi-arp-favs-v1", favs); $a("#arpFavName").value = ""; render(); toast("Saved to favourites: " + name); };
    $a("#arpFavs").onchange = e => { const f = favs[+e.target.value]; if (!f) return; remember(); applySettings(f.s); if (f.sound) useSound([f.sound]); save(); render(); if (LUMI_PLAY.playing() === "arp") play(true); };
    $a("#arpFavDel").onclick = () => { const i = +$a("#arpFavs").value; if (!$a("#arpFavs").value || !favs[i]) { toast("pick a favourite to remove"); return; } favs.splice(i, 1); store.set("lumi-arp-favs-v1", favs); render(); };
    LUMI_EXPORT.dragHandle($a("#arpMidi"), () => { const b = midiBytes(); if (!b) { toast("no notes to export yet"); return null; } return { name: fileName(), bytes: b }; });
    LUMI_EXPORT.dragHandle($a("#arpWav"), () => { const sc = score(); if (!sc.ev.length) { toast("no notes to export yet"); return null; } return { audio: true, name: fileName(), score: sc, bpm: S.bpm }; });
    $a("#arpToBlocks").onclick = () => { const sc = score(); if (!sc.ev.length) { toast("no notes yet"); return; } PADS.loadNotes ? PADS.loadNotes(sc.ev, sc.beats, S.bpm) : toast("open neon pads to edit"); };
    LUMI_PLAY.onChange(() => { sync(); });
    setInterval(() => { if (LUMI_PLAY.playing() === "arp" && !$("#arpPane").hidden) drawRoll(); }, 120);
    window.addEventListener("resize", drawRoll);
  }
  return { init, render, show: on => { if (on) render(); }, capture, score, eventsIn, source, state: S, play, surprise, regenerate, compare, genre, midiBytes, live, GENRES, PATTERNS, RATES };
})();

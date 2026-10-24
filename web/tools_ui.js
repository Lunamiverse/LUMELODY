// ================================================================ Chord Progression Finder (workspace beside the moon)
// Create: build progressions from chord blocks without playing piano. Library: the curated LUMI progressions.

// ---------------------------------------------------------------- library (curated progressions + producer notes)
const PROG = (() => {
  const GROUPS = [
    { id: "rnb", name: "R&B", bars: 1, bass: false, sounds: ["after-hours-ep", "vhs-keys", "underwater-keys", "velvet-stabs", "silk-glide", "purple-smoke"],
      vibe: { reverb: { type: 1, amount: .28 }, chorus: { intensity: .4 } },
      notes: [
        { h: "Tempo", li: ["Slow R&B: 65–85 BPM", "Modern R&B: 90–110 BPM"] },
        { h: "Instruments", li: ["Rhodes, electric piano, soft synths", "LUMELODY: After Hours EP, VHS Keys, Underwater Keys, Velvet Stabs, Silk Glide"] },
        { h: "Chord colours", li: ["maj7, m7, m9, add9, 13"] },
        { h: "Mix", li: ["Warm bass, subtle reverb, chorus and vocal harmonies", "fx: applies a subtle room reverb and chorus"] },
        { h: "Note", li: ["Suggested progressions inspired by common R&B sounds, not transcriptions of specific songs."] } ] },
    { id: "trap", name: "Psychedelic Trap", bars: 2, bass: false, sounds: ["astro-haze", "melt-bell", "reverse-dream", "vapor-choir", "rage-808", "mirage-lead"],
      vibe: { reverb: { type: 2, decay: 6.5, amount: .55 }, delay: { amount: .3, time: 2, feedback: .45, pingpong: 1 }, chorus: { intensity: .5 } },
      notes: [
        { h: "Tempo", li: ["120–150 BPM, or 60–75 BPM half-time"] },
        { h: "Instruments", li: ["Detuned bells: Melt Bell", "Dark ambient pads: Astro Haze, Purple Smoke", "Reversed piano: Reverse Dream", "Ethereal vocal chops: Vapor Choir", "Distorted 808s: Rage 808", "Wide analog synths: Mirage Lead, Hyper Glass"] },
        { h: "Effects", li: ["Long reverb", "Ping-pong delay", "Chorus and detune", "Pitch automation", "Reverse effects", "Filter automation", "fx: applies the long hall, ping-pong delay and chorus"] },
        { h: "Producer secret", li: ["Hold some chords for 2–4 bars and change a single note now and then (the move switch does this); leave space for the 808 and the melody to create the movement.", "The trippy feeling doesn't come from the chords alone: it comes from sound design, repetition, space and tension."] },
        { h: "Note", li: ["Original suggested progressions in a psychedelic-trap style, not transcriptions of specific songs."] } ] },
    { id: "disco", name: "Disco & Club", bars: 1, bass: true, sounds: ["velvet-stabs", "underwater-pad", "midnight-strings", "after-hours-ep", "velvet-bass", "squelch-bass"],
      vibe: { reverb: { type: 1, amount: .22 }, chorus: { intensity: .45 }, gate: { pattern: 0, depth: .45 } },
      notes: [
        { h: "Drums", li: ["Four-on-the-floor kick for house and disco house.", "Clap or snare on beats 2 and 4.", "Open hi-hat on offbeats.", "Shakers and tambourines for disco movement."] },
        { h: "Pads", li: ["Slow attack: 20–100 ms.", "Release: 300–1000 ms.", "Add chorus or subtle detune for width.", "Use sidechain to let the kick breathe (Gate / Pump in LUMELODY).", "Automate the filter for builds and transitions."] },
        { h: "Bass", li: ["Use a clean mono bass or warm analog synth (Velvet Bass, Squelch Bass, Pure Sub).", "Keep the sub frequencies centred.", "Sidechain to the kick when necessary.", "Use syncopation and octave jumps to make the groove move."] },
        { h: "Arrangement", li: ["Intro: drums + filtered pad.", "Groove: bass + chords.", "Build: open the filter and add percussion.", "Drop: full kick, bass, chords and lead.", "Break: remove the kick and leave atmospheric pads.", "Final drop: bring everything back with extra percussion."] },
        { h: "Note", li: ["Original suggested progressions and production recipes inspired by disco, house, dance-pop and club music. Bass notes are starting patterns; adjust their rhythm to fit your groove."] } ] },
  ];
  const P = (g, title, chords, bpm, more = {}) => Object.assign({ g, title, chords: chords.split(" – "), bpm }, more);
  const LIST = [
    P("rnb", "Dreamy", "Fmaj7 – Am7 – Dm9 – Bbmaj7", 98, { sub: "modern r&b", sound: "after-hours-ep" }),
    P("rnb", "Dark", "Dm9 – Bbmaj7 – Fmaj7 – Cadd9", 72, { sub: "melancholic r&b", sound: "underwater-keys" }),
    P("rnb", "Sexy", "Am9 – Dm9 – G13 – Cmaj7", 80, { sub: "toxic r&b", sound: "vhs-keys" }),
    P("rnb", "Nocturnal", "Fm9 – Dbmaj7 – Abmaj7 – Eb", 76, { sub: "mysterious r&b", sound: "purple-smoke" }),
    P("rnb", "Toxic Luxury", "Cm9 – Abmaj7 – Ebmaj7 – Bb", 84, { sub: "luxury r&b", sound: "velvet-stabs" }),
    P("rnb", "Sweet", "Cmaj9 – Am9 – Dm9 – G13", 70, { sub: "romantic r&b", sound: "after-hours-ep" }),

    P("trap", "Dark Psychedelic", "Fm9 – Dbmaj7 – Abmaj7 – Eb", 140, { sound: "astro-haze" }),
    P("trap", "Spacey", "Cm9 – Abmaj7 – Ebadd9 – Bb", 72, { sub: "floating, half-time", sound: "vapor-choir" }),
    P("trap", "Dark Cinematic", "Dm9 – Bbmaj7 – Gm9 – A7", 140, { sound: "purple-smoke" }),
    P("trap", "Trippy", "Em9 – Cmaj7 – Am7 – Bm7", 150, { sound: "lucid-arp" }),
    P("trap", "Evil", "F#m – Dmaj7 – C#sus4 – C#", 145, { sub: "distorted vibe", sound: "mirage-lead" }),
    P("trap", "Druggy", "Am9 – Fmaj7 – Cmaj7 – Gadd9", 70, { sub: "dreamlike, half-time", sound: "reverse-dream" }),
    P("trap", "Underwater", "Bbm9 – Gbmaj7 – Dbadd9 – Ab", 136, { sub: "drowned in reverb", sound: "underwater-pad" }),
    P("trap", "Outer Space", "Cm9 – Dbmaj7 – Abmaj7 – G7", 140, { sound: "astro-haze" }),
    P("trap", "Psychedelic Trap", "Gm9 – Ebmaj7 – Bbmaj7 – Fadd9", 150, { sound: "melt-bell" }),
    P("trap", "Haunted", "Dm – Bbmaj7 – C – Am7", 142, { sub: "haunted melody", sound: "grave-bell" }),
    P("trap", "Heavenly", "C#m9 – Amaj7 – Emaj7 – Badd9", 66, { sub: "floating, half-time", sound: "vapor-choir" }),
    P("trap", "Dark Space", "Fm – Eb – Dbmaj7 – C", 140, { sub: "dark space trap", sound: "night-siren" }),
    P("trap", "Distorted Synth", "Bm – Gmaj7 – Em9 – F#7", 150, { sound: "hyper-glass" }),
    P("trap", "Alien", "Cm – Dbmaj7 – Cm – B", 138, { sub: "unsettling", sound: "desert-fever" }),
    P("trap", "Night Drive", "Am9 – Gmaj7 – Fmaj7 – E7", 128, { sound: "mirage-lead" }),

    P("disco", "Disco House Anthem", "Am7 – Dm7 – G7 – Cmaj7", 124, { key: "A minor", bassLine: "A – A – C – E | D – D – F – A", pad: "Warm analog pad, chorus, soft sidechain.", vibeText: "Funky groove, vocal chops, dancefloor energy.", sound: "underwater-pad" }),
    P("disco", "Fashion Week Club", "Fm9 – Dbmaj7 – Abmaj7 – Eb", 122, { key: "F minor", bassLine: "F – F – Ab – C | Db – Db – F – Ab", pad: "Wide lush pad, dark filter automation.", vibeText: "Luxury, sexy, fashion-show energy.", sound: "abyss-bloom" }),
    P("disco", "Funky Disco Banger", "Dm7 – G7 – Cmaj7 – Am7", 120, { key: "D minor", bassLine: "D – D – F – A | G – G – B – D", pad: "Vintage electric piano + string pad.", vibeText: "Retro disco with modern production.", sound: "after-hours-ep" }),
    P("disco", "Underground Club Weapon", "Gm – Eb – Bb – F", 128, { key: "G minor", bassLine: "G – G – G – D | Eb – Eb – Bb – D", pad: "Dark saw pad, low-pass filter, strong sidechain.", vibeText: "Dark club, heavy drops, powerful low end.", sound: "abyss-bloom" }),
    P("disco", "Pop Girl Anthem", "Cm7 – Abmaj7 – Eb – Bb", 126, { key: "C minor", bassLine: "C – C – G – Bb | Ab – Ab – Eb – G", pad: "Glossy synth, soft supersaw, vocal layers.", vibeText: "Confident female pop, big chorus, club anthem.", sound: "velvet-stabs" }),
    P("disco", "French Touch Disco", "Fm7 – Bbm7 – Dbmaj7 – C7", 120, { key: "F minor", bassLine: "F – F – Ab – C | Bb – Bb – Db – F", pad: "Filtered disco strings, vintage synth, subtle phaser.", vibeText: "French house, retro filters, hypnotic groove.", sound: "midnight-strings" }),
    P("disco", "Glam Disco", "Em7 – A7 – Dmaj7 – Bm7", 124, { key: "E minor", bassLine: "E – E – G – B | A – A – C# – E", pad: "Bright analog pad, soft chorus, rhythmic stabs.", vibeText: "Chic, glittery, sophisticated dancefloor.", sound: "velvet-stabs" }),
    P("disco", "Sexy Late-Night House", "Am9 – Fmaj7 – Cmaj7 – Gadd9", 122, { key: "A minor", bassLine: "A – A – E – G | F – F – C – E", pad: "Airy atmospheric pad, slow attack, long reverb.", vibeText: "Sensual, smooth, late-night club.", sound: "nebula-drift" }),
    P("disco", "Big Room Club Banger", "Fm – Db – Ab – Eb", 128, { key: "F minor", bassLine: "F – F – F – C | Db – Db – Ab – C", pad: "Wide supersaw layers, filtered during the build.", vibeText: "Huge drops, energetic drums, festival club.", sound: "rage-lead" }),
    P("disco", "Italo Disco Future", "Cm – Fm – Ab – G", 118, { key: "C minor", bassLine: "C – C – Eb – G | F – F – Ab – C", pad: "Retro polysynth, chorus, bright arpeggiator.", vibeText: "Neon lights, retro-futuristic dance music.", sound: "lucid-arp" }),
    P("disco", "Dark Luxury House", "C#m7 – Amaj7 – E – B", 124, { key: "C# minor", bassLine: "C# – C# – G# – B | A – A – E – G#", pad: "Dark analog pad, slow filter movement, subtle detune.", vibeText: "Expensive, mysterious, seductive.", sound: "underwater-pad" }),
    P("disco", "Vocal House Anthem", "Gmaj7 – D/F# – Em7 – Cmaj7", 124, { key: "G major", bassLine: "G – G – D – F# | E – E – B – D", pad: "Warm piano pad, airy vocal textures, soft sidechain.", vibeText: "Emotional vocals, uplifting chorus, hands-in-the-air moment.", sound: "after-hours-ep" }),
    P("disco", "Disco Funk Groove", "Em7 – A9 – Dmaj7 – B7", 116, { key: "E minor", bassLine: "E – G – B – D | A – C# – E – G", pad: "Clavinet, electric piano, muted disco strings.", vibeText: "Funky basslines, syncopation, dancefloor bounce.", sound: "velvet-stabs" }),
    P("disco", "Hyperpop Club", "F#m – Dmaj7 – A – E", 130, { key: "F# minor", bassLine: "F# – F# – C# – E | D – D – A – C#", pad: "Bright detuned saws, digital textures, stereo movement.", vibeText: "Futuristic, glossy, explosive.", sound: "hyper-glass" }),
    P("disco", "Afterparty Disco", "Bm7 – Gmaj7 – Dmaj7 – A", 120, { key: "B minor", bassLine: "B – B – F# – A | G – G – D – F#", pad: "Soft vintage strings, warm keys, slow modulation.", vibeText: "Nostalgic, dreamy, after-hours disco.", sound: "midnight-strings" }),
  ];

  const H = HARMONY;
  // bass: the written pattern for its chord; the other chords get the same shape by chord degree; else 808 roots
  function bassBars(prog, chords) {
    if (!prog.bassLine) return chords.map(c => [{ beat: 0, len: 4, note: 28 + ((c.bass - 4 + 12) % 12) }]);
    const halves = prog.bassLine.split("|").map(h => h.split(/[–-]/).map(s => H.pcOf(s)));
    const used = new Set(), lit = new Map();
    chords.forEach((c, i) => { halves.forEach((h, hi) => { if (!used.has(hi) && !lit.has(i) && h[0] === c.bass) { used.add(hi); lit.set(i, h); } }); });
    return chords.map((c, i) => {
      const base = 33 + ((c.bass - 9 + 12) % 12);
      let pat;
      if (lit.has(i)) pat = lit.get(i).map(pc => (pc - c.bass + 12) % 12);
      else {
        const src = halves[i % halves.length], s0 = src[0];
        pat = src.map(pc => {
          const iv = (pc - s0 + 12) % 12;
          if (c.bass !== c.root) return iv === 0 ? 0 : iv === 7 ? (c.root + 7 - c.bass + 24) % 12 : (c.root - c.bass + 12) % 12;   // slash chord
          if (iv === 3 || iv === 4) return c.third;
          if (iv === 10 || iv === 11) return c.seventh != null ? c.seventh : 12;   // a triad gets the octave, never an out-of-key 7th
          return iv;
        });
      }
      return pat.map((iv, k) => ({ beat: k, len: .9, note: base + iv }));
    });
  }
  function score(prog, opt) {
    const blocks = prog.chords.map(sym => ({ sym, dur: opt.bars * 4 }));
    const sc = H.chordEvents(blocks, { move: opt.move });
    const bb = bassBars(prog, sc.chords);
    bb.forEach((bars, i) => { for (let b = 0; b < opt.bars; b++) for (const x of bars) sc.ev.push({ t: i * opt.bars * 4 + b * 4 + x.beat, d: Math.min(x.len, 4) - .02, n: x.note, vel: .78, track: 1 }); });
    return sc;
  }
  const midiOf = (prog, opt) => { const sc = score(prog, opt); return LUMI_EXPORT.midi([{ name: "LUMELODY " + prog.title + " chords", ev: sc.ev.filter(e => e.track === 0) }, { name: prog.bassLine ? "Bass" : "808", ev: sc.ev.filter(e => e.track === 1) }], opt.bpm); };
  const fileName = (prog, opt) => `LUMELODY - ${prog.title} - ${prog.chords.join(" ").replace(/\//g, "-")} - ${opt.bpm} bpm`;

  let group = "rnb", view = "list";
  const opts = { bars: null, move: true, bass: null };
  const gOf = id => GROUPS.find(x => x.id === id);
  const optFor = prog => { const g = gOf(prog.g); return { bars: opts.bars || g.bars, move: opts.move, bass: opts.bass == null ? g.bass : opts.bass, bpm: +$("#pgBpm").value || prog.bpm }; };
  const keyOf = i => "lib" + i;
  function play(i) { const p = LIST[i], o = optFor(p); LUMI_PLAY.start(keyOf(i), score(p, o), { bpm: o.bpm, tracks: o.bass ? null : new Set([0]) }); if (!PLUGIN) { $("#bpm").value = o.bpm; send({ type: "tempo", bpm: o.bpm }); } }
  function applyVibe(prog) {
    let v = gOf(prog.g).vibe;
    if (prog.pad) {   // disco: the effects its pad recipe names
      const t = prog.pad.toLowerCase(); v = { reverb: { type: 1, amount: .22 } };
      if (/chorus|detune|saw/.test(t)) v.chorus = { intensity: .45 };
      if (/sidechain/.test(t)) v.gate = { pattern: 0, depth: /strong/.test(t) ? .65 : .4 };
      if (/phaser/.test(t)) v.phaser = { intensity: .35 };
      if (/long reverb|atmospheric|airy/.test(t)) v.reverb = { type: 2, decay: 4.5, amount: .4 };
      if (/filter/.test(t)) v.filter = { type: 1, cutoff: 5200, resonance: .25, mod: .35, modrate: 0 };
    }
    WORKSPACE.applyFx(v);
  }
  function render() {
    for (const b of document.querySelectorAll("#libTabs .chip")) b.setAttribute("aria-pressed", b.dataset.g === (view === "notes" ? "notes" : group) ? "true" : "false");
    const g = gOf(group);
    $("#pgBars").querySelectorAll("button").forEach(b => b.setAttribute("aria-pressed", +b.dataset.b === (opts.bars || g.bars) ? "true" : "false"));
    $("#pgMove").setAttribute("aria-pressed", opts.move ? "true" : "false");
    $("#pgBass").setAttribute("aria-pressed", (opts.bass == null ? g.bass : opts.bass) ? "true" : "false");
    const list = $("#pgList");
    if (view === "notes") {
      list.className = "pnotes";
      list.replaceChildren(...g.notes.map(s => el("section", { className: "hl" }, [el("h3", { textContent: s.h }), el("ul", {}, s.li.map(t => el("li", { textContent: t })))])));
      return;
    }
    list.className = "pgrid";
    const playing = LUMI_PLAY.playing();
    list.replaceChildren(...LIST.map((p, i) => [p, i]).filter(([p]) => p.g === group).map(([p, i]) => {
      const t = el("b", { className: "pt" }); t.innerHTML = PIXEL_TYPE.svg(p.title); t.setAttribute("aria-label", p.title);
      const meta = [p.sub, p.key, p.bpm + " bpm"].filter(Boolean).join(" · ");
      const chips = el("div", { className: "chd" }, p.chords.map((c, k) => {
        const b = el("button", { type: "button", textContent: c, title: "Hear " + c });
        b.onclick = () => { const v = H.voice(p.chords.map(H.parse))[k]; LUMI_PLAY.once([...v.lhNotes, ...v.up]); };
        return b;
      }));
      const extra = p.pad ? [el("p", { className: "pd", textContent: "pad: " + p.pad }), el("p", { className: "pd cs", textContent: "bass: " + p.bassLine }), el("p", { className: "pd", textContent: "vibe: " + p.vibeText })] : [];
      const snd = PRESETS.find(x => x.id === p.sound);
      const on = playing === keyOf(i);
      const bPlay = el("button", { className: "btn pplay", type: "button", textContent: on ? "stop" : "play" }); bPlay.setAttribute("aria-pressed", on ? "true" : "false");
      bPlay.onclick = () => { if (LUMI_PLAY.playing() === keyOf(i)) LUMI_PLAY.stop(); else { $("#pgBpm").value = p.bpm; play(i); } };
      const bSnd = el("button", { className: "btn", type: "button", textContent: snd ? snd.name : "sound", title: "Load the suggested LUMELODY sound" }); bSnd.onclick = () => WORKSPACE.useSound(p.sound);
      const bFx = el("button", { className: "btn", type: "button", textContent: "fx", title: "Apply the effects for this vibe" }); bFx.onclick = () => applyVibe(p);
      const bEdit = el("button", { className: "btn", type: "button", textContent: "edit", title: "Open in Create to change the blocks" }); bEdit.onclick = () => FINDER.loadFrom(p, optFor(p));
      const bDrag = el("button", { className: "btn drag", type: "button", textContent: "drag to daw ↗", title: "Drag the MIDI (chords + bass) into FL Studio. Click to save the file." });
      LUMI_EXPORT.dragHandle(bDrag, () => ({ name: fileName(p, optFor(p)), bytes: midiOf(p, optFor(p)) }));
      const card = el("div", { className: "pcard" }, [t, el("small", { textContent: meta }), chips, ...extra, el("div", { className: "acts" }, [bPlay, bSnd, bFx, bEdit, bDrag])]);
      card.dataset.i = i; card.setAttribute("aria-current", on ? "true" : "false");
      return card;
    }));
  }
  function setGroup(g) {
    if (g === "notes") view = "notes"; else { if (g !== group) { LUMI_PLAY.stop(); opts.bars = null; opts.bass = null; } group = g; view = "list"; }
    render(); WORKSPACE.mood();
  }
  const restart = () => { const k = LUMI_PLAY.playing(); if (k && k.startsWith("lib")) play(+k.slice(3)); };
  function init() {
    $("#libTabs").replaceChildren(...GROUPS.map(g => g.id).concat(["notes"]).map(id => {
      const b = el("button", { className: "chip", type: "button", textContent: id === "notes" ? "producer notes" : gOf(id).name });
      b.dataset.g = id; b.onclick = () => setGroup(id); return b;
    }));
    $("#pgBars").querySelectorAll("button").forEach(b => b.onclick = () => { opts.bars = +b.dataset.b; render(); restart(); });
    $("#pgMove").onclick = () => { opts.move = !opts.move; render(); restart(); };
    $("#pgBass").onclick = () => { const g = gOf(group); opts.bass = !(opts.bass == null ? g.bass : opts.bass); render(); restart(); };
    $("#pgBpm").onchange = e => { const v = Math.max(40, Math.min(240, +e.target.value || 120)); e.target.value = v; restart(); };
    LUMI_PLAY.onChange(() => { if (!$("#libPane").hidden) for (const c of document.querySelectorAll("#pgList .pcard")) { const on = LUMI_PLAY.playing() === keyOf(+c.dataset.i); c.setAttribute("aria-current", on ? "true" : "false"); const b = c.querySelector(".pplay"); b.textContent = on ? "stop" : "play"; b.setAttribute("aria-pressed", on ? "true" : "false"); } });
  }
  return { init, render, LIST, GROUPS, score, midiOf, bassBars, isDisco: () => group === "disco" && view === "list" };
})();

// ---------------------------------------------------------------- Create: block-based progression builder
const FINDER = (() => {
  const H = HARMONY;
  const STYLES = [
    ["rnb", "R&B", 82, 1, "808"], ["trap", "Trap", 140, 2, "808"], ["hiphop", "Hip-hop", 90, 1, "808"], ["reggaeton", "Reggaeton", 94, 1, "dembow"],
    ["dembow", "Dembow", 118, 1, "dembow"], ["pop", "Pop", 110, 1, "pop"], ["disco", "Disco", 120, 1, "octave"], ["club", "Club", 124, 1, "octave"],
    ["cinematic", "Cinematic", 80, 2, "808"], ["experimental", "Experimental", 100, 2, "808"]].map(([id, name, bpm, bars, bass]) => ({ id, name, bpm, bars, bass }));
  const MOODS = [["dreamy", "major"], ["dark", "minor"], ["emotional", "minor"], ["sexy", "minor"], ["happy", "major"], ["sad", "minor"], ["nostalgic", "any"], ["energetic", "any"], ["mysterious", "minor"]];
  const LEVELS = [["simple", "Simple", "three-note chords"], ["intermediate", "Intermediate", "adds 7ths"], ["advanced", "Advanced", "adds 9ths and 13ths"]];
  const T = {   // chord templates by style, written from the key's major scale (b = flattened)
    rnb: { major: ["ii V I vi", "I vi ii V", "IV iii ii I", "I iii IV iv"], minor: ["i bVI bIII bVII", "i iv bVI V", "i bVII bVI bVII", "i iv bVII bIII"] },
    trap: { minor: ["i bVI bIII bVII", "i bVI iv V", "i bII i bVII", "i iv bVI V", "i bVI bVII i"], major: ["vi IV I V", "I V vi IV"] },
    hiphop: { minor: ["i iv i v", "i bVI iv V", "i bVII bVI V"], major: ["ii V I IV", "I IV vi V"] },
    reggaeton: { minor: ["i bVI bIII bVII", "i iv bVII bIII", "i bVII bVI bVII"], major: ["vi IV I V", "I V vi IV"] },
    dembow: { minor: ["i bVII bVI bVII", "i iv v i", "i bVI bVII i"], major: ["I V vi V", "vi IV I V"] },
    pop: { major: ["I V vi IV", "vi IV I V", "I vi IV V", "IV I V vi"], minor: ["i bVI bIII bVII", "i bVII bVI bVII"] },
    disco: { minor: ["i iv bVII bIII", "i bVI bIII bVII", "i iv v i"], major: ["ii V I vi", "vi ii V I", "I IV ii V"] },
    club: { minor: ["i bVI bIII bVII", "i bVII bVI bVII", "i bIII bVII iv"], major: ["vi IV I V", "IV V vi I"] },
    cinematic: { minor: ["i bVI bIII bVII", "i bVI iv V", "i bIII bVII iv", "i bVI bVII i"], major: ["I bVI bVII I", "I iii IV iv"] },
    experimental: { minor: ["i bII bVII bVI", "i bIII bII i", "i bVI bII V"], major: ["I bIII IV bVI", "I bVI bII V"] },
  };
  const PALETTE = { major: ["I", "ii", "iii", "IV", "V", "vi", "viio", "iv", "bVI", "bVII", "bIII"], minor: ["i", "iio", "bIII", "iv", "v", "bVI", "bVII", "V", "IV", "bII"] };
  const FEEL = {   // beginner labels: what a chord does, not its theory name
    I: ["home", "the chord the song rests on"], i: ["home", "the chord the song rests on"], ii: ["step", "a soft step away from home"], "iio": ["tension", "uneasy, wants to move"],
    iii: ["glow", "bright and gentle"], bIII: ["glow", "a warm, open lift"], IV: ["lift", "lifts the mood up"], iv: ["ache", "a bittersweet lift"],
    V: ["tension", "pulls strongly back home"], v: ["drift", "a softer pull home"], vi: ["soft", "tender, a little sad"], bVI: ["epic", "big and warm"],
    bVII: ["rise", "rises up, ready to loop"], "viio": ["tension", "uneasy, wants home"], bII: ["dark twist", "a dark, exotic shadow"] };
  const STEPS = { I: 0, II: 2, III: 4, IV: 5, V: 7, VI: 9, VII: 11 };

  const S = Object.assign({ style: "rnb", mood: "dreamy", level: "intermediate", key: "auto", mode: "auto", bpm: 82, bars: null, length: 4,
                            bass: true, move: false, spread: false, tonic: 9, kmode: "minor", blocks: [], sel: -1, compare: null }, store.get("lumi-finder-v1", {}));
  const P = Object.assign({ advanced: false, motion: true, preview: true, explain: true, large: false, guide: true, focus: false, milestones: true }, store.get("lumi-finder-prefs-v1", {}));
  let undo = [], redo = [];
  const styleOf = id => STYLES.find(s => s.id === id) || STYLES[0];
  const flats = () => H.flatsFor(S.tonic, S.kmode);
  const save = () => store.set("lumi-finder-v1", Object.assign({}, S, { compare: null }));
  const savePrefs = () => store.set("lumi-finder-prefs-v1", P);
  const snap = () => JSON.stringify({ blocks: S.blocks, tonic: S.tonic, kmode: S.kmode });
  function commit(fn) { undo.push(snap()); if (undo.length > 100) undo.shift(); redo = []; fn(); S.compare = null; save(); render(); }
  function restore(json) { const o = JSON.parse(json); S.blocks = o.blocks; S.tonic = o.tonic; S.kmode = o.kmode; S.sel = Math.min(S.sel, S.blocks.length - 1); S.compare = null; save(); render(); }
  const doUndo = () => { if (!undo.length) return; redo.push(snap()); restore(undo.pop()); };
  const doRedo = () => { if (!redo.length) return; undo.push(snap()); restore(redo.pop()); };

  // ---- harmony helpers
  function degree(tok) {
    const m = /^([b#]?)(VII|VI|IV|V|III|II|I|vii|vi|iv|v|iii|ii|i)(o?)$/.exec(tok);
    const up = m[2] === m[2].toUpperCase();
    return { root: (S.tonic + STEPS[m[2].toUpperCase()] + (m[1] === "b" ? -1 : m[1] === "#" ? 1 : 0) + 24) % 12, base: m[3] ? "dim" : up ? "M" : "m", tok };
  }
  function qualityFor(d, level) {
    const dom = d.tok === "V";
    if (level === "simple") return d.base === "dim" ? "dim" : d.base === "m" ? "m" : "";
    if (level === "intermediate") return d.base === "dim" ? "m7b5" : d.base === "m" ? "m7" : dom ? "7" : "maj7";
    return d.base === "dim" ? "m7b5" : d.base === "m" ? "m9" : dom ? (Math.random() < .5 ? "13" : "9") : (d.tok === "bVII" || d.tok === "bIII" ? "add9" : "maj9");
  }
  const symOf = (d, level) => H.name(d.root, flats() || d.tok.startsWith("b")) + qualityFor(d, level);   // borrowed chords are spelled with flats
  function feelOf(sym) {   // which scale degree a chord sits on -> beginner label
    try {
      const c = H.parse(sym), minor = c.iv.includes(3);
      for (const tok of PALETTE.major.concat(PALETTE.minor)) { const d = degree(tok); if (d.root === c.root && (d.base === "m") === (minor && !c.iv.includes(6)) && (d.base === "dim") === c.iv.includes(6)) return FEEL[tok] || ["colour", "an outside colour"]; }
      if (c.q === "7" || c.q === "9" || c.q === "13") return ["push", "a passing push into the next chord"];
    } catch (e) {}
    return ["colour", "an outside colour"];
  }
  const durBeats = () => (S.bars || styleOf(S.style).bars) * 4;
  function resolveKey() {
    const want = S.mode !== "auto" ? S.mode : (MOODS.find(m => m[0] === S.mood) || [0, "any"])[1];
    S.kmode = want === "any" ? (Math.random() < .5 ? "major" : "minor") : want;
    if (S.key !== "auto") S.tonic = +S.key;
    else { const common = S.kmode === "minor" ? [9, 4, 2, 7, 0, 5, 1, 6, 11, 10] : [0, 7, 2, 5, 9, 4, 10, 3]; S.tonic = common[Math.floor(Math.random() * common.length)]; }
  }
  function generate() {
    resolveKey();
    const st = T[S.style] || T.pop, pool = st[S.kmode] || st.minor || st.major;
    const pick = () => pool[Math.floor(Math.random() * pool.length)].split(" ");
    let toks = pick();
    if (S.length > 4) { const b = pick(); toks = toks.concat(b.slice(0, 2), toks.slice(2, Math.max(2, S.length - 6)), b.slice(2)).slice(0, S.length); }
    while (toks.length < S.length) toks = toks.concat(pick());
    toks = toks.slice(0, S.length);
    const dur = durBeats();
    return toks.map(t => ({ sym: symOf(degree(t), S.level), dur, tok: t }));
  }
  function paletteChords() { return (PALETTE[S.kmode] || PALETTE.minor).map(t => ({ tok: t, sym: symOf(degree(t), S.level), feel: FEEL[t] || ["colour", ""] })); }
  function regenOne(i) {
    const cur = S.blocks[i], nb = [S.blocks[i - 1], S.blocks[i + 1]].filter(Boolean).map(b => b.sym);
    const opts = paletteChords().filter(c => c.sym !== cur.sym && !nb.includes(c.sym));
    const diatonic = opts.slice(0, 7);
    const pool = Math.random() < .8 && diatonic.length ? diatonic : opts;
    const c = pool[Math.floor(Math.random() * pool.length)];
    return { sym: c.sym, dur: cur.dur, tok: c.tok };
  }
  // Make It Sound Expensive: richer chords, a passing dominant into a couple of chords, spread voicing
  function expensive(blocks) {
    const up = { "": "maj9", m: "m9", m7: "m9", maj7: "maj9", "7": "13", "9": "13", dim: "m7b5", add9: "maj9", sus4: "sus4", sus2: "add9" };
    let out = blocks.map(b => {
      const c = H.parse(b.sym); const isV = c.root === (S.tonic + 7) % 12 && (c.q === "" || c.q === "7" || c.q === "9");
      const q = isV ? "13" : (up[c.q] != null ? up[c.q] : c.q);
      return { sym: b.sym.replace(/^([A-G][#b]?)[^/]*/, "$1" + q), dur: b.dur, tok: b.tok };
    });
    const res = []; let added = 0;
    out.forEach((b, i) => {
      const next = out[(i + 1) % out.length], nc = H.parse(next.sym), bc = H.parse(b.sym);
      const domRoot = (nc.root + 7) % 12;
      const ok = added < 2 && b.dur >= 4 && i % 2 === 0 && nc.root !== S.tonic && bc.root !== domRoot && !(nc.q === "7" || nc.q === "13");
      if (ok) { res.push({ sym: b.sym, dur: b.dur / 2, tok: b.tok }, { sym: H.name(domRoot, flats()) + "7", dur: b.dur / 2, tok: "pass", passing: true }); added++; }
      else res.push(b);
    });
    return res;
  }
  // ---- score: chords + style bass
  function bassEvents(blocks, chords) {
    const kind = styleOf(S.style).bass, ev = []; let t = 0;
    chords.forEach((c, i) => {
      const dur = blocks[i].dur, r808 = 28 + ((c.bass - 4 + 12) % 12), r = 33 + ((c.bass - 9 + 12) % 12);
      if (kind === "808") ev.push({ t, d: dur - .05, n: r808, vel: .8, track: 1 });
      else for (let b = 0; b < dur; b += 4) {
        const L = Math.min(4, dur - b);
        if (kind === "octave") for (let k = 0; k < L * 2; k++) ev.push({ t: t + b + k * .5, d: .42, n: k % 2 ? r + 12 : r, vel: k % 2 ? .7 : .8, track: 1 });
        else if (kind === "dembow") for (const [o, d] of [[0, 1.3], [1.5, .4], [2, 1.3], [3.5, .4]]) { if (o < L) ev.push({ t: t + b + o, d, n: r, vel: .8, track: 1 }); }
        else for (let k = 0; k < L; k++) ev.push({ t: t + b + k, d: .9, n: r, vel: .78, track: 1 });
      }
      t += dur;
    });
    return ev;
  }
  function score(blocks, opt = {}) {
    const sc = H.chordEvents(blocks, { move: S.move, spread: opt.spread != null ? opt.spread : S.spread });
    sc.ev.push(...bassEvents(blocks, sc.chords));
    return sc;
  }
  const shown = () => (S.compare && S.compare.view === "exp" ? S.compare.exp : S.blocks);
  const tracks = () => (S.bass ? null : new Set([0]));
  const title = () => `LUMELODY - ${styleOf(S.style).name} ${S.mood} - ${H.name(S.tonic, flats())} ${S.kmode} - ${S.bpm} bpm`;
  const midiOf = (blocks, spread) => { const sc = score(blocks, { spread }); return LUMI_EXPORT.midi([{ name: "LUMELODY chords", ev: sc.ev.filter(e => e.track === 0) }, { name: styleOf(S.style).bass === "808" ? "808" : "Bass", ev: sc.ev.filter(e => e.track === 1) }], S.bpm); };
  function playAll() {
    const b = shown(); if (!b.length) return;
    if (LUMI_PLAY.playing() === "finder") { LUMI_PLAY.stop(); return; }
    LUMI_PLAY.start("finder", score(b, { spread: S.compare && S.compare.view === "exp" ? true : S.spread }), { bpm: S.bpm, tracks: tracks() });
    if (!PLUGIN) { $("#bpm").value = S.bpm; send({ type: "tempo", bpm: S.bpm }); }
  }
  function hear(i) {
    if (!P.preview) return;
    const b = shown(), v = H.voice(b.map(x => H.parse(x.sym)), { spread: S.spread || (S.compare && S.compare.view === "exp") })[i];
    LUMI_PLAY.once([...v.lhNotes, ...v.up]);
  }

  // ---- UI
  const $f = s => $("#finder " + s);
  function chipRow(node, items, cur, onPick, labelOf = x => x[1]) {
    node.replaceChildren(...items.map(it => { const b = el("button", { className: "chip", type: "button", textContent: labelOf(it) }); b.setAttribute("aria-pressed", it[0] === cur ? "true" : "false"); b.onclick = () => onPick(it[0]); return b; }));
  }
  function render() {
    const root = $("#finder"); if (!root) return;
    document.documentElement.classList.toggle("calm", !P.motion);
    root.classList.toggle("large", P.large); root.classList.toggle("adv", P.advanced);
    $f(".fguide").hidden = !P.guide;
    const folded = S.blocks.length > 0 && !S.stepsOpen;
    $f("#fSteps").hidden = folded; $f("#fSummary").hidden = !folded;
    $f("#fSumTxt").textContent = `${styleOf(S.style).name} · ${S.mood} · ${(LEVELS.find(l => l[0] === S.level) || [0, ""])[1]}`;
    chipRow($f("#fStyle"), STYLES.map(s => [s.id, s.name]), S.style, id => { S.style = id; S.bpm = styleOf(id).bpm; S.bars = null; save(); render(); });
    chipRow($f("#fMood"), MOODS.map(m => [m[0], m[0]]), S.mood, id => { S.mood = id; save(); render(); });
    chipRow($f("#fLevel"), LEVELS, S.level, id => { S.level = id; save(); render(); }, x => x[1]);
    $f("#fLevelHint").textContent = P.explain ? (LEVELS.find(l => l[0] === S.level) || [])[2] || "" : "";
    $f("#fKey").value = S.key; $f("#fMode").value = S.mode; $f("#fBpm").value = S.bpm; $f("#fLen").value = S.length;
    $f("#fBars").value = String(S.bars || styleOf(S.style).bars);
    $f("#fBass").setAttribute("aria-pressed", S.bass ? "true" : "false"); $f("#fMove").setAttribute("aria-pressed", S.move ? "true" : "false"); $f("#fSpread").setAttribute("aria-pressed", S.spread ? "true" : "false");
    $f("#fUndo").disabled = !undo.length; $f("#fRedo").disabled = !redo.length;
    const has = S.blocks.length > 0;
    $f("#fKeyLbl").textContent = has ? `key: ${H.name(S.tonic, flats())} ${S.kmode}` : "";
    for (const id of ["#fPlay", "#fExp", "#fDrag", "#fMidi", "#fWav", "#fDragWav"]) $f(id).disabled = !has;
    $f("#fPlay").textContent = LUMI_PLAY.playing() === "finder" ? "stop" : "play all";
    // compare bar
    const cmp = $f("#fCompare"); cmp.hidden = !S.compare;
    if (S.compare) { cmp.querySelectorAll("[data-v]").forEach(b => b.setAttribute("aria-pressed", b.dataset.v === S.compare.view ? "true" : "false")); }
    // timeline
    const tl = $f("#fTimeline"), blocks = shown(), total = blocks.reduce((a, b) => a + b.dur, 0) || 1;
    tl.replaceChildren(...(blocks.length ? blocks.map((b, i) => {
      const feel = feelOf(b.sym), bars = b.dur / 4;
      const blk = el("div", { className: "fblock" + (i === S.sel ? " sel" : "") + (b.passing ? " pass" : ""), tabIndex: 0, role: "button" });
      blk.style.flexGrow = b.dur; blk.style.flexBasis = (b.dur / total * 100) + "%"; blk.dataset.i = i;
      blk.setAttribute("aria-label", `${b.sym}, ${feel[0]}, ${bars} bar${bars === 1 ? "" : "s"}`);
      const pb = el("button", { className: "fbplay", type: "button", textContent: "▶", title: "Hear this chord" }); pb.onclick = e => { e.stopPropagation(); const was = P.preview; P.preview = true; hear(i); P.preview = was; };
      blk.append(el("b", { textContent: b.sym }), el("span", { className: "ff", textContent: feel[0] }), el("small", { textContent: (bars < 1 ? "½" : bars) + (bars === 1 ? " bar" : " bars") }), pb);
      if (P.explain && i === S.sel) blk.title = feel[1];
      return blk;
    }) : [el("p", { className: "fempty", textContent: "pick a style and a mood, then press generate. or drag chords here from the drawer below." })]));
    // selected block tools
    const tools = $f("#fTools"); tools.hidden = !(S.sel >= 0 && S.sel < S.blocks.length) || !!S.compare;
    if (!tools.hidden) { const b = S.blocks[S.sel]; $f("#fSelName").textContent = b.sym + (P.explain ? " · " + feelOf(b.sym)[1] : ""); $f("#fDur").value = String(b.dur); }
    // drawer
    const dr = $f("#fDrawer");
    dr.replaceChildren(...paletteChords().map(c => {
      const b = el("button", { className: "fchip", type: "button" }, [el("b", { textContent: c.sym }), el("span", { textContent: c.feel[0] })]);
      b.title = (P.explain ? c.feel[1] + ". " : "") + "Click to hear, drag into the timeline, or double-click to add at the end";
      b.dataset.sym = c.sym; b.dataset.tok = c.tok;
      b.onclick = () => { if (P.preview) { const v = H.voice([H.parse(c.sym)])[0]; LUMI_PLAY.once([...v.lhNotes, ...v.up]); } };
      b.ondblclick = () => commit(() => { S.blocks.push({ sym: c.sym, dur: durBeats(), tok: c.tok }); S.sel = S.blocks.length - 1; });
      return b;
    }));
    // prefs
    for (const [id, k] of [["#fpAdv", "advanced"], ["#fpMotion", "motion"], ["#fpPreview", "preview"], ["#fpExplain", "explain"], ["#fpLarge", "large"], ["#fpGuide", "guide"], ["#fpFocus", "focus"], ["#fpMile", "milestones"]]) $f(id).checked = !!P[k];
  }

  // pointer drag: reorder blocks, or drop drawer chords into the timeline
  function dragSetup() {
    const tl = $f("#fTimeline"), marker = el("i", { className: "fmark" });
    let d = null;
    const indexAt = x => { const bl = [...tl.querySelectorAll(".fblock")]; for (let i = 0; i < bl.length; i++) { const r = bl[i].getBoundingClientRect(); if (x < r.left + r.width / 2) return i; } return bl.length; };
    const showMark = idx => { const bl = [...tl.querySelectorAll(".fblock")], tr = tl.getBoundingClientRect(); let x = tr.left + 4; if (bl.length) x = idx < bl.length ? bl[idx].getBoundingClientRect().left - 3 : bl[bl.length - 1].getBoundingClientRect().right + 1; marker.style.left = (x - tr.left) + "px"; if (!marker.isConnected) tl.append(marker); };
    const begin = (e, kind, payload, node) => { d = { kind, payload, x0: e.clientX, y0: e.clientY, active: false, node, id: e.pointerId }; };
    document.addEventListener("pointermove", e => {
      if (!d || e.pointerId !== d.id) return;
      if (!d.active && Math.hypot(e.clientX - d.x0, e.clientY - d.y0) > 6) { d.active = true; d.ghost = el("div", { className: "fghost", textContent: d.kind === "block" ? S.blocks[d.payload].sym : d.payload.sym }); document.body.append(d.ghost); }
      if (!d.active) return;
      d.ghost.style.transform = `translate(${e.clientX + 8}px, ${e.clientY + 8}px)`;
      const r = tl.getBoundingClientRect(), over = e.clientX >= r.left - 20 && e.clientX <= r.right + 20 && e.clientY >= r.top - 30 && e.clientY <= r.bottom + 30;
      if (over) showMark(indexAt(e.clientX)); else marker.remove();
      d.over = over;
    });
    const end = e => {
      if (!d || e.pointerId !== d.id) return;
      const was = d; d = null; marker.remove(); if (was.ghost) was.ghost.remove();
      if (!was.active) { if (was.kind === "block") { S.sel = was.payload; render(); hear(was.payload); } return; }
      if (!was.over || S.compare) return;
      const idx = indexAt(e.clientX);
      if (was.kind === "block") commit(() => { const [b] = S.blocks.splice(was.payload, 1); const to = idx > was.payload ? idx - 1 : idx; S.blocks.splice(to, 0, b); S.sel = to; });
      else commit(() => { S.blocks.splice(idx, 0, { sym: was.payload.sym, dur: durBeats(), tok: was.payload.tok }); S.sel = idx; });
    };
    document.addEventListener("pointerup", end); document.addEventListener("pointercancel", end);
    tl.addEventListener("pointerdown", e => { const b = e.target.closest(".fblock"); if (!b || e.target.closest(".fbplay")) return; e.preventDefault(); begin(e, "block", +b.dataset.i, b); });
    $f("#fDrawer").addEventListener("pointerdown", e => { const c = e.target.closest(".fchip"); if (!c) return; begin(e, "chip", { sym: c.dataset.sym, tok: c.dataset.tok }, c); });
  }

  function init() {
    // selects
    $f("#fKey").replaceChildren(el("option", { value: "auto", textContent: "auto" }), ...[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].map(p => el("option", { value: String(p), textContent: H.name(p, [1, 3, 5, 8, 10].includes(p)) })));
    $f("#fKey").onchange = e => { S.key = e.target.value; if (S.key !== "auto" && S.blocks.length) transposeTo(+S.key); save(); render(); };
    $f("#fMode").onchange = e => { S.mode = e.target.value; save(); };
    $f("#fBpm").onchange = e => { S.bpm = Math.max(40, Math.min(240, +e.target.value || 100)); e.target.value = S.bpm; save(); if (LUMI_PLAY.playing() === "finder") { LUMI_PLAY.stop(); playAll(); } };
    $f("#fLen").onchange = e => { S.length = +e.target.value; save(); };
    $f("#fBars").onchange = e => { S.bars = +e.target.value; const d = S.bars * 4; commit(() => S.blocks.forEach(b => { if (!b.passing) b.dur = d; })); };
    $f("#fGen").onclick = () => { S.stepsOpen = false; commit(() => { S.blocks = generate(); S.sel = -1; }); if (typeof LUMI_FEEL !== "undefined") LUMI_FEEL.afterChords(S.style); };
    $f("#fSumEdit").onclick = () => { S.stepsOpen = true; render(); };
    $f("#fSumGen").onclick = () => $f("#fGen").click();
    $f("#fSurprise").onclick = () => {
      const r = a => a[Math.floor(Math.random() * a.length)];
      S.style = r(STYLES).id; S.mood = r(MOODS)[0]; S.level = r(LEVELS)[0]; S.bpm = styleOf(S.style).bpm; S.bars = null;
      commit(() => { S.blocks = generate(); S.sel = -1; });
      if (P.preview) playAll();
    };
    $f("#fPlay").onclick = playAll;
    $f("#fUndo").onclick = doUndo; $f("#fRedo").onclick = doRedo;
    $f("#fClear").onclick = () => { if (S.blocks.length) commit(() => { S.blocks = []; S.sel = -1; }); };
    $f("#fReverse").onclick = () => { if (S.blocks.length > 1) commit(() => { S.blocks = S.blocks.slice().reverse(); S.sel = S.sel >= 0 ? S.blocks.length - 1 - S.sel : -1; }); };
    $f("#fBass").onclick = () => { S.bass = !S.bass; save(); render(); };
    $f("#fMove").onclick = () => { S.move = !S.move; save(); render(); };
    $f("#fSpread").onclick = () => { S.spread = !S.spread; save(); render(); };
    // block tools
    const sel = () => S.blocks[S.sel];
    $f("#fLeft").onclick = () => { if (S.sel > 0) commit(() => { const [b] = S.blocks.splice(S.sel, 1); S.blocks.splice(--S.sel, 0, b); }); };
    $f("#fRight").onclick = () => { if (S.sel < S.blocks.length - 1) commit(() => { const [b] = S.blocks.splice(S.sel, 1); S.blocks.splice(++S.sel, 0, b); }); };
    $f("#fDup").onclick = () => { if (sel()) commit(() => { S.blocks.splice(S.sel + 1, 0, Object.assign({}, sel())); S.sel++; }); };
    $f("#fRegen").onclick = () => { if (sel()) { commit(() => { S.blocks[S.sel] = regenOne(S.sel); }); hear(S.sel); } };
    $f("#fDel").onclick = () => { if (sel()) commit(() => { S.blocks.splice(S.sel, 1); S.sel = Math.min(S.sel, S.blocks.length - 1); }); };
    $f("#fDur").onchange = e => { if (sel()) commit(() => { sel().dur = +e.target.value; }); };
    $f("#fReplace").onclick = () => { $f("#fDrawerWrap").classList.add("pick"); toast("Pick a chord in the drawer to replace " + sel().sym); };
    $f("#fDrawer").addEventListener("click", e => {
      const c = e.target.closest(".fchip"); if (!c || !$f("#fDrawerWrap").classList.contains("pick") || !sel()) return;
      $f("#fDrawerWrap").classList.remove("pick"); commit(() => { S.blocks[S.sel] = { sym: c.dataset.sym, dur: sel().dur, tok: c.dataset.tok }; });
    });
    const bDragOne = $f("#fDragOne");
    LUMI_EXPORT.dragHandle(bDragOne, () => sel() ? { name: `LUMELODY chord ${sel().sym}`, bytes: midiOf([sel()]) } : null);
    // expensive + compare
    $f("#fExp").onclick = () => { if (!S.blocks.length) return; S.compare = { exp: expensive(S.blocks), view: "exp" }; S.sel = -1; render(); if (P.preview) { LUMI_PLAY.stop(); playAll(); } };
    $f("#fCompare").querySelectorAll("[data-v]").forEach(b => b.onclick = () => { S.compare.view = b.dataset.v; const was = LUMI_PLAY.playing() === "finder"; render(); if (was) { LUMI_PLAY.stop(); playAll(); } });
    $f("#fKeepExp").onclick = () => { const exp = S.compare.exp; commit(() => { S.blocks = exp; S.spread = true; }); };
    $f("#fKeepOrig").onclick = () => { S.compare = null; render(); };
    // export
    const cur = () => ({ blocks: shown(), spread: S.compare && S.compare.view === "exp" ? true : S.spread });
    LUMI_EXPORT.dragHandle($f("#fDrag"), () => S.blocks.length ? { name: title(), bytes: midiOf(cur().blocks, cur().spread) } : null);
    $f("#fMidi").onclick = () => LUMI_EXPORT.saveMidi(title(), midiOf(cur().blocks, cur().spread));
    $f("#fWav").onclick = () => LUMI_EXPORT.saveAudio(title(), score(cur().blocks, { spread: cur().spread }), S.bpm, tracks());
    LUMI_EXPORT.dragHandle($f("#fDragWav"), () => S.blocks.length ? { audio: true, name: title(), score: score(cur().blocks, { spread: cur().spread }), bpm: S.bpm, tracks: tracks() } : null);
    // prefs
    for (const [id, k] of [["#fpAdv", "advanced"], ["#fpMotion", "motion"], ["#fpPreview", "preview"], ["#fpExplain", "explain"], ["#fpLarge", "large"], ["#fpGuide", "guide"], ["#fpFocus", "focus"], ["#fpMile", "milestones"]]) $f(id).onchange = e => { P[k] = e.target.checked; savePrefs(); render(); if (typeof LUMI_FEEL !== "undefined") LUMI_FEEL.applyFocus(); };
    $f("#fPrefsBtn").onclick = () => { const p = $f("#fPrefs"); p.hidden = !p.hidden; $f("#fPrefsBtn").setAttribute("aria-expanded", p.hidden ? "false" : "true"); };
    $f("#fGuideClose").onclick = () => { P.guide = false; savePrefs(); render(); };
    // keyboard: only while focus is inside the workspace
    $("#finder").addEventListener("keydown", e => {
      if (e.target.closest("input, select, textarea")) return;
      const mod = e.ctrlKey || e.metaKey;
      if (mod && e.key.toLowerCase() === "z") { e.preventDefault(); e.stopPropagation(); e.shiftKey ? doRedo() : doUndo(); return; }
      if (mod && e.key.toLowerCase() === "y") { e.preventDefault(); e.stopPropagation(); doRedo(); return; }
      const b = e.target.closest(".fblock"); if (!b) return;
      const i = +b.dataset.i; e.stopPropagation();
      if (e.key === "Enter" || e.key === " ") { e.preventDefault(); S.sel = i; render(); hear(i); }
      else if (e.key === "ArrowLeft" || e.key === "ArrowRight") {
        e.preventDefault(); const dir = e.key === "ArrowLeft" ? -1 : 1, j = i + dir;
        if (j < 0 || j >= S.blocks.length) return;
        if (e.shiftKey && !S.compare) commit(() => { const [x] = S.blocks.splice(i, 1); S.blocks.splice(j, 0, x); S.sel = j; }); else { S.sel = j; render(); }
        const n = $(`#fTimeline .fblock[data-i="${S.sel}"]`); if (n) n.focus();
      }
      else if ((e.key === "Delete" || e.key === "Backspace") && !S.compare) { e.preventDefault(); S.sel = i; $f("#fDel").click(); }
      else if (e.key.toLowerCase() === "d" && !S.compare) { S.sel = i; $f("#fDup").click(); }
      else if (e.key.toLowerCase() === "r" && !S.compare) { S.sel = i; $f("#fRegen").click(); }
    }, true);
    LUMI_PLAY.onChange((key, blk) => {
      if ($("#finder").hidden) return;
      $f("#fPlay").textContent = key === "finder" ? "stop" : "play all";
      document.querySelectorAll("#fTimeline .fblock").forEach(n => n.classList.toggle("now", key === "finder" && +n.dataset.i === blk));
    });
    dragSetup(); render();
  }
  function transposeTo(tonic) {
    const d = (tonic - S.tonic + 12) % 12; if (!d) return;
    commit(() => {
      S.tonic = tonic; const fl = flats();
      S.blocks = S.blocks.map(b => { const m = /^([A-G][#b]?)([^/]*)(?:\/([A-G][#b]?))?$/.exec(b.sym); const r = H.name(H.pcOf(m[1]) + d, fl) + m[2] + (m[3] ? "/" + H.name(H.pcOf(m[3]) + d, fl) : ""); return Object.assign({}, b, { sym: r }); });
    });
  }
  function loadFrom(prog, opt) {   // a library progression -> editable blocks
    const map = { rnb: "rnb", trap: "trap", disco: "disco" };
    const c0 = HARMONY.parse(prog.chords[0]);
    commit(() => {
      S.style = map[prog.g] || S.style; S.bpm = opt.bpm; S.bars = opt.bars; S.tonic = c0.root; S.kmode = c0.iv.includes(3) ? "minor" : "major";
      S.blocks = prog.chords.map(sym => ({ sym, dur: opt.bars * 4 })); S.sel = -1;
    });
    WORKSPACE.tab("create");
  }
  function loadBlocks(blocks, bpm) {   // from Neon Pads: what was played becomes editable blocks
    const c0 = HARMONY.parse(blocks[0].sym);
    commit(() => { S.bpm = bpm || S.bpm; S.tonic = c0.root; S.kmode = c0.iv.includes(3) ? "minor" : "major"; S.blocks = blocks; S.sel = -1; });
    WORKSPACE.tab("create");
  }
  return { init, render, loadFrom, loadBlocks, generate, expensive, score, midiOf, state: S, prefs: P, undo: doUndo, redo: doRedo, regenOne, feelOf, STYLES };
})();

// ---------------------------------------------------------------- workspace shell
const WORKSPACE = (() => {
  let tabNow = "create";
  const fit = () => { const det = $("#detail"); $("#prog").style.height = Math.max(420, $("#rack").offsetHeight + det.offsetHeight + 14) + "px"; };
  function open(on) {
    const ws = $("#prog");
    if (on && !$("#smp").hidden) SAMPLER.open(false);
    if (on && typeof LIBS !== "undefined" && !$("#libs").hidden) LIBS.open(false);
    if (on) fit();
    ws.hidden = !on; $("#rack").style.visibility = on ? "hidden" : ""; $("#detail").style.display = on ? "none" : "";
    $("#chordsBtn").setAttribute("aria-pressed", on ? "true" : "false");
    if (on) tab(tabNow); else { LUMI_PLAY.stop(); PADS.show(false); }
    mood();
  }
  function tab(t) {
    tabNow = t;
    $("#finder").hidden = t !== "create"; $("#libPane").hidden = t !== "library"; $("#npPane").hidden = t !== "pads"; $("#arpPane").hidden = t !== "arp";
    PADS.show(t === "pads" && !$("#prog").hidden);
    document.querySelectorAll("#wsTabs [data-t]").forEach(b => b.setAttribute("aria-pressed", b.dataset.t === t ? "true" : "false"));
    if (t === "library") PROG.render(); else if (t === "arp") ARP.render(); else FINDER.render();
    mood();
  }
  // the moon becomes a disco ball only in the Disco & Club section
  // disco-ball moon: the Sounds panel decides while it is open (its library); otherwise the Finder's disco & club library,
  // or a sound from the Disco + EDM + Club Bangers library being the one loaded on the keys
  function mood() {
    if (typeof ORB === "undefined" || !ORB.setDisco) return;
    const libsOpen = typeof LIBS !== "undefined" && !$("#libs").hidden;
    const on = libsOpen ? LIBS.discoActive() : (!$("#prog").hidden && tabNow === "library" && PROG.isDisco()) || (PRESETS[cur] && PRESETS[cur].cat === "Disco + EDM + Club");
    ORB.setDisco(on, !FINDER.prefs.motion || reduceMotion);
  }
  function applyFx(v) {
    for (const [fx, ps] of Object.entries(v)) {
      const f = fxIndex(fx); if (f < 0) continue;
      const keys = Object.keys(ps).sort((a, b) => (a === "type" ? -1 : b === "type" ? 1 : 0));   // a reverb type first: it loads its own space
      for (const k of keys) {
        const pi = SCHEMA[f].params.findIndex(p => p.id === k); if (pi < 0) continue;
        fxState[f].v[pi] = ps[k];
        send({ type: "param", e: f, i: pi, v: ps[k], echo: fx === "reverb" && k === "type" });
      }
      setOn(f, true);
    }
    toast("Effects: " + Object.keys(v).join(", "));
  }
  function useSound(id) { const i = PRESETS.findIndex(p => p.id === id); if (i >= 0) { loadPreset(i); toast("Sound: " + PRESETS[i].name); } }
  function init() {
    PROG.init(); FINDER.init(); PADS.init(); SAMPLER.init(); LIBS.init(); ARP.init(); LUMI_FEEL.init();
    document.querySelectorAll("#wsTabs [data-t]").forEach(b => b.onclick = () => tab(b.dataset.t));
    $("#wsClose").onclick = () => open(false);
    $("#chordsBtn").onclick = () => open($("#prog").hidden);
    document.addEventListener("keydown", e => { if (e.key === "Escape" && !$("#prog").hidden && !e.target.closest("input, select")) open(false); });
    window.addEventListener("resize", () => { if (!$("#prog").hidden) { $("#detail").style.display = ""; fit(); $("#detail").style.display = "none"; } });
  }
  return { init, open, tab, mood, applyFx, useSound };
})();

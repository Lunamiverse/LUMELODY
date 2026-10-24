// ================================================================ NEON PADS (second creation mode of the Chord Progression Finder)
// A playable pad instrument: in-key scale pads, one-tap chord pads or chromatic pads, with velocity, sustain, polyphony,
// recording (count-in, loop, overdub, quantize), a small pattern editor, undo/redo and the shared MIDI / audio export.
const PADS = (() => {
  const H = HARMONY;
  const COLS = 8, ROWS = 4, N = COLS * ROWS;
  const SCALES = { major: [0, 2, 4, 5, 7, 9, 11], minor: [0, 2, 3, 5, 7, 8, 10], dorian: [0, 2, 3, 5, 7, 9, 10], phrygian: [0, 1, 3, 5, 7, 8, 10],
                   "harmonic minor": [0, 2, 3, 5, 7, 8, 11], "pentatonic minor": [0, 3, 5, 7, 10], "pentatonic major": [0, 2, 4, 7, 9] };
  const ROMAN = ["I", "II", "III", "IV", "V", "VI", "VII"];
  const FEEL = ["home", "step", "glow", "lift", "tension", "soft", "rise"];
  const KEYS = ["z", "x", "c", "v", "b", "n", "m", ",", "a", "s", "d", "f", "g", "h", "j", "k", "q", "w", "e", "r", "t", "y", "u", "i", "1", "2", "3", "4", "5", "6", "7", "8"];
  const S = Object.assign({ layout: "scale", tonic: 9, scale: "minor", oct: 3, vel: "position", sustain: false, poly: 8, bpm: 100, bars: 2, quant: 16, metro: true, countIn: true,
                            pattern: [] }, store.get("lumi-pads-v1", {}));
  const save = () => store.set("lumi-pads-v1", S);
  let undo = [], redo = [], sel = -1;
  const P = () => FINDER.prefs;
  const commit = fn => { undo.push(JSON.stringify(S.pattern)); if (undo.length > 100) undo.shift(); redo = []; fn(); save(); renderRoll(); syncButtons(); };
  const doUndo = () => { if (!undo.length) return; redo.push(JSON.stringify(S.pattern)); S.pattern = JSON.parse(undo.pop()); sel = -1; save(); renderRoll(); syncButtons(); };
  const doRedo = () => { if (!redo.length) return; undo.push(JSON.stringify(S.pattern)); S.pattern = JSON.parse(redo.pop()); sel = -1; save(); renderRoll(); syncButtons(); };

  // ---------------------------------------------------------------- what each pad plays
  const flats = () => H.flatsFor(S.tonic, /minor|dorian|phrygian/.test(S.scale) ? "minor" : "major");
  const nn = n => H.name(n % 12, flats()) + (Math.floor(n / 12) - 1);
  function padMap() {
    const sc = SCALES[S.scale] || SCALES.minor, base = 12 * (S.oct + 1) + S.tonic, pads = [];
    for (let i = 0; i < N; i++) {
      const row = Math.floor(i / COLS), col = i % COLS;   // row 0 is the bottom row
      if (S.layout === "slices") {   // the sampler's slices: one pad per slice, played on the sampler's slice keys
        const k = i;
        pads.push(k < sliceCount ? { notes: [sliceBase + k], label: "slice " + (k + 1), sub: nn(sliceBase + k), role: k === 0 ? 4 : [0, 1, 2][k % 3] } : { notes: [], label: "", sub: "", role: 3, dim: true });
      } else if (S.layout === "chromatic") {
        const n = base - 12 + row * 5 + col;   // rows a fourth apart, like a guitar: shapes stay the same everywhere
        const deg = sc.indexOf(((n - S.tonic) % 12 + 12) % 12);
        pads.push({ notes: [n], label: nn(n), sub: deg >= 0 ? (deg === 0 ? "root" : "in key") : "", role: deg === 0 ? 4 : deg >= 0 ? 1 : 3, dim: deg < 0 });
      } else if (S.layout === "chords") {
        const sc7 = SCALES[S.scale].length === 7 ? SCALES[S.scale] : SCALES.minor;
        const d = col % 7, up = col === 7;
        const tone = k => { const j = d + k; return sc7[j % 7] + 12 * Math.floor(j / 7); };
        const root = base + tone(0) + (up ? 12 : 0) - (row === 0 ? 12 : 0);
        let iv = [0, tone(2) - tone(0), tone(4) - tone(0)];
        if (row === 2) iv = iv.concat([tone(6) - tone(0)]);
        if (row === 3) iv = [0, tone(2) - tone(0), tone(4) - tone(0), tone(6) - tone(0), tone(8) - tone(0)];
        const third = iv[1], fifth = iv[2], sev = iv[3];
        const q = row === 0 ? "" : third === 3 ? (fifth === 6 ? (sev != null ? "m7b5" : "dim") : row === 2 ? "m7" : row === 3 ? "m9" : "m") : (row === 2 ? (sev === 10 ? "7" : "maj7") : row === 3 ? (sev === 10 ? "9" : "maj9") : "");
        const name = H.name((root % 12), flats()) + (row === 0 ? "" : q);
        const role = d === 0 ? 4 : d === 3 || d === 1 ? 0 : d === 4 || d === 6 ? 2 : 1;
        const notes = row === 0 ? [root] : (() => { const v = H.voice([{ root: root % 12, iv, bass: root % 12, third, seventh: sev }])[0]; return [...v.lhNotes, ...v.up].map(n => n + 12 * (S.oct - 3)); })();
        pads.push({ notes, label: row === 0 ? nn(root) : name, sub: row === 0 ? "bass" : FEEL[d] + (up ? " ↑" : ""), role, chord: row > 0 });
      } else {
        const j = row * (sc.length) + col;   // each row continues the scale one octave up: every pad is in key
        const deg = j % sc.length, n = base - 12 + sc[deg] + 12 * Math.floor(j / sc.length);
        pads.push({ notes: [n], label: nn(n), sub: P().explain ? (deg === 0 ? "root" : "step " + (deg + 1)) : "", role: deg === 0 ? 4 : deg === 4 || deg === 2 ? 1 : deg % 2 ? 2 : 0 });
      }
    }
    return pads;
  }
  let sliceBase = 60, sliceCount = 0;
  let map = padMap();

  // ---------------------------------------------------------------- playing
  const active = new Map();   // pad index -> {ids, notes, t0, at}
  const held = new Set();     // sustained (released while sustain is on)
  let order = 0, ctr = 0;
  const light = new Float32Array(N), press = new Float32Array(N), target = new Float32Array(N);
  function velocityFrom(e, el2) {
    if (S.vel === "fixed" || !e) return .8;
    if (S.vel === "pressure" && e.pressure && e.pressure !== .5 && e.pressure !== 0) return Math.max(.15, Math.min(1, e.pressure));
    const r = el2.getBoundingClientRect(); return Math.max(.2, Math.min(1, .25 + .85 * (1 - (e.clientY - r.top) / r.height)));   // higher on the pad = louder
  }
  function padDown(i, vel) {
    if (active.has(i)) padUp(i, true);
    if (held.has(i)) { releaseVoice(i); held.delete(i); }
    // polyphony: the oldest pad gives way
    const voices = [...active.values()].reduce((a, v) => a + v.notes.length, 0) + [...held].length;
    if (S.poly > 0 && voices >= S.poly) { const oldest = [...active.entries()].sort((a, b) => a[1].order - b[1].order)[0]; if (oldest) { releaseVoice(oldest[0]); active.delete(oldest[0]); } }
    const pad = map[i]; if (!pad.notes.length) return;
    const ids = pad.notes.map(() => "np" + (++ctr));
    pad.notes.forEach((n, k) => setTimeout(() => noteOn(ids[k], n, vel), pad.chord ? k * 9 : 0));
    const ck = LUMI_PLAY.clock();
    active.set(i, { ids, notes: pad.notes, vel, order: ++order, rec: REC.on && ck && ck.key === "pads" && ck.beat >= 0 ? ck.beat : null });
    target[i] = .45 + .55 * vel; press[i] = 1;
    paint(i, true);
  }
  function releaseVoice(i) { const a = active.get(i) || heldVoices.get(i); if (!a) return; a.ids.forEach(id => noteOff(id)); heldVoices.delete(i); }
  const heldVoices = new Map();
  function padUp(i, force) {
    const a = active.get(i); if (!a) return;
    active.delete(i);
    if (a.rec != null) recordNotes(a);
    if (S.sustain && !force) { held.add(i); heldVoices.set(i, a); target[i] = .25; }
    else { a.ids.forEach(id => noteOff(id)); target[i] = 0; }
    press[i] = 0; paint(i, false);
  }
  function releaseHeld() { for (const i of held) { const a = heldVoices.get(i); if (a) a.ids.forEach(id => noteOff(id)); heldVoices.delete(i); target[i] = 0; } held.clear(); }
  function allOff() { for (const i of [...active.keys()]) padUp(i, true); releaseHeld(); }

  // ---------------------------------------------------------------- recording (loop + overdub)
  const REC = { on: false };
  const loopBeats = () => S.bars * 4;
  const grid = () => S.quant > 0 ? 4 / S.quant : 0;   // quant = notes per bar (4 = quarter, 8, 16), 0 = off
  function recordNotes(a) {
    const ck = LUMI_PLAY.clock(); if (!ck) return;
    const L = loopBeats(), g = grid();
    let t = ((a.rec % L) + L) % L, d = Math.max(.05, ck.beat - a.rec);
    if (g) { t = Math.round(t / g) * g % L; d = Math.max(g, Math.round(d / g) * g); }
    d = Math.min(d, L - t);
    commit(() => { for (const n of a.notes) S.pattern.push({ t: +t.toFixed(4), d: +d.toFixed(4), n, vel: +a.vel.toFixed(3) }); });
  }
  const score = () => ({ ev: S.pattern.map(x => ({ t: x.t, d: x.d, n: x.n, vel: x.vel, track: 0 })), beats: loopBeats() });
  function play(rec) {
    if (LUMI_PLAY.playing() === "pads") { LUMI_PLAY.stop(); REC.on = false; syncButtons(); return; }
    REC.on = !!rec;
    LUMI_PLAY.start("pads", score, { bpm: S.bpm, delay: rec && S.countIn ? 4 * 60 / S.bpm : 0 });
    if (!PLUGIN) { $("#bpm").value = S.bpm; send({ type: "tempo", bpm: S.bpm }); }
    syncButtons();
  }
  // metronome: beat dots always, a soft click in the browser (the plugin follows the host's own metronome)
  let lastBeat = null;
  function metroTick() {
    const ck = LUMI_PLAY.clock();
    const dots = $("#npBeats");
    if (!ck || ck.key !== "pads") { if (lastBeat !== null) { lastBeat = null; dots && dots.querySelectorAll("i").forEach(d => d.className = ""); } return; }
    const b = Math.floor(ck.beat);
    if (b === lastBeat) return;
    lastBeat = b;
    const inBar = ((b % 4) + 4) % 4;
    if (dots) dots.querySelectorAll("i").forEach((d, k) => d.className = k === inBar ? (b < 0 ? "ci" : "on") : "");
    if (S.metro && ctx && !ctx.plugin && ctx.createOscillator && (REC.on || b < 0)) {
      const o = ctx.createOscillator(), g = ctx.createGain(); o.frequency.value = inBar === 0 ? 1760 : 1320; g.gain.value = .0001;
      o.connect(g); g.connect(ctx.destination); const t = ctx.currentTime; g.gain.setValueAtTime(.12, t); g.gain.exponentialRampToValueAtTime(.0001, t + .05); o.start(t); o.stop(t + .06);
    }
  }

  // ---------------------------------------------------------------- the pads' look: WebGL transparent moulded polymer
  let gl = null, U = {}, cv = null, raf = 0, lastT = 0;
  // MIDI reverse (mirror in time inside [0, length)): note order flips, durations and velocities are kept
  function reverseNotes(notes, length) { return notes.map(x => Object.assign({}, x, { t: +Math.max(0, length - (x.t + x.d)).toFixed(6) })).sort((a, b) => a.t - b.t || a.n - b.n); }
  const PAL = [[1, .27, .66], [.95, .22, 1], [.56, .27, 1], [.4, .14, .74], [.42, .9, 1]];   // hot pink, magenta, ultraviolet, deep purple, icy cyan
  function initGL() {
    cv = $("#npCanvas");
    try { gl = cv.getContext("webgl", { antialias: false, premultipliedAlpha: false, alpha: false }); } catch (e) { gl = null; }
    if (!gl) { $("#npGrid").classList.add("nogl"); return; }
    const vs = `attribute vec2 p; void main(){ gl_Position = vec4(p, 0., 1.); }`;
    const fs = `precision highp float;
      uniform vec2 res; uniform float gap, time, dpr; uniform float lt[${N}], pr[${N}], hue[${N}], dimv[${N}];
      uniform vec3 pal[5];
      float h31(vec3 p) { return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453); }
      float n3(vec3 p) { vec3 i = floor(p), f = fract(p); f = f * f * (3. - 2. * f);
        return mix(mix(mix(h31(i), h31(i + vec3(1,0,0)), f.x), mix(h31(i + vec3(0,1,0)), h31(i + vec3(1,1,0)), f.x), f.y),
                   mix(mix(h31(i + vec3(0,0,1)), h31(i + vec3(1,0,1)), f.x), mix(h31(i + vec3(0,1,1)), h31(i + vec3(1,1,1)), f.x), f.y), f.z); }
      float sdb(vec2 p, vec2 b, float r) { vec2 q = abs(p) - b + r; return length(max(q, 0.)) + min(max(q.x, q.y), 0.) - r; }
      // a dark studio with a big soft key light top-left, a cool strip right and a warm magenta bounce from below
      vec3 env(vec3 d) {
        vec3 c = vec3(.006, .005, .01) + vec3(.03, .02, .05) * smoothstep(-.2, 1., d.y);
        c += vec3(2.4) * smoothstep(.34, .1, length((d.xy - vec2(-.45, .55)) * vec2(1., 1.3)));
        c += vec3(.75, .9, 1.) * 1.2 * smoothstep(.07, 0., abs(d.x - .8)) * smoothstep(.9, .1, abs(d.y));
        c += vec3(1., .3, .8) * .35 * smoothstep(.1, -.8, d.y);
        return c;
      }
      float get(float a[${N}], int i) { float r = 0.; for (int k = 0; k < ${N}; k++) if (k == i) r = a[k]; return r; }
      vec3 palc(float h) { vec3 c = pal[0]; for (int k = 0; k < 5; k++) if (abs(float(k) - h) < .5) c = pal[k]; return c; }
      void main() {
        vec2 fc = vec2(gl_FragCoord.x, res.y - gl_FragCoord.y);
        float cw = (res.x - gap * ${COLS - 1}.) / ${COLS}., ch = (res.y - gap * ${ROWS - 1}.) / ${ROWS}.;
        float ix = clamp(floor((fc.x + gap * .5) / (cw + gap)), 0., ${COLS - 1}.), iy = clamp(floor((fc.y + gap * .5) / (ch + gap)), 0., ${ROWS - 1}.);
        int idx = int((${ROWS - 1}. - iy) * ${COLS}. + ix);           // row 0 of the instrument is the bottom row
        vec2 p = fc - vec2(ix * (cw + gap) + cw * .5, iy * (ch + gap) + ch * .5);
        float L = get(lt, idx), Pr = get(pr, idx), Hh = get(hue, idx), Dm = get(dimv, idx);
        vec3 pc = palc(Hh); if (Dm > .5) pc = mix(pc, vec3(.3, .2, .45), .55);
        vec2 hb = vec2(cw, ch) * .5 - 2. * dpr; float rad = min(cw, ch) * .2;
        float rimW = clamp(min(cw, ch) * .17, 6. * dpr, 15. * dpr);
        float d = sdb(p, hb, rad);
        vec3 col;
        if (d > 0.) {   // the base plate: black, with a contact shadow and a little light spilling from lit pads
          col = vec3(.008, .006, .012) * (1. - .7 * exp(-d / (5. * dpr)));
          col += pc * L * .5 * exp(-d / (3.5 * dpr)) * smoothstep(gap * .5, gap * .1, d);   // light spilling onto the plate, kept inside the gap
        } else {
          vec2 e = vec2(1., 0.);
          vec2 g = normalize(vec2(sdb(p + e.xy, hb, rad) - sdb(p - e.xy, hb, rad), sdb(p + e.yx, hb, rad) - sdb(p - e.yx, hb, rad)) + 1e-5);
          vec2 q = p / hb;
          float inner = -d - rimW;
          if (inner < 0.) {
            // transparent moulded rim: a thick rounded polymer lip with refraction, internal light and crisp edges
            float t = clamp(-d / rimW, 0., 1.);                       // 0 outer edge .. 1 inner edge
            float sl = cos(t * 3.14159);
            vec3 n = normalize(vec3(g * sl * 1.1, 1.));
            vec3 R = reflect(vec3(0., 0., -1.), n);
            float F = .05 + .95 * pow(1. - n.z, 4.);
            vec3 refl = env(R);
            vec3 trans = mix(pc * (.12 + .9 * L), vec3(.02, .015, .03), .35) * (1. - F);   // the pad's glow seen through the plastic
            float ang = atan(p.y, p.x);
            float travel = pow(.5 + .5 * sin(ang - time * 3.2), 10.) * L + pow(.5 + .5 * sin(-ang * 2. - time * 2.1), 18.) * L * .5;   // light running round the rim
            float body = sin(t * 3.14159);
            vec3 lightIn = pc * (L * .9 + .08) * body * (.6 + 1.4 * travel);
            float lips = exp(-pow((t - .07) / .05, 2.)) * .9 + exp(-pow((t - .93) / .05, 2.)) * .55;
            float caust = pow(max(dot(R, normalize(vec3(-.45, .55, .7))), 0.), 70.) * 2.2;
            float topHi = pow(max(0., dot(n.xy, normalize(vec2(-.6, .8)))), 3.) * (1. - t) * .55;     // the light catching the moulded top edge
            col = refl * F * 1.5 + trans * .95 + lightIn + vec3(.92, .95, 1.) * lips * (.42 + .7 * L) + vec3(1.) * (caust + topHi) + vec3(.045, .05, .065) + pc * .06;
          } else {
            // the pad surface: pearlescent neon polymer with liquid light and a luminous core, pressed in when played
            float dep = Pr * .06;
            vec2 qq = q * (1. + dep);
            vec3 n = normalize(vec3(-qq * (.22 - .18 * Pr), 1.));
            vec3 R = reflect(vec3(0., 0., -1.), n);
            float flow = n3(vec3(qq * 1.7 + vec2(0., time * .06), time * .08 + float(idx) * 1.7));
            float flow2 = n3(vec3(qq * 3.4 - vec2(time * .04, 0.), time * .05 + float(idx)));
            vec3 base = mix(vec3(.06, .025, .1), pc * .36, .45 + .55 * flow) * (.7 + .3 * flow2);
            float film = dot(n, normalize(vec3(-.4, .5, .75))) * 2.3 + flow * .8 + qq.x * .4;
            vec3 irid = mix(mix(pal[0], pal[2], .5 + .5 * sin(film * 3.1)), pal[4], .5 + .5 * sin(film * 1.7 + 2.)) * .24;   // thin-film colour shift
            float r2 = dot(qq, qq);
            vec3 core = pc * exp(-r2 * 2.6) * (.3 + 1.35 * L) + vec3(1., .96, 1.) * exp(-r2 * 11.) * L * .85;
            vec3 echo = pc * exp(-dot(qq + vec2(.32, -.36), qq + vec2(.32, -.36)) * 5.) * (.05 + .3 * L);          // a reflection of the core inside the material
            float edge = smoothstep(0., 7. * dpr, inner);
            vec3 gloss = env(R) * (.035 + .05 * (1. - Pr)) + vec3(1.) * exp(-pow((qq.x + qq.y + 1.05) / .22, 2.)) * .09 * (1. - Pr);
            col = (base + irid * (.55 + .45 * L) + core + echo) * mix(.45, 1., edge) + gloss;
            col *= 1. - .18 * Pr * (1. - exp(-r2 * 3.));                  // pressed: the edges sink into shadow
          }
        }
        col = col / (1. + .28 * col);
        gl_FragColor = vec4(pow(col, vec3(.95)), 1.);
      }`;
    const sh = (t, s) => { const o = gl.createShader(t); gl.shaderSource(o, s); gl.compileShader(o); if (!gl.getShaderParameter(o, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(o)); return o; };
    try {
      const pr = gl.createProgram(); gl.attachShader(pr, sh(gl.VERTEX_SHADER, vs)); gl.attachShader(pr, sh(gl.FRAGMENT_SHADER, fs)); gl.linkProgram(pr); gl.useProgram(pr);
      const b = gl.createBuffer(); gl.bindBuffer(gl.ARRAY_BUFFER, b); gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
      const loc = gl.getAttribLocation(pr, "p"); gl.enableVertexAttribArray(loc); gl.vertexAttribPointer(loc, 2, gl.FLOAT, false, 0, 0);
      for (const u of ["res", "gap", "time", "dpr", "lt", "pr", "hue", "dimv", "pal"]) U[u] = gl.getUniformLocation(pr, u);
      gl.uniform3fv(U.pal, new Float32Array(PAL.flat()));
    } catch (e) { console.warn("neon pads shader:", e); gl = null; $("#npGrid").classList.add("nogl"); }
  }
  const hue = new Float32Array(N), dimv = new Float32Array(N);
  function draw(now) {
    raf = 0;
    if ($("#npPane").hidden || $("#prog").hidden) return;
    const dt = Math.min(.1, (now - (lastT || now)) / 1000); lastT = now;
    let busy = false;
    for (let i = 0; i < N; i++) {
      const tg = active.has(i) ? target[i] : held.has(i) ? .25 : 0;
      const k = tg > light[i] ? 1 - Math.exp(-dt / .03) : 1 - Math.exp(-dt / .38);
      light[i] += (tg - light[i]) * k; if (Math.abs(tg - light[i]) > .002) busy = true;
    }
    const pressV = new Float32Array(N); for (let i = 0; i < N; i++) pressV[i] = active.has(i) ? 1 : 0;
    if (gl) {
      const r = cv.getBoundingClientRect(), d = Math.min(2, window.devicePixelRatio || 1);
      const w = Math.max(2, Math.round(r.width * d)), h = Math.max(2, Math.round(r.height * d));
      if (cv.width !== w || cv.height !== h) { cv.width = w; cv.height = h; gl.viewport(0, 0, w, h); }
      const still = !P().motion || reduceMotion;
      gl.uniform2f(U.res, w, h); gl.uniform1f(U.gap, 8 * d); gl.uniform1f(U.dpr, d); gl.uniform1f(U.time, still ? 0 : now / 1000);
      gl.uniform1fv(U.lt, light); gl.uniform1fv(U.pr, pressV); gl.uniform1fv(U.hue, hue); gl.uniform1fv(U.dimv, dimv);
      gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
      if (!still) busy = true;   // the liquid light keeps moving gently
    }
    playhead();
    metroTick();
    if (busy || LUMI_PLAY.playing() === "pads") raf = requestAnimationFrame(draw); else idle = setTimeout(kick, 400);
  }
  let idle = 0;
  const kick = () => { clearTimeout(idle); if (!raf) raf = requestAnimationFrame(draw); };
  function paint(i, on) { const b = $(`#npGrid [data-i="${i}"]`); if (b) b.classList.toggle("on", on); kick(); }

  // ---------------------------------------------------------------- pattern editor (a small piano roll)
  function rollRange() {
    const ns = S.pattern.map(x => x.n).concat(map.flatMap(p => p.notes));
    let lo = Math.min(...ns), hi = Math.max(...ns);
    if (S.pattern.length) { lo = Math.min(...S.pattern.map(x => x.n)) - 2; hi = Math.max(...S.pattern.map(x => x.n)) + 2; }
    if (hi - lo < 12) { const m = (hi + lo) / 2; lo = Math.round(m - 6); hi = lo + 12; }
    return [lo, Math.min(hi, lo + 36)];
  }
  function renderRoll() {
    const roll = $("#npRoll"); if (!roll) return;
    const [lo, hi] = rollRange(), rows = hi - lo + 1, L = loopBeats();
    roll.style.setProperty("--beats", L); roll.style.setProperty("--grid", grid() ? L / grid() : L * 4);
    const notes = S.pattern.map((x, k) => {
      const nd = el("div", { className: "nnote" + (k === sel ? " sel" : "") });
      nd.style.left = (x.t / L * 100) + "%"; nd.style.width = Math.max(.6, x.d / L * 100) + "%";
      nd.style.top = ((hi - x.n) / rows * 100) + "%"; nd.style.height = (100 / rows) + "%";
      nd.style.opacity = .55 + .45 * x.vel; nd.dataset.k = k; nd.title = nn(x.n) + " · velocity " + Math.round(x.vel * 127);
      return nd;
    });
    roll.replaceChildren(...notes, el("i", { className: "nhead", id: "npHead" }),
      ...(S.pattern.length ? [] : [el("span", { className: "nempty", textContent: "press record, play the pads, and your notes appear here. click to add a note, drag to move, drag its end to resize." })]));
    roll.dataset.lo = lo; roll.dataset.hi = hi;
    $("#npSelInfo").textContent = sel >= 0 && S.pattern[sel] ? `${nn(S.pattern[sel].n)} · ${S.pattern[sel].d} beats` : `${S.pattern.length} notes · ${S.bars} bars`;
  }
  function playhead() {
    const h = $("#npHead"); if (!h) return;
    const ck = LUMI_PLAY.clock();
    if (!ck || ck.key !== "pads" || ck.beat < 0) { h.style.display = "none"; return; }
    h.style.display = ""; h.style.left = ((ck.beat % loopBeats()) / loopBeats() * 100) + "%";
  }
  function rollSetup() {
    const roll = $("#npRoll"); let dr = null;
    const toBeat = x => { const r = roll.getBoundingClientRect(); return (x - r.left) / r.width * loopBeats(); };
    const toNote = y => { const r = roll.getBoundingClientRect(), lo = +roll.dataset.lo, hi = +roll.dataset.hi; return Math.round(hi - (y - r.top) / r.height * (hi - lo + 1) + .5 - .5); };
    const snap = b => { const g = grid() || .25; return Math.round(b / g) * g; };
    roll.addEventListener("pointerdown", e => {
      const nd = e.target.closest(".nnote");
      e.preventDefault(); try { roll.setPointerCapture(e.pointerId); } catch (_) {}
      if (nd) {
        const k = +nd.dataset.k, r = nd.getBoundingClientRect();
        sel = k; dr = { k, mode: e.clientX > r.right - 6 ? "size" : "move", x0: e.clientX, y0: e.clientY, orig: Object.assign({}, S.pattern[k]), snap: JSON.stringify(S.pattern), moved: false };
        renderRoll();
        if (P().preview) LUMI_PLAY.once([S.pattern[k].n], S.pattern[k].vel, 400);
      } else {
        const t = Math.max(0, Math.min(loopBeats() - (grid() || .25), Math.floor(toBeat(e.clientX) / (grid() || .25)) * (grid() || .25)));
        const n = Math.max(0, Math.min(127, toNote(e.clientY)));
        commit(() => { S.pattern.push({ t, d: grid() || .25, n, vel: .8 }); sel = S.pattern.length - 1; });
        if (P().preview) LUMI_PLAY.once([n], .8, 300);
      }
    });
    roll.addEventListener("pointermove", e => {
      if (!dr) return;
      const x = S.pattern[dr.k]; if (!x) return;
      const db = toBeat(e.clientX) - toBeat(dr.x0), L = loopBeats();
      if (Math.abs(e.clientX - dr.x0) + Math.abs(e.clientY - dr.y0) > 3) dr.moved = true;
      if (dr.mode === "size") x.d = Math.max(grid() || .25, Math.min(L - x.t, snap(dr.orig.d + db)));
      else {
        x.t = Math.max(0, Math.min(L - x.d, snap(dr.orig.t + db)));
        const r = roll.getBoundingClientRect(), rows = +roll.dataset.hi - +roll.dataset.lo + 1;
        x.n = Math.max(0, Math.min(127, dr.orig.n - Math.round((e.clientY - dr.y0) / (r.height / rows))));
      }
      renderRoll();
    });
    const end = () => { if (!dr) return; if (dr.moved) { undo.push(dr.snap); redo = []; save(); syncButtons(); } dr = null; };
    roll.addEventListener("pointerup", end); roll.addEventListener("pointercancel", end);
  }

  // ---------------------------------------------------------------- UI
  function renderPads() {
    map = padMap();
    const g = $("#npGrid");
    for (const b of [...g.querySelectorAll(".npad")]) b.remove();
    map.forEach((p, i) => {
      hue[i] = p.role; dimv[i] = p.dim ? 1 : 0;
      const row = Math.floor(i / COLS), col = i % COLS;
      const b = el("button", { className: "npad" + (p.dim ? " dim" : ""), type: "button", tabIndex: -1 }, [el("b", { textContent: p.label }), el("small", { textContent: p.sub || "" })]);
      b.style.gridRow = String(ROWS - row); b.style.gridColumn = String(col + 1);
      b.dataset.i = i; b.setAttribute("aria-label", `${p.label}${p.sub ? ", " + p.sub : ""} (key ${KEYS[i].toUpperCase()})`);
      if (P().explain) b.title = (p.chord ? "chord: " : "note: ") + p.notes.map(nn).join(" ") + " · key " + KEYS[i].toUpperCase();
      g.append(b);
    });
    kick();
  }
  function syncButtons() {
    const playing = LUMI_PLAY.playing() === "pads";
    $("#npPlay").textContent = playing && !REC.on ? "stop" : "play";
    $("#npRec").textContent = REC.on ? "stop rec" : "record";
    $("#npRec").setAttribute("aria-pressed", REC.on ? "true" : "false");
    $("#npUndo").disabled = !undo.length; $("#npRedo").disabled = !redo.length;
    const has = S.pattern.length > 0;
    for (const id of ["#npDrag", "#npMidi", "#npWav", "#npDragWav", "#npQuant", "#npClear"]) $(id).disabled = !has;
    $("#npSus").setAttribute("aria-pressed", S.sustain ? "true" : "false"); $("#npMetro").setAttribute("aria-pressed", S.metro ? "true" : "false");
    $("#npCount").setAttribute("aria-pressed", S.countIn ? "true" : "false");
  }
  function renderControls() {
    document.querySelectorAll("#npLayout [data-l]").forEach(b => b.setAttribute("aria-pressed", b.dataset.l === S.layout ? "true" : "false"));
    $("#npKey").value = String(S.tonic); $("#npScale").value = S.scale; $("#npOct").textContent = "oct " + S.oct;
    $("#npVel").value = S.vel; $("#npPoly").value = String(S.poly); $("#npBpm").value = S.bpm; $("#npBars").value = String(S.bars); $("#npQ").value = String(S.quant);
    const snd = $("#npSound"); if (!snd.options.length) snd.replaceChildren(...PRESETS.map((p, i) => el("option", { value: String(i), textContent: p.name + " · " + p.cat.toLowerCase() })));
    snd.value = String(cur);
    $("#npPane").classList.toggle("adv", !!P().advanced); $("#npPane").classList.toggle("large", !!P().large);
    $("#npHint").textContent = P().explain ? { scale: "every pad is in key: nothing can sound wrong", chords: "one tap = one chord · bottom row = bass notes", chromatic: "all 12 notes · bright pads are in key" }[S.layout] : "";
  }
  function render() { renderControls(); renderPads(); renderRoll(); syncButtons(); }
  function title() { return `LUMELODY - neon pads - ${H.name(S.tonic, flats())} ${S.scale} - ${S.bpm} bpm`; }
  const midiOf = () => LUMI_EXPORT.midi([{ name: "LUMELODY neon pads", ev: S.pattern }], S.bpm);

  function init() {
    initGL();
    const g = $("#npGrid");
    g.addEventListener("pointerdown", e => { const b = e.target.closest(".npad"); if (!b) return; e.preventDefault(); try { b.releasePointerCapture(e.pointerId); } catch (_) {} if (!ctx) startFromGesture(); const i = +b.dataset.i; padDown(i, velocityFrom(e, b)); b._pid = e.pointerId; });
    const up = e => { for (const b of g.querySelectorAll(".npad.on")) if (b._pid === e.pointerId) { b._pid = null; padUp(+b.dataset.i); } };
    document.addEventListener("pointerup", up); document.addEventListener("pointercancel", up);
    g.addEventListener("pointerover", e => { if (!(e.buttons & 1)) return; const b = e.target.closest(".npad"); if (!b || b.classList.contains("on")) return; for (const o of g.querySelectorAll(".npad.on")) if (o._pid === e.pointerId) { o._pid = null; padUp(+o.dataset.i); } padDown(+b.dataset.i, velocityFrom(e, b)); b._pid = e.pointerId; });
    // computer keyboard: four rows of keys = four rows of pads (only while Neon Pads is open)
    const keyDown = new Set();
    document.addEventListener("keydown", e => {
      if (!window.LUMI_PADS_ON || e.metaKey || e.ctrlKey || e.altKey || (e.target.closest && e.target.closest("input, select, textarea"))) {
        if (window.LUMI_PADS_ON && (e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z" && !(e.target.closest && e.target.closest("input, select, textarea"))) { e.preventDefault(); e.shiftKey ? doRedo() : doUndo(); }
        return;
      }
      const k = e.key.toLowerCase(), i = KEYS.indexOf(k);
      if (k === " " && !e.repeat) { e.preventDefault(); S.sustain = !S.sustain; if (!S.sustain) releaseHeld(); save(); syncButtons(); return; }
      if (i < 0 || e.repeat || keyDown.has(k)) return;
      e.preventDefault(); keyDown.add(k); if (!ctx) startFromGesture(); padDown(i, .8);
    });
    document.addEventListener("keyup", e => { const k = e.key.toLowerCase(), i = KEYS.indexOf(k); if (i < 0 || !keyDown.has(k)) return; keyDown.delete(k); padUp(i); });
    window.addEventListener("blur", () => { keyDown.clear(); allOff(); });
    // controls
    document.querySelectorAll("#npLayout [data-l]").forEach(b => b.onclick = () => { allOff(); S.layout = b.dataset.l; save(); render(); });
    $("#npKey").replaceChildren(...[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11].map(p => el("option", { value: String(p), textContent: H.name(p, [1, 3, 5, 8, 10].includes(p)) })));
    $("#npScale").replaceChildren(...Object.keys(SCALES).map(k => el("option", { value: k, textContent: k })));
    $("#npKey").onchange = e => { allOff(); S.tonic = +e.target.value; save(); render(); };
    $("#npScale").onchange = e => { allOff(); S.scale = e.target.value; save(); render(); };
    $("#npOctDn").onclick = () => { allOff(); S.oct = Math.max(1, S.oct - 1); save(); render(); };
    $("#npOctUp").onclick = () => { allOff(); S.oct = Math.min(6, S.oct + 1); save(); render(); };
    $("#npSound").onchange = e => { loadPreset(+e.target.value); };
    $("#npVel").onchange = e => { S.vel = e.target.value; save(); };
    $("#npPoly").onchange = e => { S.poly = +e.target.value; save(); };
    $("#npSus").onclick = () => { S.sustain = !S.sustain; if (!S.sustain) releaseHeld(); save(); syncButtons(); };
    $("#npBpm").onchange = e => { S.bpm = Math.max(40, Math.min(240, +e.target.value || 100)); e.target.value = S.bpm; save(); if (LUMI_PLAY.playing() === "pads") { const r = REC.on; LUMI_PLAY.stop(); play(r); } };
    $("#npBars").onchange = e => { const nb = +e.target.value; commit(() => { S.bars = nb; S.pattern = S.pattern.filter(x => x.t < nb * 4).map(x => Object.assign(x, { d: Math.min(x.d, nb * 4 - x.t) })); }); if (LUMI_PLAY.playing() === "pads") { LUMI_PLAY.stop(); play(REC.on); } };
    $("#npQ").onchange = e => { S.quant = +e.target.value; save(); renderRoll(); };
    $("#npMetro").onclick = () => { S.metro = !S.metro; save(); syncButtons(); };
    $("#npCount").onclick = () => { S.countIn = !S.countIn; save(); syncButtons(); };
    $("#npPlay").onclick = () => { if (LUMI_PLAY.playing() === "pads" && REC.on) { REC.on = false; syncButtons(); return; } play(false); };
    $("#npRec").onclick = () => { if (REC.on) { REC.on = false; syncButtons(); return; } if (LUMI_PLAY.playing() === "pads") { REC.on = true; syncButtons(); } else play(true); };
    $("#npStop").onclick = () => { LUMI_PLAY.stop(); REC.on = false; allOff(); syncButtons(); };
    $("#npUndo").onclick = doUndo; $("#npRedo").onclick = doRedo;
    $("#npDel").onclick = () => { if (sel >= 0 && S.pattern[sel]) commit(() => { S.pattern.splice(sel, 1); sel = -1; }); };
    $("#npQuant").onclick = () => { const gq = grid() || .25; commit(() => S.pattern.forEach(x => { x.t = Math.min(loopBeats() - gq, Math.round(x.t / gq) * gq); x.d = Math.max(gq, Math.round(x.d / gq) * gq); })); };
    $("#npClear").onclick = () => { if (S.pattern.length) commit(() => { S.pattern = []; sel = -1; }); };
    // reverse the phrase in time (mirror): each note keeps its length and velocity and ends where the original started
    $("#npReverse").onclick = () => { if (S.pattern.length) commit(() => { S.pattern = reverseNotes(S.pattern, loopBeats()); sel = -1; }); };
    $("#npToBlocks").onclick = () => {   // what you played becomes chord blocks (one block per bar, by its lowest notes)
      if (!S.pattern.length) return;
      const blocks = [];
      for (let b = 0; b < S.bars; b++) {
        const inBar = S.pattern.filter(x => x.t >= b * 4 && x.t < b * 4 + 4); if (!inBar.length) continue;
        const pcs = [...new Set(inBar.map(x => x.n % 12))], root = Math.min(...inBar.map(x => x.n)) % 12;
        const has = iv => pcs.includes((root + iv) % 12);
        const q = has(3) ? (has(10) ? "m7" : "m") : has(4) ? (has(11) ? "maj7" : has(10) ? "7" : "") : has(5) ? "sus4" : "";
        blocks.push({ sym: H.name(root, flats()) + q, dur: 4 });
      }
      if (blocks.length) FINDER.loadBlocks(blocks, S.bpm);
    };
    LUMI_EXPORT.dragHandle($("#npDrag"), () => S.pattern.length ? { name: title(), bytes: midiOf() } : null);
    $("#npMidi").onclick = () => LUMI_EXPORT.saveMidi(title(), midiOf());
    $("#npWav").onclick = () => LUMI_EXPORT.saveAudio(title(), score(), S.bpm, null);
    LUMI_EXPORT.dragHandle($("#npDragWav"), () => S.pattern.length ? { audio: true, name: title(), score: score(), bpm: S.bpm, tracks: null } : null);
    document.addEventListener("keydown", e => { if (window.LUMI_PADS_ON && (e.key === "Delete" || e.key === "Backspace") && sel >= 0 && !(e.target.closest && e.target.closest("input, select, textarea"))) { e.preventDefault(); $("#npDel").click(); } });
    LUMI_PLAY.onChange(key => { if (key !== "pads") REC.on = false; if (!$("#npPane").hidden) { syncButtons(); kick(); } });
    rollSetup();
    window.addEventListener("resize", kick);
  }
  function useSlices(base, count) { sliceBase = base; sliceCount = Math.min(N, count); allOff(); S.layout = "slices"; save(); render(); }
  function show(on) { window.LUMI_PADS_ON = on; if (on) { render(); kick(); } else allOff(); }
  function loadNotes(ev, beats, bpm) {   // from the arpeggiator: its notes become an editable take
    const opts = [...document.querySelectorAll("#npBars option")].map(o => +o.value).sort((a, b) => a - b);
    const bars = opts.find(b => b * 4 >= beats) || opts[opts.length - 1] || 2;
    commit(() => { S.bars = bars; S.bpm = bpm || S.bpm; S.pattern = ev.filter(e => e.t < bars * 4).map(e => ({ t: +e.t.toFixed(4), d: +Math.min(e.d, bars * 4 - e.t).toFixed(4), n: e.n, vel: +e.vel.toFixed(3) })); });
    WORKSPACE.tab("pads");
  }
  return { init, show, render, padDown, loadNotes, padUp, useSlices, reverseNotes, state: S, map: () => map, score, midiOf, undo: doUndo, redo: doRedo, REC };
})();

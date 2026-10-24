// AudioWorklet host for the LUMI engine (WebAssembly build of engine/LumiEngine.h + engine/LumiFx.h).
// The main thread sends the wasm bytes and control messages; audio is rendered here in 128-frame blocks.
// (AudioWorkletGlobalScope has no TextEncoder/TextDecoder, so strings cross the boundary as bytes.)
class LumiProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.X = null;
    this.queue = [];
    this.port.onmessage = (e) => this.onMessage(e.data);
  }
  bytesAt(ptr, len) { return new Uint8Array(this.X.memory.buffer, ptr, len).slice(); }
  writeText(bytes) { new Uint8Array(this.X.memory.buffer, this.X.lumi_text(), bytes.length).set(bytes); }
  values() {
    const X = this.X;
    return Array.from(new Float32Array(X.memory.buffer, X.lumi_values(), 20 * 9));
  }
  onMessage(m) {
    if (m.type === "module") {
      const mod = new WebAssembly.Module(m.bytes);
      const stubs = {};
      for (const i of WebAssembly.Module.imports(mod)) (stubs[i.module] ??= {})[i.name] = () => 0;
      this.X = new WebAssembly.Instance(mod, stubs).exports;
      this.X._initialize();
      this.X.lumi_init(sampleRate);
      const p = this.X.lumi_schema();
      this.port.postMessage({ type: "ready", schema: this.bytesAt(p, this.X.lumi_schema_len()) });
      for (const q of this.queue) this.onMessage(q);
      this.queue = [];
      return;
    }
    if (!this.X) { this.queue.push(m); return; }
    const X = this.X;
    switch (m.type) {
      case "load":
        this.writeText(m.bytes);
        X.lumi_load(m.bpm);
        if (m.state) { this.writeText(m.state); X.lumi_set_state(); }   // the user's saved effects for this sound
        this.port.postMessage({ type: "loaded", id: m.id, values: this.values() });
        break;
      case "state": this.writeText(m.bytes); X.lumi_set_state(); this.port.postMessage({ type: "values", values: this.values() }); break;
      case "getState": { const p = X.lumi_state(); this.port.postMessage({ type: "stateOut", id: m.id, bytes: this.bytesAt(p, X.lumi_state_len()) }); break; }
      case "param": X.lumi_set_param(m.e, m.i, m.v); if (m.echo) this.port.postMessage({ type: "values", values: this.values() }); break;
      case "fxon": X.lumi_set_on(m.e, m.on ? 1 : 0); break;
      case "on": X.lumi_note_on(m.note, m.vel); break;
      case "off": X.lumi_note_off(m.note); break;
      case "alloff": X.lumi_all_off(); break;
      case "tempo": X.lumi_tempo(m.bpm); break;
      case "bend": X.lumi_bend(m.value); break;
      // sampler: the decoded file (copied into the engine), its settings, and the piano/sampler layer mix
      case "sample": {
        const n = m.L.length, pl = X.lumi_alloc(n), pr = X.lumi_alloc(n);
        new Float32Array(X.memory.buffer, pl, n).set(m.L); new Float32Array(X.memory.buffer, pr, n).set(m.R || m.L);
        X.lumi_sample_set(pl, pr, n, m.sr, m.channels || 2); X.lumi_free(pl); X.lumi_free(pr);
        if (m.settings) { this.writeText(m.settings); X.lumi_sampler_settings(1); }
        this.port.postMessage({ type: "sampleok", id: m.id, frames: n });
        break;
      }
      case "sampler": this.writeText(m.bytes); X.lumi_sampler_settings(m.rebuild ? 1 : 0); break;
      case "sampleClear": X.lumi_sample_clear(); break;
      case "layer": X.lumi_layer(m.mode, m.piano, m.sampler); break;
      // offline rendering (audio export): a note list with sample positions, played inside process()
      case "seq": this.seq = m.events; this.seqPos = 0; this.frame = 0; this.port.postMessage({ type: "seqok" }); break;
    }
  }
  process(inputs, outputs) {
    const out = outputs[0], X = this.X;
    if (!X) return true;
    const n = out[0].length;
    if (this.seq) {
      const end = this.frame + n;
      while (this.seqPos < this.seq.length && this.seq[this.seqPos].f < end) {
        const e = this.seq[this.seqPos++];
        if (e.on) X.lumi_note_on(e.note, e.vel); else X.lumi_note_off(e.note);
      }
      this.frame = end;
    }
    X.lumi_process(n);
    const mem = X.memory.buffer;
    out[0].set(new Float32Array(mem, X.lumi_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(mem, X.lumi_out_r(), n));
    return true;
  }
}
registerProcessor("lumi-engine", LumiProcessor);

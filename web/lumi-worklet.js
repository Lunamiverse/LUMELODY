// AudioWorklet host for the LUMI engine (WebAssembly build of engine/LumiEngine.h).
// The main thread sends the compiled module and control messages; audio is rendered here in 128-frame blocks.
class LumiProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.X = null;
    this.queue = [];
    this.port.onmessage = (e) => this.onMessage(e.data);
  }
  onMessage(m) {
    if (m.type === "module") {
      // bytes are compiled here (synchronous compile is allowed off the main thread, and avoids
      // relying on posting a compiled WebAssembly.Module, which not every browser supports)
      const mod = new WebAssembly.Module(m.bytes);
      const stubs = {};
      for (const i of WebAssembly.Module.imports(mod)) (stubs[i.module] ??= {})[i.name] = () => 0;
      const inst = new WebAssembly.Instance(mod, stubs);
      this.X = inst.exports;
      this.X._initialize();
      this.X.lumi_init(sampleRate);
      for (const q of this.queue) this.onMessage(q);
      this.queue = [];
      this.port.postMessage({ type: "ready" });
      return;
    }
    if (!this.X) { this.queue.push(m); return; }
    const X = this.X;
    switch (m.type) {
      case "load": {
        const bytes = m.bytes;   // NUL-terminated preset text, encoded on the main thread (no TextEncoder in worklets)
        new Uint8Array(X.memory.buffer, X.lumi_text(), bytes.length).set(bytes);
        X.lumi_load(m.bpm);
        X.lumi_preset_perf();
        const d = Array.from(new Float32Array(X.memory.buffer, X.lumi_out_l(), 9));
        this.port.postMessage({ type: "loaded", id: m.id, defaults: d });
        break;
      }
      case "on": X.lumi_note_on(m.note, m.vel); break;
      case "off": X.lumi_note_off(m.note); break;
      case "alloff": X.lumi_all_off(); break;
      case "tempo": X.lumi_tempo(m.bpm); break;
      case "bend": X.lumi_bend(m.value); break;
      case "perf": { const p = m.p; X.lumi_set_perf(p.reverb, p.reverbType, p.delay, p.delayDiv, p.delayFeedback, p.pitch, p.pitchRange, p.stutter, p.stutterDiv); break; }
    }
  }
  process(inputs, outputs) {
    const out = outputs[0];
    const X = this.X;
    if (!X) return true;
    const n = out[0].length;
    X.lumi_process(n);
    const mem = X.memory.buffer;
    out[0].set(new Float32Array(mem, X.lumi_out_l(), n));
    if (out[1]) out[1].set(new Float32Array(mem, X.lumi_out_r(), n));
    return true;
  }
}
registerProcessor("lumi-engine", LumiProcessor);

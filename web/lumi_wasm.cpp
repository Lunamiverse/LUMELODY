// WebAssembly bridge: exposes the LUMI engine (instrument + 20-effect rack) to the browser's AudioWorklet.
#include "../engine/LumiEngine.h"
#include "../engine/LumiAnalysis.h"
#include <cstdlib>
#include <cstring>

static lumi::Engine* g_eng = nullptr;
static float g_l[256], g_r[256];
static char g_text[16384];
static float g_vals[lumi::FX_COUNT * (lumi::kMaxParams + 1)];
static std::string g_schema, g_state;
// sampler: the original audio stays here untouched; edits rebuild the playback buffer from it
static lumi::AudioData g_orig;
static lumi::SamplerSettings g_sset;
static std::vector<float> g_out;   // results returned to JavaScript (peaks, analysis)

extern "C" {
__attribute__((export_name("lumi_text"))) char* lumi_text() { return g_text; }
__attribute__((export_name("lumi_out_l"))) float* lumi_out_l() { return g_l; }
__attribute__((export_name("lumi_out_r"))) float* lumi_out_r() { return g_r; }
__attribute__((export_name("lumi_init"))) void lumi_init(float sr) { if (!g_eng) g_eng = new lumi::Engine(); g_eng->prepare(sr); }

// g_text holds "key value\n" lines (a flattened preset)
__attribute__((export_name("lumi_load"))) int lumi_load(float bpm) {
    lumi::ParamMap p; int count = 0; char* s = g_text;
    while (*s) {
        while (*s == '\n' || *s == ' ') ++s;
        if (!*s) break;
        char* k = s; while (*s && *s != ' ') ++s;
        if (!*s) break;
        *s++ = 0;
        char* end; float v = std::strtof(s, &end); s = end;
        p.v[k] = v; ++count;
        while (*s && *s != '\n') ++s;
    }
    g_eng->loadPreset(p, bpm);
    return count;
}
// effect schema as JSON (names, ranges, defaults, choices) -> the UI builds every knob from it
__attribute__((export_name("lumi_schema"))) const char* lumi_schema() { g_schema = lumi::fxSchemaJson(); return g_schema.c_str(); }
__attribute__((export_name("lumi_schema_len"))) int lumi_schema_len() { return (int)g_schema.size(); }
__attribute__((export_name("lumi_set_param"))) void lumi_set_param(int e, int i, float v) { if (g_eng) g_eng->setParam(e, i, v); }
__attribute__((export_name("lumi_set_on"))) void lumi_set_on(int e, int on) { if (g_eng) g_eng->setOn(e, on != 0); }
// all effect values: for each effect [on, p0..p7]
__attribute__((export_name("lumi_values"))) float* lumi_values() {
    for (int e = 0; e < lumi::FX_COUNT; ++e) {
        g_vals[e * (lumi::kMaxParams + 1)] = g_eng->rack.on[e] ? 1.f : 0.f;
        for (int i = 0; i < lumi::kMaxParams; ++i) g_vals[e * (lumi::kMaxParams + 1) + 1 + i] = g_eng->rack.val[e][i];
    }
    return g_vals;
}
__attribute__((export_name("lumi_state"))) const char* lumi_state() { g_state = g_eng->rack.state(); return g_state.c_str(); }
__attribute__((export_name("lumi_state_len"))) int lumi_state_len() { return (int)g_state.size(); }
__attribute__((export_name("lumi_set_state"))) void lumi_set_state() { g_eng->rack.setState(g_text); }
__attribute__((export_name("lumi_tempo"))) void lumi_tempo(float bpm) { if (g_eng) g_eng->setTempo(bpm); }
__attribute__((export_name("lumi_note_on"))) void lumi_note_on(int n, float v) { if (g_eng) g_eng->noteOn(n, v); }
__attribute__((export_name("lumi_note_off"))) void lumi_note_off(int n) { if (g_eng) g_eng->noteOff(n); }
__attribute__((export_name("lumi_all_off"))) void lumi_all_off() { if (g_eng) g_eng->allNotesOff(); }
__attribute__((export_name("lumi_bend"))) void lumi_bend(float v) { if (g_eng) g_eng->setPitchBend(v); }
__attribute__((export_name("lumi_process"))) void lumi_process(int n) {
    if (!g_eng) { std::memset(g_l, 0, sizeof g_l); std::memset(g_r, 0, sizeof g_r); return; }
    g_eng->process(g_l, g_r, n > 256 ? 256 : n);
}

// ---------------------------------------------------------------- sampler + analysis
__attribute__((export_name("lumi_alloc"))) float* lumi_alloc(int floats) { return (float*) std::malloc(sizeof(float) * (size_t) (floats > 0 ? floats : 1)); }
__attribute__((export_name("lumi_free"))) void lumi_free(float* p) { std::free(p); }
// copy a decoded file in (R may equal L for mono); returns frames kept
__attribute__((export_name("lumi_sample_set"))) int lumi_sample_set(const float* L, const float* R, int n, float sr, int channels) {
    g_orig.L.assign(L, L + n); g_orig.R.assign(R, R + n); g_orig.sr = sr; g_orig.channels = channels;
    return n;
}
__attribute__((export_name("lumi_sample_clear"))) void lumi_sample_clear() { g_orig = lumi::AudioData(); if (g_eng) g_eng->sampler.setPlayback(nullptr, g_sset); }
// settings text in g_text ("key=value;..."); rebuild = 1 when stretch / reverse changed (the audio itself changes)
__attribute__((export_name("lumi_sampler_settings"))) void lumi_sampler_settings(int rebuild) {
    g_sset = lumi::SamplerSettings::fromString(g_text);
    if (!g_eng) return;
    if (rebuild || !g_eng->sampler.hasSample()) g_eng->sampler.setPlayback(g_orig.frames() ? lumi::buildPlayback(g_orig, g_sset) : nullptr, g_sset);
    else g_eng->sampler.setSettings(g_sset);
}
__attribute__((export_name("lumi_layer"))) void lumi_layer(int mode, float piano, float samp) { if (g_eng) g_eng->setLayer(mode, piano, samp); }
// analysis / waveform helpers (used on the page's main thread by a second instance, never inside the audio thread)
__attribute__((export_name("lumi_out"))) float* lumi_out() { return g_out.data(); }
__attribute__((export_name("lumi_peaks"))) int lumi_peaks(int bins, int processed) {
    auto pb = processed ? lumi::buildPlayback(g_orig, lumi::SamplerSettings::fromString(g_text)) : nullptr;
    g_out = lumi::analysis::peaks(processed ? *pb : g_orig, bins);
    return (int) g_out.size();
}
__attribute__((export_name("lumi_analyse"))) int lumi_analyse() {
    auto a = lumi::analysis::analyse(g_orig);
    g_out = { (float) a.bpm, (float) a.bpmConf, (float) a.keyTonic, a.keyMinor ? 1.f : 0.f, (float) a.keyConf, (float) a.rootNote, (float) a.rootCents,
              (float) a.rootConf, (float) a.silenceStart, (float) a.silenceEnd, (float) a.seconds, (float) a.onsets.size() };
    for (size_t f : a.onsets) g_out.push_back((float) f);
    return (int) g_out.size();
}
// the processed buffer itself (for exporting the edited sample as audio): channel 0 = L, 1 = R
__attribute__((export_name("lumi_processed"))) int lumi_processed(int channel) {
    auto pb = lumi::buildPlayback(g_orig, lumi::SamplerSettings::fromString(g_text));
    g_out = channel ? pb->R : pb->L;
    return (int) g_out.size();
}
}

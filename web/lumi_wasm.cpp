// WebAssembly bridge: exposes the LUMI engine (instrument + 20-effect rack) to the browser's AudioWorklet.
#include "../engine/LumiEngine.h"
#include <cstdlib>
#include <cstring>

static lumi::Engine* g_eng = nullptr;
static float g_l[256], g_r[256];
static char g_text[16384];
static float g_vals[lumi::FX_COUNT * (lumi::kMaxParams + 1)];
static std::string g_schema, g_state;

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
}

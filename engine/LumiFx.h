// LUMELODY effects rack: 20 performance effects, each with an on/off switch, a main knob and its own
// sub-parameters. Framework-free C++17; all buffers are allocated in prepare(), never in process().
//
// Signal flow (engine side):
//   voices (+ octave layer) -> preset tone stage -> INSERTS -> preset delay/reverb (driven by the DELAY and
//   REVERB effects) -> POST (shimmer, freeze, granular, width) -> preset pump/gain -> output safety.
// Pitch, Glide, Octave Layer and the Swell trigger are applied inside the engine (they need note events).
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace lumi {

// ------------------------------------------------------------------ schema (single source of truth for UI)
enum ParamKind { P_CONT = 0, P_ENUM = 1, P_TOGGLE = 2 };
struct ParamDef { const char* id; const char* name; float min, max, def; int kind; const char* unit; const char* choices; bool log; };
struct EffectDef { const char* id; const char* name; const char* category; const char* note; std::vector<ParamDef> p; };

enum FxId {
    FX_REVERB, FX_DELAY, FX_PITCH, FX_STUTTER, FX_CHORUS, FX_SATURATION, FX_FILTER, FX_PHASER, FX_FLANGER, FX_SHIMMER,
    FX_WIDTH, FX_GRANULAR, FX_GLIDE, FX_TAPE, FX_GATE, FX_OCTAVE, FX_RESONATOR, FX_REVERSE, FX_FREEZE, FX_GLUE, FX_COUNT
};
static constexpr int kMaxParams = 8;

inline const std::vector<EffectDef>& fxSchema() {
    static const std::vector<EffectDef> s = {
        {"reverb", "Reverb", "Space", "Wet amount of the instrument's reverb. Type sets the space; decay, pre-delay and tone shape the tail.",
         {{"amount", "Amount", 0, 1, .3f, P_CONT, "%", "", false}, {"type", "Type", 0, 4, 0, P_ENUM, "", "Preset|Room|Hall|Plate|Shimmer", false},
          {"decay", "Decay", .2f, 12, 2.5f, P_CONT, "s", "", true}, {"predelay", "Pre-delay", 0, 200, 15, P_CONT, "ms", "", false},
          {"tone", "Tone", 0, 1, .55f, P_CONT, "", "", false}}},
        {"delay", "Delay", "Space", "Tempo-synced echoes. Feedback is soft-limited so repeats always die away.",
         {{"amount", "Amount", 0, 1, .25f, P_CONT, "%", "", false}, {"time", "Time", 0, 6, 2, P_ENUM, "", "1/4|1/8|1/8 dotted|1/16|1/32|1/4 triplet|1/8 triplet", false},
          {"feedback", "Feedback", 0, .9f, .35f, P_CONT, "%", "", false}, {"tone", "Tone", -1, 1, -.2f, P_CONT, "", "", false},
          {"width", "Width", 0, 1, .8f, P_CONT, "%", "", false}, {"pingpong", "Ping-pong", 0, 1, 1, P_TOGGLE, "", "", false}}},
        {"pitch", "Pitch", "Pitch", "Transposes the instrument itself (re-synthesized, so no stretching artefacts). Keyboard octave is separate.",
         {{"semitones", "Pitch", -24, 24, 0, P_CONT, "st", "", false}, {"range", "Range", 0, 4, 3, P_ENUM, "", "±2|±5|±7|±12|±24", false},
          {"fine", "Fine", 0, 1, 0, P_TOGGLE, "", "", false}}},
        {"stutter", "Stutter", "Rhythm", "Captures a tempo-synced slice and repeats it. Seamless loop crossfades; gate shortens each repeat.",
         {{"intensity", "Intensity", 0, 1, .5f, P_CONT, "%", "", false}, {"rate", "Rate", 0, 3, 2, P_ENUM, "", "1/4|1/8|1/16|1/32", false},
          {"gate", "Gate", .1f, 1, 1, P_CONT, "%", "", false}, {"mix", "Mix", 0, 1, 1, P_CONT, "%", "", false}}},
        {"chorus", "Chorus", "Motion", "Three modulated voices per side. Intensity scales depth and blend together.",
         {{"intensity", "Intensity", 0, 1, .5f, P_CONT, "%", "", false}, {"rate", "Rate", .05f, 5, .6f, P_CONT, "Hz", "", true},
          {"depth", "Depth", 0, 1, .6f, P_CONT, "%", "", false}, {"mix", "Mix", 0, 1, .5f, P_CONT, "%", "", false},
          {"spread", "Spread", 0, 1, .8f, P_CONT, "%", "", false}}},
        {"saturation", "Saturation", "Tone", "Adds harmonics with 2x oversampling. Output is level-matched so you hear colour, not volume.",
         {{"drive", "Drive", 0, 1, .3f, P_CONT, "%", "", false}, {"mode", "Mode", 0, 2, 0, P_ENUM, "", "Warm|Tube|Crunch", false},
          {"tone", "Tone", -1, 1, 0, P_CONT, "", "", false}, {"comp", "Auto gain", 0, 1, 1, P_TOGGLE, "", "", false},
          {"mix", "Mix", 0, 1, 1, P_CONT, "%", "", false}}},
        {"filter", "Filter", "Tone", "Resonant state-variable filter. Cutoff is smoothed per sample; optional tempo-synced sweep.",
         {{"cutoff", "Cutoff", 20, 20000, 20000, P_CONT, "Hz", "", true}, {"type", "Type", 0, 4, 0, P_ENUM, "", "Low-pass 12|Low-pass 24|High-pass 12|High-pass 24|Band-pass", false},
          {"resonance", "Resonance", 0, 1, .2f, P_CONT, "%", "", false}, {"mod", "Mod amount", 0, 1, 0, P_CONT, "%", "", false},
          {"modrate", "Mod rate", 0, 4, 2, P_ENUM, "", "1 bar|1/2|1/4|1/8|1/16", false}}},
        {"phaser", "Phaser", "Motion", "Six all-pass stages swept by an LFO. Intensity scales depth and blend.",
         {{"intensity", "Intensity", 0, 1, .5f, P_CONT, "%", "", false}, {"rate", "Rate", .03f, 4, .4f, P_CONT, "Hz", "", true},
          {"depth", "Depth", 0, 1, .7f, P_CONT, "%", "", false}, {"feedback", "Feedback", 0, .8f, .4f, P_CONT, "%", "", false},
          {"mix", "Mix", 0, 1, .5f, P_CONT, "%", "", false}}},
        {"flanger", "Flanger", "Motion", "Short modulated delay mixed with the dry sound. Feedback is capped and soft-limited.",
         {{"amount", "Amount", 0, 1, .5f, P_CONT, "%", "", false}, {"rate", "Rate", .03f, 3, .25f, P_CONT, "Hz", "", true},
          {"depth", "Depth", 0, 1, .7f, P_CONT, "%", "", false}, {"feedback", "Feedback", -.85f, .85f, .5f, P_CONT, "%", "", false},
          {"mix", "Mix", 0, 1, .5f, P_CONT, "%", "", false}}},
        {"shimmer", "Shimmer", "Space", "A second reverb whose tail is pitch-shifted and fed back, building airy harmonics.",
         {{"intensity", "Intensity", 0, 1, .4f, P_CONT, "%", "", false}, {"interval", "Interval", 0, 4, 0, P_ENUM, "", "+12|+7|+19|+24|-12", false},
          {"decay", "Decay", .5f, 12, 5, P_CONT, "s", "", true}, {"tone", "Tone", 0, 1, .5f, P_CONT, "", "", false},
          {"mix", "Mix", 0, 1, .5f, P_CONT, "%", "", false}}},
        {"width", "Width", "Motion", "Mid/side width. Bass below the mono frequency stays centred; Mono check sums to mono.",
         {{"width", "Width", 0, 2, 1.3f, P_CONT, "%", "", false}, {"bassmono", "Mono bass below", 0, 400, 120, P_CONT, "Hz", "", false},
          {"balance", "Mid/side", -1, 1, 0, P_CONT, "", "", false}, {"monocheck", "Mono check", 0, 1, 0, P_TOGGLE, "", "", false}}},
        {"granular", "Granular", "Texture", "Grains taken from the last two seconds of audio, re-spread in time, pitch and space.",
         {{"intensity", "Intensity", 0, 1, .5f, P_CONT, "%", "", false}, {"size", "Grain size", 10, 400, 90, P_CONT, "ms", "", true},
          {"density", "Density", 2, 60, 18, P_CONT, "/s", "", true}, {"position", "Position", 0, 1.8f, .3f, P_CONT, "s", "", false},
          {"pitchvar", "Pitch spread", 0, 12, 0, P_CONT, "st", "", false}, {"mix", "Texture mix", 0, 1, .5f, P_CONT, "%", "", false}}},
        {"glide", "Glide", "Pitch", "Slides pitch between notes on analog, FM, voice and grain layers. Plucked-string and mallet layers strike at a fixed pitch.",
         {{"time", "Time", 0, 1.5f, .12f, P_CONT, "s", "", false}, {"mode", "Mode", 0, 2, 0, P_ENUM, "", "Legato (mono)|Always (mono)|Poly", false}}},
        {"tape", "Tape", "Tone", "Tape-style wow, flutter, saturation and high-end loss. Noise is off unless you add it.",
         {{"amount", "Amount", 0, 1, .5f, P_CONT, "%", "", false}, {"wow", "Wow", 0, 1, .4f, P_CONT, "%", "", false},
          {"flutter", "Flutter", 0, 1, .3f, P_CONT, "%", "", false}, {"sat", "Saturation", 0, 1, .4f, P_CONT, "%", "", false},
          {"noise", "Noise", 0, 1, 0, P_CONT, "%", "", false}, {"tone", "Tone", 0, 1, .6f, P_CONT, "", "", false}}},
        {"gate", "Gate / Pump", "Rhythm", "Tempo-synced volume movement with smoothed attack and release.",
         {{"depth", "Depth", 0, 1, .6f, P_CONT, "%", "", false}, {"pattern", "Pattern", 0, 5, 0, P_ENUM, "", "Pump 1/4|Pump 1/8|Pump 1/2|Gate 1/8|Gate 1/16|Trance 16", false},
          {"attack", "Attack", 1, 80, 6, P_CONT, "ms", "", true}, {"release", "Release", 10, 500, 80, P_CONT, "ms", "", true},
          {"mix", "Mix", 0, 1, 1, P_CONT, "%", "", false}}},
        {"octave", "Octave Layer", "Pitch", "A second synthesized voice per note, octaves away, phase-locked to the note.",
         {{"level", "Level", 0, 1, .5f, P_CONT, "%", "", false}, {"octave", "Octave", 0, 3, 2, P_ENUM, "", "-2|-1|+1|+2", false},
          {"tone", "Tone", 300, 18000, 6000, P_CONT, "Hz", "", true}, {"balance", "Original", 0, 1, 1, P_CONT, "%", "", false}}},
        {"resonator", "Resonator", "Tone", "Vowel filter (three formant peaks) or tuned metallic comb. Auto-gain keeps peaks under control. Not a formant shifter.",
         {{"intensity", "Intensity", 0, 1, .5f, P_CONT, "%", "", false}, {"mode", "Mode", 0, 1, 0, P_ENUM, "", "Vowel|Metallic", false},
          {"vowel", "Vowel", 0, 4, 0, P_CONT, "", "A|E|I|O|U", false}, {"freq", "Frequency", 40, 2000, 220, P_CONT, "Hz", "", true},
          {"resonance", "Resonance", 0, 1, .5f, P_CONT, "%", "", false}, {"mix", "Mix", 0, 1, .6f, P_CONT, "%", "", false}}},
        {"reverse", "Reverse & Swell", "Rhythm", "Swell fades every new note in; Reverse plays real reversed audio, one tempo-synced segment behind.",
         {{"amount", "Amount", 0, 1, .7f, P_CONT, "%", "", false}, {"mode", "Mode", 0, 2, 0, P_ENUM, "", "Swell|Reverse|Reverse + Swell", false},
          {"length", "Length", 0, 3, 2, P_ENUM, "", "1/8|1/4|1/2|1 bar", false}, {"rise", "Rise time", .005f, 3, .4f, P_CONT, "s", "", true}}},
        {"freeze", "Freeze", "Texture", "Captures the last moments of sound and sustains them as a cloud while you keep playing.",
         {{"level", "Level", 0, 1, .6f, P_CONT, "%", "", false}, {"frozen", "Freeze", 0, 1, 0, P_TOGGLE, "", "", false},
          {"texture", "Texture", 0, 1, .3f, P_CONT, "%", "", false}, {"release", "Release", .1f, 10, 2, P_CONT, "s", "", true}}},
        {"glue", "Glue / Punch", "Dynamics", "Glue: smooth compressor with conservative make-up. Punch: transient shaper that lifts attacks.",
         {{"amount", "Amount", 0, 1, .5f, P_CONT, "%", "", false}, {"mode", "Mode", 0, 1, 0, P_ENUM, "", "Glue|Punch", false},
          {"threshold", "Threshold", -40, 0, -18, P_CONT, "dB", "", false}, {"ratio", "Ratio", 1, 10, 3, P_CONT, ":1", "", false},
          {"attack", "Attack", .1f, 60, 15, P_CONT, "ms", "", true}, {"release", "Release", 20, 800, 160, P_CONT, "ms", "", true},
          {"output", "Output", -12, 6, 0, P_CONT, "dB", "", false}}},
    };
    return s;
}

inline std::string fxSchemaJson() {
    std::string j = "[";
    char buf[512];
    const auto& s = fxSchema();
    for (size_t e = 0; e < s.size(); ++e) {
        std::snprintf(buf, sizeof buf, "%s{\"id\":\"%s\",\"name\":\"%s\",\"category\":\"%s\",\"note\":\"%s\",\"params\":[",
                      e ? "," : "", s[e].id, s[e].name, s[e].category, s[e].note);
        j += buf;
        for (size_t i = 0; i < s[e].p.size(); ++i) {
            const auto& p = s[e].p[i];
            std::snprintf(buf, sizeof buf, "%s{\"id\":\"%s\",\"name\":\"%s\",\"min\":%g,\"max\":%g,\"def\":%g,\"kind\":%d,\"unit\":\"%s\",\"choices\":\"%s\",\"log\":%s}",
                          i ? "," : "", p.id, p.name, p.min, p.max, p.def, p.kind, p.unit, p.choices, p.log ? "true" : "false");
            j += buf;
        }
        j += "]}";
    }
    return j + "]";
}

// ------------------------------------------------------------------ small DSP helpers
struct XRng { uint32_t s = 0x9E3779B9u; float uni() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s >> 8) * (1.f / 16777216.f); } float bi() { return uni() * 2.f - 1.f; } };

struct Ramp {   // one-pole smoother; ~time constant in seconds
    float v = 0, t = 0, k = .001f;
    void setTime(float sec, float sr) { k = 1.f - std::exp(-1.f / (std::max(sec, 1e-4f) * sr)); }
    void snap(float x) { v = t = x; }
    inline float tick() { v += (t - v) * k; return v; }
};

struct Ring {   // delay line with cubic (Hermite) interpolation
    std::vector<float> b; int w = 0, mask = 0;
    void prepare(int minLen) { int n = 1; while (n < minLen + 8) n <<= 1; b.assign(n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill(b.begin(), b.end(), 0.f); }
    inline void push(float x) { b[w] = x; w = (w + 1) & mask; }
    inline float at(int d) const { return b[(w - 1 - d) & mask]; }
    inline float read(float d) const {   // d >= 1 samples behind the newest
        d = std::max(d, 1.f);
        int i = (int)d; float f = d - i;
        float y0 = at(i - 1), y1 = at(i), y2 = at(i + 1), y3 = at(i + 2);
        float c0 = y1, c1 = .5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - .5f * y3, c3 = .5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }
};

struct TSVF {   // TPT state-variable filter, coefficients settable per sample
    float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void set(float fc, float q, float sr) {
        fc = std::min(std::max(fc, 10.f), sr * .48f);
        g = std::tan(3.14159265f * fc / sr); k = 1.f / std::max(q, .2f);
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0; }
    inline void tick(float v0, float& lp, float& bp, float& hp) {
        float v3 = v0 - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
};

struct OnePole { float z = 0, a = 0; void setHz(float f, float sr) { a = std::exp(-6.2831853f * std::min(f, sr * .49f) / sr); } inline float lp(float x) { z = x + a * (z - x); return z; } inline float hp(float x) { return x - lp(x); } };

struct EnvFollow { float e = 0, ka = 0, kr = 0; void set(float aSec, float rSec, float sr) { ka = std::exp(-1.f / (std::max(aSec, 1e-5f) * sr)); kr = std::exp(-1.f / (std::max(rSec, 1e-5f) * sr)); }
    inline float tick(float x) { x = std::fabs(x); e = x > e ? x + ka * (e - x) : x + kr * (e - x); return e; } };

// 2x oversampler: 2nd-order-section elliptic-ish halfband via cascaded biquads (cheap, steep enough for saturation)
struct HalfBand {
    struct BQ { float b0, b1, b2, a1, a2, z1 = 0, z2 = 0; inline float t(float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } };
    BQ s[4];
    void prepare() {   // 8th-order Butterworth low-pass at 0.42 * (2*sr) / 2 -> cutoff 0.21 of the oversampled rate
        const float fc = .21f, Qs[4] = {.5098f, .6013f, .9000f, 2.5629f};
        for (int i = 0; i < 4; ++i) {
            float w = 6.2831853f * fc, cs = std::cos(w), al = std::sin(w) / (2 * Qs[i]), a0 = 1 + al;
            s[i] = {(1 - cs) / 2 / a0, (1 - cs) / a0, (1 - cs) / 2 / a0, -2 * cs / a0, (1 - al) / a0};
        }
    }
    inline float t(float x) { for (auto& q : s) x = q.t(x); return x; }
};

inline float beatsForDiv(int d) { static const float b[7] = {1.f, .5f, .75f, .25f, .125f, 2.f / 3.f, 1.f / 3.f}; return b[std::max(0, std::min(6, d))]; }

// ------------------------------------------------------------------ the rack
struct Rack {
    float sr = 44100, bpm = 120;
    float val[FX_COUNT][kMaxParams]{};
    bool on[FX_COUNT]{};
    Ramp mixOn[FX_COUNT];          // bypass crossfade per effect (10 ms) -> switching is click-free
    double pos = 0;                // transport position in samples (tempo sync)
    XRng rng;

    void prepare(float SR) {
        sr = SR;
        for (int e = 0; e < FX_COUNT; ++e) { mixOn[e].setTime(.01f, sr); mixOn[e].snap(0); }
        resetDefaults();
        // inserts
        chL.prepare((int)(.05f * sr)); chR.prepare((int)(.05f * sr));
        flL.prepare((int)(.02f * sr)); flR.prepare((int)(.02f * sr));
        tpL.prepare((int)(.05f * sr)); tpR.prepare((int)(.05f * sr));
        hbUpL.prepare(); hbUpR.prepare(); hbDnL.prepare(); hbDnR.prepare();
        for (StTrack* t : {&stCur, &stOld}) { t->l.assign((int)(4.5f * sr), 0); t->r.assign((int)(4.5f * sr), 0); t->pl.assign(1024, 0); t->pr.assign(1024, 0); }
        stHL.assign(1024, 0); stHR.assign(1024, 0); stFadeLen = (int)(.006f * sr);
        rvBufL.assign((int)(5 * sr), 0); rvBufR.assign((int)(5 * sr), 0);
        // post
        grL.prepare((int)(2.2f * sr)); grR.prepare((int)(2.2f * sr));
        fzL.assign((int)(1.6f * sr), 0); fzR.assign((int)(1.6f * sr), 0); fzRecL.prepare((int)(1.7f * sr)); fzRecR.prepare((int)(1.7f * sr));
        cmL.prepare((int)(sr / 30) + 8); cmR.prepare((int)(sr / 30) + 8); shim.prepare(sr);
    }
    void resetDefaults() {
        const auto& s = fxSchema();
        for (int e = 0; e < FX_COUNT; ++e) { for (size_t i = 0; i < s[e].p.size(); ++i) val[e][i] = s[e].p[i].def; on[e] = false; }
    }
    void setParam(int e, int i, float v) {
        const auto& s = fxSchema();
        if (e < 0 || e >= FX_COUNT || i < 0 || i >= (int)s[e].p.size()) return;
        const auto& d = s[e].p[i];
        v = std::min(std::max(v, d.min), d.max);
        if (d.kind != P_CONT) v = std::round(v);
        val[e][i] = v;
    }
    void setOn(int e, bool b) { if (e >= 0 && e < FX_COUNT) on[e] = b; }
    float v(int e, int i) const { return val[e][i]; }

    // state as "id.param=value;..." for plugin save/restore and preset storage
    std::string state() const {
        std::string out; char b[96]; const auto& s = fxSchema();
        for (int e = 0; e < FX_COUNT; ++e) {
            std::snprintf(b, sizeof b, "%s.on=%d;", s[e].id, on[e] ? 1 : 0); out += b;
            for (size_t i = 0; i < s[e].p.size(); ++i) { std::snprintf(b, sizeof b, "%s.%s=%g;", s[e].id, s[e].p[i].id, val[e][i]); out += b; }
        }
        return out;
    }
    void setState(const std::string& st) {
        const auto& s = fxSchema(); size_t p = 0;
        while (p < st.size()) {
            size_t semi = st.find(';', p); if (semi == std::string::npos) semi = st.size();
            std::string kv = st.substr(p, semi - p); p = semi + 1;
            size_t dot = kv.find('.'), eq = kv.find('=');
            if (dot == std::string::npos || eq == std::string::npos || eq < dot) continue;
            std::string fx = kv.substr(0, dot), par = kv.substr(dot + 1, eq - dot - 1); float x = std::strtof(kv.c_str() + eq + 1, nullptr);
            for (int e = 0; e < FX_COUNT; ++e) if (fx == s[e].id) {
                if (par == "on") on[e] = x > .5f;
                for (size_t i = 0; i < s[e].p.size(); ++i) if (par == s[e].p[i].id) setParam(e, (int)i, x);
            }
        }
    }

    // ---------- helpers
    inline float beatSamples() const { return 60.f / bpm * sr; }
    inline bool active(int e) { float m = mixOn[e].tick(); mixOn[e].t = on[e] ? 1.f : 0.f; return m > 1e-5f || on[e]; }
    inline float wetOf(int e) const { return mixOn[e].v; }

    // ================================================================ INSERTS
    // FILTER
    TSVF fA[2], fB[2]; Ramp fCut; bool fInit = false; float fLfoPh = 0;
    void filter(float& L, float& R) {
        if (!active(FX_FILTER)) { fInit = false; return; }
        float cutTarget = std::log2(v(FX_FILTER, 0));
        if (!fInit) { fCut.setTime(.015f, sr); fCut.snap(cutTarget); for (auto& f : fA) f.reset(); for (auto& f : fB) f.reset(); fInit = true; }
        fCut.t = cutTarget;
        static const float div[5] = {4, 2, 1, .5f, .25f};
        float rateBeats = div[(int)v(FX_FILTER, 4)];
        fLfoPh += 1.f / (rateBeats * beatSamples()); if (fLfoPh >= 1) fLfoPh -= 1;
        float lfo = std::sin(6.2831853f * fLfoPh);
        float cut = std::exp2(fCut.tick() + lfo * 3.f * v(FX_FILTER, 3));
        int type = (int)v(FX_FILTER, 1);
        float res = v(FX_FILTER, 2);
        bool four = type == 1 || type == 3;
        float q = type == 4 ? .7f + res * res * 9.f : .6f + res * res * (four ? 4.f : 6.f);
        // coefficients: every sample while the LFO sweeps (no zipper at high resonance), else every 4 samples
        bool moving = v(FX_FILTER, 3) > 0 || std::fabs(fCut.t - fCut.v) > 1e-3f;
        if (moving || (fCtr++ & 3) == 0) for (int c = 0; c < 2; ++c) { fA[c].set(cut, q, sr); fB[c].set(cut, four ? .62f : q, sr); }
        float comp = type == 4 ? 1.f : 1.f / (1.f + .3f * (q - .6f));   // resonant peak should not jump the level
        float in[2] = {L, R}, out[2];
        for (int c = 0; c < 2; ++c) {
            float lp, bp, hp; fA[c].tick(in[c], lp, bp, hp);
            float y = type <= 1 ? lp : type <= 3 ? hp : bp * fA[c].k;   // band-pass normalised to unity peak
            if (four) { fB[c].tick(y, lp, bp, hp); y = type == 1 ? lp : hp; }
            out[c] = y * comp;
        }
        float w = wetOf(FX_FILTER);
        L += (out[0] - L) * w; R += (out[1] - R) * w;
    }
    unsigned fCtr = 0;

    // RESONATOR (vowel = 3 parallel band-passes; metallic = tuned comb), with auto-gain
    TSVF vf[2][3]; Ring cmL, cmR; float cmLp[2]{}; EnvFollow rInEnv, rOutEnv; Ramp rVow, rFreq; bool rInit = false;
    void resonator(float& L, float& R) {
        if (!active(FX_RESONATOR)) { rInit = false; return; }
        if (!rInit) { rVow.setTime(.03f, sr); rVow.snap(v(FX_RESONATOR, 2)); rFreq.setTime(.03f, sr); rFreq.snap(v(FX_RESONATOR, 3)); rInEnv.set(.005f, .25f, sr); rOutEnv.set(.005f, .25f, sr); cmL.clear(); cmR.clear(); for (auto& a : vf) for (auto& f : a) f.reset(); rInit = true; }
        rVow.t = v(FX_RESONATOR, 2); rFreq.t = v(FX_RESONATOR, 3);
        float inten = v(FX_RESONATOR, 0), res = v(FX_RESONATOR, 4), mix = v(FX_RESONATOR, 5);
        float vow = rVow.tick(), fr = rFreq.tick();
        float in[2] = {L, R}, wet[2];
        if ((int)v(FX_RESONATOR, 1) == 0) {
            static const float F[5][3] = {{800, 1150, 2900}, {400, 1700, 2600}, {290, 2200, 3000}, {450, 800, 2830}, {325, 700, 2530}};
            static const float G[3] = {1.f, .63f, .32f};
            int i0 = std::min(3, (int)vow); float t = vow - i0;
            if ((rCtr++ & 7) == 0) for (int k = 0; k < 3; ++k) {
                float f = F[i0][k] + (F[i0 + 1][k] - F[i0][k]) * t;
                for (int c = 0; c < 2; ++c) vf[c][k].set(f, 2.f + res * 14.f, sr);
            }
            for (int c = 0; c < 2; ++c) { float s = 0, lp, bp, hp; for (int k = 0; k < 3; ++k) { vf[c][k].tick(in[c], lp, bp, hp); s += bp * G[k]; } wet[c] = s * 2.2f; }
        } else {
            float d = sr / fr, fb = .55f + res * .42f;   // <= 0.97, with a damping low-pass in the loop
            Ring* rr[2] = {&cmL, &cmR};
            for (int c = 0; c < 2; ++c) {
                float y = rr[c]->read(d * (c ? 1.003f : 1.f));
                cmLp[c] = y + .25f * (cmLp[c] - y);
                float s = in[c] + std::tanh(cmLp[c] * fb);
                rr[c]->push(s); wet[c] = y * (1.f - fb * .7f);
            }
        }
        // intensity = how much of the resonated colour is heard; auto-gain keeps it near the input level
        float ein = rInEnv.tick((L + R) * .5f), eout = rOutEnv.tick((wet[0] + wet[1]) * .5f);
        float ag = std::min(4.f, (ein + 1e-4f) / (eout + 1e-4f));
        float amt = inten * mix * wetOf(FX_RESONATOR);
        L += (wet[0] * ag - L) * amt; R += (wet[1] * ag - R) * amt;
    }
    unsigned rCtr = 0;

    // SATURATION (2x oversampled), level-matched
    HalfBand hbUpL, hbUpR, hbDnL, hbDnR; OnePole sTilt[2], sFizz[2]; Ramp sDrive, sTone; EnvFollow sIn, sOut; float sGain = 1;
    inline float shape(float x, int mode, float drive) {
        float g = 1.f + drive * drive * (mode == 2 ? 34.f : 24.f);
        switch (mode) {
            case 1: { float y = x * g + .25f * drive; return (std::tanh(y) - std::tanh(.25f * drive)) / std::tanh(g) ; }   // tube: asymmetric, even harmonics
            case 2: { float y = x * g; return y / std::pow(1.f + std::pow(std::fabs(y), 4.f), .25f) / (g / std::pow(1.f + g * g * g * g, .25f)); }  // crunch: harder knee
            default: return std::tanh(x * g) / std::tanh(g);   // warm
        }
    }
    void saturation(float& L, float& R) {
        if (!active(FX_SATURATION)) { sIn.e = sOut.e = 0; sGain = 1; return; }
        if (sIn.ka == 0) { sIn.set(.08f, .08f, sr); sOut.set(.08f, .08f, sr); }
        if (sDrive.k == .001f) { sDrive.setTime(.02f, sr); sDrive.snap(v(FX_SATURATION, 0)); sTone.setTime(.02f, sr); sTone.snap(v(FX_SATURATION, 2)); }
        sDrive.t = v(FX_SATURATION, 0); sTone.t = v(FX_SATURATION, 2);
        float drive = sDrive.tick(); int mode = (int)v(FX_SATURATION, 1); float tone = sTone.tick(), mix = v(FX_SATURATION, 4);   // drive/tone glide per sample: no zipper
        float in[2] = {L, R}, out[2];
        HalfBand* up[2] = {&hbUpL, &hbUpR}; HalfBand* dn[2] = {&hbDnL, &hbDnR};
        for (int c = 0; c < 2; ++c) {
            // zero-stuff upsample, shape at 2x, filter, decimate
            float a = up[c]->t(in[c] * 2.f), b = up[c]->t(0.f);
            a = shape(a, mode, drive); b = shape(b, mode, drive);
            float y = dn[c]->t(a); dn[c]->t(b);
            if (mode == 2) { sFizz[c].setHz(9500, sr); y = sFizz[c].lp(y); }   // crunch: round off the fizz above ~10 kHz
            // tone: tilt around ~1.2 kHz
            sTilt[c].setHz(1200, sr);
            float lo = sTilt[c].lp(y), hi = y - lo;
            out[c] = lo * (1.f - .4f * tone) + hi * (1.f + .5f * tone);
        }
        if (v(FX_SATURATION, 3) > .5f) {   // level-match output to input (slow, so dynamics survive)
            // mean-square followers (80 ms): matches loudness, not peaks
            float mi = (L * L + R * R) * .5f, mo = (out[0] * out[0] + out[1] * out[1]) * .5f;
            float ei = std::sqrt(std::max(0.f, sIn.tick(mi))), eo = std::sqrt(std::max(0.f, sOut.tick(mo)));
            float target = std::min(1.5f, (ei + 1e-5f) / (eo + 1e-5f));
            sGain += (target - sGain) * .0005f;
        } else sGain = 1;
        float w = mix * wetOf(FX_SATURATION);
        L += (out[0] * sGain - L) * w; R += (out[1] * sGain - R) * w;
    }

    // TAPE: wow/flutter via modulated delay, soft saturation, HF loss, optional hiss
    Ring tpL, tpR; float wowPh = 0, flPh = 0, flJit = 0; OnePole tpTone[2]; Ramp tpDepth;
    void tape(float& L, float& R) {
        if (!active(FX_TAPE)) return;
        float amt = v(FX_TAPE, 0), wow = v(FX_TAPE, 1) * amt, flu = v(FX_TAPE, 2) * amt, sat = v(FX_TAPE, 3) * amt, noise = v(FX_TAPE, 4), tone = v(FX_TAPE, 5);
        wowPh += .55f / sr; if (wowPh >= 1) wowPh -= 1;
        flPh += (6.5f + flJit) / sr; if (flPh >= 1) { flPh -= 1; flJit = rng.bi() * 1.5f; }
        // depth in samples: wow up to ~14 cents at 0.55 Hz, flutter up to ~4 cents at ~6.5 Hz
        float wowS = wow * .0026f * sr / (6.2831853f * .55f) * .3f, fluS = flu * .00012f * sr;
        float d = .006f * sr + wowS * (1 + std::sin(6.2831853f * wowPh)) + fluS * (1 + std::sin(6.2831853f * flPh));
        tpL.push(L); tpR.push(R);
        float a = tpL.read(d), b = tpR.read(d + .3f);
        float g = 1.f + sat * 4.f;   // unity gain for quiet signals; only peaks are rounded off
        a = std::tanh(a * g) / g; b = std::tanh(b * g) / g;
        float fc = 5500.f + tone * 14000.f * (1.f - .4f * amt);
        tpTone[0].setHz(fc, sr); tpTone[1].setHz(fc, sr);
        a = tpTone[0].lp(a); b = tpTone[1].lp(b);
        if (noise > 0) { a += rng.bi() * noise * .004f; b += rng.bi() * noise * .004f; }
        // the delay is always present while on, so dry/wet crossfade is the bypass ramp only
        float w = wetOf(FX_TAPE);
        L += (a - L) * w; R += (b - R) * w;
    }

    // GLUE / PUNCH
    EnvFollow gEnv, pFast, pSlow; float gGr = 1, gGrDb = 0, gAvg = 0, pG = 1, pRms = 0; bool gAvgInit = false;
    void glue(float& L, float& R) {
        if (!active(FX_GLUE)) { gGr = 1; gAvgInit = false; return; }
        float amt = v(FX_GLUE, 0); int mode = (int)v(FX_GLUE, 1);
        float th = v(FX_GLUE, 2), ratio = v(FX_GLUE, 3), att = v(FX_GLUE, 4) * .001f, rel = v(FX_GLUE, 5) * .001f, outDb = v(FX_GLUE, 6);
        float x = std::max(std::fabs(L), std::fabs(R));
        float g = 1;
        if (mode == 0) {
            gEnv.set(.003f, .03f, sr);   // fast RMS level detector; the character comes from the gain smoothing below
            float e = std::sqrt(std::max(0.f, gEnv.tick(x * x))) * 1.41f, lvl = 20.f * std::log10(e + 1e-9f);
            const float knee = 6.f; float over = lvl - th, grDb = 0;
            if (over > knee / 2) grDb = over * (1.f - 1.f / ratio);
            else if (over > -knee / 2) { float t = over + knee / 2; grDb = (1.f - 1.f / ratio) * t * t / (2 * knee); }
            // gain reduction is smoothed in dB with the attack / release times (log-domain: no pumping artefacts)
            float k = grDb > gGrDb ? 1.f - std::exp(-1.f / (std::max(att, 1e-4f) * sr)) : 1.f - std::exp(-1.f / (std::max(rel, 1e-3f) * sr));
            gGrDb += (grDb - gGrDb) * k;
            // auto make-up = the average gain reduction over the last ~1.5 s of playing: output loudness stays matched
            // to the input, so Glue changes the dynamics, not the volume
            // starts from the static estimate (half the reduction at threshold), then learns the real average
            if (!gAvgInit) { gAvg = .5f * -th * (1.f - 1.f / ratio); gAvgInit = true; }
            if (e > 3e-4f) gAvg += (gGrDb - gAvg) * (1.f / (.8f * sr));
            float makeup = std::min(24.f, std::max(0.f, gAvg));
            g = std::pow(10.f, (-gGrDb + makeup) * amt / 20.f);   // amount scales the whole gain change
        } else {
            // classic transient detection: same release, different attack. In steady sound both envelopes agree.
            pFast.set(.0003f, rel, sr); pSlow.set(std::max(.015f, att * 1.5f), rel, sr);
            pRms += (x * x - pRms) * (1.f - std::exp(-1.f / (.004f * sr)));   // 4 ms power: removes waveform ripple
            float xr = std::sqrt(std::max(0.f, pRms));
            float f = pFast.tick(xr), s = pSlow.tick(xr);
            // attacks up to +8 dB, the body pulled down 4 dB: more contrast, not just more level
            float boostDb = std::min(8.f, std::max(0.f, 20.f * std::log10((f + 1e-6f) / (s + 1e-6f))) * 2.5f) * amt;
            float tg = std::pow(10.f, (boostDb - 4.f * amt) / 20.f);
            float ceil = .85f / std::max(x, 1e-6f);   // lifted attacks never push a peak past about -1.4 dBFS
            tg = std::min(tg, std::max(std::pow(10.f, -4.f * amt / 20.f), ceil));
            pG += (tg - pG) * (1.f - std::exp(-1.f / (.0015f * sr)));   // 1.5 ms smoothing: shaped attacks, no clicks
            g = pG;
        }
        g *= std::pow(10.f, outDb / 20.f);
        float w = wetOf(FX_GLUE);
        float gg = 1.f + (g - 1.f) * w;
        L *= gg; R *= gg;
    }

    // CHORUS (3 voices per side)
    Ring chL, chR; float chPh = 0;
    void chorus(float& L, float& R) {
        if (!active(FX_CHORUS)) return;
        float inten = v(FX_CHORUS, 0), rate = v(FX_CHORUS, 1), depth = v(FX_CHORUS, 2) * inten, mix = v(FX_CHORUS, 3) * std::min(1.f, inten * 2.f), spread = v(FX_CHORUS, 4);
        chL.push(L); chR.push(R);
        chPh += rate / sr; if (chPh >= 1) chPh -= 1;
        float dm = (.3f + depth * 5.7f) * .001f * sr;
        float wl = 0, wr = 0;
        for (int i = 0; i < 3; ++i) {
            float base = (8.f + 4.f * i) * .001f * sr;
            float pl = std::sin(6.2831853f * (chPh + i / 3.f)), pr = std::sin(6.2831853f * (chPh + i / 3.f + .5f * spread));
            wl += chL.read(base + dm * (1 + pl) * .5f); wr += chR.read(base + dm * (1 + pr) * .5f);
        }
        wl /= 3; wr /= 3;
        float ml = wl, mr = wr;
        float w = mix * wetOf(FX_CHORUS);
        // three decorrelated voices averaged: blend dry down gently as wet comes up (keeps level steady)
        L = L * (1.f - .3f * w) + ml * w * .7f; R = R * (1.f - .3f * w) + mr * w * .7f;
    }

    // PHASER (6 all-pass stages)
    float phZ[2][6]{}, phFb[2]{}, phPh = 0;
    void phaser(float& L, float& R) {
        if (!active(FX_PHASER)) return;
        float inten = v(FX_PHASER, 0), rate = v(FX_PHASER, 1), depth = v(FX_PHASER, 2) * inten, fb = v(FX_PHASER, 3), mix = v(FX_PHASER, 4) * std::min(1.f, inten * 2.f);
        phPh += rate / sr; if (phPh >= 1) phPh -= 1;
        float in[2] = {L, R}, out[2];
        for (int c = 0; c < 2; ++c) {
            float l = .5f + .5f * std::sin(6.2831853f * (phPh + c * .12f));
            float fc = 180.f * std::exp2(l * depth * 4.5f);   // 180 Hz .. ~4 kHz
            float t = std::tan(3.14159265f * std::min(fc, sr * .45f) / sr), a = (t - 1) / (t + 1);
            float x = in[c] + std::tanh(phFb[c] * fb);
            for (int s = 0; s < 6; ++s) { float y = a * x + phZ[c][s]; phZ[c][s] = x - a * y; x = y; }
            phFb[c] = x; out[c] = x;
        }
        float w = mix * wetOf(FX_PHASER);
        // dry + all-passed wet at equal parts creates the moving notches; small make-up keeps level steady
        L = (L * (1 - w * .5f) + out[0] * w * .5f) * (1.f + w * .25f); R = (R * (1 - w * .5f) + out[1] * w * .5f) * (1.f + w * .25f);
    }

    // FLANGER
    Ring flL, flR; float flPhs = 0, flFbL = 0, flFbR = 0; OnePole flDc[2];
    void flanger(float& L, float& R) {
        if (!active(FX_FLANGER)) return;
        float amt = v(FX_FLANGER, 0), rate = v(FX_FLANGER, 1), depth = v(FX_FLANGER, 2), fb = v(FX_FLANGER, 3), mix = v(FX_FLANGER, 4) * amt;
        flPhs += rate / sr; if (flPhs >= 1) flPhs -= 1;
        float tri = 2.f * std::fabs(2.f * flPhs - 1.f) - 1.f, tri2 = 2.f * std::fabs(2.f * std::fmod(flPhs + .25f, 1.f) - 1.f) - 1.f;
        float minD = .0003f * sr, range = (.0003f + depth * .0065f) * sr;
        float dl = minD + range * (.5f + .5f * tri), dr = minD + range * (.5f + .5f * tri2);
        flL.push(L + std::tanh(flFbL * fb)); flR.push(R + std::tanh(flFbR * fb));
        float a = flL.read(dl), b = flR.read(dr);
        flFbL = flDc[0].hp(a); flFbR = flDc[1].hp(b);
        flDc[0].setHz(20, sr); flDc[1].setHz(20, sr);
        float w = mix * wetOf(FX_FLANGER);
        L = (L * (1 - w * .5f) + a * w * .5f) * (1.f + w * .2f); R = (R * (1 - w * .5f) + b * w * .5f) * (1.f + w * .2f);
    }

    // STUTTER (seamless loop + gate), tempo synced.
    // A window (1 beat; 2 beats for 1/4 slices) either plays live or captures its first slice and loops it.
    // The loop seam crossfades into the audio just before the slice; when a looping window ends, the old loop
    // (with its gate) keeps running for a 6 ms raised-cosine crossfade into whatever comes next.
    struct StTrack { std::vector<float> l, r, pl, pr; int slice = 0, x = 256, pos = 0; bool on = false; float gate = 1; };
    StTrack stCur, stOld; int stFade = 0, stFadeLen = 256; std::vector<float> stHL, stHR; int stHw = 0;
    static inline float loopAt(const std::vector<float>& buf, const std::vector<float>& pre, int sl, int x, int k) {
        int c = (k - (sl - x)) % sl;
        if (c < x) { float w = .5f - .5f * std::cos(3.14159265f * (c + .5f) / x); return buf[sl - x + c] * (1.f - w) + pre[c] * w; }
        return buf[c - x];
    }
    static inline float gateAt(const StTrack& t) {
        if (t.gate >= .999f || t.pos < t.slice) return 1.f;
        int rel = t.pos - t.slice, c = rel % t.slice, rep = rel / t.slice;
        float open = t.gate * t.slice, edge = std::max(64.f, t.slice * .03f);
        float g = c < open ? std::min(1.f, (open - c) / edge) : 0.f;
        if (rep > 0) g = std::min(g, std::min(1.f, (c + 1.f) / edge));
        return .5f - .5f * std::cos(3.14159265f * g);
    }
    // output of a track for the current sample (live input passes until the loop region starts)
    static inline void trackOut(const StTrack& t, float liveL, float liveR, float& oL, float& oR) {
        if (!t.on || t.pos < t.slice - t.x) { oL = liveL; oR = liveR; return; }
        float g = gateAt(t);
        oL = loopAt(t.l, t.pl, t.slice, t.x, t.pos) * g; oR = loopAt(t.r, t.pr, t.slice, t.x, t.pos) * g;
    }
    void stutter(float& L, float& R) {
        bool act = active(FX_STUTTER);
        float liveL = L, liveR = R;
        if (act) {
            float inten = v(FX_STUTTER, 0), mix = v(FX_STUTTER, 3);
            static const float div[4] = {1.f, .5f, .25f, .125f};
            float divB = div[(int)v(FX_STUTTER, 1)];
            int beat = std::max(256, (int)beatSamples());
            int window = divB >= 1.f ? 2 * beat : beat;
            int posIn = (int)std::fmod(pos, (double)window);
            if (posIn == 0) {
                if (stCur.on && stCur.pos >= stCur.slice - stCur.x) { std::swap(stCur, stOld); stFade = stFadeLen; }
                stCur.on = on[FX_STUTTER] && rng.uni() < inten;
                stCur.slice = std::min((int)stCur.l.size() - 1, std::max(512, (int)(divB * beat)));
                stCur.x = std::max(32, std::min(256, stCur.slice / 8));
                stCur.pos = 0;
                if (stCur.on) for (int i = 0; i < stCur.x; ++i) { int h = (stHw - stCur.x + i + 1024) & 1023; stCur.pl[i] = stHL[h]; stCur.pr[i] = stHR[h]; }
            }
            stCur.gate = v(FX_STUTTER, 2);
            if (stCur.on && stCur.pos < stCur.slice) { stCur.l[stCur.pos] = L; stCur.r[stCur.pos] = R; }
            float oL, oR; trackOut(stCur, L, R, oL, oR);
            if (stCur.on) ++stCur.pos;
            if (stFade > 0) {
                float aL, aR; trackOut(stOld, L, R, aL, aR); ++stOld.pos;
                float w = .5f - .5f * std::cos(3.14159265f * (float)(stFadeLen - stFade) / stFadeLen);
                oL = aL * (1 - w) + oL * w; oR = aR * (1 - w) + oR * w; --stFade;
            }
            float wv = mix * wetOf(FX_STUTTER);
            L = liveL + (oL - liveL) * wv; R = liveR + (oR - liveR) * wv;
        }
        stHL[stHw] = liveL; stHR[stHw] = liveR; stHw = (stHw + 1) & 1023;
    }

    // GATE / PUMP
    float gtGain = 1;
    void gate(float& L, float& R) {
        if (!active(FX_GATE)) { gtGain = 1; return; }
        float depth = v(FX_GATE, 0), att = v(FX_GATE, 2) * .001f, rel = v(FX_GATE, 3) * .001f, mix = v(FX_GATE, 4);
        int pat = (int)v(FX_GATE, 1);
        float beat = beatSamples(), target = 1;
        auto phase = [&](float beats) { return (float)(std::fmod(pos, (double)(beats * beat)) / (beats * beat)); };
        if (pat <= 2) {   // pump: duck at each division start, recover along a curve
            float ph = phase(pat == 0 ? 1.f : pat == 1 ? .5f : 2.f);
            target = 1.f - depth * std::pow(std::max(0.f, 1.f - ph / .6f), 2.f);
        } else if (pat <= 4) {   // straight gate: open for the first half of each step
            float ph = phase(pat == 3 ? .5f : .25f);
            target = ph < .5f ? 1.f : 1.f - depth;
        } else {   // 16-step trance pattern
            static const int P[16] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1};
            int step = (int)(std::fmod(pos, (double)(4 * beat)) / (beat / 4)) & 15;
            float ph = phase(.25f);
            target = (P[step] && ph < .7f) ? 1.f : 1.f - depth;
        }
        float k = target < gtGain ? 1.f - std::exp(-1.f / (std::max(att, 1e-4f) * sr)) : 1.f - std::exp(-1.f / (std::max(rel, 1e-3f) * sr));
        gtGain += (target - gtGain) * k;
        float g = 1.f + (gtGain - 1.f) * mix * wetOf(FX_GATE);
        L *= g; R *= g;
    }

    // REVERSE & SWELL
    std::vector<float> rvBufL, rvBufR; int rvSeg = 0, rvW = 0, rvSegLen = 0; float swellGain = 1; bool swellArmed = false;
    void noteOnEvent() { if (on[FX_REVERSE]) { swellGain = 0; swellArmed = true; } }
    void reverse(float& L, float& R) {
        if (!active(FX_REVERSE)) { swellGain = 1; return; }
        float amt = v(FX_REVERSE, 0), rise = v(FX_REVERSE, 3); int mode = (int)v(FX_REVERSE, 1);
        static const float lens[4] = {.5f, 1.f, 2.f, 4.f};
        int segLen = std::min((int)rvBufL.size() / 2 - 1, (int)(lens[(int)v(FX_REVERSE, 2)] * beatSamples()));
        float wetL = L, wetR = R;
        if (mode >= 1) {
            // record into a double buffer; play the previous segment backwards, windowed at both ends
            int posIn = (int)std::fmod(pos, (double)segLen);
            if (posIn == 0) { rvSeg ^= 1; rvSegLen = segLen; }
            int wBase = rvSeg * (rvBufL.size() / 2), rBase = (rvSeg ^ 1) * (rvBufL.size() / 2);
            rvBufL[wBase + posIn] = L; rvBufR[wBase + posIn] = R;
            int ri = rvSegLen - 1 - std::min(posIn, rvSegLen - 1);
            int edge = std::max(64, std::min(rvSegLen / 6, (int)(.012f * sr)));
            float win = std::min(1.f, std::min((float)posIn, (float)(rvSegLen - 1 - posIn)) / edge);
            win = .5f - .5f * std::cos(3.14159265f * win);
            wetL = rvBufL[rBase + ri] * win; wetR = rvBufR[rBase + ri] * win;
        }
        if (mode == 0 || mode == 2) {   // swell: every new note fades in over the rise time (eased)
            float k = 1.f - std::exp(-4.f / (std::max(rise, .005f) * sr));
            swellGain += (1.f - swellGain) * k;
            float s = swellGain * swellGain;
            wetL *= s; wetR *= s;
        }
        float w = amt * wetOf(FX_REVERSE);
        L += (wetL - L) * w; R += (wetR - R) * w;
    }

    // ================================================================ POST (after the main reverb)
    // SHIMMER: its own FDN with a pitch shifter in the feedback path
    struct ShimVerb {
        static constexpr int N = 8;
        Ring line[N]; Ring ap[4]; float len[N]{}, g[N]{}, lpz[N]{}, damp = .4f, fb = 0, fbGain = .9f; int apl[4]{};
        // pitch shifter (3 heads)
        std::vector<float> ps; int pw = 0; float pph = 0, pwin = 1800, ratio = 2;
        void prepare(float sr) {
            for (auto& l : line) l.prepare((int)(.2f * sr));
            for (auto& a : ap) a.prepare(2048);
            ps.assign(8192, 0.f); pwin = .041f * sr;
            static const float apb[4] = {4.7f, 3.6f, 12.7f, 9.3f};
            for (int i = 0; i < 4; ++i) apl[i] = (int)(apb[i] * .001f * sr);
        }
        void set(float decay, float tone, float semis, float sr) {
            static const float base[N] = {41.1f, 47.3f, 53.9f, 59.3f, 67.1f, 73.7f, 79.3f, 87.1f};
            for (int i = 0; i < N; ++i) { len[i] = base[i] * 1.6f * .001f * sr; g[i] = std::pow(.001f, len[i] / (decay * sr)); }
            damp = .75f * (1.f - tone); ratio = std::exp2(semis / 12.f);
            fbGain = .9f * std::min(1.f, 3.f / decay);   // longer tails keep less of the shifted feedback: never builds up
        }
        inline float shift(float x) {
            ps[pw] = x; float y = 0;
            for (int h = 0; h < 3; ++h) {
                float p = pph + h / 3.f; if (p >= 1) p -= 1;
                float d = 1.f + p * pwin, rp = (float)pw - d; while (rp < 0) rp += (float)ps.size();
                int i0 = (int)rp; float fr = rp - i0; int i1 = (i0 + 1) & 8191;
                float s = std::sin(3.14159265f * p);
                y += (ps[i0] + (ps[i1] - ps[i0]) * fr) * s * s;
            }
            pph -= (ratio - 1.f) / pwin; while (pph < 0) pph += 1; while (pph >= 1) pph -= 1;
            pw = (pw + 1) & 8191;
            return y * (2.f / 3.f);
        }
        inline void tick(float in, float& oL, float& oR) {
            float x = in + fb;
            for (int i = 0; i < 4; ++i) { float d = ap[i].at(apl[i]); float v = x + .6f * d; ap[i].push(v); x = d - .6f * v; }
            float v[N], sum = 0;
            for (int i = 0; i < N; ++i) { float d = line[i].at((int)len[i]); lpz[i] = (1 - damp) * d + damp * lpz[i]; v[i] = lpz[i] * g[i]; sum += v[i]; }
            sum *= 2.f / N;
            for (int i = 0; i < N; ++i) line[i].push(v[i] - sum + x * ((i & 1) ? -.5f : .5f));
            oL = (v[0] + v[2] - v[4] + v[6]) * .6f; oR = (v[1] - v[3] + v[5] + v[7]) * .6f;
            // loop gain 0.45 with soft limit: measured to always decay (fx tests)
            fb = .5f * std::tanh(shift((oL + oR) * .5f) * fbGain);
        }
    } shim;
    float shimSet[3] = {-1, -1, -1};
    void shimmer(float& L, float& R) {
        if (!active(FX_SHIMMER)) return;
        static const float iv[5] = {12, 7, 19, 24, -12};
        float decay = v(FX_SHIMMER, 2), tone = v(FX_SHIMMER, 3), semis = iv[(int)v(FX_SHIMMER, 1)];
        if (shimSet[0] != decay || shimSet[1] != tone || shimSet[2] != semis) { shim.set(decay, tone, semis, sr); shimSet[0] = decay; shimSet[1] = tone; shimSet[2] = semis; }
        float oL, oR; shim.tick((L + R) * .5f, oL, oR);
        float w = v(FX_SHIMMER, 0) * v(FX_SHIMMER, 4) * .9f * wetOf(FX_SHIMMER);
        L += oL * w; R += oR * w;
    }

    // FREEZE: capture recent audio and sustain it with overlapping grains (no feedback -> stable forever)
    Ring fzRecL, fzRecR; std::vector<float> fzL, fzR; int fzLen = 0; bool fzWas = false; Ramp fzLevel;
    struct FzGrain { float pos, len, t, gl, gr, rate; bool on; } fzg[6]{}; float fzTimer = 0;
    void freeze(float& L, float& R) {
        fzRecL.push(L); fzRecR.push(R);
        bool act = active(FX_FREEZE);
        bool frozen = on[FX_FREEZE] && v(FX_FREEZE, 1) > .5f;
        if (frozen && !fzWas) {   // capture: copy the last 1.5 s
            fzLen = (int)fzL.size();
            for (int i = 0; i < fzLen; ++i) { fzL[i] = fzRecL.at(fzLen - 1 - i); fzR[i] = fzRecR.at(fzLen - 1 - i); }
            for (auto& g : fzg) g.on = false;
            fzTimer = 0;
            fzLevel.setTime(.15f, sr);
        }
        if (!frozen && fzWas) fzLevel.setTime(std::max(.1f, v(FX_FREEZE, 3)) / 4.f, sr);
        fzWas = frozen;
        fzLevel.t = frozen ? 1.f : 0.f;
        float lvl = fzLevel.tick();
        if (!act && lvl < 1e-5f) return;
        if (lvl < 1e-6f || fzLen == 0) return;
        float tex = v(FX_FREEZE, 2);
        fzTimer -= 1;
        if (fzTimer <= 0) {
            float glen = (.55f - .4f * tex) * sr;
            fzTimer = glen * (.25f + .1f * rng.uni());   // ~3-4 overlapping grains
            for (auto& g : fzg) if (!g.on) {
                g.on = true; g.len = glen * (.85f + .3f * rng.uni()); g.t = 0;
                g.pos = rng.uni() * std::max(1.f, fzLen - g.len * 1.2f - 2);
                g.rate = std::exp2(rng.bi() * tex * .08f);   // texture adds a little pitch shimmer
                float p = rng.bi() * (.3f + .6f * tex); g.gl = std::sqrt(.5f * (1 - p)); g.gr = std::sqrt(.5f * (1 + p));
                break;
            }
        }
        float oL = 0, oR = 0;
        for (auto& g : fzg) if (g.on) {
            float w = std::sin(3.14159265f * g.t / g.len); w *= w;
            float rp = g.pos + g.t * g.rate; int i0 = (int)rp; float fr = rp - i0;
            if (i0 + 1 >= fzLen) { g.on = false; continue; }
            float a = fzL[i0] + (fzL[i0 + 1] - fzL[i0]) * fr, b = fzR[i0] + (fzR[i0 + 1] - fzR[i0]) * fr;
            oL += (a * .7f + b * .3f) * w * g.gl * 1.41f; oR += (b * .7f + a * .3f) * w * g.gr * 1.41f;
            if ((g.t += 1) >= g.len) g.on = false;
        }
        float k = v(FX_FREEZE, 0) * lvl * .75f;
        L += oL * k; R += oR * k;
    }

    // GRANULAR: grains from the last ~2 s of audio
    Ring grL, grR;
    struct Grain { float start, len, t, rate, gl, gr; bool on; } grn[32]{}; float grTimer = 0;
    void granular(float& L, float& R) {
        grL.push(L); grR.push(R);
        if (!active(FX_GRANULAR)) return;
        float inten = v(FX_GRANULAR, 0), size = v(FX_GRANULAR, 1) * .001f * sr, dens = v(FX_GRANULAR, 2) * (.3f + .7f * inten);
        float posBack = v(FX_GRANULAR, 3) * sr, pv = v(FX_GRANULAR, 4), mix = v(FX_GRANULAR, 5);
        grTimer -= 1;
        if (grTimer <= 0) {
            grTimer = sr / std::max(.5f, dens) * (.5f + rng.uni());
            for (auto& g : grn) if (!g.on) {
                g.on = true; g.len = size * (.7f + .6f * rng.uni()); g.t = 0;
                float semis = pv > 0 ? std::round(rng.bi() * pv) : 0.f;   // quantized: stays musical
                g.rate = std::exp2(semis / 12.f);
                float need = g.len * std::max(1.f, g.rate) + 4;
                g.start = std::min((float)grL.mask - 8, posBack + need + rng.uni() * size * .5f);   // samples back from "now"
                float p = rng.bi() * .8f; g.gl = std::sqrt(.5f * (1 - p)); g.gr = std::sqrt(.5f * (1 + p));
                break;
            }
        }
        float oL = 0, oR = 0; int n = 0;
        for (auto& g : grn) if (g.on) {
            float w = std::sin(3.14159265f * g.t / g.len); w *= w;
            float back = g.start - g.t * g.rate + g.t;   // read head moves forward through the past at `rate`
            if (back < 2) { g.on = false; continue; }
            oL += grL.read(back) * w * g.gl; oR += grR.read(back) * w * g.gr; ++n;
            if ((g.t += 1) >= g.len) g.on = false;
        }
        float norm = 1.4f / std::sqrt(std::max(1.f, dens * size / sr));
        float w = mix * wetOf(FX_GRANULAR);
        float lvl = (.5f + .5f * inten);
        L = L * (1 - w) + oL * norm * lvl * w + L * 0; R = R * (1 - w) + oR * norm * lvl * w;
    }

    // WIDTH (mid/side, bass kept mono, mono check)
    TSVF wLp[2]; float wSetHz = -1;
    void width(float& L, float& R) {
        if (!active(FX_WIDTH)) return;
        float wdt = v(FX_WIDTH, 0), bm = v(FX_WIDTH, 1), bal = v(FX_WIDTH, 2);
        float m = (L + R) * .5f, s = (L - R) * .5f;
        float w = wetOf(FX_WIDTH);
        if (bm > 10) {   // remove side content below the mono frequency (Linkwitz-Riley style: two 2-pole stages)
            if (bm != wSetHz) { wLp[0].set(bm, .7071f, sr); wLp[1].set(bm, .7071f, sr); wSetHz = bm; }
            // 4th-order (Linkwitz-Riley) high-pass on the side signal only: no stereo content below the cutoff
            float lp, bp, hp; wLp[0].tick(s, lp, bp, hp); float hp2; wLp[1].tick(hp, lp, bp, hp2);
            s += (hp2 - s) * w;
        }
        float mg = bal > 0 ? 1.f - .5f * bal : 1.f, sg = bal < 0 ? 1.f + .5f * bal : 1.f;   // + favours sides, - favours centre
        float sw = 1.f + (wdt - 1.f) * w;
        m *= 1.f + (mg - 1.f) * w; s *= sw * (1.f + (sg - 1.f) * w);
        if (v(FX_WIDTH, 3) > .5f && on[FX_WIDTH]) s = 0;
        L = m + s; R = m - s;
    }
};

}  // namespace lumi

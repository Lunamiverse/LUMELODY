// LUMI MELODY — core sound engine (framework-free, header-only).
// Used by the offline preview renderer today and by the JUCE plugin processor later.
// No allocation happens inside process(): all buffers are sized in prepare()/loadPreset().
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "LumiFx.h"

namespace lumi {

static constexpr float kPi = 3.14159265358979f;
static constexpr float kTwoPi = 6.28318530717958f;

inline float mtof(float n) { return 440.f * std::pow(2.f, (n - 69.f) / 12.f); }
inline float clampf(float x, float a, float b) { return std::min(std::max(x, a), b); }

// ---------------------------------------------------------------- params
struct ParamMap {
    std::map<std::string, float> v;
    float get(const std::string& k, float d = 0.f) const {
        auto it = v.find(k);
        return it == v.end() ? d : it->second;
    }
};

// ---------------------------------------------------------------- utils
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed = 22222) : s(seed ? seed : 1) {}
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (next() >> 8) * (1.f / 16777216.f); }
    float bi() { return uni() * 2.f - 1.f; }
};

struct Env {
    float sr = 44100.f, s = 1.f;
    float ak = 0.f, dc = 0.f, rc = 0.f;
    int stage = 0;
    float lvl = 0.f;
    void setup(float A, float D, float S, float R, float SR) {
        sr = SR;
        A = std::max(A, 0.0007f); D = std::max(D, 0.002f); R = std::max(R, 0.003f);
        s = clampf(S, 0.f, 1.f);
        ak = 1.609f / (A * sr);              // reaches 1.0 at time A on a 1.25 target curve
        dc = std::exp(-4.6f / (D * sr));
        rc = std::exp(-4.6f / (R * sr));
    }
    void on(bool fromZero) { if (fromZero) lvl = 0.f; stage = 1; }
    void off() { if (stage != 0) stage = 3; }
    void kill() { stage = 0; lvl = 0.f; }
    bool active() const { return stage != 0; }
    float tick() {
        switch (stage) {
            case 1:
                lvl += (1.25f - lvl) * ak;
                if (lvl >= 1.f) { lvl = 1.f; stage = 2; }
                break;
            case 2:
                lvl = s + (lvl - s) * dc;
                if (s <= 0.f && lvl < 1e-5f) { lvl = 0.f; stage = 0; }
                break;
            case 3:
                lvl *= rc;
                if (lvl < 1e-5f) { lvl = 0.f; stage = 0; }
                break;
            default: break;
        }
        return lvl;
    }
};

// Topology-preserving state variable filter (Zavalishin)
struct SVF {
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1;
    void set(float fc, float q, float sr) {
        fc = clampf(fc, 12.f, sr * 0.46f);
        float g = std::tan(kPi * fc / sr);
        k = 1.f / std::max(q, 0.3f);
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0; }
    inline void proc(float v0, float& lp, float& bp, float& hp) {
        float v3 = v0 - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
    inline float lp(float x) { float l, b, h; proc(x, l, b, h); return l; }
    inline float bp(float x) { float l, b, h; proc(x, l, b, h); return b; }
    inline float hp(float x) { float l, b, h; proc(x, l, b, h); return h; }
};

struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float proc(float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void set(int type, float f, float gainDb, float q, float sr) {   // 0 peak, 1 lowshelf, 2 highshelf
        float A = std::pow(10.f, gainDb / 40.f), w = kTwoPi * clampf(f, 15.f, sr * 0.45f) / sr;
        float cs = std::cos(w), sn = std::sin(w), al = sn / (2.f * q);
        float B0, B1, B2, A0, A1, A2;
        if (type == 0) { B0 = 1 + al * A; B1 = -2 * cs; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cs; A2 = 1 - al / A; }
        else {
            float sq = 2.f * std::sqrt(A) * al;
            if (type == 1) {
                B0 = A * ((A + 1) - (A - 1) * cs + sq); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - sq);
                A0 = (A + 1) + (A - 1) * cs + sq; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - sq;
            } else {
                B0 = A * ((A + 1) + (A - 1) * cs + sq); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - sq);
                A0 = (A + 1) - (A - 1) * cs + sq; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - sq;
            }
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

inline float polyblep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

// ---------------------------------------------------------------- layer params
enum LayerType { L_OFF = 0, L_VA = 1, L_FM = 2, L_PLUCK = 3, L_MODAL = 4, L_NOISE = 5, L_VOX = 6, L_GRAIN = 7 };

struct LayerP {
    int type = 0;
    float level = 1, pan = 0, oct = 0, semi = 0, fine = 0;
    float a = .003f, d = .5f, s = 1, r = .3f, vel = .7f, filt = 1, drift = 0, keydecay = 0;
    // VA
    float wave = 2, pw = .5f, uni = 1, det = 0, spread = 0;
    // FM
    float cr = 1, mr = 1, idx = 1, idxDec = .3f, idxSus = .2f, fb = 0, mr2 = 0, idx2 = 0, idx2Dec = .05f, idxVel = .6f, fmSpread = 0;
    // pluck
    float bright = .6f, damp = .3f, decay = 2, pick = .2f, strings = 1, sdet = 0;
    // modal
    float mat = 0, hard = .6f, click = .3f, stretch = 1, beat = 0;
    // noise
    float ncut = 2000, nq = .7f, ntrack = 0, nmode = 0;
    // vox
    float vowel = 0, vmod = 0, formShift = 1, tilt = 1, breath = 0;
    // grain
    float grate = 20, glen = .12f, gjit = 8, goct = .3f, gfifth = .15f, gfm = 0;

    float ratio = 1, panL = 1, panR = 1;   // derived at load (not per sample)
    void load(const ParamMap& p, const std::string& px) {
        auto g = [&](const char* k, float d) { return p.get(px + k, d); };
        type = (int)g("type", 0); level = g("level", 1); pan = g("pan", 0); oct = g("oct", 0); semi = g("semi", 0); fine = g("fine", 0);
        a = g("a", .003f); d = g("d", .5f); s = g("s", 1); r = g("r", .3f); vel = g("vel", .7f); filt = g("filt", 1); drift = g("drift", 0); keydecay = g("keydecay", 0);
        wave = g("wave", 2); pw = g("pw", .5f); uni = g("uni", 1); det = g("det", 0); spread = g("spread", 0);
        cr = g("cr", 1); mr = g("mr", 1); idx = g("idx", 1); idxDec = g("idx_dec", .3f); idxSus = g("idx_sus", .2f); fb = g("fb", 0);
        mr2 = g("mr2", 0); idx2 = g("idx2", 0); idx2Dec = g("idx2_dec", .05f); idxVel = g("idx_vel", .6f); fmSpread = g("fm_spread", 0);
        bright = g("bright", .6f); damp = g("damp", .3f); decay = g("decay", 2); pick = g("pick", .2f); strings = g("strings", 1); sdet = g("sdet", 0);
        mat = g("mat", 0); hard = g("hard", .6f); click = g("click", .3f); stretch = g("stretch", 1); beat = g("beat", 0);
        ncut = g("ncut", 2000); nq = g("nq", .7f); ntrack = g("ntrack", 0); nmode = g("nmode", 0);
        vowel = g("vowel", 0); vmod = g("vmod", 0); formShift = g("formshift", 1); tilt = g("tilt", 1); breath = g("breath", 0);
        grate = g("grate", 20); glen = g("glen", .12f); gjit = g("gjit", 8); goct = g("goct", .3f); gfifth = g("gfifth", .15f); gfm = g("gfm", 0);
        ratio = std::pow(2.f, oct + semi / 12.f + fine / 1200.f);
        float a = (clampf(pan, -1.f, 1.f) + 1.f) * kPi * 0.25f;
        panL = std::cos(a) * 1.41421f; panR = std::sin(a) * 1.41421f;
    }
};

// ---------------------------------------------------------------- generators
struct NoteCtx { float sr; float vel; float startFreq; float t; float lfo; Rng* rng; };

struct Gen {
    virtual ~Gen() = default;
    virtual void prepare(float sr) { (void)sr; }
    virtual void start(const LayerP& lp, const NoteCtx& c) = 0;
    bool retrig = false;   // set by the voice when a sounding voice is re-attacked (keep phase continuity)
    virtual void tick(const LayerP& lp, float freq, const NoteCtx& c, float& L, float& R) = 0;
};

inline void panGains(float pan, float& l, float& r) {
    float a = (clampf(pan, -1.f, 1.f) + 1.f) * kPi * 0.25f;
    l = std::cos(a) * 1.41421f; r = std::sin(a) * 1.41421f;
}

struct VAGen : Gen {
    static constexpr int kMax = 8;
    float ph[kMax]{}, ratio[kMax]{}, gl[kMax]{}, gr[kMax]{}, tri[kMax]{};
    int n = 1; float norm = 1;
    void start(const LayerP& p, const NoteCtx& c) override {
        n = std::max(1, std::min(kMax, (int)p.uni));
        norm = 1.f / std::sqrt((float)n);
        for (int i = 0; i < n; ++i) {
            float off = n == 1 ? 0.f : (i / (n - 1.f)) * 2.f - 1.f;
            ratio[i] = std::pow(2.f, off * p.det / 1200.f);
            panGains(off * p.spread, gl[i], gr[i]);
            if (!retrig) ph[i] = n > 1 ? c.rng->uni() : 0.f;
            tri[i] = 0.f;
        }
    }
    inline float wave(int w, float t, float dt, float pw) {
        switch (w) {
            case 0: return std::sin(kTwoPi * t);
            case 1: { float x = 4.f * std::fabs(t - 0.5f) - 1.f; return x; }
            case 2: return 2.f * t - 1.f - polyblep(t, dt);
            default: {
                float sq = t < pw ? 1.f : -1.f;
                float t2 = t - pw; if (t2 < 0) t2 += 1.f;
                return sq + polyblep(t, dt) - polyblep(t2, dt);
            }
        }
    }
    void tick(const LayerP& p, float freq, const NoteCtx& c, float& L, float& R) override {
        float wf = clampf(p.wave, 0.f, 3.f); int w0 = (int)wf; float fr = wf - w0; int w1 = std::min(3, w0 + 1);
        L = R = 0;
        for (int i = 0; i < n; ++i) {
            float dt = freq * ratio[i] / c.sr;
            float t = ph[i];
            float v = wave(w0, t, dt, p.pw);
            if (fr > 0.001f) v = v * (1 - fr) + wave(w1, t, dt, p.pw) * fr;
            ph[i] += dt; if (ph[i] >= 1.f) ph[i] -= 1.f;
            L += v * gl[i]; R += v * gr[i];
        }
        L *= norm; R *= norm;
    }
};

struct FMGen : Gen {
    float pc = 0, pc2 = 0, pm = 0, pm2 = 0, e1 = 1, e2 = 1, k1 = 1, k2 = 1, prev = 0, idxScale = 1, spreadRatio = 1;
    void start(const LayerP& p, const NoteCtx& c) override {
        if (!retrig) { pc = pc2 = pm = pm2 = 0; prev = 0; }
        e1 = e2 = 1;
        float kd = std::pow(261.6f / std::max(c.startFreq, 20.f), 0.25f);
        k1 = std::exp(-1.f / (std::max(p.idxDec * kd, 0.001f) * c.sr));
        k2 = std::exp(-1.f / (std::max(p.idx2Dec * kd, 0.0005f) * c.sr));
        idxScale = 1.f - p.idxVel + p.idxVel * c.vel;
        // tame index for high notes so the top of the keyboard is not harsh
        idxScale *= clampf(std::pow(440.f / std::max(c.startFreq, 20.f), 0.35f), 0.25f, 1.5f);
        spreadRatio = std::pow(2.f, p.fmSpread / 1200.f);
    }
    void tick(const LayerP& p, float freq, const NoteCtx& c, float& L, float& R) override {
        float I1 = p.idx * idxScale * (p.idxSus + (1.f - p.idxSus) * e1);
        float I2 = p.idx2 * idxScale * e2;
        e1 *= k1; e2 *= k2;
        float m2 = p.mr2 > 0 ? std::sin(kTwoPi * pm2) * I2 : 0.f;
        float s1 = std::sin(kTwoPi * pm + m2 + p.fb * prev);
        prev = s1;
        float m1 = s1 * I1;
        float a = std::sin(kTwoPi * pc + m1);
        if (p.fmSpread > 0) {
            float b = std::sin(kTwoPi * pc2 + m1);
            L = a * 1.2f; R = b * 1.2f;
            pc2 += freq * p.cr * spreadRatio / c.sr; if (pc2 >= 1) pc2 -= 1;
        } else { L = R = a; }
        pc += freq * p.cr / c.sr; if (pc >= 1) pc -= 1;
        pm += freq * p.mr / c.sr; if (pm >= 1) pm -= 1;
        if (p.mr2 > 0) { pm2 += freq * p.mr2 / c.sr; if (pm2 >= 1) pm2 -= 1; }
    }
};

// Karplus-Strong string with fractional all-pass tuning and pick-position excitation
struct KString {
    static constexpr int kSize = 4096;
    std::vector<float> buf, exc;
    int N = 100, w = 0, excLen = 0, excPos = 0;
    float c = 0, apx = 0, apy = 0, prevRead = 0, s = .3f, g = .99f, dcx = 0, dcy = 0;
    void prepare() { buf.assign(kSize, 0.f); exc.assign(kSize, 0.f); }
    void start(float freq, float sr, float bright, float damp, float decay, float pick, float vel, Rng& rng) {
        std::fill(buf.begin(), buf.end(), 0.f);
        float P = clampf(sr / freq, 4.f, kSize - 8.f);
        s = clampf(damp, 0.02f, 0.5f);
        float tot = P - s;
        N = (int)std::floor(tot - 0.5f);
        float d = tot - N;                          // 0.5..1.5
        c = (1.f - d) / (1.f + d);
        g = std::pow(0.001f, 1.f / (freq * std::max(decay, 0.05f)));
        apx = apy = prevRead = dcx = dcy = 0; w = 0;
        excLen = (int)P; excPos = 0;
        float b = clampf(bright * (0.45f + 0.55f * vel), 0.02f, 1.f);
        float coef = std::exp(-kTwoPi * (200.f + 9000.f * b * b) / sr);
        float lp = 0;
        int pk = std::max(1, (int)(pick * P));
        std::vector<float>& e = exc;
        for (int i = 0; i < excLen; ++i) { lp = (1 - coef) * rng.bi() + coef * lp; e[i] = lp; }
        for (int i = excLen - 1; i >= pk; --i) e[i] -= e[i - pk];
        float mx = 1e-6f; for (int i = 0; i < excLen; ++i) mx = std::max(mx, std::fabs(e[i]));
        for (int i = 0; i < excLen; ++i) e[i] /= mx;
    }
    inline float tick() {
        float x = excPos < excLen ? exc[excPos++] : 0.f;
        int ri = w - N; if (ri < 0) ri += kSize;
        float rd = buf[ri];
        float v = (1.f - s) * rd + s * prevRead; prevRead = rd;
        float a = c * v + apx - c * apy; apx = v; apy = a;
        float y = x + g * a;
        buf[w] = y; if (++w >= kSize) w = 0;
        float o = y - dcx + 0.995f * dcy; dcx = y; dcy = o;
        return o;
    }
};

struct PluckGen : Gen {
    KString s1, s2;
    int ns = 1;
    void prepare(float) override { s1.prepare(); s2.prepare(); }
    void start(const LayerP& p, const NoteCtx& c) override {
        ns = p.strings > 1.5f ? 2 : 1;
        float kd = std::pow(261.6f / c.startFreq, p.keydecay);
        s1.start(c.startFreq, c.sr, p.bright, p.damp, p.decay * kd, p.pick, c.vel, *c.rng);
        if (ns == 2) s2.start(c.startFreq * std::pow(2.f, p.sdet / 1200.f), c.sr, p.bright, p.damp, p.decay * kd, p.pick * 1.13f, c.vel, *c.rng);
    }
    void tick(const LayerP&, float, const NoteCtx&, float& L, float& R) override {
        float a = s1.tick();
        if (ns == 2) { float b = s2.tick(); L = (a * 0.8f + b * 0.2f) * 1.1f; R = (b * 0.8f + a * 0.2f) * 1.1f; }
        else L = R = a * 1.1f;
    }
};

struct Material { int n; float ratio[8]; float amp[8]; float dmul[8]; };
inline const Material& material(int i) {
    static const Material m[] = {
        {3, {1, 3.93f, 9.2f}, {1, .28f, .09f}, {1, .35f, .14f}},                                        // 0 marimba
        {3, {1, 4.0f, 10.0f}, {1, .22f, .07f}, {1, .45f, .2f}},                                        // 1 vibraphone
        {5, {1, 2.32f, 4.25f, 6.63f, 9.38f}, {1, .45f, .3f, .18f, .1f}, {1, .7f, .5f, .35f, .25f}},     // 2 glass
        {8, {.5f, 1, 1.19f, 1.5f, 2, 2.52f, 3, 4.07f}, {.45f, 1, .55f, .3f, .45f, .2f, .14f, .1f}, {1.6f, 1, .8f, .7f, .6f, .5f, .4f, .3f}}, // 3 bell
        {3, {1, 5.4f, 12.9f}, {1, .22f, .07f}, {1, .22f, .1f}},                                         // 4 kalimba
        {4, {1, 3.0f, 5.0f, 8.1f}, {1, .22f, .1f, .05f}, {1, .4f, .25f, .15f}},                         // 5 celesta / music box
        {5, {1, 2, 3, 4.02f, 5.1f}, {1, .55f, .32f, .18f, .1f}, {1, .6f, .45f, .3f, .2f}},              // 6 steel pan
        {4, {1, 2.76f, 5.4f, 8.9f}, {1, .4f, .2f, .1f}, {1, .3f, .15f, .1f}},                           // 7 log / wood
    };
    return m[std::max(0, std::min(7, i))];
}

struct ModalGen : Gen {
    int n = 0; float ph[16]{}, inc[16]{}, amp[16]{}, k[16]{}, gl[16]{}, gr[16]{};
    float clickEnv = 0, clickK = 0, att = 0, attInc = 1; SVF clickF;
    void start(const LayerP& p, const NoteCtx& c) override {
        const Material& m = material((int)p.mat);
        float bright = clampf(p.hard * (0.35f + 0.65f * c.vel), 0.f, 1.f);
        n = 0;
        for (int i = 0; i < m.n; ++i) {
            float ratio = 1.f + (m.ratio[i] - 1.f) * p.stretch;
            for (int side = 0; side < 2; ++side) {
                float f = c.startFreq * ratio * std::pow(2.f, (side ? 1.f : -1.f) * p.beat * (i + 1) / 1200.f);
                if (f > c.sr * 0.45f) continue;
                float a = m.amp[i] * std::pow(0.2f + 0.8f * bright, (float)i);
                if (f > 6000.f) a *= 6000.f / f;
                float T = std::max(0.02f, p.decay * m.dmul[i] * std::pow(261.6f / c.startFreq, 0.35f + p.keydecay));
                ph[n] = c.rng->uni() * 0.1f; inc[n] = f / c.sr; amp[n] = a * 0.5f; k[n] = std::exp(-6.9f / (T * c.sr));
                gl[n] = side ? 0.35f : 1.f; gr[n] = side ? 1.f : 0.35f;
                if (p.beat <= 0) { gl[n] = gr[n] = 0.75f; }
                ++n;
            }
        }
        clickEnv = p.click * (0.3f + 0.7f * c.vel); clickK = std::exp(-1.f / (0.004f * c.sr));
        clickF.reset(); clickF.set(std::min(c.startFreq * 5.f, 9000.f), 1.2f, c.sr);
        att = 0; attInc = 1.f / (0.0015f * c.sr);
    }
    void tick(const LayerP&, float, const NoteCtx& c, float& L, float& R) override {
        L = R = 0;
        float aw = att < 1.f ? 0.5f - 0.5f * std::cos(kPi * att) : 1.f; att += attInc;
        for (int i = 0; i < n; ++i) {
            float v = std::sin(kTwoPi * ph[i]) * amp[i];
            ph[i] += inc[i]; if (ph[i] >= 1.f) ph[i] -= 1.f;
            amp[i] *= k[i];
            L += v * gl[i]; R += v * gr[i];
        }
        L *= aw; R *= aw;
        if (clickEnv > 1e-4f) { float cl = clickF.bp(c.rng->bi()) * clickEnv; clickEnv *= clickK; L += cl; R += cl; }
    }
};

struct NoiseGen : Gen {
    SVF fl, fr; int cnt = 0;
    void start(const LayerP&, const NoteCtx&) override { fl.reset(); fr.reset(); cnt = 0; }
    void tick(const LayerP& p, float freq, const NoteCtx& c, float& L, float& R) override {
        if ((cnt++ & 15) == 0) {
            float fc = p.ntrack > 0.5f ? freq * p.ncut : p.ncut;
            fc *= std::pow(2.f, c.lfo * 0.5f * p.vmod);
            fl.set(fc, p.nq, c.sr); fr.set(fc * 1.02f, p.nq, c.sr);
        }
        float a = c.rng->bi(), b = c.rng->bi();
        if (p.nmode < 0.5f) { L = fl.lp(a); R = fr.lp(b); }
        else if (p.nmode < 1.5f) { L = fl.bp(a) * 1.8f; R = fr.bp(b) * 1.8f; }
        else { L = fl.hp(a); R = fr.hp(b); }
    }
};

// Additive source shaped by a moving vowel formant envelope (vocal-like synth)
struct VoxGen : Gen {
    static constexpr int kH = 48;
    float phase = 0, amps[kH]{}; int nh = 0, cnt = 0; SVF br;
    void start(const LayerP&, const NoteCtx& c) override { phase = c.rng->uni(); cnt = 0; br.reset(); }
    static void vowelF(float v, float& f1, float& f2, float& f3) {
        static const float F[5][3] = {{800, 1150, 2800}, {400, 2000, 2700}, {300, 2250, 3000}, {480, 820, 2700}, {330, 720, 2500}};
        v = clampf(v, 0.f, 3.999f); int i = (int)v; float fr = v - i;
        f1 = F[i][0] + (F[i + 1][0] - F[i][0]) * fr; f2 = F[i][1] + (F[i + 1][1] - F[i][1]) * fr; f3 = F[i][2] + (F[i + 1][2] - F[i][2]) * fr;
    }
    void tick(const LayerP& p, float freq, const NoteCtx& c, float& L, float& R) override {
        if ((cnt++ & 63) == 0) {
            float f1, f2, f3; vowelF(p.vowel + p.vmod * c.lfo, f1, f2, f3);
            f1 *= p.formShift; f2 *= p.formShift; f3 *= p.formShift;
            nh = 0; float sum = 0;
            for (int k = 1; k <= kH; ++k) {
                float fh = freq * k; if (fh > std::min(11000.f, c.sr * 0.45f)) break;
                auto pk = [&](float F, float bw, float g) { float x = (fh - F) / bw; return g / (1.f + x * x); };
                float a = std::pow((float)k, -p.tilt * 0.5f) * (0.03f + pk(f1, 90 + f1 * 0.08f, 1.f) + pk(f2, 110 + f2 * 0.05f, 0.55f) + pk(f3, 160, 0.25f));
                amps[nh++] = a; sum += a;
            }
            float norm = sum > 0 ? 1.4f / std::sqrt(sum * sum * 0.25f + 1e-6f) : 0;
            for (int k = 0; k < nh; ++k) amps[k] *= norm * 0.5f;
            br.set(f2, 2.f, c.sr);
        }
        // sin(k*th) by Chebyshev recurrence: one sin/cos per sample instead of one sin per harmonic
        float th = kTwoPi * phase, s1 = std::sin(th), c2 = 2.f * std::cos(th), sPrev = 0.f, sCur = s1, v = 0;
        for (int k = 0; k < nh; ++k) { v += amps[k] * sCur; float sNext = c2 * sCur - sPrev; sPrev = sCur; sCur = sNext; }
        phase += freq / c.sr; if (phase >= 1) phase -= 1;
        if (p.breath > 0) v += br.bp(c.rng->bi()) * p.breath;
        L = R = v;
    }
};

// Stochastic grain cloud around the note pitch (shimmer / granular textures)
struct GrainGen : Gen {
    static constexpr int kG = 24;
    struct G { bool on; float ph, inc, pos, len, gl, gr, fmPh; };
    G g[kG]{}; float timer = 0;
    void start(const LayerP&, const NoteCtx&) override { for (auto& x : g) x.on = false; timer = 0; }
    void tick(const LayerP& p, float freq, const NoteCtx& c, float& L, float& R) override {
        timer -= 1.f / c.sr;
        if (timer <= 0) {
            timer = (0.6f + 0.8f * c.rng->uni()) / std::max(p.grate, 0.5f);
            for (auto& x : g) if (!x.on) {
                float r = c.rng->uni(); float mult = 1.f;
                if (r < p.goct) mult = c.rng->uni() < 0.7f ? 2.f : 4.f;
                else if (r < p.goct + p.gfifth) mult = 1.5f;
                mult *= std::pow(2.f, c.rng->bi() * p.gjit / 1200.f);
                x.on = true; x.ph = c.rng->uni(); x.fmPh = 0; x.inc = freq * mult / c.sr; x.pos = 0;
                x.len = std::max(0.01f, p.glen * (0.6f + 0.8f * c.rng->uni())) * c.sr;
                panGains(c.rng->bi() * 0.9f, x.gl, x.gr);
                break;
            }
        }
        L = R = 0;
        for (auto& x : g) if (x.on) {
            float w = std::sin(kPi * x.pos / x.len); w *= w;
            float v = std::sin(kTwoPi * x.ph + p.gfm * std::sin(kTwoPi * x.fmPh)) * w;
            x.ph += x.inc; if (x.ph >= 1) x.ph -= 1; x.fmPh += x.inc * 2.f; if (x.fmPh >= 1) x.fmPh -= 1;
            L += v * x.gl; R += v * x.gr;
            if (++x.pos >= x.len) x.on = false;
        }
        float n = 0.35f / std::sqrt(std::max(1.f, p.grate * p.glen));
        L *= n; R *= n;
    }
};

inline std::unique_ptr<Gen> makeGen(int t) {
    switch (t) {
        case L_VA: return std::make_unique<VAGen>();
        case L_FM: return std::make_unique<FMGen>();
        case L_PLUCK: return std::make_unique<PluckGen>();
        case L_MODAL: return std::make_unique<ModalGen>();
        case L_NOISE: return std::make_unique<NoiseGen>();
        case L_VOX: return std::make_unique<VoxGen>();
        case L_GRAIN: return std::make_unique<GrainGen>();
        default: return nullptr;
    }
}

// ---------------------------------------------------------------- voice-level settings
struct VoiceP {
    static constexpr int kLayers = 4;
    LayerP layer[kLayers];
    int fType = 0; float fCut = 8000, fRes = .1f, fPoles = 2, fEnv = 0, fKey = .5f, fVel = 1, fDrive = 1;
    float fa = .005f, fd = .4f, fs = 0, fr = .4f;
    float pEnv = 0, pDec = .05f, glide = 0, bend = 2;
    int mono = 0;
    float vibRate = 5, vibDepth = 0, vibDelay = .3f;
    float lfoRate = 1, lfoSync = 0, lfoCut = 0, lfoAmp = 0, lfoPan = 0; int lfoShape = 0;
    float ampVelCurve = 1, voiceGain = 1;
    void load(const ParamMap& p) {
        for (int i = 0; i < kLayers; ++i) layer[i].load(p, "l" + std::to_string(i + 1) + "_");
        fType = (int)p.get("f_type", 0); fCut = p.get("f_cut", 8000); fRes = p.get("f_res", .1f); fPoles = p.get("f_poles", 2);
        fEnv = p.get("f_env", 0); fKey = p.get("f_key", .5f); fVel = p.get("f_vel", 1); fDrive = p.get("f_drive", 1);
        fa = p.get("fa", .005f); fd = p.get("fd", .4f); fs = p.get("fs", 0); fr = p.get("fr", .4f);
        pEnv = p.get("p_env", 0); pDec = p.get("p_dec", .05f); glide = p.get("glide", 0); bend = p.get("bend", 2);
        mono = (int)p.get("mono", 0);
        vibRate = p.get("vib_rate", 5); vibDepth = p.get("vib_depth", 0); vibDelay = p.get("vib_delay", .3f);
        lfoRate = p.get("lfo_rate", 1); lfoSync = p.get("lfo_sync", 0); lfoCut = p.get("lfo_cut", 0);
        lfoAmp = p.get("lfo_amp", 0); lfoPan = p.get("lfo_pan", 0); lfoShape = (int)p.get("lfo_shape", 0);
        ampVelCurve = p.get("vel_curve", 1); voiceGain = p.get("voice_gain", 1);
    }
};

struct Voice {
    std::unique_ptr<Gen> gen[VoiceP::kLayers];
    Env amp[VoiceP::kLayers], fenv;
    SVF filt[2][2];
    int note = -1; bool held = false; uint64_t order = 0;
    float vel = 0, freq = 440, target = 440, t = 0, pEnvLvl = 0, pEnvK = 0, glideK = 0;
    float vibPh = 0, lfoPh = 0, sh = 0, drift[VoiceP::kLayers]{}, driftT[VoiceP::kLayers]{};
    int ctr = 0; float cutCache = 1000;
    float lastL = 0, lastR = 0, bleedL = 0, bleedR = 0, velSm = 0, pitchAtStart = 0;
    bool layer = false; int srcNote = -1;   // octave-layer voices remember the key that started them   // de-click: old output bleeds out after a hard restart
    Rng rng;
    bool active() const { for (auto& e : amp) if (e.active()) return true; return false; }
    bool sounding() const { return active() || bleedL != 0.f || bleedR != 0.f; }
};

// ---------------------------------------------------------------- effects
struct DelayLine {
    std::vector<float> b; int w = 0;
    void prepare(int n) { b.assign(n, 0.f); w = 0; }
    inline void push(float x) { b[w] = x; if (++w >= (int)b.size()) w = 0; }
    inline float readFrac(float d) const {   // d samples behind the most recent write
        float rp = (float)w - 1.f - d; while (rp < 0) rp += (float)b.size();
        int i0 = (int)rp; float fr = rp - i0; int i1 = i0 + 1; if (i1 >= (int)b.size()) i1 = 0;
        return b[i0] + (b[i1] - b[i0]) * fr;
    }
    inline float read(int d) const { int i = w - 1 - d; while (i < 0) i += (int)b.size(); return b[i]; }
};

struct Reverb {
    // 8-line feedback delay network. Every parameter change glides (line lengths, pre-delay, damping),
    // so the space can be re-shaped while it rings without clicks.
    static constexpr int N = 8;
    DelayLine line[N], pre, ap[4];
    float len[N]{}, lenT[N]{}, g[N]{}, lp[N]{}, damp = .3f, dampT = .3f, modPh = 0, sr = 44100, preCur = 0, preT = 0;
    int apLen[4]{}; bool primed = false;
    void prepare(float SR) {
        sr = SR;
        for (auto& l : line) l.prepare((int)(0.25f * sr));
        pre.prepare((int)(0.3f * sr));
        for (auto& a : ap) a.prepare(2048);
        static const float apb[4] = {4.7f, 3.6f, 12.7f, 9.3f};
        for (int i = 0; i < 4; ++i) apLen[i] = (int)(apb[i] * 0.001f * sr);
    }
    void set(float size, float decay, float dmp, float preMs) {
        static const float base[N] = {31.7f, 37.3f, 41.9f, 47.3f, 53.9f, 59.3f, 67.1f, 73.7f};
        float sc = 0.45f + clampf(size, 0.f, 1.f) * 1.35f;
        for (int i = 0; i < N; ++i) {
            lenT[i] = base[i] * sc * 0.001f * sr;
            g[i] = std::pow(0.001f, lenT[i] / (std::max(decay, 0.1f) * sr));
            if (!primed) len[i] = lenT[i];
        }
        dampT = clampf(dmp, 0.f, 0.95f); preT = clampf(preMs * 0.001f * sr, 0.f, 0.29f * sr);
        if (!primed) { damp = dampT; preCur = preT; primed = true; }
    }
    inline void tick(float inL, float inR, float& oL, float& oR) {
        damp += (dampT - damp) * .001f; preCur += (preT - preCur) * .0005f;
        pre.push((inL + inR) * 0.5f);
        float x = preCur > 1.f ? pre.readFrac(preCur - 1.f) : pre.read(0);
        for (int i = 0; i < 4; ++i) {
            float d = ap[i].read(apLen[i]);
            float v = x + 0.6f * d; ap[i].push(v); x = d - 0.6f * v;
        }
        modPh += 0.7f / sr; if (modPh >= 1) modPh -= 1;
        float m = std::sin(kTwoPi * modPh);
        float v[N], sum = 0;
        for (int i = 0; i < N; ++i) {
            len[i] += (lenT[i] - len[i]) * .0002f;
            float dl = len[i] + (i == 1 ? m * 6.f : i == 6 ? -m * 5.f : 0.f);
            float d = line[i].readFrac(dl);
            lp[i] = (1 - damp) * d + damp * lp[i];
            v[i] = lp[i] * g[i]; sum += v[i];
        }
        sum *= 2.f / N;
        for (int i = 0; i < N; ++i) line[i].push(v[i] - sum + x * ((i & 1) ? -0.5f : 0.5f));
        oL = (v[0] + v[2] - v[4] + v[6]) * 0.6f;
        oR = (v[1] - v[3] + v[5] + v[7]) * 0.6f;
    }
};

// Delay-line pitch shifter with three overlapping read heads (sin^2 windows at thirds sum to a constant).
// Three heads avoid the deep notches a two-head design gets when both heads land out of phase.
// Used inside the shimmer reverb feedback path, where small artefacts are hidden by the tail.
struct PitchShifter {
    std::vector<float> buf; int w = 0; float ph = 0, win = 1800, ratio = 2;
    void prepare(float sr) { buf.assign(8192, 0.f); win = 0.041f * sr; }
    inline float tick(float x) {
        buf[w] = x;
        auto rd = [&](float d) {
            float rp = (float)w - d; while (rp < 0) rp += (float)buf.size();
            int i0 = (int)rp; float fr = rp - i0; int i1 = (i0 + 1) % (int)buf.size();
            return buf[i0] + (buf[i1] - buf[i0]) * fr;
        };
        float y = 0;
        for (int h = 0; h < 3; ++h) {
            float p = ph + h / 3.f; if (p >= 1) p -= 1;
            float g = std::sin(kPi * p);
            y += rd(1.f + p * win) * g * g;
        }
        ph -= (ratio - 1.f) / win; while (ph < 0) ph += 1.f; while (ph >= 1) ph -= 1.f;
        if (++w >= (int)buf.size()) w = 0;
        return y * (2.f / 3.f);
    }
};

// Preset sound-design chain. Its delay and reverb are driven by the DELAY and REVERB effects of the rack.
struct FX {
    float sr = 44100, bpm = 120;
    float satDrive = 0, satMix = 1, crushSr = 0, crushBits = 0;
    float lc = 20, hc = 20000, lsF = 120, lsG = 0, pkF = 1000, pkG = 0, pkQ = .8f, hsF = 6000, hsG = 0;
    float chRate = .4f, chDepth = 0, chMix = 0;
    float dlyDamp = .4f, width = 1, pump = 0, stutBeats = .25f, stutProb = 0, gain = 1;
    float rvSize = .6f, rvDecay = 2.5f, rvDamp = .4f, rvPre = 15;   // preset reverb character ("Preset" type)
    float presetRvMix = 0, presetDlyMix = 0, presetDlyFb = .3f, presetDlyBeats = .75f;
    SVF lcL, lcR, hcL, hcR; Biquad ls[2], pk[2], hs[2];
    DelayLine chL, chR, dL, dR; float chPh = 0;
    Reverb rv; PitchShifter shim; float shimFb = 0;
    float crushHold[2]{}, crushCnt = 0;
    double samplePos = 0;
    std::vector<float> stBufL, stBufR; int stPos = 0, stLen = 0, stSlice = 0; bool stOn = false; Rng rng{777};
    // live (smoothed) space parameters
    float rvMix = 0, dlyMix = 0, dlyFb = .3f, dlyTimeCur = -1;
    int rvTypeSet = -2; float rvDecaySet = -1, rvPreSet = -1, rvToneSet = -1;
    OnePole dTone[2][2];
    void prepare(float SR) {
        sr = SR;
        chL.prepare((int)(0.05f * sr)); chR.prepare((int)(0.05f * sr));
        dL.prepare((int)(2.6f * sr)); dR.prepare((int)(2.6f * sr));
        rv.prepare(sr); shim.prepare(sr);
        stBufL.assign((int)(2.f * sr), 0.f); stBufR.assign((int)(2.f * sr), 0.f);
    }
    void load(const ParamMap& p, float tempo) {
        bpm = tempo;
        satDrive = p.get("sat_drive", 0); satMix = p.get("sat_mix", 1);
        crushSr = p.get("crush_sr", 0); crushBits = p.get("crush_bits", 0);
        lc = p.get("eq_lc", 20); hc = p.get("eq_hc", 20000);
        lsF = p.get("eq_ls_f", 120); lsG = p.get("eq_ls_g", 0); pkF = p.get("eq_pk_f", 1000); pkG = p.get("eq_pk_g", 0); pkQ = p.get("eq_pk_q", .8f);
        hsF = p.get("eq_hs_f", 6000); hsG = p.get("eq_hs_g", 0);
        chRate = p.get("ch_rate", .4f); chDepth = p.get("ch_depth", 0); chMix = p.get("ch_mix", 0);
        presetDlyBeats = p.get("dly_beats", .75f); presetDlyFb = p.get("dly_fb", .3f); presetDlyMix = p.get("dly_mix", 0); dlyDamp = p.get("dly_damp", .4f);
        presetRvMix = p.get("rv_mix", 0);
        rvSize = p.get("rv_size", .6f); rvDecay = p.get("rv_decay", 2.5f); rvDamp = p.get("rv_damp", .4f); rvPre = p.get("rv_pre", 15);
        width = p.get("width", 1); pump = p.get("pump", 0);
        stutBeats = p.get("stut_beats", .25f); stutProb = p.get("stut_prob", 0);
        gain = std::pow(10.f, (p.get("gain_db", 0) + p.get("out_gain_db", 0)) / 20.f);   // out_gain_db: calibrated preset level
        lcL.set(lc, .707f, sr); lcR.set(lc, .707f, sr); hcL.set(hc, .707f, sr); hcR.set(hc, .707f, sr);
        for (int c = 0; c < 2; ++c) { ls[c].set(1, lsF, lsG, .707f, sr); pk[c].set(0, pkF, pkG, pkQ, sr); hs[c].set(2, hsF, hsG, .707f, sr); }
        rvTypeSet = -2; dlyTimeCur = -1;
    }
    // what the REVERB / DELAY effects start at for this preset (so a preset sounds as designed)
    void presetSpaceDefaults(Rack& r) const {
        r.on[FX_REVERB] = presetRvMix > 0; r.val[FX_REVERB][0] = clampf(presetRvMix / .85f, 0.f, 1.f); r.val[FX_REVERB][1] = 0;
        r.val[FX_REVERB][2] = rvDecay; r.val[FX_REVERB][3] = rvPre; r.val[FX_REVERB][4] = clampf(1.f - rvDamp / .85f, 0.f, 1.f);
        r.on[FX_DELAY] = presetDlyMix > 0; r.val[FX_DELAY][0] = clampf(presetDlyMix / .6f, 0.f, 1.f);
        int best = 0; float bd = 9; for (int i = 0; i < 7; ++i) if (std::fabs(beatsForDiv(i) - presetDlyBeats) < bd) { bd = std::fabs(beatsForDiv(i) - presetDlyBeats); best = i; }
        r.val[FX_DELAY][1] = (float)best; r.val[FX_DELAY][2] = presetDlyFb; r.val[FX_DELAY][3] = -dlyDamp * .8f; r.val[FX_DELAY][4] = .8f; r.val[FX_DELAY][5] = 1;
    }
    // selecting a reverb type loads that space's decay / pre-delay / tone
    static void reverbTypeDefaults(Rack& r, int type, const FX& f) {
        static const float D[5][3] = {{0, 0, 0}, {.9f, 4, .45f}, {4.2f, 28, .55f}, {2.4f, 0, .85f}, {5.5f, 30, .6f}};
        if (type == 0) { r.val[FX_REVERB][2] = f.rvDecay; r.val[FX_REVERB][3] = f.rvPre; r.val[FX_REVERB][4] = clampf(1.f - f.rvDamp / .85f, 0.f, 1.f); }
        else { r.val[FX_REVERB][2] = D[type][0]; r.val[FX_REVERB][3] = D[type][1]; r.val[FX_REVERB][4] = D[type][2]; }
    }
    inline float sat(float x) const {
        if (satDrive <= 0) return x;
        float d = 1.f + satDrive * 8.f;
        float y = std::tanh(x * d + 0.08f * satDrive) - std::tanh(0.08f * satDrive);
        y /= std::tanh(d) * 0.9f + 0.1f;
        return x + (y - x) * satMix;
    }
    void pre(float& L, float& R) {
        L = sat(L); R = sat(R);
        if (crushSr > 0) {
            crushCnt += crushSr / sr;
            if (crushCnt >= 1.f) { crushCnt -= 1.f; crushHold[0] = L; crushHold[1] = R; }
            L = crushHold[0]; R = crushHold[1];
        }
        if (crushBits > 0) { float q = std::pow(2.f, crushBits); L = std::round(L * q) / q; R = std::round(R * q) / q; }
        if (lc > 21) { L = lcL.hp(L); R = lcR.hp(R); }
        if (hc < 19999) { L = hcL.lp(L); R = hcR.lp(R); }
        if (lsG != 0) { L = ls[0].proc(L); R = ls[1].proc(R); }
        if (pkG != 0) { L = pk[0].proc(L); R = pk[1].proc(R); }
        if (hsG != 0) { L = hs[0].proc(L); R = hs[1].proc(R); }
        if (chMix > 0) {
            chL.push(L); chR.push(R);
            chPh += chRate / sr; if (chPh >= 1) chPh -= 1;
            float dm = chDepth * 0.001f * sr;
            float a = chL.readFrac(0.009f * sr + dm * (1 + std::sin(kTwoPi * chPh)));
            float b = chR.readFrac(0.011f * sr + dm * (1 + std::sin(kTwoPi * chPh + 1.7f)));
            L = L * (1 - chMix * 0.5f) + a * chMix * 0.7f; R = R * (1 - chMix * 0.5f) + b * chMix * 0.7f;
        }
        if (stutProb > 0) {   // the preset's own baked glitch (a few presets)
            int step = std::max(16, (int)(stutBeats * 60.f / bpm * sr));
            int posInStep = (int)std::fmod(samplePos, (double)step);
            if (posInStep == 0) { stOn = rng.uni() < stutProb; stPos = 0; stSlice = step / (rng.uni() < 0.5f ? 4 : 2); stLen = 0; }
            if (stOn && step < (int)stBufL.size()) {
                if (stLen < stSlice) { stBufL[stLen] = L; stBufR[stLen] = R; ++stLen; }
                else { int i = stPos % stSlice; float fade = std::min(1.f, std::min(i, stSlice - i) / 64.f); L = stBufL[i] * fade; R = stBufR[i] * fade; }
                ++stPos;
            }
        }
    }
    void space(float& L, float& R, Rack& r) {
        // DELAY: smoothed amount/feedback/time, tone filters in the loop, soft-limited feedback, stereo width
        float dlyT = r.on[FX_DELAY] ? r.v(FX_DELAY, 0) * .6f : 0.f;
        float fbT = std::min(.85f, r.v(FX_DELAY, 2));
        const float k = .0012f;
        dlyMix += (dlyT - dlyMix) * k; dlyFb += (fbT - dlyFb) * k;
        float wetL = 0, wetR = 0;
        if (dlyMix > 1e-5f || dlyT > 0) {
            float target = clampf(beatsForDiv((int)r.v(FX_DELAY, 1)) * 60.f / bpm * sr, 1.f, 2.5f * sr);
            dlyTimeCur = dlyTimeCur < 0 ? target : dlyTimeCur + (target - dlyTimeCur) * .0005f;
            float tone = r.v(FX_DELAY, 3);
            float lpHz = tone < 0 ? 18000.f * std::exp2(tone * 4.2f) : 18000.f, hpHz = tone > 0 ? 30.f * std::exp2(tone * 4.5f) : 30.f;
            for (int c = 0; c < 2; ++c) { dTone[c][0].setHz(lpHz, sr); dTone[c][1].setHz(hpHz, sr); }
            float a = dL.readFrac(dlyTimeCur), b = dR.readFrac(dlyTimeCur);
            a = dTone[0][1].hp(dTone[0][0].lp(a)); b = dTone[1][1].hp(dTone[1][0].lp(b));
            float fbL = std::tanh(a * dlyFb * 1.2f) / 1.2f, fbR = std::tanh(b * dlyFb * 1.2f) / 1.2f;
            float in = (L + R) * .5f;
            if (r.v(FX_DELAY, 5) > .5f) { dL.push(in + fbR); dR.push(fbL); }
            else { dL.push(L + fbL); dR.push(R + fbR); }
            float m = (a + b) * .5f, s = (a - b) * .5f * r.v(FX_DELAY, 4);
            wetL = (m + s) * dlyMix; wetR = (m - s) * dlyMix;
        }
        // REVERB: type sets the space; decay / pre-delay / tone re-tune the tail when they change
        int type = (int)r.v(FX_REVERB, 1); float decay = r.v(FX_REVERB, 2), preMs = r.v(FX_REVERB, 3), tone = r.v(FX_REVERB, 4);
        if (type != rvTypeSet || decay != rvDecaySet || preMs != rvPreSet || tone != rvToneSet) {
            static const float size[5] = {-1, .25f, .92f, .5f, .95f};
            rv.set(type == 0 ? rvSize : size[type], decay, .85f * (1.f - tone), preMs);
            rvTypeSet = type; rvDecaySet = decay; rvPreSet = preMs; rvToneSet = tone;
        }
        // long decays build up more energy: trim the wet level so a long tail does not swamp the mix
        float rvT = r.on[FX_REVERB] ? r.v(FX_REVERB, 0) * .85f * std::min(1.f, std::sqrt(3.f / std::max(decay, .2f))) : 0.f;
        rvMix += (rvT - rvMix) * k;
        if (rvMix > 1e-5f || rvT > 0) {
            float inL = L + wetL * .5f, inR = R + wetR * .5f;
            bool sh = type == 4;
            if (sh) { inL += shimFb; inR += shimFb; }
            float rl, rr; rv.tick(inL, inR, rl, rr);
            shimFb = sh ? .5f * std::tanh(shim.tick((rl + rr) * .5f) * .9f) : 0.f;   // tested to always decay
            wetL += rl * rvMix; wetR += rr * rvMix;
        }
        L += wetL; R += wetR;
    }
    void post(float& L, float& R) {
        if (pump > 0) {
            double beatLen = 60.0 / bpm * sr;
            float ph = (float)(std::fmod(samplePos, beatLen) / beatLen);
            const float att = 0.02f;
            float shape = ph < att ? ph / att : std::pow(std::max(0.f, 1.f - (ph - att) / (0.55f - att)), 2.f);
            float g = 1.f - pump * shape;
            L *= g; R *= g;
        }
        float m = (L + R) * 0.5f, s = (L - R) * 0.5f * width;
        L = (m + s) * gain; R = (m - s) * gain;
        samplePos += 1.0;
    }
};

// ---------------------------------------------------------------- engine
class Engine {
public:
    static constexpr int kVoices = 32;
    Rack rack;

    void prepare(float sampleRate) {
        sr = sampleRate;
        fx.prepare(sr);
        rack.prepare(sr);
        layerLevel.setTime(.02f, sr); balance.setTime(.02f, sr);
    }

    // presets reset the REVERB/DELAY effects to the preset's own space; the other effects keep their settings
    void loadPreset(const ParamMap& p, float tempo) {
        params = p;
        vp.load(p);
        fx.load(p, tempo);
        fx.presetSpaceDefaults(rack);
        bpm = tempo; rack.bpm = tempo;
        stack.clear();
        for (int v = 0; v < kVoices; ++v) {
            voices[v].rng = Rng(1234567u + v * 7919u);
            for (int l = 0; l < VoiceP::kLayers; ++l) {
                voices[v].gen[l] = makeGen(vp.layer[l].type);
                if (voices[v].gen[l]) voices[v].gen[l]->prepare(sr);
                voices[v].amp[l].kill();
            }
            voices[v].note = -1; voices[v].held = false; voices[v].bleedL = voices[v].bleedR = 0;
        }
    }
    void setPitchBend(float norm) { bendSemi = clampf(norm, -1.f, 1.f) * vp.bend; }
    void setTempo(float t) { bpm = clampf(t, 40.f, 240.f); fx.bpm = bpm; rack.bpm = bpm; }
    void setParam(int e, int i, float v) {
        bool monoBefore = isMono();
        rack.setParam(e, i, v);
        if (e == FX_REVERB && i == 1) FX::reverbTypeDefaults(rack, (int)rack.v(FX_REVERB, 1), fx);
        if (isMono() != monoBefore) allNotesOff();
    }
    void setOn(int e, bool b) { bool monoBefore = isMono(); rack.setOn(e, b); if (isMono() != monoBefore) allNotesOff(); }

    void noteOn(int note, float velocity) {
        velocity = clampf(velocity, 0.f, 1.f);
        rack.noteOnEvent();
        const bool layerOn = rack.on[FX_OCTAVE];
        static const int offs[4] = {-24, -12, 12, 24};
        const int off = offs[(int)rack.v(FX_OCTAVE, 1)];
        if (isMono()) {
            stack.erase(std::remove(stack.begin(), stack.end(), note), stack.end());
            stack.push_back(note);
            Voice& v = voices[0];
            bool legato = v.active() && v.held;
            startVoice(v, note, velocity, legato, false, note);
            if (layerOn) { Voice& w = voices[1]; startVoice(w, note + off, velocity, w.active() && w.held, true, note); }
            lastFreq = mtof((float)note);
            return;
        }
        startVoice(voices[pickVoice(note)], note, velocity, false, false, note);
        if (layerOn && note + off >= 0 && note + off < 128) startVoice(voices[pickVoice(-1)], note + off, velocity, false, true, note);
        lastFreq = mtof((float)note);
    }

    void noteOff(int note) {
        if (isMono()) {
            stack.erase(std::remove(stack.begin(), stack.end(), note), stack.end());
            for (int i = 0; i < 2; ++i) {
                Voice& v = voices[i];
                if (v.srcNote != note) continue;
                if (!stack.empty()) {
                    int back = stack.back(), d = v.note - v.srcNote;
                    v.srcNote = back; v.note = back + d; v.target = mtof((float)v.note + v.pitchAtStart);
                    lastFreq = mtof((float)back);
                } else release(v);
            }
            return;
        }
        for (auto& v : voices) if (v.srcNote == note && v.held) release(v);
    }

    void allNotesOff() { for (auto& v : voices) release(v); stack.clear(); }

    void process(float* outL, float* outR, int n) {
        for (int i = 0; i < n; ++i) {
            float L = 0, R = 0, lL = 0, lR = 0;
            float pitchT = rack.on[FX_PITCH] ? pitchTarget() : 0.f;
            pitchSm += (pitchT - pitchSm) * 0.0015f;   // pitch knob glides (~15 ms), no zipper
            for (auto& v : voices) if (v.sounding()) {
                float l, r; renderVoice(v, l, r);
                if (v.layer) { lL += l; lR += r; } else { L += l; R += r; }
            }
            // OCTAVE LAYER bus: own tone filter, level, and the original's balance
            layerLevel.t = rack.on[FX_OCTAVE] ? rack.v(FX_OCTAVE, 0) : 0.f;
            balance.t = rack.on[FX_OCTAVE] ? rack.v(FX_OCTAVE, 3) : 1.f;
            float ll = layerLevel.tick(), bal = balance.tick();
            if (ll > 1e-5f || lL != 0.f) {
                float fc = rack.v(FX_OCTAVE, 2);
                if (fc != layerToneSet) { layerTone[0].set(fc, .707f, sr); layerTone[1].set(fc, .707f, sr); layerToneSet = fc; }
                lL = layerTone[0].lp(lL); lR = layerTone[1].lp(lR);
            }
            L = L * bal + lL * ll; R = R * bal + lR * ll;
            L *= 0.35f; R *= 0.35f;

            rack.pos = fx.samplePos;
            fx.pre(L, R);
            rack.filter(L, R); rack.resonator(L, R); rack.saturation(L, R); rack.tape(L, R); rack.glue(L, R);
            rack.chorus(L, R); rack.phaser(L, R); rack.flanger(L, R);
            rack.stutter(L, R); rack.gate(L, R); rack.reverse(L, R);
            fx.space(L, R, rack);
            rack.shimmer(L, R); rack.freeze(L, R); rack.granular(L, R); rack.width(L, R);
            fx.post(L, R);
            // output safety: transparent below -1 dBFS, smooth knee above (never a hard clip)
            auto safe = [](float x) { const float t = .89f; float a = std::fabs(x); if (a <= t) return x; float y = t + (1.f - t) * std::tanh((a - t) / (1.f - t)); return x < 0 ? -y : y; };
            outL[i] = safe(std::isfinite(L) ? L : 0.f); outR[i] = safe(std::isfinite(R) ? R : 0.f);
        }
    }

private:
    float sr = 44100, bpm = 120, bendSemi = 0, pitchSm = 0, lastFreq = 0, layerToneSet = -1;
    uint64_t counter = 0;
    ParamMap params; VoiceP vp; FX fx;
    std::array<Voice, kVoices> voices;
    std::vector<int> stack;
    Ramp layerLevel, balance; OnePole dummy; SVF layerTone[2];

    bool isMono() const { return rack.on[FX_GLIDE] ? rack.v(FX_GLIDE, 1) < 1.5f : vp.mono != 0; }
    float glideTime() const { return rack.on[FX_GLIDE] ? rack.v(FX_GLIDE, 0) : vp.glide; }
    float pitchTarget() const {
        static const float rng[5] = {2, 5, 7, 12, 24};
        float r = rng[(int)rack.v(FX_PITCH, 1)];
        float s = clampf(rack.v(FX_PITCH, 0), -r, r);
        return rack.v(FX_PITCH, 2) > .5f ? s : std::round(s);
    }

    int pickVoice(int note) {
        if (note >= 0) for (auto& v : voices) if (v.srcNote == note && v.held) release(v);   // repeated key: old note rings out
        for (int i = 2; i < kVoices; ++i) if (!voices[i].active()) return i;
        int best = 2; float bestScore = 1e30f;
        for (int i = 2; i < kVoices; ++i) {
            float score = (voices[i].held ? 1e12f : 0.f) + (float)voices[i].order;
            if (score < bestScore) { bestScore = score; best = i; }
        }
        return best;
    }

    void release(Voice& v) {
        v.held = false;
        for (auto& e : v.amp) e.off();
        v.fenv.off();
    }

    void startVoice(Voice& v, int note, float vel, bool legato, bool layer, int src) {
        if (!legato) v.pitchAtStart = pitchSm;
        float f = mtof((float)note + v.pitchAtStart);
        const bool mono = isMono();
        const float gt = glideTime();
        const int gmode = rack.on[FX_GLIDE] ? (int)rack.v(FX_GLIDE, 1) : 0;
        v.note = note; v.held = true; v.order = ++counter; v.layer = layer; v.srcNote = src;
        v.target = f;
        if (!legato || gt <= 0) v.freq = f;
        // glide from the previous note: always-mono mode and poly mode also glide on detached notes
        if (rack.on[FX_GLIDE] && gt > 0 && !legato && lastFreq > 0 && gmode >= 1) {
            float ratio = (float)mtof((float)note) / mtof((float)src);
            v.freq = lastFreq * ratio * std::exp2(v.pitchAtStart / 12.f);
        }
        // GLIDE effect: arrives (~99%) within the knob time; a preset's own glide keeps its designed feel
        v.glideK = gt > 0 ? std::exp(-(rack.on[FX_GLIDE] ? 4.6f : 1.f) / (gt * sr)) : 0.f;
        if (legato) return;
        const bool wasActive = v.active();
        if (wasActive && !mono) { v.bleedL += v.lastL; v.bleedR += v.lastR; }
        v.vel = vel; v.t = 0; v.ctr = 0;
        if (!(wasActive && mono)) v.velSm = vel;
        v.pEnvLvl = vp.pEnv; v.pEnvK = std::exp(-1.f / (std::max(vp.pDec, 0.001f) * sr));
        v.vibPh = 0;
        v.lfoPh = vp.lfoSync > 0 ? 0.f : v.rng.uni();
        NoteCtx c{sr, vel, f, 0, 0, &v.rng};
        for (int l = 0; l < VoiceP::kLayers; ++l) {
            const LayerP& lp = vp.layer[l];
            if (!v.gen[l]) continue;
            float lf = f * lp.ratio;
            c.startFreq = lf;
            v.gen[l]->retrig = wasActive && mono;
            v.gen[l]->start(lp, c);
            float kd = std::pow(261.6f / f, lp.keydecay);
            v.amp[l].setup(lp.a, lp.d * kd, lp.s, lp.r, sr);
            v.amp[l].on(!(wasActive && mono));
            v.drift[l] = 0; v.driftT[l] = 0;
        }
        v.fenv.setup(vp.fa, vp.fd, vp.fs, vp.fr, sr); v.fenv.on(!(wasActive && mono));
        if (!wasActive) for (auto& st : v.filt) for (auto& f2 : st) f2.reset();
    }

    inline float lfoValue(Voice& v) {
        float rate = vp.lfoSync > 0 ? (bpm / 60.f) / vp.lfoSync : vp.lfoRate;
        float prev = v.lfoPh;
        v.lfoPh += rate / sr; if (v.lfoPh >= 1) v.lfoPh -= 1;
        switch (vp.lfoShape) {
            case 1: return 4.f * std::fabs(v.lfoPh - 0.5f) - 1.f;
            case 2: if (v.lfoPh < prev) v.sh = v.rng.bi(); return v.sh;
            default: return std::sin(kTwoPi * v.lfoPh);
        }
    }

    void renderVoice(Voice& v, float& outL, float& outR) {
        // glide in log-frequency (musical): equal time per octave, arrives within the set time
        if (v.glideK > 0 && std::fabs(v.freq - v.target) > v.target * 1e-5f) v.freq = v.target * std::exp2(std::log2(v.freq / v.target) * v.glideK); else v.freq = v.target;
        float semis = v.pEnvLvl + bendSemi + (pitchSm - v.pitchAtStart); v.pEnvLvl *= v.pEnvK;
        if (vp.vibDepth > 0) {
            float ramp = clampf((v.t - vp.vibDelay) / 0.4f, 0.f, 1.f);
            v.vibPh += vp.vibRate / sr; if (v.vibPh >= 1) v.vibPh -= 1;
            semis += std::sin(kTwoPi * v.vibPh) * vp.vibDepth / 100.f * ramp;
        }
        float lfo = lfoValue(v);
        float baseF = semis != 0.f ? v.freq * std::exp2(semis * (1.f / 12.f)) : v.freq;
        v.velSm += (v.vel - v.velSm) * 0.004f;
        NoteCtx c{sr, v.vel, baseF, v.t, lfo, &v.rng};
        float fl = 0, fr = 0, bl = 0, br = 0;
        for (int l = 0; l < VoiceP::kLayers; ++l) {
            if (!v.gen[l] || !v.amp[l].active()) continue;
            const LayerP& lp = vp.layer[l];
            float e = v.amp[l].tick();
            float lf = baseF * lp.ratio;
            if (lp.drift > 0) {
                v.driftT[l] += 0.0004f * v.rng.bi();
                v.driftT[l] = clampf(v.driftT[l], -1.f, 1.f);
                v.drift[l] += (v.driftT[l] - v.drift[l]) * 0.0005f;
                lf *= std::exp2(v.drift[l] * lp.drift * (1.f / 1200.f));
            }
            float L, R; v.gen[l]->tick(lp, lf, c, L, R);
            float vc = vp.ampVelCurve == 1.f ? v.velSm : std::pow(v.velSm, vp.ampVelCurve);
            float velG = 1.f - lp.vel + lp.vel * vc;
            float g = e * lp.level * velG;
            if (lp.filt > 0.5f) { fl += L * g * lp.panL; fr += R * g * lp.panR; }
            else { bl += L * g * lp.panL; br += R * g * lp.panR; }
        }
        if (vp.fType >= 0 && vp.fType <= 2) {
            float fe = v.fenv.tick();
            if ((v.ctr & 15) == 0) {
                float oct = fe * vp.fEnv + vp.fKey * (std::log2(baseF / 261.6f)) + (v.velSm - 0.8f) * vp.fVel + lfo * vp.lfoCut;
                v.cutCache = clampf(vp.fCut * std::exp2(oct), 20.f, sr * 0.45f);
                float q = 0.55f + vp.fRes * vp.fRes * (vp.fPoles > 2.5f ? 6.f : 12.f);
                for (int ch = 0; ch < 2; ++ch) {
                    v.filt[ch][0].set(v.cutCache, q, sr);
                    v.filt[ch][1].set(v.cutCache, vp.fPoles > 2.5f ? 0.6f : q, sr);
                }
            }
            if (vp.fDrive > 1.01f) { float d = vp.fDrive; fl = std::tanh(fl * d) / std::sqrt(d); fr = std::tanh(fr * d) / std::sqrt(d); }
            auto run = [&](SVF& f, float x) { float l, b, h; f.proc(x, l, b, h); return vp.fType == 0 ? l : vp.fType == 1 ? b * 1.5f : h; };
            fl = run(v.filt[0][0], fl); fr = run(v.filt[1][0], fr);
            if (vp.fPoles > 2.5f) { fl = run(v.filt[0][1], fl); fr = run(v.filt[1][1], fr); }
        }
        float L = fl + bl, R = fr + br;
        if (vp.lfoAmp > 0) { float g = 1.f - vp.lfoAmp * (0.5f + 0.5f * lfo); L *= g; R *= g; }
        if (vp.lfoPan > 0) { float p = lfo * vp.lfoPan; L *= 1.f - std::max(0.f, p); R *= 1.f + std::min(0.f, p); }
        outL = L * vp.voiceGain; outR = R * vp.voiceGain;
        if (v.bleedL != 0.f || v.bleedR != 0.f) {
            outL += v.bleedL; outR += v.bleedR;
            v.bleedL *= 0.993f; v.bleedR *= 0.993f;
            if (std::fabs(v.bleedL) + std::fabs(v.bleedR) < 1e-6f) v.bleedL = v.bleedR = 0.f;
        }
        v.lastL = outL; v.lastR = outR;
        v.t += 1.f / sr; ++v.ctr;
        if (!v.active()) { v.note = -1; v.srcNote = -1; v.lastL = v.lastR = 0.f; }
    }
};

}  // namespace lumi

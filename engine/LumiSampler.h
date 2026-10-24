// LUMELODY Sampler: plays imported audio from MIDI notes (framework-free, header-only, shared by the web
// engine and the plugin). The host decodes the file and keeps the ORIGINAL audio untouched; edits that change the
// audio itself (time-stretch, reverse) are rendered into a separate playback buffer by buildPlayback(), so every
// edit is non-destructive and can be undone by rebuilding from the original.
//
// Markers (start/end, loop, slices) are fractions 0..1 of the playback buffer, so they keep pointing at the same
// audio when the whole sample is reversed (the host mirrors them) or stretched.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace lumi {

struct AudioData {   // stereo float audio; mono files are stored with R == L
    std::vector<float> L, R;
    double sr = 44100;
    int channels = 2;
    size_t frames() const { return L.size(); }
};

// ---------------------------------------------------------------- non-destructive processing
namespace dsp {

inline void reverseRange(AudioData& a, size_t from, size_t to) {   // [from, to)
    to = std::min(to, a.frames());
    if (from >= to) return;
    std::reverse(a.L.begin() + (long) from, a.L.begin() + (long) to);
    std::reverse(a.R.begin() + (long) from, a.R.begin() + (long) to);   // both channels: left/right stay aligned
}

// WSOLA time-stretch: changes duration by `ratio` (2 = twice as long) without changing pitch.
inline AudioData stretch(const AudioData& in, double ratio) {
    AudioData out; out.sr = in.sr; out.channels = in.channels;
    const size_t n = in.frames();
    if (n == 0 || std::fabs(ratio - 1.0) < 1e-4) { out = in; return out; }
    const int W = std::max(256, (int) std::lround(in.sr * 0.046)) & ~1;   // ~46 ms windows
    const int H = W / 2, S = W / 4;                                        // output hop, search range
    const size_t outN = (size_t) std::llround((double) n * ratio);
    out.L.assign(outN + (size_t) W, 0.f); out.R.assign(outN + (size_t) W, 0.f);
    std::vector<float> win((size_t) W), norm(outN + (size_t) W, 0.f);
    for (int i = 0; i < W; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos(6.28318530718f * (float) i / (float) W);
    auto mono = [&](long i) { return (i >= 0 && (size_t) i < n) ? 0.5f * (in.L[(size_t) i] + in.R[(size_t) i]) : 0.f; };
    long prevIn = 0;
    for (size_t o = 0; o < outN; o += (size_t) H) {
        long ideal = (long) std::llround((double) o / ratio);
        long best = ideal;
        if (o > 0) {   // the input frame that continues the previous one most naturally
            double bestC = -1e30;
            const long natural = prevIn + H;
            for (long c = ideal - S; c <= ideal + S; c += 2) {
                double acc = 0;
                for (int k = 0; k < W / 2; k += 4) acc += (double) mono(natural + k) * (double) mono(c + k);
                if (acc > bestC) { bestC = acc; best = c; }
            }
        }
        best = std::max(0L, std::min(best, (long) n - 1));
        for (int k = 0; k < W; ++k) {
            const long src = best + k;
            if ((size_t) src >= n) break;
            out.L[o + (size_t) k] += in.L[(size_t) src] * win[(size_t) k];
            out.R[o + (size_t) k] += in.R[(size_t) src] * win[(size_t) k];
            norm[o + (size_t) k] += win[(size_t) k];
        }
        prevIn = best;
    }
    for (size_t i = 0; i < outN; ++i) { const float g = norm[i] > 1e-3f ? 1.f / norm[i] : 0.f; out.L[i] *= g; out.R[i] *= g; }
    out.L.resize(outN); out.R.resize(outN);
    return out;
}

}  // namespace dsp

// ---------------------------------------------------------------- settings (also the preset / project format)
struct SamplerSettings {
    // playback buffer edits (rebuild needed)
    double stretch = 1.0;                // time-stretch ratio
    int reverse = 0;                     // 0 off, 1 whole sample, 2 selected region
    double revA = 0, revB = 1;           // region to reverse (fractions of the stretched audio)
    // markers (fractions of the playback buffer)
    double start = 0, end = 1, loopA = 0, loopB = 1;
    int loop = 0;                        // 1 = loop between loopA and loopB while the note is held
    // mapping
    int mode = 0;                        // 0 chromatic (pitch follows the key), 1 slices on keys, 2 one-shot (same pitch on every key)
    int root = 60;                       // the key that plays the sample at its own pitch
    double tune = 0;                     // semitones (with cents)
    int sliceBase = 60;                  // slice 0 on this key (pads use the same notes)
    std::vector<double> slices;          // slice START points (fractions), ascending
    // voice
    double attack = .002, decay = .3, sustain = 1, release = .08;
    double gainDb = 0, pan = 0, velSens = .7;
    int oneShot = 0;                     // 1 = play to the end even after the key is released
    int polyphony = 16;

    std::string toString() const {
        char b[512];
        std::snprintf(b, sizeof b, "stretch=%.6g;reverse=%d;revA=%.6g;revB=%.6g;start=%.6g;end=%.6g;loopA=%.6g;loopB=%.6g;loop=%d;mode=%d;root=%d;tune=%.6g;"
                      "sliceBase=%d;attack=%.6g;decay=%.6g;sustain=%.6g;release=%.6g;gainDb=%.6g;pan=%.6g;velSens=%.6g;oneShot=%d;polyphony=%d;",
                      stretch, reverse, revA, revB, start, end, loopA, loopB, loop, mode, root, tune, sliceBase, attack, decay, sustain, release,
                      gainDb, pan, velSens, oneShot, polyphony);
        std::string s = b; s += "slices=";
        for (size_t i = 0; i < slices.size(); ++i) { std::snprintf(b, sizeof b, "%s%.6g", i ? "," : "", slices[i]); s += b; }
        return s + ";";
    }
    static SamplerSettings fromString(const std::string& st) {
        SamplerSettings s; size_t p = 0;
        while (p < st.size()) {
            size_t semi = st.find(';', p); if (semi == std::string::npos) semi = st.size();
            const std::string kv = st.substr(p, semi - p); p = semi + 1;
            const size_t eq = kv.find('='); if (eq == std::string::npos) continue;
            const std::string k = kv.substr(0, eq), v = kv.substr(eq + 1);
            const double x = std::atof(v.c_str());
            if (k == "stretch") s.stretch = std::min(4.0, std::max(.25, x)); else if (k == "reverse") s.reverse = (int) x;
            else if (k == "revA") s.revA = x; else if (k == "revB") s.revB = x; else if (k == "start") s.start = x; else if (k == "end") s.end = x;
            else if (k == "loopA") s.loopA = x; else if (k == "loopB") s.loopB = x; else if (k == "loop") s.loop = (int) x;
            else if (k == "mode") s.mode = (int) x; else if (k == "root") s.root = (int) x; else if (k == "tune") s.tune = x;
            else if (k == "sliceBase") s.sliceBase = (int) x; else if (k == "attack") s.attack = x; else if (k == "decay") s.decay = x;
            else if (k == "sustain") s.sustain = x; else if (k == "release") s.release = x; else if (k == "gainDb") s.gainDb = x;
            else if (k == "pan") s.pan = x; else if (k == "velSens") s.velSens = x; else if (k == "oneShot") s.oneShot = (int) x;
            else if (k == "polyphony") s.polyphony = (int) x;
            else if (k == "slices") { s.slices.clear(); size_t q = 0; while (q < v.size()) { size_t c = v.find(',', q); if (c == std::string::npos) c = v.size(); if (c > q) s.slices.push_back(std::atof(v.substr(q, c - q).c_str())); q = c + 1; } }
        }
        s.sanitize();
        return s;
    }
    void sanitize() {
        auto c01 = [](double& x) { x = std::min(1.0, std::max(0.0, x)); };
        c01(start); c01(end); c01(loopA); c01(loopB); c01(revA); c01(revB);
        if (end < start) std::swap(start, end);
        if (loopB < loopA) std::swap(loopA, loopB);
        if (revB < revA) std::swap(revA, revB);
        root = std::max(0, std::min(127, root)); sliceBase = std::max(0, std::min(127, sliceBase));
        polyphony = std::max(1, std::min(32, polyphony));
        attack = std::max(.0005, attack); release = std::max(.002, release); decay = std::max(.001, decay); sustain = std::min(1.0, std::max(0.0, sustain));
        std::sort(slices.begin(), slices.end());
        slices.erase(std::remove_if(slices.begin(), slices.end(), [](double x) { return x < 0 || x >= 1; }), slices.end());
        slices.erase(std::unique(slices.begin(), slices.end(), [](double a, double b) { return std::fabs(a - b) < 1e-7; }), slices.end());
    }
};

// original audio + edits -> the buffer that is played
inline std::shared_ptr<AudioData> buildPlayback(const AudioData& original, const SamplerSettings& s) {
    auto pb = std::make_shared<AudioData>(std::fabs(s.stretch - 1.0) > 1e-4 ? dsp::stretch(original, s.stretch) : original);
    const size_t n = pb->frames();
    if (s.reverse == 1) dsp::reverseRange(*pb, 0, n);
    else if (s.reverse == 2) dsp::reverseRange(*pb, (size_t) std::llround(s.revA * (double) n), (size_t) std::llround(s.revB * (double) n));
    return pb;
}

// ---------------------------------------------------------------- the instrument
class Sampler {
public:
    static constexpr int kVoices = 32;

    void prepare(double sampleRate) { sr = sampleRate; for (auto& v : voices) v.active = false; }
    void setPlayback(std::shared_ptr<const AudioData> buffer, const SamplerSettings& s) {
        for (auto& v : voices) v.active = false;   // the old buffer stops cleanly (the host swaps under its lock)
        pb = std::move(buffer); set = s; set.sanitize();
    }
    void setSettings(const SamplerSettings& s) { set = s; set.sanitize(); }   // markers / mapping / voice only (no rebuild)
    const SamplerSettings& settings() const { return set; }
    bool hasSample() const { return pb && pb->frames() > 1; }
    std::shared_ptr<const AudioData> playback() const { return pb; }

    void noteOn(int note, float vel) {
        if (!hasSample()) return;
        const size_t n = pb->frames();
        double a = set.start, b = set.end, ratio = 1.0;
        if (set.mode == 1) {   // slices: each key plays its own slice at the sample's pitch
            std::vector<double> st; st.push_back(set.start);
            for (double x : set.slices) if (x > set.start + 1e-6 && x < set.end - 1e-6) st.push_back(x);
            const int k = note - set.sliceBase;
            if (k < 0 || k >= (int) st.size()) return;
            a = st[(size_t) k]; b = k + 1 < (int) st.size() ? st[(size_t) k + 1] : set.end;
            ratio = std::pow(2.0, set.tune / 12.0);
        } else if (set.mode == 2) ratio = std::pow(2.0, set.tune / 12.0);
        else ratio = std::pow(2.0, (note - set.root + set.tune) / 12.0);
        if (b - a < 1.0 / (double) n) return;
        // retrigger: the same key restarts with a quick fade of the old voice
        for (auto& v : voices) if (v.active && v.note == note && !v.releasing) v.release(rel());
        Voice* v = pick();
        v->active = true; v->releasing = false; v->note = note;
        v->pos = a * (double) n; v->from = v->pos; v->to = b * (double) n;
        v->inc = ratio * pb->sr / sr;
        v->loop = set.loop && set.mode != 1;
        v->loopA = std::max(v->from, set.loopA * (double) n); v->loopB = std::min(v->to, set.loopB * (double) n);
        if (v->loopB - v->loopA < 64) v->loop = false;
        v->gain = (float) std::pow(10.0, set.gainDb / 20.0) * (float) (1.0 - set.velSens + set.velSens * vel);
        const double pn = std::min(1.0, std::max(-1.0, set.pan));
        v->gl = (float) std::cos((pn + 1) * .25 * 3.14159265358979); v->gr = (float) std::sin((pn + 1) * .25 * 3.14159265358979);
        v->env = 0; v->stage = 0; v->order = ++counter;
        v->aInc = 1.0 / std::max(1.0, set.attack * sr);
        v->dK = std::exp(-1.0 / std::max(1.0, set.decay * sr));
        v->sus = set.sustain;
        v->edgeIn = a > 1e-9 ? 0.0 : 1.0;   // starting inside the audio: 1 ms fade-in so it never clicks
        v->edgeInc = 1.0 / std::max(1.0, .001 * sr);
    }
    void noteOff(int note) {
        if (set.oneShot) return;
        for (auto& v : voices) if (v.active && v.note == note && !v.releasing) v.release(rel());
    }
    void allNotesOff() { for (auto& v : voices) if (v.active) v.release(rel()); }

    // adds into L/R (n frames)
    void process(float* L, float* R, int n) {
        if (!hasSample()) return;
        const float* sl = pb->L.data(); const float* sr_ = pb->R.data();
        const long N = (long) pb->frames();
        for (auto& v : voices) {
            if (!v.active) continue;
            for (int i = 0; i < n; ++i) {
                // envelope: attack, decay to sustain, release
                if (v.releasing) { v.env *= v.rK; if (v.env < 1e-4) { v.active = false; break; } }
                else if (v.stage == 0) { v.env += v.aInc; if (v.env >= 1) { v.env = 1; v.stage = 1; } }
                else { v.env = v.sus + (v.env - v.sus) * v.dK; }
                double p = v.pos;
                if (!v.loop && p >= v.to - 1) { v.active = false; break; }
                float a, b; read(sl, sr_, N, p, a, b);
                // loop seam: crossfade the last part of the loop with the audio just before the loop start
                if (v.loop) {
                    const double xf = std::min(256.0 * v.inc, (v.loopB - v.loopA) * .25);
                    if (p > v.loopB - xf) {
                        const float w = (float) ((p - (v.loopB - xf)) / xf);
                        float a2, b2; read(sl, sr_, N, p - (v.loopB - v.loopA), a2, b2);
                        a = a * (1 - w) + a2 * w; b = b * (1 - w) + b2 * w;
                    }
                }
                // tail fade over the last ~3 ms of the region (no click at the end of a slice / sample)
                double tail = 1.0;
                if (!v.loop) { const double left = (v.to - p) / v.inc, fl = .003 * sr; if (left < fl) tail = std::max(0.0, left / fl); }
                if (v.edgeIn < 1) { v.edgeIn = std::min(1.0, v.edgeIn + v.edgeInc); tail *= v.edgeIn; }
                const float g = (float) (v.env * tail) * v.gain;
                L[i] += a * g * v.gl * 1.41421356f; R[i] += b * g * v.gr * 1.41421356f;
                v.pos += v.inc;
                if (v.loop && v.pos >= v.loopB) v.pos -= (v.loopB - v.loopA);
            }
        }
    }
    int activeVoices() const { int c = 0; for (auto& v : voices) c += v.active; return c; }

private:
    struct Voice {
        bool active = false, releasing = false, loop = false;
        int note = -1, stage = 0; uint64_t order = 0;
        double pos = 0, from = 0, to = 0, inc = 1, loopA = 0, loopB = 0, env = 0, aInc = 1, dK = 1, sus = 1, rK = .99, edgeIn = 1, edgeInc = 1;
        float gain = 1, gl = .707f, gr = .707f;
        void release(double k) { releasing = true; rK = k; }
    };
    double rel() const { return std::exp(-1.0 / std::max(1.0, set.release * sr / 6.9)); }   // ~-60 dB at the release time
    Voice* pick() {
        int count = 0; Voice* oldest = nullptr; Voice* freeV = nullptr;
        for (auto& v : voices) { if (v.active) { ++count; if (!oldest || v.order < oldest->order) oldest = &v; } else if (!freeV) freeV = &v; }
        if (count >= set.polyphony && oldest) { oldest->active = false; return oldest; }
        return freeV ? freeV : oldest;
    }
    static inline void read(const float* L, const float* R, long N, double p, float& a, float& b) {   // 4-point Hermite
        const long i = (long) p; const float t = (float) (p - (double) i);
        auto at = [&](const float* x, long k) { return (k >= 0 && k < N) ? x[k] : 0.f; };
        auto h = [&](const float* x) {
            const float y0 = at(x, i - 1), y1 = at(x, i), y2 = at(x, i + 1), y3 = at(x, i + 2);
            const float c1 = .5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.f * y2 - .5f * y3, c3 = .5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * t + c2) * t + c1) * t + y1;
        };
        a = h(L); b = h(R);
    }
    double sr = 44100;
    std::shared_ptr<const AudioData> pb;
    SamplerSettings set;
    Voice voices[kVoices];
    uint64_t counter = 0;
};

}  // namespace lumi

// Sampler, reverse, stretch and analysis checks (no framework). Build: g++ -O2 -std=c++17 tools/tests/sampler_test.cpp -o build/sampler_test
#include "../../engine/LumiEngine.h"
#include "../../engine/LumiAnalysis.h"
#include <cstdio>
#include <fstream>
#include <sstream>
using namespace lumi;

static int fails = 0, checks = 0;
#define CHECK(c, ...) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: ", __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static AudioData sine(double f, double secs, double sr = 44100, float amp = .5f) {
    AudioData a; a.sr = sr; size_t n = (size_t) (secs * sr); a.L.resize(n); a.R.resize(n);
    for (size_t i = 0; i < n; ++i) { a.L[i] = a.R[i] = amp * (float) std::sin(2 * M_PI * f * (double) i / sr); }
    return a;
}
// frequency from positive zero crossings (linear interpolation) over [a, b)
static double freqOf(const std::vector<float>& x, size_t a, size_t b, double sr) {
    std::vector<double> zc;
    for (size_t i = a + 1; i < b; ++i) if (x[i - 1] < 0 && x[i] >= 0) zc.push_back((double) (i - 1) + x[i - 1] / (x[i - 1] - x[i]));
    if (zc.size() < 3) return 0;
    return sr * (double) (zc.size() - 1) / (zc.back() - zc.front());
}
static double rms(const std::vector<float>& x, size_t a, size_t b) { double s = 0; b = std::min(b, x.size()); for (size_t i = a; i < b; ++i) s += x[i] * x[i]; return b > a ? std::sqrt(s / (double) (b - a)) : 0; }

struct Out { std::vector<float> L, R; };
static Out runSampler(Sampler& s, int note, float vel, double holdSecs, double totalSecs, double sr = 44100, int offAt = -1) {
    Out o; size_t n = (size_t) (totalSecs * sr); o.L.assign(n, 0.f); o.R.assign(n, 0.f);
    s.noteOn(note, vel);
    size_t hold = (size_t) (holdSecs * sr);
    for (size_t i = 0; i < n; i += 64) { if (i <= hold && hold < i + 64) s.noteOff(note); s.process(&o.L[i], &o.R[i], (int) std::min<size_t>(64, n - i)); }
    (void) offAt; return o;
}

int main() {
    const double SR = 44100;
    // ---- pitch: a 440 Hz sample with root A4 (69) follows the keys
    {
        Sampler s; s.prepare(SR); SamplerSettings st; st.root = 69; st.release = .01;
        auto src = sine(440, 2); s.setPlayback(buildPlayback(src, st), st);
        for (auto [note, want] : std::vector<std::pair<int, double>> {{69, 440}, {81, 880}, {57, 220}, {76, 659.255}}) {
            Sampler t; t.prepare(SR); t.setPlayback(buildPlayback(src, st), st);
            auto o = runSampler(t, note, 1, .8, .9);
            double f = freqOf(o.L, 4410, 30000, SR);
            CHECK(std::fabs(f - want) / want < .002, "note %d: %.2f Hz, want %.2f", note, f, want);
            std::printf("pitch: note %d -> %.2f Hz (want %.2f)\n", note, f, want);
        }
        // sample at 48 kHz played in a 44.1 kHz engine keeps its pitch
        auto s48 = sine(440, 1, 48000); Sampler t; t.prepare(SR); t.setPlayback(buildPlayback(s48, st), st);
        auto o = runSampler(t, 69, 1, .6, .7); double f = freqOf(o.L, 4410, 25000, SR);
        CHECK(std::fabs(f - 440) < 1, "48k sample in 44.1k engine: %.2f Hz", f);
        // tune +100 cents
        SamplerSettings st2 = st; st2.tune = 1; Sampler u; u.prepare(SR); u.setPlayback(buildPlayback(src, st2), st2);
        o = runSampler(u, 69, 1, .6, .7); f = freqOf(o.L, 4410, 25000, SR);
        CHECK(std::fabs(f - 466.16) < 1, "tune +1 st: %.2f Hz", f);
    }
    // ---- reverse: exact, stereo-aligned, region-only, repeatable, original untouched
    {
        AudioData a; a.sr = SR; size_t n = 1001; a.L.resize(n); a.R.resize(n);
        for (size_t i = 0; i < n; ++i) { a.L[i] = (float) i / (float) n; a.R[i] = -(float) (i % 37) / 37.f; }
        const AudioData orig = a;
        SamplerSettings st; st.reverse = 1;
        auto pb = buildPlayback(a, st);
        bool exact = true; for (size_t i = 0; i < n; ++i) exact &= pb->L[i] == orig.L[n - 1 - i] && pb->R[i] == orig.R[n - 1 - i];
        CHECK(exact, "full reverse is not an exact mirror (both channels)");
        CHECK(a.L == orig.L && a.R == orig.R, "the original audio was modified");
        st.reverse = 2; st.revA = .2; st.revB = .6;
        pb = buildPlayback(a, st);
        size_t ra = (size_t) std::llround(.2 * n), rb = (size_t) std::llround(.6 * n); bool ok = true;
        for (size_t i = 0; i < n; ++i) { size_t j = (i >= ra && i < rb) ? ra + rb - 1 - i : i; ok &= pb->L[i] == orig.L[j] && pb->R[i] == orig.R[j]; }
        CHECK(ok, "region reverse wrong");
        AudioData twice = orig; dsp::reverseRange(twice, 0, n); dsp::reverseRange(twice, 0, n);
        CHECK(twice.L == orig.L && twice.R == orig.R, "reversing twice does not restore the original");
        // very short samples
        AudioData tiny; tiny.sr = SR; tiny.L = {1, 2, 3}; tiny.R = {4, 5, 6}; dsp::reverseRange(tiny, 0, 3);
        CHECK(tiny.L[0] == 3 && tiny.R[0] == 6 && tiny.L[2] == 1, "3-frame reverse");
        AudioData empty; empty.sr = SR; auto e = buildPlayback(empty, st); CHECK(e->frames() == 0, "empty reverse");
        // playing a reversed sample really plays it backwards: a rising sweep becomes a falling one
        AudioData sw; sw.sr = SR; size_t m = (size_t) SR; sw.L.resize(m); sw.R.resize(m); double ph = 0;
        for (size_t i = 0; i < m; ++i) { double f = 200 + 600 * (double) i / (double) m; ph += 2 * M_PI * f / SR; sw.L[i] = sw.R[i] = .5f * (float) std::sin(ph); }
        SamplerSettings rv; rv.reverse = 1; rv.root = 60; Sampler s; s.prepare(SR); s.setPlayback(buildPlayback(sw, rv), rv);
        auto o = runSampler(s, 60, 1, 1.2, 1.2);
        double f0 = freqOf(o.L, 1000, 6000, SR), f1 = freqOf(o.L, 38000, 43000, SR);
        CHECK(f0 > 700 && f1 < 320, "reversed sweep should fall: start %.0f Hz, end %.0f Hz", f0, f1);
        std::printf("reverse playback: sweep starts %.0f Hz, ends %.0f Hz\n", f0, f1);
    }
    // ---- loop: a short sample sustains while held, without clicks at the seam
    {
        auto src = sine(441, .25); SamplerSettings st; st.root = 69; st.loop = 1; st.loopA = .2; st.loopB = .8; st.release = .05;
        Sampler s; s.prepare(SR); s.setPlayback(buildPlayback(src, st), st);
        auto o = runSampler(s, 69, 1, 1.5, 1.7);
        CHECK(rms(o.L, (size_t) (1.2 * SR), (size_t) (1.45 * SR)) > .2, "loop did not sustain");
        CHECK(rms(o.L, (size_t) (1.62 * SR), (size_t) (1.7 * SR)) < .01, "loop did not release");
        double maxStep = 0; for (size_t i = 4410; i < (size_t) (1.45 * SR); ++i) maxStep = std::max(maxStep, (double) std::fabs(o.L[i] - o.L[i - 1]));
        double sineStep = 2 * M_PI * 441 / SR * .5 * 1.42;
        CHECK(maxStep < sineStep * 1.6, "loop seam click: step %.4f vs %.4f", maxStep, sineStep);
        std::printf("loop: sustained, max sample step %.4f (pure sine %.4f)\n", maxStep, sineStep);
    }
    // ---- start/end trim, one-shot, slices on keys
    {
        AudioData a; a.sr = SR; size_t seg = (size_t) (.25 * SR); double fs[4] = {220, 330, 440, 660};
        for (int k = 0; k < 4; ++k) for (size_t i = 0; i < seg; ++i) { float v = .5f * (float) std::sin(2 * M_PI * fs[k] * (double) i / SR); a.L.push_back(v); a.R.push_back(v); }
        SamplerSettings st; st.mode = 1; st.sliceBase = 60; st.slices = analysis::equalSlices(4); st.release = .01;
        for (int k = 0; k < 4; ++k) {
            Sampler s; s.prepare(SR); s.setPlayback(buildPlayback(a, st), st);
            auto o = runSampler(s, 60 + k, 1, .3, .35);
            double f = freqOf(o.L, 1000, 9000, SR);
            CHECK(std::fabs(f - fs[k]) / fs[k] < .01, "slice %d: %.1f Hz want %.1f", k, f, fs[k]);
            CHECK(rms(o.L, (size_t) (.27 * SR), (size_t) (.35 * SR)) < .005, "slice %d plays past its end", k);
        }
        Sampler s; s.prepare(SR); s.setPlayback(buildPlayback(a, st), st);
        auto o = runSampler(s, 64, 1, .3, .35); CHECK(rms(o.L, 0, o.L.size()) == 0, "key beyond the slices should be silent");
        // trim: start at .5 plays the 440 Hz part first
        SamplerSettings tr; tr.start = .5; tr.end = .75; tr.root = 60; tr.release = .01; Sampler t; t.prepare(SR); t.setPlayback(buildPlayback(a, tr), tr);
        o = runSampler(t, 60, 1, 1, 1); double f = freqOf(o.L, 1000, 9000, SR);
        CHECK(std::fabs(f - 440) < 3, "trim start: %.1f Hz", f);
        CHECK(rms(o.L, (size_t) (.26 * SR), (size_t) (.5 * SR)) < .005, "trim end not respected");
        // one-shot ignores the key release
        SamplerSettings os; os.oneShot = 1; os.root = 60; Sampler w; w.prepare(SR); w.setPlayback(buildPlayback(a, os), os);
        o = runSampler(w, 60, 1, .05, .9); CHECK(rms(o.L, (size_t) (.6 * SR), (size_t) (.7 * SR)) > .2, "one-shot stopped on release");
        // retrigger the same key many times quickly: voices stay bounded, no NaN
        Sampler q; SamplerSettings qs; qs.root = 60; qs.polyphony = 8; q.prepare(SR); q.setPlayback(buildPlayback(a, qs), qs);
        std::vector<float> L(64), R(64); bool fin = true;
        for (int i = 0; i < 400; ++i) { q.noteOn(60 + (i % 5), .8f); q.process(L.data(), R.data(), 64); for (float x : L) fin &= std::isfinite(x); }
        CHECK(fin && q.activeVoices() <= 8, "rapid retrigger: %d voices", q.activeVoices());
    }
    // ---- stretch keeps pitch, changes length
    {
        auto src = sine(440, 1); for (double r : {.5, 1.5, 2.0}) {
            auto out = dsp::stretch(src, r);
            CHECK(std::fabs((double) out.frames() - r * (double) src.frames()) < 2, "stretch %.1f length %zu", r, out.frames());
            double f = freqOf(out.L, out.frames() / 5, out.frames() * 4 / 5, SR);
            CHECK(std::fabs(f - 440) < 3, "stretch %.1f changed pitch: %.1f Hz", r, f);
            std::printf("stretch x%.1f: %zu frames, %.1f Hz\n", r, out.frames(), f);
        }
    }
    // ---- analysis on known signals
    {
        AudioData clicks; clicks.sr = SR; size_t n = (size_t) (12 * SR); clicks.L.assign(n, 0); clicks.R.assign(n, 0);
        for (double t = 0; t < 12; t += .5) { size_t i = (size_t) (t * SR); for (size_t k = 0; k < 600 && i + k < n; ++k) { float v = (float) (std::sin(2 * M_PI * 1000 * k / SR) * std::exp(-(double) k / 80)); clicks.L[i + k] = clicks.R[i + k] = v; } }
        auto an = analysis::analyse(clicks);
        CHECK(std::fabs(an.bpm - 120) < 1.5, "tempo %.1f (want 120)", an.bpm);
        CHECK(an.onsets.size() >= 20 && an.onsets.size() <= 26, "onsets %zu (want 24)", an.onsets.size());
        std::printf("analysis: tempo %.1f (conf %.2f), %zu transients\n", an.bpm, an.bpmConf, an.onsets.size());
        // A minor arpeggio-ish texture: A C E with some D and G
        AudioData ch; ch.sr = SR; n = (size_t) (8 * SR); ch.L.assign(n, 0); ch.R.assign(n, 0);
        int notes[] = {57, 60, 64, 69, 62, 67, 57, 64};
        for (size_t i = 0; i < n; ++i) { int nn = notes[(i / (size_t) SR) % 8]; double f = 440 * std::pow(2, (nn - 69) / 12.0);
            float v = (float) (.3 * std::sin(2 * M_PI * f * i / SR) + .25 * std::sin(2 * M_PI * 220 * i / SR) + .12 * std::sin(2 * M_PI * 261.63 * i / SR)); ch.L[i] = ch.R[i] = v; }
        an = analysis::analyse(ch);
        CHECK(an.keyTonic == 9 && an.keyMinor, "key %d %s (want A minor)", an.keyTonic, an.keyMinor ? "minor" : "major");
        std::printf("analysis: key %d %s (conf %.2f)\n", an.keyTonic, an.keyMinor ? "minor" : "major", an.keyConf);
        auto one = sine(220, 1.5); for (size_t i = 0; i < one.frames(); ++i) { float h = .15f * (float) std::sin(2 * M_PI * 440 * i / SR); one.L[i] += h; one.R[i] += h; }
        an = analysis::analyse(one);
        CHECK(an.rootNote == 57, "root note %d (want 57)", an.rootNote);
        std::printf("analysis: root %d %+.1f cents (conf %.2f)\n", an.rootNote, an.rootCents, an.rootConf);
        AudioData pad = sine(330, 2); for (size_t i = 0; i < 22050; ++i) pad.L[i] = pad.R[i] = 0;
        an = analysis::analyse(pad); CHECK(an.silenceStart >= 22000 && an.silenceStart <= 22100, "leading silence %zu", an.silenceStart);
        auto pk = analysis::peaks(pad, 10); CHECK(pk.size() == 20 && pk[0] == 0 && pk[19] > .4f, "peaks");
    }
    // ---- settings string round trip
    {
        SamplerSettings s; s.stretch = 1.25; s.reverse = 2; s.revA = .1; s.revB = .4; s.start = .05; s.end = .9; s.loop = 1; s.loopA = .3; s.loopB = .7;
        s.mode = 1; s.root = 48; s.tune = -.5; s.sliceBase = 36; s.slices = {.25, .5, .75}; s.gainDb = -3; s.oneShot = 1;
        auto t = SamplerSettings::fromString(s.toString());
        CHECK(t.toString() == s.toString(), "settings round trip:\n %s\n %s", s.toString().c_str(), t.toString().c_str());
    }
    // ---- layer mode inside the engine: piano only, sampler only, both
    {
        lumi::ParamMap pm; std::ifstream in("build/tmp/after-hours-ep.params"); std::string k; float v; while (in >> k >> v) pm.v[k] = v;
        auto src = sine(440, 2);
        auto render = [&](int mode) {
            Engine e; e.prepare((float) SR); e.loadPreset(pm, 120);
            SamplerSettings st; st.root = 69; e.sampler.setPlayback(buildPlayback(src, st), st); e.setLayer(mode, 1, 1);
            std::vector<float> L(44100), R(44100); e.noteOn(69, .9f); e.process(L.data(), R.data(), 44100); return L;
        };
        auto p = render(0), s = render(1), l = render(2);
        double rp = rms(p, 4410, 40000), rs = rms(s, 4410, 40000), rl = rms(l, 4410, 40000);
        double mix = 0; for (size_t i = 4410; i < 40000; ++i) mix += std::fabs(l[i] - (p[i] + s[i])); mix /= (40000 - 4410);
        CHECK(rp > 1e-3 && rs > 1e-3, "piano %.4f sampler %.4f", rp, rs);
        CHECK(mix < .02 * (rp + rs), "layer is not piano + sampler (diff %.5f)", mix);
        std::printf("layer: piano rms %.3f, sampler rms %.3f, layer rms %.3f, layer-(piano+sampler) mean diff %.5f\n", rp, rs, rl, mix);
    }
    std::printf("\n%d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}

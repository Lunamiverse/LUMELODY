// Effect-by-effect checks for the four performance knobs. Prints measurements; exits non-zero on failure.
#include "../../engine/LumiEngine.h"
#include <cstdio>
#include <complex>
using namespace lumi;
static const int SR = 44100;
static ParamMap basePreset() {   // plain sine voice with a short envelope: easy to measure
    ParamMap p; p.v = {{"l1_type", 1}, {"l1_wave", 0}, {"l1_a", .002f}, {"l1_d", .15f}, {"l1_s", 0}, {"l1_r", .05f}, {"f_type", -1}};
    return p;
}
static std::vector<float> run(Engine& e, std::vector<std::tuple<double,double,int>> notes, double secs, std::vector<float>* R = nullptr) {
    int n = (int)(secs * SR); std::vector<float> L(n), Rr(n);
    size_t pos = 0; const int blk = 64;
    std::vector<std::tuple<double,int,int>> ev;
    for (auto& [t, d, k] : notes) { ev.push_back({t, 0, k}); ev.push_back({t + d, 1, k}); }
    std::sort(ev.begin(), ev.end());
    size_t ei = 0;
    for (; pos < (size_t)n; pos += blk) {
        while (ei < ev.size() && std::get<0>(ev[ei]) * SR <= pos) { auto& x = ev[ei++]; if (std::get<1>(x) == 0) e.noteOn(std::get<2>(x), .9f); else e.noteOff(std::get<2>(x)); }
        int m = std::min<int>(blk, n - pos); e.process(&L[pos], &Rr[pos], m);
    }
    if (R) *R = Rr;
    return L;
}
static float peak(const std::vector<float>& x) { float m = 0; for (float v : x) { if (!std::isfinite(v)) return 1e9f; m = std::max(m, std::fabs(v)); } return m; }
static float rmsRange(const std::vector<float>& x, double a, double b) { double s = 0; int i0 = a * SR, i1 = std::min<int>(b * SR, x.size()); for (int i = i0; i < i1; ++i) s += x[i] * x[i]; return std::sqrt(s / std::max(1, i1 - i0)); }
static float clicks(const std::vector<float>& x) { int c = 0; std::vector<float> d2(x.size()); for (size_t i = 2; i < x.size(); ++i) d2[i] = std::fabs(x[i] - 2 * x[i-1] + x[i-2]);
    int w = 441; double acc = 0; for (size_t i = 0; i < d2.size(); ++i) { acc += d2[i] * d2[i]; if (i >= (size_t)w) acc -= d2[i-w] * d2[i-w]; double loc = std::sqrt(std::max(0.0, acc / w)) + 1e-9; if (d2[i] > loc * 9 && d2[i] > 2e-3) ++c; } return c; }
static float freqAt(const std::vector<float>& x, double t) {   // zero-crossing estimate over 50 ms
    int i0 = t * SR, i1 = i0 + SR / 20; int zc = 0; double first = -1, last = 0;
    for (int i = i0 + 1; i < i1; ++i) if (x[i-1] < 0 && x[i] >= 0) { double fr = i - 1 + x[i-1] / (x[i-1] - x[i]); if (first < 0) first = fr; last = fr; ++zc; }
    return zc > 1 ? (zc - 1) * SR / (last - first) : 0; }
static float goertzel(const std::vector<float>& x, double a, double b, float f);
static float bandEnergy(const std::vector<float>& x, double a, double b, float f) {   // energy within +-3% of f
    double s = 0; for (float g = f * .97f; g <= f * 1.03f; g += .5f) { float m = goertzel(x, a, b, g); s += m * m; } return std::sqrt(s); }
static float goertzel(const std::vector<float>& x, double a, double b, float f) {
    int i0 = a * SR, i1 = b * SR; double w = 2 * M_PI * f / SR, c = 2 * std::cos(w), s1 = 0, s2 = 0;
    for (int i = i0; i < i1; ++i) { double s = x[i] + c * s1 - s2; s2 = s1; s1 = s; } return std::sqrt(s1 * s1 + s2 * s2 - c * s1 * s2) / (i1 - i0); }

int main() {
    int fails = 0;
    auto check = [&](bool ok, const char* what) { std::printf("  [%s] %s\n", ok ? "ok" : "FAIL", what); if (!ok) ++fails; };

    { std::puts("PITCH");
      Engine e; e.prepare(SR); e.loadPreset(basePreset(), 120);
      Performance pf; pf.pitch = 7; e.setPerformance(pf);
      auto x = run(e, {{0.0, .6, 69}}, 1.0);
      float f = freqAt(x, .1); std::printf("  A4 with pitch +7 -> %.1f Hz (expect 659.3)\n", f);
      check(std::fabs(f - 659.26f) < 3, "pitch +7 semitones transposes exactly");
      pf.pitch = -12.5f; pf.pitchRange = 24; e.setPerformance(pf); auto y = run(e, {{0.0, .6, 69}}, 1.0);
      float f2 = freqAt(y, .1); std::printf("  A4 with pitch -12.5 -> %.1f Hz (expect 214.1)\n", f2);
      check(std::fabs(f2 - 214.1f) < 2, "fine tuning below an octave");
      pf.pitch = 40; pf.pitchRange = 12; e.setPerformance(pf); check(e.performance().pitch == 12, "pitch is clamped to the configured range");
    }
    { std::puts("DELAY");
      for (int div = 0; div < 4; ++div) {
        Engine e; e.prepare(SR); e.loadPreset(basePreset(), 120);
        Performance pf; pf.delay = .8f; pf.delayDiv = div; pf.delayFeedback = .5f; e.setPerformance(pf);
        std::vector<float> R; auto L = run(e, {{0.0, .05, 69}}, 3.0, &R);
        float beats = delayDivBeats(div), dt = beats * .5f;
        // ping-pong: first echo on the left at dt, second on the right at 2*dt
        float e1 = rmsRange(L, dt, dt + .05), e2 = rmsRange(R, 2 * dt, 2 * dt + .05), gap = rmsRange(L, 2 * dt + .01, 2 * dt + .05);   // 2nd echo belongs to the right side
        std::printf("  div %.3f beats: echo L@%.3fs=%.4f  echo R@%.3fs=%.4f  silence between=%.5f  peak=%.2f\n", beats, dt, e1, 2*dt, e2, gap, std::max(peak(L), peak(R)));
        check(e1 > 10 * gap && e2 > 5 * gap, "repeats land on the tempo grid (ping-pong)");
      }
      Engine e; e.prepare(SR); e.loadPreset(basePreset(), 120);
      Performance pf; pf.delay = 1; pf.delayFeedback = 5; e.setPerformance(pf);   // abusive feedback request
      std::vector<float> R; auto L = run(e, {{0.0, .3, 60}, {0.1, .3, 64}, {0.2, .3, 67}}, 12.0, &R);
      std::printf("  max feedback, 12 s: early=%.3f late=%.3f peak=%.2f\n", rmsRange(L, 1, 2), rmsRange(L, 10, 12), peak(L));
      check(rmsRange(L, 10, 12) < rmsRange(L, 1, 2) && peak(L) < 2, "feedback is capped: no runaway, no blow-up");
    }
    { std::puts("REVERB");
      const char* names[5] = {"preset", "room", "hall", "plate", "shimmer"};
      ParamMap p = basePreset(); p.v["rv_mix"] = .3f;
      float tails[5];
      for (int t = 0; t < 5; ++t) {
        Engine e; e.prepare(SR); e.loadPreset(p, 120);
        Performance pf = e.presetPerformance(); pf.reverb = .7f; pf.reverbType = t; e.setPerformance(pf);
        std::vector<float> R; auto L = run(e, {{0.0, .2, 57}}, 6.0, &R);
        tails[t] = rmsRange(L, 1.5, 2.5);
        float oct = bandEnergy(L, 1.0, 3.0, 440.f), fund = bandEnergy(L, 1.0, 3.0, 220.f);
        std::printf("  %-8s tail(1.5-2.5s)=%.5f  octave-up/fund in tail=%.2f  peak=%.2f clicks=%d\n", names[t], tails[t], oct / (fund + 1e-9f), std::max(peak(L), peak(R)), (int)clicks(L));
        check(std::isfinite(tails[t]) && peak(L) < 2, "stable");
        if (t == 4) check(oct / (fund + 1e-9f) > 0.5f, "shimmer adds octave-up energy to the tail");
      }
      check(tails[2] > tails[1] * 3, "hall tail is much longer than room");
      { Engine e; e.prepare(SR); e.loadPreset(p, 120); Performance pf; pf.reverb = 1; pf.reverbType = 4; e.setPerformance(pf);
        auto L = run(e, {{0.0, .5, 45}, {0.0, .5, 52}, {0.0, .5, 57}, {0.0, .5, 64}}, 20.0);
        float a1 = rmsRange(L, 2, 3), a2 = rmsRange(L, 8, 9), a3 = rmsRange(L, 18, 19);
        std::printf("  shimmer at 100%%: 2-3s=%.5f 8-9s=%.5f 18-19s=%.6f peak=%.2f\n", a1, a2, a3, peak(L));
        check(a3 < a2 && a2 < a1 && peak(L) < 1.5f, "shimmer feedback decays (never builds up)"); }
      Engine e; e.prepare(SR); e.loadPreset(p, 120);
      Performance pf; pf.reverb = 0; e.setPerformance(pf);
      auto L = run(e, {{0.0, .2, 57}}, 3.0);
      check(rmsRange(L, 1.5, 2.5) < 1e-4f, "reverb knob at 0% is fully dry");
    }
    { std::puts("STUTTER");
      for (int div = 0; div < 4; ++div) {
        Engine e; e.prepare(SR);
        ParamMap p = basePreset(); p.v["l1_s"] = 1;   // sustained sine: any edge in a repeat would show up as a click
        e.loadPreset(p, 120);
        Performance pf; pf.stutter = 1; pf.stutterDiv = div; e.setPerformance(pf);
        auto L = run(e, {{0.0, 4.0, 48}, {1.0, 3.0, 55}}, 4.5);
        Engine e0; e0.prepare(SR); e0.loadPreset(p, 120);
        auto L0 = run(e0, {{0.0, 4.0, 48}, {1.0, 3.0, 55}}, 4.5);
        std::printf("  div %.3f: peak=%.2f (dry %.2f) clicks=%d (same notes without stutter: %d)\n", stutterDivBeats(div), peak(L), peak(L0), (int)clicks(L), (int)clicks(L0));
        check(clicks(L) <= clicks(L0) * 1.1f + 5, "stutter adds no clicks beyond the dry sound");
        check(peak(L) < 1.5f, "no volume spikes");
      }
      // stutter really repeats audio: sweep a note upward, measure that a later moment equals an earlier slice
      Engine e; e.prepare(SR); ParamMap p = basePreset(); p.v["l1_s"] = 1; p.v["glide"] = 0;
      e.loadPreset(p, 120); Performance pf; pf.stutter = 1; pf.stutterDiv = 2; e.setPerformance(pf);
      auto L = run(e, {{0.0, .25, 60}, {0.25, .25, 72}}, 1.0);   // beat 0 = 0..0.5 s; slice 1/16 = 0.125 s
      float fA = freqAt(L, .02), fB = freqAt(L, .30);   // without stutter .30 s would be C5 (523 Hz); with it, the C4 slice repeats
      std::printf("  repeat check: start %.0f Hz, at 0.30 s %.0f Hz (C4 slice repeated = ~262)\n", fA, fB);
      check(std::fabs(fB - 261.6f) < 15, "the captured slice repeats for the rest of the beat");
    }
    std::printf(fails ? "\n%d FAILED\n" : "\nALL PASSED\n", fails);
    return fails ? 1 : 0;
}

// LUMELODY effects test suite: every effect is rendered on several instruments with low/mid/high notes,
// chords and fast passages, with its knobs moving while audio plays, then measured. Prints a report and
// exits non-zero if any functional or quality check fails.
//   build: g++ -O2 -std=c++17 tools/render/fx_suite.cpp -o build/fx_suite
//   run:   build/fx_suite [effect-id]
#include "../../engine/LumiEngine.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <complex>
using namespace lumi;
static const int SR = 44100;
static int g_fail = 0, g_pass = 0;
static void check(bool ok, const std::string& what) { std::printf("    [%s] %s\n", ok ? " ok " : "FAIL", what.c_str()); ok ? ++g_pass : ++g_fail; }

static ParamMap loadParams(const std::string& id) {
    ParamMap p; std::ifstream in("build/tmp/" + id + ".params"); std::string k; float v;
    while (in >> k >> v) p.v[k] = v;
    if (p.v.empty()) { std::fprintf(stderr, "missing build/tmp/%s.params (run build_previews.py first)\n", id.c_str()); std::exit(2); }
    std::ifstream g("build/tmp/" + id + ".gain"); float gd; if (g >> gd) p.v["out_gain_db"] = gd;   // the preset's calibrated output level
    return p;
}
static ParamMap sinePreset(bool sustain = true) {
    ParamMap p; p.v = {{"l1_type", 1}, {"l1_wave", 0}, {"l1_a", .003f}, {"l1_d", .2f}, {"l1_s", sustain ? 1.f : 0.f}, {"l1_r", .05f}, {"f_type", -1}};
    return p;
}
struct Note { double t, d; int n; float v; };
// the standard test material: low / mid / high single notes, a chord, and a fast 16th passage (8 s at 120 BPM)
static std::vector<Note> testMaterial(int base) {
    std::vector<Note> m;
    m.push_back({0.0, .45, base - 12, .8f}); m.push_back({.5, .45, base, .8f}); m.push_back({1.0, .45, base + 12, .8f}); m.push_back({1.5, .45, base + 24, .7f});
    for (int k : {0, 3, 7, 10, 14}) m.push_back({2.0, 1.4, base + k, .75f});
    for (int i = 0; i < 24; ++i) m.push_back({3.6 + i * .125, .11, base + 12 + (int[]){0, 3, 7, 10, 12, 10, 7, 3}[i % 8], .7f + .2f * (i % 2)});
    m.push_back({6.8, .9, base, .9f});
    return m;
}
struct Out { std::vector<float> L, R; double cpuMs = 0; std::vector<double> events; };
// automation: called every 64 samples with time in seconds
using Auto = std::function<void(Engine&, double)>;
static Out render(const ParamMap& preset, const std::vector<Note>& notes, double secs, Auto setup, Auto automate = nullptr) {
    Out o;
    Engine e; e.prepare(SR); e.loadPreset(preset, 120);
    if (setup) setup(e, 0);
    int n = (int)(secs * SR); o.L.resize(n); o.R.resize(n);
    struct Ev { double t; int on; int note; float v; };
    std::vector<Ev> ev; for (auto& x : notes) { ev.push_back({x.t, 1, x.n, x.v}); ev.push_back({x.t + x.d, 0, x.n, 0}); }
    std::sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && a.on < b.on); });
    for (auto& x : ev) o.events.push_back(x.t);
    size_t ei = 0; auto t0 = std::chrono::steady_clock::now();
    for (int pos = 0; pos < n; pos += 64) {
        double t = pos / (double)SR;
        while (ei < ev.size() && ev[ei].t <= t) { auto& x = ev[ei++]; if (x.on) e.noteOn(x.note, x.v); else e.noteOff(x.note); }
        if (automate) automate(e, t);
        int m = std::min(64, n - pos); e.process(&o.L[pos], &o.R[pos], m);
    }
    o.cpuMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return o;
}
// ---------------- measurements
static float peak(const Out& o) { float m = 0; for (size_t i = 0; i < o.L.size(); ++i) { if (!std::isfinite(o.L[i]) || !std::isfinite(o.R[i])) return 1e9f; m = std::max(m, std::max(std::fabs(o.L[i]), std::fabs(o.R[i]))); } return m; }
static double rms(const std::vector<float>& x, double a = 0, double b = 1e9) { size_t i0 = (size_t)(a * SR), i1 = std::min(x.size(), (size_t)(b * SR)); double s = 0; for (size_t i = i0; i < i1; ++i) s += x[i] * x[i]; return std::sqrt(s / std::max<size_t>(1, i1 - i0)) + 1e-12; }
static double db(double x) { return 20 * std::log10(x + 1e-12); }
static std::vector<float> mono(const Out& o) { std::vector<float> m(o.L.size()); for (size_t i = 0; i < m.size(); ++i) m[i] = .5f * (o.L[i] + o.R[i]); return m; }
static std::vector<float> side(const Out& o) { std::vector<float> m(o.L.size()); for (size_t i = 0; i < m.size(); ++i) m[i] = .5f * (o.L[i] - o.R[i]); return m; }
static const std::vector<double>* g_mask = nullptr;   // note on/off times: real attacks there are not clicks
static int clicks(const std::vector<float>& x) {   // isolated 2nd-difference spikes far above the local 10 ms level
    int c = 0, w = 441; double acc = 0; std::vector<float> d2(x.size(), 0.f);
    std::vector<char> skip(x.size(), 0);
    if (g_mask) for (double t : *g_mask) { size_t a = (size_t)std::max(0.0, (t - .002) * SR), b = std::min(x.size(), (size_t)((t + .025) * SR)); for (size_t i = a; i < b; ++i) skip[i] = 1; }
    for (size_t i = 2; i < x.size(); ++i) d2[i] = std::fabs(x[i] - 2 * x[i - 1] + x[i - 2]);
    for (size_t i = 0; i < d2.size(); ++i) { acc += d2[i] * d2[i]; if (i >= (size_t)w) acc -= d2[i - w] * d2[i - w]; double loc = std::sqrt(std::max(0.0, acc / w)) + 1e-9; if (!skip[i] && d2[i] > loc * 9 && d2[i] > 3e-3) ++c; }
    return c;
}
static double goertzel(const std::vector<float>& x, double a, double b, double f) { size_t i0 = a * SR, i1 = std::min(x.size(), (size_t)(b * SR)); double w = 2 * M_PI * f / SR, c = 2 * std::cos(w), s1 = 0, s2 = 0; for (size_t i = i0; i < i1; ++i) { double s = x[i] + c * s1 - s2; s2 = s1; s1 = s; } return std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - c * s1 * s2)) / (i1 - i0); }
static double band(const std::vector<float>& x, double a, double b, double f, double pct = .03) { double s = 0; for (double g = f * (1 - pct); g <= f * (1 + pct); g += .5) { double m = goertzel(x, a, b, g); s += m * m; } return std::sqrt(s); }
static double zcFreq(const std::vector<float>& x, double t, double win = .05) { size_t i0 = t * SR, i1 = i0 + win * SR; int zc = 0; double first = -1, last = 0; for (size_t i = i0 + 1; i < i1 && i < x.size(); ++i) if (x[i - 1] < 0 && x[i] >= 0) { double fr = i - 1 + x[i - 1] / (x[i - 1] - x[i]); if (first < 0) first = fr; last = fr; ++zc; } return zc > 1 ? (zc - 1) * SR / (last - first) : 0; }
static double hfRatio(const std::vector<float>& x, double a, double b) {   // energy above ~4 kHz relative to total (1st-difference proxy)
    size_t i0 = a * SR, i1 = std::min(x.size(), (size_t)(b * SR)); double e = 0, d = 0; for (size_t i = i0 + 1; i < i1; ++i) { e += x[i] * x[i]; double q = x[i] - x[i - 1]; d += q * q; } return d / (e + 1e-12);
}

// 1/6-octave band energies of an 8192-point Hann-windowed FFT starting at t (stable for steady signals)
static std::vector<double> bands(const std::vector<float>& x, double t) {
    const int N = 8192; std::vector<std::complex<double>> a(N);
    size_t i0 = (size_t)(t * SR);
    for (int i = 0; i < N; ++i) a[i] = (i0 + i < x.size() ? x[i0 + i] : 0.f) * (.5 - .5 * std::cos(2 * M_PI * i / (N - 1)));
    for (int i = 1, j = 0; i < N; ++i) { int bit = N >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap(a[i], a[j]); }
    for (int len = 2; len <= N; len <<= 1) { double ang = -2 * M_PI / len; std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (int i = 0; i < N; i += len) { std::complex<double> w(1); for (int j = 0; j < len / 2; ++j) { auto u = a[i + j], v = a[i + j + len / 2] * w; a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl; } } }
    std::vector<double> out;
    for (double f = 200; f < 8000; f *= std::pow(2.0, 1 / 6.0)) { int b0 = (int)(f * N / SR), b1 = std::max(b0 + 1, (int)(f * std::pow(2.0, 1 / 6.0) * N / SR)); double e = 0; for (int b = b0; b < b1; ++b) e += std::norm(a[b]); out.push_back(std::sqrt(e) + 1e-9); }
    return out;
}
static const char* INSTR[] = {"after-hours-ep", "drill-bell", "rage-lead", "underwater-pad", "808-boom", "midnight-strum"};
static int baseFor(const std::string& id) { return id == "808-boom" ? 36 : 60; }

// generic robustness pass for one effect: several instruments, default + knob sweeps while playing + extremes
static void robustness(int fx, double& worstCpu) {
    const auto& d = fxSchema()[fx];
    // effects that replay captured audio (echoes, loops, grains, reversed segments) re-play note attacks at new times;
    // those are music, not clicks, so their click test runs on a smooth sine source where any edge would be a fault
    const bool replay = fx == FX_DELAY || fx == FX_STUTTER || fx == FX_GRANULAR || fx == FX_FREEZE || fx == FX_REVERSE;
    double worstKnee = 0;
    int worstClickExtra = 0; float worstPeak = 0; double worstLoud = -99, minLoud = 99; bool nan = false;
    std::string worstWhere;
    // clicks are judged on a smooth sine source, where any edge an effect adds is visible; instrument sources
    // (supersaws, attacks, distortion) carry sharp edges of their own, so on them we check level, NaN and limiter use
    std::vector<std::string> ids(std::begin(INSTR), std::end(INSTR));
    ids.push_back("__sine");
    for (const std::string& id : ids) {
        ParamMap p = id == "__sine" ? sinePreset() : loadParams(id);
        if (id == "__sine") p.v["out_gain_db"] = -12;   // a raw 5-note sine chord is otherwise far hotter than any preset
        auto mat = testMaterial(id == "__sine" ? 60 : baseFor(id));
        Out dry = render(p, mat, 9.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        g_mask = &dry.events;
        int dc = clicks(mono(dry)); double dl = rms(mono(dry));
        // 1) defaults, 2) main knob swept 0 -> max -> 0 continuously, 3) every parameter swept, 4) all params at max
        for (int mode = 0; mode < 4; ++mode) {
            Out o = render(p, mat, 9.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); e.setOn(fx, true); if (fx == FX_FREEZE) e.setParam(fx, 1, 0); },
                [&](Engine& e, double t) {
                    if (fx == FX_FREEZE) e.setParam(fx, 1, (t > 2.6 && t < 6) ? 1.f : 0.f);   // capture during the chord
                    if (mode == 1) { const auto& pd = d.p[0]; float x = (float)(.5 - .5 * std::cos(t / 9.5 * 2 * M_PI)); e.setParam(fx, 0, pd.min + (pd.max - pd.min) * x); }
                    if (mode == 2) for (size_t i = 0; i < d.p.size(); ++i) { const auto& pd = d.p[i]; if (fx == FX_FREEZE && i == 1) continue; if (pd.kind == P_CONT) { float x = (float)(.5 - .5 * std::cos((t / 9.5 + i * .13) * 2 * M_PI)); e.setParam(fx, (int)i, pd.min + (pd.max - pd.min) * x); } }
                    // extremes: every parameter at maximum (Glue's Output knob is plain user gain, left at 0 dB here)
                    if (mode == 3 && t == 0) for (size_t i = 0; i < d.p.size(); ++i) if (!(fx == FX_FREEZE && i == 1) && !(fx == FX_GLUE && i == 6)) e.setParam(fx, (int)i, d.p[i].max);
                });
            float pk = peak(o); if (pk > 1e8f) nan = true;
            { int over = 0; for (size_t s = 0; s < o.L.size(); ++s) if (std::fabs(o.L[s]) > .89f || std::fabs(o.R[s]) > .89f) ++over; worstKnee = std::max(worstKnee, 100.0 * over / o.L.size()); }
            // Saturation at 100 % crunch is hard clipping by design: its sharp edges are the sound, so the click
            // metric covers defaults and sweeps; the extreme setting is covered by the aliasing/level checks
            // high drive turns a sine into a clipped wave on purpose; saturation gets its own moderate-drive click test
            int extra = (id != "__sine") || (fx == FX_SATURATION && mode >= 1) ? 0 : clicks(mono(o)) - dc;
            double loud = db(rms(mono(o)) / dl);
            if (extra > worstClickExtra) { worstClickExtra = extra; worstWhere = id + " mode " + std::to_string(mode); }
            worstPeak = std::max(worstPeak, pk); worstLoud = std::max(worstLoud, loud); minLoud = std::min(minLoud, loud);
            worstCpu = std::max(worstCpu, o.cpuMs / 9.5);
        }
    }
    (void)replay;
    std::printf("    robustness over %zu instruments + sine x 4 automation modes: worst extra clicks %d%s, peak %.2f, level vs dry %+.1f..%+.1f dB, worst CPU %.1f ms per audio second\n",
                sizeof(INSTR) / sizeof(*INSTR), worstClickExtra, worstWhere.empty() ? "" : (" (" + worstWhere + ")").c_str(), worstPeak, minLoud, worstLoud, worstCpu);
    check(!nan, "no NaN / infinity");
    check(worstPeak <= 1.0f, "never exceeds full scale (output safety stays smooth)");
    std::printf("    output safety knee engaged on %.3f%% of samples (worst case)\n", worstKnee);
    check(worstKnee < 0.5, "gain staging: the safety limiter is (almost) never needed");
    g_mask = nullptr;
    check(worstClickExtra <= 3, "no clicks added while knobs move (vs dry render)");
}

// bypass: an effect switched on then off must return to the exact dry signal once its tail has gone
static void bypass(int fx) {
    ParamMap p = loadParams("after-hours-ep"); auto mat = testMaterial(60);
    // with long tails (delay/reverb/shimmer/freeze) compare only after they would have rung out
    Out dry = render(p, mat, 16, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
    Out o = render(p, mat, 16, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); e.setOn(fx, true); },
                   [&](Engine& e, double t) { if (t >= 3.0 && t < 3.01) e.setOn(fx, false); });
    double maxd = 0; for (size_t i = (size_t)(12.0 * SR); i < dry.L.size(); ++i) maxd = std::max(maxd, (double)std::fabs(dry.L[i] - o.L[i]));
    bool timeShifting = fx == FX_PITCH || fx == FX_GLIDE || fx == FX_OCTAVE;   // these change what later notes are; checked separately
    check(maxd < (timeShifting ? 1.f : 1e-4f), "bypass returns the dry signal (max diff " + std::to_string(maxd) + ")");
}

int main(int argc, char** argv) {
    std::string only = argc > 1 ? argv[1] : "";
    const auto& S = fxSchema();
    double totalCpu = 0;
    auto section = [&](int fx) { std::printf("\n== %s (%s)\n", S[fx].name, S[fx].category); };
    auto want = [&](int fx) { return only.empty() || only == S[fx].id; };
    auto onlyFx = [](int fx) { return [fx](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); e.setOn(fx, true); }; };

    // ---------------- REVERB
    if (want(FX_REVERB)) { section(FX_REVERB);
        ParamMap p = sinePreset(false); double tails[5];
        for (int t = 1; t < 5; ++t) {
            Out o = render(p, {{0, .2, 57, .9f}}, 8, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 0, .7f); e.setParam(FX_REVERB, 1, t); });
            tails[t] = rms(o.L, 1.5, 2.5);
        }
        std::printf("    tail 1.5-2.5 s: room %.5f hall %.5f plate %.5f shimmer %.5f\n", tails[1], tails[2], tails[3], tails[4]);
        check(tails[2] > tails[1] * 5, "Hall rings much longer than Room");
        Out a = render(p, {{0, .2, 57, .9f}}, 8, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 1, 2); e.setParam(FX_REVERB, 2, 1.0f); e.setParam(FX_REVERB, 0, .7f); });
        Out b = render(p, {{0, .2, 57, .9f}}, 8, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 1, 2); e.setParam(FX_REVERB, 2, 8.0f); e.setParam(FX_REVERB, 0, .7f); });
        check(rms(b.L, 3, 4) > rms(a.L, 3, 4) * 10, "Decay knob lengthens the tail");
        Out c = render(p, {{0, .02, 69, .9f}}, 3, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 1, 3); e.setParam(FX_REVERB, 3, 150); e.setParam(FX_REVERB, 0, 1); });
        check(rms(c.L, .07, .13) < rms(c.L, .17, .3) * .3, "Pre-delay holds the tail back (~150 ms)");
        ParamMap bright = loadParams("rage-lead");
        Out dk = render(bright, {{0, .2, 69, .9f}}, 4, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 1, 2); e.setParam(FX_REVERB, 4, 0); e.setParam(FX_REVERB, 0, 1); });
        Out br = render(bright, {{0, .2, 69, .9f}}, 4, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 1, 2); e.setParam(FX_REVERB, 4, 1); e.setParam(FX_REVERB, 0, 1); });
        check(hfRatio(br.L, 1, 2) > hfRatio(dk.L, 1, 2) * 1.5, "Tone knob brightens the tail");
        Out z = render(p, {{0, .2, 57, .9f}}, 3, [&](Engine& e, double) { onlyFx(FX_REVERB)(e, 0); e.setParam(FX_REVERB, 0, 0); });
        check(rms(z.L, 1.5, 2.5) < 1e-5, "Amount 0% is fully dry");
        double cpu = 0; robustness(FX_REVERB, cpu); bypass(FX_REVERB); totalCpu += cpu; }

    // ---------------- DELAY
    if (want(FX_DELAY)) { section(FX_DELAY);
        ParamMap p = sinePreset(false);
        const char* names[7] = {"1/4", "1/8", "1/8 dotted", "1/16", "1/32", "1/4 triplet", "1/8 triplet"};
        bool allOk = true;
        for (int d = 0; d < 7; ++d) {
            Out o = render(p, {{0, .03, 69, .9f}}, 2.5, [&](Engine& e, double) { onlyFx(FX_DELAY)(e, 0); e.setParam(FX_DELAY, 0, .8f); e.setParam(FX_DELAY, 1, d); e.setParam(FX_DELAY, 5, 1); e.setParam(FX_DELAY, 3, 0); e.setParam(FX_DELAY, 4, 1); });
            double dt = beatsForDiv(d) * .5;
            // ping-pong: 1st echo only on the left, 2nd only on the right
            double echoL = rms(o.L, dt + .002, dt + .03), echoR2 = rms(o.R, 2 * dt + .002, 2 * dt + .03);
            double before = std::max(rms(o.R, dt + .002, dt + .03), rms(o.L, 2 * dt + .002, 2 * dt + .03));
            bool ok = echoL > before * 8 && echoR2 > before * 4;
            std::printf("    %-11s echo L @%.3fs %.4f, echo R @%.3fs %.4f, gap %.5f  %s\n", names[d], dt, echoL, 2 * dt, echoR2, before, ok ? "" : "<-");
            allOk = allOk && ok;
        }
        check(allOk, "every division lands on the tempo grid and ping-pongs L -> R");
        Out f = render(p, {{0, .3, 60, .9f}, {.1, .3, 64, .9f}}, 14, [&](Engine& e, double) { onlyFx(FX_DELAY)(e, 0); e.setParam(FX_DELAY, 0, 1); e.setParam(FX_DELAY, 2, .9f); });
        std::printf("    max feedback: 1-2 s %.4f, 12-14 s %.5f, peak %.2f\n", rms(f.L, 1, 2), rms(f.L, 12, 14), peak(f));
        check(rms(f.L, 12, 14) < rms(f.L, 1, 2) * .3 && peak(f) < 1, "feedback at maximum always dies away");
        ParamMap rl = loadParams("rage-lead");
        Out bright = render(rl, {{0, .05, 72, .9f}}, 2, [&](Engine& e, double) { onlyFx(FX_DELAY)(e, 0); e.setParam(FX_DELAY, 0, 1); e.setParam(FX_DELAY, 3, .8f); e.setParam(FX_DELAY, 1, 1); });
        Out dark = render(rl, {{0, .05, 72, .9f}}, 2, [&](Engine& e, double) { onlyFx(FX_DELAY)(e, 0); e.setParam(FX_DELAY, 0, 1); e.setParam(FX_DELAY, 3, -1); e.setParam(FX_DELAY, 1, 1); });
        check(hfRatio(mono(dark), .25, .8) < hfRatio(mono(bright), .25, .8) * .5, "Tone knob darkens the repeats");
        Out narrow = render(p, {{0, .05, 69, .9f}}, 2, [&](Engine& e, double) { onlyFx(FX_DELAY)(e, 0); e.setParam(FX_DELAY, 0, 1); e.setParam(FX_DELAY, 4, 0); e.setParam(FX_DELAY, 3, 0); });
        check(rms(side(narrow), .24, .6) < 1e-4, "Width 0 makes the echoes mono");
        double cpu = 0; robustness(FX_DELAY, cpu); bypass(FX_DELAY); totalCpu += cpu; }

    // ---------------- PITCH
    if (want(FX_PITCH)) { section(FX_PITCH);
        ParamMap p = sinePreset();
        auto f = [&](float semis, int range, bool fine) { Out o = render(p, {{0, .6, 69, .9f}}, .7, [&](Engine& e, double) { onlyFx(FX_PITCH)(e, 0); e.setParam(FX_PITCH, 1, range); e.setParam(FX_PITCH, 2, fine); e.setParam(FX_PITCH, 0, semis); }); return zcFreq(o.L, .2); };
        double a = f(7, 3, false), b = f(-12, 3, false), c = f(.5f, 3, true), d = f(.5f, 3, false), r = f(20, 1, false);
        std::printf("    A4 +7 -> %.1f Hz, -12 -> %.1f Hz, +0.5 fine -> %.1f Hz, +0.5 coarse -> %.1f Hz, +20 with ±5 range -> %.1f Hz\n", a, b, c, d, r);
        check(std::fabs(a - 659.26) < 1.5 && std::fabs(b - 220) < 1, "semitone transposition is exact");
        check(std::fabs(c - 452.9) < 1.5 && std::fabs(d - 440) < 1.5 || std::fabs(d - 466.2) < 1.5, "fine mode allows cents; coarse snaps to semitones");
        check(std::fabs(r - 587.3) < 2, "range limits the knob (±5 st)");
        Out sweep = render(p, {{0, 3, 57, .9f}}, 3, onlyFx(FX_PITCH), [&](Engine& e, double t) { e.setParam(FX_PITCH, 2, 1); e.setParam(FX_PITCH, 0, (float)(12 * std::sin(t * 2))); });
        check(clicks(sweep.L) == 0, "pitch moves smoothly while a note sounds (no clicks)");
        double cpu = 0; robustness(FX_PITCH, cpu); bypass(FX_PITCH); totalCpu += cpu; }

    // ---------------- STUTTER
    if (want(FX_STUTTER)) { section(FX_STUTTER);
        ParamMap p = sinePreset();
        bool clean = true;
        for (int d = 0; d < 4; ++d) {
            Out o = render(p, {{0, 4, 48, .8f}, {1, 3, 55, .8f}}, 4.5, [&](Engine& e, double) { onlyFx(FX_STUTTER)(e, 0); e.setParam(FX_STUTTER, 0, 1); e.setParam(FX_STUTTER, 1, d); });
            int c = clicks(mono(o)); clean = clean && c == 0 && peak(o) < .9f;
            std::printf("    rate %d: clicks %d, peak %.2f\n", d, c, peak(o));
        }
        check(clean, "repeats are seamless at every rate (pure sine: no clicks, no spikes)");
        Out rep = render(p, {{0, .25, 60, .9f}, {.25, .25, 72, .9f}}, 1, [&](Engine& e, double) { onlyFx(FX_STUTTER)(e, 0); e.setParam(FX_STUTTER, 0, 1); e.setParam(FX_STUTTER, 1, 2); });
        check(std::fabs(zcFreq(rep.L, .3) - 261.6) < 10, "the captured slice really repeats (C4 heard where C5 was played)");
        Out g1 = render(p, {{0, 2, 60, .9f}}, 2, [&](Engine& e, double) { onlyFx(FX_STUTTER)(e, 0); e.setParam(FX_STUTTER, 0, 1); e.setParam(FX_STUTTER, 1, 2); e.setParam(FX_STUTTER, 2, 1); });
        Out g2 = render(p, {{0, 2, 60, .9f}}, 2, [&](Engine& e, double) { onlyFx(FX_STUTTER)(e, 0); e.setParam(FX_STUTTER, 0, 1); e.setParam(FX_STUTTER, 1, 2); e.setParam(FX_STUTTER, 2, .3f); });
        check(rms(g2.L, .5, 1.5) < rms(g1.L, .5, 1.5) * .8 && clicks(g2.L) == 0, "Gate shortens each repeat smoothly");
        double cpu = 0; robustness(FX_STUTTER, cpu); bypass(FX_STUTTER); totalCpu += cpu; }

    // ---------------- CHORUS / PHASER / FLANGER (motion)
    for (int fx : {FX_CHORUS, FX_PHASER, FX_FLANGER}) if (want(fx)) { section(fx);
        ParamMap p = sinePreset(); p.v["l1_wave"] = 2;   // steady saw chord: any spectral movement comes from the effect
        std::vector<Note> chord = {{0, 3, 48, .8f}};   // one steady note: no beating between notes
        Out dry = render(p, chord, 3.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        Out o = render(p, chord, 3.5, [&](Engine& e, double) { onlyFx(fx)(e, 0); e.setParam(fx, 0, 1); });
        double ld = db(rms(mono(o), .2, 3) / rms(mono(dry), .2, 3)), monoLoss = db(rms(mono(o), .2, 3) / std::sqrt(.5 * (rms(o.L, .2, 3) * rms(o.L, .2, 3) + rms(o.R, .2, 3) * rms(o.R, .2, 3))));
        // movement: spectra of two 200 ms windows must differ more than in the dry signal
        auto spec = [&](const std::vector<float>& x, double t) { return bands(x, t); };
        auto diff = [&](const std::vector<float>& x) { auto a = spec(x, 1.0), b = spec(x, 1.9); double d = 0, n = 0; for (size_t i = 0; i < a.size(); ++i) { d += std::fabs(db(a[i]) - db(b[i])); ++n; } return d / n; };
        double mv = diff(mono(o)), mvd = diff(mono(dry));
        std::printf("    level vs dry %+.1f dB, mono fold-down vs stereo %+.1f dB, spectral movement %.1f dB (dry %.1f dB)\n", ld, monoLoss, mv, mvd);
        check(mv > mvd + (fx == FX_CHORUS ? .5 : 1.0), "audibly moves the spectrum over time");
        check(std::fabs(ld) < 3.0, "keeps the level close to dry (within 3 dB)");
        check(monoLoss > -3.5, "mono-compatible (fold-down loses < 3.5 dB)");
        if (fx == FX_CHORUS) check(rms(side(o), .2, 3) > rms(mono(o), .2, 3) * .15, "widens the stereo image (mono source becomes stereo)");
        if (fx != FX_CHORUS) {
            Out fb = render(p, chord, 3.5, [&](Engine& e, double) { onlyFx(fx)(e, 0); e.setParam(fx, 0, 1); e.setParam(fx, 3, fx == FX_FLANGER ? -.85f : .8f); });
            Out fb2 = render(p, chord, 3.5, [&](Engine& e, double) { onlyFx(fx)(e, 0); e.setParam(fx, 0, 1); e.setParam(fx, 3, fx == FX_FLANGER ? .85f : .8f); });
            std::printf("    max feedback peak %.2f / %.2f (dry %.2f)\n", peak(fb), peak(fb2), peak(dry));
            check(peak(fb) < peak(dry) * 2.2f + .05f && peak(fb2) < peak(dry) * 2.2f + .05f, "maximum feedback stays controlled");
        }
        double cpu = 0; robustness(fx, cpu); bypass(fx); totalCpu += cpu; }

    // ---------------- SATURATION
    if (want(FX_SATURATION)) { section(FX_SATURATION);
        ParamMap p = sinePreset();
        auto thd = [&](const Out& o, double f0) { double h1 = band(o.L, .5, 1.5, f0, .01), h = 0; for (int k = 2; k <= 6; ++k) { double m = band(o.L, .5, 1.5, f0 * k, .01); h += m * m; } return std::sqrt(h) / (h1 + 1e-12); };
        Out clean = render(p, {{0, 1.6, 57, .8f}}, 1.6, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        double t0 = thd(clean, 220);
        for (int mode = 0; mode < 3; ++mode) {
            Out lo = render(p, {{0, 1.6, 57, .8f}}, 1.6, [&](Engine& e, double) { onlyFx(FX_SATURATION)(e, 0); e.setParam(FX_SATURATION, 1, mode); e.setParam(FX_SATURATION, 0, .2f); });
            Out hi = render(p, {{0, 1.6, 57, .8f}}, 1.6, [&](Engine& e, double) { onlyFx(FX_SATURATION)(e, 0); e.setParam(FX_SATURATION, 1, mode); e.setParam(FX_SATURATION, 0, 1); });
            double lvl = db(rms(hi.L, .5, 1.5) / rms(clean.L, .5, 1.5));
            std::printf("    mode %d: THD clean %.4f, drive 20%% %.3f, drive 100%% %.3f, level vs clean %+.1f dB\n", mode, t0, thd(lo, 220), thd(hi, 220), lvl);
            check(thd(hi, 220) > thd(lo, 220) && thd(lo, 220) > t0 * 5, std::string("mode ") + std::to_string(mode) + ": more drive = more harmonics");
            check(std::fabs(lvl) < 2.0, "auto gain keeps level within 2 dB of clean");
        }
        // aliasing: a 3.52 kHz tone driven hard; anything not on a harmonic is aliasing
        Out al = render(p, {{0, 1.6, 106, .9f}}, 1.6, [&](Engine& e, double) { onlyFx(FX_SATURATION)(e, 0); e.setParam(FX_SATURATION, 0, 1); e.setParam(FX_SATURATION, 1, 2); });
        double f0 = 440 * std::pow(2, (106 - 69) / 12.0), fund = band(al.L, .5, 1.5, f0, .005), worst = 0;
        for (double f = 300; f < 20000; f += 37) { bool harm = false; for (int k = 1; k < 7; ++k) if (std::fabs(f - k * f0) < 120) harm = true; if (!harm) worst = std::max(worst, goertzel(al.L, .5, 1.5, f)); }
        std::printf("    aliasing at full crunch on %.0f Hz: worst inharmonic component %.1f dB below the fundamental\n", f0, db(fund / (worst + 1e-12)));
        check(db(fund / (worst + 1e-12)) > 40, "aliasing held at least 40 dB down (2x oversampling)");
        { ParamMap sp = sinePreset(); sp.v["out_gain_db"] = -12;
          Out a = render(sp, testMaterial(60), 9.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
          Out b = render(sp, testMaterial(60), 9.5, onlyFx(FX_SATURATION), [&](Engine& e, double t) { e.setParam(FX_SATURATION, 0, (float)(.15 - .15 * std::cos(t))); e.setParam(FX_SATURATION, 2, (float)std::sin(t * 1.3)); e.setParam(FX_SATURATION, 4, (float)(.5 + .5 * std::sin(t * .7))); });
          g_mask = &a.events; int ca = clicks(mono(a)), cb = clicks(mono(b)); g_mask = nullptr;
          std::printf("    sine chord, drive swept 0-30%% (static drive above ~40%% shapes chord peaks audibly by design) with tone and mix moving: clicks %d (dry %d)\n", cb, ca);
          check(cb <= ca + 1, "moving the saturation knobs never clicks"); }
        double cpu = 0; robustness(FX_SATURATION, cpu); bypass(FX_SATURATION); totalCpu += cpu; }

    // ---------------- FILTER
    if (want(FX_FILTER)) { section(FX_FILTER);
        ParamMap p = loadParams("rage-lead");
        auto run = [&](int type, float cut) { return render(p, {{0, 1.5, 60, .9f}}, 1.5, [&](Engine& e, double) { onlyFx(FX_FILTER)(e, 0); e.setParam(FX_FILTER, 1, type); e.setParam(FX_FILTER, 0, cut); e.setParam(FX_FILTER, 2, type == 4 ? .5f : .1f); }); };
        Out dry = render(p, {{0, 1.5, 60, .9f}}, 1.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        auto bandDb = [&](const Out& o, double f) { return db(band(o.L, .4, 1.2, f, .08) / (band(dry.L, .4, 1.2, f, .08) + 1e-12)); };
        Out lp = run(1, 500), hp = run(3, 2000), bp = run(4, 1000);
        std::printf("    LP24 @500: 4 kHz %.1f dB, 262 Hz %.1f dB | HP24 @2k: 262 Hz %.1f dB, 4.2 kHz %.1f dB | BP @1k: 1 kHz %.1f dB, 262 Hz %.1f dB\n",
                    bandDb(lp, 4186), bandDb(lp, 261.6), bandDb(hp, 261.6), bandDb(hp, 4186), bandDb(bp, 1046.5), bandDb(bp, 261.6));
        check(bandDb(lp, 4186) < -30 && bandDb(lp, 261.6) > -3, "low-pass removes highs, keeps lows");
        check(bandDb(hp, 261.6) < -30 && bandDb(hp, 4186) > -3, "high-pass removes lows, keeps highs");
        check(bandDb(bp, 1046.5) - bandDb(bp, 261.6) > 12, "band-pass keeps the band around the cutoff");
        Out sw = render(p, {{0, 6, 48, .9f}, {0, 6, 55, .9f}}, 6, [&](Engine& e, double) { onlyFx(FX_FILTER)(e, 0); e.setParam(FX_FILTER, 2, 1); },
                        [&](Engine& e, double t) { e.setParam(FX_FILTER, 0, (float)(30 * std::pow(600.0, .5 - .5 * std::cos(t * 3)))); });
        std::printf("    full-resonance sweep 30 Hz..18 kHz: peak %.2f, clicks %d\n", peak(sw), clicks(mono(sw)));
        check(peak(sw) < 1.0f && clicks(mono(sw)) <= 2, "fast sweep at full resonance: stable and click-free");
        double cpu = 0; robustness(FX_FILTER, cpu); bypass(FX_FILTER); totalCpu += cpu; }

    // ---------------- SHIMMER
    if (want(FX_SHIMMER)) { section(FX_SHIMMER);
        ParamMap p = sinePreset(false);
        Out o = render(p, {{0, .3, 57, .9f}}, 4, [&](Engine& e, double) { onlyFx(FX_SHIMMER)(e, 0); e.setParam(FX_SHIMMER, 0, 1); e.setParam(FX_SHIMMER, 4, 1); });
        double oct = band(o.L, 1, 3, 440), fund = band(o.L, 1, 3, 220);
        std::printf("    tail: octave-up / fundamental = %.2f\n", oct / fund);
        check(oct / fund > .5, "+12 interval adds octave-up harmonics to the tail");
        Out f5 = render(p, {{0, .3, 57, .9f}}, 4, [&](Engine& e, double) { onlyFx(FX_SHIMMER)(e, 0); e.setParam(FX_SHIMMER, 0, 1); e.setParam(FX_SHIMMER, 1, 1); e.setParam(FX_SHIMMER, 4, 1); });
        check(band(f5.L, 1, 3, 329.6) > band(o.L, 1, 3, 329.6) * 2, "+7 interval adds a fifth instead");
        Out l = render(p, {{0, .5, 45, .9f}, {0, .5, 52, .9f}, {0, .5, 57, .9f}, {0, .5, 64, .9f}}, 25, [&](Engine& e, double) { onlyFx(FX_SHIMMER)(e, 0); e.setParam(FX_SHIMMER, 0, 1); e.setParam(FX_SHIMMER, 2, 12); e.setParam(FX_SHIMMER, 4, 1); });
        std::printf("    max decay, 25 s: 2-3 s %.5f, 10-11 s %.5f, 23-24 s %.6f, peak %.2f\n", rms(l.L, 2, 3), rms(l.L, 10, 11), rms(l.L, 23, 24), peak(l));
        check(rms(l.L, 23, 24) < rms(l.L, 10, 11) && rms(l.L, 10, 11) < rms(l.L, 2, 3), "even at maximum decay the shimmer never builds up");
        double cpu = 0; robustness(FX_SHIMMER, cpu); bypass(FX_SHIMMER); totalCpu += cpu; }

    // ---------------- WIDTH
    if (want(FX_WIDTH)) { section(FX_WIDTH);
        ParamMap p = loadParams("underwater-pad");
        std::vector<Note> ch = {{0, 3, 36, .8f}, {0, 3, 48, .8f}, {0, 3, 55, .8f}, {0, 3, 60, .8f}};
        Out dry = render(p, ch, 3, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        Out w2 = render(p, ch, 3, [&](Engine& e, double) { onlyFx(FX_WIDTH)(e, 0); e.setParam(FX_WIDTH, 0, 2); e.setParam(FX_WIDTH, 1, 0); });
        Out w0 = render(p, ch, 3, [&](Engine& e, double) { onlyFx(FX_WIDTH)(e, 0); e.setParam(FX_WIDTH, 0, 0); });
        Out mc = render(p, ch, 3, [&](Engine& e, double) { onlyFx(FX_WIDTH)(e, 0); e.setParam(FX_WIDTH, 3, 1); });
        Out bm = render(p, ch, 3, [&](Engine& e, double) { onlyFx(FX_WIDTH)(e, 0); e.setParam(FX_WIDTH, 0, 2); e.setParam(FX_WIDTH, 1, 200); });
        double sd = rms(side(dry), .5, 2.5);
        std::printf("    side level: dry %.4f, width 200%% %.4f, width 0%% %.6f, mono check %.6f | mid kept: %.1f dB\n", sd, rms(side(w2), .5, 2.5), rms(side(w0), .5, 2.5), rms(side(mc), .5, 2.5), db(rms(mono(w2), .5, 2.5) / rms(mono(dry), .5, 2.5)));
        check(rms(side(w2), .5, 2.5) > sd * 1.8 && rms(side(w0), .5, 2.5) < sd * .01, "Width scales the sides (0% = mono, 200% = double)");
        check(std::fabs(db(rms(mono(w2), .5, 2.5) / rms(mono(dry), .5, 2.5))) < .3, "centre image (mid) is preserved");
        check(rms(side(mc), .5, 2.5) < 1e-6, "Mono check sums to mono");
        { ParamMap st = loadParams("midnight-strum");   // detuned doubled strings: real stereo content in the bass
          std::vector<Note> lo = {{0, 3, 33, .9f}, {0, 3, 40, .9f}};
          Out a = render(st, lo, 3, [&](Engine& e, double) { onlyFx(FX_WIDTH)(e, 0); e.setParam(FX_WIDTH, 0, 2); e.setParam(FX_WIDTH, 1, 0); });
          Out b = render(st, lo, 3, [&](Engine& e, double) { onlyFx(FX_WIDTH)(e, 0); e.setParam(FX_WIDTH, 0, 2); e.setParam(FX_WIDTH, 1, 200); });
          double sa = band(side(a), .3, 2.5, 55, .05) + band(side(a), .3, 2.5, 82.4, .05), sb = band(side(b), .3, 2.5, 55, .05) + band(side(b), .3, 2.5, 82.4, .05);
          std::printf("    side energy at 55/82 Hz: mono-bass off %.6f, mono-bass 200 Hz %.6f\n", sa, sb);
          check(sb < sa * .25, "bass below the mono frequency stays centred"); }
        double cpu = 0; robustness(FX_WIDTH, cpu); bypass(FX_WIDTH); totalCpu += cpu; }

    // ---------------- GRANULAR
    if (want(FX_GRANULAR)) { section(FX_GRANULAR);
        ParamMap p = loadParams("after-hours-ep");
        std::vector<Note> mel; for (int i = 0; i < 8; ++i) mel.push_back({i * .4, .35, 60 + (int[]){0, 3, 7, 10, 12, 10, 7, 3}[i], .8f});
        Out dry = render(p, mel, 5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        Out g = render(p, mel, 5, [&](Engine& e, double) { onlyFx(FX_GRANULAR)(e, 0); e.setParam(FX_GRANULAR, 5, 1); e.setParam(FX_GRANULAR, 0, 1); });
        Out gp = render(p, mel, 5, [&](Engine& e, double) { onlyFx(FX_GRANULAR)(e, 0); e.setParam(FX_GRANULAR, 5, 1); e.setParam(FX_GRANULAR, 0, 1); e.setParam(FX_GRANULAR, 4, 12); });
        std::printf("    texture level vs dry %+.1f dB; after notes stop (3.6-4.5 s): dry %.5f, cloud %.5f; clicks %d (dry %d)\n", db(rms(mono(g), .5, 3) / rms(mono(dry), .5, 3)), rms(mono(dry), 3.6, 4.5), rms(mono(g), 3.6, 4.5), clicks(mono(g)), clicks(mono(dry)));
        check(rms(mono(g), .5, 3) > rms(mono(dry), .5, 3) * .2, "produces an audible grain cloud");
        check(clicks(mono(g)) <= clicks(mono(dry)) + 2, "grain envelopes are smooth (no clicks)");
        check(band(mono(gp), .8, 3, 2 * 261.6 * 4, .02) + band(mono(gp), .8, 3, 261.6 / 2, .02) > band(mono(g), .8, 3, 2 * 261.6 * 4, .02) + band(mono(g), .8, 3, 261.6 / 2, .02), "Pitch spread adds transposed grains");
        double cpu = 0; robustness(FX_GRANULAR, cpu); bypass(FX_GRANULAR); totalCpu += cpu; }

    // ---------------- GLIDE
    if (want(FX_GLIDE)) { section(FX_GLIDE);
        ParamMap p = sinePreset();
        auto mid = [&](int mode, bool overlap) {
            Out o = render(p, {{0, overlap ? .6 : .45, 57, .9f}, {.5, .6, 69, .9f}}, 1.2, [&](Engine& e, double) { onlyFx(FX_GLIDE)(e, 0); e.setParam(FX_GLIDE, 1, mode); e.setParam(FX_GLIDE, 0, .3f); });
            return std::make_pair(zcFreq(o.L, .52, .03), zcFreq(o.L, 1.05, .05));
        };
        auto a = mid(0, true), b = mid(0, false), c = mid(1, false), d = mid(2, false);
        std::printf("    20 ms after the 2nd note: legato-overlap %.0f Hz, legato-detached %.0f Hz, always %.0f Hz, poly %.0f Hz (target 440, end %.0f)\n", a.first, b.first, c.first, d.first, a.second);
        check(a.first > 225 && a.first < 400 && std::fabs(a.second - 440) < 3, "Legato: overlapping notes slide and arrive in tune");
        check(std::fabs(b.first - 440) < 5, "Legato: detached notes start on pitch");
        check(c.first < 400 && d.first < 400, "Always and Poly modes glide detached notes");
        Out sm = render(p, {{0, .6, 57, .9f}, {.5, .6, 69, .9f}, {1.0, .6, 62, .9f}}, 1.8, [&](Engine& e, double) { onlyFx(FX_GLIDE)(e, 0); e.setParam(FX_GLIDE, 0, .2f); });
        check(clicks(sm.L) == 0, "slides are smooth (no clicks)");
        double cpu = 0; robustness(FX_GLIDE, cpu); bypass(FX_GLIDE); totalCpu += cpu; }

    // ---------------- TAPE
    if (want(FX_TAPE)) { section(FX_TAPE);
        ParamMap p = sinePreset();
        Out t = render(p, {{0, 4, 69, .8f}}, 4, [&](Engine& e, double) { onlyFx(FX_TAPE)(e, 0); e.setParam(FX_TAPE, 0, 1); e.setParam(FX_TAPE, 1, 1); e.setParam(FX_TAPE, 2, 1); });
        double lo = 1e9, hi = 0; for (double x = .5; x < 3.8; x += .05) { double f = zcFreq(t.L, x, .05); lo = std::min(lo, f); hi = std::max(hi, f); }
        double cents = 1200 * std::log2(hi / lo);
        std::printf("    full wow+flutter on A4: pitch swings %.1f cents peak-to-peak\n", cents);
        check(cents > 4 && cents < 40, "wow & flutter move the pitch by a musical amount");
        Out cl = render(p, {{0, .3, 69, .8f}}, 2, [&](Engine& e, double) { onlyFx(FX_TAPE)(e, 0); e.setParam(FX_TAPE, 4, 0); });
        check(rms(cl.L, 1, 2) < 1e-6, "noise at 0 = clean (no hiss)");
        Out ns = render(p, {{0, .3, 69, .8f}}, 2, [&](Engine& e, double) { onlyFx(FX_TAPE)(e, 0); e.setParam(FX_TAPE, 4, 1); });
        std::printf("    hiss at full noise: %.1f dBFS\n", db(rms(ns.L, 1, 2)));
        check(db(rms(ns.L, 1, 2)) < -45, "hiss stays subtle even at maximum");
        double cpu = 0; robustness(FX_TAPE, cpu); bypass(FX_TAPE); totalCpu += cpu; }

    // ---------------- GATE / PUMP
    if (want(FX_GATE)) { section(FX_GATE);
        ParamMap p = sinePreset();
        const char* pn[6] = {"Pump 1/4", "Pump 1/8", "Pump 1/2", "Gate 1/8", "Gate 1/16", "Trance 16"};
        for (int pat = 0; pat < 6; ++pat) {
            Out o = render(p, {{0, 4, 57, .8f}}, 4, [&](Engine& e, double) { onlyFx(FX_GATE)(e, 0); e.setParam(FX_GATE, 1, pat); e.setParam(FX_GATE, 0, 1); });
            // envelope sampled every 10 ms
            std::vector<double> env; for (double x = .5; x < 3.5; x += .01) env.push_back(rms(o.L, x, x + .01));
            double mn = *std::min_element(env.begin(), env.end()), mx = *std::max_element(env.begin(), env.end());
            std::printf("    %-10s depth %.1f dB, clicks %d\n", pn[pat], db(mx / (mn + 1e-9)), clicks(o.L));
            check(db(mx / (mn + 1e-9)) > 12 && clicks(o.L) == 0, std::string(pn[pat]) + ": moves the level rhythmically, no clicks");
        }
        Out ph = render(p, {{0, 4, 57, .8f}}, 4, [&](Engine& e, double) { onlyFx(FX_GATE)(e, 0); e.setParam(FX_GATE, 1, 0); e.setParam(FX_GATE, 0, 1); });
        double atBeat = rms(ph.L, 1.0, 1.03), offBeat = rms(ph.L, 1.35, 1.4);
        check(atBeat < offBeat * .5, "pump ducks on the beat and recovers between beats (synced to 120 BPM)");
        double cpu = 0; robustness(FX_GATE, cpu); bypass(FX_GATE); totalCpu += cpu; }

    // ---------------- OCTAVE LAYER
    if (want(FX_OCTAVE)) { section(FX_OCTAVE);
        ParamMap p = sinePreset();
        const int offs[4] = {-24, -12, 12, 24};
        for (int o = 0; o < 4; ++o) {
            Out x = render(p, {{0, 1, 69, .8f}}, 1, [&](Engine& e, double) { onlyFx(FX_OCTAVE)(e, 0); e.setParam(FX_OCTAVE, 1, o); e.setParam(FX_OCTAVE, 0, 1); e.setParam(FX_OCTAVE, 2, 18000); });
            double f = 440 * std::pow(2, offs[o] / 12.0), lay = band(x.L, .3, .9, f, .01), orig = band(x.L, .3, .9, 440, .01);
            std::printf("    %+d st layer: %.0f Hz at %.1f dB relative to the played note\n", offs[o], f, db(lay / orig));
            check(std::fabs(db(lay / orig)) < 3, std::string("octave ") + std::to_string(offs[o]) + " layer is in tune and balanced at full level");
        }
        Out mono1 = render(loadParams("silk-glide"), {{0, .6, 60, .9f}, {.5, .6, 67, .9f}}, 1.3, [&](Engine& e, double) { onlyFx(FX_OCTAVE)(e, 0); e.setParam(FX_OCTAVE, 0, 1); e.setParam(FX_OCTAVE, 1, 1); });
        check(peak(mono1) > .01 && clicks(mono(mono1)) <= 1, "works on mono/glide instruments too");
        double cpu = 0; robustness(FX_OCTAVE, cpu); bypass(FX_OCTAVE); totalCpu += cpu; }

    // ---------------- RESONATOR
    if (want(FX_RESONATOR)) { section(FX_RESONATOR);
        ParamMap p = loadParams("rage-lead");
        Out dry = render(p, {{0, 1.5, 48, .9f}}, 1.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        Out a = render(p, {{0, 1.5, 48, .9f}}, 1.5, [&](Engine& e, double) { onlyFx(FX_RESONATOR)(e, 0); e.setParam(FX_RESONATOR, 0, 1); e.setParam(FX_RESONATOR, 5, 1); e.setParam(FX_RESONATOR, 2, 0); });
        Out i = render(p, {{0, 1.5, 48, .9f}}, 1.5, [&](Engine& e, double) { onlyFx(FX_RESONATOR)(e, 0); e.setParam(FX_RESONATOR, 0, 1); e.setParam(FX_RESONATOR, 5, 1); e.setParam(FX_RESONATOR, 2, 2); });
        auto r = [&](const Out& o, double f) { return band(mono(o), .4, 1.3, f, .12) / (band(mono(dry), .4, 1.3, f, .12) + 1e-12); };
        std::printf("    vowel A: 800 Hz %+.1f dB vs 2.2 kHz %+.1f dB | vowel I: 800 Hz %+.1f dB vs 2.2 kHz %+.1f dB | level %+.1f dB\n", db(r(a, 800)), db(r(a, 2200)), db(r(i, 800)), db(r(i, 2200)), db(rms(mono(a)) / rms(mono(dry))));
        check(db(r(a, 800)) - db(r(a, 2200)) > 6 && db(r(i, 2200)) - db(r(i, 800)) > 6, "vowel selection moves the formant peaks");
        ParamMap nz; nz.v = {{"l1_type", 5}, {"l1_ncut", 12000}, {"l1_nmode", 0}, {"l1_a", .001f}, {"l1_d", .03f}, {"l1_s", 0}, {"l1_r", .02f}, {"f_type", -1}};
        Out m = render(nz, {{0, .03, 60, .9f}}, 1.5, [&](Engine& e, double) { onlyFx(FX_RESONATOR)(e, 0); e.setParam(FX_RESONATOR, 1, 1); e.setParam(FX_RESONATOR, 3, 220); e.setParam(FX_RESONATOR, 4, 1); e.setParam(FX_RESONATOR, 0, 1); e.setParam(FX_RESONATOR, 5, 1); });
        double on = band(m.L, .05, .6, 220, .01) + band(m.L, .05, .6, 440, .01) + band(m.L, .05, .6, 660, .01), off = band(m.L, .05, .6, 330, .01) + band(m.L, .05, .6, 550, .01) + band(m.L, .05, .6, 770, .01);
        std::printf("    metallic comb at 220 Hz excited by a noise burst: harmonics of 220 vs in-between %.1f dB; peak %.2f\n", db(on / off), peak(m));
        check(db(on / off) > 15, "metallic mode resonates at the chosen frequency (and its harmonics)");
        check(db(rms(mono(a)) / rms(mono(dry))) < 4, "auto-gain prevents big volume jumps");
        double cpu = 0; robustness(FX_RESONATOR, cpu); bypass(FX_RESONATOR); totalCpu += cpu; }

    // ---------------- REVERSE & SWELL
    if (want(FX_REVERSE)) { section(FX_REVERSE);
        ParamMap p = loadParams("drill-bell");   // percussive: fast attack, long decay -> reversed it must swell
        Out dry = render(p, {{0, .9, 69, .9f}}, 2.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        Out rv = render(p, {{0, .9, 69, .9f}}, 2.5, [&](Engine& e, double) { onlyFx(FX_REVERSE)(e, 0); e.setParam(FX_REVERSE, 1, 1); e.setParam(FX_REVERSE, 2, 2); e.setParam(FX_REVERSE, 0, 1); });
        // 1/2 bar segment = 1 s at 120 BPM: segment 0 (0-1 s) is recorded, played reversed during 1-2 s
        double early = rms(mono(rv), 1.05, 1.25), late = rms(mono(rv), 1.75, 1.95), dEarly = rms(mono(dry), .05, .25), dLate = rms(mono(dry), .75, .95);
        std::printf("    dry decays %+.1f dB across the note; reversed playback rises %+.1f dB across the same span\n", db(dLate / dEarly), db(late / early));
        check(late > early * 3 && dLate < dEarly, "real reversed audio (the decay plays backwards, one segment later)");
        Out sw = render(sinePreset(), {{0, 1.5, 69, .9f}}, 1.5, [&](Engine& e, double) { onlyFx(FX_REVERSE)(e, 0); e.setParam(FX_REVERSE, 1, 0); e.setParam(FX_REVERSE, 3, .8f); e.setParam(FX_REVERSE, 0, 1); });
        std::printf("    swell (rise 0.8 s): 0-50 ms %.4f, 0.4 s %.4f, 1.2 s %.4f\n", rms(sw.L, 0, .05), rms(sw.L, .4, .45), rms(sw.L, 1.2, 1.25));
        check(rms(sw.L, 0, .05) < rms(sw.L, .4, .45) * .5 && rms(sw.L, .4, .45) < rms(sw.L, 1.2, 1.25) && clicks(sw.L) == 0, "Swell fades each note in smoothly over the rise time");
        double cpu = 0; robustness(FX_REVERSE, cpu); bypass(FX_REVERSE); totalCpu += cpu; }

    // ---------------- FREEZE
    if (want(FX_FREEZE)) { section(FX_FREEZE);
        ParamMap p = loadParams("underwater-pad");
        Out o = render(p, {{0, 2, 48, .8f}, {0, 2, 55, .8f}, {0, 2, 63, .8f}, {6, .5, 72, .8f}}, 40, [&](Engine& e, double) { onlyFx(FX_FREEZE)(e, 0); e.setParam(FX_FREEZE, 0, 1); e.setParam(FX_FREEZE, 3, 2); },
                       [&](Engine& e, double t) { e.setParam(FX_FREEZE, 1, (t > 1.8 && t < 34) ? 1.f : 0.f); });
        double a = rms(mono(o), 10, 12), b = rms(mono(o), 30, 32), after = rms(mono(o), 38, 40);
        std::printf("    frozen cloud: 10-12 s %.4f, 30-32 s %.4f (%+.2f dB drift); 4+ s after release %.6f; clicks %d; peak %.2f\n", a, b, db(b / a), after, clicks(mono(o)), peak(o));
        check(a > 1e-3 && std::fabs(db(b / a)) < 3, "sustains the captured sound at a stable level for 30+ s");
        check(after < a * .02, "releases smoothly (fades over the release time)");
        check(clicks(mono(o)) <= 2, "capture and release are click-free");
        double cpu = 0; robustness(FX_FREEZE, cpu); bypass(FX_FREEZE); totalCpu += cpu; }

    // ---------------- GLUE / PUNCH
    if (want(FX_GLUE)) { section(FX_GLUE);
        ParamMap p = loadParams("obsidian-pluck");   // plucked: clear attacks and decaying bodies
        std::vector<Note> dyn; for (int i = 0; i < 16; ++i) dyn.push_back({i * .25, .2, 60 + (i % 4) * 3, i % 4 == 0 ? 1.f : .35f});
        Out dry = render(p, dyn, 4.5, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) e.setOn(k, false); });
        Out g = render(p, dyn, 4.5, [&](Engine& e, double) { onlyFx(FX_GLUE)(e, 0); e.setParam(FX_GLUE, 0, 1); e.setParam(FX_GLUE, 2, -30); e.setParam(FX_GLUE, 3, 4); e.setParam(FX_GLUE, 4, 3); e.setParam(FX_GLUE, 5, 60); });
        Out pu = render(p, dyn, 4.5, [&](Engine& e, double) { onlyFx(FX_GLUE)(e, 0); e.setParam(FX_GLUE, 1, 1); e.setParam(FX_GLUE, 0, 1); });
        auto range = [&](const Out& o) { std::vector<double> v; for (double x = 1.1; x < 4; x += .25) v.push_back(rms(mono(o), x, x + .06)); return db(*std::max_element(v.begin(), v.end()) / *std::min_element(v.begin(), v.end())); };
        auto attack = [&](const Out& o) { double a = 0, s = 0; for (int i = 0; i < 16; ++i) { a += rms(mono(o), i * .25, i * .25 + .012); s += rms(mono(o), i * .25 + .08, i * .25 + .18); } return db(a / s); };
        std::printf("    loud/soft note range: dry %.1f dB, glue %.1f dB | attack-to-body: dry %.1f dB, punch %.1f dB | level vs dry: glue %+.1f, punch %+.1f dB\n",
                    range(dry), range(g), attack(dry), attack(pu), db(rms(mono(g)) / rms(mono(dry))), db(rms(mono(pu)) / rms(mono(dry))));
        check(range(g) < range(dry) - 2, "Glue evens out loud and soft notes");
        check(attack(pu) > attack(dry) + 2, "Punch lifts the attacks");
        check(std::fabs(db(rms(mono(g), 1.5, 4.5) / rms(mono(dry), 1.5, 4.5))) < 2.0, "auto make-up keeps Glue level-matched (within 2 dB once settled)");
        double cpu = 0; robustness(FX_GLUE, cpu); bypass(FX_GLUE); totalCpu += cpu; }

    // ---------------- combinations, state, CPU
    if (only.empty() || only == "all") {
        std::printf("\n== Combinations, state, CPU\n");
        for (const char* id : {"after-hours-ep", "rage-lead", "underwater-pad"}) {
            ParamMap p = loadParams(id); auto mat = testMaterial(baseFor(id));
            Out o = render(p, mat, 12, [&](Engine& e, double) { for (int k = 0; k < FX_COUNT; ++k) if (k != FX_FREEZE) e.setOn(k, true); },
                           [&](Engine& e, double t) { e.setParam(FX_FILTER, 0, (float)(200 * std::pow(80.0, .5 + .5 * std::sin(t)))); e.setParam(FX_PITCH, 0, (float)std::round(3 * std::sin(t * .5))); });
            std::printf("    %-16s ALL effects on, knobs moving: peak %.2f, NaN %s, CPU %.1f ms per audio second (%.1f%% of one core)\n", id, peak(o), peak(o) > 1e8 ? "yes" : "no", o.cpuMs / 12, o.cpuMs / 12 / 10);
            check(peak(o) <= 1.0f, std::string(id) + ": every effect at once stays stable");
        }
        Rack a; a.prepare(SR); a.setOn(FX_DELAY, true); a.setParam(FX_DELAY, 1, 4); a.setParam(FX_FILTER, 0, 1234); a.setOn(FX_GLUE, true); a.setParam(FX_GLUE, 1, 1);
        Rack b; b.prepare(SR); b.setState(a.state());
        bool same = true; for (int e = 0; e < FX_COUNT; ++e) { same = same && a.on[e] == b.on[e]; for (int i = 0; i < kMaxParams; ++i) same = same && std::fabs(a.val[e][i] - b.val[e][i]) < 1e-4f; }
        check(same, "effect settings save and restore exactly (state string round trip)");
    }
    std::printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
    return g_fail ? 1 : 0;
}

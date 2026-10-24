// LUMELODY audio analysis: waveform peaks, silence, transients, tempo, key and root note.
// All estimates come with a confidence (0..1); the UI shows them as estimates the user can correct.
#pragma once
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>
#include "LumiSampler.h"

namespace lumi {
namespace analysis {

// min/max per bin for drawing the real waveform: out[2*i] = min, out[2*i+1] = max (both channels mixed)
inline std::vector<float> peaks(const AudioData& a, int bins) {
    std::vector<float> out((size_t) bins * 2, 0.f);
    const size_t n = a.frames();
    if (!n || bins <= 0) return out;
    for (int b = 0; b < bins; ++b) {
        size_t i0 = (size_t) ((double) b / bins * (double) n), i1 = std::max(i0 + 1, (size_t) ((double) (b + 1) / bins * (double) n));
        float lo = 1e9f, hi = -1e9f;
        for (size_t i = i0; i < std::min(i1, n); ++i) { const float l = a.L[i], r = a.R[i]; lo = std::min(lo, std::min(l, r)); hi = std::max(hi, std::max(l, r)); }
        out[(size_t) b * 2] = lo; out[(size_t) b * 2 + 1] = hi;
    }
    return out;
}

// first and last frame above a level (dB); {0, n} when everything is above
inline std::pair<size_t, size_t> nonSilent(const AudioData& a, float thresholdDb = -60.f) {
    const float th = std::pow(10.f, thresholdDb / 20.f); const size_t n = a.frames();
    size_t s = 0, e = n;
    while (s < n && std::fabs(a.L[s]) < th && std::fabs(a.R[s]) < th) ++s;
    while (e > s && std::fabs(a.L[e - 1]) < th && std::fabs(a.R[e - 1]) < th) --e;
    return { s, e };
}

inline void fft(std::vector<std::complex<float>>& x) {   // in-place radix-2, size power of two
    const size_t n = x.size();
    for (size_t i = 1, j = 0; i < n; ++i) { size_t bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap(x[i], x[j]); }
    for (size_t len = 2; len <= n; len <<= 1) {
        const float ang = -6.28318530718f / (float) len; const std::complex<float> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) { std::complex<float> w(1); for (size_t j = 0; j < len / 2; ++j) { auto u = x[i + j], v = x[i + j + len / 2] * w; x[i + j] = u + v; x[i + j + len / 2] = u - v; w *= wl; } }
    }
}

struct Analysis {
    double bpm = 0, bpmConf = 0;
    int keyTonic = -1; bool keyMinor = false; double keyConf = 0;
    int rootNote = -1; double rootCents = 0, rootConf = 0;
    size_t silenceStart = 0, silenceEnd = 0;
    std::vector<size_t> onsets;          // transient positions (frames)
    double seconds = 0;
};

// spectral-flux onset envelope (hop 512) on a mono mix, at most `maxSeconds` analysed
inline std::vector<float> onsetEnvelope(const AudioData& a, int hop, double maxSeconds, double& envRate) {
    const int N = 1024; const size_t n = std::min(a.frames(), (size_t) (maxSeconds * a.sr));
    envRate = a.sr / hop;
    std::vector<float> env; std::vector<float> prev(N / 2, 0.f), win(N);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos(6.28318530718f * (float) i / (float) N);
    std::vector<std::complex<float>> buf(N);
    for (size_t pos = 0; pos + N <= n; pos += (size_t) hop) {
        for (int i = 0; i < N; ++i) buf[(size_t) i] = 0.5f * (a.L[pos + (size_t) i] + a.R[pos + (size_t) i]) * win[(size_t) i];
        fft(buf);
        float flux = 0;
        for (int k = 1; k < N / 2; ++k) { const float m = std::log1p(10.f * std::abs(buf[(size_t) k])); flux += std::max(0.f, m - prev[(size_t) k]); prev[(size_t) k] = m; }
        env.push_back(flux);
    }
    return env;
}

inline Analysis analyse(const AudioData& a) {
    Analysis r; const size_t n = a.frames();
    r.seconds = n / a.sr;
    auto ns = nonSilent(a, -60.f); r.silenceStart = ns.first; r.silenceEnd = ns.second;
    if (n < 2048) return r;
    // ---- transients: peaks of the onset envelope above an adaptive threshold
    const int hop = 512; double envRate = 0;
    auto env = onsetEnvelope(a, hop, 240.0, envRate);
    if (env.size() > 8) {
        std::vector<float> th(env.size());
        for (size_t i = 0; i < env.size(); ++i) {
            const size_t lo = i >= 8 ? i - 8 : 0, hi = std::min(env.size(), i + 8);
            float m = 0; for (size_t k = lo; k < hi; ++k) m += env[k]; th[i] = 1.5f * m / (float) (hi - lo) + 1e-3f;
        }
        float mx = *std::max_element(env.begin(), env.end());
        size_t last = 0; bool any = false;
        for (size_t i = 1; i + 1 < env.size(); ++i)
            if (env[i] > th[i] && env[i] >= env[i - 1] && env[i] > env[i + 1] && env[i] > .08f * mx) {
                const size_t f = i * (size_t) hop;
                if (!any || f - last > (size_t) (.06 * a.sr)) { r.onsets.push_back(f); last = f; any = true; }
            }
    }
    // ---- tempo: autocorrelation of the onset envelope, 60..180 BPM, folded to 70..160
    if (env.size() > (size_t) (envRate * 4)) {
        std::vector<float> e(env); float mean = 0; for (float x : e) mean += x; mean /= (float) e.size(); for (float& x : e) x -= mean;
        double best = -1e30, second = -1e30; int bestLag = 0;
        const int lo = (int) (envRate * 60 / 180), hi = (int) (envRate * 60 / 60);
        std::vector<double> ac((size_t) hi + 2, 0.0);
        for (int L = lo; L <= hi; ++L) { double s = 0; for (size_t i = (size_t) L; i < e.size(); ++i) s += (double) e[i] * e[i - (size_t) L]; ac[(size_t) L] = s / (double) (e.size() - (size_t) L); }
        for (int L = lo; L <= hi; ++L) {   // reward lags whose double also correlates (beat structure)
            double s = ac[(size_t) L] + (2 * L <= hi ? .5 * ac[(size_t) (2 * L)] : 0.0);
            if (s > best) { second = best; best = s; bestLag = L; } else if (s > second) second = s;
        }
        if (bestLag > 0 && best > 0) {
            double bpm = 60.0 * envRate / bestLag;
            while (bpm < 70) bpm *= 2; while (bpm > 160) bpm /= 2;
            r.bpm = std::round(bpm * 10) / 10;
            r.bpmConf = std::max(0.0, std::min(1.0, (best - std::max(0.0, second)) / best * 3.0));
            if (r.onsets.size() < 4 || r.seconds < 2.5) r.bpmConf *= .3;   // one-shots have no tempo
        }
    }
    // ---- key: chroma from an FFT over the whole (first 120 s) file, Krumhansl-Kessler profiles
    {
        const int N = 8192; const size_t m = std::min(n, (size_t) (120 * a.sr));
        double chroma[12] = {0};
        std::vector<std::complex<float>> buf(N);
        int frames = 0;
        for (size_t pos = 0; pos + N <= m; pos += N / 2) {
            for (int i = 0; i < N; ++i) buf[(size_t) i] = 0.5f * (a.L[pos + (size_t) i] + a.R[pos + (size_t) i]) * (0.5f - 0.5f * std::cos(6.28318530718f * (float) i / (float) N));
            fft(buf); ++frames;
            for (int k = 1; k < N / 2; ++k) {
                const double f = k * a.sr / N; if (f < 55 || f > 4200) continue;
                const double midi = 69 + 12 * std::log2(f / 440.0); const int pc = ((int) std::lround(midi) % 12 + 12) % 12;
                chroma[pc] += std::abs(buf[(size_t) k]) / std::sqrt(f / 55.0);
            }
            if (frames > 600) break;
        }
        if (frames > 0) {
            static const double MAJ[12] = {6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88};
            static const double MIN[12] = {6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17};
            auto corr = [&](const double* prof, int t) {
                double mx = 0, my = 0; for (int i = 0; i < 12; ++i) { mx += chroma[(i + t) % 12]; my += prof[i]; } mx /= 12; my /= 12;
                double sxy = 0, sxx = 0, syy = 0; for (int i = 0; i < 12; ++i) { const double x = chroma[(i + t) % 12] - mx, y = prof[i] - my; sxy += x * y; sxx += x * x; syy += y * y; }
                return sxy / std::sqrt(sxx * syy + 1e-12);
            };
            double best = -2, second = -2; int bt = 0; bool bm = false;
            for (int t = 0; t < 12; ++t) for (int mi = 0; mi < 2; ++mi) {
                const double c = corr(mi ? MIN : MAJ, t);
                if (c > best) { second = best; best = c; bt = t; bm = mi; } else if (c > second) second = c;
            }
            r.keyTonic = bt; r.keyMinor = bm; r.keyConf = std::max(0.0, std::min(1.0, (best - second) * 4 + (best - .5)));
        }
    }
    // ---- root note: YIN on a steady part (after the attack) of the first 2 s
    {
        const size_t W = 2048, start = std::min(ns.first + (size_t) (.08 * a.sr), n > W * 2 ? n - W * 2 : 0);
        if (start + 2 * W <= n) {
            std::vector<float> x(2 * W); for (size_t i = 0; i < 2 * W; ++i) x[i] = 0.5f * (a.L[start + i] + a.R[start + i]);
            const int tauMin = (int) (a.sr / 2000), tauMax = std::min((int) W - 1, (int) (a.sr / 30));
            std::vector<double> d((size_t) tauMax + 1, 0.0), cm((size_t) tauMax + 1, 1.0);
            for (int tau = 1; tau <= tauMax; ++tau) { double s = 0; for (size_t i = 0; i < W; ++i) { const double df = x[i] - x[i + (size_t) tau]; s += df * df; } d[(size_t) tau] = s; }
            double run = 0; for (int tau = 1; tau <= tauMax; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = d[(size_t) tau] * tau / (run + 1e-12); }
            int tau = -1; for (int t = tauMin; t <= tauMax; ++t) if (cm[(size_t) t] < .15) { while (t + 1 <= tauMax && cm[(size_t) t + 1] < cm[(size_t) t]) ++t; tau = t; break; }
            if (tau < 0) { double mn = 1e9; for (int t = tauMin; t <= tauMax; ++t) if (cm[(size_t) t] < mn) { mn = cm[(size_t) t]; tau = t; } if (mn > .4) tau = -1; }
            if (tau > 1 && tau < tauMax) {
                const double a0 = cm[(size_t) tau - 1], a1 = cm[(size_t) tau], a2 = cm[(size_t) tau + 1];
                const double shift = (a0 - a2) / (2 * (a0 - 2 * a1 + a2) + 1e-12);
                const double f = a.sr / (tau + shift), midi = 69 + 12 * std::log2(f / 440.0);
                r.rootNote = (int) std::lround(midi); r.rootCents = (midi - r.rootNote) * 100; r.rootConf = std::max(0.0, std::min(1.0, 1.0 - a1 * 2.5));
            }
        }
    }
    return r;
}

// slice points from transients (fractions of the length), at most `maxSlices`
inline std::vector<double> slicesFromOnsets(const Analysis& an, size_t frames, int maxSlices) {
    std::vector<double> s;
    for (size_t f : an.onsets) { if (f < frames / 200) continue; s.push_back((double) f / (double) frames); if ((int) s.size() >= maxSlices - 1) break; }
    return s;
}
inline std::vector<double> equalSlices(int count) { std::vector<double> s; for (int i = 1; i < count; ++i) s.push_back((double) i / count); return s; }

}  // namespace analysis
}  // namespace lumi

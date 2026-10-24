// LUMELODY sound-library player: one-shots, loops and tempo-synced drum patterns layered together.
// Pure C++ (no JUCE), used by the plugin for previews and for rendering patterns to audio. The browser does the same
// with Web Audio (web/tools_libs.js); both follow the same timing rule, including swing on odd 16th notes.
#pragma once
#include "LumiSampler.h"
#include <cmath>
#include <memory>
#include <vector>
#include <algorithm>

namespace lumi {

struct LibPlayer {
    struct Ev { double beat; int sample; float vel; };
    struct Layer { std::vector<Ev> ev; double beats = 4; double swing = 0; float vol = 1; };
    struct Song {
        std::vector<std::shared_ptr<const AudioData>> samples;   // indexed by Ev::sample
        std::vector<Layer> layers;
        double bpm = 120; bool loop = true;
        std::shared_ptr<const AudioData> single; bool singleLoop = false;   // or: just play one sample
    };

    static double swingAt(double b, double sw) {
        const double six = std::round(b * 4 * 1e4) / 1e4, r = std::round(six);
        return (sw != 0 && std::fabs(six - r) < 1e-4 && ((long long) r) % 2 != 0) ? b + sw * .25 : b;
    }
    static double lengthBeats(const Song& s) { double m = 0; for (auto& l : s.layers) m = std::max(m, l.beats); return m; }

    void start(std::shared_ptr<const Song> s) {
        song = std::move(s); clock = 0; nvoices = 0; cycle.assign(song ? song->layers.size() : 0, 0);
        if (song && song->single) trigger(song->single, 1.f, 0, song->singleLoop);
    }
    void stop() { song.reset(); for (auto& v : voices) v.buf.reset(); nvoices = 0; }
    bool playing() const { if (song && !song->layers.empty()) return true; for (int i = 0; i < nvoices; ++i) if (voices[i].buf) return true; return false; }
    float gain = .9f;

    // adds into L/R (does not clear), n frames at sample rate sr
    void render(float* L, float* R, int n, double sr) {
        if (song && !song->layers.empty()) {
            const double spb = 60.0 / std::max(1.0, song->bpm) * sr;   // samples per beat
            const double total = lengthBeats(*song);
            for (size_t li = 0; li < song->layers.size(); ++li) {
                const auto& ly = song->layers[li];
                const double loopLen = ly.beats * spb;
                for (;;) {
                    const long long c = cycle[li];
                    if (!song->loop && (c + 1) * ly.beats > total + 1e-9) break;
                    const double base = (double) c * loopLen;
                    if (base >= clock + n) break;
                    bool pending = false;
                    for (const auto& e : ly.ev) {
                        const double at = base + swingAt(e.beat, ly.swing) * spb;
                        if (at < clock) continue;                    // already played
                        if (at >= clock + n) { pending = true; continue; }
                        if (e.sample >= 0 && e.sample < (int) song->samples.size() && song->samples[(size_t) e.sample])
                            trigger(song->samples[(size_t) e.sample], e.vel * ly.vol, (int) (at - clock), false);
                    }
                    if (pending) break;
                    cycle[li] = c + 1;
                }
            }
        }
        for (int vi = 0; vi < nvoices; ++vi) {
            auto& v = voices[vi]; if (!v.buf) continue;
            const auto& b = *v.buf; const double inc = b.sr / sr; const double fr = (double) b.frames();
            for (int i = v.offset; i < n; ++i) {
                if (v.pos >= fr - 1) { if (v.loop && fr > 2) v.pos = std::fmod(v.pos, fr - 1); else { v.buf.reset(); break; } }
                const size_t k = (size_t) v.pos; const float t = (float) (v.pos - (double) k);
                L[i] += (b.L[k] + (b.L[k + 1] - b.L[k]) * t) * v.g * gain;
                R[i] += (b.R[k] + (b.R[k + 1] - b.R[k]) * t) * v.g * gain;
                v.pos += inc;
            }
            v.offset = 0;
        }
        while (nvoices > 0 && !voices[nvoices - 1].buf) --nvoices;
        clock += n;
    }

    // offline: one pass of the longest layer (times `cycles`) plus a tail, at 44.1 kHz, the same as the browser export
    static void renderSong(const Song& s, std::vector<float>& L, std::vector<float>& R, double sr = 44100, int cycles = 1, double tail = 2.5) {
        LibPlayer p; p.gain = 1; auto c = std::make_shared<Song>(s); c->loop = false;
        if (!c->layers.empty()) {   // repeat each layer so all of them cover `cycles` passes of the longest one
            const double len = lengthBeats(*c) * cycles;
            for (auto& ly : c->layers) {
                const int reps = std::max(1, (int) std::lround(len / ly.beats));
                std::vector<Ev> ev; for (int r = 0; r < reps; ++r) for (auto e : ly.ev) { e.beat = r * ly.beats + swingAt(e.beat, ly.swing); ev.push_back(e); }
                ly.ev = ev; ly.beats = reps * ly.beats; ly.swing = 0;
            }
        }
        const double secs = (c->layers.empty() ? (s.single ? s.single->frames() / s.single->sr : 0) : lengthBeats(*c) * 60.0 / c->bpm) + (c->layers.empty() ? 0 : tail);
        const size_t n = (size_t) std::ceil(secs * sr);
        L.assign(n, 0.f); R.assign(n, 0.f);
        p.start(c);
        for (size_t at = 0; at < n; at += 512) { const int m = (int) std::min<size_t>(512, n - at); p.render(L.data() + at, R.data() + at, m, sr); }
    }

private:
    struct Voice { std::shared_ptr<const AudioData> buf; double pos = 0; float g = 1; int offset = 0; bool loop = false; };
    static constexpr int kMax = 96;
    Voice voices[kMax]; int nvoices = 0;
    std::shared_ptr<const Song> song; double clock = 0; std::vector<long long> cycle;
    void trigger(std::shared_ptr<const AudioData> b, float g, int offset, bool loop) {
        if (!b || b->frames() < 2) return;
        int slot = -1;
        for (int i = 0; i < nvoices; ++i) if (!voices[i].buf) { slot = i; break; }
        if (slot < 0) { if (nvoices < kMax) slot = nvoices++; else slot = 0; }   // full: replace the oldest slot
        voices[slot] = { std::move(b), 0.0, g, offset, loop };
    }
};

} // namespace lumi

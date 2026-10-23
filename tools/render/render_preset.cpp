// Offline renderer: plays a note list through the LUMI engine and writes a 24-bit stereo WAV.
// usage: render_preset <params.txt> <notes.txt> <out.wav> <bpm> [tail_seconds]
//   params.txt : "key value" per line (flattened preset JSON)
//   notes.txt  : "start_sec dur_sec midi_note velocity(0-1)" per line; "bend time value(-1..1)" lines optional
#include "../../engine/LumiEngine.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>

struct Ev { double t; int type; int note; float val; };   // type 0 on, 1 off, 2 bend

static void writeWav(const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr) {
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::perror(path); std::exit(1); }
    uint32_t n = (uint32_t)L.size(), dataBytes = n * 2 * 3;
    auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); w32(36 + dataBytes); std::fwrite("WAVE", 1, 4, f);
    std::fwrite("fmt ", 1, 4, f); w32(16); w16(1); w16(2); w32(sr); w32(sr * 6); w16(6); w16(24);
    std::fwrite("data", 1, 4, f); w32(dataBytes);
    for (uint32_t i = 0; i < n; ++i) {
        for (float s : {L[i], R[i]}) {
            int32_t v = (int32_t)std::lround(lumi::clampf(s, -1.f, 0.9999999f) * 8388607.f);
            uint8_t b[3] = {(uint8_t)(v & 255), (uint8_t)((v >> 8) & 255), (uint8_t)((v >> 16) & 255)};
            std::fwrite(b, 1, 3, f);
        }
    }
    std::fclose(f);
}

int main(int argc, char** argv) {
    if (argc < 5) { std::cerr << "usage: render_preset params notes out.wav bpm [tail]\n"; return 1; }
    const int sr = 44100;
    float bpm = std::stof(argv[4]);
    float tail = argc > 5 ? std::stof(argv[5]) : 3.f;

    lumi::ParamMap p;
    { std::ifstream in(argv[1]); std::string k; float v; while (in >> k >> v) p.v[k] = v; }

    std::vector<Ev> evs; double end = 0;
    {
        std::ifstream in(argv[2]); std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            if (line.rfind("bend", 0) == 0) { std::string w; double t; float v; ss >> w >> t >> v; evs.push_back({t, 2, 0, v}); continue; }
            double t, d; int n; float vel; ss >> t >> d >> n >> vel;
            evs.push_back({t, 0, n, vel}); evs.push_back({t + d, 1, n, 0});
            end = std::max(end, t + d);
        }
    }
    std::stable_sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && a.type > b.type); });

    lumi::Engine eng;
    eng.prepare((float)sr);
    eng.loadPreset(p, bpm);

    size_t total = (size_t)((end + tail) * sr);
    std::vector<float> L(total), R(total);
    size_t ei = 0; const int block = 32;
    for (size_t pos = 0; pos < total; pos += block) {
        while (ei < evs.size() && evs[ei].t * sr <= (double)pos) {
            const Ev& e = evs[ei++];
            if (e.type == 0) eng.noteOn(e.note, e.val);
            else if (e.type == 1) eng.noteOff(e.note);
            else eng.setPitchBend(e.val);
        }
        int n = (int)std::min<size_t>(block, total - pos);
        eng.process(&L[pos], &R[pos], n);
    }
    // report peak / NaN guard
    float peak = 0; bool bad = false;
    for (size_t i = 0; i < total; ++i) { if (!std::isfinite(L[i]) || !std::isfinite(R[i])) { bad = true; L[i] = R[i] = 0; } peak = std::max(peak, std::max(std::fabs(L[i]), std::fabs(R[i]))); }
    // float-safe: scale down if engine output exceeds full scale (loudness matching happens later)
    if (peak > 0.98f) { float g = 0.98f / peak; for (size_t i = 0; i < total; ++i) { L[i] *= g; R[i] *= g; } }
    writeWav(argv[3], L, R, sr);
    std::printf("peak=%.3f nan=%d secs=%.2f\n", peak, bad ? 1 : 0, total / (double)sr);
    return 0;
}

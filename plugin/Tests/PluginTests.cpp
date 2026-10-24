// Headless checks of the LUMELODY processor: notes from the host's piano roll, every sound, automation, state.
// Exit code 0 = all passed.  Run: build/LumiTests_artefacts/Release/LumiTests
#include "PluginProcessor.h"
#include <cstdio>

using namespace juce;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL %s:%d  ", __FILE__, __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

struct Out { std::vector<float> L, R; float peak() const { float p = 0; for (size_t i = 0; i < L.size(); ++i) p = jmax (p, std::abs (L[i]), std::abs (R[i])); return p; }
             bool finite() const { for (size_t i = 0; i < L.size(); ++i) if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) return false; return true; }
             float rms (size_t a, size_t b) const { double s = 0; b = jmin (b, L.size()); for (size_t i = a; i < b; ++i) s += L[i] * L[i] + R[i] * R[i]; return b > a ? (float) std::sqrt (s / (2.0 * (double) (b - a))) : 0.f; } };

struct Ev { int sample; MidiMessage msg; };

// render `seconds` of audio in host-sized blocks, delivering the events at their exact sample positions
static Out run (LumiProcessor& p, std::vector<Ev> evs, double seconds, int block = 512)
{
    const int total = (int) (seconds * 44100);
    Out o; o.L.resize ((size_t) total); o.R.resize ((size_t) total);
    AudioBuffer<float> buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        buf.setSize (2, n, false, false, true);
        MidiBuffer midi;
        for (auto& e : evs) if (e.sample >= pos && e.sample < pos + n) midi.addEvent (e.msg, e.sample - pos);
        p.processBlock (buf, midi);
        std::memcpy (&o.L[(size_t) pos], buf.getReadPointer (0), sizeof (float) * (size_t) n);
        std::memcpy (&o.R[(size_t) pos], buf.getReadPointer (1), sizeof (float) * (size_t) n);
    }
    return o;
}

static void pump() { MessageManager::getInstance()->runDispatchLoopUntil (30); }

int main()
{
    ScopedJuceInitialiser_GUI gui;

    {   // ---- every sound plays a note from the piano roll: audible, finite, never clipping
        LumiProcessor p; p.prepareToPlay (44100, 512);
        CHECK (p.getNumPrograms() == lumi::kNumPresets, "programs %d", p.getNumPrograms());
        int quiet = 0;
        for (int i = 0; i < lumi::kNumPresets; ++i)
        {
            p.selectPreset (i);
            const int note = jlimit (0, 127, lumi::kPresets[i].baseNote + 12);
            auto o = run (p, { { 100, MidiMessage::noteOn (1, note, (uint8) 100) }, { 22050, MidiMessage::noteOff (1, note) } }, 1.5);
            const float pk = o.peak();
            CHECK (o.finite(), "%s: non-finite output", lumi::kPresets[i].name);
            CHECK (pk > 1e-3f, "%s: silent (peak %g)", lumi::kPresets[i].name, pk);
            CHECK (pk <= 1.0f, "%s: peak %g above full scale", lumi::kPresets[i].name, pk);
            if (pk < 0.02f) ++quiet;
            std::printf ("  %-18s peak %.3f\n", lumi::kPresets[i].name, pk);
            p.processBlock (*std::make_unique<AudioBuffer<float>> (2, 512), *std::make_unique<MidiBuffer>());
            MidiBuffer off; off.addEvent (MidiMessage::allNotesOff (1), 0);
            AudioBuffer<float> b (2, 512); p.processBlock (b, off);
            for (int k = 0; k < 400; ++k) { AudioBuffer<float> t (2, 512); MidiBuffer m; p.processBlock (t, m); }   // let tails die
        }
        std::printf ("sounds checked: %d (%d quiet)\n", lumi::kNumPresets, quiet);
    }

    {   // ---- the plugin plays exactly what the engine plays (same sound, same note)
        LumiProcessor p; p.selectPreset (3); p.prepareToPlay (44100, 512);   // as when a project opens: the engine starts on this sound
        auto o = run (p, { { 0, MidiMessage::noteOn (1, 60, 0.8f) }, { 30000, MidiMessage::noteOff (1, 60) } }, 1.2, 300);
        lumi::Engine e; e.prepare (44100);
        lumi::ParamMap pm; { const char* s = lumi::kPresets[3].params; std::string t (s); size_t a = 0;
            while (a < t.size()) { size_t nl = t.find ('\n', a); if (nl == std::string::npos) nl = t.size(); auto ln = t.substr (a, nl - a); auto sp = ln.find (' ');
                if (sp != std::string::npos) pm.v[ln.substr (0, sp)] = std::strtof (ln.c_str() + sp + 1, nullptr); a = nl + 1; } }
        e.loadPreset (pm, 120);
        std::vector<float> L (o.L.size()), R (o.L.size());
        e.noteOn (60, 0.8f); e.process (L.data(), R.data(), 30000); e.noteOff (60); e.process (L.data() + 30000, R.data() + 30000, (int) L.size() - 30000);
        float md = 0; for (size_t i = 0; i < L.size(); ++i) md = jmax (md, std::abs (L[i] - o.L[i]), std::abs (R[i] - o.R[i]));
        { size_t first = 0, at = 0; float best = 0; for (size_t i = 0; i < L.size(); ++i) { float d = std::abs (L[i] - o.L[i]); if (d > 1e-6f && ! first) first = i; if (d > best) { best = d; at = i; } }
          std::printf ("  first diff at %zu, max at %zu (engine %g plugin %g)\n", first, at, L[at], o.L[at]); }
        // a sound's own reverb/delay values can sit slightly outside the knob ranges; as host parameters they are kept
        // inside them, which changes the tail by a hair (well under -50 dBFS) -- anything bigger is a real mismatch
        CHECK (md < 3e-3f, "plugin vs engine max difference %g", md);
        std::printf ("plugin == engine: max diff %g\n", md);
    }

    {   // ---- notes start on their exact sample inside a block
        LumiProcessor p; p.prepareToPlay (44100, 512); p.selectPreset (0);
        auto o = run (p, { { 300, MidiMessage::noteOn (1, 60, (uint8) 110) } }, 0.1);
        float before = 0; for (int i = 0; i < 300; ++i) before = jmax (before, std::abs (o.L[(size_t) i]));
        CHECK (before == 0.0f, "sound before the note (%g)", before);
        CHECK (o.rms (300, 2000) > 1e-4f, "no sound after the note");
    }

    {   // ---- sustain pedal holds released notes, lifting it lets them go
        LumiProcessor p; p.prepareToPlay (44100, 512);
        int pad = 0; for (int i = 0; i < lumi::kNumPresets; ++i) if (String (lumi::kPresets[i].category) == "Pads & Atmospheres") { pad = i; break; }
        p.selectPreset (pad);
        auto held = run (p, { { 0, MidiMessage::controllerEvent (1, 64, 127) }, { 10, MidiMessage::noteOn (1, 60, (uint8) 100) },
                               { 22050, MidiMessage::noteOff (1, 60) } }, 3.0);
        LumiProcessor q; q.prepareToPlay (44100, 512); q.selectPreset (pad);
        auto free = run (q, { { 10, MidiMessage::noteOn (1, 60, (uint8) 100) }, { 22050, MidiMessage::noteOff (1, 60) } }, 3.0);
        const float h = held.rms (110250, 132300), f = free.rms (110250, 132300);
        CHECK (h > f * 1.5f, "sustain pedal did not hold (held %g, free %g)", h, f);
        std::printf ("sustain (%s): held %.4f vs released %.4f\n", lumi::kPresets[pad].name, h, f);
    }

    {   // ---- automation of an effect control reaches the sound
        LumiProcessor p; p.prepareToPlay (44100, 512);
        auto* amt = p.apvts.getParameter ("chorus_rate"); auto* on = p.apvts.getParameter ("chorus_on");
        on->setValueNotifyingHost (1.0f); amt->setValueNotifyingHost (amt->convertTo0to1 (2.5f));
        run (p, {}, 0.02);
        const String st = p.fxStateString();
        CHECK (st.contains ("chorus.on=1;") && st.contains ("chorus.rate=2.5;"), "automation not applied: %s", st.toRawUTF8());
    }

    {   // ---- changing the sound from the host (automation / program list)
        LumiProcessor p; p.prepareToPlay (44100, 512);
        p.setCurrentProgram (7); pump();
        CHECK (p.currentPresetIndex() == 7, "program change -> %d", p.currentPresetIndex());
        run (p, {}, 0.02);
    }

    {   // ---- project save / reopen keeps the sound, the effects and the volume
        LumiProcessor p; p.prepareToPlay (44100, 512);
        p.selectPreset (12);
        p.setFxOn (lumi::FX_CHORUS, true); p.setFxParam (lumi::FX_CHORUS, 1, 1.7f); p.setFxParam (lumi::FX_REVERB, 0, 0.66f);
        p.apvts.getParameter ("volume")->setValueNotifyingHost (p.apvts.getParameter ("volume")->convertTo0to1 (-6.0f));
        run (p, {}, 0.02);
        const String before = p.fxStateString();
        MemoryBlock mb; p.getStateInformation (mb);

        LumiProcessor q; q.prepareToPlay (44100, 512);
        q.setStateInformation (mb.getData(), (int) mb.getSize()); pump();
        run (q, {}, 0.02);
        CHECK (q.currentPresetIndex() == 12, "sound after reopen %d", q.currentPresetIndex());
        CHECK (q.fxStateString() == before, "effects differ after reopen:\n  %s\n  %s", before.toRawUTF8(), q.fxStateString().toRawUTF8());
        CHECK (std::abs (q.apvts.getRawParameterValue ("volume")->load() + 6.0f) < 0.01f, "volume after reopen");
        // and the restored plugin sounds identical to the original
        auto a = run (p, { { 0, MidiMessage::noteOn (1, 60, (uint8) 90) } }, 0.5), b = run (q, { { 0, MidiMessage::noteOn (1, 60, (uint8) 90) } }, 0.5);
        float md = 0; for (size_t i = 0; i < a.L.size(); ++i) md = jmax (md, std::abs (a.L[i] - b.L[i]));
        CHECK (md < 1e-4f, "reopened plugin sounds different (%g)", md);
    }

    {   // ---- odd host block sizes and sample rates
        for (double sr : { 48000.0, 96000.0 })
        {
            LumiProcessor p; p.prepareToPlay (sr, 64); p.selectPreset (1);
            auto o = run (p, { { 5, MidiMessage::noteOn (1, 64, (uint8) 100) } }, 0.3, 37);
            CHECK (o.finite() && o.peak() > 1e-3f && o.peak() <= 1.0f, "sr %g peak %g", sr, o.peak());
        }
        LumiProcessor p; p.prepareToPlay (44100, 256); p.selectPreset (1);
        auto o = run (p, { { 5, MidiMessage::noteOn (1, 64, (uint8) 100) } }, 0.3, 2048);   // bigger than announced
        CHECK (o.finite() && o.peak() > 1e-3f, "large block");
    }

    {   // ---- audio export renders the notes with the current sound
        LumiProcessor p; p.prepareToPlay (44100, 512); p.selectPreset (2);
        std::vector<LumiProcessor::RenderNote> notes { { 0.0, 2.0, 60, 0.8f }, { 0.0, 2.0, 64, 0.7f }, { 2.0, 2.0, 67, 0.7f } };
        auto a = p.renderOffline (notes, 120.0, 4.0);
        CHECK (a.getNumSamples() == (int) std::ceil ((2.0 + 4.0) * 44100.0), "render length %d", a.getNumSamples());
        const float pk = a.getMagnitude (0, a.getNumSamples());
        CHECK (pk > 1e-3f && pk <= 0.981f, "render peak %g", pk);
        std::printf ("offline render: %d samples, peak %.3f\n", a.getNumSamples(), pk);
    }

    std::printf (failures ? "\n%d FAILED\n" : "\nALL PASSED\n", failures);
    return failures ? 1 : 0;
}

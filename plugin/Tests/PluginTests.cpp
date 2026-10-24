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

    {   // ================= SAMPLER inside the plugin: real files on disk, MIDI at the right time and pitch, reverse, project reload
        auto tmp = File::getSpecialLocation (File::tempDirectory).getChildFile ("lumi-sampler-test"); tmp.deleteRecursively(); tmp.createDirectory();
        auto writeWav = [&] (const File& f, std::function<float (int, int)> gen, int n, double sr = 44100.0)
        {
            AudioBuffer<float> b (2, n); for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) b.setSample (c, i, gen (c, i));
            WavAudioFormat wav; std::unique_ptr<OutputStream> os (f.createOutputStream().release());
            std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)); os.release(); w->writeFromAudioSampleBuffer (b, 0, n);
        };
        const File sine = tmp.getChildFile ("sine440.wav"), sweep = tmp.getChildFile ("sweep.wav");
        writeWav (sine, [] (int c, int i) { return (c ? .4f : .5f) * (float) std::sin (2 * MathConstants<double>::pi * 440 * i / 44100.0); }, 88200);
        double ph = 0; std::vector<float> sw (88200); for (int i = 0; i < 88200; ++i) { ph += 2 * MathConstants<double>::pi * (200 + 600.0 * i / 88200) / 44100; sw[(size_t) i] = .5f * (float) std::sin (ph); }
        writeWav (sweep, [&] (int c, int i) { return sw[(size_t) i] * (c ? .5f : 1.f); }, 88200);
        tmp.getChildFile ("empty.wav").create();
        tmp.getChildFile ("corrupt.wav").replaceWithText ("RIFF....WAVEfmt this is not audio at all");

        auto d = lumifiles::decodeFile (sine);
        CHECK (d.audio != nullptr && d.audio->frames() == 88200 && d.audio->channels == 2, "decode wav: %s", d.error.toRawUTF8());
        CHECK (lumifiles::decodeFile (tmp.getChildFile ("empty.wav")).error.contains ("empty"), "empty file message");
        CHECK (lumifiles::decodeFile (tmp.getChildFile ("corrupt.wav")).error.contains ("decode"), "corrupt file message");
        CHECK (lumifiles::decodeFile (tmp.getChildFile ("missing.wav")).error.contains ("not found"), "missing file message");

        auto freq = [] (const std::vector<float>& x, size_t a, size_t b) { std::vector<double> z; for (size_t i = a + 1; i < b; ++i) if (x[i - 1] < 0 && x[i] >= 0) z.push_back ((double) (i - 1) + x[i - 1] / (x[i - 1] - x[i])); return z.size() < 3 ? 0.0 : 44100.0 * (double) (z.size() - 1) / (z.back() - z.front()); };
        {
            LumiProcessor p; p.prepareToPlay (44100, 512);
            CHECK (p.setSamplerSample (d.audio, "id1", "sine440", sine.getFullPathName(), "root=69;release=0.01;", false).isEmpty(), "load sample");
            p.apvts.getParameter (LumiProcessor::kLayerId)->setValueNotifyingHost (p.apvts.getParameter (LumiProcessor::kLayerId)->convertTo0to1 (1));
            p.applyFxState ("reverb.on=0;delay.on=0;");
            auto o = run (p, { { 300, MidiMessage::noteOn (1, 81, (uint8) 110) }, { 300 + 30000, MidiMessage::noteOff (1, 81) } }, 1.0);
            float before = 0; for (int i = 0; i < 300; ++i) before = jmax (before, std::abs (o.L[(size_t) i]));
            const double f = freq (o.L, 2000, 28000);
            CHECK (before == 0.0f, "sampler sounded before its note");
            CHECK (std::abs (f - 880) < 2, "MIDI note 81 on a 440 Hz sample with root 69: %.2f Hz", f);
            std::printf ("plugin sampler: note 81 -> %.2f Hz, starts exactly at its sample offset\n", f);
            // layer: piano + sampler both sound; sampler-only has no piano
            auto rmsOf = [&] (int mode) { p.apvts.getParameter (LumiProcessor::kLayerId)->setValueNotifyingHost (p.apvts.getParameter (LumiProcessor::kLayerId)->convertTo0to1 ((float) mode));
                                          auto x = run (p, { { 0, MidiMessage::noteOn (1, 69, (uint8) 110) }, { 30000, MidiMessage::noteOff (1, 69) } }, 1.5); MidiBuffer m; m.addEvent (MidiMessage::allNotesOff (1), 0); AudioBuffer<float> b (2, 512); p.processBlock (b, m);
                                          for (int k = 0; k < 200; ++k) { AudioBuffer<float> t (2, 512); MidiBuffer e; p.processBlock (t, e); } return x.rms (2000, 28000); };
            const float rp = rmsOf (0), rs = rmsOf (1), rl = rmsOf (2);
            CHECK (rp > 1e-3f && rs > 1e-3f && rl > jmax (rp, rs), "layer modes: piano %.4f sampler %.4f layer %.4f", rp, rs, rl);
            std::printf ("plugin layer: piano %.3f, sampler %.3f, both %.3f\n", rp, rs, rl);
            // reverse: the processed audio is an exact mirror; original untouched
            p.applySamplerSettings ("root=69;reverse=1;", true);
            auto pb = p.samplerProcessed(); bool exact = pb && pb->frames() == d.audio->frames();
            for (size_t i = 0; exact && i < pb->frames(); ++i) exact = pb->L[i] == d.audio->L[pb->frames() - 1 - i] && pb->R[i] == d.audio->R[pb->frames() - 1 - i];
            CHECK (exact, "plugin reverse is not an exact stereo mirror");
            CHECK (p.samplerInfo().orig->L[10] == d.audio->L[10], "original audio changed");
            // offline audio export includes the sampler
            p.applySamplerSettings ("root=69;", true);
            p.apvts.getParameter (LumiProcessor::kLayerId)->setValueNotifyingHost (p.apvts.getParameter (LumiProcessor::kLayerId)->convertTo0to1 (1));
            run (p, {}, 0.02);
            auto a = p.renderOffline ({ { 0.0, 2.0, 69, 0.9f } }, 120.0, 2.0);
            std::vector<float> L (a.getReadPointer (0), a.getReadPointer (0) + a.getNumSamples());
            CHECK (std::abs (freq (L, 2000, 30000) - 440) < 2, "offline render of the sampler: %.2f Hz", freq (L, 2000, 30000));
            // project save / reopen reloads the file and its edits
            p.applySamplerSettings ("root=64;reverse=1;slices=0.25,0.5;gainDb=-3;", true);
            MemoryBlock mb; p.getStateInformation (mb);
            LumiProcessor q; q.prepareToPlay (44100, 512); q.setStateInformation (mb.getData(), (int) mb.getSize());
            CHECK (q.samplerInfo().orig != nullptr && q.samplerInfo().path == sine.getFullPathName(), "sample not reloaded with the project");
            CHECK (q.samplerSettingsText() == p.samplerSettingsText(), "sampler settings differ after reopen:\n %s\n %s", p.samplerSettingsText().toRawUTF8(), q.samplerSettingsText().toRawUTF8());
            CHECK (roundToInt (q.apvts.getRawParameterValue (LumiProcessor::kLayerId)->load()) == 1, "layer mode not restored");
            // the file disappears: the project still opens, says why, and keeps the path
            const File moved = tmp.getChildFile ("moved.wav"); sine.moveFileTo (moved);
            LumiProcessor r; r.prepareToPlay (44100, 512); r.setStateInformation (mb.getData(), (int) mb.getSize());
            CHECK (r.samplerInfo().orig == nullptr && r.samplerError().contains ("not found") && r.samplerInfo().path == sine.getFullPathName(), "missing file handling: %s", r.samplerError().toRawUTF8());
            auto silent = run (r, { { 0, MidiMessage::noteOn (1, 60, (uint8) 100) } }, 0.2);
            CHECK (silent.finite(), "processor with a missing sample must keep running");
            moved.moveFileTo (sine);
        }
        {   // WAV export of an edited sample reopens with the right content
            LumiProcessor p; p.prepareToPlay (44100, 512);
            auto ds = lumifiles::decodeFile (sweep);
            p.setSamplerSample (ds.audio, "id2", "sweep", sweep.getFullPathName(), "reverse=1;start=0;end=0.5;", false);
            auto pb = p.samplerProcessed(); const int a = 0, b = (int) std::ceil (.5 * (double) pb->frames());
            AudioBuffer<float> out (2, b - a); for (int i = a; i < b; ++i) { out.setSample (0, i, pb->L[(size_t) i]); out.setSample (1, i, pb->R[(size_t) i]); }
            const File f = tmp.getChildFile ("export.wav");
            { WavAudioFormat wav; std::unique_ptr<OutputStream> os (f.createOutputStream().release()); std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (os.get(), 44100, 2, 24, {}, 0)); os.release(); w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples()); }
            auto back = lumifiles::decodeFile (f);
            CHECK (back.audio && back.audio->frames() == (size_t) (b - a), "exported length");
            const double f0 = freq (back.audio->L, 1000, 6000);
            CHECK (f0 > 700, "exported file should start with the end of the sweep (reversed): %.0f Hz", f0);
            double ratio = 0; for (int i = 1000; i < 20000; ++i) ratio += back.audio->L[(size_t) i] != 0 ? back.audio->R[(size_t) i] / back.audio->L[(size_t) i] : .5; ratio /= 19000;
            CHECK (std::abs (ratio - .5) < .01, "stereo alignment in the exported reverse: R/L = %.3f", ratio);
        }
        tmp.deleteRecursively();
    }

    {   // ---- sound libraries: embedded factory samples, previews through the plugin's output, tempo-synced patterns, export
        LumiProcessor p; p.prepareToPlay (44100, 512);
        const char* ids[] = { "dd-tambora-palito", "dd-guira-largo", "dc-floor-kick", "nj-hat-tight", "rg-dembow-kick" };
        int found = 0;
        for (auto* id : ids) if (auto a = p.librarySample (id)) { ++found; CHECK (a->frames() > 1000 && a->sr == 44100, "%s decoded badly", id); }
        CHECK (found >= 4, "embedded library samples found: %d of 5", found);
        CHECK (p.librarySample ("../../etc/passwd") == nullptr && p.librarySample ("nope") == nullptr, "unknown ids must not resolve");
        const auto kick = p.librarySample ("dc-floor-kick");
        CHECK (kick != nullptr, "dc-floor-kick missing");
        if (kick != nullptr)
        {
            // a 4-on-the-floor pattern at 120 bpm: hits every 22050 samples, starting right at the first block
            auto mk = [] (double bpm) {
                DynamicObject::Ptr L = new DynamicObject(); Array<var> ev;
                for (int b = 0; b < 4; ++b) { DynamicObject::Ptr e = new DynamicObject(); e->setProperty ("s", "dc-floor-kick"); e->setProperty ("b", b); e->setProperty ("v", 1.0); ev.add (var (e.get())); }
                L->setProperty ("events", ev); L->setProperty ("beats", 4); L->setProperty ("swing", 0); L->setProperty ("vol", 1.0);
                DynamicObject::Ptr m = new DynamicObject(); m->setProperty ("layers", Array<var> { var (L.get()) }); m->setProperty ("bpm", bpm); m->setProperty ("loop", true);
                return var (m.get());
            };
            String err; auto song = p.librarySong (mk (120), err);
            CHECK (song != nullptr && err.isEmpty(), "pattern song: %s", err.toRawUTF8());
            p.setNonRealtime (false);
            p.libraryGain (1.0f); p.libraryStart (song);
            p.apvts.getParameter ("volume")->setValueNotifyingHost (p.apvts.getParameter ("volume")->convertTo0to1 (0.0f));
            auto o = run (p, {}, 2.5);
            // the reference: the kick placed by hand every 22050 samples (120 bpm), looping after 4 beats
            std::vector<float> ref (o.L.size(), 0.0f);
            for (size_t at = 0; at < ref.size(); at += 22050)
                for (size_t i = 0; i < kick->frames() && at + i < ref.size(); ++i) ref[at + i] += kick->L[i];
            float md = 0, pk = 0; for (size_t i = 0; i < ref.size(); ++i) { md = jmax (md, std::abs (o.L[i] - ref[i])); pk = jmax (pk, std::abs (ref[i])); }
            CHECK (pk > 0.5f && md < 1e-4f, "pattern != kicks at exact 120 bpm positions (max diff %g, peak %g)", md, pk);
            std::printf ("library pattern: 4 bars of 120 bpm kicks match the hand-placed reference to %g (peak %.2f)\n", md, pk);
            p.libraryStop();
            auto quiet = run (p, {}, 0.3);
            CHECK (quiet.peak() < 1e-6f, "library stop leaves silence (%g)", quiet.peak());
            // export: render to WAV, same timing at 90 bpm, file reopens
            const File f = File::getSpecialLocation (File::tempDirectory).getChildFile ("lumi-lib-test.wav");
            CHECK (p.libraryRenderWav (mk (90), f).isEmpty(), "pattern export");
            auto back = lumifiles::decodeFile (f);
            CHECK (back.audio != nullptr && std::abs ((double) back.audio->frames() - (4 * 60.0 / 90 + 2.5) * 44100) < 2, "export length %d", back.audio ? (int) back.audio->frames() : -1);
            if (back.audio)
            {
                std::vector<float> ref (back.audio->frames(), 0.0f);
                for (int k = 0; k < 4; ++k) { const size_t at = (size_t) std::llround (k * 60.0 / 90 * 44100); for (size_t i = 0; i < kick->frames() && at + i < ref.size(); ++i) ref[at + i] += kick->L[i]; }
                float pk = 0; for (auto v : ref) pk = jmax (pk, std::abs (v)); const float g = pk > 0.98f ? 0.98f / pk : 1.0f;
                float md = 0; for (size_t i = 0; i < ref.size(); ++i) md = jmax (md, std::abs (back.audio->L[i] - ref[i] * g));
                CHECK (md < 1e-4f, "exported 90 bpm pattern differs from the reference by %g", md);
            }
            DynamicObject::Ptr one = new DynamicObject(); one->setProperty ("id", "dc-floor-kick");
            CHECK (p.libraryRenderWav (var (one.get()), f).isEmpty(), "one-shot export");
            auto b2 = lumifiles::decodeFile (f);
            CHECK (b2.audio && b2.audio->frames() == kick->frames(), "one-shot exported exactly (%d vs %d)", b2.audio ? (int) b2.audio->frames() : -1, (int) kick->frames());
            DynamicObject::Ptr bad = new DynamicObject(); bad->setProperty ("id", "missing-sound");
            CHECK (p.libraryRenderWav (var (bad.get()), f).contains ("not in the plugin"), "missing sound reports an error");
            f.deleteFile();
        }
    }

    std::printf (failures ?"\n%d FAILED\n" : "\nALL PASSED\n", failures);
    return failures ? 1 : 0;
}

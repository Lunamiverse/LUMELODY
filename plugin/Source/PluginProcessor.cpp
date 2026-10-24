#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>

using namespace juce;

namespace
{
    constexpr int kOnSlot = 0;   // slot 0 of each effect is its on/off switch, slots 1..8 its controls
    int refIndex[lumi::FX_COUNT][lumi::kMaxParams + 1];

    String pretty (float v, const char* unit)
    {
        const String u (unit);
        if (u == "%")  return String (roundToInt (v * 100.0f)) + " %";
        if (u == "Hz") return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (v, v < 10 ? 2 : 0) + " Hz";
        if (u == "ms") return String (v, v < 10 ? 1 : 0) + " ms";
        if (u == "s")  return String (v, 2) + " s";
        if (u == "st") return String (v, 1) + " st";
        if (u == "dB") return String (v, 1) + " dB";
        if (u.isNotEmpty()) return String (v, 2) + " " + u;
        return String (v, 2);
    }
}

String LumiProcessor::paramId (int fx, int param) { return String (lumi::fxSchema()[(size_t) fx].id) + "_" + lumi::fxSchema()[(size_t) fx].p[(size_t) param].id; }
String LumiProcessor::onId (int fx)               { return String (lumi::fxSchema()[(size_t) fx].id) + "_on"; }

AudioProcessorValueTreeState::ParameterLayout LumiProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray names;
    for (const auto& p : lumi::kPresets) names.add (p.name);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { kSoundId, 1 }, "Sound", names, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { kVolumeId, 1 }, "Volume",
                                                       NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { kLayerId, 1 }, "Keys Play", StringArray { "Piano", "Sampler", "Layer" }, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { kPianoLvlId, 1 }, "Piano Level", NormalisableRange<float> (0.0f, 1.5f, 0.001f), 1.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { kSamplerLvlId, 1 }, "Sampler Level", NormalisableRange<float> (0.0f, 1.5f, 0.001f), 0.7f));

    const auto& schema = lumi::fxSchema();
    for (int e = 0; e < lumi::FX_COUNT; ++e)
    {
        const auto& fx = schema[(size_t) e];
        auto group = std::make_unique<AudioProcessorParameterGroup> (fx.id, fx.name, " | ");
        group->addChild (std::make_unique<AudioParameterBool> (ParameterID { onId (e), 1 }, String (fx.name) + " On", false));

        for (int i = 0; i < (int) fx.p.size(); ++i)
        {
            const auto& d = fx.p[(size_t) i];
            const String id = paramId (e, i), name = String (fx.name) + " " + d.name;

            if (d.kind == lumi::P_ENUM)
            {
                group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name,
                                                                         StringArray::fromTokens (d.choices, "|", ""), roundToInt (d.def)));
            }
            else if (d.kind == lumi::P_TOGGLE)
            {
                group->addChild (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, d.def > 0.5f));
            }
            else
            {
                NormalisableRange<float> range (d.min, d.max);
                if (d.log && d.min > 0) range.setSkewForCentre (std::sqrt (d.min * d.max));
                const char* unit = d.unit;
                group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, d.def,
                    AudioParameterFloatAttributes()
                        .withLabel (d.unit)
                        .withStringFromValueFunction ([unit] (float v, int) { return pretty (v, unit); })));
            }
        }
        layout.add (std::move (group));
    }
    return layout;
}

LumiProcessor::LumiProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LUMELODY", createLayout())
{
    for (auto& row : refIndex) for (auto& x : row) x = -1;
    const auto& schema = lumi::fxSchema();
    for (int e = 0; e < lumi::FX_COUNT; ++e)
    {
        refIndex[e][kOnSlot] = (int) fxRefs.size();
        fxRefs.push_back ({ e, 0, true, apvts.getRawParameterValue (onId (e)), apvts.getParameter (onId (e)), -1.0f });
        for (int i = 0; i < (int) schema[(size_t) e].p.size(); ++i)
        {
            refIndex[e][i + 1] = (int) fxRefs.size();
            fxRefs.push_back ({ e, i, false, apvts.getRawParameterValue (paramId (e, i)), apvts.getParameter (paramId (e, i)), -1e30f });
        }
    }
    for (auto& r : fxRefs) jassert (r.raw != nullptr && r.p != nullptr);
    volumeRaw = apvts.getRawParameterValue (kVolumeId);
    layerRaw = apvts.getRawParameterValue (kLayerId); pianoLvlRaw = apvts.getRawParameterValue (kPianoLvlId); samplerLvlRaw = apvts.getRawParameterValue (kSamplerLvlId);
    soundParam = apvts.getParameter (kSoundId);
    apvts.addParameterListener (kSoundId, this);

    engine = std::make_unique<lumi::Engine>();
    engine->prepare ((float) sampleRate);
    {
        const ScopedLock sl (engineLock);
        loadEngine (0);
        loadedPreset = 0;
    }
    syncParamsFromEngine();
    startTimerHz (15);
}

LumiProcessor::~LumiProcessor()
{
    stopTimer();
    cancelPendingUpdate();
    apvts.removeParameterListener (kSoundId, this);
}

bool LumiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet().isDisabled()
        && (out == AudioChannelSet::stereo() || out == AudioChannelSet::mono());
}

int LumiProcessor::currentPresetIndex() const
{
    return jlimit (0, lumi::kNumPresets - 1, roundToInt (apvts.getRawParameterValue (kSoundId)->load()));
}

// ------------------------------------------------------------------------------------------------ engine
lumi::ParamMap LumiProcessor::presetParams (int index)
{
    index = jlimit (0, lumi::kNumPresets - 1, index);
    lumi::ParamMap pm;
    const char* s = lumi::kPresets[index].params;
    while (*s)
    {
        while (*s == '\n' || *s == ' ' || *s == '\r') ++s;
        if (! *s) break;
        const char* k = s;
        while (*s && *s != ' ') ++s;
        if (! *s) break;
        std::string key (k, (size_t) (s - k));
        char* end = nullptr;
        pm.v[key] = std::strtof (s, &end);
        s = end;
        while (*s && *s != '\n') ++s;
    }
    return pm;
}

void LumiProcessor::loadEngine (int index)
{
    engine->loadPreset (presetParams (index), lastBpm > 0 ? (float) lastBpm : 120.0f);
    sustainDown = false;
    for (int n = 0; n < 128; ++n) keyDown[n] = sustained[n] = false;
}

AudioBuffer<float> LumiProcessor::renderOffline (const std::vector<RenderNote>& notes, double bpm, double beats, double sr) const
{
    bpm = jlimit (40.0, 240.0, bpm);
    const double spb = 60.0 / bpm;
    const int total = (int) std::ceil ((beats * spb + 4.0) * sr);
    AudioBuffer<float> out (2, jmax (1, total));
    out.clear();

    auto eng = std::make_unique<lumi::Engine>();
    eng->prepare ((float) sr);
    eng->loadPreset (presetParams (currentPresetIndex()), (float) bpm);
    eng->setTempo ((float) bpm);
    eng->rack.setState (fxStateString().toStdString());
    {
        const ScopedLock sl (engineLock);
        if (engine->sampler.hasSample()) eng->sampler.setPlayback (engine->sampler.playback(), engine->sampler.settings());
    }
    eng->setLayer (roundToInt (layerRaw->load()), pianoLvlRaw->load(), samplerLvlRaw->load());

    struct Ev { int64 at; bool on; int note; float vel; };
    std::vector<Ev> evs;
    for (const auto& n : notes)
    {
        evs.push_back ({ (int64) std::llround (n.t * spb * sr), true, jlimit (0, 127, n.note), jlimit (0.01f, 1.0f, n.vel) });
        evs.push_back ({ (int64) std::llround ((n.t + n.d) * spb * sr), false, jlimit (0, 127, n.note), 0.0f });
    }
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.at < b.at || (a.at == b.at && ! a.on && b.on); });

    size_t ei = 0;
    const int block = 64;
    for (int pos = 0; pos < total; pos += block)
    {
        while (ei < evs.size() && evs[ei].at <= pos)
        {
            const auto& e = evs[ei++];
            if (e.on) eng->noteOn (e.note, e.vel); else eng->noteOff (e.note);
        }
        const int n = jmin (block, total - pos);
        eng->process (out.getWritePointer (0, pos), out.getWritePointer (1, pos), n);
    }
    out.applyGain (Decibels::decibelsToGain (volumeRaw->load()));
    const float pk = out.getMagnitude (0, total);
    if (pk > 0.98f) out.applyGain (0.98f / pk);
    return out;
}

void LumiProcessor::pushParamsToEngine (bool force)
{
    {
        const float m = layerRaw->load(), pl = pianoLvlRaw->load(), sl = samplerLvlRaw->load();
        if (force || m != layerCache[0] || pl != layerCache[1] || sl != layerCache[2])
        {
            layerCache[0] = m; layerCache[1] = pl; layerCache[2] = sl;
            engine->setLayer (roundToInt (m), pl, sl);
        }
    }
    for (auto& r : fxRefs)
    {
        const float v = r.raw->load (std::memory_order_relaxed);
        if (! force && v == r.cache) continue;
        r.cache = v;
        if (r.isOn) engine->setOn (r.fx, v > 0.5f);
        else
        {
            engine->setParam (r.fx, r.param, v);
            // choosing a reverb type loads that space's decay / pre-delay / tone: show them on the controls too
            if (! force && r.fx == lumi::FX_REVERB && r.param == 1) needsSyncFromEngine = true;
        }
    }
}

void LumiProcessor::syncParamsFromEngine()
{
    for (auto& r : fxRefs)
    {
        const float v = r.isOn ? (engine->rack.on[r.fx] ? 1.0f : 0.0f) : engine->rack.val[r.fx][r.param];
        const float current = r.p->convertFrom0to1 (r.p->getValue());
        if (std::abs (current - v) > 1e-6f * (1.0f + std::abs (v)))
            r.p->setValueNotifyingHost (r.p->convertTo0to1 (v));
    }
}

void LumiProcessor::setParamFromUi (RangedAudioParameter* p, float plainValue)
{
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    p->endChangeGesture();
}

// ------------------------------------------------------------------------------------------------ sounds
void LumiProcessor::selectPreset (int index, const String& fxState)
{
    index = jlimit (0, lumi::kNumPresets - 1, index);
    {
        const ScopedLock sl (engineLock);
        loadEngine (index);
        if (fxState.isNotEmpty()) engine->rack.setState (fxState.toStdString());
        loadedPreset = index;
    }
    if (currentPresetIndex() != index) setParamFromUi (soundParam, (float) index);
    syncParamsFromEngine();
    ++presetChangeSerial;
}

void LumiProcessor::applyFxState (const String& fxState)
{
    {
        const ScopedLock sl (engineLock);
        engine->rack.setState (fxState.toStdString());
    }
    syncParamsFromEngine();
}

String LumiProcessor::fxStateString() const
{
    const ScopedLock sl (engineLock);
    return engine->rack.state();
}

void LumiProcessor::setFxParam (int fx, int param, float value)
{
    if (fx < 0 || fx >= lumi::FX_COUNT || param < 0 || param >= lumi::kMaxParams) return;
    const int ri = refIndex[fx][param + 1];
    if (ri >= 0) setParamFromUi (fxRefs[(size_t) ri].p, value);
}

void LumiProcessor::setFxOn (int fx, bool on)
{
    if (fx < 0 || fx >= lumi::FX_COUNT) return;
    setParamFromUi (fxRefs[(size_t) refIndex[fx][kOnSlot]].p, on ? 1.0f : 0.0f);
}

std::vector<float> LumiProcessor::fxValues() const
{
    std::vector<float> v ((size_t) lumi::FX_COUNT * (lumi::kMaxParams + 1), 0.0f);
    for (const auto& r : fxRefs)
        v[(size_t) (r.fx * (lumi::kMaxParams + 1) + (r.isOn ? 0 : 1 + r.param))] = r.raw->load();
    return v;
}

void LumiProcessor::setCurrentProgram (int index)
{
    index = jlimit (0, lumi::kNumPresets - 1, index);
    soundParam->setValueNotifyingHost (soundParam->convertTo0to1 ((float) index));
}

const String LumiProcessor::getProgramName (int index)
{
    return isPositiveAndBelow (index, lumi::kNumPresets) ? String (lumi::kPresets[index].name) : String();
}

void LumiProcessor::parameterChanged (const String& id, float)
{
    if (id == kSoundId) triggerAsyncUpdate();   // the host changed the sound (automation, program list)
}

void LumiProcessor::handleAsyncUpdate()
{
    const int index = currentPresetIndex();
    if (index == loadedPreset) return;
    {
        const ScopedLock sl (engineLock);
        loadEngine (index);
        loadedPreset = index;
    }
    syncParamsFromEngine();
    ++presetChangeSerial;
}

void LumiProcessor::timerCallback()
{
    if (needsSyncFromEngine.exchange (false)) syncParamsFromEngine();
}

// ------------------------------------------------------------------------------------------------ notes
void LumiProcessor::uiNote (int note, float velocity, bool on)
{
    const auto scope = uiFifo.write (1);
    if (scope.blockSize1 > 0) uiNotes[scope.startIndex1] = { note, velocity, on };
}

int LumiProcessor::popHostNotes (NoteEvent* out, int max)
{
    const auto scope = hostFifo.read (jmin (max, hostFifo.getNumReady()));
    for (int i = 0; i < scope.blockSize1; ++i) out[i] = hostNotes[scope.startIndex1 + i];
    for (int i = 0; i < scope.blockSize2; ++i) out[scope.blockSize1 + i] = hostNotes[scope.startIndex2 + i];
    return scope.blockSize1 + scope.blockSize2;
}

void LumiProcessor::handleMidi (const MidiMessage& m, bool fromHost)
{
    auto tell = [&] (int note, float vel, bool on)
    {
        if (! fromHost || ! uiListening.load (std::memory_order_relaxed)) return;
        const auto scope = hostFifo.write (1);
        if (scope.blockSize1 > 0) hostNotes[scope.startIndex1] = { note, vel, on };
    };

    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        keyDown[n] = true; sustained[n] = false;
        engine->noteOn (n, m.getFloatVelocity());
        tell (n, m.getFloatVelocity(), true);
    }
    else if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();
        keyDown[n] = false;
        if (sustainDown) sustained[n] = true;
        else engine->noteOff (n);
        tell (n, 0.0f, false);
    }
    else if (m.isSustainPedalOn())
    {
        sustainDown = true;
    }
    else if (m.isSustainPedalOff())
    {
        sustainDown = false;
        for (int n = 0; n < 128; ++n)
            if (sustained[n]) { sustained[n] = false; if (! keyDown[n]) engine->noteOff (n); }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        engine->allNotesOff();
        for (int n = 0; n < 128; ++n) keyDown[n] = sustained[n] = false;
        sustainDown = false;
    }
    else if (m.isPitchWheel())
    {
        engine->setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    }
}

// ------------------------------------------------------------------------------------------------ audio
void LumiProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    const ScopedLock sl (engineLock);
    sampleRate = sr;
    engine->prepare ((float) sr);            // resets the rack: reload the sound, then put every control back
    loadEngine (currentPresetIndex());
    loadedPreset = currentPresetIndex();
    pushParamsToEngine (true);
    scratch.setSize (2, jmax (512, samplesPerBlock));
    outGain.reset (sr, 0.03);
    outGain.setCurrentAndTargetValue (Decibels::decibelsToGain (volumeRaw->load()));
}

void LumiProcessor::render (AudioBuffer<float>& buffer, int start, int end)
{
    float* L = scratch.getWritePointer (0);
    float* R = scratch.getWritePointer (1);
    const int chans = buffer.getNumChannels();
    while (start < end)
    {
        const int n = jmin (end - start, scratch.getNumSamples());
        engine->process (L, R, n);
        if (chans >= 2) { buffer.copyFrom (0, start, L, n); buffer.copyFrom (1, start, R, n); }
        else if (chans == 1) { buffer.copyFrom (0, start, L, n, 0.5f); buffer.addFrom (0, start, R, n, 0.5f); }
        start += n;
    }
}

void LumiProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    const ScopedTryLock tl (engineLock);
    if (! tl.isLocked()) return;   // a sound is being loaded on the message thread: one silent block

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 0 && std::abs (*bpm - lastBpm) > 1e-4) { lastBpm = *bpm; engine->setTempo ((float) *bpm); }

    pushParamsToEngine (false);

    if (allOffRequest.exchange (false))
    {
        engine->allNotesOff();
        for (int n = 0; n < 128; ++n) keyDown[n] = sustained[n] = false;
    }

    {   // keys clicked in the plugin window start at the top of the block
        const auto scope = uiFifo.read (uiFifo.getNumReady());
        auto play = [&] (const NoteEvent& e)
        {
            handleMidi (e.on ? MidiMessage::noteOn (1, e.note, jlimit (0.01f, 1.0f, e.velocity)) : MidiMessage::noteOff (1, e.note), false);
        };
        for (int i = 0; i < scope.blockSize1; ++i) play (uiNotes[scope.startIndex1 + i]);
        for (int i = 0; i < scope.blockSize2; ++i) play (uiNotes[scope.startIndex2 + i]);
    }

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = jlimit (0, numSamples, meta.samplePosition);
        render (buffer, pos, t);
        handleMidi (meta.getMessage(), true);
        pos = jmax (pos, t);
    }
    render (buffer, pos, numSamples);

    if (previewOn.load (std::memory_order_relaxed) && previewBuf != nullptr)   // library preview (one voice, independent of the sampler)
    {
        const auto& pb = *previewBuf; const double inc = pb.sr / sampleRate; const auto n = (double) pb.frames();
        for (int i = 0; i < numSamples; ++i)
        {
            if (previewPos >= n - 1) { previewOn = false; break; }
            const int k = (int) previewPos; const float t = (float) (previewPos - k);
            const float l = pb.L[(size_t) k] + (pb.L[(size_t) k + 1] - pb.L[(size_t) k]) * t, r = pb.R[(size_t) k] + (pb.R[(size_t) k + 1] - pb.R[(size_t) k]) * t;
            buffer.addSample (0, i, l * 0.7f);
            if (buffer.getNumChannels() > 1) buffer.addSample (1, i, r * 0.7f);
            previewPos += inc;
        }
    }

    outGain.setTargetValue (Decibels::decibelsToGain (volumeRaw->load()));
    if (outGain.isSmoothing() || std::abs (outGain.getTargetValue() - 1.0f) > 1e-6f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outGain.getNextValue();
            for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.getWritePointer (c)[i] *= g;
        }
    }
}

// ------------------------------------------------------------------------------------------------ state
void LumiProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("soundId", lumi::kPresets[currentPresetIndex()].id, nullptr);
    state.setProperty ("stateVersion", 1, nullptr);
    state.setProperty ("smpPath", smp.path, nullptr);
    state.setProperty ("smpId", smp.id, nullptr);
    state.setProperty ("smpName", smp.name, nullptr);
    state.setProperty ("smpSettings", samplerSettingsText(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void LumiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = ValueTree::fromXml (*xml);
    apvts.replaceState (tree);

    int index = currentPresetIndex();
    const String sid = tree.getProperty ("soundId").toString();   // the sound by name survives a reordered library
    for (int i = 0; i < lumi::kNumPresets; ++i)
        if (sid == lumi::kPresets[i].id) index = i;
    if (index != currentPresetIndex()) soundParam->setValueNotifyingHost (soundParam->convertTo0to1 ((float) index));

    {
        const ScopedLock sl (engineLock);
        loadEngine (index);
        loadedPreset = index;
        pushParamsToEngine (true);   // the saved controls win over the sound's own space
    }
    ++presetChangeSerial;

    // the sampler's file is referenced by path (never copied into the project): reload it, or report why not
    const String path = tree.getProperty ("smpPath").toString();
    smpError = {};
    if (path.isNotEmpty())
    {
        auto d = lumifiles::decodeFile (File (path));
        if (d.audio == nullptr) { smpError = "The sampler's file could not be loaded: " + d.error; clearSampler(); smp.path = path; smp.name = tree.getProperty ("smpName").toString(); }
        else setSamplerSample (d.audio, tree.getProperty ("smpId").toString(), tree.getProperty ("smpName").toString(), path, tree.getProperty ("smpSettings").toString(), false);
    }
    else clearSampler();
}

// ------------------------------------------------------------------------------------------------ sampler
String LumiProcessor::setSamplerSample (std::shared_ptr<const lumi::AudioData> audio, const String& id, const String& name, const String& path,
                                        const String& settingsText, bool autoSettings)
{
    if (audio == nullptr || audio->frames() < 2) return "no audio";
    if ((double) audio->frames() / audio->sr > lumifiles::kMaxSamplerSeconds) return name + ": the sampler plays up to 5 minutes; trim the file first";
    smp.orig = audio; smp.id = id; smp.name = name; smp.path = path;
    smp.an = lumi::analysis::analyse (*audio);
    smpSet = lumi::SamplerSettings::fromString (settingsText.toStdString());
    if (autoSettings && settingsText.isEmpty())
    {
        if (smp.an.rootNote > 0 && smp.an.rootConf > .5) smpSet.root = smp.an.rootNote;
        if (smp.an.silenceStart > 0) smpSet.start = jmin (.5, (double) smp.an.silenceStart / (double) audio->frames());
    }
    smpError = {};
    auto pb = lumi::buildPlayback (*audio, smpSet);   // built outside the audio lock
    const ScopedLock sl (engineLock);
    engine->sampler.setPlayback (pb, smpSet);
    return {};
}

void LumiProcessor::applySamplerSettings (const String& settingsText, bool rebuild)
{
    smpSet = lumi::SamplerSettings::fromString (settingsText.toStdString());
    if (smp.orig == nullptr) return;
    if (rebuild)
    {
        auto pb = lumi::buildPlayback (*smp.orig, smpSet);
        const ScopedLock sl (engineLock);
        engine->sampler.setPlayback (pb, smpSet);
    }
    else
    {
        const ScopedLock sl (engineLock);
        engine->sampler.setSettings (smpSet);
    }
}

void LumiProcessor::clearSampler()
{
    smp = SampleInfo(); smpSet = lumi::SamplerSettings();
    const ScopedLock sl (engineLock);
    engine->sampler.setPlayback (nullptr, smpSet);
}

String LumiProcessor::samplerSettingsText() const { return smp.orig != nullptr ? String (smpSet.toString()) : String(); }

std::shared_ptr<lumi::AudioData> LumiProcessor::samplerProcessed() const
{
    return smp.orig != nullptr ? lumi::buildPlayback (*smp.orig, smpSet) : nullptr;
}

void LumiProcessor::setLayerFromUi (int mode, float piano, float samp)
{
    setParamFromUi (apvts.getParameter (kLayerId), (float) jlimit (0, 2, mode));
    setParamFromUi (apvts.getParameter (kPianoLvlId), piano);
    setParamFromUi (apvts.getParameter (kSamplerLvlId), samp);
}

void LumiProcessor::previewStart (std::shared_ptr<const lumi::AudioData> audio)
{
    const ScopedLock sl (engineLock);
    previewBuf = std::move (audio); previewPos = 0; previewOn = previewBuf != nullptr;
}

void LumiProcessor::previewStop() { previewOn = false; }

AudioProcessorEditor* LumiProcessor::createEditor() { return new LumiEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LumiProcessor(); }

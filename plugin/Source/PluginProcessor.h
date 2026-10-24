// LUMELODY plugin processor: hosts the LUMI engine as an instrument.
//  - plays the host's notes (piano roll, MIDI keyboard) sample-accurately, with velocity, pitch bend, sustain pedal
//  - every effect switch and control is a host-automatable parameter (FL Studio: right-click > Create automation clip)
//  - the sound is a parameter too (and the plugin's program list), saved with the project
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <vector>
#include "LumiEngine.h"
#include "PresetData.h"
#include "SampleFiles.h"

class LumiProcessor : public juce::AudioProcessor,
                      private juce::AudioProcessorValueTreeState::Listener,
                      private juce::AsyncUpdater,
                      private juce::Timer
{
public:
    LumiProcessor();
    ~LumiProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LUMELODY"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return lumi::kNumPresets; }
    int getCurrentProgram() override { return currentPresetIndex(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- used by the editor (message thread)
    juce::AudioProcessorValueTreeState apvts;
    int currentPresetIndex() const;
    void selectPreset (int index, const juce::String& fxState = {});   // load a sound (optionally with saved effects)
    void applyFxState (const juce::String& fxState);                    // "reverb.amount=0.3;..." -> parameters
    juce::String fxStateString() const;                                  // the current effects as that string
    void setFxParam (int fx, int param, float value);                    // from the UI, as a host gesture
    void setFxOn (int fx, bool on);
    std::vector<float> fxValues() const;                                  // [on, p0..p7] per effect (20 x 9)
    void uiNote (int note, float velocity, bool on);                      // keys clicked in the plugin window
    void uiAllNotesOff() { allOffRequest = true; }
    // audio export: renders notes {startBeat, lengthBeats, note, velocity} with the current sound and effects
    struct RenderNote { double t, d; int note; float vel; };
    juce::AudioBuffer<float> renderOffline (const std::vector<RenderNote>& notes, double bpm, double beats, double sampleRate = 44100.0) const;

    // ---- sampler (message thread). The original decoded audio is kept untouched; edits rebuild the playback buffer.
    struct SampleInfo { juce::String id, name, path; std::shared_ptr<const lumi::AudioData> orig; lumi::analysis::Analysis an; };
    juce::String setSamplerSample (std::shared_ptr<const lumi::AudioData> audio, const juce::String& id, const juce::String& name,
                                   const juce::String& path, const juce::String& settingsText, bool autoSettings);   // "" = ok, else error
    void applySamplerSettings (const juce::String& settingsText, bool rebuild);
    void clearSampler();
    const SampleInfo& samplerInfo() const { return smp; }
    juce::String samplerSettingsText() const;
    std::shared_ptr<lumi::AudioData> samplerProcessed() const;             // the edited audio (for export / waveform)
    juce::String samplerError() const { return smpError; }
    void setLayerFromUi (int mode, float piano, float samp);
    void previewStart (std::shared_ptr<const lumi::AudioData> audio);
    void previewStop();
    bool previewPlaying() const { return previewOn.load(); }

    struct NoteEvent { int note; float velocity; bool on; };
    int popHostNotes (NoteEvent* out, int max);   // notes the host played, for the keyboard lights
    int presetSerial() const { return presetChangeSerial.load(); }
    std::atomic<bool> uiListening { false };   // the window is open: queue the host's notes for its lights

    static juce::String paramId (int fx, int param);   // e.g. "reverb_amount"
    static juce::String onId (int fx);                 // e.g. "reverb_on"
    static constexpr const char* kSoundId = "sound";
    static constexpr const char* kVolumeId = "volume";
    static constexpr const char* kLayerId = "layer_mode";
    static constexpr const char* kPianoLvlId = "piano_level";
    static constexpr const char* kSamplerLvlId = "sampler_level";

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String& id, float value) override;
    void handleAsyncUpdate() override;
    void timerCallback() override;

    static lumi::ParamMap presetParams (int index);
    void loadEngine (int index);                  // caller holds engineLock
    void pushParamsToEngine (bool force);         // audio thread or with engineLock held
    void syncParamsFromEngine();                  // engine -> parameters (preset space, reverb type defaults)
    void handleMidi (const juce::MidiMessage&, bool fromHost);
    void render (juce::AudioBuffer<float>&, int start, int end);
    void setParamFromUi (juce::RangedAudioParameter*, float plainValue);

    struct FxRef { int fx, param; bool isOn; std::atomic<float>* raw; juce::RangedAudioParameter* p; float cache; };
    std::vector<FxRef> fxRefs;
    std::atomic<float>* volumeRaw = nullptr;
    std::atomic<float>* layerRaw = nullptr; std::atomic<float>* pianoLvlRaw = nullptr; std::atomic<float>* samplerLvlRaw = nullptr;
    float layerCache[3] { -1, -1, -1 };
    SampleInfo smp; lumi::SamplerSettings smpSet; juce::String smpError;
    std::shared_ptr<const lumi::AudioData> previewBuf; double previewPos = 0; std::atomic<bool> previewOn { false };
    juce::RangedAudioParameter* soundParam = nullptr;

    std::unique_ptr<lumi::Engine> engine;
    juce::CriticalSection engineLock;
    int loadedPreset = -1;
    double lastBpm = 0, sampleRate = 44100;
    std::atomic<bool> needsSyncFromEngine { false }, allOffRequest { false };
    std::atomic<int> presetChangeSerial { 0 };
    bool sustainDown = false, keyDown[128] {}, sustained[128] {};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outGain;
    juce::AudioBuffer<float> scratch;

    // single-producer/single-consumer note queues
    juce::AbstractFifo uiFifo { 512 }, hostFifo { 1024 };
    NoteEvent uiNotes[512] {}, hostNotes[1024] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumiProcessor)
};

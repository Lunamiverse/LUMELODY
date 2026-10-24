// LUMELODY plugin window: the approved LUMELODY interface (plugin/ui/index.html, generated from
// web/instrument_template.html) running in the system web view (WebView2 on Windows, WKWebView on macOS).
// The page talks to the processor with the same messages it sends the browser's AudioWorklet, and receives the
// host's notes so the keys and the moon light up when the piano roll plays.
#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

#if LUMI_WEB_UI

class LumiEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit LumiEditor (LumiProcessor&);
    ~LumiEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::WebBrowserComponent::Options makeOptions();
    static std::optional<juce::WebBrowserComponent::Resource> resource (const juce::String& url);
    void onMessage (const juce::var&);
    void send (juce::DynamicObject::Ptr);
    juce::var valuesVar (const std::vector<float>&) const;
    void timerCallback() override;
    void reply (const juce::String& type, bool ok, const juce::String& detail = {});
    static juce::File exportFolder();
    static juce::File dragFolder();
    void dragOut (const juce::File&);
    // sampler / library / youtube (replies carry the page's request id)
    bool onSamplerMessage (const juce::String& type, const juce::var& m);
    void replyTo (const juce::var& m, juce::DynamicObject::Ptr o);
    void replyError (const juce::var& m, const juce::String& error);
    void sendSamplerState();
    juce::var libraryEntry (const juce::File& f, const lumi::AudioData& a, const lumi::analysis::Analysis& an);
    juce::Array<juce::var> readLibrary();
    void writeLibrary (const juce::Array<juce::var>& entries);
    void addToLibrary (const juce::var& m, const juce::File& f, std::shared_ptr<lumi::AudioData> audio);
    void runAsync (std::function<juce::var()> work, std::function<void (const juce::var&)> done);
    std::unique_ptr<juce::FileChooser> chooser;
    bool previewWasOn = false, libWasOn = false;

    LumiProcessor& proc;
    juce::WebBrowserComponent web;
    std::vector<float> lastValues;
    int lastSerial = -1;
    bool pageReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumiEditor)
};

#else

// plain fallback window (builds without a web view, and the headless tests)
class LumiEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit LumiEditor (LumiProcessor& p) : juce::GenericAudioProcessorEditor (p) { setSize (520, 640); }
};

#endif

#include "PluginEditor.h"

#if LUMI_WEB_UI
#include "BinaryData.h"
#include <juce_audio_formats/juce_audio_formats.h>

using namespace juce;

namespace
{
    constexpr int kSlots = lumi::kMaxParams + 1;

    bool differs (const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.size() != b.size()) return true;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::abs (a[i] - b[i]) > 1e-4f * (1.0f + std::abs (b[i]))) return true;
        return false;
    }
}

WebBrowserComponent::Options LumiEditor::makeOptions()
{
    return WebBrowserComponent::Options {}
        .withBackend (WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (WebBrowserComponent::Options::WinWebView2 {}
                                     .withUserDataFolder (File::getSpecialLocation (File::tempDirectory).getChildFile ("LUMELODY-WebView2"))
                                     .withBackgroundColour (Colours::black)
                                     .withStatusBarDisabled())
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withEventListener ("lumi", [this] (const var& v) { onMessage (v); })
        .withResourceProvider ([] (const String& url) { return resource (url); });
}

LumiEditor::LumiEditor (LumiProcessor& p)
    : AudioProcessorEditor (p), proc (p), web (makeOptions())
{
    LumiProcessor::NoteEvent stale[256];
    while (proc.popHostNotes (stale, 256) > 0) {}
    proc.uiListening = true;

    addAndMakeVisible (web);
    setResizable (true, true);
    setResizeLimits (900, 620, 2400, 1600);
    setSize (1200, 800);
    web.goToURL (WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (30);
}

LumiEditor::~LumiEditor()
{
    stopTimer();
    proc.uiListening = false;
}

void LumiEditor::paint (Graphics& g) { g.fillAll (Colours::black); }
void LumiEditor::resized() { web.setBounds (getLocalBounds()); }

std::optional<WebBrowserComponent::Resource> LumiEditor::resource (const String& url)
{
    String path = url.upToFirstOccurrenceOf ("?", false, false);
    if (path == "/" || path.isEmpty()) path = "/index.html";
    const String file = path.fromLastOccurrenceOf ("/", false, false);

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (file != BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i])) continue;
        int size = 0;
        const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        if (data == nullptr) return std::nullopt;
        WebBrowserComponent::Resource r;
        r.data.resize ((size_t) size);
        std::memcpy (r.data.data(), data, (size_t) size);
        r.mimeType = file.endsWith (".html") ? "text/html; charset=utf-8"
                   : file.endsWith (".ttf")  ? "font/ttf"
                   : file.endsWith (".js")   ? "text/javascript"
                                             : "application/octet-stream";
        return r;
    }
    return std::nullopt;
}

var LumiEditor::valuesVar (const std::vector<float>& v) const
{
    Array<var> a;
    a.ensureStorageAllocated ((int) v.size());
    for (float x : v) a.add (x);
    return a;
}

void LumiEditor::send (DynamicObject::Ptr o)
{
    web.emitEventIfBrowserIsVisible ("lumi", var (o.get()));
}

void LumiEditor::onMessage (const var& m)
{
    const String type = m["type"].toString();

    if (type == "hello")
    {
        pageReady = true;
        lastValues = proc.fxValues();
        lastSerial = proc.presetSerial();
        DynamicObject::Ptr o = new DynamicObject();
        o->setProperty ("type", "ready");
        o->setProperty ("id", lumi::kPresets[proc.currentPresetIndex()].id);
        o->setProperty ("values", valuesVar (lastValues));
        send (o);
    }
    else if (type == "load")
    {
        const String id = m["id"].toString();
        for (int i = 0; i < lumi::kNumPresets; ++i)
            if (id == lumi::kPresets[i].id)
                proc.selectPreset (i, m["state"].isString() ? m["state"].toString() : String());
        // the timer sees the new sound and sends its controls back
    }
    else if (type == "state")
    {
        proc.applyFxState (m["bytes"].toString());
    }
    else if (type == "getState")
    {
        DynamicObject::Ptr o = new DynamicObject();
        o->setProperty ("type", "stateOut");
        o->setProperty ("id", m["id"]);
        o->setProperty ("bytes", proc.fxStateString());
        send (o);
    }
    else if (type == "param")
    {
        const int e = (int) m["e"], i = (int) m["i"];
        const float v = (float) (double) m["v"];
        proc.setFxParam (e, i, v);
        if (e >= 0 && e < lumi::FX_COUNT && i >= 0 && i < lumi::kMaxParams && ! lastValues.empty())
            lastValues[(size_t) (e * kSlots + 1 + i)] = v;   // the page already shows it: no echo
    }
    else if (type == "fxon")
    {
        const int e = (int) m["e"];
        const bool on = (bool) m["on"];
        proc.setFxOn (e, on);
        if (e >= 0 && e < lumi::FX_COUNT && ! lastValues.empty()) lastValues[(size_t) (e * kSlots)] = on ? 1.0f : 0.0f;
    }
    else if (type == "on")     proc.uiNote ((int) m["note"], (float) (double) m["vel"], true);
    else if (type == "off")    proc.uiNote ((int) m["note"], 0.0f, false);
    else if (type == "alloff") proc.uiAllNotesOff();
    // ---- universal export: save or drag MIDI / audio into the DAW (FL Studio accepts dropped .mid and .wav files)
    else if (type == "saveFile" || type == "dragFile")
    {
        MemoryOutputStream mo;
        const bool decoded = Base64::convertFromBase64 (mo, m["b64"].toString());
        const String name = File::createLegalFileName (m["name"].toString());
        const bool drag = type == "dragFile";
        File f = drag ? dragFolder().getChildFile (name) : exportFolder().getNonexistentChildFile (name.upToLastOccurrenceOf (".", false, false), "." + name.fromLastOccurrenceOf (".", false, false), false);
        const bool ok = decoded && name.isNotEmpty() && f.getParentDirectory().createDirectory().wasOk()
                     && f.replaceWithData (mo.getData(), mo.getDataSize());
        if (drag && ok) dragOut (f);
        else reply ("saved", ok, f.getFullPathName());
    }
    else if (type == "renderAudio")
    {
        std::vector<LumiProcessor::RenderNote> notes;
        if (auto* arr = m["ev"].getArray())
            for (const auto& e : *arr)
                if (e.isArray() && e.size() >= 4)
                    notes.push_back ({ (double) e[0], (double) e[1], (int) e[2], (float) (double) e[3] });
        auto audio = proc.renderOffline (notes, (double) m["bpm"], (double) m["beats"]);
        const String name = File::createLegalFileName (m["name"].toString());
        const bool drag = (bool) m["drag"];
        File f = drag ? dragFolder().getChildFile (name) : exportFolder().getNonexistentChildFile (name.upToLastOccurrenceOf (".", false, false), ".wav", false);
        bool ok = f.getParentDirectory().createDirectory().wasOk();
        if (ok)
        {
            f.deleteFile();
            WavAudioFormat wav;
            std::unique_ptr<OutputStream> os (f.createOutputStream().release());
            std::unique_ptr<AudioFormatWriter> w (os != nullptr ? wav.createWriterFor (os.get(), 44100.0, 2, 24, {}, 0) : nullptr);
            if (w != nullptr) { os.release(); ok = w->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples()); }
            else ok = false;
        }
        if (drag && ok) dragOut (f);
        else reply ("saved", ok, f.getFullPathName());
    }
    // "tempo" and "bend": the host's tempo and pitch wheel are used inside the plugin
}

File LumiEditor::exportFolder() { return File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("LUMELODY"); }
File LumiEditor::dragFolder()   { return File::getSpecialLocation (File::tempDirectory).getChildFile ("LUMELODY-drag"); }

void LumiEditor::dragOut (const File& f)
{
    // runs while the mouse button is still held on the page's DRAG TO DAW handle
    if (! performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this))
        reply ("saved", false, f.getFullPathName());
}

void LumiEditor::reply (const String& type, bool ok, const String& detail)
{
    DynamicObject::Ptr o = new DynamicObject();
    o->setProperty ("type", type);
    o->setProperty ("ok", ok);
    o->setProperty ("path", detail);
    send (o);
}

void LumiEditor::timerCallback()
{
    if (! pageReady) return;

    LumiProcessor::NoteEvent ev[256];
    for (int n; (n = proc.popHostNotes (ev, 256)) > 0;)
        for (int i = 0; i < n; ++i)
        {
            DynamicObject::Ptr o = new DynamicObject();
            o->setProperty ("type", "note");
            o->setProperty ("note", ev[i].note);
            o->setProperty ("vel", ev[i].velocity);
            o->setProperty ("on", ev[i].on);
            send (o);
        }

    const auto values = proc.fxValues();
    if (proc.presetSerial() != lastSerial)
    {
        lastSerial = proc.presetSerial();
        lastValues = values;
        DynamicObject::Ptr o = new DynamicObject();
        o->setProperty ("type", "preset");
        o->setProperty ("id", lumi::kPresets[proc.currentPresetIndex()].id);
        o->setProperty ("values", valuesVar (values));
        send (o);
    }
    else if (differs (values, lastValues))   // automation from the host, or a reverb type's own settings
    {
        lastValues = values;
        DynamicObject::Ptr o = new DynamicObject();
        o->setProperty ("type", "values");
        o->setProperty ("values", valuesVar (values));
        send (o);
    }
}

#endif

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
    if (onSamplerMessage (type, m)) return;

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
        sendSamplerState();
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
    if (! warmQueue.isEmpty()) { proc.librarySample (warmQueue[0]); warmQueue.remove (0); }   // decode the samples on screen, one per tick
    if (libWasOn && ! proc.libraryPlaying()) { DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("type", "libEnd"); send (o); }
    libWasOn = proc.libraryPlaying();
    if (previewWasOn && ! proc.previewPlaying()) { DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("type", "smpPreviewEnd"); send (o); }
    previewWasOn = proc.previewPlaying();

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


#if LUMI_WEB_UI
// ================================================================================================ sampler bridge
void LumiEditor::replyTo (const var& m, DynamicObject::Ptr o)
{
    o->setProperty ("type", "reply");
    o->setProperty ("rid", m["rid"]);
    send (o);
}

void LumiEditor::replyError (const var& m, const String& error)
{
    DynamicObject::Ptr o = new DynamicObject();
    o->setProperty ("error", error);
    replyTo (m, o);
}

void LumiEditor::runAsync (std::function<var()> work, std::function<void (const var&)> done)
{
    Component::SafePointer<LumiEditor> safe (this);
    Thread::launch ([work = std::move (work), done = std::move (done), safe]
    {
        auto result = work();
        MessageManager::callAsync ([result, done, safe] { if (safe != nullptr) done (result); });
    });
}

Array<var> LumiEditor::readLibrary()
{
    Array<var> out;
    auto j = JSON::parse (lumifiles::libraryFile());
    if (auto* arr = j.getArray())
        for (auto& e : *arr)
        {
            if (auto* o = e.getDynamicObject()) o->setProperty ("missing", ! File (e["path"].toString()).existsAsFile());
            out.add (e);
        }
    return out;
}

void LumiEditor::writeLibrary (const Array<var>& entries)
{
    auto f = lumifiles::libraryFile();
    f.getParentDirectory().createDirectory();
    f.replaceWithText (JSON::toString (var (entries), false));
}

var LumiEditor::libraryEntry (const File& f, const lumi::AudioData& a, const lumi::analysis::Analysis& an)
{
    auto* o = new DynamicObject();
    o->setProperty ("id", lumifiles::idForPath (f.getFullPathName()));
    o->setProperty ("name", f.getFileNameWithoutExtension());
    o->setProperty ("file", f.getFileName());
    o->setProperty ("path", f.getFullPathName());
    o->setProperty ("size", (double) f.getSize());
    o->setProperty ("added", (double) Time::currentTimeMillis());
    o->setProperty ("fav", false);
    o->setProperty ("seconds", (double) a.frames() / a.sr);
    o->setProperty ("sr", a.sr);
    o->setProperty ("channels", a.channels);
    o->setProperty ("peaks", lumifiles::floatsVar (lumi::analysis::peaks (a, 160)));
    o->setProperty ("an", lumifiles::analysisVar (an));
    o->setProperty ("folder", f.getParentDirectory().getFileName());
    return var (o);
}

void LumiEditor::addToLibrary (const var& m, const File& f, std::shared_ptr<lumi::AudioData> audio)
{
    auto lib = readLibrary();
    const String id = lumifiles::idForPath (f.getFullPathName());
    for (auto& e : lib)
        if (e["id"].toString() == id)
        {
            DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("name", e["name"]); o->setProperty ("dupe", true); replyTo (m, o); return;
        }
    runAsync ([f, audio] () -> var
    {
        auto an = lumi::analysis::analyse (*audio);
        auto* o = new DynamicObject();
        o->setProperty ("an", lumifiles::analysisVar (an));
        o->setProperty ("peaks", lumifiles::floatsVar (lumi::analysis::peaks (*audio, 160)));
        return var (o);
    },
    [this, m, f, audio] (const var& r)
    {
        auto lib2 = readLibrary();
        auto entry = libraryEntry (f, *audio, lumi::analysis::Analysis());
        if (auto* o = entry.getDynamicObject()) { o->setProperty ("an", r["an"]); o->setProperty ("peaks", r["peaks"]); }
        lib2.add (entry);
        writeLibrary (lib2);
        DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("name", entry["name"]); o->setProperty ("dupe", false); replyTo (m, o);
    });
}

void LumiEditor::sendSamplerState()
{
    const auto& in = proc.samplerInfo();
    DynamicObject::Ptr o = new DynamicObject();
    o->setProperty ("type", "smpState");
    o->setProperty ("loaded", in.orig != nullptr);
    o->setProperty ("error", proc.samplerError());
    auto* lay = new DynamicObject();
    lay->setProperty ("mode", roundToInt (proc.apvts.getRawParameterValue (LumiProcessor::kLayerId)->load()));
    lay->setProperty ("piano", proc.apvts.getRawParameterValue (LumiProcessor::kPianoLvlId)->load());
    lay->setProperty ("sampler", proc.apvts.getRawParameterValue (LumiProcessor::kSamplerLvlId)->load());
    o->setProperty ("layer", var (lay));
    if (in.orig != nullptr)
    {
        o->setProperty ("id", in.id); o->setProperty ("name", in.name); o->setProperty ("path", in.path);
        o->setProperty ("sr", in.orig->sr); o->setProperty ("channels", in.orig->channels); o->setProperty ("frames", (double) in.orig->frames());
        o->setProperty ("an", lumifiles::analysisVar (in.an));
        o->setProperty ("settings", proc.samplerSettingsText());
        if (auto pb = proc.samplerProcessed()) o->setProperty ("peaks", lumifiles::floatsVar (lumi::analysis::peaks (*pb, 1400)));
    }
    send (o);
}

bool LumiEditor::onSamplerMessage (const String& type, const var& m)
{
    // ---- sound libraries: the factory samples are inside the plugin; previews play through the plugin's output
    if (type == "libPlay" || type == "libPattern")
    {
        String err; auto song = proc.librarySong (m, err);
        if (song == nullptr) { DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("type", "libError"); o->setProperty ("error", err); send (o); return true; }
        if (m.hasProperty ("vol")) proc.libraryGain ((float) (double) m["vol"]);
        proc.libraryStart (song); libWasOn = true;
        return true;
    }
    if (type == "libStop") { proc.libraryStop(); return true; }
    if (type == "libWarm") { if (auto* a = m["ids"].getArray()) for (auto& v : *a) if (warmQueue.size() < 512) warmQueue.add (v.toString()); return true; }
    if (type == "libVol") { proc.libraryGain ((float) (double) m["vol"]); return true; }
    if (type == "libExport")
    {
        const String name = File::createLegalFileName (m["name"].toString().isNotEmpty() ? m["name"].toString() : String ("LUMELODY sound.wav"));
        const bool drag = (bool) m["drag"];
        const File f = drag ? dragFolder().getChildFile (name) : exportFolder().getNonexistentChildFile (name.upToLastOccurrenceOf (".", false, false), ".wav", false);
        const String err = proc.libraryRenderWav (m, f);
        if (err.isNotEmpty()) { DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("type", "libError"); o->setProperty ("error", err); send (o); return true; }
        if (drag) dragOut (f); else reply ("saved", true, f.getFullPathName());
        return true;
    }
    if (type == "smpImport")
    {
        chooser = std::make_unique<FileChooser> ("Import audio into LUMELODY", File(), "*.wav;*.wave;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a;*.aac;*.caf");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, m] (const FileChooser& fc)
        {
            const File f = fc.getResult();
            if (f == File()) { DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("cancelled", true); replyTo (m, o); return; }
            runAsync ([f] { auto d = lumifiles::decodeFile (f); auto* o = new DynamicObject(); o->setProperty ("error", d.error);
                            if (d.audio) o->setProperty ("ok", true); return var (o); },
                      [this, m, f] (const var& r)
                      {
                          if (r["error"].toString().isNotEmpty()) { replyError (m, r["error"].toString()); return; }
                          auto d = lumifiles::decodeFile (f);   // cheap to repeat for a single file, keeps ownership simple
                          if (d.audio == nullptr) { replyError (m, d.error); return; }
                          addToLibrary (m, f, d.audio);
                      });
        });
        return true;
    }
    if (type == "smpImportData")   // a file dropped on the page: the page only has its bytes, so a copy is kept in Documents/LUMELODY
    {
        MemoryOutputStream mo;
        const String name = File::createLegalFileName (m["name"].toString());
        if (! Base64::convertFromBase64 (mo, m["b64"].toString())) { replyError (m, name + ": the dropped data was damaged"); return true; }
        MemoryBlock mb (mo.getData(), mo.getDataSize());
        auto d = lumifiles::decodeMemory (name, mb);
        if (d.audio == nullptr) { replyError (m, d.error); return true; }
        auto dir = lumifiles::importedDir(); dir.createDirectory();
        File f = dir.getChildFile (name);
        if (! (f.existsAsFile() && f.getSize() == (int64) mb.getSize())) { f = dir.getNonexistentChildFile (f.getFileNameWithoutExtension(), f.getFileExtension(), false); f.replaceWithData (mb.getData(), mb.getSize()); }
        addToLibrary (m, f, d.audio);
        return true;
    }
    if (type == "smpImportPath")
    {
        const File f (m["path"].toString());
        auto d = lumifiles::decodeFile (f);
        if (d.audio == nullptr) { replyError (m, d.error); return true; }
        addToLibrary (m, f, d.audio);
        return true;
    }
    if (type == "smpFolder")
    {
        chooser = std::make_unique<FileChooser> ("Open a sample folder", File());
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories, [this, m] (const FileChooser& fc)
        {
            const File dir = fc.getResult();
            if (dir == File()) { replyError (m, "no folder chosen"); return; }
            Array<var> files;
            for (const auto& entry : RangedDirectoryIterator (dir, true, "*", File::findFiles))
            {
                const auto f = entry.getFile();
                if (! lumifiles::isAudioName (f.getFileName())) continue;
                auto* o = new DynamicObject(); o->setProperty ("path", f.getFullPathName()); o->setProperty ("name", f.getFileName()); o->setProperty ("size", (double) f.getSize());
                files.add (var (o));
                if (files.size() >= 2000) break;
            }
            DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("folder", dir.getFileName()); o->setProperty ("files", files); replyTo (m, o);
        });
        return true;
    }
    if (type == "libLoad") { DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("entries", readLibrary()); replyTo (m, o); return true; }
    if (type == "libSave")
    {
        Array<var> entries; if (auto* a = m["entries"].getArray()) entries = *a;
        writeLibrary (entries); replyTo (m, new DynamicObject()); return true;
    }
    if (type == "smpPreview")
    {
        if (! (bool) m["on"]) { proc.previewStop(); replyTo (m, new DynamicObject()); return true; }
        const File f (m["path"].toString());
        runAsync ([f] { auto d = lumifiles::decodeFile (f); auto* o = new DynamicObject(); o->setProperty ("error", d.error); return var (o); },
                  [this, m, f] (const var& r)
                  {
                      if (r["error"].toString().isNotEmpty()) { replyError (m, r["error"].toString()); return; }
                      auto d = lumifiles::decodeFile (f);
                      if (d.audio == nullptr) { replyError (m, d.error); return; }
                      proc.previewStart (d.audio); replyTo (m, new DynamicObject());
                  });
        return true;
    }
    if (type == "smpUse")
    {
        const File f (m["path"].toString());
        auto d = lumifiles::decodeFile (f);
        if (d.audio == nullptr) { replyError (m, d.error); return true; }
        const String err = proc.setSamplerSample (d.audio, m["id"].toString(), f.getFileNameWithoutExtension(), f.getFullPathName(), m["settings"].toString(), true);
        if (err.isNotEmpty()) { replyError (m, err); return true; }
        const auto& in = proc.samplerInfo();
        DynamicObject::Ptr o = new DynamicObject();
        o->setProperty ("frames", (double) in.orig->frames()); o->setProperty ("sr", in.orig->sr); o->setProperty ("channels", in.orig->channels);
        o->setProperty ("an", lumifiles::analysisVar (in.an));
        replyTo (m, o);
        if (proc.apvts.getRawParameterValue (LumiProcessor::kLayerId)->load() < .5f) proc.setLayerFromUi (1, proc.apvts.getRawParameterValue (LumiProcessor::kPianoLvlId)->load(), proc.apvts.getRawParameterValue (LumiProcessor::kSamplerLvlId)->load());
        return true;
    }
    if (type == "smpSettings")
    {
        const bool rebuild = (bool) m["rebuild"];
        proc.applySamplerSettings (m["bytes"].toString().upToFirstOccurrenceOf (String::charToString (0), false, false), rebuild);
        DynamicObject::Ptr o = new DynamicObject();
        if (rebuild || ! (bool) m["havePeaks"]) if (auto pb = proc.samplerProcessed()) o->setProperty ("peaks", lumifiles::floatsVar (lumi::analysis::peaks (*pb, jlimit (100, 4000, (int) m["bins"]))));
        replyTo (m, o);
        return true;
    }
    if (type == "smpClear") { proc.clearSampler(); return true; }
    if (type == "layer") { proc.setLayerFromUi ((int) m["mode"], (float) (double) m["piano"], (float) (double) m["sampler"]); return true; }
    if (type == "smpExport")
    {
        auto pb = proc.samplerProcessed();
        if (pb == nullptr) { reply ("saved", false, "no sample loaded"); return true; }
        const auto& st = lumi::SamplerSettings::fromString (proc.samplerSettingsText().toStdString());
        const int a = (int) std::floor (st.start * (double) pb->frames()), b = (int) std::ceil (st.end * (double) pb->frames());
        AudioBuffer<float> out (2, jmax (1, b - a));
        for (int i = a; i < b; ++i) { out.setSample (0, i - a, pb->L[(size_t) i]); out.setSample (1, i - a, pb->R[(size_t) i]); }
        const String name = File::createLegalFileName (m["name"].toString());
        const bool drag = (bool) m["drag"];
        File f = drag ? dragFolder().getChildFile (name) : exportFolder().getNonexistentChildFile (name.upToLastOccurrenceOf (".", false, false), ".wav", false);
        f.getParentDirectory().createDirectory(); f.deleteFile();
        bool ok = false;
        {
            WavAudioFormat wav;
            std::unique_ptr<OutputStream> os (f.createOutputStream().release());
            std::unique_ptr<AudioFormatWriter> w (os != nullptr ? wav.createWriterFor (os.get(), pb->sr, 2, 24, {}, 0) : nullptr);
            if (w != nullptr) { os.release(); ok = w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples()); }
        }
        if (drag && ok) dragOut (f); else reply ("saved", ok, f.getFullPathName());
        return true;
    }
    if (type == "smpPresetSave")
    {
        const var p = m["preset"];
        auto dir = lumifiles::presetDir(); dir.createDirectory();
        const bool ok = dir.getChildFile (File::createLegalFileName (p["name"].toString()) + ".json").replaceWithText (JSON::toString (p, false));
        if (ok) replyTo (m, new DynamicObject()); else replyError (m, "could not write to " + dir.getFullPathName());
        return true;
    }
    if (type == "smpPresetList")
    {
        Array<var> ps;
        for (const auto& e : RangedDirectoryIterator (lumifiles::presetDir(), false, "*.json", File::findFiles))
        {
            auto j = JSON::parse (e.getFile());
            if (j.isObject()) ps.add (j);
        }
        DynamicObject::Ptr o = new DynamicObject(); o->setProperty ("presets", ps); replyTo (m, o);
        return true;
    }
    if (type == "ytMeta")   // public oEmbed lookup (title / channel only); nothing is downloaded
    {
        const String video = m["url"].toString();
        runAsync ([video]
        {
            auto* o = new DynamicObject();
            URL url ("https://www.youtube.com/oembed");
            url = url.withParameter ("format", "json").withParameter ("url", video);
            int status = 0;
            auto stream = url.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress).withConnectionTimeoutMs (7000).withStatusCode (&status));
            if (stream == nullptr) { o->setProperty ("error", "the title could not be looked up (no connection)"); return var (o); }
            const String body = stream->readEntireStreamAsString();
            if (status == 401 || status == 403) o->setProperty ("error", "the owner does not allow this video to be shown outside YouTube, or it is private");
            else if (status == 404 || status == 400) o->setProperty ("error", "the video is unavailable (removed, private or the id is wrong)");
            else if (status != 200) o->setProperty ("error", "YouTube answered with an error (" + String (status) + ")");
            else
            {
                auto j = JSON::parse (body);
                o->setProperty ("title", j["title"]); o->setProperty ("author", j["author_name"]); o->setProperty ("thumb", j["thumbnail_url"]);
            }
            return var (o);
        },
        [this, m] (const var& r) { if (r["error"].toString().isNotEmpty()) replyError (m, r["error"].toString()); else replyTo (m, r.getDynamicObject()); });
        return true;
    }
    if (type == "openUrl")
    {
        const String u = m["url"].toString();
        if (u.startsWith ("https://studio.youtube.com/")) URL (u).launchInDefaultBrowser();   // only the one page the sampler links to
        return true;
    }
    return false;
}
#endif

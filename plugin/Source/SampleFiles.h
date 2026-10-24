// File side of the LUMELODY sampler inside the plugin: decoding any format the system supports, the user's sample
// library index and sampler presets (JSON files in Documents/LUMELODY). The audio itself is never modified on disk.
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>
#include "LumiAnalysis.h"

namespace lumifiles
{
    constexpr double kMaxImportSeconds = 600.0, kMaxSamplerSeconds = 300.0;
    constexpr juce::int64 kMaxFileBytes = 300ll * 1024 * 1024;

    struct Decoded { std::shared_ptr<lumi::AudioData> audio; juce::String error; };

    inline juce::AudioFormatManager& formats()
    {
        static juce::AudioFormatManager fm;
        static bool init = [] { fm.registerBasicFormats(); return true; }();
        juce::ignoreUnused (init);
        return fm;
    }

    inline Decoded decode (std::unique_ptr<juce::InputStream> in, const juce::String& name, juce::int64 size)
    {
        Decoded d;
        if (in == nullptr) { d.error = name + ": the file could not be opened (moved, locked or no permission)"; return d; }
        if (size == 0) { d.error = name + ": the file is empty"; return d; }
        if (size > kMaxFileBytes) { d.error = name + ": the file is " + juce::String (size / 1048576) + " MB; the limit is 300 MB"; return d; }
        std::unique_ptr<juce::AudioFormatReader> r (formats().createReaderFor (std::move (in)));
        if (r == nullptr) { d.error = name + ": this is not an audio format this computer can decode, or the file is damaged"; return d; }
        if (r->lengthInSamples < 2 || r->sampleRate <= 0) { d.error = name + ": the file contains no audio"; return d; }
        const double secs = (double) r->lengthInSamples / r->sampleRate;
        if (secs > kMaxImportSeconds) { d.error = name + ": the audio is " + juce::String (juce::roundToInt (secs / 60)) + " minutes long; LUMELODY imports up to 10 minutes"; return d; }
        const int n = (int) r->lengthInSamples, ch = (int) juce::jmin (2u, r->numChannels);
        juce::AudioBuffer<float> buf (juce::jmax (1, ch), n);
        if (! r->read (&buf, 0, n, 0, true, ch > 1)) { d.error = name + ": the audio data could not be read (damaged file)"; return d; }
        auto a = std::make_shared<lumi::AudioData>();
        a->sr = r->sampleRate; a->channels = ch;
        a->L.assign (buf.getReadPointer (0), buf.getReadPointer (0) + n);
        a->R.assign (buf.getReadPointer (ch > 1 ? 1 : 0), buf.getReadPointer (ch > 1 ? 1 : 0) + n);
        for (size_t i = 0; i < a->L.size(); ++i) if (! std::isfinite (a->L[i]) || ! std::isfinite (a->R[i])) { a->L[i] = a->R[i] = 0; }
        d.audio = a;
        return d;
    }
    inline Decoded decodeFile (const juce::File& f)
    {
        if (! f.existsAsFile()) return { nullptr, f.getFileName() + ": file not found at " + f.getFullPathName() };
        return decode (f.createInputStream(), f.getFileName(), f.getSize());
    }
    inline Decoded decodeMemory (const juce::String& name, const juce::MemoryBlock& mb)
    {
        return decode (std::make_unique<juce::MemoryInputStream> (mb, true), name, (juce::int64) mb.getSize());
    }

    inline juce::File root()          { return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("LUMELODY"); }
    inline juce::File libraryFile()   { return root().getChildFile ("Library").getChildFile ("library.json"); }
    inline juce::File importedDir()   { return root().getChildFile ("Library").getChildFile ("Imported"); }
    inline juce::File presetDir()     { return root().getChildFile ("Sampler Presets"); }
    inline juce::String idForPath (const juce::String& path) { return juce::String::toHexString (path.hashCode64()); }

    inline bool isAudioName (const juce::String& n)
    {
        static const juce::StringArray ext { ".wav", ".wave", ".aif", ".aiff", ".flac", ".mp3", ".ogg", ".m4a", ".aac", ".caf", ".wma" };
        for (auto& e : ext) if (n.endsWithIgnoreCase (e)) return true;
        return false;
    }

    // analysis summary as the page expects it
    inline juce::var analysisVar (const lumi::analysis::Analysis& an)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("bpm", an.bpm); o->setProperty ("bpmConf", an.bpmConf);
        o->setProperty ("key", an.keyTonic); o->setProperty ("minor", an.keyMinor); o->setProperty ("keyConf", an.keyConf);
        o->setProperty ("root", an.rootNote); o->setProperty ("cents", an.rootCents); o->setProperty ("rootConf", an.rootConf);
        o->setProperty ("silS", (double) an.silenceStart); o->setProperty ("silE", (double) an.silenceEnd);
        juce::Array<juce::var> on; for (auto f : an.onsets) on.add ((double) f);
        o->setProperty ("onsets", on);
        return juce::var (o);
    }
    inline juce::var floatsVar (const std::vector<float>& v)
    {
        juce::Array<juce::var> a; a.ensureStorageAllocated ((int) v.size());
        for (float x : v) a.add (x);
        return a;
    }
}

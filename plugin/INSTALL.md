# Installing LUMELODY in FL Studio

## Windows
1. Copy `LUMELODY.vst3` to `C:\Program Files\Common Files\VST3\`.
2. In FL Studio: Options → Manage plugins → Find installed plugins.
3. Add it from the Channel rack (+) → LUMELODY. Open its piano roll and draw notes: LUMELODY plays them.

The window uses Microsoft Edge WebView2, which is already part of Windows 10 and 11.

## macOS (Apple silicon and Intel)
1. Copy `LUMELODY.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and `LUMELODY.component` to `~/Library/Audio/Plug-Ins/Components/`.
2. The test builds are not notarized yet. If macOS blocks them, run once in Terminal:
   `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/LUMELODY.vst3 ~/Library/Audio/Plug-Ins/Components/LUMELODY.component`
3. In FL Studio: Options → Manage plugins → Find installed plugins, then add LUMELODY from the Channel rack.

## In FL Studio
- Piano roll, MIDI keyboard, velocity, pitch bend and sustain pedal all play the sound.
- Every effect control can be automated: move it in LUMELODY, then in FL use Tools → Last tweaked → Create automation clip.
- FL's preset arrows step through the sounds. The project remembers the sound and every effect.
- Chord Progression Finder: **drag to daw ↗** drags the MIDI (chords + bass) straight into the piano roll or playlist;
  **save midi** / **save audio** put the files in `Documents/LUMELODY`.
- Arpeggiator (inside the Chord Progression Finder): **midi ↗** drags the arpeggio into the piano roll. Notes you hold
  on LUMELODY's keys feed it live; notes from FL's piano roll play the sound directly (export the arpeggio as MIDI to put
  it in the piano roll).
- Sounds: the library samples are inside the plugin. Previews and patterns play through LUMELODY's output (so they go
  through FL's mixer); **wav ↗** / **midi ↗** drag a sample, a pattern or a layered groove into the playlist or piano roll.

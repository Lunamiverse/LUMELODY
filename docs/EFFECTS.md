# LUMELODY effects: what is implemented and how it was tested

All 20 effects run in the C++ engine (`engine/LumiFx.h`, wired in `engine/LumiEngine.h`). The same code runs in
the browser (WebAssembly) and will run in the plugin. Each effect has an on/off switch (10 ms crossfade, so
switching is click-free), a main knob and the sub-controls listed in `fxSchema()`, which is also what the UI reads.

Signal flow: voices (+ Octave Layer) → preset tone stage → Filter → Resonator → Saturation → Tape → Glue/Punch →
Chorus → Phaser → Flanger → Stutter → Gate/Pump → Reverse & Swell → Delay → Reverb → Shimmer → Freeze →
Granular → Width → preset pump/gain → output safety (transparent below −1 dBFS, smooth knee above).
Pitch, Glide, Octave Layer and the Swell trigger work inside the voice engine because they need note events.

## Test suite

`tools/render/fx_suite.cpp` renders every effect on six instruments (EP keys, drill bell, mono saw lead, pad,
808, plucked string) plus a pure sine source, with low/mid/high notes, a 5-note chord and a fast 16th passage,
and moves the knobs while audio plays (defaults, main knob sweep, all knobs sweeping, all knobs at maximum).

It checks, for every effect: no NaN/inf, never above full scale, the output safety almost never engaged,
no clicks added while knobs move (judged on the sine source), bypass returns the exact dry signal, and CPU.
Then it checks each effect does what its label says (echo timing per division, reverb decay/pre-delay/tone,
exact pitch, seamless stutter repeats, filter slopes, aliasing, mono compatibility, freeze stability over 30 s,
real reversed audio, octave layers in tune, etc.). Latest result: **184 checks passed, 0 failed**
(`build/fx_suite_report.txt` after running it).

## Honest limits

- **Glide** slides analog, FM, voice and grain layers. Plucked-string and mallet layers strike at a fixed pitch.
- **Resonator** is a vowel (formant-peak) filter and a tuned comb. It is not a formant *shifter*.
- **Saturation** above ~40 % drive audibly shapes dense chord peaks; at 100 % Crunch it is hard clipping by design.
- **Pitch** re-synthesizes the instrument at the new pitch (no time-stretching artefacts). It applies to the
  instrument, not to audio passing through other effects.
- CPU (native, one core): typical single effect adds a few %; every effect on at once ≈ 25 % on dense material.
  The browser build runs roughly 1.5–2× slower than native.
- All tests are numeric. They catch faults (clicks, instability, wrong timing) but do not replace listening.

# LUMELODY visual design (web instrument; the plugin UI will follow the same system)

- **Direction:** a night-sky instrument with one hero object. Hairline panels, lowercase monospace labels
  (JetBrains Mono, OFL), an italic serif for the wordmark and sound names (Cormorant Garamond, OFL). Holographic foil is
  used sparingly as a precious material: the moon, the wordmark, the knob caps, lit keys, one foil hairline on the frame.
- **Layout:** wordmark + sound picker + themes on top; the moon on the left with the sound name; all 20 effect knobs
  in six hairline panels (space, motion, tone, rhythm, pitch, grain & glue) to its right, above the keys; a detail
  panel for the selected effect's extra controls; a deck row (octave, demo, bpm, midi, stop, live tip text); the
  line-art keyboard (4 octaves desktop, 3 tablet, 2 phone).
- **The moon:** a WebGL holographic-foil sphere lit as a crescent, with a fine foil relief that rotates slowly, a
  faint glimmer on the night side and a thin holographic limb. Played notes brighten it and widen the crescent
  (driven by the keys' note-on/off light envelopes). Tapping it loads a random sound. Two hairline orbits with small
  satellites circle it. Stars fill the window.
- **Themes:** Rose, Gold, Silver (default), Violet, Prism. Each defines window tones, hairline tint, accent and a
  six-colour foil spectrum (registered CSS colours, 0.8 s morph); the WebGL moon and knobs lerp the same palette.
- **Knobs:** brushed-metal caps (concentric brushing, anisotropic highlight) carrying holographic film, a polished
  chrome rim, a domed chrome bead indicator ringed with light when the effect is on, and an LED ring of glass beads
  lit to the current value (bipolar controls light from centre). Bypassed effects keep their LEDs dimly lit.
- **Keyboard:** line-art keys: hairline separators, outlined black keys. A key fills with foil light while its note
  sounds (brightness from velocity), spills light onto neighbours and fades after release, from the real note events.
- **Wordmark:** "lumelody" in Cormorant Garamond italic with a chrome fill tinted by the foil spectrum and a light
  sweep on open.
- **Limits:** CSS + WebGL approximation of foil and chrome, not physically based rendering. Without WebGL the
  foil falls back to a still gradient and knobs to a flat drawing.

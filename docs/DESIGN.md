# LUMELODY visual design (web instrument; the plugin UI will follow the same system)

- **Direction:** a night-sky instrument with one hero object. Hairline panels, lowercase monospace labels
  (JetBrains Mono, OFL), an italic serif for the wordmark and sound names (Cormorant Garamond, OFL). Holographic foil is
  used sparingly as a precious material: the moon, the wordmark, the knob caps, lit keys, one foil hairline on the frame.
- **Layout:** wordmark + sound picker + themes on top; the moon on the left with the sound name; all 20 effect knobs
  in six hairline panels (space, motion, tone, rhythm, pitch, grain & glue) to its right, above the keys; a detail
  panel for the selected effect's extra controls; a deck row (octave, demo, bpm, midi, stop, live tip text); the
  line-art keyboard (4 octaves desktop, 3 tablet, 2 phone).
- **The moon:** a WebGL holographic-foil moon with sculpted impact craters (bowl + raised rim), darker smoother
  seas, a slight axial tilt and slow spin. Its colour comes from the surface angle between light and view (like a
  diffraction foil), strongest along the shadow line (a prism edge), with faint shimmering micro-lines, earthshine
  relief on the night side and a thin holographic limb. Played notes brighten it and widen the crescent (driven by
  the keys' note-on/off light envelopes). Two hairline orbits pass behind and in front of it, with satellites that
  go behind it. Renders every frame and lowers its own resolution on slow machines.
- **Tap motion:** the moon is a spring: it lifts slightly on hover, squashes to 90% on press and bounces back on
  release; a tap spins it, flares the light, sends out foil ripples and loads a random sound, whose name fades in
  out of a blur. Keys press with a quick tilt and spring back with a small overshoot; their light rises from the
  front edge on the strike and sinks as it fades.
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

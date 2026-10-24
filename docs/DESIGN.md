# LUMELODY visual design (web instrument; the plugin UI will follow the same system)

- **Layout:** one compact window. Small logo + sound picker on top, all 20 effect knobs in grouped rows
  (Space, Motion, Tone, Rhythm, Pitch, Texture, Dynamics) directly above the keys, a detail strip for the selected
  effect's extra controls, and the keyboard at the bottom (4 octaves on desktop, 3 on tablet, 2 on phone).
- **Holographic foil:** the page background and the plugin body are a live diffraction-foil surface rendered in
  WebGL: a crinkled sheet (domain-warped noise, baked once per size into a texture) re-lit every frame by a light
  that follows the pointer and drifts slowly. The surface tilt picks a colour from the theme's six-colour foil
  spectrum, with silver diffuse, specular glints, foil-can streaks and fine sparkle. Panels are smoked glass
  (backdrop blur) so the foil glows through and text stays readable. The bezel is streaky holographic chrome.
- **Themes:** Rose, Gold, Silver (default), Violet, Prism Chrome. Each defines metal, glass, accent and a six-colour
  foil spectrum as registered CSS colour properties, so switching morphs every surface over 0.8 s; the WebGL
  knobs, logo and foil lerp the same palette. The choice is remembered.
- **Knobs:** brushed-metal caps (concentric brushing, anisotropic highlight) carrying holographic film, a polished
  chrome rim, a domed chrome bead indicator ringed with light when the effect is on, and an LED ring of glass beads
  lit to the current value (bipolar controls light from centre). Bypassed effects keep their LEDs dimly lit.
- **Keyboard:** line-art keys on a dark bed: holographic foil separator lines, black keys as foil outlines, a foil
  frame. A key fills with foil light while its note sounds (brightness from velocity), spills light onto its
  neighbours and fades out after release, driven by the real note-on/off events (mouse, computer keys, MIDI).
- **Wordmark:** LUMELODY in Jost (SIL Open Font License) with wide tracking, chrome fill with foil tint, a light
  sweep on open, and a WebGL chrome crescent beside it.
- **Limits:** CSS + WebGL approximation of foil and chrome, not physically based rendering. Without WebGL the
  foil falls back to a still gradient and knobs to a flat drawing.

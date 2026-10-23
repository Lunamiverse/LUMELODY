# LUMELODY visual design (web instrument; the plugin UI will follow the same system)

- **Themes:** Rose, Gold, Silver, Violet, Prism. Each is a full material set (body, metal, pearl, glass, film hues,
  accent) defined as registered CSS colour properties, so switching themes morphs every surface over 0.8 s.
  The knob shader lerps its own material uniforms over the same time. The choice is remembered.
- **Knobs:** rendered on the GPU (one shared WebGL context) with a domed, concentric-brushed cap, polished chamfer,
  machined knurling that rotates with the value, Fresnel edge light, thin-film interference tinted by the theme's
  film hues, a contact shadow away from the light, and an engraved indicator with a luminous inlay. The light
  direction follows the pointer, so reflections move. Bypassed effects keep the material but go quieter.
  If WebGL is unavailable the knobs fall back to a flat drawing (stated on the page).
- **Keyboard:** pearl white keys with an iridescent film layer whose phase shifts with the light and with each key's
  tilt; smoked-glass black keys with a spectral edge; keys rotate down on press with a contact-shadow change and a
  restrained glow, independently per key, synced to the actual note events.
- **Wordmark:** LUMELODY set in Jost (SIL Open Font License, free for commercial use) with wide tracking and a
  pearl-holographic fill; the "O" is the crescent emblem (`design/lumelody-emblem.svg`).
- **Limits:** this is CSS + WebGL approximation of thin-film and pearl materials, not physically based rendering.

# LUMELODY visual design (web instrument; the plugin UI will follow the same system)

- **Direction:** restrained silver chrome on near-black obsidian, with a hint of rose, violet and icy cyan coming from
  the lighting and the material, not painted on. One hero object (the moon), a liquid-chrome frame, dark-glass panels.
- **Typography system:**
  - *Logo:* a custom vector wordmark (drawn as SVG paths, not a font): monoline geometric capitals, cap height 100,
    stroke 7.5, wide tracking. Signatures: the M's middle is a draped crescent swoop; the E's middle bar floats free;
    the O is a moon, with a crescent of light inside its ring; the Y's arms form a crescent cup on a stem. Polished
    silver chrome stroke with faint rose/cyan in the lower reflections, a light sweep on open. Reads in plain black.
  - *Sound and preset names, titles:* Federo (OFL, Google Fonts), capitals, wide tracking; art-deco geometry that
    feels otherworldly but stays readable small. The selected sound has a silver finish and a fine luminous line;
    in the browser the selected item gets a slim chrome indicator and a silver highlight. Long names step down in
    size to stay on one line.
  - *Labels, values, technical text:* Martian Mono (OFL), light, semi-condensed, lowercase.
  - Evaluated and not chosen for names: Italiana and Gruppo (too thin small), Julius Sans One and Syne (elegant
    but generic), Poiret One (too decorative), Tenor Sans (too plain), Montserrat Alternates (playful).
  - Fallbacks: Federo → Josefin Sans/Avenir Next/Futura; Martian Mono → SF Mono/Menlo/Consolas. The plugin build
    must embed both font files (they are OFL) so it never depends on the network.
- **Layout:** wordmark + sound picker + themes on top; the moon on the left with the sound name; all 20 effect knobs
  in six hairline panels (space, motion, tone, rhythm, pitch, grain & glue) to its right, above the keys; a detail
  panel for the selected effect's extra controls; a deck row (octave, demo, bpm, midi, stop, live tip text); the
  line-art keyboard (4 octaves desktop, 3 tablet, 2 phone).
- **The moon:** a WebGL holographic-foil moon with sculpted impact craters (bowl + raised rim), darker smoother
  seas, a slight axial tilt and slow spin. Its colour comes from the surface angle between light and view (like a
  diffraction foil), strongest along the shadow line (a prism edge), with faint shimmering micro-lines, earthshine
  relief on the night side and a thin holographic limb. Played notes brighten it and widen the crescent (driven by
  the keys' note-on/off light envelopes). Renders every frame and lowers its own resolution on slow machines.
- **Rings:** real planetary rings rendered in the same shader: the view ray meets a tilted ring plane; density by
  radius from layered ringlets with a bright middle ring, a wide division and a fine gap; translucent, foil-tinted
  ice lit by the same light. They are hidden behind the moon and cross in front of it, the moon casts its shadow
  on them and they cast theirs across the crescent.
- **Background:** a nebula painted once per theme/size from the theme's foil colours (domain-warped clouds, dark
  dust lanes, a glow behind the moon, vignette) with stars, some tinted, some glowing.
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
- **Limits:** CSS + WebGL approximation of foil and chrome, not physically based rendering. Without WebGL the
  foil falls back to a still gradient and knobs to a flat drawing.

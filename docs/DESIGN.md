# LUMELODY visual design (web instrument; the plugin UI will follow the same system)

- **Direction:** restrained silver chrome on near-black obsidian, with a hint of rose, violet and icy cyan coming from
  the lighting and the material, not painted on. One hero object (the moon), a liquid-chrome frame, dark-glass panels.
- **Pixel typography (current):** Pixelify Sans (OFL; modern, clean pixel face) for sound names, titles, buttons and
  readable text; Silkscreen (OFL; crisp micro-pixel, set at its native 8 px) for knob labels, values and panel titles.
  The selected sound is set in Pixelify Sans 600 capitals with a holographic glass gradient, a soft magenta/cyan
  glow and a light sweep on each change. The logo is a hand-drawn pixel wordmark on a 9-row grid (one-pixel strokes, wide
  tracking, crisp square pixels, white silver with a soft glow and a slow light sweep; floating E bar, crescent-cup
  Y, moon O with its lit edge against the ring). Cliché arcade faces (Press Start 2P)
  and terminal faces (VT323) were rejected. The plugin build must embed both font files.
- **Typography system:**
  - *Logo:* a custom vector wordmark (drawn as SVG paths, not a font): monoline geometric capitals, cap height 100,
    stroke 9, wide tracking, L with an upturned crescent flick. Liquid-chrome bevel (SVG lighting filter), a slow
    holographic film in soft-light, and a light sweep every 8 s. Signatures: the M's middle is a draped crescent swoop; the E's middle bar floats free;
    the O is a moon, with a crescent of light inside its ring; the Y's arms form a crescent cup on a stem. Polished
    silver chrome stroke with faint rose/cyan in the lower reflections, a light sweep on open. Reads in plain black.
  - *Sound and preset names, effect and panel titles:* LUMI TYPE, a custom alphabet drawn for LUMELODY (A-Z, 0-9,
    space - / & . ') in the same monoline system as the wordmark: draped crescent M, floating E/F bars,
    crescent-cup Y, round letters on true ellipses. Rendered as SVG strokes (thicker strokes at small sizes), so it
    needs no font file. The selected sound gets the liquid-chrome finish, a light sweep on every change and a fine
    luminous line; list items are drawn in ink, the selected one in chrome with a slim indicator. Long names scale
    down to fit.
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
- **Background:** sculpted obsidian glass (WebGL): a slow domain-warped height field reflecting the studio faintly
  like black glass, turning into polished liquid-chrome folds toward the edges, with pearlescent sheen and faint
  thin-film interference where it bends, the moon's light on the glass, a vignette and fine grain. No stars.
- **Tap motion:** the moon is a spring: it lifts slightly on hover, squashes to 90% on press and bounces back on
  release; a tap spins it, flares the light, sends out foil ripples and loads a random sound, whose name fades in
  out of a blur. Keys press with a quick tilt and spring back with a small overshoot; their light rises from the
  front edge on the strike and sinks as it fades.
- **Clear-plastic look (the only theme), after the CD jewel case and the clear cassette:** pure black, no background
  art. Panels and buttons are colourless clear moulded plastic: bright rims, an inner moulded rim line, a gloss streak,
  a faint white-to-spectrum refraction along the rim, corner screw bosses. The window border is polished silver like a
  silver vinyl record: monochrome, high contrast, fine grooves running along the border, long anisotropic sheen bands,
  a thin rainbow fringe on the outer edge, two large blown-out light blooms and a small one, with horizontal rainbow
  lens flares that spill softly just inside the frame. Knobs are clear acrylic discs like tape-reel hubs (see-through
  body, solid-looking edges with rim refraction, moulded rings, six hub teeth that turn with the value, a bright
  indicator slit, a gloss streak) with a white value arc. All text is white (no coloured type). Colour lives only in
  the moon and its ring, like the disc in the case.
- **Theme:** one only, Crystal Holo (the theme picker was removed). Each defines window tones, hairline tint, accent and a
  six-colour foil spectrum (registered CSS colours, 0.8 s morph); the WebGL moon and knobs lerp the same palette.
- **Knobs:** brushed-metal caps (concentric brushing, anisotropic highlight) carrying holographic film, a polished
  chrome rim, a domed chrome bead indicator ringed with light when the effect is on, and an LED ring of glass beads
  lit to the current value (bipolar controls light from centre). Bypassed effects keep their LEDs dimly lit.
- **Keyboard:** line-art keys: hairline separators, outlined black keys. A key fills with foil light while its note
  sounds (brightness from velocity), spills light onto neighbours and fades after release, from the real note events.
- **Limits:** CSS + WebGL approximation of foil and chrome, not physically based rendering. Without WebGL the
  foil falls back to a still gradient and knobs to a flat drawing.

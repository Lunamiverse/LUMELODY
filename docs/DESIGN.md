# LUMELODY visual design (web instrument and plugin window share one interface)

- **Direction:** pure black, colourless clear moulded plastic, white text. Colour lives in the moon, its ring and the
  lit pads/keys only. One theme: Crystal Holo.
- **Typography:** hand-drawn pixel wordmark (9-row grid, one-pixel strokes, moon O); sound names in the LUMELODY pixel
  alphabet (A-Z, 0-9, - / . ' &); Pixelify Sans for readable text and buttons, Silkscreen for micro labels (both OFL,
  embedded in the plugin). All type is white; no coloured text.
- **Layout:** wordmark + sound picker on top; the moon on the left with the sound name; the effect panels to its right;
  the detail strip; the deck row; the line-art keyboard. The Chord Progression Finder opens in the effects area, so the
  moon stays visible.
- **The moon:** WebGL holographic-foil moon with sculpted craters and planetary rings; notes brighten it and widen the
  crescent. Tap: spring press, spin, ripples, random sound.
- **Disco & Club section only:** the same moon (same silhouette, crescent lighting and ring) becomes a mirror ball:
  48 rows of slightly crooked mirror tiles with bevels, seams, occlusion and a hanging cap, reflecting a club (pink wash,
  warm orange, blue, magenta, violet, icy glints and the moon's own key light), supersampled 4x for crisp seams. It throws
  light spots and faint beams that move with the rotation, plus a few slow glints. Entry: tiles catch the light from the
  lit edge inward, then a gentle turn and a short burst; leaving reverses it. Reduced motion / "animations off": instant
  swap, no spin. The page background stays black.
- **Borders:** colourless clear plastic tube with blooms, white lens streaks and sheen (no rainbow).
- **Panels and buttons:** clear moulded plastic with bright rims, an inner rim line, a gloss streak and screw bosses.
- **Knobs:** ray-marched fluted clear-acrylic knobs (n = 1.49) on a turned-metal ring, refraction, shadow and caustic;
  white value arc in perspective.
- **Keyboard:** line-art keys that fill with light from the real note events (also the host's notes in the plugin).
- **Chord Progression Finder** (calm, predictable, nothing flashes): three tabs in a fixed order.
  - *Chord blocks:* style, mood, richness → generate / surprise me; the choices fold into one line once there is a
    progression. Blocks show chord, feeling ("home", "lift", "tension"…) and length; tap to hear, drag to move, chord drawer
    to drag in. Make it sound expensive with an original/expensive compare. Undo/redo, autosave, settings for advanced
    mode, previews, animations, explanations, larger text, guide.
  - *Neon pads:* 8 x 4 pads rendered in one WebGL pass: thick transparent polymer rims (refraction of the pad's glow,
    Fresnel studio reflections, crisp lips, light running round the rim when played), pearlescent neon surfaces with
    liquid light, a luminous core and an inner reflection, contact shadows, a subtle press-in. Palette: hot pink, magenta,
    ultraviolet, deep purple, icy cyan (cyan = root/home).
  - *Library:* the curated R&B, psychedelic trap and disco & club progressions with producer notes.
- **Limits:** WebGL approximations of optics, not physically based rendering; without WebGL the moon and pads fall back
  to still drawings.

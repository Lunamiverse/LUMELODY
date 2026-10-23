"""Source definitions for the first LUMI MELODY synthesis presets.

Run make_presets.py to write one JSON file per preset into /presets.
Layer types: 1 VA (band-limited osc + unison), 2 FM, 3 physical string (Karplus-Strong),
4 modal mallet/bell, 5 filtered noise, 6 formant voice, 7 grain cloud.
"""

def L(type, **kw):
    kw["type"] = type
    return kw

PRESETS = []

def preset(name, category, genres, desc, use, phrase, layers, voice=None, fx=None, rng=(36, 96)):
    PRESETS.append({
        "name": name, "category": category, "genres": genres, "description": desc, "use": use,
        "preview_phrase": phrase, "range": list(rng), "source": "original synthesis (LUMI engine)",
        "layers": layers, "voice": voice or {}, "fx": fx or {},
    })

# ======================================================== KEYS
preset("Velvet Tine", "Keys", ["R&B", "Neo-soul", "Latin pop"],
       "Rhodes-inspired FM electric piano: round body, soft bell bark when you dig in, slow stereo tremolo.",
       "Chords and top-lines in slow R&B; gets brighter with velocity.", "keys_rnb",
       [L(2, cr=1, mr=1, idx=1.3, idx_dec=.9, idx_sus=.18, fb=.04, idx_vel=.8, fm_spread=5, a=.002, d=3.2, s=.0, r=.4, keydecay=.45, vel=.75),
        L(2, cr=1, mr=14, idx=1.4, idx_dec=.012, idx_sus=0, idx_vel=1, a=.001, d=.35, s=0, r=.1, level=.28, vel=1),
        L(1, wave=0, a=.003, d=2.5, s=0, r=.3, level=.15, filt=0)],
       {"f_type": -1, "lfo_rate": 3.6, "lfo_pan": .3},
       {"eq_ls_f": 220, "eq_ls_g": 1.5, "eq_hs_f": 7000, "eq_hs_g": -2, "ch_mix": .25, "ch_depth": 1.5, "ch_rate": .5,
        "rv_mix": .16, "rv_size": .45, "rv_decay": 1.8, "rv_damp": .5})

preset("Midnight Wurl", "Keys", ["R&B", "Pop", "Lo-fi"],
       "Reedy Wurlitzer-inspired keys with a gritty growl at high velocity and a gentle tremolo.",
       "Comping, intimate verses, lo-fi R&B.", "keys_rnb",
       [L(1, wave=3, pw=.42, a=.002, d=1.8, s=.0, r=.25, keydecay=.4, vel=.8),
        L(2, cr=1, mr=2, idx=1.8, idx_dec=.18, idx_sus=.1, idx_vel=1, a=.001, d=1.2, s=0, r=.2, level=.55, keydecay=.4)],
       {"f_type": 0, "f_cut": 1400, "f_res": .12, "f_env": 2.2, "fa": .001, "fd": .35, "fs": .1, "f_vel": 1.6, "f_drive": 1.8,
        "lfo_rate": 5.2, "lfo_amp": .18},
       {"sat_drive": .25, "eq_lc": 70, "rv_mix": .12, "rv_size": .35, "rv_decay": 1.2})

preset("Satin Keys", "Keys", ["R&B", "Alt R&B", "Ambient"],
       "Soft, felt-like digital keys — very little attack, warm low-mids, closes the room around a vocal.",
       "Late-night R&B chords that leave space for vocals.", "keys_rnb",
       [L(2, cr=1, mr=2, idx=.55, idx_dec=.6, idx_sus=.2, idx_vel=.7, a=.008, d=2.8, s=.2, r=.8, keydecay=.3, vel=.6),
        L(1, wave=1, a=.01, d=3, s=.25, r=.8, level=.35, keydecay=.3),
        L(5, ncut=3.5, ntrack=1, nq=1.2, nmode=1, a=.001, d=.04, s=0, r=.05, level=.12, filt=0)],
       {"f_type": 0, "f_cut": 2600, "f_res": .05, "f_env": 1, "fd": .6, "f_key": .6},
       {"eq_ls_f": 180, "eq_ls_g": 2, "eq_hc": 9000, "ch_mix": .3, "ch_depth": 2, "rv_mix": .22, "rv_size": .6, "rv_decay": 2.6, "rv_damp": .6})

preset("Ghost Detune", "Keys", ["Trap", "Dark pop", "Drill"],
       "Dark, slightly seasick detuned keys. Three drifting saws under a soft FM body — haunting, not muddy.",
       "Dark trap chord loops, eerie intros.", "keys_dark",
       [L(1, wave=2, uni=3, det=22, spread=.8, drift=9, a=.004, d=2.4, s=.25, r=.6, vel=.6),
        L(2, cr=1, mr=1, idx=.9, idx_dec=.5, idx_sus=.1, a=.002, d=2, s=.1, r=.5, level=.6)],
       {"f_type": 0, "f_cut": 900, "f_res": .18, "f_env": 2.3, "fa": .002, "fd": .5, "fs": .15, "f_key": .7,
        "vib_rate": .35, "vib_depth": 7, "vib_delay": 0},
       {"eq_lc": 90, "eq_pk_f": 350, "eq_pk_g": -2.5, "ch_mix": .35, "ch_depth": 2.5, "ch_rate": .25,
        "rv_mix": .3, "rv_size": .7, "rv_decay": 3.5, "rv_damp": .55, "width": 1.2})

preset("Dream Glass EP", "Keys", ["R&B", "Pop", "Reggaeton"],
       "Glassy electric piano with a sparkling grain halo and dotted-eighth echoes.",
       "Dreamy intros, reggaeton romantics, pop pre-choruses.", "keys_rnb",
       [L(2, cr=1, mr=3.5, idx=1.1, idx_dec=.5, idx_sus=.12, idx_vel=.7, fm_spread=6, a=.002, d=2.6, s=.05, r=.6, keydecay=.4),
        L(2, cr=1, mr=1, idx=.6, idx_dec=1.2, idx_sus=.3, a=.003, d=3, s=.1, r=.6, level=.6),
        L(7, oct=1, grate=14, glen=.18, goct=.4, gfifth=.1, gjit=6, a=.3, d=3, s=.3, r=1.2, level=.22, filt=0)],
       {"f_type": -1},
       {"eq_lc": 80, "dly_beats": .75, "dly_fb": .32, "dly_mix": .18, "dly_damp": .55,
        "rv_mix": .3, "rv_size": .75, "rv_decay": 3.8, "rv_damp": .4, "rv_pre": 25})

preset("Tape Memory", "Keys", ["Lo-fi", "R&B", "Trap"],
       "Vintage digital keys bounced through a worn tape: gentle wow, softened top, a touch of bit grit.",
       "Sample-style loops, nostalgic R&B and trap.", "keys_rnb",
       [L(2, cr=1, mr=1, idx=1.1, idx_dec=.7, idx_sus=.2, fb=.06, a=.002, d=2.5, s=.05, r=.4, keydecay=.4),
        L(2, cr=2, mr=1, idx=.5, idx_dec=.3, idx_sus=.05, a=.002, d=1.2, s=0, r=.3, level=.35)],
       {"f_type": -1, "vib_rate": .8, "vib_depth": 9, "vib_delay": 0},
       {"sat_drive": .3, "crush_sr": 22000, "crush_bits": 11, "eq_lc": 90, "eq_hc": 6500, "eq_ls_f": 250, "eq_ls_g": 2,
        "rv_mix": .14, "rv_size": .4, "rv_decay": 1.5})

preset("Sol de Bossa", "Keys", ["Bossa / MPB", "Latin pop", "R&B"],
       "Mellow, rounded electric keys with a soft vibraphone shimmer — Brazilian-inspired colour for extended chords.",
       "Syncopated comping on maj9/13 chords, Latin pop and neo-soul.", "keys_bossa",
       [L(2, cr=1, mr=1, idx=.9, idx_dec=.8, idx_sus=.2, idx_vel=.6, a=.003, d=2.2, s=.05, r=.35, keydecay=.4),
        L(4, mat=1, hard=.35, click=.05, decay=2.5, beat=2, a=.001, d=3, s=.0, r=.4, level=.35)],
       {"f_type": 0, "f_cut": 3500, "f_res": .05, "lfo_rate": 4.8, "lfo_amp": .1},
       {"eq_ls_f": 200, "eq_ls_g": 1.5, "ch_mix": .2, "rv_mix": .18, "rv_size": .5, "rv_decay": 2})

preset("Prism Organ", "Keys", ["House", "Pop", "Experimental"],
       "Hybrid organ-synth: drawbar-like square stack with an 'ooh' formant layer and a soft rhythmic pump.",
       "House stabs, gospel-tinged pop chords, experimental hooks.", "pad",
       [L(1, wave=3, pw=.5, a=.005, d=1, s=.85, r=.15, level=.6),
        L(1, wave=0, oct=1, a=.005, d=1, s=.8, r=.15, level=.35),
        L(6, vowel=3.2, vmod=.4, tilt=1.2, a=.08, d=1, s=.9, r=.3, level=.4)],
       {"f_type": 0, "f_cut": 3200, "f_res": .1, "lfo_rate": 5.5, "lfo_amp": .08},
       {"pump": .35, "ch_mix": .4, "ch_depth": 3, "ch_rate": 1.1, "rv_mix": .15, "eq_lc": 100})

# ======================================================== PLUCKS & MALLETS
preset("Crystal Drop", "Plucks & Mallets", ["Reggaeton", "Latin pop", "Pop"],
       "Clear glass pluck: inharmonic glass modes over a soft FM core, with tempo echoes that trail off.",
       "Romantic reggaeton toplines, catchy bell hooks.", "bell_latin",
       [L(4, mat=2, hard=.65, click=.15, decay=1.4, beat=1.5, a=.001, d=2, s=0, r=.3, level=.8, keydecay=.2),
        L(2, cr=1, mr=4, idx=1, idx_dec=.08, idx_sus=0, a=.001, d=.9, s=0, r=.25, level=.55)],
       {"f_type": -1},
       {"eq_lc": 180, "eq_hs_f": 9000, "eq_hs_g": -1.5, "dly_beats": .75, "dly_fb": .35, "dly_mix": .22, "dly_damp": .5,
        "rv_mix": .22, "rv_size": .6, "rv_decay": 2.4})

preset("Chrome Pick", "Plucks & Mallets", ["Dancehall", "Afroswing", "Pop"],
       "Bright metallic pluck — FM steel bite plus a tight string body. Sits on top of busy drums.",
       "Rhythmic riffs, counter-melodies over percussion.", "pluck_dancehall",
       [L(2, cr=1, mr=3.01, idx=2.6, idx_dec=.06, idx_sus=0, idx_vel=.9, a=.001, d=.6, s=0, r=.15),
        L(3, bright=.85, damp=.15, decay=.9, pick=.13, a=.001, d=2, s=0, r=.15, level=.6)],
       {"f_type": 2, "f_cut": 250},
       {"eq_pk_f": 2500, "eq_pk_g": 1.5, "dly_beats": .5, "dly_fb": .2, "dly_mix": .12, "rv_mix": .12, "rv_size": .3})

preset("Teak Pluck", "Plucks & Mallets", ["Afrobeats", "Amapiano", "Dancehall"],
       "Warm wooden pluck: log-drum style modal body with a dampened string tick. Organic and percussive.",
       "Afrobeats riffs, call-and-response with percussion.", "pluck_afro",
       [L(4, mat=7, hard=.55, click=.14, decay=.55, a=.001, d=1, s=0, r=.12),
        L(3, bright=.5, damp=.35, decay=.4, pick=.3, a=.001, d=1, s=0, r=.1, level=.5)],
       {"f_type": -1},
       {"eq_ls_f": 300, "eq_ls_g": 1.5, "eq_lc": 70, "rv_mix": .12, "rv_size": .35, "rv_decay": 1})

preset("Round Candy", "Plucks & Mallets", ["Dembow", "Pop", "House"],
       "Rounded analog pluck with a quick filter snap. Punchy, sweet and very clear in a dense mix.",
       "Dembow motifs, bouncy pop and house riffs.", "pluck_dembow",
       [L(1, wave=2, uni=2, det=9, spread=.5, a=.001, d=.45, s=0, r=.15),
        L(1, wave=3, pw=.5, oct=-1, a=.001, d=.35, s=0, r=.12, level=.35)],
       {"f_type": 0, "f_cut": 450, "f_res": .25, "f_env": 4.6, "fa": .001, "fd": .16, "fs": 0, "f_vel": 1.2, "f_key": .8},
       {"sat_drive": .15, "eq_lc": 120, "dly_beats": .5, "dly_fb": .2, "dly_mix": .1, "rv_mix": .1, "rv_size": .3})

preset("Isla Bright", "Plucks & Mallets", ["Dancehall", "Soca", "Afroswing"],
       "Bright tropical steel-drum colour fused with a short square pluck — sunny, playful, cutting.",
       "Dancehall and soca hooks, summery pop.", "pluck_dancehall",
       [L(4, mat=6, hard=.7, click=.2, decay=.9, beat=3, a=.001, d=1.5, s=0, r=.2),
        L(1, wave=3, pw=.35, a=.001, d=.18, s=0, r=.08, level=.25)],
       {"f_type": 0, "f_cut": 6000},
       {"eq_lc": 200, "dly_beats": .75, "dly_fb": .25, "dly_mix": .12, "rv_mix": .15, "rv_size": .4})

preset("Liquid Drip", "Plucks & Mallets", ["Trap", "Jersey club", "Experimental"],
       "Watery droplet pluck: resonant band-pass sweep with a falling pitch blip, like a drop hitting glass water.",
       "Ear-candy melodies, plinky trap and Jersey club fills.", "glitch",
       [L(1, wave=2, a=.001, d=.35, s=0, r=.15),
        L(1, wave=0, a=.001, d=.4, s=0, r=.15, level=.5, filt=0)],
       {"f_type": 1, "f_cut": 3200, "f_res": .75, "f_env": -2.3, "fa": .001, "fd": .12, "fs": 0, "f_key": .9,
        "p_env": 7, "p_dec": .02},
       {"eq_lc": 150, "dly_beats": .375, "dly_fb": .4, "dly_mix": .2, "dly_damp": .3, "rv_mix": .18, "rv_size": .5})

preset("Halo Bell", "Plucks & Mallets", ["Trap", "Drill", "Dark pop"],
       "Dark, bell-like pluck with a long halo: FM bell partials over a soft church-bell hum.",
       "Classic dark trap bell melodies, drill toplines.", "bell_trap",
       [L(2, cr=1, mr=3.5, idx=2.2, idx_dec=.25, idx_sus=.05, idx_vel=.7, fm_spread=4, a=.001, d=2.5, s=0, r=.6, keydecay=.3),
        L(4, mat=3, hard=.45, click=.05, decay=2.5, beat=1, a=.001, d=3, s=0, r=.6, level=.35)],
       {"f_type": 0, "f_cut": 5000, "f_key": .3},
       {"eq_lc": 140, "eq_pk_f": 2800, "eq_pk_g": -2, "rv_mix": .3, "rv_size": .7, "rv_decay": 3.5, "rv_damp": .5,
        "dly_beats": 1, "dly_fb": .25, "dly_mix": .1})

preset("Lumen Mallet", "Plucks & Mallets", ["R&B", "Ambient", "Cinematic"],
       "Shimmering vibraphone-style mallet with motor tremolo and a halo of grains an octave above.",
       "Dreamy R&B fills, ambient beds, film cues.", "bell_latin",
       [L(4, mat=1, hard=.5, click=.12, decay=3, beat=2.5, a=.001, d=4, s=0, r=.8),
        L(7, oct=1, grate=10, glen=.2, goct=.3, gjit=5, a=.2, d=2.5, s=0, r=1, level=.18, filt=0)],
       {"f_type": -1, "lfo_rate": 5.8, "lfo_amp": .22},
       {"eq_lc": 100, "rv_mix": .3, "rv_size": .7, "rv_decay": 3.6, "rv_pre": 20})

preset("Obsidian Pluck", "Plucks & Mallets", ["Drill", "Trap", "Dark pop"],
       "Dark, muted string pluck with a sub shadow underneath. Moody and close.",
       "Drill melodies, sinister counter-lines.", "keys_dark",
       [L(3, bright=.35, damp=.42, decay=1.6, pick=.25, strings=2, sdet=5, a=.001, d=3, s=0, r=.25),
        L(1, wave=0, oct=-1, a=.002, d=.6, s=0, r=.2, level=.3, filt=0)],
       {"f_type": 0, "f_cut": 1600, "f_res": .1, "f_key": .5},
       {"eq_lc": 60, "ch_mix": .2, "rv_mix": .22, "rv_size": .55, "rv_decay": 2.2, "rv_damp": .6})

preset("Neon Shard", "Plucks & Mallets", ["Jersey club", "Hyperpop", "Futuristic"],
       "Sharp futuristic pluck: double-modulator FM with a downward zap and a little digital crunch.",
       "Jersey club and hyperpop riffs, sci-fi ear candy.", "glitch",
       [L(2, cr=1, mr=2, idx=2, idx_dec=.08, idx_sus=0, mr2=7.13, idx2=2.5, idx2_dec=.03, a=.001, d=.35, s=0, r=.1),
        L(1, wave=2, uni=3, det=15, spread=1, a=.001, d=.2, s=0, r=.1, level=.4)],
       {"f_type": 0, "f_cut": 7000, "p_env": -12, "p_dec": .012},
       {"crush_bits": 9, "eq_lc": 200, "dly_beats": .25, "dly_fb": .3, "dly_mix": .15, "rv_mix": .1, "width": 1.3})

preset("Kalimba Dusk", "Plucks & Mallets", ["Afrobeats", "R&B", "Lo-fi"],
       "Thumb-piano style tines with a soft woody knock and a short room.",
       "Afro-R&B riffs, mellow loops.", "pluck_afro",
       [L(4, mat=4, hard=.55, click=.18, decay=1.6, beat=1.2, a=.001, d=2, s=0, r=.2),
        L(3, bright=.4, damp=.3, decay=.6, pick=.4, a=.001, d=1, s=0, r=.1, level=.25)],
       {"f_type": -1},
       {"eq_lc": 120, "eq_ls_f": 400, "eq_ls_g": 1, "rv_mix": .16, "rv_size": .4, "rv_decay": 1.4})

# ======================================================== LEADS
preset("Silk Glide", "Synths & Leads", ["R&B", "Trap", "Afroswing"],
       "Smooth monophonic lead with natural portamento and delayed vibrato — sings like a vocal ad-lib.",
       "Melodic hooks, vocal-style counter-melodies.", "lead_mono",
       [L(1, wave=2, uni=3, det=7, spread=.4, a=.01, d=1, s=.8, r=.25),
        L(1, wave=3, pw=.5, oct=-1, a=.01, d=1, s=.8, r=.25, level=.3)],
       {"mono": 1, "glide": .06, "f_type": 0, "f_cut": 2100, "f_poles": 4, "f_res": .2, "f_env": 1, "fd": .4, "fs": .3, "f_key": .5,
        "vib_rate": 5.3, "vib_depth": 18, "vib_delay": .35},
       {"eq_lc": 120, "dly_beats": .75, "dly_fb": .3, "dly_mix": .15, "rv_mix": .2, "rv_size": .6, "rv_decay": 2.5})

preset("Analog Honey", "Synths & Leads", ["Pop", "Funk", "R&B"],
       "Warm analog-style lead, saw plus narrow pulse with drifting oscillators and a vocal filter bite.",
       "G-funk-style whistles, pop leads, solos.", "lead_mono",
       [L(1, wave=2, drift=6, a=.005, d=.8, s=.7, r=.2),
        L(1, wave=3, pw=.3, drift=6, fine=6, a=.005, d=.8, s=.7, r=.2, level=.7)],
       {"mono": 1, "glide": .03, "f_type": 0, "f_cut": 1100, "f_res": .35, "f_env": 2.2, "fa": .005, "fd": .3, "fs": .35,
        "vib_rate": 5.5, "vib_depth": 12, "vib_delay": .4},
       {"sat_drive": .15, "eq_lc": 100, "rv_mix": .15, "rv_size": .5, "dly_beats": .5, "dly_fb": .2, "dly_mix": .1})

preset("Torch Lead", "Synths & Leads", ["Drill", "Dark pop", "EDM"],
       "Distorted melodic lead: five-voice saw stack pushed into warm saturation, tamed by a low-pass.",
       "Aggressive hooks, drops, dark-pop choruses.", "lead_mono",
       [L(1, wave=2, uni=5, det=14, spread=.7, a=.005, d=1, s=.85, r=.2)],
       {"mono": 1, "glide": .04, "f_type": 0, "f_cut": 2400, "f_res": .2, "f_env": .8, "fd": .3, "fs": .5, "f_drive": 2.5,
        "vib_rate": 5.6, "vib_depth": 10, "vib_delay": .5},
       {"sat_drive": .55, "sat_mix": .8, "eq_lc": 150, "eq_hc": 7000, "eq_pk_f": 600, "eq_pk_g": -2,
        "rv_mix": .15, "rv_size": .5, "dly_beats": .75, "dly_fb": .25, "dly_mix": .1})

preset("Aria Vox", "Synths & Leads", ["Ambient", "R&B", "Cinematic"],
       "Vocal-like synth: moving 'ah-oh' formants with breath and vibrato. Human-feeling without samples.",
       "Wordless vocal hooks, choir-like toplines.", "lead_slow",
       [L(6, vowel=0, vmod=1.6, tilt=1.1, breath=.06, a=.06, d=1, s=.9, r=.5)],
       {"mono": 1, "glide": .08, "f_type": -1, "lfo_rate": .25, "vib_rate": 5.1, "vib_depth": 22, "vib_delay": .3},
       {"eq_lc": 140, "ch_mix": .3, "ch_depth": 2, "rv_mix": .35, "rv_size": .8, "rv_decay": 4, "rv_pre": 30})

preset("Pop Sugar", "Synths & Leads", ["Pop", "Latin pop", "Dance"],
       "Bright, catchy polyphonic lead: wide supersaw edge with a square core and eighth-note echoes.",
       "Choruses, drops, hooks that need to be heard.", "lead_pop",
       [L(1, wave=2, uni=5, det=13, spread=.9, a=.003, d=.6, s=.6, r=.2),
        L(1, wave=3, pw=.5, a=.003, d=.6, s=.6, r=.2, level=.5)],
       {"f_type": 0, "f_cut": 2600, "f_res": .15, "f_env": 1.5, "fd": .25, "fs": .4},
       {"eq_lc": 180, "eq_hs_f": 9000, "eq_hs_g": -2.5, "dly_beats": .5, "dly_fb": .28, "dly_mix": .14, "rv_mix": .16, "rv_size": .5, "width": 1.15})

preset("Phantom Whistle", "Synths & Leads", ["R&B", "Trap", "Ambient"],
       "Airy whistle lead: pure tone wrapped in tuned breath, slow vibrato and a big hall.",
       "Ethereal hooks, haunting trap melodies.", "lead_slow",
       [L(1, wave=0, a=.04, d=1, s=.9, r=.4),
        L(5, ncut=1, ntrack=1, nq=6, nmode=1, a=.04, d=1, s=.9, r=.4, level=.25)],
       {"mono": 1, "glide": .1, "f_type": -1, "vib_rate": 5.2, "vib_depth": 16, "vib_delay": .25},
       {"eq_lc": 200, "dly_beats": .75, "dly_fb": .35, "dly_mix": .14, "rv_mix": .38, "rv_size": .85, "rv_decay": 4.5, "rv_pre": 35})

preset("Cinder", "Synths & Leads", ["Cinematic", "Dark pop", "Trap"],
       "Dark, cinematic lead: seven-voice saw cloud behind a slowly breathing low-pass.",
       "Film-style themes, dark intros, low register melodies.", "lead_slow",
       [L(1, wave=2, uni=7, det=18, spread=.9, a=.08, d=1, s=.9, r=.8),
        L(1, wave=0, oct=-1, a=.08, d=1, s=.9, r=.8, level=.3, filt=0)],
       {"f_type": 0, "f_cut": 800, "f_res": .3, "f_key": .6, "lfo_rate": .18, "lfo_cut": .9},
       {"eq_lc": 70, "rv_mix": .32, "rv_size": .85, "rv_decay": 5, "rv_damp": .55, "width": 1.2})

preset("Hyperline", "Synths & Leads", ["Jersey club", "House", "Futuristic"],
       "Futuristic feedback-FM lead with a hard sync-like edge and a pitch flick on every note.",
       "Club hooks, futuristic toplines.", "lead_pop",
       [L(2, cr=1, mr=1, idx=1.4, idx_dec=.3, idx_sus=.45, fb=.6, a=.002, d=.5, s=.7, r=.15),
        L(1, wave=2, uni=3, det=10, spread=.8, a=.002, d=.5, s=.6, r=.15, level=.5)],
       {"f_type": 0, "f_cut": 3500, "f_res": .25, "f_env": 1, "fd": .15, "fs": .3, "p_env": 4, "p_dec": .015},
       {"crush_bits": 10, "eq_lc": 200, "dly_beats": .25, "dly_fb": .25, "dly_mix": .12, "rv_mix": .1, "width": 1.2})

# ======================================================== PADS & ATMOSPHERES
preset("Velour Pad", "Pads & Atmospheres", ["R&B", "Pop", "House"],
       "Warm analog pad: chorused saw ensemble with a soft attack and a velvety top.",
       "Chord beds under R&B and pop.", "pad",
       [L(1, wave=2, uni=5, det=14, spread=.9, drift=4, a=.5, d=2, s=.85, r=1.4)],
       {"f_type": 0, "f_cut": 1300, "f_res": .1, "f_key": .4, "lfo_rate": .15, "lfo_cut": .25},
       {"eq_lc": 120, "eq_pk_f": 400, "eq_pk_g": -2, "ch_mix": .35, "ch_depth": 2.5, "rv_mix": .25, "rv_size": .7, "rv_decay": 3})

preset("Horizon Wide", "Pads & Atmospheres", ["Cinematic", "Ambient", "Pop"],
       "Huge cinematic pad: wide saw ensemble blended with a soft choir-like 'ah' and a long hall.",
       "Film builds, emotional choruses, intros.", "pad",
       [L(1, wave=2, uni=7, det=24, spread=1, a=.9, d=2, s=.9, r=2.2),
        L(6, vowel=0, vmod=.6, tilt=1.2, breath=.05, a=1.2, d=2, s=.9, r=2.2, level=.45)],
       {"f_type": 0, "f_cut": 1800, "f_res": .08, "lfo_rate": .1, "lfo_cut": .3},
       {"eq_lc": 140, "rv_mix": .45, "rv_size": .95, "rv_decay": 6, "rv_damp": .45, "rv_pre": 40, "width": 1.3})

preset("Air Silk", "Pads & Atmospheres", ["Ambient", "R&B", "Afroswing"],
       "Airy, floating texture: tuned breath, high sine shimmer and a few drifting grains. Lives above the mix.",
       "Top-layer air on R&B/afro records, ambient beds.", "pad",
       [L(5, ncut=2, ntrack=1, nq=3, nmode=1, a=.8, d=2, s=.8, r=2, level=.7),
        L(1, wave=0, oct=1, a=.8, d=2, s=.6, r=2, level=.3),
        L(7, oct=1, grate=8, glen=.25, goct=.3, gjit=10, a=1, d=2, s=.8, r=2, level=.3)],
       {"f_type": 2, "f_cut": 400},
       {"eq_hs_f": 8000, "eq_hs_g": -2, "rv_mix": .45, "rv_size": .9, "rv_decay": 5, "width": 1.4})

preset("Nebula Drift", "Pads & Atmospheres", ["Ambient", "Trap", "R&B"],
       "Dreamy ambient layer: a cloud of soft grains over a slow FM pad that breathes in and out.",
       "Ambient interludes, spacious trap and R&B.", "pad",
       [L(7, grate=22, glen=.3, goct=.2, gfifth=.2, gjit=12, a=1.2, d=2, s=.9, r=2.5, level=.8),
        L(2, cr=1, mr=2, idx=.4, idx_dec=3, idx_sus=.3, fm_spread=8, a=1, d=2, s=.8, r=2.5, level=.6)],
       {"f_type": 0, "f_cut": 2200, "lfo_rate": .12, "lfo_cut": .6},
       {"eq_lc": 130, "dly_beats": 1.5, "dly_fb": .4, "dly_mix": .15, "dly_damp": .6, "rv_mix": .45, "rv_size": .9, "rv_decay": 6})

preset("Abyss Bloom", "Pads & Atmospheres", ["Dark pop", "Trap", "Cinematic"],
       "Dark evolving pad — a low saw mass whose filter slowly blooms open and closes again.",
       "Dark intros, tension builds, drill/trap atmospheres.", "pad_dark",
       [L(1, wave=2, uni=6, det=16, spread=.9, drift=6, a=1.2, d=2, s=.9, r=2),
        L(1, wave=0, oct=-1, a=1, d=2, s=.9, r=2, level=.25, filt=0)],
       {"f_type": 0, "f_cut": 380, "f_res": .3, "f_poles": 4, "f_key": .3, "lfo_rate": .09, "lfo_cut": 1.6, "lfo_shape": 1},
       {"eq_lc": 50, "rv_mix": .35, "rv_size": .85, "rv_decay": 5, "rv_damp": .6})

preset("Glimmer Veil", "Pads & Atmospheres", ["Pop", "Ambient", "Reggaeton"],
       "Shimmering pad: octave-up grain sparkle over soft vibraphone tones — bright without harshness.",
       "Choruses that need lift, romantic reggaeton beds.", "pad",
       [L(7, oct=1, grate=26, glen=.15, goct=.5, gfifth=.15, gjit=6, a=.6, d=2, s=.9, r=2, level=.75),
        L(4, mat=1, hard=.3, click=0, decay=4, beat=2, a=.05, d=4, s=0, r=1.5, level=.4),
        L(1, wave=1, a=.6, d=2, s=.8, r=2, level=.35)],
       {"f_type": 0, "f_cut": 6000, "lfo_rate": 5, "lfo_amp": .06},
       {"eq_lc": 200, "rv_mix": .45, "rv_size": .9, "rv_decay": 5.5, "rv_pre": 30, "width": 1.3})

preset("Heart Strings", "Guitars & Strings", ["R&B", "Pop", "Cinematic"],
       "Emotional string-like synth layer (not a recorded ensemble): legato saw section with bowed attack and vibrato.",
       "Synth-string pads under R&B ballads and pop. Pair with real strings later.", "strings",
       [L(1, wave=2, uni=7, det=11, spread=.9, a=.35, d=1, s=.9, r=.9),
        L(1, wave=3, pw=.3, uni=3, det=8, a=.35, d=1, s=.9, r=.9, level=.3)],
       {"f_type": 0, "f_cut": 2400, "f_res": .1, "f_env": .5, "fa": .4, "fd": 1, "fs": .7, "f_key": .5,
        "vib_rate": 5.1, "vib_depth": 12, "vib_delay": .4},
       {"eq_lc": 150, "eq_pk_f": 900, "eq_pk_g": 1.5, "eq_hs_f": 8000, "eq_hs_g": -3, "ch_mix": .25,
        "rv_mix": .32, "rv_size": .8, "rv_decay": 3.5})

# ======================================================== GUITAR-LIKE (physical model, not recordings)
preset("Moon Strum", "Guitars & Strings", ["R&B", "Reggaeton", "Pop"],
       "Dreamy plucked-string model (synthesized, not a recording): doubled strings, chorus and echoes.",
       "Arpeggios for R&B and reggaeton intros.", "guitar_arp",
       [L(3, bright=.55, damp=.2, decay=3, pick=.18, strings=2, sdet=4, a=.001, d=4, s=0, r=.5, keydecay=.2)],
       {"f_type": 0, "f_cut": 5500},
       {"eq_lc": 90, "eq_pk_f": 220, "eq_pk_g": 1.5, "ch_mix": .35, "ch_depth": 2, "dly_beats": .75, "dly_fb": .3, "dly_mix": .15,
        "rv_mix": .25, "rv_size": .65, "rv_decay": 3})

preset("Palm Night", "Guitars & Strings", ["Afrobeats", "Dancehall", "Reggaeton"],
       "Muted string-model pluck (synthesized): short, thumpy palm-mute character for rhythmic patterns.",
       "Afro and dancehall guitar-style rhythms.", "guitar_muted",
       [L(3, bright=.5, damp=.45, decay=.28, pick=.22, a=.001, d=.6, s=0, r=.06)],
       {"f_type": 0, "f_cut": 2400, "f_res": .1},
       {"eq_lc": 100, "eq_pk_f": 1200, "eq_pk_g": 2, "rv_mix": .1, "rv_size": .3, "ch_mix": .15})

preset("Koto Circuit", "Guitars & Strings", ["Experimental", "Trap", "Pop"],
       "Synthetic guitar hybrid: a twangy plucked string that blooms into an FM bell tail.",
       "Unusual melodic riffs, trap toplines with character.", "bell_trap",
       [L(3, bright=.75, damp=.12, decay=1.8, pick=.08, a=.001, d=2.5, s=0, r=.3),
        L(2, cr=2, mr=5, idx=1.2, idx_dec=.5, idx_sus=0, a=.03, d=1.5, s=0, r=.4, level=.3)],
       {"f_type": 0, "f_cut": 5000, "p_env": .3, "p_dec": .05},
       {"eq_lc": 150, "dly_beats": .75, "dly_fb": .3, "dly_mix": .14, "rv_mix": .2, "rv_size": .55})

preset("Nylon Mist", "Guitars & Strings", ["Bossa / MPB", "R&B", "Latin pop"],
       "Soft nylon-style plucked string model (synthesized) — warm, rounded, gentle.",
       "Sketching Latin and bossa-inspired figures before real guitar recordings exist.", "keys_bossa",
       [L(3, bright=.32, damp=.3, decay=2.4, pick=.28, strings=2, sdet=2, a=.001, d=3, s=0, r=.35, keydecay=.2)],
       {"f_type": 0, "f_cut": 3500},
       {"eq_lc": 80, "eq_pk_f": 180, "eq_pk_g": 2, "rv_mix": .2, "rv_size": .5, "rv_decay": 2})

# ======================================================== BASS
preset("Deep 808 Glide", "Bass", ["Trap", "Drill", "Brazilian funk"],
       "Long 808 with a punchy pitch drop, warm saturation for small speakers, and smooth glides between notes.",
       "Trap and drill 808 lines; overlap notes to slide.", "bass_808",
       [L(1, wave=0, a=.001, d=3, s=.6, r=.15),
        L(1, wave=1, a=.001, d=.15, s=0, r=.1, level=.25)],
       {"mono": 1, "glide": .07, "f_type": -1, "p_env": 9, "p_dec": .025},
       {"sat_drive": .45, "sat_mix": .7, "eq_lc": 25, "eq_hc": 9000}, rng=(24, 48))

preset("Dembow Thump", "Bass", ["Dembow", "Reggaeton", "Dancehall"],
       "Short, hard-hitting bass with a click on top — cuts through dense dembow percussion.",
       "Dembow and reggaeton bass hits.", "bass_dembow",
       [L(1, wave=0, a=.001, d=.5, s=.2, r=.08),
        L(1, wave=3, pw=.5, a=.001, d=.12, s=0, r=.06, level=.25)],
       {"mono": 1, "glide": .015, "f_type": 0, "f_cut": 900, "f_env": 2, "fd": .06, "fs": 0, "p_env": 12, "p_dec": .012},
       {"sat_drive": .35, "eq_lc": 28}, rng=(24, 50))

preset("Pure Sub", "Bass", ["All genres"],
       "Clean, round sub-bass with just a whisper of second harmonic so it reads on phones.",
       "Doubling 808s or kicks, any genre.", "bass_melodic",
       [L(1, wave=0, a=.006, d=1, s=1, r=.12),
        L(1, wave=0, oct=1, a=.006, d=1, s=1, r=.12, level=.08)],
       {"mono": 1, "glide": .02, "f_type": -1}, {"eq_lc": 22}, rng=(24, 52))

preset("Velvet Bass", "Bass", ["R&B", "Neo-soul", "Afroswing"],
       "Round melodic synth bass with a soft filter pluck — warm and articulate.",
       "Moving R&B bass lines.", "bass_melodic",
       [L(1, wave=2, a=.003, d=.8, s=.6, r=.15),
        L(1, wave=0, a=.003, d=1, s=.9, r=.15, level=.6, filt=0)],
       {"mono": 1, "glide": .03, "f_type": 0, "f_cut": 350, "f_res": .15, "f_env": 2, "fd": .25, "fs": .2, "f_key": .6},
       {"eq_lc": 28, "sat_drive": .1}, rng=(28, 60))

preset("Rubber Bounce", "Bass", ["House", "Afrobeats", "Jersey club"],
       "Plucky rubbery bass with a fast filter snap — bounces on offbeats.",
       "House and afro bass patterns.", "bass_house",
       [L(1, wave=2, uni=2, det=6, a=.001, d=.3, s=0, r=.08),
        L(1, wave=0, a=.001, d=.35, s=0, r=.08, level=.6, filt=0)],
       {"f_type": 0, "f_cut": 260, "f_res": .35, "f_env": 3.5, "fd": .09, "fs": 0},
       {"eq_lc": 30, "sat_drive": .15}, rng=(28, 60))

preset("Yard Bass", "Bass", ["Dancehall", "Reggae", "Afroswing"],
       "Deep, round dancehall bass — sine weight plus a soft triangle edge, slight bloom.",
       "Dancehall and reggae bass lines.", "bass_dancehall",
       [L(1, wave=0, a=.004, d=.8, s=.7, r=.12),
        L(1, wave=1, a=.004, d=.4, s=.3, r=.1, level=.35)],
       {"mono": 1, "glide": .02, "f_type": 0, "f_cut": 700, "f_key": .5},
       {"sat_drive": .2, "eq_lc": 28}, rng=(26, 55))

preset("Grit Reese", "Bass", ["Drill", "DnB", "Experimental"],
       "Distorted detuned reese: phasing saws pushed into grit, low-passed to stay heavy not harsh.",
       "Drill/UK basslines, dark experimental drops.", "bass_808",
       [L(1, wave=2, uni=3, det=16, spread=.3, a=.005, d=1, s=1, r=.15),
        L(1, wave=0, a=.005, d=1, s=1, r=.15, level=.5, filt=0)],
       {"mono": 1, "glide": .05, "f_type": 0, "f_cut": 650, "f_res": .2, "f_drive": 2.2, "lfo_rate": .3, "lfo_cut": .4},
       {"sat_drive": .5, "eq_lc": 28, "eq_hc": 7000}, rng=(24, 52))

# ======================================================== TEXTURES & FX
preset("Glitch Prism", "Textures & FX", ["Experimental", "Hyperpop", "Trap"],
       "Glitchy melodic bell: tempo-synced stutter repeats and bit-grit chop an FM melody into new rhythms.",
       "Ear-candy, transitions, glitch hooks.", "glitch",
       [L(2, cr=1, mr=3, idx=1.6, idx_dec=.2, idx_sus=.1, a=.001, d=.8, s=.1, r=.2)],
       {"f_type": -1},
       {"stut_prob": .4, "stut_beats": .5, "crush_bits": 8, "crush_sr": 16000, "eq_lc": 200, "eq_hc": 11000,
        "dly_beats": .375, "dly_fb": .3, "dly_mix": .15, "rv_mix": .15})

preset("Breath Cloud", "Textures & FX", ["Ambient", "R&B", "Cinematic"],
       "Breath-like pad: whispered formant noise that slowly changes vowel.",
       "Ambient layers, intimate R&B intros.", "texture",
       [L(6, vowel=1, vmod=2, tilt=1.6, breath=.5, a=1.2, d=2, s=.8, r=2, level=.5),
        L(5, ncut=1.5, ntrack=1, nq=2, nmode=1, vmod=1, a=1.2, d=2, s=.8, r=2, level=.6)],
       {"f_type": 2, "f_cut": 250, "lfo_rate": .2},
       {"rv_mix": .45, "rv_size": .9, "rv_decay": 5, "width": 1.4})

preset("Sparkle Rise", "Textures & FX", ["Pop", "EDM", "Latin pop"],
       "Sparkling transition texture: bright grains and airy noise that glitter upward.",
       "Risers, pre-chorus lifts, fairy-dust fills.", "rise",
       [L(7, oct=1, grate=30, glen=.08, goct=.6, gjit=4, a=.01, d=1.5, s=.4, r=1.2),
        L(5, ncut=6000, nq=.8, nmode=2, a=.3, d=1, s=.5, r=1.2, level=.25, filt=0),
        L(4, mat=5, hard=.6, click=.1, decay=1.2, a=.001, d=1.5, s=0, r=.5, level=.6)],
       {"f_type": -1},
       {"eq_lc": 300, "dly_beats": .25, "dly_fb": .45, "dly_mix": .2, "rv_mix": .4, "rv_size": .85, "rv_decay": 4})

preset("Reverse Tide", "Textures & FX", ["Pop", "R&B", "Cinematic"],
       "Reverse-style swell: tones that rise out of nothing and cut off, like a reversed tape — great before downbeats.",
       "Transitions, intros, reversed-pad feel.", "texture",
       [L(1, wave=2, uni=5, det=12, spread=1, a=2.2, d=1, s=1, r=.06),
        L(7, grate=18, glen=.2, goct=.3, a=2.2, d=1, s=1, r=.06, level=.5, filt=0)],
       {"f_type": 0, "f_cut": 1600, "f_env": 1.5, "fa": 2.2, "fd": 1, "fs": 1},
       {"eq_lc": 120, "rv_mix": .25, "rv_size": .7, "rv_decay": 2.5, "width": 1.3})

preset("Granular Dust", "Textures & FX", ["Experimental", "Lo-fi", "Ambient"],
       "Grainy, crumbling texture — FM grains through a bit-crusher, like a melody dissolving into dust.",
       "Experimental beds, lo-fi transitions.", "texture",
       [L(7, grate=35, glen=.06, goct=.3, gfifth=.3, gjit=30, gfm=1.2, a=.5, d=2, s=.8, r=1.5)],
       {"f_type": 0, "f_cut": 4000, "lfo_rate": .3, "lfo_cut": .7},
       {"crush_sr": 11000, "crush_bits": 7, "eq_lc": 150, "rv_mix": .35, "rv_size": .8, "rv_decay": 4})

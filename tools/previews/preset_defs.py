"""Source definitions for LUMI MELODY synthesis presets — round 2 (dark, record-ready).

Direction from the producer: darker, built for hit records. Reference *styles* (not songs): trapsoul and
underwater R&B, Bronx/NY drill, dark dancehall-pop, bounce house, hard trap and Latin trap, dark reggaeton.
Run build_previews.py to render and write one JSON per preset into /presets.
Layer types: 1 VA, 2 FM, 3 string model, 4 modal mallet/bell, 5 filtered noise, 6 formant voice, 7 grain cloud.
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

# common "record" finishes
TAPE = {"sat_drive": .3, "eq_hc": 7500, "eq_lc": 60}
UNDERWATER = {"eq_hc": 1500, "eq_lc": 70, "sat_drive": .2}

def fx(*parts, **kw):
    d = {}
    for p in parts: d.update(p)
    d.update(kw)
    return d

# ======================================================== KEYS (trapsoul / underwater R&B)
preset("Underwater Keys", "Keys", ["Trapsoul", "R&B", "Dark pop"],
       "Electric piano heard through a wall: heavily low-passed, tape-warm, with a long dark room and a soft duck on every beat.",
       "Late-night R&B and trapsoul chord loops under a vocal.", "trapsoul",
       [L(2, cr=1, mr=1, idx=1.4, idx_dec=.8, idx_sus=.2, fb=.05, fm_spread=6, a=.003, d=3.5, s=.1, r=.8, keydecay=.35),
        L(2, cr=1, mr=14, idx=1, idx_dec=.012, idx_sus=0, a=.001, d=.3, s=0, r=.1, level=.15),
        L(1, wave=0, a=.004, d=3, s=.1, r=.8, level=.25, filt=0)],
       {"f_type": 0, "f_cut": 1100, "f_res": .12, "f_key": .3, "vib_rate": .6, "vib_depth": 7, "vib_delay": 0},
       fx(UNDERWATER, pump=.18, ch_mix=.3, ch_depth=2, rv_mix=.32, rv_size=.8, rv_decay=4.5, rv_damp=.7, rv_pre=20, width=1.2))

preset("After Hours EP", "Keys", ["R&B", "Trapsoul", "Neo-soul"],
       "Dark Rhodes-style piano: round low-mids, bark only when you hit hard, slow tremolo and tape wobble.",
       "Moody R&B chords and intros.", "trapsoul",
       [L(2, cr=1, mr=1, idx=1.3, idx_dec=.9, idx_sus=.18, fb=.04, idx_vel=.9, fm_spread=5, a=.002, d=3.2, s=.05, r=.5, keydecay=.45),
        L(2, cr=1, mr=14, idx=1.2, idx_dec=.012, idx_sus=0, idx_vel=1, a=.001, d=.3, s=0, r=.1, level=.22),
        L(1, wave=0, a=.003, d=2.5, s=0, r=.3, level=.15, filt=0)],
       {"f_type": 0, "f_cut": 2400, "f_res": .05, "f_vel": 1.4, "lfo_rate": 3.2, "lfo_pan": .35, "vib_rate": .7, "vib_depth": 5, "vib_delay": 0},
       fx(TAPE, eq_ls_f=200, eq_ls_g=2, ch_mix=.25, ch_depth=1.5, rv_mix=.22, rv_size=.6, rv_decay=2.8, rv_damp=.6))

preset("Muffled Felt", "Keys", ["Trapsoul", "Drill", "Ambient"],
       "Soft felt-piano tone with the hammer muted — close, intimate and dark, like a piano in the next room.",
       "Sad intros, drill and trapsoul piano lines.", "trapsoul",
       [L(2, cr=1, mr=1, idx=.9, idx_dec=.4, idx_sus=.1, idx_vel=.8, a=.004, d=3.5, s=0, r=.7, keydecay=.5),
        L(2, cr=2, mr=1, idx=.4, idx_dec=.2, idx_sus=0, a=.004, d=1.8, s=0, r=.5, level=.35, keydecay=.5),
        L(5, ncut=2.5, ntrack=1, nq=1, nmode=1, a=.001, d=.05, s=0, r=.04, level=.08, filt=0)],
       {"f_type": 0, "f_cut": 1300, "f_res": .05, "f_env": 1, "fd": .3, "f_key": .5, "f_vel": 1},
       fx(TAPE, eq_hc=6000, eq_ls_f=180, eq_ls_g=2.5, rv_mix=.3, rv_size=.7, rv_decay=3.5, rv_damp=.7))

preset("Haunted Keys", "Keys", ["Trap", "Drill", "Dark pop"],
       "Seasick detuned keys — three drifting saws and an FM body that never quite agree. Eerie, wide, unsettling.",
       "Dark trap loops, horror-flavoured drill.", "trapsoul",
       [L(1, wave=2, uni=3, det=24, spread=.9, drift=12, a=.004, d=2.4, s=.25, r=.7, vel=.6),
        L(2, cr=1, mr=1, idx=.9, idx_dec=.5, idx_sus=.1, a=.002, d=2, s=.1, r=.6, level=.6)],
       {"f_type": 0, "f_cut": 800, "f_res": .2, "f_env": 2, "fd": .5, "fs": .15, "f_key": .6, "vib_rate": .3, "vib_depth": 10, "vib_delay": 0},
       fx(TAPE, eq_pk_f=350, eq_pk_g=-2.5, ch_mix=.35, ch_depth=2.5, ch_rate=.2, rv_mix=.35, rv_size=.8, rv_decay=4.5, rv_damp=.6, width=1.25))

preset("VHS Keys", "Keys", ["Trapsoul", "Lo-fi", "Trap"],
       "Digital keys sampled off a worn tape: wow and flutter, crushed top end and a washed-out echo.",
       "Sample-style loops for trap and trapsoul.", "trapsoul",
       [L(2, cr=1, mr=1, idx=1.1, idx_dec=.7, idx_sus=.2, fb=.06, a=.002, d=2.5, s=.05, r=.5, keydecay=.4),
        L(2, cr=2, mr=1, idx=.5, idx_dec=.3, idx_sus=.05, a=.002, d=1.2, s=0, r=.3, level=.3)],
       {"f_type": -1, "vib_rate": .9, "vib_depth": 13, "vib_delay": 0},
       fx(sat_drive=.35, crush_sr=18000, crush_bits=10, eq_lc=110, eq_hc=4800, eq_ls_f=250, eq_ls_g=2,
          dly_beats=.75, dly_fb=.35, dly_mix=.16, dly_damp=.7, rv_mix=.18, rv_size=.5, rv_decay=2))

preset("Velvet Stabs", "Keys", ["Bounce house", "R&B", "Dance"],
       "Warm detuned chord stabs through a squelchy filter, swung and pumping — the funky side of dark.",
       "Bouncy house and dance-R&B chord grooves.", "bounce_house",
       [L(1, wave=2, uni=4, det=14, spread=.8, drift=5, a=.002, d=.5, s=.15, r=.15),
        L(1, wave=3, pw=.35, a=.002, d=.4, s=.1, r=.12, level=.4)],
       {"f_type": 0, "f_cut": 500, "f_res": .35, "f_env": 3.2, "fa": .001, "fd": .17, "fs": .1, "f_key": .4, "f_vel": 1.2},
       fx(sat_drive=.2, eq_lc=140, pump=.4, ch_mix=.3, ch_depth=2, dly_beats=.75, dly_fb=.25, dly_mix=.12, rv_mix=.14, rv_size=.45))

preset("Smoke Organ", "Keys", ["Dark pop", "Dancehall-pop", "Gospel trap"],
       "Dark drawbar-style organ with a slow rotating shimmer, low-passed and driven — church in a basement.",
       "Dark pop choruses, gospel-tinged trap.", "pad_dark",
       [L(1, wave=0, a=.006, d=1, s=.9, r=.2, level=.7),
        L(1, wave=0, oct=1, a=.006, d=1, s=.8, r=.2, level=.4),
        L(1, wave=3, pw=.5, oct=-1, a=.006, d=1, s=.7, r=.2, level=.25)],
       {"f_type": 0, "f_cut": 1800, "lfo_rate": 5.6, "lfo_amp": .1, "lfo_pan": .3},
       fx(sat_drive=.35, eq_lc=60, ch_mix=.45, ch_depth=3, ch_rate=.9, rv_mix=.25, rv_size=.7, rv_decay=3))

# ======================================================== BELLS & PLUCKS
preset("Drill Bell", "Bells & Plucks", ["NY drill", "UK drill", "Trap"],
       "Hard, dark bell built for drill: metallic FM strike, a sub-octave shadow and a short black room.",
       "Staccato drill melodies over sliding 808s.", "drill_bell",
       [L(2, cr=1, mr=3.5, idx=2.4, idx_dec=.18, idx_sus=.05, idx_vel=.7, fm_spread=4, a=.001, d=1.6, s=0, r=.35, keydecay=.25),
        L(4, mat=3, hard=.5, click=.05, decay=1.6, beat=1, a=.001, d=2, s=0, r=.4, level=.3),
        L(1, wave=0, oct=-1, a=.001, d=.6, s=0, r=.2, level=.25, filt=0)],
       {"f_type": 0, "f_cut": 4200, "f_key": .3},
       fx(sat_drive=.15, eq_lc=150, eq_pk_f=2800, eq_pk_g=-2, rv_mix=.22, rv_size=.5, rv_decay=1.8, rv_damp=.6,
          dly_beats=.75, dly_fb=.2, dly_mix=.08))

preset("Grave Bell", "Bells & Plucks", ["Trap", "Dark pop", "Cinematic"],
       "Haunting, slightly detuned bell with a long low halo — the kind of bell that opens a dark record.",
       "Intros, dark trap hooks, cinematic hits.", "trap_lead",
       [L(2, cr=1, mr=3.5, idx=2, idx_dec=.35, idx_sus=.05, idx_vel=.7, fm_spread=7, a=.001, d=3, s=0, r=1, keydecay=.3),
        L(4, mat=3, hard=.4, click=0, decay=4, beat=2, a=.001, d=4, s=0, r=1, level=.4)],
       {"f_type": 0, "f_cut": 3200, "f_key": .3, "vib_rate": .25, "vib_depth": 6, "vib_delay": 0},
       fx(TAPE, eq_lc=120, rv_mix=.4, rv_size=.9, rv_decay=6, rv_damp=.6, rv_pre=30, dly_beats=1.5, dly_fb=.3, dly_mix=.12, width=1.2))

preset("Night Glass", "Bells & Plucks", ["Dark reggaeton", "Latin trap", "R&B"],
       "Dark glass pluck: inharmonic glass partials rolled off at the top, trailing into tempo echoes.",
       "Romantic-dark reggaeton and Latin trap toplines.", "reggaeton_dark",
       [L(4, mat=2, hard=.5, click=.08, decay=1.2, beat=1.5, a=.001, d=2, s=0, r=.3, level=.8, keydecay=.2),
        L(2, cr=1, mr=4, idx=.8, idx_dec=.06, idx_sus=0, a=.001, d=.8, s=0, r=.25, level=.45),
        L(1, wave=1, oct=-1, a=.002, d=.5, s=0, r=.2, level=.2)],
       {"f_type": 0, "f_cut": 2600, "f_key": .4},
       fx(eq_lc=160, sat_drive=.12, dly_beats=.75, dly_fb=.38, dly_mix=.2, dly_damp=.6, rv_mix=.25, rv_size=.7, rv_decay=3))

preset("Phonk Cowbell", "Bells & Plucks", ["Phonk", "Brazilian phonk", "Memphis"],
       "Tuned 808-style cowbell melody: two clashing square tones through a band-pass, driven hard.",
       "Phonk and drift-phonk melodies; also great as a rhythmic hit.", "phonk",
       [L(1, wave=3, pw=.5, a=.001, d=.35, s=0, r=.08),
        L(1, wave=3, pw=.5, semi=6.79, a=.001, d=.3, s=0, r=.08, level=.85)],
       {"f_type": 1, "f_cut": 1300, "f_res": .35, "f_key": 1},
       fx(sat_drive=.55, sat_mix=.8, eq_lc=300, eq_hc=9000, rv_mix=.12, rv_size=.35))

preset("Obsidian Pluck", "Bells & Plucks", ["Drill", "Trap", "Dark pop"],
       "Dark, muted string pluck with a sub shadow underneath. Moody and close.",
       "Drill melodies, sinister counter-lines.", "drill_bell",
       [L(3, bright=.32, damp=.42, decay=1.4, pick=.25, strings=2, sdet=6, a=.001, d=3, s=0, r=.25),
        L(1, wave=0, oct=-1, a=.002, d=.6, s=0, r=.2, level=.3, filt=0)],
       {"f_type": 0, "f_cut": 1500, "f_res": .1, "f_key": .5},
       fx(TAPE, ch_mix=.2, rv_mix=.25, rv_size=.6, rv_decay=2.5, rv_damp=.65))

preset("Shadow Zip", "Bells & Plucks", ["Dancehall-pop", "Dark pop", "Afroswing"],
       "Dark digital pluck with a quick filter zip and a ping-pong echo — rhythmic and moody, not sunny.",
       "Dark dancehall-pop riffs.", "dancehall_dark",
       [L(1, wave=2, uni=3, det=12, spread=.7, a=.001, d=.35, s=0, r=.12),
        L(2, cr=1, mr=2, idx=1.2, idx_dec=.05, idx_sus=0, a=.001, d=.3, s=0, r=.1, level=.5)],
       {"f_type": 0, "f_cut": 380, "f_res": .4, "f_env": 3.6, "fa": .001, "fd": .1, "fs": 0, "f_key": .7},
       fx(sat_drive=.2, eq_lc=150, dly_beats=.75, dly_fb=.32, dly_mix=.18, dly_damp=.6, rv_mix=.16, rv_size=.5, width=1.2))

preset("Perreo Pluck", "Bells & Plucks", ["Dark reggaeton", "Latin trap", "Dembow"],
       "Dark, hollow square pluck with a detuned twin — the moody reggaeton pluck that sits right in the pocket.",
       "Reggaeton riffs and dembow hooks after dark.", "reggaeton_dark",
       [L(1, wave=3, pw=.42, uni=2, det=10, spread=.6, a=.001, d=.4, s=.05, r=.15),
        L(1, wave=0, oct=-1, a=.001, d=.35, s=0, r=.12, level=.35, filt=0)],
       {"f_type": 0, "f_cut": 700, "f_res": .25, "f_env": 2.6, "fa": .001, "fd": .14, "fs": .05, "f_key": .6},
       fx(sat_drive=.18, eq_lc=130, dly_beats=.75, dly_fb=.3, dly_mix=.16, dly_damp=.55, rv_mix=.18, rv_size=.55))

preset("Dembow Blade", "Bells & Plucks", ["Dembow", "Dark reggaeton", "Jersey club"],
       "Punchy, cutting minor pluck with a hard transient — slices through dense dembow percussion.",
       "Dembow motifs, energetic hooks.", "dembow_dark",
       [L(2, cr=1, mr=1, idx=2.2, idx_dec=.04, idx_sus=.1, fb=.3, a=.001, d=.3, s=0, r=.1),
        L(1, wave=2, uni=3, det=10, spread=.8, a=.001, d=.22, s=0, r=.08, level=.55)],
       {"f_type": 0, "f_cut": 2400, "f_res": .2, "f_env": 1.5, "fd": .08, "fs": 0, "p_env": 2, "p_dec": .01},
       fx(sat_drive=.3, eq_lc=180, eq_pk_f=450, eq_pk_g=-2, dly_beats=.5, dly_fb=.2, dly_mix=.1, rv_mix=.1, rv_size=.35))

preset("Shadow Mallet", "Bells & Plucks", ["Trapsoul", "R&B", "Cinematic"],
       "Low vibraphone-style mallet, dark and slow-shimmering, drowned in a deep room.",
       "Dark R&B colour, cinematic transitions.", "trapsoul",
       [L(4, mat=1, hard=.3, click=.04, decay=3, beat=1.5, a=.001, d=4, s=0, r=.8),
        L(1, wave=0, a=.004, d=2, s=0, r=.8, level=.25)],
       {"f_type": 0, "f_cut": 2000, "lfo_rate": 3.2, "lfo_amp": .2},
       fx(TAPE, eq_hc=5500, rv_mix=.38, rv_size=.85, rv_decay=5, rv_damp=.65, rv_pre=20))

preset("Rain Drip", "Bells & Plucks", ["Trap", "Plugg", "Ambient"],
       "Dark droplet pluck: resonant band-pass blip with a falling pitch, echoing into a black room.",
       "Plugg melodies, trap ear-candy.", "glitch",
       [L(1, wave=2, a=.001, d=.3, s=0, r=.15),
        L(1, wave=0, a=.001, d=.4, s=0, r=.15, level=.5, filt=0)],
       {"f_type": 1, "f_cut": 2000, "f_res": .7, "f_env": -1.8, "fa": .001, "fd": .12, "fs": 0, "f_key": .9, "p_env": 5, "p_dec": .02},
       fx(eq_lc=150, eq_hc=6500, dly_beats=.375, dly_fb=.42, dly_mix=.2, dly_damp=.5, rv_mix=.28, rv_size=.75, rv_decay=3.5))

preset("Blade Koto", "Guitars & Strings", ["Trap", "Drill", "Dark pop"],
       "Twangy plucked-string model with a cold FM ring — sinister and Eastern-tinged (synthesized, not recorded).",
       "Dark trap riffs with character.", "drill_bell",
       [L(3, bright=.6, damp=.14, decay=1.6, pick=.08, a=.001, d=2.5, s=0, r=.3),
        L(2, cr=2, mr=5, idx=.9, idx_dec=.5, idx_sus=0, a=.03, d=1.5, s=0, r=.4, level=.25)],
       {"f_type": 0, "f_cut": 3200, "p_env": .3, "p_dec": .05},
       fx(TAPE, eq_lc=150, dly_beats=.75, dly_fb=.3, dly_mix=.14, rv_mix=.24, rv_size=.6, rv_decay=2.5))

preset("Midnight Strum", "Guitars & Strings", ["Trapsoul", "R&B", "Dark reggaeton"],
       "Moody doubled plucked-string model (synthesized): dark, chorused, trailing echoes. Placeholder until real guitar is recorded.",
       "Guitar-style arpeggios for R&B and reggaeton intros.", "guitar_dark",
       [L(3, bright=.42, damp=.25, decay=3, pick=.2, strings=2, sdet=5, a=.001, d=4, s=0, r=.5, keydecay=.2)],
       {"f_type": 0, "f_cut": 3200},
       fx(TAPE, eq_pk_f=220, eq_pk_g=1.5, ch_mix=.35, ch_depth=2, dly_beats=.75, dly_fb=.32, dly_mix=.16, rv_mix=.3, rv_size=.75, rv_decay=3.5))

# ======================================================== LEADS
preset("Ghost Flute", "Synths & Leads", ["Trap", "Drill", "Latin trap"],
       "Breathy, haunting flute-style lead with air in the attack and a slow vibrato — the dark trap flute sound.",
       "Trap and drill toplines, eerie hooks.", "trap_lead",
       [L(1, wave=0, a=.05, d=.6, s=.85, r=.35),
        L(1, wave=1, a=.05, d=.6, s=.85, r=.35, level=.25),
        L(5, ncut=1, ntrack=1, nq=3, nmode=1, a=.02, d=.18, s=.35, r=.3, level=.6)],
       {"mono": 1, "glide": .05, "f_type": 0, "f_cut": 2800, "f_key": .4, "vib_rate": 5, "vib_depth": 22, "vib_delay": .25},
       fx(TAPE, eq_lc=250, dly_beats=.75, dly_fb=.3, dly_mix=.14, dly_damp=.6, rv_mix=.3, rv_size=.75, rv_decay=3.5))

preset("Rage Lead", "Synths & Leads", ["Rage", "Trap", "Hyperpop"],
       "Distorted seven-voice supersaw, dark and wide, with glide — huge, aggressive, still musical.",
       "Rage beats, hard trap hooks, drops.", "trap_lead",
       [L(1, wave=2, uni=7, det=22, spread=1, a=.004, d=1, s=.9, r=.25),
        L(1, wave=2, oct=-1, uni=3, det=10, spread=.4, a=.004, d=1, s=.9, r=.25, level=.4)],
       {"mono": 1, "glide": .06, "f_type": 0, "f_cut": 2600, "f_res": .15, "f_drive": 2.5},
       fx(sat_drive=.6, sat_mix=.85, eq_lc=140, eq_hc=8000, eq_pk_f=500, eq_pk_g=-2.5, rv_mix=.2, rv_size=.7, rv_decay=3, width=1.3))

preset("Silk Glide", "Synths & Leads", ["R&B", "Trapsoul", "Afroswing"],
       "Smooth, dark monophonic lead with natural portamento and delayed vibrato — sings like a vocal ad-lib.",
       "Melodic hooks, vocal-style counter-melodies.", "trap_lead",
       [L(1, wave=2, uni=3, det=7, spread=.4, a=.01, d=1, s=.8, r=.25),
        L(1, wave=3, pw=.5, oct=-1, a=.01, d=1, s=.8, r=.25, level=.3)],
       {"mono": 1, "glide": .07, "f_type": 0, "f_cut": 1500, "f_poles": 4, "f_res": .2, "f_env": 1, "fd": .4, "fs": .3, "f_key": .5,
        "vib_rate": 5.3, "vib_depth": 18, "vib_delay": .35},
       fx(TAPE, eq_lc=120, dly_beats=.75, dly_fb=.3, dly_mix=.15, rv_mix=.25, rv_size=.7, rv_decay=3))

preset("Perreo Lead", "Synths & Leads", ["Dark reggaeton", "Latin trap", "Dembow"],
       "Dark, hollow detuned square lead with slides — the moody reggaeton lead voice.",
       "Reggaeton hooks, Latin trap melodies.", "reggaeton_lead",
       [L(1, wave=3, pw=.45, uni=2, det=9, spread=.5, a=.005, d=.8, s=.75, r=.2),
        L(1, wave=0, oct=-1, a=.005, d=.8, s=.75, r=.2, level=.3, filt=0)],
       {"mono": 1, "glide": .05, "f_type": 0, "f_cut": 1300, "f_res": .25, "f_env": 1.2, "fd": .25, "fs": .4, "f_key": .5,
        "vib_rate": 5.4, "vib_depth": 12, "vib_delay": .4},
       fx(sat_drive=.25, eq_lc=150, dly_beats=.75, dly_fb=.3, dly_mix=.15, rv_mix=.2, rv_size=.6, rv_decay=2.5))

preset("Siren Vox", "Synths & Leads", ["Dark pop", "R&B", "Cinematic"],
       "Dark vocal-like synth that moves between 'oh' and 'oo' — wordless, human-feeling, slightly eerie.",
       "Wordless hooks, ghostly vocal layers.", "trap_lead",
       [L(6, vowel=3, vmod=.8, tilt=1.4, breath=.08, a=.06, d=1, s=.9, r=.5),
        L(6, vowel=3.5, vmod=.6, tilt=1.4, fine=-12, oct=-1, a=.08, d=1, s=.9, r=.5, level=.4)],
       {"mono": 1, "glide": .08, "f_type": 0, "f_cut": 2500, "lfo_rate": .2, "vib_rate": 5, "vib_depth": 20, "vib_delay": .3},
       fx(eq_lc=150, ch_mix=.35, ch_depth=2.5, rv_mix=.38, rv_size=.85, rv_decay=4.5, rv_pre=30, width=1.2))

preset("Night Whistle", "Synths & Leads", ["Trap", "R&B", "West coast"],
       "Low, eerie whistle lead: near-pure tone with tuned breath, slow vibrato and a deep hall.",
       "Haunting trap melodies, dark G-funk-style lines.", "trap_lead",
       [L(1, wave=0, a=.04, d=1, s=.9, r=.4),
        L(5, ncut=1, ntrack=1, nq=6, nmode=1, a=.04, d=1, s=.9, r=.4, level=.2)],
       {"mono": 1, "glide": .1, "f_type": -1, "vib_rate": 4.8, "vib_depth": 18, "vib_delay": .25},
       fx(eq_lc=200, eq_hc=6000, dly_beats=.75, dly_fb=.35, dly_mix=.14, rv_mix=.4, rv_size=.85, rv_decay=5, rv_pre=35))

preset("Torch Lead", "Synths & Leads", ["Drill", "Dark pop", "EDM"],
       "Distorted melodic lead: five-voice saw stack pushed into warm saturation, tamed by a low-pass.",
       "Aggressive hooks, dark-pop choruses.", "reggaeton_lead",
       [L(1, wave=2, uni=5, det=14, spread=.7, a=.005, d=1, s=.85, r=.2)],
       {"mono": 1, "glide": .04, "f_type": 0, "f_cut": 1800, "f_res": .2, "f_env": .8, "fd": .3, "fs": .5, "f_drive": 2.5,
        "vib_rate": 5.6, "vib_depth": 10, "vib_delay": .5},
       fx(sat_drive=.55, sat_mix=.8, eq_lc=150, eq_hc=7000, eq_pk_f=600, eq_pk_g=-2, rv_mix=.18, rv_size=.6, dly_beats=.75, dly_fb=.25, dly_mix=.1))

preset("Bounce Squelch", "Synths & Leads", ["Bounce house", "Funk", "Jersey club"],
       "Rubbery resonant lead with an envelope squelch on every note — funky, swung and a little dirty.",
       "Bouncy house toplines, funky counter-lines.", "bounce_lead",
       [L(1, wave=3, pw=.3, a=.002, d=.4, s=.3, r=.1),
        L(1, wave=2, a=.002, d=.4, s=.3, r=.1, level=.5)],
       {"f_type": 0, "f_cut": 420, "f_res": .6, "f_env": 3.4, "fa": .001, "fd": .14, "fs": .15, "f_key": .6, "f_vel": 1.4},
       fx(sat_drive=.25, eq_lc=150, ch_mix=.2, dly_beats=.75, dly_fb=.22, dly_mix=.1, rv_mix=.12, rv_size=.4))

# ======================================================== PADS, CHOIR, STRINGS, BRASS
preset("Underwater Pad", "Pads & Atmospheres", ["Trapsoul", "R&B", "Dark pop"],
       "Warm saw pad sunk below the surface — heavy low-pass, slow swell and a breathing duck.",
       "The bed under trapsoul and dark R&B records.", "pad_dark",
       [L(1, wave=2, uni=6, det=14, spread=.9, drift=5, a=.8, d=2, s=.9, r=2)],
       {"f_type": 0, "f_cut": 700, "f_res": .12, "f_key": .3, "lfo_rate": .12, "lfo_cut": .4},
       fx(UNDERWATER, pump=.25, ch_mix=.35, ch_depth=2.5, rv_mix=.35, rv_size=.85, rv_decay=5, width=1.3))

preset("Dark Choir", "Pads & Atmospheres", ["Trap", "Drill", "Cinematic"],
       "Synthesized dark choir: two formant voice layers singing 'oh–oo', detuned and spread in a cathedral reverb.",
       "Drill and trap intros, epic dark choruses.", "cinematic",
       [L(6, vowel=3, vmod=.5, tilt=1.4, breath=.06, fine=-9, pan=-.5, a=.6, d=2, s=.9, r=1.6),
        L(6, vowel=3.4, vmod=.5, tilt=1.4, breath=.06, fine=9, pan=.5, a=.7, d=2, s=.9, r=1.6),
        L(1, wave=2, uni=5, det=12, spread=.8, a=.8, d=2, s=.8, r=1.6, level=.18)],
       {"f_type": 0, "f_cut": 2200, "lfo_rate": .15, "vib_rate": 4.8, "vib_depth": 9, "vib_delay": .5},
       fx(eq_lc=110, ch_mix=.4, ch_depth=3, rv_mix=.45, rv_size=.95, rv_decay=6, rv_pre=35, width=1.3))

preset("Midnight Strings", "Pads & Atmospheres", ["Cinematic", "Drill", "Dark pop"],
       "Dark cinematic string-like section (synthesized), weighted low with a bowed swell and vibrato.",
       "Drill strings, dramatic intros. Pair with real strings later.", "cinematic",
       [L(1, wave=2, uni=7, det=12, spread=.9, a=.4, d=1, s=.9, r=1),
        L(1, wave=3, pw=.3, uni=3, det=8, oct=-1, a=.4, d=1, s=.9, r=1, level=.3)],
       {"f_type": 0, "f_cut": 1600, "f_res": .1, "f_env": .6, "fa": .5, "fd": 1, "fs": .7, "f_key": .5,
        "vib_rate": 5, "vib_depth": 12, "vib_delay": .4},
       fx(eq_lc=80, eq_pk_f=900, eq_pk_g=1, eq_hs_f=7000, eq_hs_g=-4, ch_mix=.25, rv_mix=.38, rv_size=.9, rv_decay=4.5))

preset("Dark Horns", "Pads & Atmospheres", ["Trap", "Drill", "Cinematic"],
       "Synth brass stabs with a growling filter bloom — big, dark and dramatic (synthesized, not recorded horns).",
       "Hard trap and drill stabs, cinematic hits.", "brass",
       [L(1, wave=2, uni=4, det=10, spread=.6, a=.03, d=.8, s=.7, r=.3),
        L(1, wave=2, oct=-1, a=.03, d=.8, s=.7, r=.3, level=.4)],
       {"f_type": 0, "f_cut": 450, "f_res": .15, "f_env": 2.4, "fa": .06, "fd": .5, "fs": .4, "f_key": .5, "f_vel": 1.2},
       fx(sat_drive=.4, eq_lc=70, rv_mix=.28, rv_size=.8, rv_decay=3.5, width=1.15))

preset("Abyss Bloom", "Pads & Atmospheres", ["Dark pop", "Trap", "Cinematic"],
       "Dark evolving pad — a low saw mass whose filter slowly blooms open and closes again.",
       "Dark intros, tension builds, trap atmospheres.", "pad_dark",
       [L(1, wave=2, uni=6, det=16, spread=.9, drift=6, a=1.2, d=2, s=.9, r=2),
        L(1, wave=0, oct=-1, a=1, d=2, s=.9, r=2, level=.25, filt=0)],
       {"f_type": 0, "f_cut": 380, "f_res": .3, "f_poles": 4, "f_key": .3, "lfo_rate": .09, "lfo_cut": 1.6, "lfo_shape": 1},
       fx(eq_lc=50, rv_mix=.38, rv_size=.9, rv_decay=5.5, rv_damp=.6))

preset("Cold Air", "Pads & Atmospheres", ["Trapsoul", "Ambient", "R&B"],
       "Dark airy layer: tuned breath and low grains drifting above the mix — cold night air.",
       "Top-layer atmosphere for R&B and trap.", "pad_dark",
       [L(5, ncut=2, ntrack=1, nq=3, nmode=1, a=.8, d=2, s=.8, r=2, level=.7),
        L(7, grate=10, glen=.3, goct=.15, gjit=10, a=1, d=2, s=.8, r=2, level=.4)],
       {"f_type": 2, "f_cut": 250},
       fx(eq_hc=6000, rv_mix=.5, rv_size=.95, rv_decay=6, width=1.4))

preset("Nebula Drift", "Pads & Atmospheres", ["Ambient", "Trap", "R&B"],
       "Dark ambient cloud of soft grains over a slow FM pad that breathes in and out.",
       "Interludes, spacious trap and R&B.", "pad_dark",
       [L(7, grate=22, glen=.3, goct=.1, gfifth=.2, gjit=12, a=1.2, d=2, s=.9, r=2.5, level=.8),
        L(2, cr=1, mr=2, idx=.4, idx_dec=3, idx_sus=.3, fm_spread=8, a=1, d=2, s=.8, r=2.5, level=.6)],
       {"f_type": 0, "f_cut": 1400, "lfo_rate": .12, "lfo_cut": .6},
       fx(eq_lc=110, dly_beats=1.5, dly_fb=.4, dly_mix=.15, dly_damp=.7, rv_mix=.45, rv_size=.9, rv_decay=6))

preset("Reverse Tide", "Textures & FX", ["Trapsoul", "Dark pop", "Cinematic"],
       "Reverse-style swell: dark tones that rise out of nothing and cut off — perfect before a drop.",
       "Transitions, intros, reversed-pad feel.", "texture",
       [L(1, wave=2, uni=5, det=12, spread=1, a=2.2, d=1, s=1, r=.06),
        L(7, grate=18, glen=.2, goct=.2, a=2.2, d=1, s=1, r=.06, level=.5, filt=0)],
       {"f_type": 0, "f_cut": 1100, "f_env": 1.3, "fa": 2.2, "fd": 1, "fs": 1},
       fx(eq_lc=110, rv_mix=.3, rv_size=.8, rv_decay=3, width=1.3))

# ======================================================== BASS
preset("808 Boom", "Bass", ["Trap", "Trapsoul", "Latin trap"],
       "Long, clean 808 with a firm knock and warm harmonics so it still reads on phone speakers. Overlap notes to slide.",
       "Trap and R&B 808 lines.", "bass_trap",
       [L(1, wave=0, a=.001, d=4, s=.7, r=.2),
        L(1, wave=1, a=.001, d=.12, s=0, r=.1, level=.2)],
       {"mono": 1, "glide": .08, "f_type": -1, "p_env": 8, "p_dec": .025},
       fx(sat_drive=.4, sat_mix=.6, eq_lc=24, eq_hc=8000), rng=(24, 48))

preset("808 Grit", "Bass", ["Drill", "Rage", "Brazilian phonk"],
       "Distorted 808: hard saturation folds harmonics on top of the sub. Aggressive and loud.",
       "Drill, rage and phonk low end.", "bass_808",
       [L(1, wave=0, a=.001, d=3, s=.65, r=.15),
        L(1, wave=1, a=.001, d=.2, s=.1, r=.1, level=.3)],
       {"mono": 1, "glide": .06, "f_type": 0, "f_cut": 2500, "f_drive": 3, "p_env": 10, "p_dec": .022},
       fx(sat_drive=.85, sat_mix=.75, eq_lc=25, eq_hc=6500, eq_pk_f=900, eq_pk_g=-3), rng=(24, 48))

preset("Drill Slide 808", "Bass", ["NY drill", "UK drill", "Trap"],
       "Punchy, short-tailed 808 tuned for drill: fast pitch knock and long, smooth slides between notes.",
       "Sliding drill bass lines.", "bass_808",
       [L(1, wave=0, a=.001, d=1.4, s=.45, r=.12),
        L(1, wave=1, a=.001, d=.08, s=0, r=.08, level=.25)],
       {"mono": 1, "glide": .12, "f_type": -1, "p_env": 12, "p_dec": .018},
       fx(sat_drive=.5, sat_mix=.7, eq_lc=25), rng=(24, 48))

preset("Dembow Thump", "Bass", ["Dembow", "Reggaeton", "Dancehall"],
       "Short, hard-hitting bass with a click on top — cuts through dense dembow percussion.",
       "Dembow and reggaeton bass hits.", "bass_dembow",
       [L(1, wave=0, a=.001, d=.5, s=.2, r=.08),
        L(1, wave=3, pw=.5, a=.001, d=.12, s=0, r=.06, level=.22)],
       {"mono": 1, "glide": .015, "f_type": 0, "f_cut": 800, "f_env": 2, "fd": .06, "fs": 0, "p_env": 12, "p_dec": .012},
       fx(sat_drive=.4, eq_lc=28), rng=(24, 50))

preset("Velvet Bass", "Bass", ["Trapsoul", "R&B", "Neo-soul"],
       "Round, dark melodic synth bass with a soft filter pluck — warm and articulate.",
       "Moving R&B bass lines.", "bass_rnb",
       [L(1, wave=2, a=.004, d=.8, s=.6, r=.15),
        L(1, wave=0, a=.004, d=1, s=.9, r=.15, level=.6, filt=0)],
       {"mono": 1, "glide": .04, "f_type": 0, "f_cut": 300, "f_res": .15, "f_env": 1.8, "fd": .25, "fs": .2, "f_key": .6},
       fx(eq_lc=26, sat_drive=.15), rng=(24, 60))

preset("Squelch Bass", "Bass", ["Bounce house", "Funk", "Jersey club"],
       "Resonant, rubbery bass that squelches on every note — swung, funky and dark.",
       "Bounce house and funk bass grooves.", "bass_bounce",
       [L(1, wave=2, uni=2, det=6, a=.001, d=.3, s=.1, r=.08),
        L(1, wave=0, a=.001, d=.35, s=.2, r=.08, level=.6, filt=0)],
       {"f_type": 0, "f_cut": 240, "f_res": .55, "f_env": 3.2, "fd": .1, "fs": .05, "f_vel": 1.2},
       fx(eq_lc=30, sat_drive=.25), rng=(28, 60))

preset("Yard Bass", "Bass", ["Dancehall", "Dancehall-pop", "Afroswing"],
       "Deep, round dancehall bass — sine weight plus a soft triangle edge, slight bloom.",
       "Dancehall and dark dancehall-pop bass lines.", "bass_dancehall",
       [L(1, wave=0, a=.004, d=.8, s=.7, r=.12),
        L(1, wave=1, a=.004, d=.4, s=.3, r=.1, level=.35)],
       {"mono": 1, "glide": .02, "f_type": 0, "f_cut": 650, "f_key": .5},
       fx(sat_drive=.25, eq_lc=28), rng=(26, 55))

preset("Grit Reese", "Bass", ["Drill", "UK", "Experimental"],
       "Distorted detuned reese: phasing saws pushed into grit, low-passed to stay heavy not harsh.",
       "UK/drill basslines, dark drops.", "bass_808",
       [L(1, wave=2, uni=3, det=16, spread=.3, a=.005, d=1, s=1, r=.15),
        L(1, wave=0, a=.005, d=1, s=1, r=.15, level=.5, filt=0)],
       {"mono": 1, "glide": .05, "f_type": 0, "f_cut": 600, "f_res": .2, "f_drive": 2.2, "lfo_rate": .3, "lfo_cut": .4},
       fx(sat_drive=.5, eq_lc=28, eq_hc=6500), rng=(24, 52))

preset("Pure Sub", "Bass", ["All genres"],
       "Clean, round sub-bass with a whisper of second harmonic so it reads on phones.",
       "Doubling 808s or kicks, any genre.", "bass_rnb",
       [L(1, wave=0, a=.008, d=1, s=1, r=.12),
        L(1, wave=0, oct=1, a=.008, d=1, s=1, r=.12, level=.08)],
       {"mono": 1, "glide": .02, "f_type": -1}, fx(eq_lc=22), rng=(24, 52))

# ======================================================== TEXTURES & FX
preset("Glitch Prism", "Textures & FX", ["Experimental", "Rage", "Trap"],
       "Glitchy dark bell: tempo-synced stutters and bit-grit chop a minor melody into new rhythms.",
       "Ear-candy, transitions, glitch hooks.", "glitch",
       [L(2, cr=1, mr=3, idx=1.6, idx_dec=.2, idx_sus=.1, a=.001, d=.8, s=.1, r=.2)],
       {"f_type": 0, "f_cut": 3500},
       fx(stut_prob=.4, stut_beats=.5, crush_bits=8, crush_sr=16000, eq_lc=200, eq_hc=9000,
          dly_beats=.375, dly_fb=.3, dly_mix=.15, rv_mix=.2, rv_size=.6))

preset("Breath Cloud", "Textures & FX", ["Trapsoul", "Ambient", "Cinematic"],
       "Whispered formant breath that slowly changes vowel — ghostly and intimate.",
       "Ambient layers, intimate R&B intros.", "texture",
       [L(6, vowel=3, vmod=1.5, tilt=1.8, breath=.5, a=1.2, d=2, s=.8, r=2, level=.5),
        L(5, ncut=1.5, ntrack=1, nq=2, nmode=1, vmod=1, a=1.2, d=2, s=.8, r=2, level=.55)],
       {"f_type": 0, "f_cut": 3000, "lfo_rate": .2},
       fx(eq_lc=200, rv_mix=.5, rv_size=.95, rv_decay=6, width=1.4))

preset("Granular Dust", "Textures & FX", ["Experimental", "Lo-fi", "Ambient"],
       "Grainy, crumbling texture — FM grains through a bit-crusher, like a melody dissolving into dust.",
       "Experimental beds, lo-fi transitions.", "texture",
       [L(7, grate=35, glen=.06, goct=.2, gfifth=.3, gjit=30, gfm=1.2, a=.5, d=2, s=.8, r=1.5)],
       {"f_type": 0, "f_cut": 2800, "lfo_rate": .3, "lfo_cut": .7},
       fx(crush_sr=11000, crush_bits=7, eq_lc=150, rv_mix=.38, rv_size=.85, rv_decay=4.5))

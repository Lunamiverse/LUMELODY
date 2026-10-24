"""The four sound libraries' playable instruments (pads, bass, strings, keys): 5 each per library = 80 presets.
Original synthesis on the LUMI engine. Libraries: NEW JAZZ, DISCO + EDM + CLUB, REGGAETON, DEMBOW DOMINICANO.
Drums, hi-hats, percussion, effects and textures are audio samples (tools/samples/), not presets.
"""

def add(preset, L, fx, TAPE, UNDERWATER, PRESETS):
    SPACE = {"rv_mix": .3, "rv_size": .8, "rv_decay": 3.5, "rv_damp": .55}
    HALL = {"rv_mix": .38, "rv_size": .92, "rv_decay": 5.5, "rv_damp": .5, "rv_pre": 25}
    ROOM = {"rv_mix": .18, "rv_size": .5, "rv_decay": 1.4, "rv_damp": .5}
    PLATE = {"rv_mix": .24, "rv_size": .65, "rv_decay": 2.2, "rv_damp": .35}
    DOT = {"dly_beats": .75, "dly_fb": .35, "dly_mix": .16, "dly_damp": .5}
    EIGHTH = {"dly_beats": .5, "dly_fb": .3, "dly_mix": .12, "dly_damp": .5}
    CHORUS = {"ch_mix": .35, "ch_depth": 2.5, "ch_rate": .4}
    K, P, B, S = (40, 88), (36, 84), (24, 52), (40, 90)

    def lib(name, library, kind, genres, desc, use, phrase, layers, voice, f, rng):
        preset(name, library, genres, desc, use, phrase, layers, voice, f, rng)
        PRESETS[-1]["library"] = library
        PRESETS[-1]["kind"] = kind

    # =============================================================== NEW JAZZ
    NJ = "New Jazz"
    G = ["New jazz", "Underground rap", "Ambient hip-hop"]
    # keys
    lib("Velvet Rhodes", NJ, "Keys", G + ["Experimental R&B"], "Warm tine electric piano with a soft bark, slow tremolo and a dark room: the late-night chord sound.",
        "Extended chords (9ths, 13ths) and lazy voicings.", "nj_keys",
        [L(2, cr=1, mr=1, idx=1.6, idx_dec=.7, idx_sus=.18, fb=.04, fm_spread=4, a=.002, d=3.8, s=.12, r=.7, keydecay=.3),
         L(2, cr=1, mr=14, idx=1.2, idx_dec=.01, idx_sus=0, a=.001, d=.25, s=0, r=.1, level=.16), L(1, wave=0, a=.003, d=3, s=.1, r=.7, level=.2, filt=0)],
        {"f_type": 0, "f_cut": 2600, "f_res": .1, "f_key": .35, "lfo_rate": 4.2, "lfo_amp": .16, "lfo_pan": .2},
        fx(TAPE, SPACE, CHORUS, eq_hc=9000, width=1.25), K)
    lib("Smoke Wurli", NJ, "Keys", G, "Reedy electric piano pushed into a little grit: nasal, warm and close, with tape wobble.",
        "Comping, short stabs and dusty loops.", "nj_keys",
        [L(2, cr=1, mr=1, idx=2.6, idx_dec=.35, idx_sus=.25, fb=.12, a=.002, d=2.2, s=.2, r=.4, keydecay=.4),
         L(2, cr=1, mr=3, idx=.6, idx_dec=.08, idx_sus=0, a=.001, d=.4, s=0, r=.2, level=.2)],
        {"f_type": 0, "f_cut": 3200, "f_res": .15, "f_key": .3, "f_drive": 1.6, "vib_rate": .7, "vib_depth": 6, "vib_delay": 0},
        fx(TAPE, ROOM, sat_drive=.35, sat_mix=.5, eq_lc=90, width=1.1), K)
    lib("Glass Keys", NJ, "Keys", G + ["Melodic trap"], "Digital glassy keys: a bell-like FM tine over a soft body, spread by a dotted echo.",
        "Top-line chords and sparkling melodies over dark pads.", "nj_keys",
        [L(2, cr=1, mr=3.5, idx=1.4, idx_dec=.5, idx_sus=.1, a=.002, d=2.5, s=.08, r=.8, keydecay=.25),
         L(4, mat=2, hard=.55, click=.15, a=.001, d=2.5, s=0, r=.6, level=.35), L(1, wave=1, a=.01, d=2, s=.2, r=.6, level=.18, filt=0)],
        {"f_type": 0, "f_cut": 6000, "f_key": .2},
        fx(SPACE, DOT, eq_lc=140, width=1.4), K)
    lib("Late Set Piano", NJ, "Keys", G + ["Lo-fi"], "Soft, slightly detuned upright piano with the felt pressed down: intimate and imperfect.",
        "Sparse melodies and slow chords; leave space.", "nj_keys",
        [L(3, bright=.42, damp=.45, decay=3.5, pick=.12, strings=2, sdet=3, a=.001, d=4, s=.05, r=.5, keydecay=.5),
         L(2, cr=1, mr=1, idx=.5, idx_dec=.3, idx_sus=.1, a=.002, d=3, s=.05, r=.5, level=.3)],
        {"f_type": 0, "f_cut": 3800, "f_key": .4, "f_vel": 1.2},
        fx(TAPE, ROOM, eq_hc=6500, width=1.15), K)
    lib("Neon Clav", NJ, "Keys", G + ["Funk"], "Funky pulse clav with a snappy filter: bright on the attack, gone in a moment.",
        "Rhythmic chord chops and syncopated riffs.", "nj_keys",
        [L(1, wave=3, pw=.28, a=.001, d=.35, s=.1, r=.12), L(1, wave=2, oct=1, a=.001, d=.2, s=0, r=.1, level=.25)],
        {"f_type": 0, "f_cut": 900, "f_res": .35, "f_env": 3.2, "fa": .001, "fd": .16, "fs": 0, "fr": .1, "f_key": .5, "f_vel": 1.4},
        fx(ROOM, CHORUS, eq_lc=120, width=1.2), K)
    # pads
    lib("Blue Hour Pad", NJ, "Pads", G, "Dark, wide detuned pad whose filter breathes very slowly, like the sky right after sunset.",
        "Long chords under rap verses; let it evolve.", "nj_pad",
        [L(1, wave=2, uni=6, det=16, spread=1, drift=8, a=1.2, d=2, s=.9, r=2.5), L(1, wave=0, oct=-1, a=1, d=2, s=.9, r=2.5, level=.3, filt=0)],
        {"f_type": 0, "f_cut": 1100, "f_res": .18, "f_poles": 4, "lfo_rate": .07, "lfo_cut": .9, "lfo_pan": .2},
        fx(HALL, CHORUS, eq_lc=70, width=1.5), P)
    lib("Midnight Choir", NJ, "Pads", G + ["Experimental R&B"], "Two synthesized voices singing a soft 'oh' that drifts toward 'oo', in a long dark room.",
        "Hook beds and emotional breakdowns.", "nj_pad",
        [L(6, vowel=1.5, vmod=.5, formshift=1, tilt=1.2, breath=.3, a=.8, d=2, s=.9, r=2), L(6, vowel=3, vmod=.4, formshift=1.04, breath=.25, oct=-1, a=1, d=2, s=.9, r=2, level=.6)],
        {"f_type": 0, "f_cut": 3500, "lfo_rate": .15, "vib_rate": 4.5, "vib_depth": 8, "vib_delay": .8},
        fx(HALL, eq_lc=150, width=1.45), P)
    lib("Tape Nebula", NJ, "Pads", G + ["Ambient"], "A cloud of soft grains over a warm saw bed, warbling like an old tape.",
        "Atmosphere for intros, interludes and outros.", "nj_pad",
        [L(7, grate=18, glen=.3, goct=.15, gjit=12, gfifth=.2, a=1.4, d=2, s=.85, r=2.8, level=.6), L(1, wave=2, uni=3, det=10, drift=12, a=1.2, d=2, s=.8, r=2.8, level=.6)],
        {"f_type": 0, "f_cut": 1800, "f_res": .1, "vib_rate": .3, "vib_depth": 14, "vib_delay": 0},
        fx(TAPE, HALL, eq_hc=7000, width=1.5), P)
    lib("Cold Horizon", NJ, "Pads", G, "Glassy FM pad with a slow attack and faint bell partials, clean and futuristic.",
        "Chord pads that leave room for 808 and vocals.", "nj_pad",
        [L(2, cr=1, mr=2, idx=.9, idx_dec=2, idx_sus=.4, fm_spread=8, a=1.5, d=3, s=.8, r=3), L(2, cr=2, mr=5, idx=.4, idx_dec=1, idx_sus=.2, a=2, d=3, s=.6, r=3, level=.3)],
        {"f_type": 0, "f_cut": 5000, "lfo_rate": .09, "lfo_pan": .3},
        fx(HALL, DOT, eq_lc=120, width=1.6), P)
    lib("Smoked Glass Pad", NJ, "Pads", G + ["Melodic trap"], "Hollow pulse pad with breath noise on top, low-passed and moody.",
        "Minor chords for melodic trap and underground rap.", "nj_pad",
        [L(1, wave=3, pw=.4, uni=3, det=9, a=.9, d=2, s=.85, r=2.2), L(5, ncut=1.5, nq=2, ntrack=1, nmode=1, a=1, d=2, s=.7, r=2, level=.2)],
        {"f_type": 0, "f_cut": 1400, "f_res": .2, "f_poles": 4, "lfo_rate": .12, "lfo_cut": .6},
        fx(SPACE, CHORUS, eq_lc=80, width=1.35), P)
    # bass
    lib("Upright Shadow", NJ, "Bass", G + ["Jazz rap"], "Round plucked upright-style bass (string model): woody attack, short warm sustain.",
        "Walking lines and melodic bass under keys.", "nj_bass",
        [L(3, bright=.3, damp=.55, decay=1.6, pick=.25, a=.001, d=1.8, s=0, r=.25, keydecay=.2), L(1, wave=0, a=.004, d=1.2, s=.2, r=.2, level=.45, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 1200, "f_key": .3, "f_vel": 1},
        fx(ROOM, eq_lc=35, eq_hc=4500, rv_mix=.08), B)
    lib("Fretless Night", NJ, "Bass", G, "Singing fretless-style bass that slides between notes with a slow vibrato.",
        "Melodic bass lines that answer the vocal.", "nj_bass",
        [L(1, wave=1, a=.01, d=1, s=.8, r=.3), L(1, wave=0, oct=-1, a=.01, d=1, s=.8, r=.3, level=.5, filt=0)],
        {"mono": 1, "glide": .09, "f_type": 0, "f_cut": 900, "f_key": .4, "vib_rate": 4.8, "vib_depth": 14, "vib_delay": .35},
        fx(TAPE, eq_lc=30, rv_mix=.06), B)
    lib("Dusty Sub", NJ, "Bass", G + ["Lo-fi"], "Clean sub with a soft second harmonic and tape warmth: felt more than heard.",
        "Long low notes under sparse drums.", "nj_bass",
        [L(1, wave=0, a=.005, d=2, s=.85, r=.3), L(1, wave=1, oct=1, a=.005, d=1, s=.3, r=.3, level=.12)],
        {"mono": 1, "glide": .05, "f_type": -1},
        fx(TAPE, eq_lc=25, eq_hc=4000), B)
    lib("Velvet Ladder", NJ, "Bass", G + ["Experimental R&B"], "Warm analog saw bass through a four-pole filter that opens on every note.",
        "Moving bass lines with a little bite.", "nj_bass",
        [L(1, wave=2, uni=2, det=4, a=.002, d=.8, s=.6, r=.2), L(1, wave=0, oct=-1, a=.002, d=.8, s=.7, r=.2, level=.5, filt=0)],
        {"mono": 1, "glide": .04, "f_type": 0, "f_cut": 380, "f_res": .32, "f_poles": 4, "f_env": 2.4, "fa": .002, "fd": .28, "fs": .25, "fr": .2, "f_key": .3, "f_drive": 1.4},
        fx(eq_lc=30, sat_drive=.2, sat_mix=.4), B)
    lib("Jazz 808", NJ, "Bass", G + ["Melodic trap"], "Mellow 808 with a soft knock and a gentle pitch drop, round enough for jazzy chords.",
        "808 lines under extended chords.", "nj_bass",
        [L(1, wave=0, a=.001, d=4, s=.75, r=.35), L(1, wave=1, oct=1, a=.001, d=.25, s=.05, r=.2, level=.1)],
        {"mono": 1, "glide": .12, "f_type": -1, "p_env": 5, "p_dec": .03},
        fx(eq_lc=24, eq_hc=6000, sat_drive=.25, sat_mix=.4), B)
    # strings
    lib("Noir Strings", NJ, "Strings", G + ["Cinematic"], "A dark, slow-blooming string section (synthesized), weighted low, in a long hall.",
        "Cinematic layers and emotional chord swells.", "nj_strings",
        [L(1, wave=2, uni=7, det=14, spread=1, drift=6, a=.7, d=2, s=.9, r=1.6), L(6, vowel=0, vmod=.2, breath=.1, a=.8, d=2, s=.6, r=1.5, level=.25)],
        {"f_type": 0, "f_cut": 2000, "f_res": .1, "f_key": .3, "vib_rate": 5, "vib_depth": 9, "vib_delay": .5},
        fx(HALL, eq_lc=90, width=1.4), S)
    lib("Cello Dusk", NJ, "Strings", G, "Solo cello-style voice with bow noise and a slow vibrato that blooms after the attack.",
        "Lead lines in the low-mid register; play legato.", "nj_strings",
        [L(1, wave=2, a=.15, d=1, s=.9, r=.4), L(6, vowel=2, vmod=.15, breath=.2, a=.18, d=1, s=.7, r=.4, level=.35), L(5, ncut=3, nq=1.5, ntrack=1, nmode=1, a=.05, d=.3, s=.15, r=.3, level=.08)],
        {"mono": 1, "glide": .08, "f_type": 0, "f_cut": 2200, "f_key": .4, "vib_rate": 5.2, "vib_depth": 18, "vib_delay": .45},
        fx(SPACE, eq_lc=60, width=1.2), (28, 72))
    lib("Pizzicato Rain", NJ, "Strings", G, "Plucked string section, short and woody, scattering into a long dark room.",
        "Ostinatos, counter-melodies and tension.", "nj_strings",
        [L(3, bright=.5, damp=.4, decay=.8, pick=.3, strings=3, sdet=5, a=.001, d=.9, s=0, r=.3)],
        {"f_type": 0, "f_cut": 4500, "f_key": .3},
        fx(HALL, EIGHTH, eq_lc=100, width=1.4), S)
    lib("Tape Violins", NJ, "Strings", G + ["Lo-fi"], "High violins that waver like a worn tape, band-limited and nostalgic.",
        "Sampled-sounding string loops and hooks.", "nj_strings",
        [L(1, wave=2, uni=5, det=10, spread=.8, drift=14, a=.25, d=1.5, s=.85, r=.8)],
        {"f_type": 0, "f_cut": 3000, "f_key": .2, "vib_rate": .45, "vib_depth": 18, "vib_delay": 0},
        fx(TAPE, SPACE, eq_lc=300, eq_hc=6000, crush_bits=0, width=1.3), S)
    lib("Velvet Quartet", NJ, "Strings", G + ["Experimental R&B"], "Soft, warm chamber strings: four voices close together, gentle attack.",
        "Supporting chords under keys and vocals.", "nj_strings",
        [L(1, wave=2, uni=4, det=8, spread=.7, a=.35, d=2, s=.88, r=1.2), L(2, cr=1, mr=1, idx=.4, idx_dec=1, idx_sus=.4, a=.4, d=2, s=.8, r=1.2, level=.3)],
        {"f_type": 0, "f_cut": 2400, "f_key": .3, "vib_rate": 5.4, "vib_depth": 7, "vib_delay": .6},
        fx(SPACE, eq_lc=110, width=1.3), S)

    # =============================================================== DISCO + EDM + CLUB
    DC = "Disco + EDM + Club"
    G = ["Disco house", "Nu-disco", "Dance-pop", "Club"]
    lib("Disco Rhodes", DC, "Keys", G, "Bright, bouncy electric piano with a chorus shimmer: the classic disco chord sound, cleaned up.",
        "Offbeat chord stabs and funky comping.", "disco_keys",
        [L(2, cr=1, mr=1, idx=2, idx_dec=.45, idx_sus=.22, fb=.05, a=.001, d=2, s=.2, r=.35, keydecay=.3), L(2, cr=1, mr=14, idx=1.4, idx_dec=.01, idx_sus=0, a=.001, d=.2, s=0, r=.1, level=.2)],
        {"f_type": 0, "f_cut": 5000, "f_key": .3},
        fx(ROOM, CHORUS, eq_lc=110, eq_hs_f=6000, eq_hs_g=2, width=1.3), K)
    lib("House Organ Stab", DC, "Keys", G + ["Classic house"], "Punchy organ-bass stab, the hollow '90s house chord: short, round, cutting.",
        "One-chord house riffs; play rhythmically.", "disco_keys",
        [L(2, cr=1, mr=1, idx=1.1, idx_dec=.15, idx_sus=.3, fb=.25, a=.001, d=.45, s=.25, r=.12), L(1, wave=3, pw=.5, oct=-1, a=.001, d=.3, s=.2, r=.1, level=.35)],
        {"f_type": 0, "f_cut": 2600, "f_res": .2, "f_env": 1.2, "fa": .001, "fd": .12, "fs": .3, "f_key": .3},
        fx(ROOM, eq_lc=90, sat_drive=.25, sat_mix=.4, width=1.1), K)
    lib("Piano House", DC, "Keys", G + ["Piano house"], "Bright, percussive piano for house chords: strong attack, quick decay, a touch of plate.",
        "Classic piano-house riffs and uplifting chords.", "disco_keys",
        [L(3, bright=.75, damp=.25, decay=2.4, pick=.1, strings=3, sdet=2, a=.001, d=2.5, s=.05, r=.35, keydecay=.4), L(4, mat=3, hard=.6, click=.25, a=.001, d=1.2, s=0, r=.3, level=.25)],
        {"f_type": 0, "f_cut": 9000, "f_vel": 1},
        fx(PLATE, eq_lc=120, eq_hs_f=5000, eq_hs_g=2.5, width=1.25), K)
    lib("Funk Clav", DC, "Keys", G + ["Funk"], "Wah-ed clav with a resonant envelope filter: quacky, tight and danceable.",
        "16th-note funk riffs and syncopated chords.", "disco_keys",
        [L(1, wave=3, pw=.22, a=.001, d=.3, s=.15, r=.08), L(1, wave=2, a=.001, d=.25, s=.05, r=.08, level=.3)],
        {"f_type": 1, "f_cut": 700, "f_res": .55, "f_env": 3.5, "fa": .001, "fd": .13, "fs": .1, "fr": .08, "f_key": .4, "f_vel": 1.6},
        fx(ROOM, eq_lc=140, width=1.15), K)
    lib("Supersaw Stab", DC, "Keys", G + ["EDM"], "Seven-voice supersaw chord stab with a short tail and a sidechain pump.",
        "EDM chord stabs and drop riffs.", "disco_keys",
        [L(1, wave=2, uni=7, det=22, spread=1, a=.001, d=.4, s=.25, r=.15), L(1, wave=2, oct=-1, uni=3, det=10, a=.001, d=.3, s=.2, r=.12, level=.35)],
        {"f_type": 0, "f_cut": 6500, "f_res": .1, "f_env": 1, "fa": .001, "fd": .2, "fs": .5},
        fx(PLATE, EIGHTH, pump=.35, eq_lc=150, width=1.6), K)
    lib("Euphoria Pad", DC, "Pads", G + ["EDM"], "Bright, wide supersaw pad that pumps with the kick: hands-in-the-air energy.",
        "Breakdowns and big drops.", "disco_pad",
        [L(1, wave=2, uni=7, det=20, spread=1, a=.25, d=2, s=.9, r=1.2), L(1, wave=2, oct=1, uni=3, det=12, a=.3, d=2, s=.8, r=1.2, level=.3)],
        {"f_type": 0, "f_cut": 7000, "f_res": .1},
        fx(HALL, pump=.45, eq_lc=130, width=1.7), P)
    lib("Mirrorball Pad", DC, "Pads", G, "Shimmering detuned pad with a glassy top and a lush chorus, like light on a disco ball.",
        "Glittering chord beds for disco and nu-disco.", "disco_pad",
        [L(1, wave=2, uni=5, det=14, spread=1, a=.4, d=2, s=.85, r=1.6), L(4, mat=2, hard=.4, click=0, a=.6, d=2, s=.3, r=1.6, level=.18), L(2, cr=2, mr=1, idx=.5, idx_dec=1.5, idx_sus=.3, a=.5, d=2, s=.6, r=1.6, level=.25)],
        {"f_type": 0, "f_cut": 6000, "lfo_rate": .25, "lfo_pan": .25},
        fx(HALL, CHORUS, eq_lc=140, width=1.6), P)
    lib("Analog Sunrise", DC, "Pads", G + ["French house"], "Warm analog saw pad whose filter slowly opens and closes: retro and hypnotic.",
        "Filter-house chords; automate the filter for builds.", "disco_pad",
        [L(1, wave=2, uni=4, det=10, spread=.8, drift=4, a=.3, d=2, s=.9, r=1.2)],
        {"f_type": 0, "f_cut": 1500, "f_res": .3, "f_poles": 4, "lfo_sync": 4, "lfo_cut": 1.4},
        fx(SPACE, CHORUS, eq_lc=100, width=1.4), P)
    lib("Festival Air", DC, "Pads", G + ["EDM", "Festival"], "Airy, open pad of noise and saws, huge and light at the top of the mix.",
        "Intros, risers and breakdown atmospheres.", "disco_pad",
        [L(1, wave=2, uni=6, det=24, spread=1, a=.8, d=2, s=.85, r=2), L(5, ncut=6000, nq=.5, nmode=2, a=1, d=2, s=.8, r=2, level=.16)],
        {"f_type": 0, "f_cut": 9000, "lfo_rate": .1, "lfo_pan": .3},
        fx(HALL, eq_lc=220, width=1.8), P)
    lib("Velvet Rope Pad", DC, "Pads", G + ["Deep house"], "Darker lush pad for deep house: rounded saws and a soft breathy layer.",
        "Late-night deep house chords.", "disco_pad",
        [L(1, wave=2, uni=4, det=9, a=.6, d=2, s=.9, r=1.6), L(6, vowel=1, vmod=.3, breath=.3, a=.7, d=2, s=.6, r=1.6, level=.25)],
        {"f_type": 0, "f_cut": 2200, "f_res": .15, "f_poles": 4},
        fx(SPACE, pump=.2, eq_lc=90, width=1.4), P)
    lib("Disco Octave Bass", DC, "Bass", G, "Plucky, bright octave bass: a snappy filter attack on every note, tight low end.",
        "Classic octave-jumping disco lines.", "disco_bass",
        [L(1, wave=2, a=.001, d=.35, s=.3, r=.1), L(1, wave=3, pw=.45, oct=-1, a=.001, d=.3, s=.4, r=.1, level=.4)],
        {"mono": 1, "f_type": 0, "f_cut": 500, "f_res": .3, "f_env": 2.8, "fa": .001, "fd": .12, "fs": .2, "fr": .1, "f_key": .3},
        fx(eq_lc=35, sat_drive=.15, sat_mix=.3), B)
    lib("Funk Finger Bass", DC, "Bass", G + ["Funk"], "Round finger-style bass (string model) with a short growl, made for syncopation.",
        "Funky disco lines with ghost notes.", "disco_bass",
        [L(3, bright=.45, damp=.45, decay=1.2, pick=.3, a=.001, d=1.2, s=0, r=.15, keydecay=.2), L(1, wave=0, a=.003, d=.9, s=.3, r=.15, level=.4, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 1800, "f_vel": 1.2},
        fx(eq_lc=35, eq_hc=6000, sat_drive=.15, sat_mix=.3), B)
    lib("Rolling Club Bass", DC, "Bass", G + ["Tech house"], "Offbeat saw bass that rolls under four-on-the-floor, ducking with the kick.",
        "Rolling offbeat bass for club grooves.", "disco_bass",
        [L(1, wave=2, uni=2, det=5, a=.001, d=.25, s=.2, r=.08), L(1, wave=0, oct=-1, a=.001, d=.25, s=.5, r=.08, level=.6, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 700, "f_res": .25, "f_env": 1.8, "fa": .001, "fd": .1, "fs": .2},
        fx(pump=.4, eq_lc=30), B)
    lib("Big Room Reese", DC, "Bass", G + ["EDM"], "Wide, clean detuned reese for EDM drops: moving phase, solid centre.",
        "Drop bass and sustained low chords.", "disco_bass",
        [L(1, wave=2, uni=4, det=18, spread=.6, a=.005, d=1, s=.9, r=.2), L(1, wave=0, oct=-1, a=.005, d=1, s=.9, r=.2, level=.6, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 1300, "f_res": .2, "f_poles": 4, "lfo_rate": .3, "lfo_cut": .4},
        fx(eq_lc=30, sat_drive=.25, sat_mix=.45, width=1.3), B)
    lib("French Filter Bass", DC, "Bass", G + ["French house"], "Resonant filtered bass that wah-wahs in time: the retro filter-house sound.",
        "Hypnotic, filtered basslines.", "disco_bass",
        [L(1, wave=2, a=.002, d=.6, s=.7, r=.15), L(1, wave=3, pw=.3, a=.002, d=.6, s=.7, r=.15, level=.4)],
        {"mono": 1, "glide": .03, "f_type": 0, "f_cut": 600, "f_res": .55, "f_poles": 4, "lfo_sync": 1, "lfo_cut": 1.2},
        fx(eq_lc=35, sat_drive=.2, sat_mix=.4), B)
    lib("Disco Strings", DC, "Strings", G, "Bright, lush disco string section with a chorus sheen and a light vibrato.",
        "Runs, stabs and soaring held chords.", "disco_strings",
        [L(1, wave=2, uni=7, det=12, spread=1, a=.08, d=1.5, s=.9, r=.6), L(1, wave=2, oct=1, uni=3, det=8, a=.1, d=1.5, s=.7, r=.6, level=.25)],
        {"f_type": 0, "f_cut": 7000, "vib_rate": 5.5, "vib_depth": 8, "vib_delay": .3},
        fx(PLATE, CHORUS, eq_lc=160, width=1.6), S)
    lib("String Stabs 79", DC, "Strings", G, "Short, punchy orchestral stabs: fast attack, quick decay, a bit of plate.",
        "Rhythmic hits on the offbeats.", "disco_strings",
        [L(1, wave=2, uni=6, det=10, spread=1, a=.005, d=.3, s=.1, r=.15), L(5, ncut=3000, nq=.6, a=.001, d=.08, s=0, r=.05, level=.15)],
        {"f_type": 0, "f_cut": 6000, "f_env": 1, "fa": .001, "fd": .1, "fs": .5},
        fx(PLATE, eq_lc=180, width=1.5), S)
    lib("Rising Strings", DC, "Strings", G + ["EDM"], "Strings that swell in slowly, built for builds and transitions.",
        "Hold a chord through a build-up.", "disco_strings",
        [L(1, wave=2, uni=7, det=14, spread=1, a=1.8, d=2, s=1, r=1), L(6, vowel=0, breath=.15, a=2, d=2, s=.6, r=1, level=.2)],
        {"f_type": 0, "f_cut": 3000, "f_env": 1.2, "fa": 2, "fd": 1, "fs": 1, "vib_rate": 5, "vib_depth": 8, "vib_delay": .8},
        fx(HALL, eq_lc=150, width=1.6), S)
    lib("Silk Violins", DC, "Strings", G + ["Dance-pop"], "Bright, high violins with an expressive vibrato, silky on top.",
        "Toplines and counter-melodies above the chords.", "disco_strings",
        [L(1, wave=2, uni=4, det=8, spread=.8, a=.12, d=1, s=.9, r=.5), L(6, vowel=2, vmod=.1, breath=.1, a=.15, d=1, s=.6, r=.5, level=.2)],
        {"f_type": 0, "f_cut": 7500, "vib_rate": 5.8, "vib_depth": 14, "vib_delay": .35},
        fx(PLATE, eq_lc=250, width=1.4), (52, 96))
    lib("Orchestra Hit", DC, "Strings", G + ["Freestyle"], "Big orchestral hit: brass-like saws, a bell strike and a burst of noise, all at once.",
        "Accents, drops and retro dance hits.", "disco_strings",
        [L(1, wave=2, uni=7, det=16, spread=1, a=.001, d=.5, s=0, r=.3), L(4, mat=3, hard=.7, click=.4, a=.001, d=.8, s=0, r=.3, level=.3), L(5, ncut=2500, nq=.5, a=.001, d=.15, s=0, r=.1, level=.3)],
        {"f_type": 0, "f_cut": 5000, "f_env": 1.5, "fa": .001, "fd": .2, "fs": .3},
        fx(HALL, eq_lc=120, width=1.5), S)

    # =============================================================== REGGAETON
    RG = "Reggaetón"
    G = ["Reggaetón", "Latin urban", "Perreo"]
    lib("Perreo Keys", RG, "Keys", G + ["Dark reggaetón"], "Dark, warm electric keys with a soft bark: sensual minor chords.",
        "Chord loops for dark and sensual reggaetón.", "rg_keys",
        [L(2, cr=1, mr=1, idx=1.8, idx_dec=.6, idx_sus=.2, fb=.06, a=.002, d=3, s=.15, r=.6, keydecay=.3), L(1, wave=1, a=.003, d=2.5, s=.15, r=.6, level=.2, filt=0)],
        {"f_type": 0, "f_cut": 2400, "f_key": .35},
        fx(SPACE, CHORUS, eq_lc=100, width=1.3), K)
    lib("Sensual Piano", RG, "Keys", G + ["Romantic"], "Soft piano with a long dark reverb, for romantic and emotional reggaetón.",
        "Intros, ballad sections and emotional hooks.", "rg_keys",
        [L(3, bright=.55, damp=.35, decay=3, pick=.12, strings=2, sdet=1.5, a=.001, d=3.5, s=.05, r=.5, keydecay=.45), L(2, cr=1, mr=1, idx=.4, idx_dec=.5, idx_sus=.1, a=.002, d=3, s=.05, r=.5, level=.25)],
        {"f_type": 0, "f_cut": 5000, "f_vel": 1},
        fx(HALL, eq_lc=90, width=1.25), K)
    lib("Romántico Pluck", RG, "Keys", G + ["Bachata-pop"], "Nylon-string pluck (string model), warm and romantic, with a short echo.",
        "Romantic guitar-style riffs and arpeggios.", "rg_keys",
        [L(3, bright=.5, damp=.3, decay=1.8, pick=.2, strings=2, sdet=3, a=.001, d=2, s=0, r=.3)],
        {"f_type": 0, "f_cut": 6000},
        fx(SPACE, EIGHTH, eq_lc=120, width=1.3), K)
    lib("Neon Marimba", RG, "Keys", G + ["Old-school reggaetón"], "Bright marimba with a round wooden tone: the old-school reggaetón melody sound.",
        "Catchy riffs and 3-3-2 melodies.", "rg_keys",
        [L(4, mat=0, hard=.6, click=.3, a=.001, d=1.2, s=0, r=.3), L(1, wave=0, a=.001, d=.3, s=0, r=.2, level=.15)],
        {"f_type": -1},
        fx(ROOM, eq_lc=150, width=1.2), K)
    lib("2000s Synth Lead", RG, "Keys", G + ["Old-school reggaetón"], "Classic square-and-saw synth lead with a short glide: early-2000s energy.",
        "Hooks and call-and-response riffs.", "rg_keys",
        [L(1, wave=3, pw=.5, a=.002, d=.6, s=.6, r=.15), L(1, wave=2, uni=2, det=8, a=.002, d=.6, s=.6, r=.15, level=.5)],
        {"mono": 1, "glide": .05, "f_type": 0, "f_cut": 3200, "f_res": .2, "vib_rate": 5.5, "vib_depth": 10, "vib_delay": .3},
        fx(ROOM, EIGHTH, eq_lc=200, width=1.2), K)
    lib("Noche Pad", RG, "Pads", G + ["Dark reggaetón"], "Warm, dark pad that sits under the dembow without crowding it.",
        "Chord bed for dark reggaetón.", "rg_pad",
        [L(1, wave=2, uni=5, det=12, spread=1, a=.6, d=2, s=.9, r=1.5)],
        {"f_type": 0, "f_cut": 1300, "f_res": .15, "f_poles": 4, "lfo_rate": .1, "lfo_cut": .4},
        fx(SPACE, CHORUS, eq_lc=110, width=1.45), P)
    lib("Sensual Haze", RG, "Pads", G, "Breathy vocal pad, a soft 'ah' hanging in the air.",
        "Sensual hooks and pre-choruses.", "rg_pad",
        [L(6, vowel=0, vmod=.4, breath=.35, a=.7, d=2, s=.85, r=1.8), L(1, wave=1, uni=3, det=8, a=.7, d=2, s=.7, r=1.8, level=.35)],
        {"f_type": 0, "f_cut": 3000, "vib_rate": 4.5, "vib_depth": 7, "vib_delay": .7},
        fx(HALL, eq_lc=150, width=1.5), P)
    lib("Club Perreo Pad", RG, "Pads", G + ["Club"], "Brighter saw pad that pumps in time: club reggaetón energy.",
        "Choruses and club drops.", "rg_pad",
        [L(1, wave=2, uni=7, det=18, spread=1, a=.2, d=2, s=.9, r=1)],
        {"f_type": 0, "f_cut": 5000, "f_res": .1},
        fx(SPACE, pump=.4, eq_lc=140, width=1.6), P)
    lib("Futuro Pad", RG, "Pads", G + ["Experimental"], "Experimental FM-and-grain pad, metallic and moving, for futuristic reggaetón.",
        "Intros and atmospheres with an edge.", "rg_pad",
        [L(2, cr=1, mr=1.41, idx=1.2, idx_dec=2, idx_sus=.5, fm_spread=10, a=.8, d=2, s=.8, r=2), L(7, grate=22, glen=.2, goct=.3, gjit=10, a=1, d=2, s=.7, r=2, level=.4)],
        {"f_type": 0, "f_cut": 4000, "lfo_rate": .2, "lfo_pan": .35},
        fx(HALL, DOT, eq_lc=150, width=1.6), P)
    lib("Melancolía Pad", RG, "Pads", G + ["Melodic"], "Emotional, string-like pad with a slow vibrato, for melodic reggaetón.",
        "Heartbreak sections and bridges.", "rg_pad",
        [L(1, wave=2, uni=6, det=10, spread=1, a=.9, d=2, s=.9, r=1.8), L(6, vowel=3, vmod=.2, breath=.15, a=1, d=2, s=.6, r=1.8, level=.25)],
        {"f_type": 0, "f_cut": 2500, "vib_rate": 5, "vib_depth": 9, "vib_delay": .6},
        fx(HALL, eq_lc=100, width=1.5), P)
    lib("Reggaetón Sub", RG, "Bass", G, "Deep, clean sub locked to the dembow: pure low end, no mud.",
        "Bass on the kick pattern.", "rg_bass",
        [L(1, wave=0, a=.003, d=1, s=.9, r=.12)],
        {"mono": 1, "f_type": -1},
        fx(eq_lc=25, eq_hc=3000), B)
    lib("Perreo 808", RG, "Bass", G, "Punchy 808 with a short tail and some drive: hits hard on the dembow.",
        "808 lines that follow the kick.", "rg_bass",
        [L(1, wave=0, a=.001, d=1.2, s=.4, r=.15), L(1, wave=1, oct=1, a=.001, d=.15, s=0, r=.1, level=.15)],
        {"mono": 1, "glide": .06, "f_type": -1, "p_env": 4, "p_dec": .025},
        fx(eq_lc=25, sat_drive=.4, sat_mix=.5), B)
    lib("Latin Groove Bass", RG, "Bass", G + ["Latin pop"], "Round, bouncy synth bass with a soft filter pluck: groovy and controlled.",
        "Melodic Latin bass lines.", "rg_bass",
        [L(1, wave=2, a=.002, d=.4, s=.5, r=.1), L(1, wave=0, oct=-1, a=.002, d=.4, s=.7, r=.1, level=.5, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 650, "f_res": .2, "f_env": 1.6, "fa": .001, "fd": .15, "fs": .3},
        fx(eq_lc=30), B)
    lib("Oscuro 808", RG, "Bass", G + ["Dark reggaetón"], "Long, dark, distorted 808 with wide slides for dark reggaetón.",
        "Slow, heavy 808 lines with slides.", "rg_bass",
        [L(1, wave=0, a=.001, d=5, s=.7, r=.3)],
        {"mono": 1, "glide": .15, "f_type": -1, "p_env": 6, "p_dec": .03},
        fx(eq_lc=24, eq_hc=6000, sat_drive=.6, sat_mix=.6), B)
    lib("Old School Bass", RG, "Bass", G + ["Old-school reggaetón"], "Square-edged sub bass with a touch of buzz: the early dembow low end.",
        "Old-school bass patterns.", "rg_bass",
        [L(1, wave=3, pw=.5, a=.002, d=.6, s=.6, r=.1), L(1, wave=0, a=.002, d=.6, s=.8, r=.1, level=.6, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 450, "f_res": .1},
        fx(eq_lc=30), B)
    lib("Romántico Strings", RG, "Strings", G + ["Romantic"], "Lush, warm string section for romantic reggaetón and Latin pop.",
        "Held chords in choruses and bridges.", "rg_strings",
        [L(1, wave=2, uni=7, det=12, spread=1, a=.4, d=2, s=.9, r=1.2)],
        {"f_type": 0, "f_cut": 3500, "vib_rate": 5.2, "vib_depth": 8, "vib_delay": .5},
        fx(HALL, CHORUS, eq_lc=120, width=1.5), S)
    lib("Pizzicato Perreo", RG, "Strings", G, "Short plucked strings with a playful bounce, tight to the beat.",
        "Rhythmic pizzicato riffs on the 3-3-2.", "rg_strings",
        [L(3, bright=.55, damp=.35, decay=.6, pick=.3, strings=3, sdet=4, a=.001, d=.7, s=0, r=.2)],
        {"f_type": 0, "f_cut": 5000},
        fx(SPACE, eq_lc=120, width=1.35), S)
    lib("Drama Strings", RG, "Strings", G + ["Cinematic"], "Low, dark staccato strings for tension and drama.",
        "Pre-drops and dark verses.", "rg_strings",
        [L(1, wave=2, uni=6, det=10, spread=1, a=.01, d=.4, s=.3, r=.2), L(5, ncut=2000, nq=.6, a=.001, d=.06, s=0, r=.05, level=.1)],
        {"f_type": 0, "f_cut": 2200, "f_env": .8, "fa": .001, "fd": .15, "fs": .5},
        fx(HALL, eq_lc=60, width=1.4), (28, 76))
    lib("Violín Solo", RG, "Strings", G + ["Melodic"], "Expressive solo violin-style lead with bow air and vibrato.",
        "Emotional melodies and intros.", "rg_strings",
        [L(1, wave=2, uni=2, det=4, a=.1, d=1, s=.9, r=.35), L(6, vowel=2, vmod=.15, breath=.12, a=.12, d=1, s=.7, r=.35, level=.3)],
        {"mono": 1, "glide": .07, "f_type": 0, "f_cut": 5000, "vib_rate": 5.6, "vib_depth": 14, "vib_delay": .4},
        fx(SPACE, eq_lc=200, width=1.2), (55, 96))
    lib("Cinematic Swell", RG, "Strings", G + ["Cinematic"], "A string swell that rises out of nothing: transitions and builds.",
        "Hold one chord into a drop.", "rg_strings",
        [L(1, wave=2, uni=7, det=14, spread=1, a=2.2, d=2, s=1, r=1)],
        {"f_type": 0, "f_cut": 2500, "f_env": 1.4, "fa": 2.4, "fd": 1, "fs": 1},
        fx(HALL, eq_lc=120, width=1.6), S)

    # =============================================================== DEMBOW DOMINICANO
    DD = "Dembow Dominicano"
    G = ["Dembow dominicano", "Dominican urban", "Dembow"]
    lib("Calle Keys", DD, "Keys", G, "Bright, punchy square-wave keys that cut through fast dembow: street energy.",
        "Catchy dembow riffs and hooks.", "dd_keys",
        [L(1, wave=3, pw=.4, a=.001, d=.5, s=.3, r=.1), L(1, wave=3, pw=.25, oct=1, a=.001, d=.3, s=.1, r=.1, level=.25)],
        {"f_type": 0, "f_cut": 3500, "f_res": .2, "f_env": 1, "fa": .001, "fd": .15, "fs": .5},
        fx(ROOM, eq_lc=180, width=1.2), K)
    lib("Tíguere Organ", DD, "Keys", G + ["Playful"], "Cheeky, buzzy organ with a fast vibrato: playful and loud.",
        "Fun melodies and party sections.", "dd_keys",
        [L(2, cr=1, mr=2, idx=1, idx_dec=1, idx_sus=.8, a=.002, d=1, s=.9, r=.1), L(2, cr=2, mr=1, idx=.6, idx_dec=1, idx_sus=.6, a=.002, d=1, s=.8, r=.1, level=.4)],
        {"f_type": 0, "f_cut": 4500, "vib_rate": 6.5, "vib_depth": 12, "vib_delay": 0},
        fx(ROOM, eq_lc=200, sat_drive=.25, sat_mix=.4), K)
    lib("Dembow Romántico Piano", DD, "Keys", G + ["Romantic"], "Soft, clear piano for the lighter, romantic side of dembow.",
        "Romantic hooks and intros.", "dd_keys",
        [L(3, bright=.6, damp=.3, decay=2.5, pick=.12, strings=2, sdet=1.5, a=.001, d=3, s=.05, r=.4, keydecay=.4)],
        {"f_type": 0, "f_cut": 6500, "f_vel": 1},
        fx(SPACE, eq_lc=110, width=1.25), K)
    lib("Barrio Bell", DD, "Keys", G, "Bright, metallic bell lead with a short echo: the sound that answers the vocal.",
        "Bell hooks and call-and-response.", "dd_keys",
        [L(4, mat=3, hard=.7, click=.3, a=.001, d=1.4, s=0, r=.3), L(2, cr=1, mr=3.5, idx=1.2, idx_dec=.4, idx_sus=0, a=.001, d=1, s=0, r=.3, level=.35)],
        {"f_type": -1},
        fx(ROOM, EIGHTH, eq_lc=250, width=1.3), K)
    lib("Playful Pluck", DD, "Keys", G + ["Playful"], "Bouncy synth pluck with a little pitch blip on every note.",
        "Light, fun melodies and bouncy riffs.", "dd_keys",
        [L(1, wave=2, a=.001, d=.25, s=0, r=.1), L(1, wave=3, pw=.3, oct=1, a=.001, d=.15, s=0, r=.08, level=.3)],
        {"f_type": 0, "f_cut": 2500, "f_res": .3, "f_env": 2, "fa": .001, "fd": .1, "fs": 0, "p_env": 1, "p_dec": .02},
        fx(ROOM, EIGHTH, eq_lc=200, width=1.3), K)
    lib("Malecón Pad", DD, "Pads", G + ["Melodic"], "Warm night pad, like the sea wall after midnight: soft and wide.",
        "Melodic dembow chords and breaks.", "dd_pad",
        [L(1, wave=2, uni=5, det=12, spread=1, a=.5, d=2, s=.9, r=1.4), L(1, wave=0, oct=-1, a=.5, d=2, s=.9, r=1.4, level=.25, filt=0)],
        {"f_type": 0, "f_cut": 2000, "f_res": .12, "f_poles": 4},
        fx(SPACE, CHORUS, eq_lc=110, width=1.5), P)
    lib("Underground Pad", DD, "Pads", G + ["Underground"], "Dark, gritty pad with a crushed edge for underground dembow.",
        "Dark intros and hard sections.", "dd_pad",
        [L(1, wave=2, uni=5, det=16, spread=1, a=.4, d=2, s=.9, r=1.2), L(5, ncut=1.2, nq=3, ntrack=1, nmode=1, a=.5, d=2, s=.6, r=1.2, level=.15)],
        {"f_type": 0, "f_cut": 1200, "f_res": .25, "f_poles": 4, "lfo_rate": .2, "lfo_cut": .5},
        fx(SPACE, eq_lc=100, crush_bits=9, sat_drive=.35, sat_mix=.4, width=1.3), P)
    lib("Fiesta Pad", DD, "Pads", G + ["Party"], "Bright, energetic pad that pumps with the beat: party time.",
        "Choruses and high-energy sections.", "dd_pad",
        [L(1, wave=2, uni=7, det=20, spread=1, a=.15, d=2, s=.9, r=.8)],
        {"f_type": 0, "f_cut": 6500},
        fx(ROOM, pump=.45, eq_lc=150, width=1.6), P)
    lib("Coro Pad", DD, "Pads", G, "Synthesized choir 'ah' for big moments: wide and bright.",
        "Hooks, chants and big chord moments.", "dd_pad",
        [L(6, vowel=0, vmod=.3, breath=.25, a=.4, d=2, s=.9, r=1.5), L(6, vowel=1, vmod=.3, breath=.2, formshift=1.05, oct=1, a=.5, d=2, s=.8, r=1.5, level=.5)],
        {"f_type": 0, "f_cut": 4000, "vib_rate": 5, "vib_depth": 8, "vib_delay": .4},
        fx(HALL, eq_lc=180, width=1.6), P)
    lib("Tropical Haze", DD, "Pads", G + ["Melodic"], "Airy, wide pad with a soft breathy top: warm, tropical, light.",
        "Lighter, melodic dembow and intros.", "dd_pad",
        [L(1, wave=1, uni=5, det=10, spread=1, a=.6, d=2, s=.85, r=1.6), L(5, ncut=5000, nq=.5, nmode=2, a=.8, d=2, s=.7, r=1.6, level=.12)],
        {"f_type": 0, "f_cut": 5000, "lfo_rate": .12, "lfo_pan": .25},
        fx(HALL, eq_lc=150, width=1.7), P)
    lib("Dembow 808 Duro", DD, "Bass", G + ["Aggressive"], "Hard, distorted 808 with a short tail and a big knock: aggressive and club-ready.",
        "Hard dembow lines on the accents.", "dd_bass",
        [L(1, wave=0, a=.001, d=.9, s=.3, r=.1), L(1, wave=1, oct=1, a=.001, d=.12, s=0, r=.08, level=.2)],
        {"mono": 1, "glide": .04, "f_type": -1, "p_env": 8, "p_dec": .02},
        fx(eq_lc=28, sat_drive=.7, sat_mix=.6), B)
    lib("Bajo Calle", DD, "Bass", G + ["Street"], "Gritty mid-range bass that reads on small speakers: street and punchy.",
        "Bass riffs that need to cut through.", "dd_bass",
        [L(1, wave=2, uni=2, det=6, a=.001, d=.3, s=.4, r=.08), L(1, wave=0, oct=-1, a=.001, d=.3, s=.6, r=.08, level=.6, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 1100, "f_res": .3, "f_env": 1.5, "fa": .001, "fd": .1, "fs": .3, "f_drive": 1.8},
        fx(eq_lc=30, sat_drive=.4, sat_mix=.5), B)
    lib("Sub Malecón", DD, "Bass", G + ["Melodic"], "Deep, clean, round sub for the melodic side of dembow.",
        "Long low notes under chords.", "dd_bass",
        [L(1, wave=0, a=.003, d=1.5, s=.85, r=.15)],
        {"mono": 1, "glide": .05, "f_type": -1},
        fx(eq_lc=25, eq_hc=2500), B)
    lib("Bounce Bass", DD, "Bass", G + ["Playful"], "Playful bass with a pitch bounce on every note: fun, light and catchy.",
        "Bouncy patterns for party dembow.", "dd_bass",
        [L(1, wave=1, a=.001, d=.35, s=.2, r=.08), L(1, wave=0, oct=-1, a=.001, d=.35, s=.4, r=.08, level=.6, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 900, "p_env": 3, "p_dec": .04},
        fx(eq_lc=30), B)
    lib("Dembow Reese", DD, "Bass", G + ["Underground"], "Dark detuned reese for underground dembow: wide, moving and menacing.",
        "Sustained bass in dark sections.", "dd_bass",
        [L(1, wave=2, uni=3, det=22, spread=.5, a=.004, d=1, s=.9, r=.15), L(1, wave=0, oct=-1, a=.004, d=1, s=.9, r=.15, level=.6, filt=0)],
        {"mono": 1, "f_type": 0, "f_cut": 900, "f_res": .25, "f_poles": 4, "lfo_rate": .5, "lfo_cut": .5},
        fx(eq_lc=30, sat_drive=.45, sat_mix=.5, width=1.25), B)
    lib("Cuerdas Fiesta", DD, "Strings", G + ["Party"], "Bright staccato string stabs that bounce on the offbeats.",
        "Party stabs and fills.", "dd_strings",
        [L(1, wave=2, uni=6, det=10, spread=1, a=.004, d=.25, s=.1, r=.12)],
        {"f_type": 0, "f_cut": 6500, "f_env": 1, "fa": .001, "fd": .08, "fs": .5},
        fx(ROOM, eq_lc=200, width=1.5), S)
    lib("Cuerdas Románticas", DD, "Strings", G + ["Romantic"], "Lush, sweet strings for romantic dembow.",
        "Held chords and slow melodies.", "dd_strings",
        [L(1, wave=2, uni=7, det=11, spread=1, a=.4, d=2, s=.9, r=1.2)],
        {"f_type": 0, "f_cut": 4000, "vib_rate": 5.2, "vib_depth": 8, "vib_delay": .5},
        fx(HALL, CHORUS, eq_lc=130, width=1.5), S)
    lib("Pizz Tíguere", DD, "Strings", G + ["Playful"], "Playful pizzicato, bright and bouncy, with a short room.",
        "Cheeky riffs and pickups.", "dd_strings",
        [L(3, bright=.6, damp=.3, decay=.5, pick=.35, strings=2, sdet=4, a=.001, d=.6, s=0, r=.15)],
        {"f_type": 0, "f_cut": 6500},
        fx(ROOM, eq_lc=150, width=1.35), S)
    lib("Drama Callejero", DD, "Strings", G + ["Underground"], "Dark, low strings with a hard attack: tension in the street.",
        "Intros and dark build-ups.", "dd_strings",
        [L(1, wave=2, uni=6, det=12, spread=1, a=.02, d=.6, s=.5, r=.3)],
        {"f_type": 0, "f_cut": 1800, "f_res": .15, "f_env": .8, "fa": .001, "fd": .2, "fs": .6},
        fx(HALL, eq_lc=60, sat_drive=.2, sat_mix=.3, width=1.4), (28, 76))
    lib("Violín Merengue", DD, "Strings", G + ["Merengue"], "Bright, quick violin lead with a tight vibrato, a nod to merengue fiddle lines.",
        "Fast melodic runs and hooks.", "dd_strings",
        [L(1, wave=2, a=.03, d=.8, s=.85, r=.2), L(6, vowel=2, vmod=.1, breath=.15, a=.04, d=.8, s=.6, r=.2, level=.25)],
        {"mono": 1, "glide": .03, "f_type": 0, "f_cut": 7000, "vib_rate": 6.5, "vib_depth": 12, "vib_delay": .2},
        fx(ROOM, eq_lc=250, width=1.2), (55, 96))

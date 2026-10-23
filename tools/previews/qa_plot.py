"""Spectrogram + waveform + click markers for a rendered preview (visual QA)."""
import sys, os, numpy as np, matplotlib
matplotlib.use("Agg"); import matplotlib.pyplot as plt
sys.path.insert(0, os.path.dirname(__file__))
from build_previews import read_wav
def clicks(mono, sr):
    d2 = np.abs(np.diff(mono, 2)); w = int(sr * .01)
    rms = np.sqrt(np.convolve(d2 ** 2, np.ones(w) / w, mode="same")) + 1e-9
    return np.nonzero((d2 > rms * 9) & (d2 > 2e-3))[0] / sr
out = sys.argv[1]; names = sys.argv[2:]
fig, axs = plt.subplots(len(names), 2, figsize=(16, 3.2*len(names)), squeeze=False)
for i, n in enumerate(names):
    x, sr = read_wav(f"build/tmp/{n}.raw.wav"); m = x.mean(1)
    axs[i,0].specgram(m, NFFT=2048, Fs=sr, noverlap=1536, cmap="magma", vmin=-120); axs[i,0].set_ylim(0, 20000); axs[i,0].set_title(n)
    t = np.arange(len(m))/sr; axs[i,1].plot(t, x[:,0], lw=.3); axs[i,1].plot(t, x[:,1], lw=.3, alpha=.6)
    for c in clicks(m, sr): axs[i,1].axvline(c, color="r", lw=.5, alpha=.5)
plt.tight_layout(); plt.savefig(out, dpi=60)

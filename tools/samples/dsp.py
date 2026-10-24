"""Small DSP toolkit for LUMELODY's sample libraries (original synthesis, numpy/scipy only)."""
import numpy as np
from scipy.signal import butter, sosfilt, fftconvolve

SR = 44100
_rng = np.random.default_rng(2026)

class Sig(np.ndarray):
    """audio array whose + and - pad the shorter signal with silence (layers of different lengths just mix)."""
    def __array_ufunc__(self, ufunc, method, *inputs, **kw):
        if ufunc in (np.add, np.subtract) and method == "__call__" and len(inputs) == 2:
            a, b = (np.asarray(x) if isinstance(x, np.ndarray) else x for x in inputs)
            if isinstance(a, np.ndarray) and isinstance(b, np.ndarray) and a.ndim >= 1 and b.ndim >= 1 and a.shape[0] != b.shape[0]:
                n = max(a.shape[0], b.shape[0])
                pad = lambda x: np.concatenate([x, np.zeros((n - x.shape[0],) + x.shape[1:])]) if x.shape[0] < n else x
                a, b = pad(a), pad(b)
            return ufunc(a, b, **{k: v for k, v in kw.items() if k != "out"}).view(Sig)
        args = [np.asarray(x) if isinstance(x, Sig) else x for x in inputs]
        if "out" in kw: kw["out"] = tuple(np.asarray(o) if isinstance(o, Sig) else o for o in kw["out"])
        r = getattr(ufunc, method)(*args, **kw)
        return r.view(Sig) if isinstance(r, np.ndarray) and not isinstance(r, Sig) else r

def sig(x):
    return np.asarray(x, dtype=float).view(Sig)

def rng(seed):
    return np.random.default_rng(seed)

def t(secs):
    return np.arange(int(secs * SR)) / SR

def env_ad(n, a, d, curve=1.0):
    """attack (s) then exponential decay with time constant d (s)."""
    x = np.arange(n) / SR
    att = np.clip(x / max(a, 1e-5), 0, 1) ** curve
    return sig(att * np.exp(-np.maximum(0, x - a) / max(d, 1e-5)))

def env_adsr(n, a, d, s, r, hold):
    x = np.arange(n) / SR
    e = np.where(x < a, x / max(a, 1e-5), s + (1 - s) * np.exp(-(x - a) / max(d, 1e-5)))
    rel = x > hold
    e[rel] *= np.exp(-(x[rel] - hold) / max(r, 1e-5))
    return e

def lp(x, f, order=2):
    return sig(sosfilt(butter(order, min(f, SR * .45), "low", fs=SR, output="sos"), x, axis=0))

def hp(x, f, order=2):
    return sig(sosfilt(butter(order, max(f, 10), "high", fs=SR, output="sos"), x, axis=0))

def bp(x, lo, hi, order=2):
    return sig(sosfilt(butter(order, [max(lo, 10), min(hi, SR * .45)], "band", fs=SR, output="sos"), x, axis=0))

def sweep_sine(n, f0, f1, tau):
    """sine whose frequency glides from f0 to f1 with time constant tau (s)."""
    x = np.arange(n) / SR
    f = f1 + (f0 - f1) * np.exp(-x / max(tau, 1e-5))
    return sig(np.sin(2 * np.pi * np.cumsum(f) / SR))

def modal(n, f0, ratios, amps, decays, r=None, detune=0.0):
    """sum of exponentially decaying partials (membranes, wood, metal)."""
    x = np.arange(n) / SR
    out = np.zeros(n)
    rr = r or _rng
    for k, (ra, am, de) in enumerate(zip(ratios, amps, decays)):
        f = f0 * ra * (1 + detune * rr.uniform(-1, 1))
        out += am * np.sin(2 * np.pi * f * x + rr.uniform(0, 6.28)) * np.exp(-x / de)
    return sig(out)

def noise(n, seed=0):
    return sig(rng(seed).standard_normal(n))

def sat(x, drive):
    return sig(np.tanh(x * drive) / np.tanh(drive))

def stereo(x, width=0.0, seed=0, delay_ms=0.0):
    """mono -> stereo with optional decorrelation (tiny delay + independent filtering)."""
    if x.ndim == 2: return x
    if width <= 0 and delay_ms <= 0: return np.stack([x, x], 1)
    d = int(delay_ms * SR / 1000)
    R = np.concatenate([np.zeros(d), x[:len(x) - d]]) if d else x.copy()
    L = x
    if width > 0:
        m, s = (L + R) / 2, (L - R) / 2
        L, R = m + s * (1 + width), m - s * (1 + width)
    return np.stack([L, R], 1)

def reverb(x, secs=1.2, mix=.25, seed=3, damp=6000, pre_ms=10):
    """convolution with a synthesized, decorrelated stereo tail."""
    if x.ndim == 1: x = np.stack([x, x], 1)
    n = int(secs * SR)
    ir = np.zeros((n, 2))
    for c in range(2):
        e = noise(n, seed + c) * np.exp(-np.arange(n) / (secs * SR / 6.9))
        ir[:, c] = lp(e, damp)
    pre = int(pre_ms * SR / 1000)
    ir = np.concatenate([np.zeros((pre, 2)), ir])
    ir /= np.sqrt((ir ** 2).sum(axis=0, keepdims=True)) + 1e-9
    wet = np.stack([fftconvolve(x[:, c], ir[:, c])[: len(x) + len(ir)] for c in range(2)], 1)
    dry = np.vstack([x, np.zeros((len(wet) - len(x), 2))])
    return dry * (1 - mix * .5) + wet * mix

def fade(x, fin=.002, fout=.01):
    n = len(x)
    a, b = int(fin * SR), int(fout * SR)
    w = np.ones(n)
    if a: w[:a] = np.linspace(0, 1, a) ** 2
    if b: w[n - b:] *= np.linspace(1, 0, b) ** 2
    return x * (w[:, None] if x.ndim == 2 else w)

def trim_tail(x, thresh_db=-70, pad=.03):
    m = np.abs(x).max(axis=1) if x.ndim == 2 else np.abs(x)
    idx = np.nonzero(m > 10 ** (thresh_db / 20))[0]
    if not len(idx): return x
    return x[: min(len(x), idx[-1] + int(pad * SR))]

def place(out, x, at, gain=1.0):
    i = int(round(at * SR))
    if i >= len(out): return
    m = min(len(x), len(out) - i)
    out[i:i + m] += x[:m] * gain

def finish(x, peak_db=-1.0, rms_db=None, dc=True, tail=True, fi=.0015, fo=.015):
    """clean, headroom-safe output: DC removed, trimmed, faded, normalised to a peak (or an RMS for beds)."""
    if x.ndim == 1: x = np.stack([x, x], 1)
    if dc: x = hp(x, 18, 1)
    if tail: x = trim_tail(x)
    x = fade(x, fi, fo)
    if rms_db is not None:
        r = np.sqrt(np.mean(x ** 2)) + 1e-12
        x = x * (10 ** (rms_db / 20) / r)
    pk = np.abs(x).max() + 1e-12
    lim = 10 ** (peak_db / 20)
    if rms_db is None or pk > lim:
        x = x * (lim / pk)
    return x.astype(np.float32)

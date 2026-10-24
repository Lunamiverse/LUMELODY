// ================================================================ LUMI_FEEL: suggestions, milestones, focus mode (inside existing elements only)
// Suggestions appear in the tip line under the deck; tapping one opens where it points (nothing is replaced without a
// tap, and every place it opens has its own undo / compare). Milestones are a one-time toast, optional. No streaks, no
// random rewards, no notifications outside the window.
const LUMI_FEEL = (() => {
  const prefs = () => (typeof FINDER !== "undefined" ? FINDER.prefs : { focus: false, milestones: true });
  let current = null, defaultTip = null;
  function suggest(text, act) {
    const t = $("#tip"); if (!t) return;
    if (defaultTip === null) defaultTip = [...t.childNodes].map(n => n.cloneNode(true));
    setTip("try", text); t.classList.add("sugg"); t.title = act ? "tap to try it" : ""; current = act || null;
  }
  function clear() {
    const t = $("#tip"); if (!t || !t.classList.contains("sugg")) return;
    t.classList.remove("sugg"); t.title = ""; current = null; if (defaultTip) t.replaceChildren(...defaultTip.map(n => n.cloneNode(true)));
  }
  // what goes with the sound you just picked: real items from the libraries, matched by library, tempo and role
  function afterPreset(p) {
    if (!p || typeof LIBS === "undefined") return;
    const M = LIBS.manifest, lib = M.libraries.includes(p.cat) ? p.cat : ({ Trippy: "Reggaetón", Bass: "Dembow Dominicano", Keys: "New Jazz", "Pads & Atmospheres": "New Jazz", "Synths & Leads": "Disco + EDM + Club" })[p.cat] || null;
    if (!lib) return;
    const pats = M.items.filter(i => i.lib === lib && i.kind === "pattern" && !i.hidden);
    if (!pats.length) return;
    const pick = pats[(p.id.length * 7) % pats.length];
    suggest(`${p.name} with the ${pick.name.toLowerCase()} (${pick.bpm} bpm) from ${lib === "Disco + EDM + Club" ? "disco + edm + club bangers" : lib.toLowerCase()}`, () => {
      LIBS.open(true); LIBS.setLib(lib); const c = document.querySelector(`#libCats [data-cat="${pick.cat}"]`); if (c) c.click();
      const card = document.querySelector(`#libItems .lcard[data-id="${pick.id}"]`); if (card) { card.scrollIntoView({ block: "nearest" }); card.classList.add("hint"); setTimeout(() => card.classList.remove("hint"), 1600); }
    });
  }
  const ARP_FOR = { rnb: "rnb", trap: "trap", hiphop: "trap", reggaeton: "reggaeton", dembow: "dembow", pop: "club", disco: "disco", club: "club", cinematic: "ambient", experimental: "jazz" };
  function afterChords(style) {
    const g = ARP_FOR[style] || "rnb";
    suggest(`hear these chords as a ${ARP.GENRES[g].name.toLowerCase()} arpeggio`, () => {
      WORKSPACE.open(true); WORKSPACE.tab("arp"); ARP.state.source = "blocks"; ARP.genre(g, { keepRhythm: true, keepNotes: true, keepSound: true }); ARP.play(true);
    });
  }
  function afterSample(name) {
    suggest(`layer ${name} with the piano: the keys play both`, () => { SAMPLER.open(true); SAMPLER.setLayer(2); });
  }
  // ---------------------------------------------------------------- milestones: once each, optional
  const seen = store.get("lumi-milestones-v1", {});
  const TEXT = { exported: "Your first idea is out of LUMELODY. Drop it in the playlist and keep going.", recorded: "First loop recorded. Overdub on top, or undo anytime.",
                 imported: "Your first sample is in. Try it on the keys or as slices on the pads.", groove: "Two patterns, one groove. Drag it out as MIDI or audio." };
  function milestone(k) {
    if (seen[k] || !prefs().milestones) return;
    seen[k] = Date.now(); store.set("lumi-milestones-v1", seen); setTimeout(() => toast(TEXT[k]), 2600);   // after the action's own message
  }
  function applyFocus() { document.documentElement.classList.toggle("focus", !!prefs().focus); }
  function init() {
    const t = $("#tip"); if (t) t.addEventListener("click", () => { const a = current; if (a) { clear(); a(); } });
    window.addEventListener("lumi:milestone", e => milestone(e.detail));
    applyFocus();
  }
  return { init, suggest, clear, afterPreset, afterChords, afterSample, milestone, applyFocus };
})();

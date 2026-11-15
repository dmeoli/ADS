// Render math delimited by $...$ and $$...$$ once each page (and each
// instant-navigation swap) has loaded, using the KaTeX auto-render
// extension. Paired with pymdownx.arithmatex in generic mode. The captions
// of the figures, whose math arithmatex has already set within \( \), are
// rendered apart with those delimiters only, so that a $ of their text
// (abracadabra$) is not taken as one [see figcaption.js].
document$.subscribe(() => {
  renderMathInElement(document.body, {
    delimiters: [
      { left: "$$", right: "$$", display: true },
      { left: "$", right: "$", display: false },
      { left: "\\(", right: "\\)", display: false },
      { left: "\\[", right: "\\]", display: true }
    ],
    ignoredTags: ["script", "noscript", "style", "textarea", "pre", "code",
                  "option", "figcaption"],
    throwOnError: false
  });
  document.querySelectorAll("figcaption").forEach((cap) => {
    renderMathInElement(cap, {
      delimiters: [{ left: "\\(", right: "\\)", display: false }],
      throwOnError: false
    });
  });
});

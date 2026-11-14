// Show the caption of each figure on the site. A figure of the Markdown is
// `![Figure N.M. caption](../figures/x.svg){#fig-N-M}`: Pandoc makes it a
// LaTeX figure with that caption, while MkDocs leaves the caption only in
// the alt text of the image. Here each image whose id starts with "fig-" is
// wrapped in a <figure> with the alt text as its <figcaption>, before
// katex-init.js (loaded after this file) typesets the math in it.
document$.subscribe(() => {
  document.querySelectorAll('img[id^="fig-"]').forEach((img) => {
    if (img.closest("figure")) return;
    const fig = document.createElement("figure");
    const cap = document.createElement("figcaption");
    cap.textContent = img.getAttribute("alt") || "";
    img.parentNode.insertBefore(fig, img);
    fig.appendChild(img);
    fig.appendChild(cap);
  });
});

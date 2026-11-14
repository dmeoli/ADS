# ADS User Manual — Maintenance Notes

*Internal document for whoever maintains this manual. It is excluded from
the MkDocs site and from the PDF build.*

---

## 1. What this is

A manual for ADS, the header-only C++20 library of algorithms and data
structures, written area by area (the plan is in `OUTLINE.md`). Single
source = Markdown under `manual/chapters/`, built to **both** a PDF
(Pandoc + LaTeX) and a web site (MkDocs Material), with the same
pipeline and the same conventions as the user manual of SMS++, so that
the two read as one family.

---

## 2. Authoring conventions

- **Language:** scientific English, in the style of the papers of the
  group (long sentences built by subordination, hedges on the exact point
  of doubt, no marketing words, no bold for emphasis in the prose). Check
  a chapter on the PDF with the style checker before committing it.
- **Section numbering** is written by hand in the headings (`# 3. Stacks
  {#ch-3}`, `## 3.2 ... {#sec-3-2}`); LaTeX numbering is suppressed in
  `build/preamble.tex`. Number new sections by hand, without gaps.
- **Labelled items** carry an anchor, `<span id="..."></span>`, before
  their bold label, and are numbered by chapter: `lem-3-1` (Lemma 3.1),
  `alg-3-1`, `tab-3-1`, `rem-3-1`. A figure instead takes its id on the
  image, in a paragraph of its own, `![Figure 3.1. caption](../figures/x.svg){#fig-3-1}`:
  a span in the same paragraph would stop Pandoc from making a figure, and
  the PDF would lose the caption. Equations are numbered with
  `\tag{3.1}` inside the display and cited as "(3.1)".
- **Cross-references** are Markdown links to `FILE.md#id`, from a chapter
  to a sibling file (`[§3.2](03-stack.md#sec-3-2)`); `build/crossref-link.lua`
  turns them into `\hyperref` in the PDF.
- **Citations** link to the anchor of the entry in
  `chapters/Z-bibliography.md`, as `[Morin 2013, §2.1](Z-bibliography.md#ref-morin2013)`;
  every entry has its DOI when it has one, and the entries are those of
  the `\see` lines of the headers.
- **Pseudocode and code are never copied into a chapter.** They are
  included from `generated/` with a line `--8<-- "generated/<file>"`
  (alone in a paragraph for pseudocode, alone in a fenced `cpp` block for
  code), which MkDocs expands with `pymdownx.snippets` and Pandoc with
  `build/snippets.lua`. `build/snarf.py` writes them from the headers:
  `<File>-<name>.pseudo.md` from each `\class{pseudocode}` block of the
  comments, `<File>-<name>.cpp` from the code between `//! [name]` markers
  of a `.h` or `.tpp`. To change a pseudocode or a listing, change the
  header.
- **Lemmas and theorems** are paragraphs: `**Lemma 3.1.** *statement*`,
  then `*Proof.* ... $\square$`.
- **Exercises** close each chapter, each one naming the ctest that solves
  it; a name that does not exist in `test/` is an error.
- **Status admonitions** as defined in `index.md` (four forms).
- **Math delimiters:** `$...$` and `$$...$$` only; KaTeX renders the site,
  so no `@{}` in array column specifications (`snarf.py` rewrites the
  `r@{\quad}l` of the headers as `rl`).

---

## 3. Source of truth

The headers of the library, their theory pages in `docs/theory`, and the
tests. Every claim of a chapter on a cost, a bound or a behavior is
checked against the code before it is written; every number of a
benchmark table comes from a file of results of a campaign on an idle
machine, cited in the caption.

---

## 4. Layout

```
manual/
  OUTLINE.md          # the plan: one chapter per area
  MAINTENANCE.md      # this file
  index.md            # front matter (title, abstract, conventions, ToC)
  ADS-manual.pdf      # built PDF (commit it)
  chapters/           # NN-<area>.md, A-.. appendices, Z-bibliography.md
  figures/            # <name>.tex (TikZ, standalone) + .pdf + .svg
    tikz-ads.tex      # the styles shared by the figures of the memory
  generated/          # written by snarf.py, not committed
  build/
    Makefile  pandoc.yaml  preamble.tex  mkdocs.yml  snarf.py
    snippets.lua  figure-ext.lua  crossref-link.lua  break-code.lua
    katex-init.js  figcaption.js   # the second shows the captions on the site
```

---

## 5. Building locally

From `manual/build/`:

```sh
make pdf       # -> ../ADS-manual.pdf    (Pandoc + pdflatex)
make html      # -> ../../manual-site/   (MkDocs Material)
make serve     # live preview at 127.0.0.1:8000
make figures   # rebuild the .pdf and .svg of each changed figure
make snippets  # rerun snarf.py on ../../include
```

PDF acceptance check: zero `Overfull \hbox` and zero undefined
references. `pandoc.yaml` lists the input files in reading order: keep it
in sync with the `nav` of `mkdocs.yml` and the table of contents of
`index.md`. `build/preamble.tex` differs from that of SMS++ in one point:
figures keep their natural size, since the TikZ figures are drawn at it.

---

## 6. Adding or updating a figure

1. Write `figures/<chapter>-<name>.tex` as a `standalone` TikZ file that
   loads `lmodern`, `tikz` and `lamalunga` and then `\input`s `tikz-ads`
   (see `stack-array.tex`). The style is flat, one colour per role, with
   the palette that lamalunga computes from the primary (`7030A0`): what
   the operation reads or follows is in the primary (`touched`,
   `node-touched`, `vertex-touched`, `edge-touched`), what it adds in the
   first colour of its triad (`added`, `node-added`, `vertex-added`), the
   rest in golden tints of the primary (`cell`, `node`, `vertex`, `edge`,
   and `free` for a free place, the only shape with a hairline of ink, so
   that an empty place shows on the white page); no outlines, gradients or shadows, and no
   colour outside the palette. The Makefile finds lamalunga in
   `~/GDrive/lamalunga` (variable `LAMALUNGA`).
2. `make figures` writes the `.pdf` and the `.svg`.
3. Reference it as `![Figure N.M. caption](../figures/<name>.svg){#fig-N-M}`,
   a paragraph of its own (see section 2).
4. Commit the `.tex`, the `.pdf` and the `.svg`.

---

## 7. Deployment

`.github/workflows/docs.yml` builds the site of the manual into the folder
`manual` of the pages of the repository, next to the Sphinx and Doxygen
sites; `.github/workflows/release.yml` attaches `manual/ADS-manual.pdf`
to each release. The root `.gitignore` excludes `build/`, with the
exception `!manual/build/` (without it the build files are not tracked).

---

## 8. Updating for a new ADS version

1. Bump the version in `index.md`, in the subtitle of `build/pandoc.yaml`
   and in the `site_description` of `build/mkdocs.yml`.
2. Rebuild both outputs; check that every `--8<--` still resolves
   (`check_paths` stops MkDocs, `snippets.lua` stops Pandoc) and that the
   ctest names of the exercises still exist.
3. Revisit each Status admonition.
4. Commit the refreshed `ADS-manual.pdf`.

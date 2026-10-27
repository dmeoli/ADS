# The documentation of an area: theory page and pseudocode

This guide is for whoever documents an area of the library, i.e., a folder
of `include/ADS` (the graph area, `include/ADS/graph`, is the finished
model to imitate). An area is documented when a reader who has not got the
books can understand it from the two sites alone: the area has a theory
page with the definitions, the notation, the invariants and the arguments
behind the costs, and every algorithm of its headers has its pseudocode in
the comment of its function. The sites are built by
`.github/workflows/docs.yml` (see the head of `docs/Doxyfile` for the
steps), and nothing below needs a change of `docs/apiref.py`: a theory page
dropped in `docs/theory` is found and linked by itself.

## 1. The theory page

### Where it goes

One file per area, `docs/theory/<folder>.dox`, e.g., `docs/theory/sort.dox`
for `include/ADS/sort` and `docs/theory/ADS.dox` for the headers of
`include/ADS` itself. The directory is not installed with the headers. The
file holds one comment of Doxygen that defines a group:

```
/*--------------------------------------------------------------------------*/
/*----------------------------- File sort.dox ------------------------------*/
/*--------------------------------------------------------------------------*/
/** \addtogroup area_sort */
/** \defgroup theory_sort Theory of sorting
 \ingroup area_sort

 \details The headers of `include/ADS/sort` realize ...

*/
/*--------------------------------------------------------------------------*/
/*--------------------------- End File sort.dox ----------------------------*/
/*--------------------------------------------------------------------------*/
```

- The name of the group is `theory_<folder>`, and it goes `\ingroup
  area_<folder>`, the group of the area that `docs/apiref.py` writes; the
  boxes are those of the headers, 78 columns, with the name of the file.
- The line `/** \addtogroup area_<folder> */` before it gives the group of
  the area to the first run of Doxygen, which reads the theory pages with
  the headers but not the groups that `docs/apiref.py` writes from its
  XML; without it that run warns of an `\ingroup` to a group that does not
  exist.
- The title is "Theory of" followed by the title of the area in lowercase,
  as `NAMES` of `docs/apiref.py` gives it ("Theory of priority queues",
  "Theory of sequences and lists"), and the title of the area for
  `include/ADS` itself is "Foundations" ("Theory of the foundations").
- The text opens with `\details`, so that the group has no brief
  description: Doxygen prints the brief of a group above its text and
  again as the first paragraph of it, and a theory page has no members
  whose table would stand between the two.
- A group, not a `\page`: Doxygen puts a group first among the subgroups of
  the area and gives it a page of its own, while a `\page` in a group
  becomes a section of the page of the group, after the headers.

With the file in place, the build does the rest: the Topics of the Doxygen
site list the page first under the area, the group of each header of the
area links to it, and on the Sphinx site the page is the first one of the
area (`api/<folder>/theory`), linked from the index of the area and from the
page of each of its headers.

### What it says

Model: `docs/theory/graph.dox`. The sections, each with `\section
theory_<folder>_<name> <Title in sentence case>`, are:

1. an opening paragraph, before `\tableofcontents`: what the headers of the
   area realize, which chapters of the book they follow, and what the page
   collects, with `\ref` to its sections;
2. "Definitions and notation": the objects and the symbols shared by the
   headers, with an HTML table of the symbols (`<caption>`, then rows of
   `<th>` and `<td>`), each symbol in `\f$ ... \f$`; a caption may hold
   formulae and code, which the Sphinx site shows as text, since a caption
   of Breathe holds text only [see `caption_text()` in `docs/conf.py`];
3. for an area of data structures, the realizations and the costs of their
   operations, as a table, followed by the reason of each cost that is not
   obvious and by the case in which each realization is preferable;
4. one section per family of algorithms: the property on which the
   correctness rests, as a statement, and the argument behind each bound
   (the argument, not the bound alone: why each node enters the queue once,
   why each node leaves it at most n times, and so on); the variants and
   how they compare, as a table when there are more than two;
5. "References": a `\see` per source, as in the headers, with the doi
   whenever the source has one (checked, never guessed), and the chapters
   of the book with the link to their PDF.

The statements are paragraphs that open with their label in italics,
numbered on the page, and the argument is the paragraph after them:

```
 <em>Lemma 1 (the visit in breadth).</em> With \f$ Q \f$ a queue, ...,
 one has at the end
 \f[
   d[ v ] = \delta( r , v ) \quad \forall v \in V . \tag{1}
 \f]

 Indeed, the values of \f$ d \f$ in the queue never decrease ...
```

Do not use `\par` for them: Breathe moves the paragraphs of a `\par` after
the other paragraphs of the description, out of their place on the Sphinx
site.

The equations are numbered with `\tag{k}`, in the order of the page, and
cited as "(k)" in the text, never as "Eq." or "Equation". A display with
`\tag` must not contain `\\`: Sphinx wraps such a display in a `split`
environment, where MathJax refuses the tag. The rows of an array end with
`\cr` instead, in every display (see (2) of `graph.dox`).

The prose follows the style of the rest of ADS: impersonal (no "we", no
"I"), sentences long by subordination rather than by tails added with
commas, the explanation after a colon and the consequence after "; hence",
the asides and the pointers in parentheses, no em dash, no bold in the
text, no word of hype (key, novel, powerful, seamless, and the like), and
the limits of each bound stated where the bound is. American spelling, the
Latin plurals (formulae, minima), `i.e.,` and `e.g.,` with the comma.

### Language, references, code and spacing

These rules hold for the theory pages and for every comment of the headers,
and they are the points that a review of the graph area found; each is
checked by rereading the page as rendered, not the source.

- English only. The numbered parts of the book are named in English, a
  theorem with its capital as a name and the rest in lowercase within a
  sentence: "Theorem 9.1 of the book", "by Theorem 14.1" (no article),
  "Lemma 10.2", "exercise 4.6", "figure 6.4", "table 19.1", "section 9.5",
  "chapter 11"; never Teorema, Esercizio, Figura, Tabella, Capitolo,
  Sezione or Conclusione. The titles of the cited works stay as published
  (Algoritmi e Strutture Dati, Esercizi Greedy), and so do the names in the
  code; the problems that the book names in capitals in Italian (CRICCA,
  ZAINO, COMMESSO VIAGGIATORE, ...) are named in English in the prose
  (CLIQUE, KNAPSACK, TRAVELING SALESMAN, as Garey and Johnson name them),
  while the abstract data types keep the English capitals of the library
  (GRAPH, SET, QUEUE). The procedures of the book are called by the name of
  their C++ function (`shortestPaths()`, not camminiMinimi()).
- A cross-reference reads as a reference, not as a noun of the sentence:
  `\ref` renders as the title of the section, so "the representations
  (see \ref theory_graph_repr)", "(from \ref theory_graph_visits to \ref
  theory_graph_euler)", "as \ref theory_graph_sp shows", never "the
  representations of \ref theory_graph_repr" or a bare "(\ref ...)".
  Reread every link of the page as it renders.
- The names of the code are in code font on both sites: functions with
  their parentheses (`distance()`, `hasCycle()`), classes and concepts
  (`DfsSchema`, `FlowNetwork`, `Graph`), headers and paths (`Visit.h`,
  `include/ADS/graph`), enumerators, types and constants (`DINIC`, `W`,
  `INF`, `NIL`), and calls with their arguments (`adj( u )`,
  `forEachAdj( u , f )`). The mechanism is the backquote of Markdown in
  the prose of the comments and of the `.dox` files, and `<tt>...</tt>`
  inside the cells of an HTML table; a code span never crosses a line,
  since the input filter of Doxygen ends it with its line. The abstract
  data types of the book in capitals (GRAPH, MFSET) are not code. The
  symbols of the mathematics are not code either: a node is \f$ u \f$, not
  `u`, and the function \f$ \mathrm{adj}( u ) \f$ of a formula is
  `\mathrm{adj}` inside the formula.
- The spacing of the code (`( u , v )`, `f( x )`) is for the code and for
  the inside of the formulae; the parentheses of the prose have no space
  inside: "(\f$ 2 m \f$ for an unoriented graph)", "(see ...)". Doxygen
  writes a space before every inline formula, and the build drops it after
  an opening bracket [see `mend_xml()` in `docs/conf.py` and the `sed` of
  `.github/workflows/docs.yml`], so a formula right after "(" or "[" is
  written with no space, as above.
- The first sentence of a comment (its brief, which the tables of the API
  reference show up to its first colon) follows the same rules: every
  symbol and every formula in `\f$ ... \f$`, every name of the code in
  backquotes, e.g., "the distance in moves of each cell of the
  \f$ \mathit{rows} \times \mathit{cols} \f$ grid from the nearest
  source"; `docs/apiref.py` turns them into `:math:` and literals in the
  tables of Sphinx, and Doxygen renders them as everywhere else.
- The first sentence describes the function, and never opens with its own
  name ("`bfs()`: ..."), which both sites already print in front of it:
  "the visit in breadth from \f$ r \f$, calling `node( u )` when \f$ u \f$
  is examined and `edge( u , v )` when \f$ ( u , v ) \f$ is examined". An
  overload says what it adds to the other one: "the distances of
  `distance()` above, with the queue \f$ S \f$ given". The comment of a
  member, of a function and of a type opens in lowercase, that of a header
  with a capital, and the tables of both sites show it as it is written.
- The contracts (`\pre`, `\post`, `\invariant`) follow the rules of the
  prose: "\f$ r \f$ is a node", "\f$ n \ge 0 \f$", "\f$ |B| = |R| - 1 \f$,
  each edge of \f$ B \f$ from a node already in \f$ R \f$ (audit)"; an
  expression of the code stays code, "not `hasEdge( u , v )`" for
  `! hasEdge( u , v )`. The lines that continue a `\pre` are indented by
  six spaces after `///`, those of a `\post` by seven, under its text.
- Every enumerator, every field of a public type and every public member
  function has its comment, one line, `///<` after an enumerator or a field
  and `///` before a function: the tables of Doxygen show an empty cell for
  the ones that have none. The documented members with an empty
  description are found in the XML of Doxygen (a `memberdef` or an
  `enumvalue` with an empty `briefdescription` and `detaileddescription`).
- The constants, the sentinels and the enumerators named in the prose are
  code (`INF`, `NIL`, `NEWEST`, `DINIC`, `QUEUE` when it is the value of
  `Frontier`), while the abstract data types of the book in capitals
  (GRAPH, QUEUE, STACK) are not. A term is in italics where it is defined
  (`<em>sparse</em>`) and in roman afterwards; quotation marks are for
  quotations only, never for emphasis.

## 2. The pseudocode

Every function of a header that is an algorithm (not an accessor or a
constructor that only copies) has its pseudocode in its comment, after the
description and before `\pre` and `\post`, introduced by a paragraph that
says the source:

```
/// the visit in breadth from \f$ r \f$, calling `node( u )` when \f$ u \f$ is
/// examined and `edge( u , v )` when \f$ ( u , v ) \f$ is examined
///
/// In pseudocode (section 9.5):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{bfs}( G , r ) \cr
/// 2 & \quad Q \gets \text{an empty queue};\ \mathrm{enqueue}( Q , r );
///     \ \mathit{visited}[ r ] \gets \textbf{true} \cr
/// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathrm{dequeue}( Q );\ \mathrm{node}( u ) \cr
/// 5 & \qquad \textbf{foreach}\ v \in G.\mathrm{adj}( u )\ \textbf{do} \cr
/// 6 & \qquad\quad \mathrm{edge}( u , v ) \cr
/// 7 & \qquad\quad \textbf{if not}\ \mathit{visited}[ v ]\ \textbf{then}
///     \cr
/// 8 & \qquad\qquad \mathit{visited}[ v ] \gets \textbf{true};
///     \ \mathrm{enqueue}( Q , v )
/// \end{array}}
/// \f]
/// \pre \f$ r \f$ is a node
template< Graph G , class FN , class FE >
void bfs( const G & g , int r , FN && node , FE && edge );
```

It renders the same on both sites (MathJax 3 on both, on Sphinx through
Breathe), in the colours of the light and of the dark theme.
The rules:

- the whole pseudocode is one display `\f[ \class{pseudocode}{
  \begin{array}{r@{\quad}l} ... \end{array}} \f]`, one row per line, the
  number of the line in the first column, the rows separated by `\cr` (no
  `\tag` here); the class sets it on the left, a little indented, as a
  listing [see `docs/_static/ads.css` and `docs/doxygen/custom.css`, and
  the extension html of MathJax in `docs/Doxyfile`], while the other
  displays stay in the middle;
- the first line names the routine: `\textbf{function}` if it returns a
  value, `\textbf{procedure}` if not, and the name as `\mathrm{name}` with
  its arguments; a few routines (a recursive helper and the procedure it
  calls) may share one display, numbered on;
- indentation by nesting level: none, `\quad`, `\qquad`, `\qquad\quad`,
  `\qquad\qquad`, and so on;
- keywords in `\textbf{}`, followed by `\ ` (a space): function,
  procedure, if, then, else, else if, while, do, for, to, downto, step,
  foreach, with, loop, repeat, until, return, break, continue, and, or,
  not, true, false, nil, with no variants of them;
  `\textbf{foreach}\ v \in S\ \textbf{with}\ \ldots\ \textbf{do}` for a
  loop with a condition,
  `\textbf{for}\ i \gets n\ \textbf{downto}\ 1\ \textbf{do}` for a
  decreasing loop (never "down to"),
  `\textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{step}\ b\ \textbf{do}`
  for a loop by blocks, and `\textbf{break}` for the exit from the
  innermost loop;
- the assignment is `\gets` (←), comparisons `=`, `\ne`, `\le`, `\ge`;
  sets with `\{ \}`, `\in`, `\cup`, `\setminus`, `\emptyset`; infinity
  `\infty`;
- calls and the operations of the abstract data types in `\mathrm{}`
  (`\mathrm{enqueue}( Q , v )`, `G.\mathrm{adj}( u )`), variables of one
  letter as they are, of more letters in `\mathit{}` (`\mathit{visited}`);
- a subscript or a superscript that is a word, or the abbreviation of
  one, is upright, `v_{\mathrm{in}}`, `d_{\mathrm{max}}`, never `v_{in}`,
  which TeX reads as the product of \f$ i \f$ and \f$ n \f$; a subscript
  that is a variable stays italic (`x_i`, `d^k`);
- words in `\text{}` (`\text{an empty queue}`), with a space inside the
  braces on the side of a mathematical symbol (`\text{the nodes of } S`);
- a comment at the end of a line is `\quad \triangleright\ \text{...}`;
- more statements on one line are separated by `;\ `;
- a `\text{}` never runs across two comment lines: MathJax keeps inside it
  the spaces that indent the next line, which open a gap between two
  words; close it at the end of the line and open another on the next one
  (`\text{the }` and then `\text{newest for depth}`);
- the comment lines stay within 79 columns: a long row continues on the
  next comment line indented by four spaces (`///     `), which TeX reads
  as a space;
- no comment line ends with a backslash: in a comment `//` (and so in
  `///`) a backslash at the end of the line joins the next line to the
  comment, which g++ reports with `-Wcomment`. The rows end with `\cr`,
  never with `\\`, and a control space `\ ` that falls at the end of a
  line goes to the head of the next one (`///     \ \textbf{then}`);
  `docs/build.sh` fails on any such line;
- a statement that does not fit in the width of the page may continue on
  a row with no number, which opens with `&` and is indented one level
  more than the statement it continues (e.g., `& \qquad\quad \ldots` under
  a statement at `\qquad`); a statement that fits on its row is never
  split;
- the arrays of the C++ code become the arrays of the book, A[ 1 .. n ]
  or A[ 0 .. n - 1 ] as the header says, and the same names as the code
  where they exist.

The paragraph before the display says where the algorithm comes from: "In
pseudocode (section 11.2):" for the book, "In pseudocode (Kahn's algorithm,
beyond the book):" or "(Johnson's algorithm, beyond the book)" for a paper
already cited by a `\see` of the header, followed, when it helps, by the
property that makes it correct in one sentence. An overload that only
delegates gets a one-line pseudocode that calls the other one.

### Sources and copyright

The pseudocode of A. Montresor, Algoritmi e Strutture Dati
(algoritmicamente.it), is under CC BY-NC 4.0: it may be adapted, with the
section cited, and the convention above is such an adaptation (English
keywords, the names of the C++ code). The Art of Computer Programming,
Ferragina's Pearls of Algorithm Engineering and the papers are under
copyright: write the pseudocode of the algorithm in your own words, from its
description and from the code of ADS, cite the source in the paragraph and
by a `\see`, and never copy their text or their pseudocode.

P. Ferragina, Pearls of Algorithm Engineering, Cambridge University Press,
2023, doi:10.1017/9781009128933, is cited by the number and the title of
its chapter only ("chapter <n>, <title of the chapter>"), never by the
number of a theorem, a lemma, a section or a table inside it: those numbers
come from the lecture notes that preceded the book and cannot be checked on
the printed one. The lecture notes themselves are never cited.

### The other mathematics of the comments

Every symbol, every formula and every bound of a comment of a header is
LaTeX of Doxygen, in the first sentence as in the rest
(`\f$ O( n \log n ) \f$`, `\f$ \Theta( n + m ) \f$`,
`\f$ 2 n + o( n ) \f$`), never plain text such as "O( n log n )"; the
names of functions inside a formula go in `\mathrm{}`
(`\f$ O( |\mathrm{adj}( u )| ) \f$`). The accented words are written with
the accent (Università, più), never with an apostrophe.

These rules hold for every comment that Doxygen extracts (`///`, `/** */`,
`///<`), those of the private members and of the namespace `detail`
included: a site may not show them today, but the comment is written once
for whoever reads the header and for the sites alike. The comments `//`
inside the bodies of the functions, in the `.tpp` files and in the
headers, are shown by no site: they stay plain text and code, as the code
they explain is written (`// D[ i - 2 ], D[ i - 1 ]`), with no LaTeX and no
backquotes.

## 3. Checks before pushing

1. Build both sites with `bash docs/build.sh` (Doxygen 1.17, `DOXYGEN=`
   to name another binary), the script that the workflow runs, never with
   runs of Doxygen written by hand: its first run reads `docs/theory` with
   the headers, and one that does not leaves the `\ref` of the headers
   unresolved. Doxygen must print no warning (but "is not documented",
   which EXTRACT_ALL silences), and `sphinx-build` no `WARNING` and no
   `ParseWarning` of Breathe (a part of the XML that Breathe does not know,
   which it leaves out of the Sphinx site); the script prints them and
   fails if there is one. A crash of Breathe on the XML (e.g., "missing id
   attribute") stops the build with a traceback in its log, and is to be
   fixed, not silenced.
2. Lines of the headers, of the `.dox` and of the scripts within 79
   columns, the boxes aligned (run the box checker of the project, if you
   have it, on every file with a box).
3. Screenshots, light and dark, with headless Chrome, of the theory page and
   of one header with pseudocode on both sites (`--window-size=1600,9000`
   and a crop around the pseudocode):
   - Doxygen follows the colour scheme of the system:
     `--blink-settings=preferredColorScheme=0` gives the dark theme, `=1`
     the light one;
   - Sphinx loads its dark style only from the switch, which stores
     `theme` in `localStorage`: serve the site (`python3 -m http.server`)
     and open first a page of the same origin that sets
     `localStorage.setItem( "theme" , "dark" )` and then goes to the page to
     shoot; setting `data-theme` by hand does not load the dark rules and
     gives false alarms.
   - both sites typeset a formula only when it comes into view (the
     extension `ui/lazy` of MathJax), which headless Chrome never does:
     shoot a local copy of the site from which `ui/lazy` is removed (the
     `"loader": {"load": ["ui/lazy"]}` of the pages of Sphinx, the line
     `window.MathJax.loader.load.push( 'ui/lazy' );` of those of
     Doxygen), and check the published pages in a real browser, scrolled
     to the end, counting the `mjx-container` with no `mjx-math` inside
     (none must be left) and the `mjx-merror` (none).
   Read them and look for formulae that do not render (a red error of
   MathJax, raw `\f$` or `\textbf`), literal entities (`&#x2F;`, `&amp;`),
   text too dark or too light on its background, rows of a pseudocode out
   of order or misaligned, and pages wider than the content.
4. On the Doxygen site the functions of a header are documented on the page
   of its group in the Topics (`group__hdr__<folder>__<Header>.html`),
   where the pseudocode is: check it there, not on the page of the file.

A header refers to the theory page of its area, or to a section of it,
with `\ref`, as the page refers to its own sections, e.g., "(see \ref
theory_graph_visits)" in the comment of `Visit.h`: both runs of Doxygen
read `docs/theory` with the headers, and the reference is a link on both
sites. An equation of the page is cited by its number and the page, "(4)
of \ref theory_graph_sp".

A trap of Doxygen that the build does not report clearly:

- A name of the book written as a function in the prose, e.g., "(the
  bbTsp() of the book)", is linked by Doxygen to the function of ADS with
  the same name, which may be another thing: write it `%bbTsp()`, which
  Doxygen does not link, or with no parentheses ("the procedure bbTsp of
  the book"). The names of ADS itself go in backquotes, as above.

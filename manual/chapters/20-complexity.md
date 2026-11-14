# 20. Complexity and intractable problems {#ch-20}

[Source: `include/ADS/complexity/Problems.h`, `Nondeterminism.h`,
`Domino.h`, `Reductions.h`, `EasyCases.h`;
`include/ADS/intractable/Pseudopolynomial.h`, `Approximation.h`,
`BranchAndBound.h`, `Heuristics.h`, `FifteenPuzzle.h`]

We are concerned in this chapter with the problems for which no algorithm
of polynomial time is known, and with what one can do about them. The
headers of `ADS/complexity` follow chapter 18 of [Montresor
2025](Z-bibliography.md#ref-montresor2025): they give each problem of the
class NP its certificate and an enumeration that decides it in exponential
time, simulate the nondeterministic algorithms, realize the polynomial
reductions that make a chain of NP-complete problems, and solve a few
problems close to them in polynomial time. Those of `ADS/intractable`
follow its chapter 19, i.e., the ways of facing a problem that is NP-hard,
each of which gives up one of three things (the time polynomial in the
size of the input, the optimality of the solution, or the generality of
the instances). We organize the chapter as follows.
Sections [20.1](20-complexity.md#sec-20-1) to [20.5](20-complexity.md#sec-20-5)
are devoted to the theory: the classes P and NP and the certificates, the
nondeterminism and its simulation, the enumerations, the reductions, and
the polynomial cases. Sections [20.6](20-complexity.md#sec-20-6) to
[20.10](20-complexity.md#sec-20-10) are devoted to the strategies, in the
order of the book: the pseudopolynomial algorithms, the approximations,
the branch-and-bound, the heuristics and the search of the fifteen puzzle.
Throughout the chapter $n$ is the number of the nodes, the cities, the
objects or the variables of an instance, and the words in small capitals
of the book are written in capitals (e.g., CLIQUE).

## 20.1 Problems, certificates and the classes P and NP {#sec-20-1}

A *decision problem* $\Pi$ is a set of instances, each with the answer YES
or NO, and $Y_\Pi$ is the set of the instances whose answer is YES. The
problems of optimization (the largest clique, the shortest tour) become
decision problems by a bound $k$ on the value (whether a clique of $k$
nodes exists), which is as hard up to a polynomial factor, since a binary
search on $k$ finds the optimum with a logarithmic number of questions.
As usual, the size $\lvert x \rvert$ of an instance is the length of a "reasonable"
encoding of it, with the numbers written in binary; hence a number $k$
counts $\Theta( \log k )$ instead of $k$, which is the distinction on which
the pseudopolynomial algorithms of [§20.6](20-complexity.md#sec-20-6)
rest. Then the class *P* holds the problems decided by an algorithm in time
$O( \lvert x \rvert^c )$ for a constant $c$; the class *NP* those that have
a polynomial *certificate*, i.e., a polynomial $p$ and an algorithm $V$ of
polynomial time such that

$$
x \in Y_\Pi \iff \exists \, y : \ \lvert y \rvert \le p( \lvert x \rvert )
\ \text{and} \ V( x , y ) = \text{YES}
\tag{20.1}
$$

([Montresor 2025, §18.1](Z-bibliography.md#ref-montresor2025)). The
functions `cliqueCertificate()`, `tspCertificate()`, `satCertificate()` of
`Problems.h` and `legalTiling()` of `Domino.h` are such algorithms $V$, and
so are the postconditions of the other enumerations of `Problems.h`, each
with a certificate of polynomial size (a set of nodes, a tour, an
assignment, a covering); hence each problem of the area is in NP. Also,
$\mathrm{P} \subseteq \mathrm{NP}$, since a polynomial algorithm is a
checker that ignores $y$; whether the inclusion is strict is, as is well
known, open.

We now fix the representations that the headers share. A graph is an `Adjacency`, i.e.,
a square matrix of `bool`, symmetric when the graph is unoriented, whose
diagonal (a loop of a node) each problem ignores: a node with a loop may
be in an independent set, need not be in a cover, and takes a color like
any other. A formula in conjunctive normal form is a `Cnf`, a vector of
clauses, each a vector of literals, $+( v + 1 )$ for the variable $v$ and
$-( v + 1 )$ for its negation; a set of nodes, a path or a tour is a
`Nodes`, a vector of `int`; and a system of 0-1 integer programming is
$A x \le b$. Also, the contracts check that the graphs are square (and symmetric
where the problem is unoriented), that the literals name the variables of
the formula and that the numbers of the numerical problems are not
negative, and each certificate returned by an enumeration is checked as a
postcondition.

## 20.2 Nondeterminism {#sec-20-2}

A *nondeterministic* algorithm may call `choice( k )`, which returns one
of $k$ alternatives, and it ends with success or failure; it answers YES
on $x$ if some sequence of choices leads to success, and its time is that
of the longest sequence of choices ([Montresor 2025,
§18.2](Z-bibliography.md#ref-montresor2025)). An algorithm of this kind
that runs in polynomial time is a certificate in disguise: the sequence of
the choices that leads to success is a $y$ of (20.1), and, conversely, the
algorithm that chooses the bits of $y$ one by one and then runs $V( x , y
)$ succeeds if and only if (20.1) holds. Hence NP is also the class of the
problems decided in polynomial time by a nondeterministic algorithm,
whence its name.

In `Nondeterminism.h`, a nondeterministic algorithm is a function that
receives a `Chooser` and calls its `choice( k )`, returning true for
success and false for failure. Internally, the `Chooser` keeps the sequence $P$ of the
pairs (choice, number of alternatives) of the current run and the number
$t$ of the choices made in it ([Algorithm 20.1](20-complexity.md#alg-20-1)).
A choice at a depth reached for the first time is 0, and one at a depth
already in $P$ repeats the choice stored there. `run( alg )` replays the
algorithm on each sequence of choices in lexicographic order, and stops at
the first success; after a failure, it drops from $P$ the choices of the
run beyond the depth reached and, from the end, those that have no
alternative left, and it advances the last one that has
([Figure 20.1](20-complexity.md#fig-20-1)). This is the visit in depth of
the tree of the choices with a backtrack on the last choice still open
([Montresor 2025, §18.3](Z-bibliography.md#ref-montresor2025)).

![Figure 20.1. The sequence of the pairs (choice, alternatives) of a `Chooser` after a run that made three choices and failed, and before the next run: the last two pairs have no alternative left and are dropped, and the first one is advanced.](../figures/complexity-chooser.svg){#fig-20-1}

<span id="alg-20-1"></span>**Algorithm 20.1.** The choice of a
`Chooser`, and the simulation of a nondeterministic algorithm by the
enumeration of its sequences of choices ([Montresor 2025,
§18.3](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Nondeterminism-Chooser-choice.pseudo.md"

--8<-- "generated/Nondeterminism-Chooser-run.pseudo.md"

```cpp
--8<-- "generated/Nondeterminism-run.cpp"
```

<span id="lem-20-1"></span>**Lemma 20.1 (the simulation).** *A
nondeterministic algorithm that makes at most $h$ choices of at most $k$
alternatives each, in time $t$, is simulated by `run()` in time*

$$
O( k^h \, t ) . \tag{20.2}
$$

*Proof.* The sequences of choices are the paths from the root of a tree of
height at most $h$ whose nodes have at most $k$ children, and the tree has
at most $k^h$ leaves. Each run follows one path, in time $O( t )$, and the
next sequence differs from the last one in a suffix; hence the runs are at
most $k^h$, and the bookkeeping of $P$ costs $O( h ) = O( t )$ per run.
$\square$

With $h$ and $t$ polynomial in $\lvert x \rvert$, (20.2) is $2^{O( \lvert x
\rvert^c )}$; hence $\mathrm{NP} \subseteq \mathrm{EXP}$, and no
simulation essentially faster than the visit of the tree is known for the
NP-complete problems of [§20.4](20-complexity.md#sec-20-4). If a pointer
`runs` is given, `run()` counts there the runs that it makes, which a
test uses to observe the bound (20.2); `choices()` returns the sequence of
the successful run after a YES, i.e., the certificate.

## 20.3 Certificates and enumerations {#sec-20-3}

In fact, the enumerations of `Problems.h` and `squareTiling()` of `Domino.h` are
(20.2) specialized to each problem: a backtrack builds the certificate one
piece at a time and checks each partial certificate, so that a subtree
whose root already fails the check is skipped. Its tree of the choices is
that of the subsets, of the permutations or of the functions of the
problem, and [Table 20.1](20-complexity.md#tab-20-1) gives its size, i.e. (cf. (20.2)),
the time in the worst case up to the polynomial cost of a node, which is typically "small".

<span id="tab-20-1"></span>**Table 20.1.** The enumerations of
`ADS/complexity` and the trees of their choices; $m$ is the number of the
processors, or of the tiles, and $Y$ the family of the 3-sets.

| Problem | Function | Certificate | Leaves of the tree |
|---|---|---|---|
| CLIQUE, INDEPENDENT SET | `clique()`, `independentSet()` | $k$ nodes | $\binom{n}{k}$ |
| VERTEX COVER | `vertexCover()` | at most $k$ nodes | $\sum_{s \le k} \binom{n}{s}$ |
| GRAPH COLORABILITY | `coloring()` | a color per node | $k^n$ |
| BISECTION | `bisection()` | $n / 2$ nodes, 0 among them | $\binom{n - 1}{n / 2 - 1}$ |
| TRAVELING SALESMAN, the Hamiltonian problems | `tsp()`, `hamiltonianCircuit()`, and so on | a permutation | $( n - 1 )!$ or $n!$ |
| LONGEST PATH | `longestPath()` | a simple path | $O( ( n - 1 )! )$ |
| DEGREE-CONSTRAINED SPANNING TREE | `boundedSpanningTree()` | $n - 1$ edges | at most $2^{\lvert E \rvert}$ |
| SATISFIABILITY, 0-1 INTEGER PROGRAMMING | `satisfiable()`, `lp01()` | a 0-1 vector | $2^n$ |
| SUBSET SUM, PARTITION, KNAPSACK | `subsetSumExists()`, `partition()`, `knapsackDecision()` | a subset | $2^n$ |
| MULTIPROCESSOR SCHEDULING | `multiprocessorScheduling()` | a processor per program | $m^n$ |
| EXACT COVER BY 3-SETS, 3-DIMENSIONAL MATCHING | `exactCover3()`, `threeDimensionalMatching()` | a subset of the sets | $2^{\lvert Y \rvert}$ |
| BOUNDED TILING | `squareTiling()` | a tile per square | $m^{n^2 - 1}$ |

**Graphs.** `clique()` (the `enCricca()` of the book) lists the sets of
$k$ nodes in increasing order, and extends a set only by a node adjacent
to all of it; `independentSet()` is the same enumeration with the test
negated, and `vertexCover()` lists the sets by increasing size up to $k$
([Algorithm 20.2](20-complexity.md#alg-20-2)). Note that the enumeration of the sets
stops extending a partial set when the nodes left cannot complete it to
$k$ nodes. `coloring()` (exercise 18.6) colors the nodes in order, and
numbers the colors in the order of their first use, so that two colorings
that differ by a renaming of the colors are not both tried; `bisection()`
looks for the halves of $n / 2$ nodes that contain the node 0, so that
each partition is tried once, and cut by at most $k$ edges. The
certificate `cliqueCertificate()` (the `certificatoCricca()` of the book)
checks in $O( \lvert S \rvert^2 )$ that the set $S$ has distinct nodes, at
least $k$ of them, all adjacent.

<span id="alg-20-2"></span>**Algorithm 20.2.** The certificate of CLIQUE,
and the enumerations of CLIQUE, INDEPENDENT SET, VERTEX COVER, GRAPH
COLORABILITY and BISECTION ([Montresor 2025, chapter 18 and exercise
18.6](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Problems-cliqueCertificate.pseudo.md"

--8<-- "generated/Problems-clique.pseudo.md"

--8<-- "generated/Problems-independentSet.pseudo.md"

--8<-- "generated/Problems-vertexCover.pseudo.md"

--8<-- "generated/Problems-color.pseudo.md"

--8<-- "generated/Problems-bisection.pseudo.md"

**Tours and paths.** `tsp()` (the `enTsp()` of the book) lists the
permutations that start with the city 0, which breaks the symmetry of the
rotations, and does not extend a partial tour longer than $k$
([Algorithm 20.3](20-complexity.md#alg-20-3)); the pruning is correct only
for distances not negative, which are a precondition (adding a constant
$c$ to all the distances and $n c$ to $k$ keeps the answer, and makes them
so). Also, the four Hamiltonian problems share one backtracking, `hamilton( P )`,
which extends a path by the nodes adjacent to its last one, in increasing
order: `hamiltonianCircuit()` from $P = ( 0 )$ and closed,
`hamiltonianCycle()` on the oriented edges (exercise 18.12),
`hamiltonianPath()` from each possible start, or from a given $r$ to a
given $t$ (exercise 18.13), and `hamiltonianChain()` on the unoriented
edges (exercise 18.14). `longestPath()` (exercise 18.15) looks for a simple
path from $r$ to $t$ of at least $k$ edges, and does not extend a path that
reaches $t$ too short, since it could not come back to $t$; on an acyclic
graph the same problem is polynomial, since `longestPathDag()` computes the
longest path by one scan of the nodes in topological order (a path never
comes back to a node already scanned), in $O( n^2 )$ on the matrix.
`boundedSpanningTree()` (exercise 18.16) takes each edge and then leaves
it, in order, never taking an edge that closes a cycle or exceeds a degree
$k$, and stops when the edges left cannot complete a tree.

<span id="alg-20-3"></span>**Algorithm 20.3.** The certificate and the
enumeration of TRAVELING SALESMAN, the backtracking of the Hamiltonian
problems, and the paths and the trees of exercises 18.12 to 18.16.

--8<-- "generated/Problems-tspCertificate.pseudo.md"

--8<-- "generated/Problems-extend.pseudo.md"

--8<-- "generated/Problems-hamilton.pseudo.md"

--8<-- "generated/Problems-hamiltonianCycle.pseudo.md"

--8<-- "generated/Problems-hamiltonianPath.pseudo.md"

--8<-- "generated/Problems-hamiltonianChain.pseudo.md"

--8<-- "generated/Problems-extend-2.pseudo.md"

--8<-- "generated/Problems-longestPathDag.pseudo.md"

--8<-- "generated/Problems-extend-3.pseudo.md"

**Formulae and numbers.** `satisfiable()` assigns the variables in order,
and checks a clause as soon as its largest variable is set, which cuts the
tree at the first clause made false; `satCertificate()` checks an
assignment in time linear in the size of the formula, and `lp01()` lists
the $2^n$ vectors of 0-1 integer programming in lexicographic order
([Algorithm 20.4](20-complexity.md#alg-20-4)). Each numerical problem takes
each element and then leaves it: `subsetSumExists()` does not extend a sum
above $k$, `partition()` looks for a subset of half the sum, and
`knapsackDecision()` (the `enZaino()` of the book, exercise 18.5) does not
extend a volume above the capacity. `multiprocessorScheduling()` tries each
program on each processor on which it fits within the time $d$, but on one
empty processor only, since the empty processors are all equal; it
answers NO for $d < 0$, even with no programs, since an idle processor is
loaded 0.

<span id="alg-20-4"></span>**Algorithm 20.4.** The certificate and the
enumeration of SATISFIABILITY, 0-1 INTEGER PROGRAMMING, and the numerical
problems of chapter 18 ([Montresor 2025, chapter 18 and exercise
18.5](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Problems-satCertificate.pseudo.md"

--8<-- "generated/Problems-assign.pseudo.md"

--8<-- "generated/Problems-lp01.pseudo.md"

--8<-- "generated/Problems-extend-4.pseudo.md"

--8<-- "generated/Problems-partition.pseudo.md"

--8<-- "generated/Problems-extend-5.pseudo.md"

--8<-- "generated/Problems-assign-2.pseudo.md"

**Covers and matchings.** `exactCover3()` (the `enCopertura()` of the
book, exercise 18.7) looks for sets of three elements of $\{ 0 , \ldots , m
- 1 \}$ that partition it, taking each set that meets none of those chosen
and then leaving it; `threeDimensionalMatching()` reduces 3-DIMENSIONAL
MATCHING to it, by making the three coordinates of the triples disjoint
(the triple $( w , x , v )$ becomes the set $\{ w , q + x , 2 q + v \}$).
`bribes()` (the `mazzette()` of the book, exercises 18.1 and 18.4) gives
the $n$ bribes $M$ to the $n$ politicians in such a way that the party PS gets twice
as much as the party PDF, by the permutations of the bribes in
lexicographic order ([Algorithm 20.5](20-complexity.md#alg-20-5)).

<span id="alg-20-5"></span>**Algorithm 20.5.** The exact cover by 3-sets,
the 3-dimensional matching, and the bribes ([Montresor 2025, exercises
18.1, 18.4 and 18.7](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Problems-extend-6.pseudo.md"

--8<-- "generated/Problems-threeDimensionalMatching.pseudo.md"

--8<-- "generated/Problems-bribes.pseudo.md"

**The square tiling.** In BOUNDED TILING (the DOMINO LIMITATO of the
book), the $n \times n$ squares of a board are to be covered with copies
of $m$ oriented tiles, each split by its diagonals into four colored
quarters, never rotated, with the tile $d = D[ 0 ]$ in the corner $( 0 , 0
)$ and touching sides of the same color ([Figure 20.2](20-complexity.md#fig-20-2)).
A `Tile` holds the colors of its four sides, and the square $( i , j )$ is
the column $i$ and the row $j$ from the bottom left. `legalTiling()` is the
certificate, in $O( n^2 )$; `squareTiling()` fills the squares by rows from
the bottom, and places a tile only if it fits its left and lower
neighbours, which prunes the enumeration of the $m^{n^2 - 1}$ coverings at
the first wrong side ([Algorithm 20.6](20-complexity.md#alg-20-6)).

![Figure 20.2. Four tiles, each with the colors of its upper, right, lower and left sides, and a legal covering of the $2 \times 2$ board with $d = D[ 0 ]$ in the corner $( 0 , 0 )$; the touching sides have the same color.](../figures/complexity-tiling.svg){#fig-20-2}

<span id="alg-20-6"></span>**Algorithm 20.6.** The certificate and the
enumeration of BOUNDED TILING ([Montresor 2025,
chapter 18](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Domino-legalTiling.pseudo.md"

--8<-- "generated/Domino-place.pseudo.md"

We remark that the pruning of the enumerations usually changes the time in practice, but never
the bound of [Table 20.1](20-complexity.md#tab-20-1): there are instances
(e.g., those with no solution and no early failure) on which the tree is
visited in full.

## 20.4 Reductions and NP-completeness {#sec-20-4}

A *polynomial reduction* of $A$ to $B$ is a function $f$, computable in
polynomial time, such that

$$
x \in Y_A \iff f( x ) \in Y_B , \tag{20.3}
$$

written $A \propto B$ ([Montresor 2025, §18.4](Z-bibliography.md#ref-montresor2025)).

<span id="lem-20-2"></span>**Lemma 20.2.** *The relation $\propto$ is
transitive, and $A \propto B$ with $B \in \mathrm{P}$ gives $A \in
\mathrm{P}$.*

*Proof.* An algorithm of polynomial time writes an output of polynomial
size, and therefore $\lvert f( x ) \rvert \le q( \lvert x \rvert )$ for a
polynomial $q$; a polynomial algorithm for $B$ run on $f( x )$, and a
second reduction applied to $f( x )$, take time polynomial in $\lvert x
\rvert$, since a polynomial of a polynomial is a polynomial. $\square$

A problem is *NP-hard* if every problem of NP reduces to it, and
*NP-complete* if it is also in NP; one NP-complete problem in P would give
$\mathrm{P} = \mathrm{NP}$, and, by [Lemma 20.2](20-complexity.md#lem-20-2),
a problem of NP to which a known NP-complete problem reduces is
NP-complete in its turn.

<span id="thm-20-1"></span>**Theorem 20.1 (Cook; Levin).**
*SATISFIABILITY is NP-complete.*

We refer to [Cook 1971](Z-bibliography.md#ref-cook1971) and [Levin
1973](Z-bibliography.md#ref-levin1973) for the proof, which writes, for a
nondeterministic algorithm of polynomial time and an input $x$, a formula
of polynomial size whose variables describe the contents of the memory at
each step and whose clauses force each step to follow from the one before
it; the formula is satisfiable if and only if a sequence of choices leads
to success. Instead, the book starts the chain from BOUNDED TILING instead, whose
rows of tiles can encode the successive configurations of a computation in
the same way (the tiles of [Wang 1961](Z-bibliography.md#ref-wang1961)),
and reduces it to the other problems, as [Karp
1972](Z-bibliography.md#ref-karp1972) did from SATISFIABILITY. Altogether, the functions
of `Reductions.h`, and those of `Problems.h` that delegate to one another,
realize the chain of [Table 20.2](20-complexity.md#tab-20-2), each in the
time needed to write its output.

<span id="tab-20-2"></span>**Table 20.2.** The reductions realized in
`ADS/complexity` ([Montresor 2025, §18.5 and
§18.7](Z-bibliography.md#ref-montresor2025)).

| From | To | Where | Function |
|---|---|---|---|
| BOUNDED TILING | CLIQUE | Theorem 18.2 | `dominoToClique()` |
| BOUNDED TILING | SATISFIABILITY | Theorem 18.3 | `dominoToSat()` |
| BOUNDED TILING | 0-1 INTEGER PROGRAMMING | Theorem 18.4 | `dominoToLp01()` |
| CLIQUE | INDEPENDENT SET | Theorem 18.5 | `complement()` |
| INDEPENDENT SET | VERTEX COVER | exercise 18.11 | the complement of the set, $k' = n - k$ |
| HAMILTONIAN CIRCUIT | TRAVELING SALESMAN | Theorem 18.6 | `hamiltonToTsp()` |
| PARTITION | MULTIPROCESSOR SCHEDULING, SUBSET SUM, KNAPSACK | Theorems 18.7 to 18.9 | `halfSum()` |
| HAMILTONIAN CIRCUIT | DIRECTED HAMILTONIAN CIRCUIT | exercise 18.12 | both the orientations of each edge |
| DIRECTED HAMILTONIAN CIRCUIT | DIRECTED HAMILTONIAN PATH | exercise 18.13 | `cycleToPath()` |
| DIRECTED HAMILTONIAN CIRCUIT | HAMILTONIAN PATH | exercise 18.14 | `cycleToChain()` |
| DIRECTED HAMILTONIAN PATH | LONGEST PATH | exercise 18.15 | $k = n - 1$ |
| HAMILTONIAN PATH | DEGREE-CONSTRAINED SPANNING TREE | exercise 18.16 | $k = 2$ |
| 3-DIMENSIONAL MATCHING | EXACT COVER BY 3-SETS | chapter 18 | `threeDimensionalMatching()` |

Each reduction rests on a correspondence between the certificates of the
two problems, which we now state for the reductions from the tiling, the
others being (somewhat) simpler. In all of them, the node, or the variable, of the tile $h$ in the
square $( i , j )$ is $u_{hij}$, at the place $( h n + i ) n + j$ given by
`tileVar()`.

<span id="lem-20-3"></span>**Lemma 20.3.** *In the graph of
`dominoToClique()`, with $m n^2$ nodes, the cliques of $n^2$ nodes are the
legal coverings; hence BOUNDED TILING $\propto$ CLIQUE, in time $\Theta(
m^2 n^4 )$.*

*Proof.* The graph joins $u_{hij}$ to all the nodes of the squares not
adjacent to $( i , j )$, and to the nodes of the adjacent squares whose
tile fits next to $h$, and it leaves isolated the nodes of $( 0 , 0 )$ but
$u_{000}$. Two nodes of one square are never joined, and therefore a
clique of $n^2$ nodes takes one tile per square, the tile $d$ in $( 0 , 0
)$; two of its tiles in adjacent squares are joined, i.e., they fit, and
the clique is a legal covering. Conversely, the nodes of a legal covering
are pairwise joined. Finally, the adjacency matrix has $( m n^2 )^2$ entries.
$\square$

`dominoToSat()` writes the same conditions as clauses on the variables
$u_{hij}$, true if the tile $h$ is in $( i , j )$: the unit clause
$u_{000}$, at least one tile and at most one tile in each square, and,
for each pair of adjacent squares and each pair of tiles that do not fit
there side by side, the clause that forbids both. `dominoToLp01()` writes
them as linear constraints: $x_{000} = 1$, one tile per square (each
equation as two inequalities), and $x_{hij}$ plus the tiles that do not fit
next to it in a neighbouring square at most 1; the example of the text
writes one inequality per pair, which is the same, since a square holds one
tile ([Algorithm 20.7](20-complexity.md#alg-20-7)).

<span id="alg-20-7"></span>**Algorithm 20.7.** The reductions of BOUNDED
TILING to CLIQUE, to SATISFIABILITY and to 0-1 INTEGER PROGRAMMING
(Theorems 18.2 to 18.4 of [Montresor 2025](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Reductions-dominoToClique.pseudo.md"

--8<-- "generated/Reductions-dominoToSat.pseudo.md"

--8<-- "generated/Reductions-dominoToLp01.pseudo.md"

One can write the other reductions of the chain as follows
([Algorithm 20.8](20-complexity.md#alg-20-8)). In `complement()`, a set is
independent in $G$ if and only if it is a clique of the complement graph;
and $S$ is independent if and only if $V \setminus S$ covers the edges,
since an edge left uncovered by $V \setminus S$ has both its ends in $S$,
which gives VERTEX COVER with $k' = n - k$. In `hamiltonToTsp()` the
distance is 1 on the edges of $G$ and 2 elsewhere, with $k = n$: a tour has
$n$ edges of length at least 1, and its length is $n$ if and only if all
its edges are edges of $G$. In `halfSum()` the bound is half the sum of
the numbers, or $-1$ if the sum is odd (then no partition exists, and the
bound $-1$ has no YES in any of the three problems): a set of numbers of
sum $s / 2$ leaves the other half to the second processor, or to the
objects left out of the knapsack, whose profits and volumes are the
numbers themselves. `cycleToPath()` splits the node 0 into a new node $n$,
which takes the edges out of 0, and the node 0, which keeps the edges into
it, so that a
cycle through 0 becomes a path from $n$ to 0, and conversely;
`cycleToChain()` makes each node $v$ of that path a chain $v_{in} -
v_{mid} - v_{out}$, each edge $( u , v )$ an edge $[ u_{out} , v_{in} ]$,
and adds a pendant node at each end, so that an unoriented Hamiltonian path
must cross each chain from $v_{in}$ to $v_{out}$, which keeps the
orientation, and the pendant nodes fix its two ends ($3 ( n + 1 ) + 2$
nodes in all). Both drop the loops of $G$, as the problems of
`Problems.h` ignore them.

<span id="alg-20-8"></span>**Algorithm 20.8.** The reductions of CLIQUE
to INDEPENDENT SET, of HAMILTONIAN CIRCUIT to TRAVELING SALESMAN, of
PARTITION to the numerical problems, and of the directed Hamiltonian
circuit to the paths.

--8<-- "generated/Reductions-complement.pseudo.md"

--8<-- "generated/Reductions-hamiltonToTsp.pseudo.md"

--8<-- "generated/Reductions-halfSum.pseudo.md"

--8<-- "generated/Reductions-cycleToPath.pseudo.md"

--8<-- "generated/Reductions-cycleToChain.pseudo.md"

The same tool proves lower bounds within P. Sorting reduces to the convex
hull (`sortToHull()`, [Algorithm 20.9](20-complexity.md#alg-20-9)): the
points $( a_i , a_i^2 )$ lie on a parabola, all of them are vertices of
the hull, and the hull lists them in the order of the $a_i$; hence a hull
computed with $o( n \log n )$ comparisons would sort in $o( n \log n )$,
against the lower bound of the sorting by comparisons, and the convex hull
costs $\Omega( n \log n )$ ([Montresor 2025,
§18.5](Z-bibliography.md#ref-montresor2025)). Note that the squares of the numbers
must fit in a `long`, which is a precondition.

<span id="alg-20-9"></span>**Algorithm 20.9.** The reduction of sorting
to the convex hull.

--8<-- "generated/Reductions-sortToHull.pseudo.md"

## 20.5 Polynomial cases {#sec-20-5}

The square tiling is NP-complete, while the tiling of a strip $1 \times n$
is in P ([Montresor 2025, §18.5](Z-bibliography.md#ref-montresor2025)): a
strip is a walk of $n - 1$ edges from $d$ in the graph $H$ whose nodes are
the $m$ tiles and whose edges $( d_u , d_v )$ join a tile to those that fit
on its right.

<span id="lem-20-4"></span>**Lemma 20.4 (the linear tiling).** *A strip of
$n$ tiles from $d$ exists if and only if*

$$
\text{a cycle of } H \text{ is reachable from } d \quad \text{or} \quad
L( d ) \ge n - 1 , \tag{20.4}
$$

*$L( d )$ being the number of the edges of the longest path from $d$.
`linearTiling()` decides (20.4) in $O( m^2 )$ time, whatever $n$ is.*

*Proof.* A walk may go round a reachable cycle as many times as it needs,
and so has any length; without a reachable cycle, every walk from $d$ is a
path, and its length is at most $L( d )$. One visit in depth of $H$ from
$d$ finds a reachable cycle as a back edge, i.e., an edge to a node still
open, and computes $L$ in the postvisit, since without cycles the longest
path from $u$ is one edge plus the longest from a successor; the visit
examines the $m^2$ pairs of tiles of the implicit matrix of $H$ once each.
$\square$

```cpp
--8<-- "generated/Domino-linearTiling.cpp"
```

Note that $n$ enters the input with $\log n$ bits, and an enumeration of
the strips would take time exponential in it; the visit, instead, does not
depend on $n$ at all ([Algorithm 20.10](20-complexity.md#alg-20-10)).

<span id="alg-20-10"></span>**Algorithm 20.10.** The linear tiling by a
visit in depth of the graph of the tiles.

--8<-- "generated/Domino-visit.pseudo.md"

In its conclusion, [Montresor 2025](Z-bibliography.md#ref-montresor2025)
pairs several NP-complete problems with a polynomial one that differs from
it in a detail, and `EasyCases.h` realizes the polynomial halves that the
other areas do not ([Table 20.3](20-complexity.md#tab-20-3)).

<span id="tab-20-3"></span>**Table 20.3.** The polynomial halves of the
pairs of table 19.1 of the book, and where the library realizes them.

| NP-complete | Polynomial | Where |
|---|---|---|
| SATISFIABILITY, three literals per clause | two literals per clause | `twoSat()` |
| 3-COLORABILITY | 2-COLORABILITY | `twoColoring()` |
| 3-DIMENSIONAL MATCHING | 2-DIMENSIONAL MATCHING | `bipartiteMatching()` |
| CLIQUE | CLIQUE on an interval graph | `intervalClique()` |
| BOUNDED TILING | the linear tiling | `linearTiling()` of `Domino.h` |
| HAMILTONIAN CIRCUIT | the Eulerian cycle | `eulerianCycle()` of `EulerianCycle.h` |
| KNAPSACK | the fractional knapsack | `fractionalKnapsack()` ([§13.11](13-algorithm.md#sec-13-11)) |
| DEGREE-CONSTRAINED SPANNING TREE | the minimum spanning tree | `kruskal()` of `MinimumSpanningTree.h` |
| LONGEST PATH | the shortest path | `shortestPaths()` of `ShortestPaths.h` |

<span id="thm-20-2"></span>**Theorem 20.2 (Aspvall, Plass and Tarjan).**
*A formula of clauses of at most two literals is satisfiable if and only
if no variable $v$ lies in the strongly connected component of $\neg v$ in
the graph of the implications, which has the edges $( \neg a , b )$ and $(
\neg b , a )$ for each clause $a \vee b$; the assignment that makes $v$
true when the component of $v$ comes after that of $\neg v$ in a
topological order of the components satisfies the formula.*

*Proof.* A path of implications from $v$ to $\neg v$ and one back would
force $v$ to be both true and false. Conversely, write $\kappa( a )$ for
the place of the component of the literal $a$ in the topological order, so
that an edge $( a , b )$ gives $\kappa( a ) \le \kappa( b )$, and suppose
that the assignment of the statement sets a literal $a$ true and a
successor $b$ of it false, i.e., $\kappa( \neg a ) < \kappa( a )$ and
$\kappa( b ) < \kappa( \neg b )$. Since the graph is symmetric under negation
(it has $( \neg b , \neg a )$ whenever it has $( a , b )$), one has
$\kappa( \neg a ) < \kappa( a ) \le \kappa( b ) < \kappa( \neg b ) \le
\kappa( \neg a )$, which is impossible. A clause $a \vee b$ made false
would give such a pair, the edge $( \neg a , b )$ from a true literal to a
false one ([Aspvall, Plass and Tarjan
1979](Z-bibliography.md#ref-aspvall1979)). $\square$

`twoSat()` takes a clause of one literal $a$ as $a \vee a$, builds the
graph of the $2 n$ literals (the literal $\pm( v + 1 )$ at the node $2 v$
or $2 v + 1$, by `literalNode()`) with each implication once, and ranks
the strongly connected components in the topological order
([Algorithm 20.11](20-complexity.md#alg-20-11)), in $O( n + m )$ time for
$m$ clauses (up to the logarithmic factor of the set that removes the
repeated implications).

<span id="alg-20-11"></span>**Algorithm 20.11.** The polynomial halves:
2-SATISFIABILITY, 2-COLORABILITY, the bipartite matching, and the cliques
of an interval graph.

--8<-- "generated/EasyCases-twoSat.pseudo.md"

```cpp
--8<-- "generated/EasyCases-twoSat.cpp"
```

--8<-- "generated/EasyCases-twoColoring.pseudo.md"

--8<-- "generated/EasyCases-bipartiteMatching.pseudo.md"

--8<-- "generated/EasyCases-intervalGraph.pseudo.md"

--8<-- "generated/EasyCases-intervalClique.pseudo.md"

Similarly, a graph is 2-colorable if and only if it has no odd cycle,
which the visit in breadth of `twoColoring()` finds as an edge between two
nodes of the same color, in $O( n^2 )$ on the adjacency matrix (the
diagonal ignored, as by `coloring()`). A bipartite matching is a maximum
flow with unit capacities from a source to the left nodes, along the
edges, to a sink (`bipartiteMatching()`, by the reduction of section 15.2
of the book). On intervals of the line, finally, a set that meets pairwise
has a common point (the property of Helly in one dimension: the latest
left end lies before the earliest right end), and the largest clique of
the graph of the intervals $[ a , b )$ built by `intervalGraph()` is the
largest number of intervals through one point, which a sweep of the sorted
ends counts in $O( n \log n )$ (`intervalClique()`, the right ends before
the left ones, so that an empty interval meets none).

## 20.6 Pseudopolynomial algorithms {#sec-20-6}

We now turn to the strategies for the problems that are NP-hard. A problem
of optimization asks, for an instance $x$, a feasible solution of least
cost (or of largest value) among the set $F( x )$ of the feasible ones;
$x^*$ is an optimal solution and $\mathit{OPT} = c( x^* )$ its cost. When
the decision version is NP-complete, no algorithm is known that finds
$x^*$ in polynomial time on every instance, and each strategy gives up
something. An algorithm is *pseudopolynomial* if its time is bounded by a
polynomial in the size $d$ of the instance and in its largest integer
$\#$ ([Montresor 2025, §19.1](Z-bibliography.md#ref-montresor2025)); since
$\#$ may be exponential in $d$, such an algorithm is polynomial only on the
instances whose numbers are "small". For SUBSET SUM, with $M[ i ][ j ]$
true if some of the first $i$ elements sum to $j$,

$$
M[ i ][ j ] = M[ i - 1 ][ j ] \ \text{or} \ ( j \ge A_i \ \text{and} \ M[
i - 1 ][ j - A_i ] ) , \tag{20.5}
$$

since a subset of the first $i$ elements either avoids the $i$-th or takes
it once. `subsetSumDP()` of `Pseudopolynomial.h` fills (20.5) in $O( n k )$
time with one row of $k + 1$ bits, written from the right, so that $M[ j -
A_i ]$ is still the value of the row before
([Algorithm 20.12](20-complexity.md#alg-20-12)); the row is a vector of
`Bits`, which may be `std::vector< bool >` (the default),
`boost::dynamic_bitset` or the `BitVector` of the library, with the same
cells written. If a pointer `rows` is given, the function appends there the
row after each element. `subsetSumSet()` keeps all the rows, and reads a
subset backwards (an element is needed when the row above does not have
the sum), in $O( n k )$ time and space; `partitionDP()` (exercise 19.1) is
the same with half the sum.

<span id="alg-20-12"></span>**Algorithm 20.12.** SUBSET SUM and PARTITION
by dynamic programming ([Montresor 2025, §19.1 and exercise
19.1](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Pseudopolynomial-subsetSumDP.pseudo.md"

```cpp
--8<-- "generated/Pseudopolynomial-subsetSumDP.cpp"
```

--8<-- "generated/Pseudopolynomial-subsetSumSet.pseudo.md"

--8<-- "generated/Pseudopolynomial-partitionDP.pseudo.md"

With more parts, the same idea gives the other dynamic programmings of
the header ([Algorithm 20.13](20-complexity.md#alg-20-13)). In `bottino()`
(exercise 19.5), three thieves share jewels of prices $P[ i ]$ so that
each takes a third of the value $m$: $S[ i ][ j ][ h ]$ records whether the
first $i$ jewels give $j$ to the first thief and $h$ to the second, the
third taking what is left, and since the prices are not negative $j$ and
$h$ range in $[ 0 , m / 3 ]$, i.e., $O( n m^2 )$ entries. In
`schedulingDP()` (exercise 19.2), the vectors of the loads of the $m$
processors reachable with the first $i$ programs, each load at most $d$,
are at most $( d + 1 )^m$ after each program, and the time is $O( n ( d + 1
)^m )$, pseudopolynomial for $m$ fixed.

<span id="alg-20-13"></span>**Algorithm 20.13.** The loot of the three
thieves, and MULTIPROCESSOR SCHEDULING by the vectors of the loads.

--8<-- "generated/Pseudopolynomial-bottino.pseudo.md"

--8<-- "generated/Pseudopolynomial-schedulingDP.pseudo.md"

A problem is *strongly* NP-complete if it stays NP-complete on the
instances with $\#$ bounded by a polynomial in $d$ (CLIQUE, whose numbers
are bounded by $n$, TRAVELING SALESMAN, 3-PARTITION), and *weakly*
otherwise (SUBSET SUM, PARTITION, KNAPSACK); cf. [Garey and Johnson
1978](Z-bibliography.md#ref-garey1978).

<span id="thm-20-3"></span>**Theorem 20.3.** *A strongly NP-complete
problem has no pseudopolynomial algorithm unless $\mathrm{P} =
\mathrm{NP}$.*

*Proof.* On the instances with $\# \le q( d )$, $q$ a polynomial, the time
of a pseudopolynomial algorithm is a polynomial in $d$ and $q( d )$, i.e.,
in $d$; such an algorithm would solve in polynomial time a restriction
that is NP-complete. $\square$

When a program may be interrupted and resumed on another processor, the
scheduling becomes easy. No schedule is shorter than

$$
d = \max\Bigl\{ \max_i t_i , \ \sum_i t_i / m \Bigr\} , \tag{20.6}
$$

since each program runs for its time and the $m$ processors do at most $m
d$ work, and the rule of [McNaughton 1959](Z-bibliography.md#ref-mcnaughton1959)
(`mcNaughton()`, exercise 19.3) reaches (20.6) by filling the processors
one after the other up to $d$, in $O( n + m )$ time
([Algorithm 20.14](20-complexity.md#alg-20-14)).

<span id="lem-20-5"></span>**Lemma 20.5.** *The schedule of `mcNaughton()`
runs each program for its time within $[ 0 , d ]$, and never on two
processors at once.*

*Proof.* A program split between the processors $q$ and $q + 1$ runs on
$q$ from some time $s$ to $d$, and on $q + 1$ from 0 to $t_i - ( d - s )$;
since $t_i \le d$, one has $t_i - ( d - s ) \le s$, and the two pieces do
not overlap in time. Also, the total work $\sum_i t_i \le m d$ fits in the $m$
processors. $\square$

With $t_i = C_i / T_i$ and $d = 1$, the same rule schedules a period of
the periodic tasks of REAL-TIME SCHEDULING without a priority rule
(`realTimeScheduling()`, exercise 19.4), the task $i$ running for the
fraction $C_i / T_i$ of each period, which succeeds if and only if $C_i \le
T_i$ for each $i$ and $\sum_i C_i / T_i \le m$. Schedules are lists of
`Piece`s, each the program and the interval $[ \mathit{start} ,
\mathit{end} )$ in which it runs, and the contracts check them as a
postcondition.

<span id="alg-20-14"></span>**Algorithm 20.14.** The rule of McNaughton,
and REAL-TIME SCHEDULING ([Montresor 2025, exercises 19.3 and
19.4](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Pseudopolynomial-mcNaughton.pseudo.md"

--8<-- "generated/Pseudopolynomial-realTimeScheduling.pseudo.md"

## 20.7 Approximation algorithms {#sec-20-7}

An algorithm is an *approximation* with relative error $e$ if it gives a
feasible $x'$ with

$$
\frac{\lvert c( x' ) - \mathit{OPT} \rvert}{\mathit{OPT}} \le e \tag{20.7}
$$

on every instance, and an *approximation scheme* if it takes $\epsilon >
0$ in input and satisfies (20.7) with $e = \epsilon$ in a time polynomial
in $d$ for each fixed $\epsilon$ (fully polynomial if also in $1 /
\epsilon$) ([Montresor 2025, §19.2](Z-bibliography.md#ref-montresor2025)).
All the guarantees of `Approximation.h` rest on a lower bound of
$\mathit{OPT}$ that the algorithm can compare with its own cost
([Table 20.4](20-complexity.md#tab-20-4)).

<span id="tab-20-4"></span>**Table 20.4.** The approximations of
`Approximation.h` ([Montresor 2025, §19.2 and exercises 19.6 and
19.8](Z-bibliography.md#ref-montresor2025)); $N$ and $N^*$ are the boxes
used and the fewest possible.

| Problem | Function | Guarantee | Time |
|---|---|---|---|
| BIN PACKING | `firstFit()` | $N < 2 N^*$ | $O( n^2 )$ |
| TRAVELING SALESMAN, triangular | `deltaTsp()` | $c( x' ) \le 2 \, \mathit{OPT}$ | $O( n^2 \log n )$ |
| SUBSET SUM | `apSubsetSum()` | $e < 1 / ( h + 1 )$ | $O( h n^{h+1} )$ |
| VERTEX COVER | `vertexCoverApprox()` | $\lvert S \rvert \le 2 \, \mathit{OPT}$ | $O( n^2 )$ |
| KNAPSACK | `knapsackApprox()` | $c( x' ) \ge ( 1 - \epsilon ) \mathit{OPT}$ | $O( n^3 / \epsilon )$ |

**Bin packing.** `firstFit()` puts each object of volume in $[ 0 , k ]$
in the first box with room for it, the objects in the order given (ff) or
by decreasing volume (ffd), and opens a new box when none has room
([Algorithm 20.15](20-complexity.md#alg-20-15)).

<span id="alg-20-15"></span>**Algorithm 20.15.** The first fit, and its
decreasing variant, for BIN PACKING.

--8<-- "generated/Approximation-firstFit.pseudo.md"

<span id="lem-20-6"></span>**Lemma 20.6.** *If some object has a positive
volume, the first fit uses $N < 2 N^*$ boxes.*

*Proof.* No two boxes have contents that sum to at most $k$, since the
objects of the later one would have fitted in the earlier one. If $N \ge
2$, summing the contents of the $N$ pairs of consecutive boxes (the last
with the first) counts each box twice, and twice the total volume $V$
exceeds $N k$; since $N^* \ge V / k$, one has $N < 2 V / k \le 2 N^*$. If
$N = 1$, then $N^* = 1$. $\square$

In practice, the decreasing order of ffd usually helps, and it improves the constant, to $11 / 9$
asymptotically ([Johnson et al. 1974](Z-bibliography.md#ref-johnson1974)).
Since a box is found by a scan, the time is $O( n N )$, i.e., $O( n^2 )$ in the
worst case.

**The triangular salesman.** `deltaTsp()` builds a minimum spanning tree
$T$ of the complete graph of the symmetric distances (by `kruskal()` on
the $n ( n - 1 ) / 2$ edges, whence the time $O( n^2 \log n )$), and
returns the cities in the preorder of $T$ from 0, the children by
increasing name ([Algorithm 20.16](20-complexity.md#alg-20-16);
[Rosenkrantz, Stearns and Lewis 1977](Z-bibliography.md#ref-rosenkrantz1977)).
`tourCost()` gives the cost of a circuit.

<span id="lem-20-7"></span>**Lemma 20.7.** *If the distances satisfy the
triangle inequality, the circuit of `deltaTsp()` costs at most $2 \,
\mathit{OPT}$.*

*Proof.* The tree $T$ weighs less than any circuit, which is a spanning
path plus an edge, and the walk that goes twice along each edge of $T$
costs $2 w( T ) < 2 \, \mathit{OPT}$. Then the preorder visits the cities in the
order of their first visit by the walk, and each shortcut over the cities
already visited costs no more than the stretch of the walk that it
replaces, by the triangle inequality. $\square$

The bound is reached, up to a vanishing term, by the two concentric
polygons of figure 19.3 of the book. We remark that the triangle inequality is checked by
an audit in $O( n^3 )$, with a relative tolerance for the floating point.
Without it there is no constant at all, as follows.

<span id="thm-20-4"></span>**Theorem 20.4.** *For no constant $s \ge 1$
does TRAVELING SALESMAN have a polynomial algorithm with $c( x' ) \le s \,
\mathit{OPT}$, unless $\mathrm{P} = \mathrm{NP}$.*

*Proof.* With the distances of `tspGap()`, 1 on the edges of $G$ and $s n$
elsewhere, a Hamiltonian circuit of $G$ is a tour of cost $n$, while any
other tour costs more than $s n$; an algorithm within $s$ would therefore
return a tour of cost at most $s n$ if and only if $G$ has a Hamiltonian
circuit, and decide HAMILTONIAN CIRCUIT in polynomial time. $\square$

<span id="alg-20-16"></span>**Algorithm 20.16.** The salesman with the
triangle inequality, and the distances of Theorem 19.2 of the book.

--8<-- "generated/Approximation-deltaTsp.pseudo.md"

--8<-- "generated/Approximation-tspGap.pseudo.md"

**Subset sum.** For the optimization version of SUBSET SUM (the largest
sum at most $k$), `apSubsetSum()` tries each set of at most $h$ elements,
in the order of the text (by size, then lexicographically on the elements
in decreasing order), and completes it greedily on the elements in
decreasing order ([Algorithm 20.17](20-complexity.md#alg-20-17)).

<span id="lem-20-8"></span>**Lemma 20.8.** *Let $z^* \le k$ be the
optimum. The sum $z$ found by `apSubsetSum()` is either $z^*$, or*

$$
z > z^* \Bigl( 1 - \frac{1}{h + 1} \Bigr) ; \tag{20.8}
$$

*the time is $O( h n^{h+1} )$.*

*Proof.* Let $S^*$ be optimal. If $\lvert S^* \rvert \le h$, the algorithm
tries $S^*$ itself. Otherwise it tries the $h$ largest elements of $S^*$,
and the greedy completion either takes all of $S^*$, or leaves out an
element $a$ of $S^*$, not larger than those $h$, with $z + a > k \ge z^*$.
Since $( h + 1 ) a \le z^*$, one has $z > z^* - a \ge z^* ( 1 - 1 / ( h +
1 ) )$. As for the time, the $O( n^h )$ sets of at most $h$ elements are each completed in
$O( n )$. $\square$

<span id="alg-20-17"></span>**Algorithm 20.17.** The approximation of
SUBSET SUM, the cover by the ends of a matching, and the approximation
scheme of KNAPSACK.

--8<-- "generated/Approximation-apSubsetSum.pseudo.md"

--8<-- "generated/Approximation-vertexCoverApprox.pseudo.md"

```cpp
--8<-- "generated/Approximation-vertexCoverApprox.cpp"
```

--8<-- "generated/Approximation-knapsackApprox.pseudo.md"

**Vertex cover.** `vertexCoverApprox()` (exercise 19.6) takes both the
ends of each edge $[ u , v ]$, $u < v$, whose ends are both out of the
cover so far. Indeed, the edges taken share no end, i.e., they form a matching
$M$; any cover has an end of each edge of $M$, and hence $\mathit{OPT} \ge
\lvert M \rvert$, while the cover returned has $2 \lvert M \rvert$ nodes.
This takes $O( n^2 )$ time on the adjacency matrix.

**Knapsack.** `knapsackApprox()` (exercise 19.8) is the scheme of [Ibarra
and Kim 1975](Z-bibliography.md#ref-ibarra1975): it keeps the objects that
fit by themselves, divides their profits by $K = \max\{ 1 , \epsilon \, p_{\max}
/ n' \}$, $n'$ being their number and $p_{\max}$ their largest profit, and
rounds them down; then it finds, by dynamic programming, the least volume
$V[ i ][ s ]$ of the first $i$ objects of rounded profit $s$, and returns
the set of largest rounded profit within the capacity.

<span id="lem-20-9"></span>**Lemma 20.9.** *`knapsackApprox()` returns a
set of profit at least $( 1 - \epsilon ) \, \mathit{OPT}$, in $O( n^3 /
\epsilon )$ time.*

*Proof.* Rounding a profit down to a multiple of $K$ loses less than $K$,
and therefore less than $n' K$ on any set, which is $\epsilon \, p_{\max}$
if $K > 1$ (and nothing if $K = 1$, since the profits are integers). The
set optimal for the rounded profits is thus within $\epsilon \, p_{\max}
\le \epsilon \, \mathit{OPT}$ of the optimum, since each of the objects
kept fits alone. As for the time, the table has a column per rounded profit, at most $n'
\cdot n' / \epsilon$ of them, and a row per object. $\square$

## 20.8 Branch-and-bound {#sec-20-8}

A branch-and-bound visits the tree of the partial solutions of the
enumeration of chapter 19 (backtracking), and skips a subtree when a lower
bound of the costs in it is not below the best cost found
([Montresor 2025, §19.3](Z-bibliography.md#ref-montresor2025)).

<span id="lem-20-10"></span>**Lemma 20.10 (the bound).** *If
$\mathit{lb}( S ) \le c( x )$ for each complete solution $x$ that extends
the partial solution $S$, then skipping $S$ when*

$$
\mathit{lb}( S ) \ge \mathit{minCost} \tag{20.9}
$$

*loses no solution better than the best found so far, and the visit
returns an optimum.*

*Proof.* A solution $x$ in the subtree of a skipped $S$ has $c( x ) \ge
\mathit{lb}( S ) \ge \mathit{minCost}$, and the best found so far is at
least as good; the solutions not skipped are all compared. $\square$

Note that the guarantee holds whatever the order of the visit and the bound, and the
time in the worst case stays that of the enumeration; what changes is the
number of the nodes in practice, which `bbTsp()`, `littleTsp()`,
`maxWeightClique()` and the knapsacks report in `*generated`, if given.

**The salesman, depth first.** `bbTsp()` chooses the cities of the circuit
one after the other from $S[ 0 ] = 0$, in increasing order, depth first,
starting from the circuit `start` if one is given
([Algorithm 20.18](20-complexity.md#alg-20-18)). A child is generated for
each city not in $S$, and visited only if its bound is below the best; a
complete circuit is the bound of its last node, i.e., its cost.
`tspLowerBound()` computes two bounds, chosen by `TspBound`. Let $S = S[ 0
.. i ]$ be the path chosen, $C_i$ its length, $R$ the cities out of $S$, $A$
the least distance from a city of $R$ back to $S[ 0 ]$, $B$ the least
distance from $S[ i ]$ to a city of $R$, and $D_h$, for $h \in R$, the least
cost $D[ p ][ h ] + D[ h ][ q ]$ of entering and leaving $h$ over the
cities $p \ne q$ different from $h$.

<span id="lem-20-11"></span>**Lemma 20.11.** *Both*

$$
\mathit{lb}_{\mathrm{BOOK}}( S ) = C_i + \Bigl\lceil \frac{A + B +
\sum_{h \in R} D_h}{2} \Bigr\rceil , \qquad
\mathit{lb}_{\mathrm{ONE\_TREE}}( S ) = C_i + A + B + w( T_R ) , \tag{20.10}
$$

*with $T_R$ a minimum spanning tree of $R$ (the ceiling only on integer
distances, and the second bound for symmetric distances), satisfy the
hypothesis of [Lemma 20.10](20-complexity.md#lem-20-10).*

*Proof.* A circuit that extends $S$ goes on from $S[ i ]$ through all the
cities of $R$ and back to $S[ 0 ]$, with $\lvert R \rvert + 1$ edges. Each
city of $R$ is entered once and left once, which costs at least $D_h$; the
sum of the $D_h$ counts each edge of the rest of the circuit between two
cities of $R$ twice, and the first and the last edge once, which cost at
least $B$ and $A$. Hence the rest costs at least half the sum, and the cost
is an integer on integer distances. For the 1-tree, the rest is a path
through the cities of $R$, i.e., a spanning tree of $R$, plus its first and
last edge. $\square$

```cpp
--8<-- "generated/BranchAndBound-tspLowerBound.cpp"
```

Our first bound is that of the text (`TspBound::BOOK`), which uses the
least distances into and out of each city over all the cities, as in its
example ($\mathit{lb} = 18$ for $S = 1 , 3$ of figure 19.5 of the book;
there the least distance $A$ is taken from $S[ 0 ]$, which is the same for
symmetric distances). We remark that two values of figure 19.6 of the
book do not follow from this formula (21 for $1 , 2 , 3$ and for $1 , 5$,
where it gives 19 and 20), and we found no rule that gives all the values
of the figure; with the formula, the tree has 25 nodes instead of 23, and
it ends with the same circuit of cost 19 (we suspect a slip in the figure, although we cannot exclude another rule). The second bound is that of the
1-tree of [Held and Karp 1971](Z-bibliography.md#ref-held1971) (exercise
19.10), which replaces the sum by the weight of a minimum spanning tree of
the cities out of $S$; `spanningTreeWeight()` and `oneTree()` compute the
tree of a set of cities and the minimum 1-tree of a city (a minimum
spanning tree of the others plus the two least edges of that city), which
bounds the optimum from below, since each circuit is a 1-tree.

<span id="alg-20-18"></span>**Algorithm 20.18.** The salesman by
branch-and-bound with the bound of the text or of the 1-tree
([Montresor 2025, §19.3.1 and exercise 19.10](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/BranchAndBound-tspLowerBound.pseudo.md"

--8<-- "generated/BranchAndBound-bb.pseudo.md"

--8<-- "generated/BranchAndBound-spanningTreeWeight.pseudo.md"

--8<-- "generated/BranchAndBound-oneTree.pseudo.md"

**The salesman, by the reduction of the matrix.** `littleTsp()` (exercise
19.11, the algorithm of [Little et al. 1963](Z-bibliography.md#ref-little1963))
branches on including or excluding an edge, the entry 0 of the reduced
matrix whose exclusion raises the bound the most, visits the nodes by
least bound with a priority queue, and bounds a node by reducing the rows
and the columns of its matrix to a least entry 0
([Algorithm 20.19](20-complexity.md#alg-20-19)). Subtracting a constant
from a row or a column lowers the cost of each circuit by that constant,
since a circuit uses one entry of each row and one of each column; hence
the sum of the constants subtracted is a lower bound, and the entries 0
left are the edges that a circuit of that cost would use. Including an edge
$( i , j )$ marks its row and its column as done and sets to $\infty$ the
edge that would close a subcircuit, and excluding it sets $M[ i ][ j ] =
\infty$. Since the visit is by least bound, the first complete circuit
extracted is optimal: any other node has a bound at least as large.

<span id="alg-20-19"></span>**Algorithm 20.19.** The algorithm of Little,
Murty, Sweeney and Karel ([Montresor 2025, exercise
19.11](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/BranchAndBound-littleTsp.pseudo.md"

**Cliques and knapsacks.** `maxWeightClique()` (exercise 19.9) extends a
set by the candidates adjacent to all of it, and prunes it when its weight
plus that of all its candidates is not above the best. `knapsackBB()`
(exercise 19.12) takes the objects by decreasing profit per volume, each
in and then out, and prunes a node when the fractional knapsack of the
objects left within the capacity left ([§13.11](13-algorithm.md#sec-13-11))
does not beat the best; this is an upper bound of the best profit of the
objects left, since the fractional greedy is optimal for the relaxation
([Lemma 13.15](13-algorithm.md#lem-13-15)). `knapsackBacktrack()` is the
same visit with the objects in their order and the simple bound, the sum
of the profits still to decide, and `knapsackGreedy()` the heuristic that
takes the objects by decreasing profit per volume while they fit, which is
not optimal in general ([Algorithm 20.20](20-complexity.md#alg-20-20)); the
test `beyond.knapsack-heuristics` checks that the two visits find the
optimum of `knapsack()` and that the greedy finds no more.

<span id="alg-20-20"></span>**Algorithm 20.20.** The clique of largest
weight, and KNAPSACK by branch-and-bound, by backtracking and by the
greedy choice ([Montresor 2025, exercises 19.9 and
19.12](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/BranchAndBound-extend.pseudo.md"

--8<-- "generated/BranchAndBound-bb-2.pseudo.md"

--8<-- "generated/BranchAndBound-bt.pseudo.md"

--8<-- "generated/BranchAndBound-knapsackGreedy.pseudo.md"

## 20.9 Heuristics {#sec-20-9}

A heuristic gives a feasible solution with no guarantee on its cost
([Montresor 2025, §19.4](Z-bibliography.md#ref-montresor2025)); it is
judged by its time and by its results in practice, and the contracts of
`Heuristics.h` check that each result is feasible, not that it is "good".

**Constructive heuristics.** The constructive heuristics of the header are
greedy ([Algorithm 20.21](20-complexity.md#alg-20-21)). `greedyTsp()`
keeps the lightest edges that leave each city with at most two of them
and close no cycle, as `kruskal()` does with an MFSET (chapter 11), and
so it ends with a Hamiltonian chain, which the edge of its two ends
closes; the sort of the $n ( n - 1 ) / 2$ edges gives $O( n^2 \log n )$.
`greedySuperstring()` (exercise 19.13) is the same greedy on the oriented
edges weighed by the overlaps of the strings (`overlap( a , b )`, the
longest suffix of $a$ that is a prefix of $b$), which leaves oriented
paths, after dropping the strings contained in another. `dsatur()`
(exercise 19.19, [Brélaz 1979](Z-bibliography.md#ref-brelaz1979)) colors
first the node that sees the most colors among its neighbours, i.e., the
most constrained, with the least color not on its neighbours, in $O( n^3 )$.
`starAlignment()` (exercise 19.18, after [Gusfield
1993](Z-bibliography.md#ref-gusfield1993)) aligns $k$ strings against the
center, the string with the largest sum of the global scores against the
others, in $O( k^2 \ell^2 )$ time for strings of length at most $\ell$.

<span id="alg-20-21"></span>**Algorithm 20.21.** The greedy heuristics:
the salesman, the superstring, the coloring by saturation degree and the
star alignment ([Montresor 2025, §19.4.1 and exercises 19.13, 19.18 and
19.19](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Heuristics-greedyTsp.pseudo.md"

--8<-- "generated/Heuristics-overlap.pseudo.md"

--8<-- "generated/Heuristics-greedySuperstring.pseudo.md"

--8<-- "generated/Heuristics-dsatur.pseudo.md"

--8<-- "generated/Heuristics-starAlignment.pseudo.md"

**The bisection by buckets.** `minMaxGreedy()` (the min-max-greedy of the
book) grows the two halves $V_0$ and $V_1$ of a bisection in turn, from a
node $a$ in $V_0$ and a node $b$ in $V_1$, each time with the node out of
them that has the fewest edges to the other half and, among those, the
most to its own ([Algorithm 20.22](20-complexity.md#alg-20-22)). The
structure `Buckets` of exercise 19.17 keeps, for $k = 0 , 1$, the nodes not
placed by the number $h$ of their edges to $V_k$: `bucket( k , h )` is a
vector of nodes, each node knows its place in its two buckets, and
`min( k )` and `max( k )` are the least and the largest bucket not empty
([Figure 20.3](20-complexity.md#fig-20-3)). `inc( k , u )` moves $u$ to
the next bucket of side $k$, by taking it out of its bucket (the last node
of the bucket fills its place) and appending it to the next one; `choose(
k )` scans the least bucket of the other side for the node with the most
edges to $V_k$.

![Figure 20.3. The buckets of the side 0 of a `Buckets` before and after `inc(0,2)`: the node 2 leaves the bucket 1, where the last node 3 takes its place, and is appended to the bucket 2, and the largest bucket not empty becomes 2.](../figures/intractable-buckets.svg){#fig-20-3}

```cpp
--8<-- "generated/Heuristics-take.cpp"
```

<span id="lem-20-12"></span>**Lemma 20.12.** *`insert()`, `remove()` and
`inc()` cost $O( 1 )$ amortized time, and `minMaxGreedy()` takes $O( n^2 )$
time on the adjacency matrix, $O( n )$ choices of $O( n )$ time each.*

*Proof.* A node leaves a bucket in $O( 1 )$ by its place, which the node
moved into it inherits. Furthermore, the bounds $\mathit{min}( k )$ and
$\mathit{max}( k )$ move by a scan only when a bucket empties. In
`minMaxGreedy()` all the nodes are inserted at the start, with the count
0, and the counts only grow; hence $\mathit{min}( k )$ never decreases, and
its scans move it forward at most $n + 1$ places in all, while
$\mathit{max}( k )$ grows by one at most per `inc()`, and its scans move
it down at most as many places as it grew, plus $n + 1$. Each node placed updates the counts of its
neighbours, $O( n )$ on the matrix, and each choice scans one bucket.
$\square$

<span id="alg-20-22"></span>**Algorithm 20.22.** The min-max-greedy for
the BISECTION ([Montresor 2025, §19.4.1](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Heuristics-minMaxGreedy.pseudo.md"

**Local search.** A *local search* starts from a solution $\pi$ and moves
to a better solution of its neighbourhood $I( \pi )$ while there is one; it
ends at a *local optimum*, since the cost decreases strictly at each step
and the solutions are finitely many ([Montresor 2025,
§19.4.2](Z-bibliography.md#ref-montresor2025)). For the salesman, the
neighbourhood $I_2( \pi )$ of `twoOpt()` holds the circuits obtained by
removing two edges of $\pi$ that do not touch, and reconnecting the two
paths in the only other way, i.e., by reversing the stretch between them
([Figure 20.4](20-complexity.md#fig-20-4); [Lin
1965](Z-bibliography.md#ref-lin1965)), which gives

$$
\lvert I_2( \pi ) \rvert = \frac{n ( n - 1 )}{2} - n , \tag{20.11}
$$

the pairs of edges less the $n$ that touch. Since the change of cost of a move
depends only on the four edges that change, a step of the search, which takes the
best circuit of the neighbourhood, costs $O( n^2 )$
([Algorithm 20.23](20-complexity.md#alg-20-23)). `twoOptMove( pi , i , j )`
applies one move, reversing $\pi[ i + 1 .. j ]$, and the costs of the
circuits visited go to `*trace`, if given.

![Figure 20.4. A move of `twoOpt()` on eight cities: the edges $( 2 , 3 )$ and $( 5 , 6 )$ of $\pi$ (dashed), which cross, are replaced by $( 2 , 5 )$ and $( 3 , 6 )$ (orange), and the stretch $3 , 4 , 5$ is reversed.](../figures/intractable-twoopt.svg){#fig-20-4}

<span id="alg-20-23"></span>**Algorithm 20.23.** The local search on
$I_2$, the size of $I_k$, and the local search for PARTITION
([Montresor 2025, §19.4.2 and exercises 19.15 and
19.16](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/Heuristics-twoOpt.pseudo.md"

```cpp
--8<-- "generated/Heuristics-twoOpt.cpp"
```

--8<-- "generated/Heuristics-kOptNeighbours.pseudo.md"

--8<-- "generated/Heuristics-partitionLocalSearch.pseudo.md"

`kOptNeighbours()` (exercise 19.15) counts $\lvert I_k( \pi ) \rvert$, the
circuits that share $n - k$ edges with $\pi$, by enumerating all the
circuits in $O( n! )$, which checks (20.11) for $k = 2$.
`partitionLocalSearch()` (exercise 19.16) does the same for PARTITION, with
the moves of one element and the swaps of two, each changing the
difference of the sums by twice the elements moved, the best move first,
in $O( n^2 )$ per step. Arguably, the classical local search for the bisection,
whose moves swap two nodes, is that of [Kernighan and Lin
1970](Z-bibliography.md#ref-kernighan1970); the library does not implement
it.

## 20.10 The fifteen puzzle {#sec-20-10}

In the FIFTEEN PUZZLE (the GIOCO DEL QUINDICI of the book), $n^2 - 1$ tiles
numbered from 1 lie on an $n \times n$ grid with a hole, a move slides a
tile next to the hole into it, and the goal has the tiles in increasing
order and the hole last ([Montresor 2025, §19.5](Z-bibliography.md#ref-montresor2025)).
A configuration is a `Board`, the tile of each square by rows, 0 the hole;
`puzzleGoal()` gives the goal and `puzzleMoves()` the configurations one
move away. `solvePuzzle()` is the branch-and-bound of the text: a node is
a configuration, its children are those one move away, its cost $g( x )$
is the number of moves from the root, its lower bound $h( x )$ is the sum
of the distances of the tiles from their places (`manhattan()`), and the
next node visited is the one of least $f( x ) = g( x ) + h( x )$, by a
Fibonacci heap ([Figure 20.5](20-complexity.md#fig-20-5)). This is the
algorithm A* of [Hart, Nilsson and Raphael
1968](Z-bibliography.md#ref-hart1968) ([Algorithm 20.24](20-complexity.md#alg-20-24)).

![Figure 20.5. A configuration of the $3 \times 3$ puzzle and the moves of the tiles 5 and 8 that reach the goal, with the moves $g$ from the start and the bound $h$ of `manhattan()`; the violet tiles are those out of place, or just moved.](../figures/intractable-puzzle.svg){#fig-20-5}

<span id="alg-20-24"></span>**Algorithm 20.24.** The fifteen puzzle by
A*, by the visit in breadth and by backtracking, and the test of
solvability ([Montresor 2025, §19.5](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/FifteenPuzzle-solvePuzzle.pseudo.md"

```cpp
--8<-- "generated/FifteenPuzzle-solvePuzzle.cpp"
```

<span id="lem-20-13"></span>**Lemma 20.13 (A* with a consistent bound).**
*If $h$ is 0 at the goal and*

$$
h( x ) \le 1 + h( y ) \quad \text{for each move from } x \text{ to } y ,
\tag{20.12}
$$

*then the first goal extracted from the queue is reached in the fewest
moves, and each configuration is expanded at most once.*

*Proof.* A move changes the place of one tile by one square, and so
changes `manhattan()` by at most 1, which is (20.12); then $f$ never
decreases along a path, and $h$ never exceeds the moves left. When a goal
$x$ is extracted, every node of a shorter solution would have $f \le g^* <
g( x ) = f( x )$, and it would have been extracted before $x$, with its
successors. Finally, the map of the least level of each configuration lets a
configuration enter the queue again only with fewer moves, and an entry
with more moves than the map is stale and skipped. $\square$

We made the map a parameter of `solvePuzzle()`, `std::map` by default, and
the same search therefore runs with the maps of the standard library, of Boost or of
the library (e.g., `FlatHashTable`) and visits the same nodes; if a pointer
`visited` is given, the function counts there the nodes extracted. Beyond
the text, two searches work with any goal, e.g., the spiral one of
`puzzleSpiralGoal()` ([Algorithm 20.24](20-complexity.md#alg-20-24)):
`puzzleBFS()` is the case $h = 0$, the visit in breadth, whose first goal
reached is one of the fewest moves, and which examines many more
configurations than A*; `puzzleDFS()` is the backtracking that goes back
when a configuration is already on the current path or the moves exceed a
bound, and it finds a solution of at most that many moves if there is
one, not necessarily the shortest.

--8<-- "generated/FifteenPuzzle-puzzleBFS.pseudo.md"

--8<-- "generated/FifteenPuzzle-extend.pseudo.md"

--8<-- "generated/FifteenPuzzle-solvable.pseudo.md"

<span id="lem-20-14"></span>**Lemma 20.14.** *Let $I$ be the number of
the inversions of the tiles read by rows, the hole left out, and $r$ the
row of the hole. A move keeps the parity of $I$ when $n$ is odd, and that
of $I + r$ when $n$ is even; hence half of the configurations cannot reach
the goal.*

*Proof.* A move of the hole along a row leaves the order of the tiles
unchanged. A move along a column moves one tile past the $n - 1$ tiles
between its two places in the order by rows, which changes $I$ by an
amount of the parity of $n - 1$, and changes $r$ by one. With $n$ odd,
$n - 1$ is even; with $n$ even, $I$ and $r$ change parity together.
$\square$

`solvable()` compares that parity with the one of the goal ([Johnson and
Story 1879](Z-bibliography.md#ref-johnson1879)), which is also sufficient,
as they proved; `solvePuzzle()` calls it first, and answers nothing on a
configuration that cannot reach the goal, instead of exhausting the half
of the configurations that it can reach.

## 20.11 Discussion and references {#sec-20-11}

We followed [Montresor 2025](Z-bibliography.md#ref-montresor2025) closely: the classes P and NP, the nondeterministic algorithms, the reductions and
the chain from BOUNDED TILING are those of [Montresor 2025,
chapter 18](Z-bibliography.md#ref-montresor2025), and the strategies for
the intractable problems those of its chapter 19. As is well known, the NP-completeness of
SATISFIABILITY is due to [Cook 1971](Z-bibliography.md#ref-cook1971) and,
independently, to [Levin 1973](Z-bibliography.md#ref-levin1973), the
chain of the classical problems to [Karp
1972](Z-bibliography.md#ref-karp1972), and the tiles to [Wang
1961](Z-bibliography.md#ref-wang1961); [Garey and Johnson
1979](Z-bibliography.md#ref-garey1979) remains the reference for the
problems and their reductions, and [Garey and Johnson
1978](Z-bibliography.md#ref-garey1978) for the strong NP-completeness. The
linear algorithm for 2-SATISFIABILITY is that of [Aspvall, Plass and Tarjan
1979](Z-bibliography.md#ref-aspvall1979). Among the strategies, the rule of
[McNaughton 1959](Z-bibliography.md#ref-mcnaughton1959) is one of the first
results on preemptive scheduling; the analysis of the first fit is that of
[Johnson et al. 1974](Z-bibliography.md#ref-johnson1974), the double tree
for the triangular salesman that of [Rosenkrantz, Stearns and Lewis
1977](Z-bibliography.md#ref-rosenkrantz1977), and the approximation scheme
of the knapsack that of [Ibarra and Kim 1975](Z-bibliography.md#ref-ibarra1975).
Also, the branch-and-bound by the reduction of the matrix is that of [Little et
al. 1963](Z-bibliography.md#ref-little1963), and the bound of the 1-tree
that of [Held and Karp 1971](Z-bibliography.md#ref-held1971); the
neighbourhoods $I_k$ go back to [Lin 1965](Z-bibliography.md#ref-lin1965),
and the local search for the bisection to [Kernighan and Lin
1970](Z-bibliography.md#ref-kernighan1970). The coloring by saturation
degree is due to [Brélaz 1979](Z-bibliography.md#ref-brelaz1979), the star
alignment to [Gusfield 1993](Z-bibliography.md#ref-gusfield1993), the
algorithm A* to [Hart, Nilsson and Raphael 1968](Z-bibliography.md#ref-hart1968),
and the parity of the fifteen puzzle to [Johnson and Story
1879](Z-bibliography.md#ref-johnson1879). The set cover, of which the cover
of a segment of [§13.11](13-algorithm.md#sec-13-11) is the case on the
line, is approximated within $\ln n$ by the greedy of [Chvátal
1979](Z-bibliography.md#ref-chvatal1979). All the headers of the two areas
are covered by the chapter.

## 20.12 Exercises {#sec-20-12}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Simulate a nondeterministic algorithm by the enumeration of its
   sequences of choices, and count the runs. Test:
   `ch18.sec18.2.nondeterminism`.
2. Write the certificates of CLIQUE, TRAVELING SALESMAN and
   SATISFIABILITY, and decide the three problems by enumeration. Test:
   `ch18.sec18.examples`.
3. Decide whether a graph can be colored with $k$ colors, trying each
   coloring once up to a renaming of the colors ([Montresor 2025, exercise
   18.6](Z-bibliography.md#ref-montresor2025)). Test: `ch18.ex18.6`.
4. Decide KNAPSACK by an enumeration pruned on the volume. Test:
   `ch18.ex18.5`.
5. Decide EXACT COVER BY 3-SETS by an enumeration that skips the sets
   that meet those chosen. Test: `ch18.ex18.7`.
6. Give the bribes to the politicians so that one party gets twice the
   other. Tests: `ch18.ex18.1`, `ch18.ex18.4`.
7. Reduce BOUNDED TILING to CLIQUE, to SATISFIABILITY and to 0-1 INTEGER
   PROGRAMMING, and check the answers on the coverings found by
   enumeration. Test: `ch18.sec18.7.domino`.
8. Reduce CLIQUE to INDEPENDENT SET, HAMILTONIAN CIRCUIT to TRAVELING
   SALESMAN and PARTITION to the numerical problems. Test:
   `ch18.sec18.7.reductions`.
9. Reduce INDEPENDENT SET to VERTEX COVER. Test: `ch18.ex18.11`.
10. Reduce the directed Hamiltonian circuit to the directed Hamiltonian
    path, and to the unoriented one. Tests: `ch18.ex18.13`,
    `ch18.ex18.14`.
11. Find the longest path of an acyclic graph in polynomial time, and
    reduce the Hamiltonian path to the longest path of a general graph.
    Test: `ch18.ex18.15`.
12. Decide the linear tiling in $O( m^2 )$ time, whatever $n$ is. Test:
    `ch18.sec18.5.linear-tiling`.
13. Prove that the convex hull costs $\Omega( n \log n )$ by a reduction
    of sorting. Test: `ch18.sec18.5.hull-sort`.
14. Solve 2-SATISFIABILITY in linear time by the strongly connected
    components of the graph of the implications. Test:
    `conclusion.table19.1.sat`.
15. Find the largest clique of an interval graph. Test:
    `conclusion.intervals`.
16. Solve SUBSET SUM in pseudopolynomial time with one row of bits, and
    read a subset back from the table. Test: `ch19.sec19.1.subset-sum`.
17. Schedule $n$ programs on $m$ processors by the dynamic programming on
    the loads, and by the rule of McNaughton when they may be interrupted.
    Tests: `ch19.ex19.2`, `ch19.ex19.3`.
18. Share a loot among three thieves. Test: `ch19.ex19.5`.
19. Show that the first fit uses fewer than twice the boxes of the
    optimum. Test: `ch19.sec19.2.bin-packing`.
20. Approximate the triangular salesman within a factor 2 by the double
    tree, and show that no constant factor is possible in general. Tests:
    `ch19.sec19.2.delta-tsp`, `ch19.sec19.2.gap`.
21. Approximate SUBSET SUM within $1 / ( h + 1 )$. Test:
    `ch19.sec19.2.ap-subset-sum`.
22. Approximate VERTEX COVER within a factor 2, and KNAPSACK within $1 -
    \epsilon$ in time polynomial in $n$ and $1 / \epsilon$. Tests:
    `ch19.ex19.6`, `ch19.ex19.8`.
23. Solve the salesman by branch-and-bound with the bound of the text
    and with the bound of the 1-tree. Tests: `ch19.sec19.3.bb-tsp`,
    `ch19.ex19.10`.
24. Solve the salesman by the algorithm of Little, Murty, Sweeney and
    Karel. Test: `ch19.ex19.11`.
25. Find a clique of largest weight by branch-and-bound. Test:
    `ch19.ex19.9`.
26. Solve KNAPSACK by branch-and-bound with the fractional bound, and
    compare the nodes with those of the backtracking. Test:
    `ch19.ex19.12`.
27. Build a circuit by the greedy on the edges and improve it by the local
    search on $I_2$. Tests: `ch19.sec19.4.greedy-tsp`,
    `ch19.sec19.4.local-search`.
28. Count the neighbourhood $I_k$ of a circuit. Test: `ch19.ex19.15`.
29. Realize the structure with buckets of the min-max-greedy, and use it
    for the bisection. Tests: `ch19.ex19.17`, `ch19.sec19.4.bisection`.
30. Find a common superstring by the greedy on the overlaps, and color a
    graph by saturation degree. Tests: `ch19.ex19.13`, `ch19.ex19.19`.
31. Solve the fifteen puzzle in the fewest moves by A* with the bound of
    the distances, and decide whether a configuration can reach the goal.
    Test: `ch19.sec19.5.fifteen`.

# 6. Searching {#ch-6}

[Source: `include/ADS/search/BinarySearch.h`, `MonotoneSearch.h`,
`InterpolationSearch.h`, `KnuthSearches.h`, `SortedSearches.h`,
`StaticSearch.h`, `CountLess.h`, `PGMIndex.h`, `CompressedPGMIndex.h`,
`DynamicPGMIndex.h`, `Minimum.h`, `LocalOptima.h`, `Selection.h`,
`NthElement.h`, `RangeMinimum.h`, `WordRangeMinimum.h`]

We are concerned in this chapter with the questions that can be answered
on a vector by looking at few of its items: the place of a key in a
sorted vector, the minimum and the local optima, the item of a given rank,
and the minimum of any range of an array after a preprocessing. We first
treat the searches in a sorted vector, both in the forms of the textbooks
([§6.2](06-search.md#sec-6-2)) and in the forms that count the misses of
the cache and the branches that the processor mispredicts
([§6.3](06-search.md#sec-6-3)). Then come the learned indexes, which
replace the comparisons of the upper levels of a search by a model of the
keys, in the static, compressed and dynamic forms of the PGM-index
([§6.4](06-search.md#sec-6-4) to [§6.6](06-search.md#sec-6-6)). Finally, the
minimum and the local optima ([§6.7](06-search.md#sec-6-7)), the
selection ([§6.8](06-search.md#sec-6-8)) and the range minima
([§6.9](06-search.md#sec-6-9)) precede a comparison of the searches on
what each one is meant for ([§6.10](06-search.md#sec-6-10)).

## 6.1 Definitions and notation {#sec-6-1}

We are given a vector $A[ 0 .. n - 1 ]$ of items of a totally ordered
universe, compared by an order `less()` (the operator $<$ unless one is
given); $A$ is *sorted* if $A[ i ] \le A[ i + 1 ]$ for each $i$. A
*comparison* is the evaluation of the order on two items, and the cost of
a search is the number of its comparisons, the other operations being
proportional to it unless we say otherwise. We call *rank* of an item its
place in the sorted order, counted from 1 in `Selection.h` (as in
[Montresor 2025, §17.1](Z-bibliography.md#ref-montresor2025)) and from 0
in `NthElement.h` and `SortedSearches.h` (as in the standard library). We
write $A[ i .. j ]$ for the items of the places from $i$ to $j$, and
$A[ i .. j )$ for those from $i$ to $j - 1$. Furthermore,
$\mathrm{lowerBound}( v )$ is the first place of an item not smaller than
$v$ ($n$ if there is none), $\mathrm{upperBound}( v )$ the first place of
an item larger than $v$, and $\mathrm{RMQ}( i , j )$, $i \le j$, the
place of the leftmost minimum of $A[ i .. j ]$. Finally, $B$ is the number
of items in a line of cache of 64 bytes (e.g., 16 of 32 bits), and
$\epsilon$ and $\epsilon_r$ are the largest errors, in places, of the
lines of a learned index at its last level and at the others.

A comparison-based search is a *decision tree* ([Montresor 2025,
§2.7.4](Z-bibliography.md#ref-montresor2025)), i.e., a binary tree whose
inner nodes are the comparisons and whose leaves are the answers. A run of
the search is a path from the root to a leaf, and the worst case of the
search is thus the height of its tree. [Table 6.1](06-search.md#tab-6-1)
collects the searches of a key in a sorted vector.

<span id="tab-6-1"></span>**Table 6.1.** The searches of a key in a sorted
vector of `ADS/search` and their comparisons; $h$ is the place of the
answer.

| Function | Comparisons | Source |
|---|---|---|
| `binarySearch()` | $2 ( \lfloor \log_2 n \rfloor + 1 )$ at most | Montresor 2025, §1.2 |
| `lowerBound()`, `upperBound()` | $\lceil \log_2 ( n + 1 ) \rceil$, as (6.1) | Montresor 2025, exercise 12.13 |
| `gallopSearch()`, `exponentialSearch()` | $2 \lceil \log_2 ( h + 1 ) \rceil + O( 1 )$, as (6.3) | Bentley and Yao 1976 |
| `interpolationSearch()` | $O( \log \log n )$ expected on uniform keys, $O( n )$ in the worst case | Montresor 2025, §12.1; Perl et al. 1978 |
| `fibonacciSearch()` | about $1.44 \log_2 n$ | Ferguson 1960; Knuth 1998 |
| `uniformBinarySearch()` | $\lfloor \log_2 n \rfloor + 1$ at most | Knuth 1998 |
| `branchlessLowerBound()` | $\lceil \log_2 n \rceil + 1$, none mispredicted | Khuong and Morin 2017 |
| `SPlusTree::lowerBound()` | $H$ lines of (6.5) | Slotin 2022 |
| `PGMIndex::lowerBound()` | (6.6) | Ferragina and Vinciguerra 2020 |

## 6.2 Searching a sorted vector {#sec-6-2}

The binary search of [Montresor 2025,
§1.2](Z-bibliography.md#ref-montresor2025) (`binarySearch()`) compares $v$
with the middle item $A[ m ]$ of the part $A[ i .. j ]$ still to examine,
and goes on in $A[ i .. m - 1 ]$ if $v < A[ m ]$ and in
$A[ m + 1 .. j ]$ if $v > A[ m ]$ ([Algorithm 6.1](06-search.md#alg-6-1)).
It stops as soon as it meets $v$, at the price of two comparisons per
step, and returns $-1$ when $v$ is not there; an overload calls a function
`trace(i, j, m)` at each step, which prints the table of the example of the
book. In the code, the middle is computed as $m = i + \lfloor ( j - i ) / 2 \rfloor$,
which, unlike $\lfloor ( i + j ) / 2 \rfloor$, cannot overflow (the bug
of `Arrays.binarySearch` of Java until 2003). In turn, `decisionTree()`
builds the decision tree of the search as a `BinaryTree` that the caller
owns (nil on an empty vector), whose root is the middle item of the vector
and whose subtrees are the decision trees of the two halves
([Figure 6.1](06-search.md#fig-6-1)). Its invisit gives $A$, it is a
binary search tree, and all its levels are full but the last one;
its height, the worst case of the search, is therefore $\lfloor \log_2 n \rfloor$.

<span id="alg-6-1"></span>**Algorithm 6.1.** The binary search
([Montresor 2025, §1.2](Z-bibliography.md#ref-montresor2025)), the tail
recursion of the book as a loop, and its decision tree.

--8<-- "generated/BinarySearch-binarySearch.pseudo.md"

--8<-- "generated/BinarySearch-decision.pseudo.md"

![Figure 6.1. A sorted vector of 7 items and the decision tree of the binary search on it, each node labelled by the item it compares (and the place of the inner nodes); a search is a path from the root, and the height of the tree is $\lfloor \log_2 7 \rfloor = 2$.](../figures/search-decision.svg){#fig-6-1}

<span id="lem-6-1"></span>**Lemma 6.1.** *Each comparison-based search
for the place $\mathrm{lowerBound}( v )$ in a sorted vector of $n$ items
makes, in the worst case, at least*

$$
\lceil \log_2 ( n + 1 ) \rceil \tag{6.1}
$$

*comparisons.*

*Proof.* The answer is one of the $n + 1$ places $0 , \ldots , n$, each of
which is the answer for some $v$; the decision tree of the search has
therefore at least $n + 1$ leaves. A binary tree of height $h$ has at most
$2^h$ leaves, whence (6.1). $\square$

`lowerBound()` meets (6.1), and `upperBound()` is the same search with the
test reversed; their difference is the number of the occurrences of $v$
([Montresor 2025, exercise 12.13](Z-bibliography.md#ref-montresor2025);
[Algorithm 6.2](06-search.md#alg-6-2)). Both take any order `less()`, and
`lowerBound()` also searches a range $[ \mathit{lo} , \mathit{hi} )$ of
the vector, which the other searches of the chapter call.

<span id="alg-6-2"></span>**Algorithm 6.2.** The lower bound and the upper
bound ([Montresor 2025, exercise 12.13](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/BinarySearch-lowerBound.pseudo.md"

--8<-- "generated/BinarySearch-upperBound.pseudo.md"

<span id="lem-6-2"></span>**Lemma 6.2.** *`lowerBound()` keeps the
invariant*

$$
A[ \mathit{lo} - 1 ] < v \le A[ \mathit{hi} ] , \tag{6.2}
$$

*with $A[ -1 ] = -\infty$ and $A[ n ] = +\infty$, and returns
$\mathrm{lowerBound}( v )$ after at most $\lceil \log_2 ( n + 1 ) \rceil$
comparisons.*

*Proof.* (6.2) holds at the start, with $\mathit{lo} = 0$ and
$\mathit{hi} = n$. A comparison of $v$ with $A[ m ]$ moves $\mathit{lo}$
to $m + 1$ when $A[ m ] < v$ and $\mathit{hi}$ to $m$ otherwise, which
keeps (6.2) since $A$ is sorted. Each step takes the length $\ell = \mathit{hi} -
\mathit{lo}$ to at most $\lfloor \ell / 2 \rfloor$; the loop thus
ends after at most $\lfloor \log_2 n \rfloor + 1 = \lceil \log_2 ( n + 1 ) \rceil$
comparisons with $\mathit{lo} = \mathit{hi}$, which is the answer by (6.2). $\square$

Its contracts check (6.2) at the result, and that the range is sorted, an
audit that costs $O( n )$ against the $O( \log n )$ of the search:

```cpp
--8<-- "generated/BinarySearch-lowerBound.cpp"
```

When the answer $h$ is near the start, or the length of the vector is not
known, the search of [Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976)
doubles a bound before the binary search (`gallopSearch()`, exercise 12.9
of [Montresor 2025](Z-bibliography.md#ref-montresor2025), and
`exponentialSearch()` and `unboundedSearch()` of `MonotoneSearch.h`). The
probes at the places $0 , 1 , 3 , 7 , \ldots , 2^k - 1$ stop at the first
$k$ with $A[ 2^k - 1 ] \ge v$, i.e., $k = \lceil \log_2 ( h + 1 ) \rceil$,
after $k + 1$ comparisons. In `gallopSearch()` the binary search that
follows runs on the places $2^{k-1}$ to $2^k - 2$, $2^{k-1} - 1$ of them,
since the place $2^k - 1$ is already known to hold an item not smaller
than $v$, and so costs at most $k - 1$ more comparisons
([Algorithm 6.3](06-search.md#alg-6-3) and
[Figure 6.2](06-search.md#fig-6-2)); when the doubling passes the end of
the vector before the place $2^k - 1$, the probes are $k$ and the binary
search runs on fewer places. In all, for $h \ge 1$, at most

$$
2 \lceil \log_2 ( h + 1 ) \rceil \tag{6.3}
$$

comparisons (1 for $h = 0$), which depend on the answer but not on $n$.
[Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976) also show that
applying the same idea to the search of $k$ gives
$\log_2 h + \log_2 \log_2 h + \cdots$ comparisons, close to the lower
bound $\log_2 h$.

![Figure 6.2. `gallopSearch()` for $v = 40$, each probe numbered above in its order: the probes at the places 0, 1, 3 and 7 double the bound until $A[ 7 ] \ge v$, and the binary search on the places 4 to 6 (the brace), which leaves out the place 7 compared by the last doubling, probes the places 5 and 4 and finds the answer $h = 5$, so that the 6 comparisons are those of (6.3) with $k = 3$.](../figures/search-gallop.svg){#fig-6-2}

<span id="alg-6-3"></span>**Algorithm 6.3.** The searches with a bound that
doubles ([Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976)): on a
sorted vector, on a predicate $p$ false and then true on the integers, and
on a sequence of unknown length.

--8<-- "generated/BinarySearch-gallopSearch.pseudo.md"

--8<-- "generated/MonotoneSearch-exponentialSearch.pseudo.md"

--8<-- "generated/MonotoneSearch-unboundedSearch.pseudo.md"

`exponentialSearch()` finds the least $i \ge \mathit{lo}$ at which a
predicate $p$, false and then true, holds (the method $B_1$ of Bentley and
Yao). Its binary search too leaves out the last probe, whose $p$ is known
to be true, and hence makes at most $k - 1$ evaluations after the $k + 1$ of
the probes, within the bound $2 \lceil \log_2 ( i - \mathit{lo} + 1 ) \rceil + 1$
of its header. Its probes are cut at the largest value of the type of the
integers, which they never overflow, and the middle of its binary search is
computed on the unsigned distance. `unboundedSearch()` applies it to a
sequence read one item at a time, whose function `get(i)` gives an optional
item, empty past the end; the typical case is the count of the items
smaller than a value in a sorted sequence of unknown length. The same scheme
searches any monotone predicate on the integers, e.g., the integer square
root (`integerSqrt()`, [Algorithm 6.4](06-search.md#alg-6-4)). It keeps
$\mathit{lo}^2 \le n < \mathit{hi}^2$ on $[ 0 , 2^{w/2} )$ for $w$ bits
and tests $m^2 \le n$ as $m \le \lfloor n / m \rfloor$, so that no
product overflows; since $\mathit{hi} - \mathit{lo}$ starts at $2^{w/2}$
and halves at each step, it makes exactly $w / 2$ steps.
`isPerfectSquare()` compares the square of that root with $n$. (Newton's
iteration $x \gets \lfloor ( x + \lfloor n / x \rfloor ) / 2 \rfloor$
converges quadratically from a start above the root, in
$O( \log \log n )$ steps from a start within a constant factor.)

<span id="alg-6-4"></span>**Algorithm 6.4.** The integer square root by a
binary search with no overflow (Esercizi Divide-et-impera of [Montresor
2025](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/MonotoneSearch-integerSqrt.pseudo.md"

In turn, the search by interpolation of [Montresor 2025,
§12.1](Z-bibliography.md#ref-montresor2025) (`interpolationSearch()`)
probes the place that a linear model of the keys predicts,

$$
m = \mathit{lo} + \Big\lfloor ( \mathit{hi} - \mathit{lo} )
\frac{v - A[ \mathit{lo} ]}{A[ \mathit{hi} ] - A[ \mathit{lo} ]}
\Big\rfloor , \tag{6.4}
$$

instead of the middle one, as one opens a phone book near its start to
look for a name in C ([Algorithm 6.5](06-search.md#alg-6-5)). On keys
drawn independently from the uniform distribution, the expected number of
probes is $\log_2 \log_2 n + O( 1 )$ ([Perl et al.
1978](Z-bibliography.md#ref-perl1978)); roughly, the expected error of
(6.4) on a part of length $\ell$ is of the order of $\sqrt{\ell}$, and each
probe thus takes, in expectation, the square root of the length. On keys
far from uniform (e.g., growing exponentially), however, a probe may
exclude a single item, and the worst case is $O( n )$. Our search stops
when $v$ is out of $[ A[ \mathit{lo} ] , A[ \mathit{hi} ] ]$, and probes
$A[ \mathit{lo} ]$ when $A[ \mathit{lo} ] = A[ \mathit{hi} ]$. In the code, the
fraction is computed in `double` and clamped to the part; when it is
undefined ($0 / 0$ for distinct keys equal as doubles, i.e., integers past
$2^{53}$, or $\infty / \infty$ for infinite keys) the probe is the middle
place. As for `binarySearch()`, an overload reports each probe to a
function `trace(i, j, m)`. Note that the learned indexes of
[§6.4](06-search.md#sec-6-4) refine the same idea with a model whose error
is guaranteed.

<span id="alg-6-5"></span>**Algorithm 6.5.** The search by interpolation
([Montresor 2025, §12.1](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/InterpolationSearch-interpolationSearch.pseudo.md"

Two searches of [Knuth 1998, §6.2.1](Z-bibliography.md#ref-knuth1998v3)
replace the division of the middle by additions
([Algorithm 6.6](06-search.md#alg-6-6)), and both may count their
comparisons in a variable passed by pointer. First, the Fibonacci search
(`fibonacciSearch()`, after Algorithm 6.2.1F and [Ferguson
1960](Z-bibliography.md#ref-ferguson1960)) splits a part of length
$F_{k+1} - 1$ at its $F_{k-1}$-th item, into parts of lengths
$F_{k-1} - 1$ on the left and $F_k - 1$ on the right. This is the mirror
image of Algorithm F, which compares the $F_k$-th item, with the same
lengths and at most about $\log_\phi n \approx 1.44 \log_2 n$ comparisons,
$\phi$ the golden ratio. When $n$ is not a Fibonacci number, a probe past
the end is moved to the last item, as if items larger than all the others
filled the vector up to the next Fibonacci number. Second, the uniform
binary search (`uniformBinarySearch()`, Algorithms 6.2.1U and 6.2.1C) keeps
a place $i$ and the width $\delta_j = \lfloor ( n + 2^{j-1} ) / 2^j \rfloor$
of the step $j$, taken from a table computed once. A step adds or
subtracts the next width, and the search thus moves on a fixed tree of
differences with no endpoint to update, in at most
$\lfloor \log_2 n \rfloor + 1$ comparisons; its places 0 and $n + 1$ stand
for the keys $-\infty$ and $+\infty$ of the book, and are compared without
reading the vector.

<span id="alg-6-6"></span>**Algorithm 6.6.** The Fibonacci search and the
uniform binary search ([Knuth 1998, Algorithms 6.2.1F, 6.2.1U and
6.2.1C](Z-bibliography.md#ref-knuth1998v3)).

--8<-- "generated/KnuthSearches-fibonacciSearch.pseudo.md"

--8<-- "generated/KnuthSearches-uniformBinarySearch.pseudo.md"

Finally, the searches of `SortedSearches.h` apply the invariant (6.2) to
other predicates that are false and then true along a sorted vector
([Algorithm 6.7](06-search.md#alg-6-7)). In a strictly increasing vector
rotated by an unknown number of places, the minimum (`rotatedMinimum()`)
is at the first place $m$ with $A[ m ] \le A[ n - 1 ]$, and the search,
which compares $A[ m ]$ with the last item of the part, finds it in
$\lceil \log_2 n \rceil$ comparisons. With repeated items no search beats
$\Omega( n )$ comparisons in the worst case, as a vector of equal items but
one shows, and the items must therefore be distinct. Similarly, the item
closest to a value $x$ (`closestValue()`) is $A[ r ]$ or $A[ r - 1 ]$ for
$r = \mathrm{lowerBound}( x )$; the smaller one wins a tie, the first of
its equal items is returned, and the distances are computed, for the
integers, on the unsigned type of the same width, which cannot overflow.
Two places with a given sum (`pairWithSum()`) are found by two indices
from the ends in $O( n )$ time, each step excluding all the pairs of the
item left behind, with the sum computed in the type of the target, possibly
wider than that of the items. Also, the item of rank $k$ of the union of
two sorted vectors $X$ and $Y$ (`kthOfTwoSorted()`, [Frederickson and
Johnson 1982](Z-bibliography.md#ref-frederickson1982)) is found by a binary
search of the number $i$ of the $k + 1$ smallest items taken from the
shorter vector $X$. The right $i$ is the first one with
$X[ i ] \ge Y[ k - i ]$, and then $X[ i - 1 ] \le Y[ j ]$ and
$Y[ j - 1 ] \le X[ i ]$ for $j = k + 1 - i$; the search costs
$O( \log \min( m , n ) )$ comparisons, and the two medians of two sorted
vectors of $n$ items are the items of ranks $n - 1$ and $n$.

<span id="alg-6-7"></span>**Algorithm 6.7.** The searches of
`SortedSearches.h` (Esercizi Divide-et-impera and Esercizi vari of
[Montresor 2025](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/SortedSearches-rotatedMinimum.pseudo.md"

--8<-- "generated/SortedSearches-closestValue.pseudo.md"

--8<-- "generated/SortedSearches-pairWithSum.pseudo.md"

--8<-- "generated/SortedSearches-kthOfTwoSorted.pseudo.md"

## 6.3 Searches laid out for the memory {#sec-6-3}

On a modern processor the comparisons of a binary search typically cost less
than the two events that surround them. Its first levels miss the cache,
since the places probed are far apart, and the branch that follows each
comparison is mispredicted half of the times on random keys. We now see
how `StaticSearch.h` and `PGMIndex.h` remove the two costs, one at a time.

The binary search with no branch of [Khuong and Morin
2017](Z-bibliography.md#ref-khuong2017) (`branchlessLowerBound()`) keeps a
base $b$ and a length $\ell$. At each step it sets $b \gets b + h$ if
$A[ b + h ] < v$, with $h = \lfloor \ell / 2 \rfloor$, and
$\ell \gets \ell - h$ whatever the outcome; the comparison thus feeds a
conditional move instead of a jump. Since the two places that the next
step may probe are known, both are prefetched, and the misses of two steps
overlap ([Algorithm 6.8](06-search.md#alg-6-8)).

<span id="alg-6-8"></span>**Algorithm 6.8.** The binary search with no
branch ([Khuong and Morin 2017](Z-bibliography.md#ref-khuong2017)); the
part still to examine is $A[ b .. b + \ell - 1 ]$, and $[ \cdot ]$ is 1
when its condition holds and 0 otherwise.

--8<-- "generated/StaticSearch-branchlessLowerBound.pseudo.md"

```cpp
--8<-- "generated/StaticSearch-branchlessLowerBound.cpp"
```

<span id="lem-6-3"></span>**Lemma 6.3.** *On a sorted vector of $n > 0$
items, `branchlessLowerBound()` returns $\mathrm{lowerBound}( v )$ after
$\lceil \log_2 n \rceil + 1$ comparisons, whatever $v$.*

*Proof.* The invariant is that the answer is in $[ b , b + \ell ]$, that
the items before $b$ are smaller than $v$, and that $b + \ell \le n$; it
holds at the start, with $b = 0$ and $\ell = n$. If $A[ b + h ] < v$, the
answer is past $b + h$, and the new range $[ b + h , b + \ell ]$ holds it;
otherwise the answer is at most $b + h$, which is within the new range
$[ b , b + \ell - h ]$ since $h \le \ell - h$. In both cases $b + \ell$
does not grow. The length goes from $\ell$ to $\lceil \ell / 2 \rceil$,
and reaches 1 after $\lceil \log_2 n \rceil$ steps; then the answer is
$b$ or $b + 1$, with $b < n$, and the last comparison decides between
them. $\square$

The S+ tree of [Slotin 2022](Z-bibliography.md#ref-slotin2022)
(`SPlusTree<T, Less, Bytes>`) removes the misses instead. A copy of its
sorted items, cut into nodes of $B$ items (the items in `Bytes` bytes, 64
by default, and at least 2), forms the leaves of a static B+-tree whose
inner nodes have $B$ keys and $B + 1$ children, the key $j$ of a node being
the first item under its child $j + 1$. In memory, the children of the node
$k$ of a level are the nodes $k ( B + 1 ) , \ldots , k ( B + 1 ) + B$ of
the next level, so that no pointer is stored, and the levels follow one
another, the leaves last ([Figure 6.3](06-search.md#fig-6-3)). The level of
the leaves has $s_{H-1} = \lceil n / B \rceil$ nodes and each level above
$s_\ell = \lceil s_{\ell+1} / ( B + 1 ) \rceil$, up to the root; there
are thus

$$
H = 1 + \Big\lceil \log_{B+1} \Big\lceil \frac{n}{B} \Big\rceil
\Big\rceil \tag{6.5}
$$

levels, and a search reads one line per level, against the
$\log_2 n - \log_2 B$ misses of the binary search (5 lines for a million
keys of 32 bits, against 20 probes, of which the last 4 share a line).

![Figure 6.3. An `SPlusTree` with $B = 4$ on 16 sorted items: the root, whose key $j$ is the first item of the leaf $j + 1$ (the last key, with no leaf, padded with $\infty$), over the four leaves, which are the items themselves; the search of $v = 26$ counts 2 keys smaller than $v$ in the root and 2 in the leaf 2 (violet), i.e., the place $2 B + 2 = 10$.](../figures/search-splus.svg){#fig-6-3}

In a node, the child to take is the count of the keys smaller than $v$.
On the arithmetic keys under `std::less`, the count runs on the whole line
with no branch, the places past the last item holding the largest value of
the type (or $+\infty$). For the integers of 32 and 64 bits and the floating
types, `countLess()` of `CountLess.h` (in `ADS::detail`) makes it one or
two comparisons of vectors of AVX-512 or of AVX2, each giving a mask of bits
that a count of the bits sums; with AVX2, which has no unsigned comparison,
the unsigned integers are compared as signed ones after flipping their top
bit. Elsewhere, and on the other types, it is the loop of the comparisons,
which the compiler may vectorize or not, and on the other orders a scan of
the keys in use. Its companion `greaterBits()` gives the items larger than a
key as the bits of a word, which [§6.9](06-search.md#sec-6-9) uses
([Algorithm 6.9](06-search.md#alg-6-9)).

<span id="alg-6-9"></span>**Algorithm 6.9.** The layout and the search of
`SPlusTree` ([Slotin 2022](Z-bibliography.md#ref-slotin2022)), and the
count with no branch in a node; the levels are numbered from the root, 0,
to the leaves, $H - 1$.

--8<-- "generated/StaticSearch-SPlusTree-layout.pseudo.md"

--8<-- "generated/StaticSearch-SPlusTree-lowerBound.pseudo.md"

--8<-- "generated/CountLess-countLess.pseudo.md"

--8<-- "generated/CountLess-greaterBits.pseudo.md"

A search is thus a loop of counts, one per level, and `upperBound()` counts
the keys not larger than $v$ instead:

```cpp
--8<-- "generated/StaticSearch-search.cpp"
```

Besides the two bounds, the tree answers `contains()` by `lowerBound()`,
reads the item of a place by `operator[]`, and gives `size()`, `empty()`
and its `height()`, i.e., $H$.

<span id="lem-6-4"></span>**Lemma 6.4.** *An `SPlusTree` on $n$ items has
the height $H$ of (6.5), takes about $n ( B + 1 ) / B$ places of items,
and is built in $O( n )$ time; a search reads $H$ lines of cache.*

*Proof.* Level $\ell$ has $s_\ell$ nodes of $B$ items each, and
$B \sum_\ell s_\ell \approx n ( 1 + 1 / ( B + 1 ) + 1 / ( B + 1 )^2 +
\cdots ) = n ( B + 1 ) / B$. Each item is copied once into the leaves, and
each key once from the leaves. The nodes are aligned to a line, and the
search reads one node per level. $\square$

Its space is on huge pages when it reaches 2 MB (cf.
[§5.9](05-memory.md#sec-5-9)), and a search therefore misses the TLB at
most once. In two points the S+ tree differs from the B-tree layout of [Khuong and Morin
2017](Z-bibliography.md#ref-khuong2017): the leaves are the
array itself, the inner keys only routing the search, and a node is
searched by a count instead of a search. The layout of Eytzinger (a heap
order of the items) also reads one line for the last 4 levels of the
binary search, and with the prefetch of the 16 descendants 4 levels down
it hides the latency of the misses instead of avoiding them, at the price
of the bandwidth. [Khuong and Morin 2017](Z-bibliography.md#ref-khuong2017)
found it the fastest of the layouts of the binary search, and the B-tree
layout, with a node per line, faster on the arrays that do not fit in the
cache. B-trees with no pointers go back to the CSS trees of [Rao and
Ross 1999](Z-bibliography.md#ref-rao1999). Later, the FAST trees of [Kim et
al. 2010](Z-bibliography.md#ref-kim2010) laid out a binary tree in blocks
of lines and pages and searched a block with SIMD comparisons. In a lookup
among the classic, the compressed and the learned indexes of integers, the
comparative study of [Bellomo et al. 2025](Z-bibliography.md#ref-bellomo2025)
finds the B-trees with SIMD searches the fastest.

## 6.4 The PGM-index {#sec-6-4}

The PGM-index of [Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020) (`PGMIndex<K, E, R>`, on fewer
than $2^{32}$ arithmetic keys) replaces the inner nodes of a B+-tree by a
model of the keys. Its items $A[ 0 .. n )$ are the points $( A[ i ] , i )$
of the plane, a repeated key standing for its first place. An
*$\epsilon$-approximation* is a sequence of segments, each a line
$f( x ) = a + b ( x - k_s )$ over a range of keys from its first key $k_s$,
such that $| f( A[ i ] ) - i | \le \epsilon$ for each key of its range
([Figure 6.4](06-search.md#fig-6-4)). In turn, the first keys of the
segments are approximated, with error $\epsilon_r$, by the segments of a
level above, and so on up to a single segment. By default, $\epsilon = 64$
and $\epsilon_r = 4$.

![Figure 6.4. The points $( A[ i ] , i )$ of 14 keys covered by two segments whose lines predict each place within $\epsilon = 2$ (the light bands); a search predicts the place of $v$ on the line of its segment and searches the window of the places within the error.](../figures/search-pgm.svg){#fig-6-4}

The fewest segments, i.e., the *optimal* approximation, are found in one
pass by the algorithm of [O'Rourke 1981](Z-bibliography.md#ref-orourke1981)
as engineered by [Xie et al. 2014](Z-bibliography.md#ref-xie2014)
([Algorithm 6.10](06-search.md#alg-6-10)). Indeed, the lines that pass
within $\epsilon$ of the points of a range are those between the line of
least slope and the line of largest slope that the convex hulls of the
points moved up and down by $\epsilon$ admit. A new point updates the two
hulls in amortized $O( 1 )$ time, and a range ends when a point leaves no
line. A segment keeps the line of the average of the two extreme slopes
through the point where they cross, since any line of slope between them
through that point stays within the error; its slope is made not negative,
so that the prediction never decreases. To make all this exact, the keys
are first mapped to integers of 64 bits in the same order (the signed ones
by flipping the top bit, the floating ones by their bits), and the hulls are
kept with products of 128 bits.

<span id="alg-6-10"></span>**Algorithm 6.10.** The construction and the
search of `PGMIndex` (the optimal approximation of [O'Rourke
1981](Z-bibliography.md#ref-orourke1981) and [Xie et al.
2014](Z-bibliography.md#ref-xie2014), and the search of [Ferragina and
Vinciguerra 2020](Z-bibliography.md#ref-ferragina2020)).

--8<-- "generated/PGMIndex-approximate.pseudo.md"

--8<-- "generated/PGMIndex-lowerBound.pseudo.md"

<span id="lem-6-5"></span>**Lemma 6.5.** *Each segment of the optimal
$\epsilon$-approximation but the last covers keys whose first places span
at least $2 \epsilon + 1$ places of $A$ (at least $2 \epsilon + 1$ keys
when the keys are distinct); hence the last level has
$m \le \lceil n / ( 2 \epsilon + 1 ) \rceil$ segments, and there are at most
$1 + \lceil \log_{2 \epsilon_r + 1} m \rceil$ levels. On distinct keys a
search costs*

$$
O( \log \epsilon_r \cdot \log_{2 \epsilon_r + 1} m + \log \epsilon )
\tag{6.6}
$$

*time, in a space of 28 bytes per segment besides the items.*

*Proof.* We first show that the greedy, which closes a segment only when
no line is left, is optimal. If the points of a range are within
$\epsilon$ of a line, so are those of any of its suffixes; hence, by
induction on $j$, the $j$-th segment of the greedy ends no earlier than the
$j$-th segment of any other approximation, since it starts no later, and
the greedy uses the fewest segments. Now let a segment start at a point of
first place $y$. Each later point whose first place is at most
$y + 2 \epsilon$ is within $\epsilon$ of the horizontal line through
$y + \epsilon$; the segment is thus not closed before a point of first
place larger than $y + 2 \epsilon$, which gives the first claim and the
bound on $m$. The points of a level above are the first keys of the level
below with the places $0 , 1 , 2 , \ldots$, which are distinct, and the
same argument with $\epsilon_r$ divides the segments by at least
$2 \epsilon_r + 1$ per level. As for the search, let
$k_s \le v < k_{s+1}$ for two first keys of the level below. The line does
not decrease, and it is within $\epsilon_r$ of $s$ at $k_s$ and of $s + 1$
at $k_{s+1}$; hence its prediction at $v$, truncated to an integer $p$,
satisfies $p - \epsilon_r - 1 \le s \le p + \epsilon_r + 1$. The code
searches the $\epsilon_r + 2$ places on each side of $p$, one more for the
rounding of the line in `double`, by a binary search of $O( \log \epsilon_r )$
comparisons. At the last level, on distinct keys, the same argument puts the
answer within $\epsilon + 1$ places of the prediction, in a window of
$2 \epsilon + 5$ places. Finally, a segment keeps its first key (8 bytes),
the two doubles of its line (16) and its first place (4). $\square$

At the last level, the window has its lines of cache prefetched at once, so
that their misses overlap, and is searched by a binary search with no
branch. On repeated keys, whose places a line cannot all approximate, the
answer may be out of the window, and the search then goes on past it by
galloping, at a cost logarithmic in the distance. Since the items are a copy
on huge pages, the window adds no miss of the TLB:

```cpp
--8<-- "generated/PGMIndex-lowerBound.cpp"
```

Like the S+ tree, the index answers `contains()`, `operator[]`, `size()`,
`empty()` and `height()` (its levels), and also gives `segments()`, the
segments $m$ of the last level, and `bytes()`, the space of the levels
without the items. Note that this space is proportional to $m$, which on
regular keys is far smaller than $n / ( 2 \epsilon )$: on keys in
arithmetic progression, one segment covers them all. This is the point of
a learned index, whose space depends on the regularity of the keys more
than on their number.

## 6.5 The compressed PGM-index {#sec-6-5}

The compressed PGM-index of [Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020) (`CompressedPGMIndex<K, E>`)
keeps the segments in a few tens of bits each instead of 28 bytes, with
the same search at the last level. Consider the lines that are within
$\epsilon$ of the $k$ points of a segment, i.e., the pairs
$( \alpha , \beta )$ with $| \beta + \alpha x_i - y_i | \le \epsilon$ for
each of its points. They are the intersection of $2 k$ half-planes, a convex
region, whose shadow on the axis of the slopes is the interval
$[ \alpha^- , \alpha^+ ]$ between the two extreme lines of the algorithm of
O'Rourke. Each slope of the interval has an intercept within the error, the
middle of

$$
\Big[ \max_i ( y_i - \alpha x_i ) - \epsilon ,
\min_i ( y_i - \alpha x_i ) + \epsilon \Big] ; \tag{6.7}
$$

hence a set of slopes serves all the segments if it hits each of their
intervals ([Figure 6.5](06-search.md#fig-6-5)).

![Figure 6.5. The intervals of the slopes of five segments, sorted by their left ends, and the two groups of the greedy stabbing (segments 1 to 3, and 4 and 5): the violet strips are the intersections of the groups, and the slope of each group, $S[ 0 ]$ and $S[ 1 ]$, is the middle of its intersection.](../figures/search-compressed.svg){#fig-6-5}

<span id="lem-6-6"></span>**Lemma 6.6.** *Sort the intervals by their left
ends, and grow a group while the next interval meets the intersection of
the group, opening a new group otherwise. One slope per group, in its
intersection, hits all the intervals, and no set of fewer slopes does.*

*Proof.* Each interval of a group contains the intersection of the group,
which is not empty by construction. In each group, let $I_g$ be the
interval of least right end, which is the right end of the intersection;
the first interval of the next group begins after it, and so do all the
later ones, since the left ends increase. The intervals $I_g$ are thus
pairwise disjoint, and a slope hits at most one of them; as many slopes as
groups are therefore needed. $\square$

Each segment then keeps the index of its slope and its intercept by (6.7),
as a distance from its first place of at most $\epsilon + 1$ (at most
$\epsilon$ with the exact slope, one more for its rounding). The first
places of the segments, increasing, are kept by Elias-Fano (cf.
[§22.6](22-succinct.md#sec-22-6)), and their first keys whole, searched by
a PGM-index of their own (with $\epsilon = \epsilon_r = 4$) in place of the
levels above ([Algorithm 6.11](06-search.md#alg-6-11)). With $m$ segments
and $\sigma$ slopes, this takes

$$
64 + 2 + \Big\lceil \log_2 \frac{n}{m} \Big\rceil +
\lceil \log_2 \sigma \rceil + \lceil \log_2( 2 \epsilon + 3 ) \rceil
\tag{6.8}
$$

bits per segment, plus the segments of the PGM-index of the keys, the
directory of Elias-Fano and the table of the slopes, $32 \sigma$ bits in
single precision (as in the implementation of the paper).

<span id="alg-6-11"></span>**Algorithm 6.11.** The construction and the
search of `CompressedPGMIndex` ([Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020)); $k_s$ and $p_s$ are the first
key and the first place of the segment $s$.

--8<-- "generated/CompressedPGMIndex-build.pseudo.md"

--8<-- "generated/CompressedPGMIndex-lowerBound.pseudo.md"

Elias-Fano on the first keys would save $62 - \log_2( u / m )$ bits a
segment, for keys in a universe of size $u$. Yet, its rank (the search of
a segment) scans the bucket of the key, and on skewed keys most of the
first keys crowd into a few buckets. In contrast, the PGM-index of the keys
finds the segment in the time of the levels of the plain PGM-index, and a
search adds to that two selects and a field of bits. However, the rounding of a
shared slope to a `float` may take a line out of the error; the
construction checks each segment, and gives its own slope (appended to the
table) to one whose line misses a point by more than $\epsilon + 1$. Besides
`lowerBound()`, the class has the same accessors as `PGMIndex` but
`height()`, and gives the number of its slopes by `slopes()`.

## 6.6 The dynamic PGM-index {#sec-6-6}

The dynamic PGM-index (`DynamicPGMIndex<K, V, E, B, I>`) is an ordered map
from arithmetic keys to values, with insertions and removals, made of
static PGM-indexes by the logarithmic method of [Bentley and Saxe
1980](Z-bibliography.md#ref-bentley1980). The pairs lie in a sorted buffer
$L_0$ of $b_0 = 128$ pairs (`kBuffer`) and in sorted levels
$L_1 , L_2 , \ldots$ of capacities $c_t = b_0 B^t$ ($B = 8$ by default),
each key at most once per level. A key may be in several levels, and its
pair in the newest one (the buffer first, then by increasing $t$) is the
one that counts; a removal (`erase()`) inserts a *tombstone*, a pair that
marks the key as absent. When an insertion finds the buffer full, the
buffer is merged, with the new pair and with the levels before $L_t$, into
the first level $L_t$ that has room for them all
([Algorithm 6.12](06-search.md#alg-6-12) and
[Figure 6.6](06-search.md#fig-6-6)). A constructor puts a vector of pairs,
sorted by distinct keys, into the one level that holds them.

![Figure 6.6. A `DynamicPGMIndex` with $b_0 = 4$ and $B = 2$ (for the figure; the defaults are 128 and 8): the buffer $L_0$ and the levels $L_1$ and $L_2$, of capacities 8 and 16, before and after an insertion that finds the buffer full and merges it, with the new pair, into $L_1$, which has room for them (violet); `x` is a tombstone, kept since $L_1$ is not the last level, and the places in the faintest tint are free.](../figures/search-dynamic.svg){#fig-6-6}

<span id="alg-6-12"></span>**Algorithm 6.12.** The operations of
`DynamicPGMIndex` (the logarithmic method of [Bentley and Saxe
1980](Z-bibliography.md#ref-bentley1980)); $\| L \|$ is the number of the
pairs of a level and $c_i = b_0 B^i$ the capacity of $L_i$.

--8<-- "generated/DynamicPGMIndex-insert.pseudo.md"

--8<-- "generated/DynamicPGMIndex-find.pseudo.md"

--8<-- "generated/DynamicPGMIndex-lowerBound.pseudo.md"

<span id="lem-6-7"></span>**Lemma 6.7.** *An insertion or a removal costs
$O( B \log_B( n / b_0 ) )$ amortized time, $n$ the pairs inserted, and a
lookup searches $O( \log_B( n / b_0 ) )$ levels.*

*Proof.* $L_t$ is chosen because the pairs of the buffer and of the levels
before $L_{t-1}$ do not fit in the room left in $L_{t-1}$; with those of
$L_{t-1}$, they are thus more than $c_{t-1} = c_t / B$ (more than $b_0$
for $t = 1$, since the buffer holds $b_0 + 1$ pairs with the new one). The
merge and the construction of the PGM-index of the result are linear,
$O( c_t )$, i.e., $O( B )$ per pair brought into $L_t$. A pair is brought
into a level only from the levels before it, hence into each level at most
once. Finally, a level $L_t$ is created only when more than $c_{t-1}$
pairs exist, and therefore $t < 1 + \log_B( n / b_0 )$. $\square$

A lookup (`find()`, which gives a pointer to the value, nil if the key is
absent, valid until the next change, and `contains()`) stops at the newest
level that has the key. In turn, `lowerBound()` and the visit of a range
(`forEach()`) merge the levels from their places of the key, and skip the
keys whose newest pair is a tombstone. Tombstones are dropped when the
merge lands on the last level, where no older pair is left for them to
hide. Several choices of the layout fix the constants. As in the
implementation of the paper, only the levels of capacity at least $I$
(`kIndexed`, $2^{24}$ by default) have a PGM-index, with $\epsilon = E$
(16 by default) and $\epsilon_r = 4$, since on the smaller ones a model
does not pay for its construction at each merge; the others are searched by
the binary search with no branch of [§6.3](06-search.md#sec-6-3). The keys
and the values of a level are two arrays, so that a search reads only the
keys, and the tombstones are a sorted list of their keys apart (they are
usually few). A level of capacity at least $2^{12}$ (`kFiltered`) also has
a blocked filter of Bloom of its keys (cf. [§10.8](10-set.md#sec-10-8)),
10 bits per key, as the levels of the stores by log-structured merge have
([O'Neil et al. 1996](Z-bibliography.md#ref-oneil1996); [Dayan et al.
2017](Z-bibliography.md#ref-dayan2017)). A lookup of a key that is not
there thus reads one line of cache per level instead of searching each,
and one that is there reads a line per newer level and searches its own
(false positives aside). The filter of a level takes the keys that a merge
brings in, and is built again, for twice the keys, only when it is full;
the filters thus add a constant number of insertions into a filter per key
and level to the cost of [Lemma 6.7](06-search.md#lem-6-7). The number of
distinct keys is not kept, since it would cost a lookup per insertion.
Instead, `entries()` counts the pairs stored, tombstones and older pairs included,
and `levels()` the levels below the buffer, empty or not.

## 6.7 The minimum and the local optima {#sec-6-7}

`min()` of `Minimum.h` keeps the minimum so far and compares it with each
of the other items, and `argmin()` gives the place of the first minimum of
a range, as the selection sort of [Montresor 2025,
§2.7.1](Z-bibliography.md#ref-montresor2025) needs
([Algorithm 6.13](06-search.md#alg-6-13)).

<span id="lem-6-8"></span>**Lemma 6.8.** *Each comparison-based algorithm
that finds the minimum of $n$ distinct items makes at least $n - 1$
comparisons ([Montresor 2025, §2.6](Z-bibliography.md#ref-montresor2025)),
and `min()` makes $n - 1$. The minimum and the maximum together need*

$$
\Big\lceil \frac{3 n}{2} \Big\rceil - 2 \tag{6.9}
$$

*comparisons, which `minMax()` makes ([Pohl
1972](Z-bibliography.md#ref-pohl1972)).*

*Proof.* An item that has never lost a comparison may still be the
minimum, and a comparison makes at most one item lose for the first time;
hence $n - 1$ comparisons are needed to leave a single item that never
lost. For the minimum and the maximum, each item but one must lose once,
and each item but one must win once; we call these $2 n - 2$ first losses
and first wins the units of information. An adversary keeps a value for
each item and answers by it, with the freedom to raise an item that never
lost and to lower an item that never won, which leaves all the past
answers true. When an item that never lost meets one that has lost, the
former wins (raised if needed); when two items that have both lost (or
both never lost, at most one of them new) meet, it answers so that at most one of
them gets its first win (lowering one that never won) or its first loss
(raising one that never lost). Each comparison thus gives at most one unit,
except a comparison of two items never compared, which gives two. There are
at most $\lfloor n / 2 \rfloor$ such comparisons, and hence at least
$\lfloor n / 2 \rfloor + ( 2 n - 2 - 2 \lfloor n / 2 \rfloor ) =
\lceil 3 n / 2 \rceil - 2$ comparisons. `minMax()` pairs the items, compares
the two of a pair, and then the smaller with the minimum and the larger with
the maximum, 3 comparisons per pair, after 1 for the first pair when $n$ is
even, which gives (6.9) for $n$ even and odd. $\square$

<span id="alg-6-13"></span>**Algorithm 6.13.** The minimum ([Montresor
2025, §1.2 and §2.7.1](Z-bibliography.md#ref-montresor2025)), and the
minimum and the maximum together by pairs and by halves ([Pohl
1972](Z-bibliography.md#ref-pohl1972)).

--8<-- "generated/Minimum-argmin.pseudo.md"

--8<-- "generated/Minimum-min.pseudo.md"

--8<-- "generated/Minimum-minMax.pseudo.md"

--8<-- "generated/Minimum-minMaxRec.pseudo.md"

The divide et impera of `minMaxDivide()` reaches (6.9) when $n$ is a
power of 2, by the recurrence $T( n ) = 2 T( n / 2 ) + 2$, $T( 2 ) = 1$,
whose solution is $3 n / 2 - 2$; on other lengths it may make a few more
(8 instead of 7 for $n = 6$).

A local optimum is cheaper than the minimum. A *peak* of $A$ is an item
not smaller than its neighbours, the places out of $A$ counting as
$-\infty$; one always exists (e.g., the maximum). `findPeak()` of
`LocalOptima.h` keeps the invariant that $A[ \mathit{lo} .. \mathit{hi} ]$
holds a peak of $A$, which is true when $A[ \mathit{lo} - 1 ] < A[ \mathit{lo} ]$
(or $\mathit{lo} = 0$) and $A[ \mathit{hi} ] \ge A[ \mathit{hi} + 1 ]$ (or
$\mathit{hi} = n - 1$). Indeed, under these conditions the maximum of the
part is a peak of $A$: inside the part its neighbours are not larger, and
at an end the neighbour out of the part is smaller or not larger by the
conditions ([Algorithm 6.14](06-search.md#alg-6-14)).

<span id="lem-6-9"></span>**Lemma 6.9.** *`findPeak()` returns a peak of
$A$ after at most $\lceil \log_2 n \rceil$ comparisons.*

*Proof.* Comparing $A[ m ]$ with $A[ m + 1 ]$ keeps the invariant on one
of the two halves: if $A[ m ] < A[ m + 1 ]$, the part
$A[ m + 1 .. \mathit{hi} ]$ rises into $m + 1$; otherwise the part
$A[ \mathit{lo} .. m ]$ falls out of $m$. A part of $\ell$ items leaves at
most $\lceil \ell / 2 \rceil$, and a part of one item is a peak by the
invariant. $\square$

<span id="alg-6-14"></span>**Algorithm 6.14.** A peak and a local minimum
of a vector, and a local minimum of a binary tree (Esercizi
Divide-et-impera and Esercizi Alberi of [Montresor
2025](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/LocalOptima-findPeak.pseudo.md"

--8<-- "generated/LocalOptima-localMinimum.pseudo.md"

--8<-- "generated/LocalOptima-localMinimum-2.pseudo.md"

A peak thus costs $\lceil \log_2 n \rceil$ comparisons, against the
$n - 1$ of the maximum, and $\Omega( \log n )$ are needed in the worst
case; `localMinimum()` of a vector is the same search with the order
reversed. On a binary tree, the walk down to a child smaller than the
current node (the left one first) gives a local minimum, a node smaller
than its parent and not larger than its children, in $O( h )$ comparisons,
$h$ the height. The same idea works in two dimensions, where a peak of an
$n \times n$ matrix costs $\Theta( n )$ probes by the search on the middle
row and the middle column alternately. On general graphs, however, no such
bound exists, since a local minimum on the hypercube of dimension $d$ needs
about $2^{d / 2}$ evaluations, even for a randomized algorithm ([Aldous
1983](Z-bibliography.md#ref-aldous1983)).

## 6.8 The selection {#sec-6-8}

The item of rank $k$ can be found without sorting. The selection of [Hoare
1961](Z-bibliography.md#ref-hoare1961) (`selection()` of `Selection.h`,
[Montresor 2025, §17.1](Z-bibliography.md#ref-montresor2025)) partitions
the part around a pivot as quicksort does, and goes on only in the side
that holds the rank $k$ ([Algorithm 6.15](06-search.md#alg-6-15)). The
pivot is chosen at random when a generator is given, and is the first
item of the part otherwise. With a random pivot its rank is uniform, the
side kept is in the worst case the larger one, and the expected number of
comparisons satisfies

$$
T( n ) \le n - 1 + \frac{2}{n} \sum_{q = \lfloor n / 2 \rfloor}^{n - 1}
T( q ) . \tag{6.10}
$$

The sum of $q$ over the range is at most $3 n^2 / 8$, and substituting
$T( q ) \le c q$ thus gives $T( n ) \le n - 1 + 3 c n / 4$, i.e.,
$T( n ) \le c n$ for $c = 4$; hence the selection is linear in
expectation on every input, while a pivot that is always the smallest
item gives $\Theta( n^2 )$.

<span id="alg-6-15"></span>**Algorithm 6.15.** The selection of [Hoare
1961](Z-bibliography.md#ref-hoare1961) ([Montresor 2025,
§17.1](Z-bibliography.md#ref-montresor2025)) and the median of the
medians ([Blum et al. 1973](Z-bibliography.md#ref-blum1973)).

--8<-- "generated/Selection-selection.pseudo.md"

--8<-- "generated/Selection-select.pseudo.md"

<span id="lem-6-10"></span>**Lemma 6.10.** *If the pivot is the median of
the medians of the groups of 5 items, at least $3 n / 10 - O( 1 )$ items
lie on each side of it, and the selection makes*

$$
T( n ) \le T( n / 5 ) + T( 7 n / 10 ) + O( n ) = O( n ) \tag{6.11}
$$

*comparisons in the worst case ([Blum et al.
1973](Z-bibliography.md#ref-blum1973)).*

*Proof.* There are $M = \lfloor n / 5 \rfloor \ge ( n - 4 ) / 5$ groups,
and the pivot is the median of rank $\lceil M / 2 \rceil$ of their medians.
At least $\lceil M / 2 \rceil$ medians are not larger than the pivot, each
with 3 items of its group not larger than it, and the same holds on the
other side; hence each of the parts smaller and larger than the pivot has at
most $n - 3 ( n - 4 ) / 10 \le 7 n / 10 + 2$ items. The groups cost at most
10 comparisons of insertion each, and the partition in three ways at most 2
per item, so that, with $T$ taken not decreasing, $T( n ) \le T( n / 5 ) +
T( 7 n / 10 + 2 ) + a n$ for $n > 10$ and a constant $a$. For $n \ge 80$
one has $7 n / 10 + 2 \le 3 n / 4$, and by induction
$T( n ) \le c n$ with $c = \max\{ 20 a , \max_{n < 80} T( n ) / n \}$,
since $c n / 5 + 3 c n / 4 + a n = 19 c n / 20 + a n \le c n$. $\square$

`medianOfMedians()` partitions in three ways (smaller, equal and larger
than the pivot), so that many equal items cost no further round, and
sorts the parts of at most 10 items by insertion. Both functions take the
rank from 1 and rearrange $A$.

The engineered `nthElement()` of `NthElement.h` puts in the place `nth`
of a range the item that would be there if the range were sorted, the
items before it not larger and those after it not smaller, as
`std::nth_element`, which it is meant to replace; an overload on a vector
takes the rank $k$ from 0 and returns the item
([Algorithm 6.16](06-search.md#alg-6-16)). On a range of $n > 600$ items,
the pivot comes from the sampling of [Floyd and Rivest
1975](Z-bibliography.md#ref-floyd1975): a sample of about
$s = n^{2/3} / 2$ items around the place $k$ is selected recursively for
the rank $k$, and the item that lands in the place $k$ then falls, with
high probability, just on the side of $k$ that leaves the smaller part.
The expected number of comparisons is

$$
n + \min( k , n - k ) + O( n^{2/3} ( \ln n )^{1/3} ) \tag{6.12}
$$

([Kiwiel 2005](Z-bibliography.md#ref-kiwiel2005)), which is optimal up to
the last term, against the
$2 n + 2 k \ln( n / k ) + 2 ( n - k ) \ln( n / ( n - k ) ) + o( n )$ of a
pivot at random (up to $2 ( 1 + \ln 2 ) n \approx 3.39 n$ for the median).
Below 600 items the pivot is the median of three, and a range of at most
24 items is sorted by insertion.

<span id="alg-6-16"></span>**Algorithm 6.16.** The engineered selection
(the selection of [Hoare 1961](Z-bibliography.md#ref-hoare1961) with the
sample of [Floyd and Rivest 1975](Z-bibliography.md#ref-floyd1975)); the
range is $A[ 0 .. n - 1 ]$ and $k$ the place `nth`.

--8<-- "generated/NthElement-nthElement.pseudo.md"

The partition suits the order of the range, which 64 items taken at a
stride reveal. On a random order (more than 16 turns, i.e., a rise followed
by a fall or the opposite, among them; about $2 \cdot 62 / 3 \approx 41$
on a random permutation) it is the scheme of Lomuto with no branch. An
item is moved to the end of the smaller ones and the item there to its
place, and the end advances by the outcome of the comparison, an addition
instead of a jump ([Edelkamp and Weiß
2016](Z-bibliography.md#ref-edelkamp2016) apply the same idea to
quicksort), while the partition of Hoare mispredicts half of its
comparisons on random items. The loop is unrolled by 4, so that it does not
depend on the alignment of one jump.

```cpp
--8<-- "generated/NthElement-selectLomuto.cpp"
```

On a range in some order (an organ pipe, a few runs) the partition is the
scheme of Hoare, whose branches are then predicted and which writes only
the items on the wrong side; the sample of Floyd and Rivest is then
gathered from the whole range at a stride, since a contiguous one would
not represent it. A range whose 64 items all rise (or all fall) is checked
to be sorted, by blocks of 64 pairs with no exit inside a block, and is
then left as it is (or reversed in the same pass of the check), in about
$n$ comparisons. When the pivot is the smallest item of the range, the
items equal to it go to its left, so that many equal items end in one step;
since the items of a range cut on its left by a pivot are not smaller than
it, a later pivot equal to it is known to be the smallest with no pass.
Finally, each round charges the items of its part to a budget of $4 n$
items, which a random order hardly ever exhausts (by (6.12), the rounds of
a random range of more than 600 items partition about $1.5 n$ items at
most),
and a round whose part the rest of the budget does not cover takes as its
pivot the median of the medians, whose rounds cost (6.11). Hence the other
rounds partition at most $4 n$ items in all, and their samples, selected
recursively within their own budgets, hold $O( m^{2/3} )$ items of a part
of $m > 600$, i.e., less than $m / 16$. By induction on $n$, the selection
therefore takes $O( n )$ time in the worst case, as the selection of
[Musser 1997](Z-bibliography.md#ref-musser1997) does by the same device;
against the adversary of [McIlroy 1999](Z-bibliography.md#ref-mcilroy1999),
which fixes the order of the items while they are compared, the
comparisons stay below $16 n$ from $n = 2^{12}$ to $2^{18}$ (the test
`ordered.nth-element-adversary`), where a budget counted in rounds let them
grow with $\log n$.
[Table 6.2](06-search.md#tab-6-2) compares the three selections.

<span id="tab-6-2"></span>**Table 6.2.** The selections of the item of
rank $k$ in `ADS/search`.

| Function | Pivot | Comparisons |
|---|---|---|
| `selection()` | at random, or the first item | $O( n )$ expected by (6.10), $\Theta( n^2 )$ in the worst case |
| `medianOfMedians()` | the median of the medians of the groups of 5 | $O( n )$ in the worst case, by (6.11) |
| `nthElement()` | the sample of Floyd and Rivest, the median of three on short parts | (6.12) expected, $O( n )$ in the worst case |

## 6.9 Range minima {#sec-6-9}

After a preprocessing of $A$, the query $\mathrm{RMQ}( i , j )$ is
answered without scanning the range; the problem underlies the longest
common extensions of a text and the lowest common ancestors of a tree
(cf. [§15.5](15-string.md#sec-15-5) and [§8.4](08-tree.md#sec-8-4)). The
*sparse table* (`SparseTable` of `RangeMinimum.h`) stores
$M[ L ][ i ] = \mathrm{RMQ}( i , i + 2^L - 1 )$ for each $i$ and $L$, each
from two entries of the row before, in $O( n \log n )$ time and space. A
query takes the better of the two entries of length $2^L$,
$L = \lfloor \log_2 ( j - i + 1 ) \rfloor$, that start at $i$ and end at
$j$; they cover the range, and the minimum does not mind their overlap
([Algorithm 6.17](06-search.md#alg-6-17) and
[Figure 6.7](06-search.md#fig-6-7)).

![Figure 6.7. The query of the range from 2 to 8 (violet) on a `SparseTable`: the two entries of length $2^2$ that start at 2 and end at 8 overlap, and the better of their minima, 2 at the place 7, is the answer.](../figures/search-sparse.svg){#fig-6-7}

<span id="alg-6-17"></span>**Algorithm 6.17.** The sparse table ([Bender
and Farach-Colton 2000](Z-bibliography.md#ref-bender2000)); the better of
two places is the one of the smaller item, the left one on a tie.

--8<-- "generated/RangeMinimum-SparseTable-build.pseudo.md"

--8<-- "generated/RangeMinimum-SparseTable-query.pseudo.md"

The linear preprocessing rests on a tree. The *Cartesian tree* of $A$
([Vuillemin 1980](Z-bibliography.md#ref-vuillemin1980)) has the leftmost
minimum of $A$ as its root, and the Cartesian trees of the parts on its
left and on its right as its subtrees. `cartesianTree()` builds it, as the
parent of each place ($n$ for the root), in $O( n )$ time with a stack that
holds its right spine, each item being pushed and popped once
([Algorithm 6.18](06-search.md#alg-6-18)); equal items go to the right.

<span id="alg-6-18"></span>**Algorithm 6.18.** The Cartesian tree of $A$
([Vuillemin 1980](Z-bibliography.md#ref-vuillemin1980)), the stack $S$
holding the right spine of the tree of $A[ 0 .. i - 1 ]$.

--8<-- "generated/RangeMinimum-cartesianTree.pseudo.md"

```cpp
--8<-- "generated/RangeMinimum-cartesianTree.cpp"
```

<span id="lem-6-11"></span>**Lemma 6.11.** *$\mathrm{RMQ}( i , j )$ is the
lowest common ancestor of $i$ and $j$ in the Cartesian tree of $A$, i.e.,
the node of least depth among those that the Euler tour of the tree visits
between the first visits of $i$ and $j$ ([Bender and Farach-Colton
2000](Z-bibliography.md#ref-bender2000)).*

*Proof.* The invisit of the tree is $A$, and each subtree is thus a range
of places. Going down from the root, while the place of the node is out of
$[ i , j ]$ both $i$ and $j$ lie in the same subtree; the lowest common
ancestor $a$ is therefore the first node of the path whose place falls in
$[ i , j ]$, and the range lies in the range of the subtree of $a$. The
root $a$ of that subtree is the leftmost minimum of its range, and hence
of $[ i , j ]$: an item equal to it on its left is an ancestor of $a$,
since equal items go to the right, hence out of its subtree. Between the two
first visits the tour does not leave the subtree of $a$, and it visits $a$
(at the start, if $a$ is $i$, or between the two subtrees of $a$
otherwise), the only node of least depth there. $\square$

![Figure 6.8. An array of 7 items and its Cartesian tree (each node labelled by its item, its place beside it), and the nodes $E$ and the depths $D$ of the Euler tour of the tree; the query of the range from 2 to 4 is the node of least depth, 3, between the first visits of 2 and 4 (the violet part of the tour).](../figures/search-cartesian.svg){#fig-6-8}

The depths along a tour differ by exactly 1 at each step, and the queries
on such an array (`PlusMinusOneRangeMinimum`) are answered in $O( 1 )$
time after $O( n )$ preprocessing. The array is cut into blocks of
$d = \max\{ 1 , \lfloor \lfloor \log_2 n \rfloor / 2 \rfloor \}$ items
(`blockSize()`), and a sparse table on the $n / d$ minima of the blocks
costs $O( ( n / d ) \log n ) = O( n )$. A block is determined, up to an
additive constant, by its $d - 1$ steps of $\pm 1$; hence the answers
inside all the possible blocks are built once in a table (the trick of the
Four Russians, [Arlazarov et al. 1970](Z-bibliography.md#ref-arlazarov1970))
of

$$
2^{d - 1} \cdot d^2 \le \sqrt{n} \, \log_2^2 n = o( n ) \tag{6.13}
$$

entries ([Algorithm 6.19](06-search.md#alg-6-19)). Thus `RangeMinimum`
answers in $O( 1 )$ time after $O( n )$ preprocessing on any array, by
the query on the $2 n - 1$ depths of the tour of its Cartesian tree
([Figure 6.8](06-search.md#fig-6-8)), whose nodes `eulerTour()` gives.

<span id="alg-6-19"></span>**Algorithm 6.19.** The range minima on steps
of $\pm 1$ and on any array ([Bender and Farach-Colton
2000](Z-bibliography.md#ref-bender2000)).

--8<-- "generated/RangeMinimum-PlusMinusOneRangeMinimum-build.pseudo.md"

--8<-- "generated/RangeMinimum-PlusMinusOneRangeMinimum-query.pseudo.md"

--8<-- "generated/RangeMinimum-build.pseudo.md"

--8<-- "generated/RangeMinimum-query.pseudo.md"

The bound $O( n )$ was first reached by [Harel and Tarjan
1984](Z-bibliography.md#ref-harel1984) on the lowest common ancestors,
with a far more involved structure; [Fischer and Heun
2011](Z-bibliography.md#ref-fischer2011) need only $2 n + o( n )$ bits
besides $A$, by describing each block by its own Cartesian tree, which
removes the Euler tour and its factor 2. In practice the constant factors
of the reduction are large, and the sparse table, the simplest of the
three, is often faster on the arrays that fit in a cache. All the
structures of the section keep a copy of the array, which `size()` and
`operator[]` read (and `SparseTable` and `RangeMinimum` also as a range of
constant iterators); `PlusMinusOneRangeMinimum` gives only its `size()`.

`WordRangeMinimum` keeps the decomposition into blocks but answers inside
a block of 64 items by the bits of a word. Inside a block that starts at
$b$, the stack of the minima from the right is kept as a mask for each
place $j$,

$$
\mathit{mask}[ j ] = \sum \big\{ 2^{t - b} : b \le t \le j , \;
A[ t ] \le A[ u ] \text{ for all } t < u \le j \big\} , \tag{6.14}
$$

i.e., the places on the stack of the scan of the block from $b$ up to
$j$, which pops the items larger than the new one and pushes it
([Figure 6.9](06-search.md#fig-6-9)).

![Figure 6.9. A block of 8 items (64 in `WordRangeMinimum`) and the mask of the place 6 by (6.14), whose set bits are the places 2 and 6 of the minima from the right of the items 0 to 6; the leftmost minimum of the items 3 to 6 is the lowest set bit at or above 3, i.e., the place 6.](../figures/search-word.svg){#fig-6-9}

<span id="lem-6-12"></span>**Lemma 6.12.** *Inside a block, the leftmost
minimum of $A[ i .. j ]$ is the place of the lowest set bit of
$\mathit{mask}[ j ]$ at or above $i$, i.e.,
$b + \mathrm{ctz}( \mathit{mask}[ j ] \wedge \neg ( 2^{i-b} - 1 ) )$.*

*Proof.* Let $t^*$ be the leftmost minimum of $A[ i .. j ]$. Since
$A[ t^* ] \le A[ u ]$ for each $u$ in $( t^* , j ]$, the bit of $t^*$ is
set by (6.14). A place $t$ with $i \le t < t^*$, instead, has
$A[ t ] > A[ t^* ]$, since $t^*$ is the leftmost minimum, and its bit is
not set, by (6.14) with $u = t^*$. The lowest set bit at or above $i$ is
thus the bit of $t^*$. $\square$

A query across blocks takes the minimum of the suffix of the block of
$i$, that of the prefix of the block of $j$ and, if there are blocks in
between, that of the two entries of a sparse table on the minima of the
blocks that cover them, the leftmost of the smallest of three
([Algorithm 6.20](06-search.md#alg-6-20)). For each place, the minima of the suffixes and
of the prefixes are kept, with their offsets in the block
side by side, and so are the minima in the table; a query thus makes at
most five reads of the memory, independent of one another, and never reads
the array. Both answers (in one block and across blocks) are computed, and
one is chosen by a conditional move, which a random range would otherwise
mispredict half of the times:

```cpp
--8<-- "generated/WordRangeMinimum-query.cpp"
```

<span id="alg-6-20"></span>**Algorithm 6.20.** The range minima by the
stacks in a word (after [Baumstark et al.
2017](Z-bibliography.md#ref-baumstark2017)); $S$ is the table of the
minima of the blocks, as in `SparseTable`.

--8<-- "generated/WordRangeMinimum-build.pseudo.md"

--8<-- "generated/WordRangeMinimum-query.pseudo.md"

All the masks are built by the stacks in $O( n )$ time or, on the integers
and the floating types under `std::less`, by one comparison of vectors of
the block per item (`greaterBits()`). The larger items are then cleared
from the mask of the item before all at once,
$\mathit{mask}[ j ] = ( \mathit{mask}[ j-1 ] \wedge \neg G_j ) \vee
2^{j-b}$ with $G_j$ the bits of the items of the block larger than
$A[ j ]$, with no branch; the stack is increasing, and the items it pops
are exactly those of it larger than $A[ j ]$. On fewer than $2^{32}$ items,
the space is

$$
n ( 8 + p ) + \Big\lceil \frac{n}{64} \Big\rceil \Big( \Big\lfloor \log_2
\Big\lceil \frac{n}{64} \Big\rceil \Big\rfloor + 1 \Big) q \tag{6.15}
$$

bytes besides the array (the masks, the minima of the suffixes and of the
prefixes with their offsets, and the table), on huge pages, where $p$ and
$q$ are $2 \, \mathrm{sizeof}( T ) + 2$ and $\mathrm{sizeof}( T ) + 4$
rounded up to the alignment of their records (12 and 8 for items of 32
bits). For a million items of 32 bits this is about 22 bytes per item,
against the $8 ( \lfloor \log_2 n \rfloor + 1 ) = 160$ bytes per item of
the places of 64 bits of a `SparseTable` on all the items.
[Table 6.3](06-search.md#tab-6-3) summarizes the four structures.

<span id="tab-6-3"></span>**Table 6.3.** The structures of the range
minima in `ADS/search`.

| Structure | Preprocessing | Query | Source |
|---|---|---|---|
| `SparseTable` | $O( n \log n )$ time and space | $O( 1 )$ | Bender and Farach-Colton 2000 |
| `PlusMinusOneRangeMinimum` | $O( n )$ | $O( 1 )$, on steps of $\pm 1$ | Bender and Farach-Colton 2000 |
| `RangeMinimum` | $O( n )$ | $O( 1 )$ | Bender and Farach-Colton 2000 |
| `WordRangeMinimum` | $O( n )$, about 22 bytes per item of 32 bits | $O( 1 )$, five reads at most | Baumstark et al. 2017 |

## 6.10 The searches compared {#sec-6-10}

The searches of the textbooks (`binarySearch()`, `lowerBound()`,
`interpolationSearch()`, the searches of Knuth, `selection()`,
`SparseTable` and `RangeMinimum`) are meant to show the methods and to
check their bounds. The engineered ones are meant to be compared with the
best implementations available, i.e., `std::lower_bound` for
`branchlessLowerBound()` and `SPlusTree`, the implementation of the paper
for the three PGM-indexes, `std::nth_element` for `nthElement()`, and the
structures of [Baumstark et al. 2017](Z-bibliography.md#ref-baumstark2017)
for `WordRangeMinimum`. Which search of a sorted array is fastest depends
(possibly more than on anything else) on whether the array fits in the
cache: on a small array the branch-free binary search usually wins, since
it has no structure to read, while on a large one the S+ tree and the
PGM-index read fewer lines, the latter in far less space on regular keys.
Their comparison on an idle machine, with the sizes of the arrays and the
distributions of the keys at which the ranking changes, belongs to
[Appendix B](B-benchmarks.md#app-B).

## 6.11 Discussion and references {#sec-6-11}

The binary search, the search by interpolation and the selection of
Hoare are in [Montresor 2025, §1.2, §12.1 and
§17.1](Z-bibliography.md#ref-montresor2025), the searches of Fibonacci and
the uniform binary search in [Knuth 1998,
§6.2.1](Z-bibliography.md#ref-knuth1998v3), and the unbounded searches in
[Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976); the expected
cost of the interpolation is that of [Perl et al.
1978](Z-bibliography.md#ref-perl1978). [Khuong and Morin
2017](Z-bibliography.md#ref-khuong2017) study the layouts of the arrays for
the searches by comparison, and the S+ tree is that of
[Slotin 2022](Z-bibliography.md#ref-slotin2022). The PGM-index, compressed
and dynamic, is that of [Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020), on the optimal piecewise
linear approximation of [O'Rourke 1981](Z-bibliography.md#ref-orourke1981)
and [Xie et al. 2014](Z-bibliography.md#ref-xie2014) and on the
logarithmic method of [Bentley and Saxe
1980](Z-bibliography.md#ref-bentley1980). The bound of the minimum and the
maximum together is that of [Pohl 1972](Z-bibliography.md#ref-pohl1972),
the selection in linear time that of [Blum et al.
1973](Z-bibliography.md#ref-blum1973); the sampling of [Floyd and Rivest
1975](Z-bibliography.md#ref-floyd1975) is analyzed by [Kiwiel
2005](Z-bibliography.md#ref-kiwiel2005), and the lower bound on the local
minima of the hypercube is in [Aldous 1983](Z-bibliography.md#ref-aldous1983).
The range minima follow [Bender and Farach-Colton
2000](Z-bibliography.md#ref-bender2000), with the space of [Fischer and
Heun 2011](Z-bibliography.md#ref-fischer2011) and the engineering of
[Baumstark et al. 2017](Z-bibliography.md#ref-baumstark2017); [Ferragina
2023, chapter 10](Z-bibliography.md#ref-ferragina2023) uses them for the
suffix arrays and the suffix trees.

## 6.12 Exercises {#sec-6-12}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Find the minimum of a vector with $n - 1$ comparisons ([Montresor
   2025, §1.2](Z-bibliography.md#ref-montresor2025)). Test:
   `ch01.sec1.2.min`.
2. Realize the binary search with no overflow of the middle, and build its
   decision tree, whose height is $\lfloor \log_2 n \rfloor$. Tests:
   `ch01.sec1.2.binary-search`, `beyond.decision-tree`,
   `contract_search.binary-search`.
3. Count the occurrences of a key in a sorted vector with two binary
   searches (exercise 12.13), and find the first item not smaller than a
   key in $O( \log h )$ time if it is at the place $h$ (exercise 12.9).
   Tests: `ch12.ex12.13`, `ch12.ex12.9`.
4. Realize the search by interpolation, and show a vector on which it
   makes a linear number of probes. Tests: `ch12.sec12.1.interpolation`,
   `contract_search.interpolation-search`.
5. Realize the Fibonacci search and the uniform binary search of Knuth.
   Test: `taocp.knuth-searches`.
6. Search a monotone predicate on the integers with no bound on its
   answer, and compute the integer square root with no overflow and tell
   the perfect squares. Tests: `lib_search.exponential-search`,
   `lib_search.integer-sqrt`.
7. Find the minimum of a rotated vector, the item closest to a value, two
   items with a given sum and the item of rank $k$ of two sorted vectors.
   Test: `lib_search.sorted-searches`.
8. Find a peak and a local minimum of a vector, and a local minimum of a
   binary tree, in a logarithmic number of comparisons. Tests:
   `lib_search.local-optima`, `contract_search.local-and-sorted`.
9. Find the minimum and the maximum together with
   $\lceil 3 n / 2 \rceil - 2$ comparisons. Test:
   `contract_search.minimum`.
10. Realize the selection of Hoare with a random pivot and the median of
    the medians. Tests: `ch17.sec17.1.selection`,
    `contract_search.selection`.
11. Realize the selection with the sample of Floyd and Rivest and the
    partition of Lomuto with no branch, with the median of the medians
    after too many rounds that cut little. Test: `ordered.nth-element`.
12. Realize the binary search with no branch and the static B+-tree of
    Slotin. Test: `ordered.static-search`.
13. Realize the PGM-index, its compressed form with shared slopes, and its
    dynamic form by the logarithmic method. Tests: `ordered.pgm-index`,
    `ordered.pgm-compressed`, `ordered.pgm-dynamic`.
14. Answer the range minimum queries by the sparse table and by the
    reduction of Bender and Farach-Colton. Test:
    `pearls_suffix.sec10.4.1.range-minimum`.
15. Answer the range minimum queries by the stacks in a word, with at most
    five reads of the memory per query. Test:
    `ordered.word-range-minimum`.

# 6. Searching {#ch-6}

[Source: `include/ADS/search/BinarySearch.h`, `MonotoneSearch.h`,
`InterpolationSearch.h`, `KnuthSearches.h`, `SortedSearches.h`,
`StaticSearch.h`, `CountLess.h`, `PGMIndex.h`, `CompressedPGMIndex.h`,
`DynamicPGMIndex.h`, `Minimum.h`, `LocalOptima.h`, `Selection.h`,
`NthElement.h`, `RangeMinimum.h`, `WordRangeMinimum.h`]

We are concerned in this chapter with the questions that can be answered
on a vector by looking at few of its items: the place of a key in a
sorted vector, the minimum and the local optima, the item of a given rank,
and the minimum of any range of an array after a preprocessing. The
searches in a sorted vector come first, both in the forms of the textbooks
([§6.2](06-search.md#sec-6-2)) and in the forms that count the misses of
the cache and the branches that the processor mispredicts
([§6.3](06-search.md#sec-6-3)); then come the learned indexes, which
replace the comparisons of the upper levels of a search by a model of the
keys, in the static, compressed and dynamic forms of the PGM-index
([§6.4](06-search.md#sec-6-4) to [§6.6](06-search.md#sec-6-6)). Finally, the
minimum and the local optima ([§6.7](06-search.md#sec-6-7)), the
selection ([§6.8](06-search.md#sec-6-8)) and the range minima
([§6.9](06-search.md#sec-6-9)) close the chapter, before a comparison of
the searches on what each one is meant for ([§6.10](06-search.md#sec-6-10)).

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
inner nodes are the comparisons and whose leaves are the answers; a run of
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
| `gallopSearch()`, `exponentialSearch()` | $2 \lceil \log_2 ( h + 1 ) \rceil + O( 1 )$ | Bentley and Yao 1976 |
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
$A[ m + 1 .. j ]$ if $v > A[ m ]$ ([Algorithm 6.1](06-search.md#alg-6-1));
it stops as soon as it meets $v$, at the price of two comparisons per
step. In the code, the middle is computed as $m = i + \lfloor ( j - i ) / 2 \rfloor$,
which, unlike $\lfloor ( i + j ) / 2 \rfloor$, cannot overflow (the bug
of `Arrays.binarySearch` of Java until 2003). `decisionTree()` builds the
decision tree of the search, whose root is the middle item of the vector
and whose subtrees are the decision trees of the two halves
([Figure 6.1](06-search.md#fig-6-1)): its invisit gives $A$, it is a
binary search tree, and all its levels are full but the last one, so that
its height, the worst case of the search, is $\lfloor \log_2 n \rfloor$.

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
keeps (6.2) since $A$ is sorted. The length $\ell = \mathit{hi} -
\mathit{lo}$ goes to at most $\lfloor \ell / 2 \rfloor$, and the loop thus
ends after at most $\lceil \log_2 ( n + 1 ) \rceil$ comparisons with
$\mathit{lo} = \mathit{hi}$, which is the answer by (6.2). $\square$

The contracts check (6.2) at the result, and that the range is sorted, an
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
and a binary search on the last interval, of length $2^{k-1}$, costs
$k - 1$ more comparisons ([Algorithm 6.3](06-search.md#alg-6-3) and
[Figure 6.2](06-search.md#fig-6-2)); in all,

$$
2 \lceil \log_2 ( h + 1 ) \rceil + O( 1 ) \tag{6.3}
$$

comparisons, which depend on the answer but not on $n$. [Bentley and Yao
1976](Z-bibliography.md#ref-bentley1976) also show that applying the same
idea to the search of $k$ gives $\log_2 h + \log_2 \log_2 h + \cdots$
comparisons, close to the lower bound $\log_2 h$.

![Figure 6.2. `gallopSearch()` for $v = 40$: the probes at the places 0, 1, 3 and 7 (shaded, numbered above in their order) double the bound until $A[ 7 ] \ge v$, and a binary search on the places 4 to 7 (grey) finds the answer $h = 5$.](../figures/search-gallop.svg){#fig-6-2}

<span id="alg-6-3"></span>**Algorithm 6.3.** The searches with a bound that
doubles ([Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976)): on a
sorted vector, on a predicate $p$ false and then true on the integers, and
on a sequence of unknown length.

--8<-- "generated/BinarySearch-gallopSearch.pseudo.md"

--8<-- "generated/MonotoneSearch-exponentialSearch.pseudo.md"

--8<-- "generated/MonotoneSearch-unboundedSearch.pseudo.md"

`exponentialSearch()` finds the least $i \ge \mathit{lo}$ at which a
predicate $p$, false and then true, holds, in at most
$2 \lceil \log_2 ( i - \mathit{lo} + 1 ) \rceil + 1$ evaluations of $p$
(the method $B_1$ of Bentley and Yao). Its probes never overflow the type
of the integers, and the middle of its binary search is computed on the
unsigned distance. `unboundedSearch()` applies it to a sequence read one
item at a time, whose function `get(i)` gives no item past the end: the
count of the items smaller than a value, in a sorted sequence of unknown
length, is the typical case. The same scheme searches any monotone
predicate on the integers, e.g., the integer square root
(`integerSqrt()`, [Algorithm 6.4](06-search.md#alg-6-4)), which keeps
$\mathit{lo}^2 \le n < \mathit{hi}^2$ on $[ 0 , 2^{w/2} )$ for $w$ bits
and tests $m^2 \le n$ as $m \le \lfloor n / m \rfloor$, so that no
product overflows; it makes $w / 2$ steps. (Newton's iteration
$x \gets \lfloor ( x + \lfloor n / x \rfloor ) / 2 \rfloor$ converges
quadratically from a start above the root, in $O( \log \log n )$ steps
from a start within a constant factor.)

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
1978](Z-bibliography.md#ref-perl1978)): the expected error of (6.4) on a
part of length $\ell$ is of the order of $\sqrt{\ell}$, and each probe
thus takes, in expectation, the square root of the length. On keys far
from uniform (e.g., growing exponentially), however, a probe may exclude
a single item, and the worst case is $O( n )$. Our search stops when $v$
is out of $[ A[ \mathit{lo} ] , A[ \mathit{hi} ] ]$, and probes
$A[ \mathit{lo} ]$ when $A[ \mathit{lo} ] = A[ \mathit{hi} ]$; the
fraction is computed in `double`, and when it is undefined ($0 / 0$ for
distinct keys equal as doubles, i.e., integers past $2^{53}$, or
$\infty / \infty$ for infinite keys) the probe is the middle place. Note that the
learned indexes of [§6.4](06-search.md#sec-6-4) refine the same idea with
a model that has a guaranteed error.

<span id="alg-6-5"></span>**Algorithm 6.5.** The search by interpolation
([Montresor 2025, §12.1](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/InterpolationSearch-interpolationSearch.pseudo.md"

Two searches of [Knuth 1998, §6.2.1](Z-bibliography.md#ref-knuth1998)
replace the division of the middle by additions
([Algorithm 6.6](06-search.md#alg-6-6)). First, the Fibonacci search
(`fibonacciSearch()`, Algorithm 6.2.1F, after [Ferguson
1960](Z-bibliography.md#ref-ferguson1960)) splits a part of length
$F_{k+1} - 1$ at its $F_k$-th item into parts of lengths $F_k - 1$ and
$F_{k-1} - 1$, in at most about $\log_\phi n \approx 1.44 \log_2 n$
comparisons, $\phi$ the golden ratio; when $n$ is not a Fibonacci number,
a probe past the end is moved to the last item, as if items larger than
all the others filled the vector up to the next Fibonacci number. Second, the
uniform binary search (`uniformBinarySearch()`, Algorithms 6.2.1U and
6.2.1C) keeps a place $i$ and the width
$\delta_j = \lfloor ( n + 2^{j-1} ) / 2^j \rfloor$ of the step $j$, taken
from a table computed once: a step adds or subtracts the next width, and
the search moves on a fixed tree of differences with no endpoint to
update. Its places 0 and $n + 1$ stand for the keys $-\infty$ and
$+\infty$ of the book, and are compared without reading the vector.

<span id="alg-6-6"></span>**Algorithm 6.6.** The Fibonacci search and the
uniform binary search ([Knuth 1998, Algorithms 6.2.1F, 6.2.1U and
6.2.1C](Z-bibliography.md#ref-knuth1998)).

--8<-- "generated/KnuthSearches-fibonacciSearch.pseudo.md"

--8<-- "generated/KnuthSearches-uniformBinarySearch.pseudo.md"

Finally, the searches of `SortedSearches.h` apply the invariant (6.2) to
other predicates that are false and then true along a sorted vector
([Algorithm 6.7](06-search.md#alg-6-7)). The minimum of a strictly
increasing vector rotated by an unknown number of places
(`rotatedMinimum()`) is at the first place $m$ with
$A[ m ] \le A[ n - 1 ]$, in $\lceil \log_2 n \rceil$ comparisons; with
repeated items no search beats $\Omega( n )$ comparisons in the worst
case, as a vector of equal items but one shows, and the items must
therefore be distinct. Similarly, the item closest to a value $x$ (`closestValue()`)
is $A[ r ]$ or $A[ r - 1 ]$ for $r = \mathrm{lowerBound}( x )$; the
distances are computed on the unsigned type of the same width for the
integers, which cannot overflow. Two places with a given sum
(`pairWithSum()`) are found by two indices from the ends in $O( n )$
time, each step excluding all the pairs of the item left behind. Also, the item
of rank $k$ of the union of two sorted vectors $X$ and $Y$
(`kthOfTwoSorted()`, [Frederickson and Johnson
1982](Z-bibliography.md#ref-frederickson1982)) is found by a binary search
of the number $i$ of the $k + 1$ smallest items taken from the shorter
vector, the right $i$ being the first one with $X[ i ] \ge Y[ k - i ]$
(in the code, $X[ i - 1 ] \le Y[ j ]$ and $Y[ j - 1 ] \le X[ i ]$ with
$j = k + 1 - i$), in $O( \log \min( m , n ) )$ comparisons; the two
medians of two sorted vectors of $n$ items are the items of ranks $n - 1$
and $n$.

<span id="alg-6-7"></span>**Algorithm 6.7.** The searches of
`SortedSearches.h` (Esercizi Divide-et-impera and Esercizi vari of
[Montresor 2025](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/SortedSearches-rotatedMinimum.pseudo.md"

--8<-- "generated/SortedSearches-closestValue.pseudo.md"

--8<-- "generated/SortedSearches-pairWithSum.pseudo.md"

--8<-- "generated/SortedSearches-kthOfTwoSorted.pseudo.md"

## 6.3 Searches laid out for the memory {#sec-6-3}

On a modern processor the comparisons of a binary search cost less than
the two events that surround them. Its first levels miss the cache, since
the places probed are far apart; also, the branch that follows each
comparison is mispredicted half of the times on random keys. The headers
`StaticSearch.h` and `PGMIndex.h` remove the two costs, one at a time.

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

*Proof.* The invariant is that the answer is in $[ b , b + \ell ]$ and
that the items before $b$ are smaller than $v$; it holds at the start,
with $b = 0$ and $\ell = n$. If $A[ b + h ] < v$, the answer is past
$b + h$, and the new range $[ b + h , b + \ell ]$ holds it; otherwise the
answer is at most $b + h$, which is within the new range
$[ b , b + \ell - h ]$ since $h \le \ell - h$. The length goes from
$\ell$ to $\lceil \ell / 2 \rceil$, and reaches 1 after
$\lceil \log_2 n \rceil$ steps; then the answer is $b$ or $b + 1$, and
the last comparison decides between them. $\square$

The S+ tree of [Slotin 2022](Z-bibliography.md#ref-slotin2022)
(`SPlusTree`) removes the misses instead: its sorted items, cut into
nodes of $B$ items, are the leaves of a static B+-tree whose inner nodes
have $B$ keys and $B + 1$ children, the key $j$ of a node being the first
item under its child $j + 1$. In memory, the nodes of a level are stored one after
the other, the children of the node $k$ being the nodes
$k ( B + 1 ) , \ldots , k ( B + 1 ) + B$ of the next level, so that no
pointer is stored; the levels are stored one after the other, the leaves
last ([Figure 6.3](06-search.md#fig-6-3)). The level of the leaves has
$s_{H-1} = \lceil n / B \rceil$ nodes and each level above
$s_\ell = \lceil s_{\ell+1} / ( B + 1 ) \rceil$, up to the root; there
are thus

$$
H = 1 + \Big\lceil \log_{B+1} \Big\lceil \frac{n}{B} \Big\rceil
\Big\rceil \tag{6.5}
$$

levels, and a search reads one line per level, against the
$\log_2 n - \log_2 B$ misses of the binary search (3 lines for a million
keys of 32 bits, against 20 probes, of which the last 4 share a line).

![Figure 6.3. An `SPlusTree` with $B = 4$ on 16 sorted items: the root, whose key $j$ is the first item of the leaf $j + 1$ (the last key, with no leaf, padded with $\infty$), over the four leaves, which are the items themselves; the search of $v = 26$ counts 2 keys smaller than $v$ in the root and 2 in the leaf 2 (shaded), i.e., the place $2 B + 2 = 10$.](../figures/search-splus.svg){#fig-6-3}

In a node, the child to take is the count of the keys smaller than $v$.
On the arithmetic keys under `std::less`, the count runs on the whole line
with no branch, the places past the last item holding the largest value of
the type (or $+\infty$); `countLess()` of `CountLess.h` makes it one or
two comparisons of vectors of AVX-512 or AVX2, each giving a mask of bits
that a count of the bits sums (the unsigned integers are compared as
signed ones after flipping their top bit). Elsewhere, and on the other
types, it is the loop of the comparisons, which the compiler may vectorize
or not; `greaterBits()` gives the items larger than a key as the bits of a
word, which [§6.9](06-search.md#sec-6-9) uses
([Algorithm 6.9](06-search.md#alg-6-9)).

<span id="alg-6-9"></span>**Algorithm 6.9.** The layout and the search of
`SPlusTree` ([Slotin 2022](Z-bibliography.md#ref-slotin2022)), and the
count with no branch in a node; the levels are numbered from the root, 0,
to the leaves, $H - 1$.

--8<-- "generated/StaticSearch-SPlusTree-layout.pseudo.md"

--8<-- "generated/StaticSearch-SPlusTree-lowerBound.pseudo.md"

--8<-- "generated/CountLess-countLess.pseudo.md"

--8<-- "generated/CountLess-greaterBits.pseudo.md"

The search is a loop of counts, one per level; `upperBound()` counts the
keys not larger than $v$ instead:

```cpp
--8<-- "generated/StaticSearch-search.cpp"
```

<span id="lem-6-4"></span>**Lemma 6.4.** *An `SPlusTree` on $n$ items has
the height $H$ of (6.5), takes about $n ( B + 1 ) / B$ places of items,
and is built in $O( n )$ time; a search reads $H$ lines of cache.*

*Proof.* The levels have $s_\ell$ nodes of $B$ items each, and
$B \sum_\ell s_\ell \approx n ( 1 + 1 / ( B + 1 ) + 1 / ( B + 1 )^2 +
\cdots ) = n ( B + 1 ) / B$. Each key is copied once from the array. The
nodes are aligned to a line, and the search reads one node per level.
$\square$

The space is on huge pages when it reaches 2 MB (cf.
[§5.9](05-memory.md#sec-5-9)), so that a search misses the TLB at most
once. The S+ tree differs from the B-tree layout of [Khuong and Morin
2017](Z-bibliography.md#ref-khuong2017) in two points: the leaves are the
array itself, the inner keys only routing the search, and a node is
searched by a count instead of a search. The layout of Eytzinger (a heap
order of the items) also reads one line for the last 4 levels of the
binary search, and with the prefetch of the 16 descendants 4 levels down
it hides the latency of the misses instead of avoiding them, at the price
of the bandwidth; [Khuong and Morin 2017](Z-bibliography.md#ref-khuong2017)
found it the fastest of the layouts of the binary search, and the B-tree
layout, with a node per line, faster on the arrays that do not fit in the
cache. Historically, the B-trees with no pointers go back to the CSS trees of [Rao and
Ross 1999](Z-bibliography.md#ref-rao1999), and the FAST trees of [Kim et
al. 2010](Z-bibliography.md#ref-kim2010) lay out a binary tree in blocks
of lines and pages and search a block with SIMD comparisons; the
comparative study of [Bellomo et al. 2025](Z-bibliography.md#ref-bellomo2025)
finds the B-trees with SIMD searches the fastest in a lookup among the
classic, the compressed and the learned indexes of integers.

## 6.4 The PGM-index {#sec-6-4}

The PGM-index of [Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020) (`PGMIndex<K, E, R>`) replaces
the inner nodes of a B+-tree by a model of the keys. Its items
$A[ 0 .. n )$ are the points $( A[ i ] , i )$ of the plane, a repeated
key standing for its first place. An *$\epsilon$-approximation* is a
sequence of segments, each a line $f( x ) = a + b ( x - k_s )$ over a
range of keys from its first key $k_s$, such that
$| f( A[ i ] ) - i | \le \epsilon$ for each key of its range
([Figure 6.4](06-search.md#fig-6-4)). In turn, the first keys of the segments are
approximated, with error $\epsilon_r$, by the segments of a level
above, and so on up to a single segment. By default, $\epsilon = 64$
and $\epsilon_r = 4$.

![Figure 6.4. The points $( A[ i ] , i )$ of 14 keys covered by two segments whose lines predict each place within $\epsilon = 2$ (the shaded bands); a search predicts the place of $v$ on the line of its segment and searches the window of the places within the error.](../figures/search-pgm.svg){#fig-6-4}

The fewest segments, i.e., the *optimal* approximation, are found in one
pass by the algorithm of [O'Rourke 1981](Z-bibliography.md#ref-orourke1981)
as engineered by [Xie et al. 2014](Z-bibliography.md#ref-xie2014)
([Algorithm 6.10](06-search.md#alg-6-10)). Indeed, the lines that pass within
$\epsilon$ of the points of a range are those between the line of least
slope and the line of largest slope that the convex hulls of the points
moved up and down by $\epsilon$ admit. A new point updates the two hulls
in amortized $O( 1 )$ time, and a range ends when a point leaves no line.
A segment keeps the line of the average of the two extreme slopes through
the point where they cross, since any line of slope between them through
that point stays within the error; its slope is made not negative, so that
the prediction never decreases. Before all this, the keys are mapped to integers of 64 bits
in the same order (the signed ones by flipping the top bit, the floating
ones by their bits), and the hulls are kept with exact products of 128
bits.

<span id="alg-6-10"></span>**Algorithm 6.10.** The construction and the
search of `PGMIndex` (the optimal approximation of [O'Rourke
1981](Z-bibliography.md#ref-orourke1981) and [Xie et al.
2014](Z-bibliography.md#ref-xie2014), and the search of [Ferragina and
Vinciguerra 2020](Z-bibliography.md#ref-ferragina2020)).

--8<-- "generated/PGMIndex-approximate.pseudo.md"

--8<-- "generated/PGMIndex-lowerBound.pseudo.md"

<span id="lem-6-5"></span>**Lemma 6.5.** *A segment of the optimal
$\epsilon$-approximation covers at least $2 \epsilon$ keys, but possibly
for the last one; hence the last level has $m \le n / ( 2 \epsilon )$
segments, there are at most $1 + \lceil \log_{2 \epsilon_r} m \rceil$
levels, and a search costs*

$$
O( \log \epsilon_r \cdot \log_{2 \epsilon_r} m + \log \epsilon ) \tag{6.6}
$$

*time, in a space of 28 bytes per segment besides the items.*

*Proof.* The horizontal line through the middle of any $2 \epsilon$
consecutive places is within $\epsilon$ of each of them; hence, a range of
the optimal approximation that ends before covering $2 \epsilon$ keys
could be extended, and the greedy that closes a segment only when no line
is left covers at least as many keys as any other approximation. The same
argument with $\epsilon_r$ bounds the levels above. At each level, the
prediction of the line of the segment, clamped to its range, is within
$\epsilon_r + 1$ places of the next segment (one more for the rounding of
the line in `double`), which a binary search of $O( \epsilon_r )$ places
finds; at the last level the window has $O( \epsilon )$ places. A segment
keeps its first key, the two doubles of its line and its first place.
$\square$

At the last level the window, of $2 \epsilon + 5$ places, has its lines
of cache prefetched at once, so that their misses overlap, and is searched
by a binary search with no branch; on repeated keys, whose places a line
cannot all approximate, the search goes on past the window by galloping.
Since the items are a copy on huge pages, the window costs no miss of
the TLB of its own:

```cpp
--8<-- "generated/PGMIndex-lowerBound.cpp"
```

Note that the space is proportional to $m$, which on regular keys is far smaller
than $n / ( 2 \epsilon )$: on keys in arithmetic progression, one segment
covers them all. This is the point of a learned index, whose space depends
on the regularity of the keys more than on their number.

## 6.5 The compressed PGM-index {#sec-6-5}

The compressed PGM-index of [Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020) (`CompressedPGMIndex<K, E>`)
keeps the segments in a few tens of bits each instead of 28 bytes, with
the same search at the last level. Indeed, the lines that are within $\epsilon$ of
the $k$ points of a segment, i.e., the pairs $( \alpha , \beta )$ with
$| \beta + \alpha x_i - y_i | \le \epsilon$ for each of its points, are
the intersection of $2 k$ half-planes, a convex region; its shadow on the
axis of the slopes is the interval $[ \alpha^- , \alpha^+ ]$ whose ends
are the two extreme lines of the algorithm of O'Rourke. Each slope of the
interval has an intercept within the error, the middle of

$$
\Big[ \max_i ( y_i - \alpha x_i ) - \epsilon ,
\min_i ( y_i - \alpha x_i ) + \epsilon \Big] ; \tag{6.7}
$$

hence a set of slopes serves all the segments if it hits each of their
intervals ([Figure 6.5](06-search.md#fig-6-5)).

![Figure 6.5. The intervals of the slopes of five segments, sorted by their left ends, and the two groups of the greedy stabbing (segments 1 to 3, and 4 and 5): the shaded strips are the intersections of the groups, and the slope of each group, $S[ 0 ]$ and $S[ 1 ]$, is the middle of its intersection.](../figures/search-compressed.svg){#fig-6-5}

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
groups are needed. $\square$

Each segment then keeps the index of its slope and its intercept by (6.7),
as a distance from its first place of at most $\epsilon + 1$. The first
places of the segments, increasing, are kept by Elias-Fano (cf. chapter
22), and their first keys whole, searched by a PGM-index of their own
(of error 4) in place of the levels above
([Algorithm 6.11](06-search.md#alg-6-11)). With $m$ segments and $\sigma$
slopes, this takes

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

Elias-Fano on the first keys would save some ten bits a segment. Yet, its
rank (the search of a segment) scans the bucket of the key, and on skewed
keys most of the first keys crowd into a few buckets; the PGM-index of the
keys finds the segment in the time of the levels of the plain PGM-index,
and a search adds to that two selects and a field of bits. The rounding of
a shared slope to a `float` may take a line out of the error; the
construction checks each segment, and gives its own slope (appended to the
table) to one whose line misses a point by more than $\epsilon + 1$.

## 6.6 The dynamic PGM-index {#sec-6-6}

The dynamic PGM-index (`DynamicPGMIndex<K, V, E, B, I>`) is an ordered map
from arithmetic keys to values, with insertions and removals, made of
static PGM-indexes by the logarithmic method of [Bentley and Saxe
1980](Z-bibliography.md#ref-bentley1980). The pairs lie in a sorted buffer
$L_0$ of $b_0 = 128$ pairs and in sorted levels $L_1 , L_2 , \ldots$ of
capacities $c_t = b_0 B^t$ ($B = 8$ by default), each key at most once per
level. A key may be in several levels, and its pair in the newest one (the
buffer first, then by increasing $t$) is the one that counts; a removal
inserts a *tombstone*, a pair that marks the key as absent. When the
buffer is full, it is merged, with the new pair and with the levels before
$L_t$, into the first level $L_t$ that has room for them all
([Algorithm 6.12](06-search.md#alg-6-12) and
[Figure 6.6](06-search.md#fig-6-6)).

![Figure 6.6. A `DynamicPGMIndex` with $b_0 = 4$ and $B = 2$ (for the figure; the defaults are 128 and 8): the buffer $L_0$ and the levels $L_1$ and $L_2$, of capacities 8 and 16, before and after an insertion that finds the buffer full and merges it, with the new pair, into $L_1$, which has room for them (shaded); `x` is a tombstone, kept since $L_1$ is not the last level, and the grey places are free.](../figures/search-dynamic.svg){#fig-6-6}

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
for $t = 1$, since the buffer is full). The merge and the construction of
the PGM-index of the result are linear, $O( c_t )$, i.e., $O( B )$ per
pair brought into $L_t$. A pair is brought into a level only from the
levels before it, hence into each level at most once, and the levels are
$O( \log_B( n / b_0 ) )$ since $c_t$ grows by the factor $B$. $\square$

A lookup stops at the newest level that has the key; `lowerBound()` and
the visit of a range (`forEach()`) merge the levels from their places of
the key, and skip the keys whose newest pair is a tombstone. The
tombstones are dropped when the merge lands on the last level, where no
older pair is left for them to hide. Several choices of the layout fix the
constants. As in the implementation of the paper, only the levels of
capacity at least $I$ ($2^{24}$ by default) have a PGM-index, since on the
smaller ones a model does not pay for its construction at each merge; the
others are searched by the binary search with no branch of
[§6.3](06-search.md#sec-6-3). The keys and the values of a level are two
arrays, so that a search reads only the keys, and the tombstones are a
sorted list of their keys apart (they are usually few). A level of
capacity at least $2^{12}$ also has a blocked filter of Bloom of its keys
(cf. chapter 10), 10 bits per key, as the levels of the stores by
log-structured merge have ([O'Neil et al.
1996](Z-bibliography.md#ref-oneil1996); [Dayan et al.
2017](Z-bibliography.md#ref-dayan2017)). A lookup of a key that is not
there thus reads one line of cache per level instead of searching each,
and one that is there reads a line per newer level and searches its own.
The filter of a level takes the keys that a merge brings in, and is built
again only when the keys have doubled; the filters thus add a constant
number of insertions into a filter per key and level to the cost of
[Lemma 6.7](06-search.md#lem-6-7). The number of distinct keys is not
kept, since it would cost a lookup per insertion; `entries()` counts the
pairs stored, tombstones and older pairs included.

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
lost. For the minimum and the maximum, an item must lose once to be
excluded from being the minimum and win once to be excluded from being the
maximum, i.e., $2 n - 2$ outcomes are needed in all. A comparison of two
items never compared gives one loss and one win at once, and the other
comparisons give (against an adversary) at most one of the two; there are
at most $\lfloor n / 2 \rfloor$ comparisons of the first kind, whence
(6.9). `minMax()` pairs the items, compares the two of a pair, and then
the smaller with the minimum and the larger with the maximum, 3
comparisons per pair. $\square$

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
whose solution is $3 n / 2 - 2$.

A local optimum is cheaper than the minimum. A *peak* of $A$ is an item
not smaller than its neighbours, the places out of $A$ counting as
$-\infty$; one always exists (e.g., the maximum). `findPeak()` of
`LocalOptima.h` keeps the invariant that $A[ \mathit{lo} .. \mathit{hi} ]$
holds a peak of $A$, true when $A[ \mathit{lo} - 1 ] < A[ \mathit{lo} ]$
(or $\mathit{lo} = 0$) and $A[ \mathit{hi} ] \ge A[ \mathit{hi} + 1 ]$ (or
$\mathit{hi} = n - 1$): under it, the maximum of the part is a peak of $A$
([Algorithm 6.14](06-search.md#alg-6-14)).

<span id="lem-6-9"></span>**Lemma 6.9.** *`findPeak()` returns a peak of
$A$ after $\lceil \log_2 n \rceil$ comparisons.*

*Proof.* Comparing $A[ m ]$ with $A[ m + 1 ]$ keeps the invariant on one
of the two halves: if $A[ m ] < A[ m + 1 ]$, the part
$A[ m + 1 .. \mathit{hi} ]$ rises into $m + 1$; otherwise the part
$A[ \mathit{lo} .. m ]$ falls out of $m$. The part halves at each
comparison, and a part of one item is a peak by the invariant. $\square$

<span id="alg-6-14"></span>**Algorithm 6.14.** A peak and a local minimum
of a vector, and a local minimum of a binary tree (Esercizi
Divide-et-impera and Esercizi Alberi of [Montresor
2025](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/LocalOptima-findPeak.pseudo.md"

--8<-- "generated/LocalOptima-localMinimum.pseudo.md"

--8<-- "generated/LocalOptima-localMinimum-2.pseudo.md"

A peak thus costs $\lceil \log_2 n \rceil$ comparisons, against the
$n - 1$ of the maximum, and $\Omega( \log n )$ are needed in the worst
case. On a binary tree, the walk down to a child smaller than the current
node gives a local minimum in $O( h )$ comparisons, $h$ the height. The
same idea works in two dimensions, where a peak of an $n \times n$ matrix
costs $\Theta( n )$ probes by the search on the middle row and the middle
column alternately; on general graphs, however, no such bound exists,
since a local minimum on the hypercube of dimension $d$ needs about
$2^{d / 2}$ evaluations, even for a randomized algorithm ([Aldous
1983](Z-bibliography.md#ref-aldous1983)).

## 6.8 The selection {#sec-6-8}

The item of rank $k$ can be found without sorting. The selection of [Hoare
1961](Z-bibliography.md#ref-hoare1961) (`selection()` of `Selection.h`,
[Montresor 2025, §17.1](Z-bibliography.md#ref-montresor2025)) partitions
the part around a pivot as quicksort does, and goes on only in the side
that holds the rank $k$ ([Algorithm 6.15](06-search.md#alg-6-15)). With
the pivot chosen at random (when a generator is given; otherwise the first
item of the part), its rank is uniform, the side kept is in the worst
case the larger one, and the expected number of comparisons satisfies

$$
T( n ) \le n - 1 + \frac{2}{n} \sum_{q = \lfloor n / 2 \rfloor}^{n - 1}
T( q ) . \tag{6.10}
$$

Substituting $T( q ) \le c q$ gives $T( n ) \le n - 1 + 3 c n / 4 + O( c )$,
i.e., $T( n ) \le c n$ for $c = 4$; hence the selection is linear in
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

*Proof.* Half of the $n / 5$ medians are not larger than the pivot, and
each of them has 3 items of its group not larger than it; the same holds
on the other side. The pivot costs a selection on $n / 5$ items, the part
left has at most $7 n / 10$ of them, and since $1 / 5 + 7 / 10 < 1$ the
work of the levels of the recursion falls geometrically. $\square$

`medianOfMedians()` partitions in three ways (smaller, equal and larger
than the pivot), so that many equal items cost no further round, and
sorts the parts of at most 10 items by insertion. Both functions
rearrange $A$.

The engineered `nthElement()` of `NthElement.h` puts in the place `nth`
of a range the item that would be there if the range were sorted, the
items before it not larger and those after it not smaller, as
`std::nth_element`, which it is meant to replace
([Algorithm 6.16](06-search.md#alg-6-16)). On a range of $n > 600$ items,
the pivot comes from the sampling of [Floyd and Rivest
1975](Z-bibliography.md#ref-floyd1975): a sample of about
$s = n^{2/3} / 2$ items around the place $k$ is selected recursively for
the rank $k$, so that the item that lands in the place $k$ falls, with
high probability, just on the side of $k$ that leaves the smaller part.
The expected number of comparisons is then

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
on a random permutation) it is the scheme of Lomuto with no branch: an
item is moved to the end of the smaller ones and the item there to its
place, and the end advances by the outcome of the comparison, an addition
instead of a jump ([Edelkamp and Weiß
2016](Z-bibliography.md#ref-edelkamp2016) apply the same idea to
quicksort). The partition of Hoare mispredicts half of its comparisons on
random items; the loop is unrolled by 4, so that it does not depend on the
alignment of one jump.

```cpp
--8<-- "generated/NthElement-selectLomuto.cpp"
```

On a range in some order (an organ pipe, a few runs) the partition is the
scheme of Hoare, whose branches are then predicted and which writes only
the items on the wrong side; the sample of Floyd and Rivest is then
gathered from the whole range at a stride, since a contiguous one would
not represent it. A range whose 64 items all rise (or all fall) is checked
to be sorted, by blocks of 64 pairs with no exit inside a block, and is
then left as it is (or reversed in the same pass of the check), in $n$
comparisons. When the pivot is the smallest item of the range, the items
equal to it go to its left, so that many equal items end in one step.
Finally, each round that does not cut the range by at least a quarter
counts against a budget of $2 \lceil \log_2 ( n + 1 ) \rceil$ rounds;
past it the pivot is the median of the medians, which keeps the bound
(6.11) in the worst case. [Table 6.2](06-search.md#tab-6-2) compares the
three selections.

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
(cf. chapter 15). The *sparse table* (`SparseTable` of `RangeMinimum.h`)
stores $M[ L ][ i ] = \mathrm{RMQ}( i , i + 2^L - 1 )$ for each $i$ and
$L$, each from two entries of the row before, in $O( n \log n )$ time and
space. A query takes the better of the two entries of length $2^L$,
$L = \lfloor \log_2 ( j - i + 1 ) \rfloor$, that start at $i$ and end at
$j$; they cover the range, and the minimum does not mind their overlap
([Algorithm 6.17](06-search.md#alg-6-17) and
[Figure 6.7](06-search.md#fig-6-7)).

![Figure 6.7. The query of the range from 2 to 8 (shaded) on a `SparseTable`: the two entries of length $2^2$ that start at 2 and end at 8 overlap, and the better of their minima, 2 at the place 7, is the answer.](../figures/search-sparse.svg){#fig-6-7}

<span id="alg-6-17"></span>**Algorithm 6.17.** The sparse table ([Bender
and Farach-Colton 2000](Z-bibliography.md#ref-bender2000)); the better of
two places is the one of the smaller item, the left one on a tie.

--8<-- "generated/RangeMinimum-SparseTable-build.pseudo.md"

--8<-- "generated/RangeMinimum-SparseTable-query.pseudo.md"

The linear preprocessing rests on a tree. The *Cartesian tree* of $A$
([Vuillemin 1980](Z-bibliography.md#ref-vuillemin1980)) has the leftmost
minimum of $A$ as its root, and the Cartesian trees of the parts on its
left and on its right as its subtrees. `cartesianTree()` builds it in
$O( n )$ time with a stack that holds its right spine, each item being
pushed and popped once ([Algorithm 6.18](06-search.md#alg-6-18)); equal
items go to the right.

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

*Proof.* The lowest common ancestor $a$ of $i$ and $j$ is the first node,
on the path from the root, whose place falls in $[ i , j ]$; the range
thus lies in the subtree of $a$, whose root is its minimum. Between the
two first visits the tour does not leave that subtree and returns to $a$,
the only node of least depth there. $\square$

![Figure 6.8. An array of 7 items and its Cartesian tree (each node labelled by its item, its place beside it), and the nodes $E$ and the depths $D$ of the Euler tour of the tree; the query of the range from 2 to 4 is the node of least depth, 3, between the first visits of 2 and 4 (the shaded part of the tour).](../figures/search-cartesian.svg){#fig-6-8}

The depths along a tour differ by exactly 1 at each step, and the queries
on such an array (`PlusMinusOneRangeMinimum`) are answered in $O( 1 )$
time after $O( n )$ preprocessing. The array is cut into blocks of
$d = \max\{ 1 , \lfloor \log_2 n \rfloor / 2 \}$ items, and a sparse table
on the $n / d$ minima of the blocks costs
$O( ( n / d ) \log n ) = O( n )$. A block is determined, up to an additive
constant, by its $d - 1$ steps of $\pm 1$; hence the answers inside all
the possible blocks are built once in a table (the trick of the Four
Russians, [Arlazarov et al. 1970](Z-bibliography.md#ref-arlazarov1970)) of

$$
2^{d - 1} \cdot d^2 \le \sqrt{n} \, \log_2^2 n = o( n ) \tag{6.13}
$$

entries ([Algorithm 6.19](06-search.md#alg-6-19)). Thus `RangeMinimum`
answers in $O( 1 )$ time after $O( n )$ preprocessing on any array, by
the query on the $2 n - 1$ depths of the tour of its Cartesian tree
([Figure 6.8](06-search.md#fig-6-8)).

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
three, is often faster on the arrays that fit in a cache.

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

*Proof.* The places of (6.14) from $i$ on are the minima from the right
of $A[ i .. j ]$, increasing in their items, and the place $j$ is one of
them; the first of them is thus the minimum, and the leftmost one, since
an item equal to a later one stays on the stack (the condition is
$\le$). $\square$

A query across blocks takes the minimum of the suffix of the block of
$i$, that of the prefix of the block of $j$ and, if there are blocks in
between, that of the two entries of a sparse table on the minima of the
blocks that cover them, the leftmost of the smallest of three
([Algorithm 6.20](06-search.md#alg-6-20)). The minima of the suffixes and
of the prefixes are kept for each place, with their offsets in the block
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

The masks are built by the stacks in $O( n )$ time, or, on the integers
and the floating types under `std::less`, by one comparison of vectors of
the block per item (`greaterBits()`): the larger items are cleared from
the mask of the item before all at once,
$\mathit{mask}[ j ] = ( \mathit{mask}[ j-1 ] \wedge \neg G_j ) \vee
2^{j-b}$ with $G_j$ the bits of the items of the block larger than
$A[ j ]$, with no branch (the stack is increasing, so the items it pops are
exactly those of it larger than $A[ j ]$). The space is

$$
n \big( 8 + 2 \, \mathrm{sizeof}( T ) + 2 \big) +
\Big\lceil \frac{n}{64} \Big\rceil \Big\lceil \log_2
\Big\lceil \frac{n}{64} \Big\rceil \Big\rceil
\big( \mathrm{sizeof}( T ) + 4 \big) \tag{6.15}
$$

bytes besides the array (the masks, the minima of the suffixes and of the
prefixes with their offsets, and the table), on huge pages: about 20 bytes
per item of 32 bits, against the $4 \lceil \log_2 n \rceil$ bytes of a
sparse table on all the items (80 for a million).
[Table 6.3](06-search.md#tab-6-3) summarizes the four structures.

<span id="tab-6-3"></span>**Table 6.3.** The structures of the range
minima in `ADS/search`.

| Structure | Preprocessing | Query | Source |
|---|---|---|---|
| `SparseTable` | $O( n \log n )$ time and space | $O( 1 )$ | Bender and Farach-Colton 2000 |
| `PlusMinusOneRangeMinimum` | $O( n )$ | $O( 1 )$, on steps of $\pm 1$ | Bender and Farach-Colton 2000 |
| `RangeMinimum` | $O( n )$ | $O( 1 )$ | Bender and Farach-Colton 2000 |
| `WordRangeMinimum` | $O( n )$, about 20 bytes per item of 32 bits | $O( 1 )$, five reads at most | Baumstark et al. 2017 |

## 6.10 The searches compared {#sec-6-10}

The searches of the textbooks (`binarySearch()`, `lowerBound()`,
`interpolationSearch()`, the searches of Knuth, `selection()`,
`SparseTable` and `RangeMinimum`) are meant to show the methods and to
check their bounds; the engineered ones are meant to be compared with the
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
Appendix B.

## 6.11 Discussion and references {#sec-6-11}

The binary search, the search by interpolation and the selection of
Hoare are in [Montresor 2025, §1.2, §12.1 and
§17.1](Z-bibliography.md#ref-montresor2025), the searches of Fibonacci and
the uniform binary search in [Knuth 1998,
§6.2.1](Z-bibliography.md#ref-knuth1998), and the unbounded searches in
[Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976); the expected
cost of the interpolation is that of [Perl et al.
1978](Z-bibliography.md#ref-perl1978). [Khuong and Morin
2017](Z-bibliography.md#ref-khuong2017) study the layouts of the arrays for the
searches by comparison, and the S+ tree is that of
[Slotin 2022](Z-bibliography.md#ref-slotin2022). The PGM-index, compressed
and dynamic, is that of [Ferragina and Vinciguerra
2020](Z-bibliography.md#ref-ferragina2020), on the optimal piecewise
linear approximation of [O'Rourke 1981](Z-bibliography.md#ref-orourke1981)
and [Xie et al. 2014](Z-bibliography.md#ref-xie2014) and on the
logarithmic method of [Bentley and Saxe
1980](Z-bibliography.md#ref-bentley1980). The bound of the minimum and the
maximum together is that of [Pohl 1972](Z-bibliography.md#ref-pohl1972),
the selection in linear time that of [Blum et al.
1973](Z-bibliography.md#ref-blum1973), the sampling of [Floyd and Rivest
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
   answer, and compute the integer square root with no overflow. Tests:
   `lib_search.exponential-search`, `lib_search.integer-sqrt`.
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

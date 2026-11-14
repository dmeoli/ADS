# 10. Sets and filters {#ch-10}

[Source: `include/ADS/set/BoolSet.h`, `ListSet.h`, `SortedListSet.h`,
`DictionarySet.h`, `SetOperations.h`, `LinkedHashSet.h`, `HashMultiset.h`,
`RoaringBitmap.h`, `SortedIntersection.h`, `VectorIntersection.h`,
`BloomFilter.h`, `BloomVariants.h`, `BlockedBloomFilter.h`,
`CuckooFilter.h`, `PrefixFilter.h`, `BinaryFuseFilter.h`, `RibbonFilter.h`,
`GrafiteFilter.h`]

We are concerned in this chapter with the SET, i.e., a finite collection of
distinct items of a universe $U$, queried for membership and combined with
other sets by the union, the intersection and the differences. The
headers of `ADS/set` realize the SET of [Montresor 2025, chapter
8](Z-bibliography.md#ref-montresor2025) on a boolean vector
([§10.2](10-set.md#sec-10-2)), on an unsorted and on a sorted list
([§10.3](10-set.md#sec-10-3)) and on any dictionary
([§10.4](10-set.md#sec-10-4)). Beyond the book, they add three families
that the engineering of the sets has produced. The first one is the
compressed bitmaps of Roaring ([§10.5](10-set.md#sec-10-5)), and the
second one the intersection of sorted sequences of [Ferragina 2023,
chapter 6](Z-bibliography.md#ref-ferragina2023)
([§10.6](10-set.md#sec-10-6)). The third one is the *filters*, i.e., the
sets that may answer yes for an item that they do not hold, in exchange for
a space that does not depend on the items ([§10.7](10-set.md#sec-10-7) to
[§10.10](10-set.md#sec-10-10)), together with the filters of ranges of
[§10.11](10-set.md#sec-10-11).

## 10.1 The interface {#sec-10-1}

The operations of the SET are those of the concept `Set` of `ADS/ADT.h`:
`contains(x)`, `insert(x)` and `remove(x)`, together with `forEach(f)`,
which calls `f` on each item, and the four operations of two sets, the
union $A \cup B$, the intersection $A \cap B$, the difference
$A \setminus B$ and the symmetric difference $A \triangle B = ( A \setminus
B ) \cup ( B \setminus A )$. They are called `setUnion()`,
`setIntersection()`, `setDifference()` and `setSymmetricDifference()`,
since `union` is a keyword of C++ ([Montresor 2025,
§8.1](Z-bibliography.md#ref-montresor2025)). Their meaning is fixed by the
axioms

$$
\mathrm{contains}( \mathrm{insert}( S , x ) , x ) , \qquad
\lnot \mathrm{contains}( \mathrm{remove}( S , x ) , x ) , \tag{10.1}
$$

together with the two axioms that say that `insert(x)` and `remove(x)`
leave the answers for $y \ne x$ as they were, which the contracts of the
realizations check as postconditions. A *multiset* may hold an item more
than once, and the number of its occurrences is its *multiplicity*.
Throughout the chapter $n$ and $m$ are the sizes of the two sets of an
operation (with $m \le n$ for the intersections), $N$ the dimension of a
boolean vector, and, in a hash table, $n_X$ and $m_X$ the items and the
size of the table of the set $X$. [Table 10.1](10-set.md#tab-10-1)
summarizes the realizations of the SET; the bounds of the hash tables are
expected, over the choice of the hash function.

<span id="tab-10-1"></span>**Table 10.1.** The realizations of the SET
and the cost of their operations; $c$ is the number of the containers of a
Roaring bitmap.

| Realization | Space | `contains`, `insert`, `remove` | `forEach` | Two sets |
|---|---|---|---|---|
| `BoolSet` | $\Theta( N )$ bits | $O( 1 )$ | $O( N )$ | $\Theta( N )$ |
| `ListSet` | $\Theta( n )$ | $O( n )$ | $O( n )$ | $O( n m )$ |
| `SortedListSet` | $\Theta( n )$ | $O( n )$ | $O( n )$ | $O( n + m )$ |
| `ChainedHashSet` | $\Theta( n_X + m_X )$ | $O( 1 )$ expected | $O( n_X + m_X )$ | $O( \max( m_A , n_A ) + \max( m_B , n_B ) )$ |
| `OpenHashSet` | $\Theta( m_X )$ | $O( 1 )$ expected | $O( m_X )$ | as above |
| `LinkedHashSet` | $\Theta( n_X + m_X )$ | $O( 1 )$ expected | $O( n )$ | as above |
| `TreeSet` | $\Theta( n )$ | $O( \log n )$ | $O( n )$, in order | $O( n + m \log( n + m ) )$ |
| `RoaringBitmap` | $\lesssim \min( 2 n , \vert U \vert / 8 )$ bytes | $O( \log c )$ plus a container | $O( n + c )$ | merge of the containers |

A *filter* of a set $S$ of $n$ keys answers a query $\mathrm{maybe}( x )$
with no *false negative*, i.e., $\mathrm{maybe}( x )$ is true for each
$x \in S$, and with a *false positive*, i.e., true for an $x \notin S$,
with a probability $\varepsilon$ over the choice of its hash functions,
the *false positive rate*. A filter is *static* when it is built once from
$S$, *incremental* when it takes insertions one at a time, and *dynamic*
when it takes removals too.

## 10.2 A set on a boolean vector {#sec-10-2}

`BoolSet` holds a set of the integers of $[ 0 , N )$ as a vector of $N$
booleans: the item $x$ is in the set if and only if its boolean is true
([Montresor 2025, §8.1](Z-bibliography.md#ref-montresor2025)). The
booleans are the bits of words of 64 bits, the item $x$ in the bit
$x \bmod 64$ of the word $\lfloor x / 64 \rfloor$; a counter of the items,
kept by `insert()` and `remove()`, makes `size()` cost $O( 1 )$, and its
invariant (the counter equals the number of bits set) is checked at the
contract level 3. Hence `contains()`, `insert()` and `remove()` read or
write one bit, in $O( 1 )$ time ([Figure 10.1](10-set.md#fig-10-1)), and
`contains()` answers false for an $x$ out of $[ 0 , N )$, whereas
`insert()` requires one in it.

![Figure 10.1. A `BoolSet` of dimension $N = 16$ holding $\{ 1 , 3 , 4 , 9 , 14 \}$, before and after `insert(6)`, which sets one bit (in colour) and increments the size.](../figures/set-bool.svg){#fig-10-1}

The scans go one word at a time. `min()` returns $64 k$ plus the trailing
zeros of the first word $W[ k ] \ne 0$, `max()` does the same from the last
word with the leading zeros, and `forEach()` visits the bits set of each
word from the lowest, clearing it at each step by
$w \gets w \,\&\, ( w - 1 )$ (cf. (18.1) of
[Chapter 18](18-combinatorial.md#ch-18)):

--8<-- "generated/BoolSet-forEach.pseudo.md"

```cpp
--8<-- "generated/BoolSet-forEach.cpp"
```

The bounds stay $O( N )$, but the loop runs $\lceil N / 64 \rceil$ times
plus once per item, and the time falls by up to 64. The operations of two
sets are one operation on each pair of words: $\vee$ for the union,
$\wedge$ for the intersection, $\wedge \lnot$ for the difference and
$\oplus$ for the symmetric difference, where a word missing in the shorter
vector counts as 0 (the dimension of the result is the larger one for the union and
the symmetric difference, the smaller one for the intersection, and that
of $A$ for the difference).

--8<-- "generated/BoolSet-setUnion.pseudo.md"

Two details of the engineering may be worth remarking. First, the size of the result is
counted in the same pass that writes its words; with AVX2, four words at a
time, by the method of [Muła et al. 2018](Z-bibliography.md#ref-mula2018)
described in [§18.3](18-combinatorial.md#sec-18-3) (a shuffle of bytes
looks up the bits of each half byte, and a sum of absolute differences
adds the bytes). Second, the words of the result are written once, and
not filled with zeros before, since its vector uses an allocator that
leaves the items uninitialized; the bits past the dimension of the result
are cleared in its last word, which keeps the invariant. The boolean
vector is therefore preferable when $N$ is not much larger than the sets
(say, a few times their size);
otherwise its cost, which depends on the universe rather than on the items,
makes the realizations below better.

## 10.3 Sets on lists {#sec-10-3}

`ListSet` holds the items in an unsorted LIST, and needs of them only an
equality ([Montresor 2025, §8.2](Z-bibliography.md#ref-montresor2025)).
Thus, `contains()` scans the list, `insert()` scans it and adds the item at the
head if it is not there, and `remove()` scans it and unlinks the item, each
in $O( n )$ time; `insertNew()` adds at the head an item known to be absent,
in $O( 1 )$, which the operations of two sets use whenever the item is new
by construction. The union, for instance, copies $B$ and adds the items of
$A$ not in $B$, and the symmetric difference adds to $A \setminus B$ the
items of $B$ not in $A$, both with $n$ (or $m$) scans of the other list:

--8<-- "generated/ListSet-setUnion.pseudo.md"

The cost $O( n m )$ of these operations cannot be improved when the items
can only be compared for equality.

<span id="lem-10-1"></span>**Lemma 10.1.** *If the items can only be
compared for equality, each algorithm that computes $A \cap B$ makes $n m$
comparisons in the worst case; the same holds for the union, the
difference and the symmetric difference.*

*Proof.* Let an adversary answer "different" to each comparison. If the
algorithm stops without having compared some $a \in A$ with some
$b \in B$, the input in which $a = b$ (and all the other items are
distinct) agrees with each answer given, and the algorithm therefore returns the same
result on it; yet, the intersections of the two inputs differ by $a$. The
same argument holds for the other operations, whose results also change
with $a = b$. $\square$

`SortedListSet` keeps the list in increasing order, and needs a total order
of the items ([Montresor 2025, §8.3](Z-bibliography.md#ref-montresor2025)).
A search stops at the first item not smaller than $x$, and `insert()` puts
$x$ before that item ([Figure 10.2](10-set.md#fig-10-2)); the three
operations still cost $O( n )$, but `min()` and `max()` read the ends of
the list in $O( 1 )$.

![Figure 10.2. An insertion into a `ListSet`, at the head after a scan that does not find the item, and into a `SortedListSet`, before the first item not smaller than it (in colour).](../figures/set-lists.svg){#fig-10-2}

--8<-- "generated/SortedListSet-insert.pseudo.md"

With the lists sorted, the four operations are merges: the two lists are
scanned together, the smaller of their two current items is examined and
passed, and an item of $A$ only, of $B$ only, or of both goes to the
result if the operation takes it (`merge()`, whose three flags select the
operation, [Algorithm 10.1](10-set.md#alg-10-1)).

<span id="alg-10-1"></span>**Algorithm 10.1.** The merge of two sorted
lists ([Montresor 2025, §8.3](Z-bibliography.md#ref-montresor2025)); the
flags $a$, $b$ and $\mathit{both}$ take the items of $A$ only, of $B$ only
and of both.

--8<-- "generated/SortedListSet-merge.pseudo.md"

<span id="lem-10-2"></span>**Lemma 10.2.** *The merge makes at most
$n + m$ comparisons, writes the result in increasing order in $O( n + m )$
time, and is optimal on lists, which allow no access but a scan.*

*Proof.* Each comparison passes at least one item, and the result is
written in increasing order since each item written is the smallest of
those not yet passed. A list must be scanned up to its last item to know
it, which takes $\Omega( n + m )$ steps. $\square$

Also, note that the result is written by `insertLast()`, which appends
after the tail. We
remark that the `C.insert( C.tail() , x )` of the book, with the
`insert()` of the LIST of section 4.1 that puts $x$ before the item in the
given position, would put $x$ before the last item; the header appends at
$\mathrm{next}( \mathrm{tail}() )$, the position past the end, instead.

```cpp
--8<-- "generated/SortedListSet-merge.cpp"
```

## 10.4 Sets on dictionaries {#sec-10-4}

Any DICTIONARY with a `forEach()` realizes the SET: the items are its
keys, and the values are ignored ([Montresor 2025, §8.4 and
§8.5](Z-bibliography.md#ref-montresor2025)). `DictionarySet< D >` does so
for a dictionary `D` of Chapter 7 and
Chapter 8, and three aliases name its usual forms:
`ChainedHashSet` on a hash table with lists of overflow, `OpenHashSet` on
one with open addressing, and `TreeSet` on a Red-Black tree. The three
operations on single items cost $O( 1 )$ expected time on the hash tables
and $O( \log n )$ on the tree; `forEach()` costs $O( n + m )$ on the table
with lists, since it reads the $m$ heads and the $n$ cells, $O( m )$ on the
table with open addressing, and $O( n )$, in order, on the tree.

The same adaptor makes every dictionary of the library a set of Morin.
A dictionary that gives `findKey( k )`, the address of the key $k$ it
holds (a `KeyedDictionary`), makes `DictionarySet` a `USet`, whose
`find( x )` is that address; one that also gives `lowerBoundKey( k )`, the
address of the smallest key not smaller than $k$ (an `OrderedDictionary`),
makes it an `SSet`, whose `lowerBound( x )` is the successor search. The
hash tables are of the first kind, and the search trees, the skip lists,
the B-trees and the sorted array of the second, each at the cost of its
own search; `insert()` and `remove()` say whether they changed the set.
The trees keep their own `lowerBound()`, which gives the key with its
value as a map does, and the adaptor reads the address instead, the item
of the interfaces of Morin. A splay tree, whose searches move its nodes,
sits in the adaptor as a `mutable` member, so that the constant `find()`
of a `USet` still splays and keeps the amortized bound.

The operations of two sets of `SetOperations.h` take any SET that can be
copied and has a `forEach()`. The union copies $A$ and inserts the items of
$B$, and the intersection copies the smaller set and removes its items not
in the other one; the difference copies $A$ and removes its items in $B$,
and the symmetric difference adds to $A \setminus B$ the items of $B$ not
in $A$.

--8<-- "generated/SetOperations-setIntersection.pseudo.md"

```cpp
--8<-- "generated/SetOperations-setIntersection.cpp"
```

Each is a copy, a scan and $n$ or $m$ queries. Hence, on the hash tables
it costs $O( \max( m_A , n_A ) + \max( m_B , n_B ) )$ expected time
(exercise 8.6 of [Montresor 2025](Z-bibliography.md#ref-montresor2025)),
the scans paying the sizes of the tables, and on the trees
$O( n + m \log( n + m ) )$ (exercise 8.7). The realizations that do better
have their own operations, which overload the generic ones (`BoolSet`,
`ListSet`, `SortedListSet` and `RoaringBitmap`). At the contract level 3 a
result is checked item by item against the definition of its operation.

`LinkedHashSet` is a hash table whose `forEach()` follows the order of
insertion (exercise 8.9). A LIST keeps the items in that order, and a table with
lists of overflow (which starts with 7 lists and grows by restructuring)
maps each item to its position in the list
([Figure 10.3](10-set.md#fig-10-3)). An insertion appends the item to the
list and indexes its position, and a removal looks the position up and
unlinks it, both in $O( 1 )$ expected time, while `forEach()` scans the
list in $O( n )$ instead of the $O( n + m )$ of the table.

![Figure 10.3. A `LinkedHashSet` after the insertions of `c`, `a`, `b` and `d` (in colour, the last): the list of the order, whose positions are $p_1 , \ldots , p_4$, and the table of 5 lists that maps each item to its position.](../figures/set-linked.svg){#fig-10-3}

--8<-- "generated/LinkedHashSet-insert.pseudo.md"

--8<-- "generated/LinkedHashSet-remove.pseudo.md"

The invariant, checked at the contract level 3, is that each item of the
list is indexed at its position and that the table has no other item.
Finally, `HashMultiset` realizes a multiset on a hash table with lists of
overflow that maps each item to its multiplicity (exercise 8.10): the
insertion of a new item sets its counter to 1 and that of an item already
there increments it, while a removal decrements the counter and removes the
item when it reaches 0. Each operation costs $O( 1 )$ expected time, and
the invariant is that the size equals the sum of the counters, none of
which is 0.

--8<-- "generated/HashMultiset-remove.pseudo.md"

## 10.5 Compressed bitmaps {#sec-10-5}

The boolean vector takes a bit per integer of the universe, which is too
much for the integers of 32 bits, and the sorted list takes a word per
item, which is too much for the dense sets. The Roaring bitmap of [Chambi
et al. 2016](Z-bibliography.md#ref-chambi2016) interpolates between the
two. It cuts the universe into *chunks* of $2^{16}$ integers by their top
16 bits. Each chunk that holds some item is a *container* of the low 16
bits of its items, kept in a vector sorted by the chunks
([Figure 10.4](10-set.md#fig-10-4)):

* an *array container*, the sorted array of the low bits, when the chunk
  holds at most 4096 items (2 bytes per item, at most 8 KB);
* a *bitmap container*, the $2^{16}$ bits of the chunk in 1024 words
  (8 KB), when it holds more.

Note that the two forms take the same space at 4096 items ($4096 \cdot 16 = 2^{16}$
bits), and the threshold chooses the smaller one in either case.

![Figure 10.4. A `RoaringBitmap` with three containers: the chunks $K$, kept apart for the searches, an array container of 3 items, a bitmap container of 5000 items, and the array into which `insert`$( 5 \cdot 2^{16} + 7 )$ puts the low bits 7 (in colour).](../figures/set-roaring.svg){#fig-10-4}

<span id="lem-10-3"></span>**Lemma 10.3.** *No container of a
`RoaringBitmap` takes more than 8 KB, and a set of $n$ items in $c$
containers takes at most $\min( 2 n , 2^{29} ) + O( c )$ bytes, i.e., about
the smaller of the space of a sorted array of 16 bits per item and that of
a boolean vector on the universe.*

*Proof.* An array container of $k \le 4096$ items takes $2 k \le 8192$
bytes, while a bitmap container takes 8192 bytes and holds $k > 4096$ items,
i.e., fewer than $2 k$ bytes. Hence each container takes at most
$\min( 2 k , 8192 )$ bytes for its $k$ items, and summing over the
containers gives at most $2 n$ bytes and at most $2^{16} \cdot 8192 =
2^{29}$ bytes, the bits of the universe. The keys and the counts of the
containers add $O( c )$. $\square$

A search finds the container by a binary search on the chunks, which are
kept apart in an array, and hence the search reads 2 bytes per step. It
has no branch: the comparison chooses the half by a conditional move.

```cpp
--8<-- "generated/RoaringBitmap-find.cpp"
```

Furthermore, it then tests a bit, or searches the array in the same way, in
$O( \log c )$ time plus $O( \log 4096 )$ in an array
([Algorithm 10.2](10-set.md#alg-10-2)). An insertion sets a bit, or shifts
the array to make room (at most 4096 items, a few lines of cache), and an
array that passes 4096 items becomes a bitmap; a removal clears a bit or
shifts the array, removes a container left empty, and turns into an array a
bitmap that falls to 4096 items.

<span id="alg-10-2"></span>**Algorithm 10.2.** The search and the
insertion of a Roaring bitmap ([Chambi et al.
2016](Z-bibliography.md#ref-chambi2016)).

--8<-- "generated/RoaringBitmap-contains.pseudo.md"

--8<-- "generated/RoaringBitmap-insert.pseudo.md"

The operations of two sets merge the two vectors of the containers by
their chunks, as in [Algorithm 10.1](10-set.md#alg-10-1), and combine the
containers of the same chunk; a chunk of one set only is copied, or
dropped, according to the operation.

--8<-- "generated/RoaringBitmap-setIntersection.pseudo.md"

Two containers are combined according to their forms: (i) two bitmaps
word by word, with the bits of the result counted in the same pass (four
words at a time with AVX2, as in `BoolSet`), and a result of at most 4096
items written as an array; (ii) a bitmap and an array, by testing,
setting, clearing or flipping the bits of the array in a copy of the
bitmap; (iii) two arrays, with SSE4.2, by blocks of 8 items. For the
intersection and the difference, a block of each array is compared all
against all by one comparison of strings (the method of [Schlegel et al.
2011](Z-bibliography.md#ref-schlegel2011)), the items found (or not found
in any block of the other array) are packed by a shuffle and stored, and
the block that ends first moves on. For the union, the two blocks are
merged by the network of minima and maxima of [Inoue and Taura
2015](Z-bibliography.md#ref-inoue2015), the 8 smallest are written but
those equal to the one before them, and the next block comes from the
array with the smaller head; a union or a symmetric difference of more than
4096 items becomes a bitmap. Each pair of containers thus costs
$O( 4096 )$ word operations at most, and typically much less.

The class also has `rank( x )`, the number of items smaller than $x$,
which sums the counts of the containers before that of $x$ and adds the
place of $x$ in it, in $O( c )$ time plus a search or the counts of at most
1024 words; `select( i )`, the item of rank $i$, by the same counts; and
`lowerBound( x )`, an iterator on the first item not smaller than $x$.

--8<-- "generated/RoaringBitmap-select.pseudo.md"

It is fair to remark that the library of Roaring has a third form of
container, for the runs of consecutive integers, and the lazy unions of
many bitmaps whose counts are computed at the end ([Lemire et al.
2018](Z-bibliography.md#ref-lemire2018)); the header has neither. The
bitmaps of the other families (WAH, Concise, EWAH) compress runs of words,
and are reported by [Chambi et al. 2016](Z-bibliography.md#ref-chambi2016)
to be slower to combine.

## 10.6 Intersections of sorted sequences {#sec-10-6}

The intersection of two sorted sequences is the core of the answer to a
query of several words by an inverted index, each word having the sorted
list of the documents that contain it ([Ferragina 2023, chapter
6](Z-bibliography.md#ref-ferragina2023)). There $m \le n$, and the two
lists may differ in length by orders of magnitude; `SortedIntersection.h`
gives the algorithms of that chapter, on vectors of any type with a strict
order, strictly increasing, and [Table 10.2](10-set.md#tab-10-2)
summarizes them.

<span id="lem-10-4"></span>**Lemma 10.4.** *An algorithm that intersects
by comparisons a sorted array of $n$ items with one of $m \le n$ items
makes, in the worst case,*

$$
\log_2 \binom{n}{m} \ge m \log_2 \frac{n}{m} \tag{10.2}
$$

*comparisons.*

*Proof.* When $B \subseteq A$, the algorithm must find out which of the
$\binom{n}{m}$ subsets of $A$ of $m$ items $B$ is, and a tree of binary
comparisons with that many leaves has depth at least the logarithm of
their number; the inequality follows from $\binom{n}{m} \ge
( n / m )^m$. $\square$

The merge of `intersectMerge()` (the procedure of
[Algorithm 10.1](10-set.md#alg-10-1) on arrays) makes at most $n + m$
comparisons, and is optimal when $m$ and $n$ are close, since then (10.2)
is of the order of $n + m$; it is far from (10.2) when $m \ll n$. A binary
search in $A$ of each item of $B$ (`intersectBinarySearch()`) costs
$O( m \log n )$, which is better than the merge when $m = o( n / \log n )$.
The mutual partition of [Baeza-Yates 2004](Z-bibliography.md#ref-baezayates2004)
(`intersectMutualPartition()`) searches the median of the shorter list in
the longer one, which splits both lists into two pairs of shorter lists,
and recurs on each pair, with the roles swapped when the part of the longer
list is the shorter; its cost $O( m ( 1 + \log( n / m ) ) )$ meets (10.2).

--8<-- "generated/SortedIntersection-partition.pseudo.md"

The search by doubling reaches the same bound with no recursion
([Algorithm 10.3](10-set.md#alg-10-3)). Each item of $B$ is searched in
$A$ from the place of the previous one, by probes at the distances
$1 , 2 , 4 , \ldots$ and then a binary search in the last gap (the
exponential search of [Bentley and Yao
1976](Z-bibliography.md#ref-bentley1976)).

<span id="alg-10-3"></span>**Algorithm 10.3.** The intersection by
doubling ([Ferragina 2023, chapter 6](Z-bibliography.md#ref-ferragina2023);
[Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976)).

--8<-- "generated/SortedIntersection-intersectDoubling.pseudo.md"

<span id="lem-10-5"></span>**Lemma 10.5.** *Let the $j$-th item of $B$ be
found (or placed) $\Delta_j$ places after the previous one in $A$. Then
the intersection by doubling makes $O( \sum_j ( 1 + \log \Delta_j ) )$
comparisons, and*

$$
\sum_{j=1}^m ( 1 + \log_2 \Delta_j ) \le m + m \log_2 \frac{n}{m} ;
\tag{10.3}
$$

*hence it costs $O( m ( 1 + \log( n / m ) ) )$ time, which meets (10.2).*

*Proof.* The probes at the distances $1 , 2 , \ldots , 2^i$ stop when
$2^i \ge \Delta_j$, i.e., after $O( 1 + \log \Delta_j )$ comparisons, and
the binary search in the last gap, of at most $2^{i-1}$ places, takes as
many. The gaps sum to at most $n$, and since the logarithm is concave, the
sum of the logarithms of $m$ numbers of a given sum is largest when they
are equal (Jensen), which gives (10.3). $\square$

```cpp
--8<-- "generated/SortedIntersection-intersectDoubling.cpp"
```

The *two-level storage* keeps in an array $A'$ the first item of each
block of $L$ items of $A$; a merge of $A'$ with $B$ finds the block where
each item of $B$ may be, and each block met is merged with its items of
$B$ (`intersectTwoLevel()`), in $O( n / L + m L )$ time, i.e.,
$O( \sqrt{n m} )$ for $L = \sqrt{n / m}$. On the disk, with blocks of $B$
items (cf. Chapter 23), a block of $A$ met costs
at most $\lceil L / B \rceil + 1$ reads, and
`intersectTwoLevelExternal()` takes $O( n / L B + m L / B + m )$ I/Os,
against the $\lceil n / B \rceil + \lceil m / B \rceil$ of
`intersectMergeExternal()`.

--8<-- "generated/SortedIntersection-intersectTwoLevel.pseudo.md"

Finally, the *permuted buckets* of [Sanders and Transier
2007](Z-bibliography.md#ref-sanders2007) map the universe $[ 0 , 2^w )$ by
the permutation $\pi( x ) = ( a x + c ) \bmod 2^w$, with $a$ odd so that
$\pi$ is invertible, and split each list by the top
$\ell = \lceil \log_2( n / L ) \rceil$ bits of its images into buckets of
about $L$ items, sorted and addressed by a table of offsets. An image of
the shorter list is looked for in the bucket of the longer one named by
its top bits; hence a bucket of $A$ is read only if $B$ has an item in
it, at most once, and the time is $O( m + \min\{ n , m L \} )$ expected.
The lists need not be sorted beforehand, only their images are.

--8<-- "generated/SortedIntersection-PermutedBuckets-intersect.pseudo.md"

<span id="tab-10-2"></span>**Table 10.2.** The intersections of a sorted
list of $n$ items and one of $m \le n$ items ([Ferragina 2023, chapter
6](Z-bibliography.md#ref-ferragina2023)).

| Algorithm | Function | Time | Preferable |
|---|---|---|---|
| merge | `intersectMerge()` | $O( n + m )$ | $m$ close to $n$ |
| binary searches | `intersectBinarySearch()` | $O( m \log n )$ | $m = o( n / \log n )$ |
| mutual partition | `intersectMutualPartition()` | $O( m ( 1 + \log( n / m ) ) )$ | optimal by (10.2), recursive |
| doubling | `intersectDoubling()` | $O( m ( 1 + \log( n / m ) ) )$ | optimal, no recursion |
| two levels | `intersectTwoLevel()` | $O( n / L + m L )$ | on the disk, few reads |
| permuted buckets | `PermutedBuckets` | $O( m + \min\{ n , m L \} )$ expected | similar lengths, unsorted lists |

The bounds of the table count comparisons. On a modern processor, however,
the branches that the comparisons decide may cost more than the
comparisons, since those of a merge of two lists that interleave at random
are mispredicted half of the time. `VectorIntersection.h` therefore
engineers the two regimes. On lists of similar lengths (the longer at most
32 times the shorter) it merges with no branch: an item is written whatever
happens, the count of the output advances on equality, and each list
advances when its item is not larger, three additions in place of the
jumps of the merge.

```cpp
--8<-- "generated/VectorIntersection-mergeCommon.cpp"
```

On the integers of 32 bits with AVX2 the merge goes a block of 8 items at a
time on each side, in at most $( n + m ) / 8$ steps. The block of $A$ is
compared with the 8 rotations of the block of $B$, i.e., 8 comparisons of
vectors that find all the equal pairs; the items found are packed to the
front of a vector by a permutation looked up from the mask of the
comparison, and the block whose last item is not larger moves on (both, on
a tie). On lists of very different lengths each item of the shorter one is
looked for in the longer one from the place of the previous one. On any type this is done by galloping
and a binary search with no branch; on the integers of 32 bits with AVX2,
through a window of 64 items that moves by
strides of 1024 and then of 64 items, and the item is compared with the 64
of the window at once. An item at a gap of $g$ places then costs
$\lfloor g / 1024 \rfloor + \min( 16 , \lfloor g / 64 \rfloor ) + O( 1 )$
steps, which the processor predicts.

--8<-- "generated/VectorIntersection-intersect.pseudo.md"

The blocks compared all against all are the method of [Schlegel et al.
2011](Z-bibliography.md#ref-schlegel2011) and of [Lemire et al.
2016](Z-bibliography.md#ref-lemire2016), whose library switches to
galloping at a ratio of 50 rather than 32. The bounds stay those of
[Table 10.2](10-set.md#tab-10-2), and the time should fall by a constant
factor; the comparison belongs to the campaign of Appendix B.

## 10.7 Filters and the filter of Bloom {#sec-10-7}

We now turn to the filters, which trade exactness for space: a filter does
not store its keys, and can therefore take fewer bits than any exact set,
but no fewer than the following bound allows.

<span id="lem-10-6"></span>**Lemma 10.6.** *A filter of any $n$ keys of a
universe $U$, with $n \ll \varepsilon | U |$ and false positive rate
$\varepsilon$, takes at least*

$$
n \log_2 \frac{1}{\varepsilon} \tag{10.4}
$$

*bits ([Carter et al. 1978](Z-bibliography.md#ref-carter1978)).*

*Proof.* A filter of $s$ bits has at most $2^s$ states, and a state answers
yes on a set of about $n + \varepsilon | U |$ keys, which contains at most
$\binom{n + \varepsilon | U |}{n}$ sets of $n$ keys. Since each of the
$\binom{| U |}{n}$ sets of $n$ keys must be contained in the answers of
some state,

$$
2^s \ge \binom{| U |}{n} \Big/ \binom{n + \varepsilon | U |}{n} \approx
\Bigl( \frac{| U |}{\varepsilon | U |} \Bigr)^n = \varepsilon^{-n}
$$

for $n \ll \varepsilon | U |$, which is (10.4). $\square$

The filters of the library differ by how close they come to (10.4), by the
operations that they take, and by the lines of cache that a query reads;
[Table 10.3](10-set.md#tab-10-3) in [§10.10](10-set.md#sec-10-10) compares
them. The oldest one is the filter of [Bloom
1970](Z-bibliography.md#ref-bloom1970): `BloomFilter` keeps $m$ bits, an
insertion sets the bits of $k$ places of the key, and a query answers yes
when the $k$ bits of its key are all set
([Figure 10.5](10-set.md#fig-10-5)).

![Figure 10.5. A `BloomFilter` of $m = 16$ bits: `insert(x)` sets the $k = 3$ bits $2 , 7 , 12$, i.e., $h_1 + i h_2 \bmod m$ with $h_1 = 2$ and $h_2 = 5$, and `maybe(y)`, whose places are $7 , 12 , 1$, answers false at the bit 1.](../figures/set-bloom.svg){#fig-10-5}

<span id="alg-10-4"></span>**Algorithm 10.4.** The insertion and the query
of the filter of Bloom ([Bloom 1970](Z-bibliography.md#ref-bloom1970);
[Kirsch and Mitzenmacher 2006](Z-bibliography.md#ref-kirsch2006)).

--8<-- "generated/BloomFilter-insert.pseudo.md"

--8<-- "generated/BloomFilter-maybe.pseudo.md"

<span id="lem-10-7"></span>**Lemma 10.7.** *Let a filter of $m$ bits set,
for each of $n$ keys, the bits of $k$ hash functions taken as independent
and uniform. A bit is still 0 with probability
$p = ( 1 - 1 / m )^{k n} \approx e^{-k n / m}$, and the false positive
rate is*

$$
\varepsilon = ( 1 - p )^k \approx \bigl( 1 - e^{-k n / m} \bigr)^k ,
\tag{10.5}
$$

*least for $k = ( m / n ) \ln 2$, where $\varepsilon = 2^{-k} \approx
0.6185^{m / n}$; the filter then takes $m = n \log_2( 1 / \varepsilon ) /
\ln 2 \approx 1.44 \, n \log_2( 1 / \varepsilon )$ bits, i.e., 1.44 times
(10.4).*

*Proof.* A key out of $S$ is a false positive when its $k$ bits are all
set, each with probability $1 - p$, which gives (10.5) up to the
dependence of the bits, negligible for large $m$. With
$k = - ( m / n ) \ln p$ one has $\ln \varepsilon = - ( m / n ) \ln p \,
\ln( 1 - p )$, which is symmetric in $p$ and $1 - p$, and hence least at
$p = 1 / 2$, i.e., when half of the bits are set; then $\varepsilon =
2^{-k}$ with $k = ( m / n ) \ln 2$, and solving for $m$ gives the space.
$\square$

For $\varepsilon = 0.01$ the filter takes about 9.6 bits per key, and
`forItems( n , p )` builds the filter of the lemma for $n$ keys and the
rate $p$, with $m = \lceil - n \ln p / ( \ln 2 )^2 \rceil$ and $k$ the
nearest integer to $( m / n ) \ln 2$. The $k$ places of a key are
$h_1 + i h_2 \bmod m$, $i = 0 , \ldots , k - 1$, of two hashes only, both
derived from the hash of the key by `splitmix64()`; [Kirsch and
Mitzenmacher 2006](Z-bibliography.md#ref-kirsch2006) show that this leaves
(10.5) unchanged asymptotically. The step $h_2$ is made odd, so that it is
never 0 and, when $m$ is a power of 2, the first $m$ places are distinct,
and each place follows from the previous one by an addition and a
conditional subtraction, with no remainder:

```cpp
--8<-- "generated/BloomFilter-maybe.cpp"
```

The variants of `BloomVariants.h` change one assumption at a time.
`PartitionedBloomFilter` gives each of its $r$ hashes a part of $m / r$
bits, and hence the $r$ bits of a key are distinct: a bit of a
part is 0 with probability $( 1 - r / m )^n \approx e^{-r n / m}$, and
(10.5) holds with a slightly smaller variance ([Grandi
2018](Z-bibliography.md#ref-grandi2018)). `SpectralBloomFilter`, the
filter of [Cohen and Matias 2003](Z-bibliography.md#ref-cohen2003), keeps
$m$ counters instead of bits, adds the multiplicity of a key to its $r$
counters, and estimates the multiplicity by the least of them. Each counter
of $x$ has received all the occurrences of $x$; hence the estimate is never
below the multiplicity, and it is above it only if each counter of $x$ is
shared with another key, i.e., with the probability (10.5). A removal
subtracts the multiplicity, and a counter that reaches its largest value
sticks there, so that a removal never makes a false negative (the counting
filter of [Fan et al. 2000](Z-bibliography.md#ref-fan2000)).

--8<-- "generated/BloomVariants-SpectralBloomFilter-count.pseudo.md"

--8<-- "generated/BloomVariants-SpectralBloomFilter-remove.pseudo.md"

The other functions of the header are the analysis and two applications.
`bloomFalsePositive()` computes (10.5) exactly, `bloomBestHashes()` the
number of hashes that minimizes it (the better of the two integers around
$( m / n ) \ln 2$), `bloomLowerBound()` the bound (10.4), and `bloomBits()`
the bits of the optimal filter. The compressed filter of [Mitzenmacher
2002](Z-bibliography.md#ref-mitzenmacher2002) is sent through a channel
compressed by an arithmetic coder, in $m \, H( p )$ bits, $H$ the binary
entropy; `compressedBloom()` shows that, for a given number of bits sent, a
larger filter with fewer hashes, whose bits are sparser, gives fewer false
positives than the filter of the optimal $k$.

--8<-- "generated/BloomVariants-compressedBloom.pseudo.md"

Finally, `bloomIntersection()` intersects two sets held by two machines
([Broder and Mitzenmacher 2004](Z-bibliography.md#ref-broder2004)): the
first one sends the filter of $A$, the second one sends back the keys $Q$
of $B$ that the filter accepts, with $A \cap B \subseteq Q$, and the first
one computes $A \cap Q$. About $| A \cap B | + 0.6185^{m_A / | A |} | B |$
keys travel back, instead of the whole of $A$.

--8<-- "generated/BloomVariants-bloomIntersection.pseudo.md"

## 10.8 The blocked filter of Bloom {#sec-10-8}

The $k$ bits of a key of `BloomFilter` are in $k$ lines of cache anywhere,
and a query of a key of $S$ therefore misses the cache $k$ times. The
blocked filter of [Putze et al. 2009](Z-bibliography.md#ref-putze2009)
puts them in one block of 256 bits, one line of cache: `BlockedBloomFilter`
cuts its bits into blocks of 8 words of 32 bits, and a key sets 8 bits of
one block, one in each word ([Figure 10.6](10-set.md#fig-10-6)). The block
is $\lfloor h_1 B / 2^{32} \rfloor$ for $B$ blocks (a product instead of a
remainder, as in [Lemire 2019](Z-bibliography.md#ref-lemire2019)), and the
bit of the word $i$ is the top 5 bits of $h_0 s_i \bmod 2^{32}$ for 8 odd
constants $s_i$, where $h_1$ and $h_0$ are the high and the low halves of
the hash of the key.

![Figure 10.6. A block of a `BlockedBloomFilter`, 8 words of 32 bits, and the 8 bits (in colour) that a key sets, one per word.](../figures/set-blocked.svg){#fig-10-6}

--8<-- "generated/BlockedBloomFilter-maybeHash.pseudo.md"

With AVX2, the 8 bits are made by one product, one shift and one variable
shift of a vector of 8 words, and a query tests them by one instruction
`vptest`; on any machine, by 8 products of 32 bits, with the same result:

```cpp
--8<-- "generated/BlockedBloomFilter-BlockMask.cpp"
```

Putting the bits of a key in one block costs false positives. The blocks
get a Poisson number of keys of mean $\lambda = 256 n / m$, and a block of
$j$ keys, which sets one bit of each of its 8 words per key, answers yes
with probability $( 1 - ( 1 - 1 / 32 )^j )^8$; hence

$$
\varepsilon = \sum_{j \ge 0} e^{-\lambda} \frac{\lambda^j}{j!}
\Bigl( 1 - \Bigl( 1 - \frac{1}{32} \Bigr)^j \Bigr)^8 , \tag{10.6}
$$

which `falsePositiveRate()` computes. The crowded blocks answer yes more
often than the sparse ones save, and (10.6) is above (10.5) with the same
bits: 1.26 per cent against 0.82 (with 7 hashes) at 10 bits per key, and
0.31 against 0.15 per cent at 13.5 bits, the price of one miss of the cache
per query. The split of the 8 bits into 8 words is that of the filters of
Impala and of the format Parquet ([Apache Parquet
2019](Z-bibliography.md#ref-parquet2019)); the number of blocks is any,
not a power of 2, so that the bits per key are those asked. The filter
also takes the union with a filter of the same number of blocks, by an or
of the blocks, and serves as the spare of the prefix filter of
[§10.9](10-set.md#sec-10-9).

## 10.9 Filters of fingerprints {#sec-10-9}

A filter of fingerprints keeps, for each key, a *fingerprint* of $f$ bits
of its hash, and a key out of $S$ is a false positive when its fingerprint
equals one of those that it is compared with. Two of them take insertions
one at a time.

**The cuckoo filter.** `CuckooFilter` is the filter of [Fan et al.
2014](Z-bibliography.md#ref-fan2014): a cuckoo table (cf. [Pagh and Rodler
2004](Z-bibliography.md#ref-pagh2004) and Chapter 7)
of $2^\ell$ buckets of 4 places, which keeps of each key only a
fingerprint of $f = 8$ or 16 bits, never 0, so that 0 marks a free place.
A key with the hash $h$ has the fingerprint $t$, the high $f$ bits of
$h a \bmod 2^{64}$ for an odd constant $a$ (which depend on each bit of
$h$), and the two buckets

$$
b_1 = h_1( h ) \bmod 2^\ell , \qquad
b_2 = b_1 \oplus g( t ) \bmod 2^\ell , \tag{10.7}
$$

where $h_1( h )$ is the high half of $h$ and $g( t )$ the high half of a
product of $t$ by an odd constant (the *partial-key cuckoo hashing*). The
other bucket of a fingerprint is thus known from the fingerprint and
either bucket, $b_1 = b_2 \oplus g( t )$, and a fingerprint can be moved
with no key at hand. An insertion puts $t$ in a free place of $b_1$ or of
$b_2$; if both are full, it evicts the fingerprint of a random place of one
of them and moves it to its other bucket, and so on, at most 500 times
([Figure 10.7](10-set.md#fig-10-7)). If the last fingerprint evicted finds
no place, it is kept in a stash of one entry, the *victim*, that the
queries read too ([Kirsch et al. 2009](Z-bibliography.md#ref-kirsch2009)),
and the next insertion fails: a filter keeps no key and cannot be rehashed,
so an insertion into a full filter returns false and changes nothing.

![Figure 10.7. A `CuckooFilter` of 8 buckets of 4 places: the fingerprint $t$ has the buckets 1 and 6, both full, and takes the place of $u$ in the bucket 1, which moves to its other bucket 3.](../figures/set-cuckoo.svg){#fig-10-7}

--8<-- "generated/CuckooFilter-insertHash.pseudo.md"

The 4 fingerprints of a bucket are one word (32 bits for $f = 8$, 64 for
$f = 16$). A query compares $t$ with the 8 fingerprints of its two buckets
by broadword operations: with $u = w \oplus t \cdot \mathbf{1}$,
$\mathbf{1}$ the word with the bit 0 of each lane set, a lane of $u$ is 0
where the fingerprint equals $t$, and the test of a zero lane is that of
`hasZeroByte()` (cf. [Lemma 18.4](18-combinatorial.md#lem-18-4)). A borrow
may mark a lane above a zero one, but never a word with no zero lane; the
test is therefore exact. The victim is read only when there is one, a
branch taken only once the filter is full:

```cpp
--8<-- "generated/CuckooFilter-maybeHash.cpp"
```

An insertion and a removal also need the place of the fingerprint, and use
the exact form $\lnot( ( ( u \,\&\, \mu ) + \mu ) \mid u \mid \mu )$, with
$\mu$ the word whose lanes have all the bits but the top one set. At the
load $\alpha$ (the fraction of the places taken) a query of a key out of
$S$ compares $t$ with $8 \alpha$ fingerprints on the average, each equal to
$t$ with probability $1 / ( 2^f - 1 )$; hence

$$
\varepsilon = 1 - \Bigl( 1 - \frac{1}{2^f - 1} \Bigr)^{8 \alpha} \le
\frac{8 \alpha}{2^f - 1} , \tag{10.8}
$$

at most 3.1 per cent for $f = 8$ and 0.012 per cent for $f = 16$, in
$f / \alpha$ bits per key. With 2 choices and buckets of $d$ places each
key finds a place with high probability up to the load
$1 / ( 1 + \delta )$ for any $\delta > ( 2 / e )^{d-1}$ ([Dietzfelbinger
and Weidling 2007](Z-bibliography.md#ref-dietzfelbinger2007)), i.e., 0.71
for $d = 4$; the bound is far from tight, and the insertions by 500
evictions reach about 0.95 in practice ([Fan et al.
2014](Z-bibliography.md#ref-fan2014)), from 0.97 at $2^{10}$ buckets to
0.95 at $2^{22}$ in the tests of the library. The constructor for $n$ keys
takes the fewest buckets, a power of 2, that hold them at the load 0.94. A
removal clears one place holding $t$ in $b_1$ or $b_2$; it must concern a
key that was inserted, which no contract can check, since the removal of
another key may clear the fingerprint of a key of $S$, a false negative.

**The prefix filter.** `PrefixFilter` is the incremental filter of [Even
et al. 2022](Z-bibliography.md#ref-even2022), for at most $n$ keys, $n$
fixed when it is built. Its first level is a table of $m$ *bins*, each a
pocket dictionary of [Bercea and Even 2020](Z-bibliography.md#ref-bercea2020)
of 32 bytes (half a line of cache) holding at most $k = 25$ fingerprints;
its second level, the *spare*, is a blocked filter of Bloom
([§10.8](10-set.md#sec-10-8)) that takes the fingerprints for which the
bins have no room. A key with the hash $h$ goes to the bin
$b = \lfloor h_1 m / 2^{32} \rfloor$, and has the mini-fingerprint
$x = \lfloor h_0 s / 2^{32} \rfloor$ in $[ 0 , s )$, $s = 25 \cdot 2^8$,
cut into a quotient $q = \lfloor x / 2^8 \rfloor < 25$ and a remainder
$r = x \bmod 2^8$. A bin has a header of 50 bits, 6 more bits and a body of 25 bytes. The
header writes in unary the number $c_q$ of the remainders of each
quotient, $1^{c_0} 0 \, 1^{c_1} 0 \cdots 1^{c_{24}} 0$ from the low bit;
the 6 bits hold the quotient of the largest fingerprint of the bin once it
has overflowed; and the body holds the remainders in the order of their
quotients, those of one quotient (its *run*) in increasing order
([Figure 10.8](10-set.md#fig-10-8)).

![Figure 10.8. A bin of a `PrefixFilter`, with 4 quotients instead of 25 for brevity: the header writes the counts 2, 0, 3, 1 of the runs in unary from the low bit, and the body holds the remainders; the run of the quotient 2 (in colour) is the slots $[ z_1 + 1 - 2 , z_2 - 2 )$, $z_q$ the place of the zero of rank $q$ of the header.](../figures/set-prefix.svg){#fig-10-8}

The run of $q$ is the slots $[ z_{q-1} + 1 - q , z_q - q )$ of the body,
where $z_q$ is the place of the zero of rank $q$ of the header (and
$z_{-1} = -1$). With BMI2, one `pdep` of $3 \cdot 2^q$ into the complement
of the header, shifted by one place, gives the two zeros at once (cf. the
`select()` of [§18.3](18-combinatorial.md#sec-18-3)); on any machine, the
lowest bit is cleared $q$ times. A query compares the 32 bytes of the bin
with $r$ at once (AVX-512 or AVX2, a byte at a time otherwise) and stops if
no byte of the body is equal, as happens for about 91 per cent of the keys
out of $S$; otherwise it checks whether one of the equal bytes is in the run
of $q$.

--8<-- "generated/PrefixFilter-maybeHash.pseudo.md"

The bins keep the *prefix* of their fingerprints. Each bin holds the
smallest, in the order of $x = 2^8 q + r$, of the fingerprints that came to
it, at most 25. An insertion into a full bin with $x$ not below the
maximum $x_{\max}$ of the bin sends $( b , x )$ to the spare, and one with
$x < x_{\max}$ evicts $x_{\max}$ to the spare and takes its place; either
way the bin is marked as overflowed. Each fingerprint in the spare is then
at least the maximum of its bin, and a query looks in the spare only if its
bin has overflowed and $x > x_{\max}$. Hence a query of a key of $S$ is
always answered yes, and a query typically reads one line of cache, against
the two of the cuckoo filter.

<span id="lem-10-8"></span>**Lemma 10.8.** *Let $n$ keys fall into
$m = n / ( \alpha k )$ bins, so that the load $B$ of a bin is binomial of
mean $\alpha k$. The fingerprints sent to the spare are, on the average,*

$$
\mathrm{E}[ X ] = n - m \, \mathrm{E}[ \min( B , k ) ] , \qquad
\mathrm{E}[ X ] \le \frac{n}{\sqrt{2 \pi k}} \ \text{for } \alpha = 1 ,
\tag{10.9}
$$

*i.e., at most 8.0 per cent of the keys at $\alpha = 1$, and the false
positive rate is at most $\alpha k / s + \varepsilon' / \sqrt{2 \pi k}$,
$\varepsilon'$ that of the spare.*

*Proof.* A bin keeps $\min( B , k )$ of its $B$ fingerprints, which gives
the first identity. For $m = n / k$ the excess equals
$n ( 1 - 1 / m ) \Pr[ B = k ]$, and the bound of Stirling on the binomial
at its mean gives $\Pr[ B = k ] \le 1 / \sqrt{2 \pi k}$ ([Even et al.
2022](Z-bibliography.md#ref-even2022), Theorem 5). A key out of $S$ is a
false positive if its fingerprint is among those of its bin (at most
$\alpha k$ on the average, each equal with probability $1 / s$), or if the
spare answers yes to it, which is queried only when the bin has
overflowed;
the union bound gives the second claim (Theorem 2 of the same paper).
$\square$

The header takes $\alpha = 0.95$, where $\mathrm{E}[ X ]$ is 5.9 per cent
of the keys, and builds the spare for $1.1 \, \mathrm{E}[ X ]$
fingerprints at 12 bits each ($\varepsilon' \approx 0.35$ per cent), as in
the paper. A blocked filter never fails: a spare that gets more
fingerprints only answers yes more often, and each insertion succeeds until
$n$ keys are in. When the filter is full, the false positive rate is 0.39
per cent (0.37 from the fingerprints and 0.02 from the spare), about the
$1 / 256$ of a filter of fingerprints of 8 bits. The bins take
$256 / ( \alpha k ) = 10.78$ bits per key and the spare about 0.77, i.e.,
11.55 in all.

## 10.10 Static filters {#sec-10-10}

A static filter is built once from $S$, and can therefore solve a linear
system over $\mathbb{F}_2$ whose unknowns are its cells: each key gives an
equation, and a key out of $S$ meets its equation with the probability that
a fixed value equals its fingerprint.

<span id="lem-10-9"></span>**Lemma 10.9.** *Let a filter answer
$\mathrm{maybe}( x )$ by testing whether a value $v( x )$, computed from
the cells of the filter at places given by the hash of $x$, equals a
fingerprint $f( x )$ of $b$ bits of the hash, independent of the places.
If the filter satisfies the equations of the keys of $S$, it has no false
negative, and its false positive rate is $2^{-b}$.*

*Proof.* A key of $S$ meets its equation by construction. For a key out of
$S$, the value $v( x )$ is determined by the cells and the places, and the
fingerprint is uniform on the $2^b$ values and independent of them; hence
the two are equal with probability $2^{-b}$. $\square$

**The binary fuse filter.** `BinaryFuseFilter` is the filter of [Graf and
Lemire 2022](Z-bibliography.md#ref-graf2022), the successor of their xor
filter ([Graf and Lemire 2020](Z-bibliography.md#ref-graf2020)). A key
with the hash $h$ has 3 places $h_0 , h_1 , h_2$ of an array $F$ of
fingerprints of $b = 8$ or 16 bits, and is maybe in $S$ if
$F[ h_0 ] \oplus F[ h_1 ] \oplus F[ h_2 ] = f( h )$. The array is cut into
segments of $L$ places, a power of 2: $h_0$ is a place of the first $C L$
ones, and $h_1$, $h_2$ are in the two segments that follow
([Figure 10.9](10-set.md#fig-10-9)). This *spatial coupling* lets the
array have only about $1.125 n$ places for large $n$, against the
$1.23 n$ of 3 places anywhere ([Dietzfelbinger and Walzer
2019a](Z-bibliography.md#ref-dietzfelbinger2019a)); the header takes
$L = 2^{\lfloor \log_{3.33} n + 2.25 \rfloor}$ (at most $2^{18}$) and
about $\max( 1.125 , 0.875 + 0.25 \ln 10^6 / \ln n ) \, n$ places, the
choices of the paper.

![Figure 10.9. The array of a `BinaryFuseFilter`, in segments of $L = 4$ places, and the 3 places of a key (in colour) in 3 consecutive segments.](../figures/set-fuse.svg){#fig-10-9}

The build solves the system by *peeling* ([Algorithm 10.5](10-set.md#alg-10-5)):
a place reached by one key only is solved last for that key, which is then
removed from its other two places, and so on. Each place keeps the number
of its keys, the exclusive or of their hashes and the exclusive or of the
indices (0, 1 or 2) by which they reach it. A place left with one key
therefore gives the hash of that key and which of its places it is, and
the peeling reads and writes only these arrays. If all the keys are
peeled, the fingerprints are assigned in the reverse order of the peeling,
since the place of each key is still free when its key is reached;
otherwise the build
draws a new seed (and gives up with an exception after 1000 seeds). Before
the counts, the hashes are sorted by their segment by a counting sort, so
that the counts and the peeling walk the arrays in order and stay in the
cache; a key repeated in the input has twice the same hash and cannot be
peeled, and a failed build removes the equal hashes before drawing a new
seed.

<span id="alg-10-5"></span>**Algorithm 10.5.** The build of the binary
fuse filter by peeling ([Graf and Lemire
2022](Z-bibliography.md#ref-graf2022)).

--8<-- "generated/BinaryFuseFilter-build.pseudo.md"

**The Ribbon filter.** `RibbonFilter` is the standard Ribbon filter of
[Dillinger and Walzer 2021](Z-bibliography.md#ref-dillinger2021). The filter is the
solution $Z$, a matrix of $m$ rows of $r$ bits, of the system of the $n$
keys. A key with the hash $h$ has a start $s( h )$, coefficients $c( h )$
(a word of 64 bits whose bit 0 is 1) and a fingerprint $f( h )$ of $r$
bits, and its equation is

$$
\bigoplus_{j=0}^{63} c_j( h ) \, Z_{s( h ) + j} = f( h ) . \tag{10.10}
$$

The build solves the system by Gauss elimination *on the fly*: the
equations are kept in echelon form, the row $i$ holding either no equation
or one whose first coefficient is in the column $i$
([Figure 10.10](10-set.md#fig-10-10)). A new equation goes to the row of
its start if it is free; otherwise it is reduced by the equation of that
row and shifted to the first column of the result that is 1, until it
finds a free row. An equation that becomes 0 is redundant if its right side
is 0 too (a repeated key), and makes the system unsolvable otherwise, in
which case the build draws a new seed.

![Figure 10.10. The system of a `RibbonFilter` in echelon form, with windows of 4 coefficients instead of 64: each row holds an equation whose first coefficient (in colour) is in its column, or none (pale), and $f$ is the right side.](../figures/set-ribbon.svg){#fig-10-10}

--8<-- "generated/RibbonFilter-build.pseudo.md"

The rows are then solved from the last, keeping for each of the $r$
columns the word of the 64 rows below, so that a row costs $r$ parities of
a word; with AVX-512 the back substitution takes 4 rows at a time. The
solution is stored interleaved, $r$ words per block of 64 rows, and a
query reads the consecutive words of the two blocks of its 64 rows and
makes each bit of its result by a parity; a query compares the 8 bits of
each line of cache with those of $f( h )$ before reading the next line,
and a key out of $S$ reads 2 lines in most cases.

--8<-- "generated/RibbonFilter-maybe.pseudo.md"

A system of $n = ( 1 - \delta ) m$ equations with random windows of
length $O( \log n / \delta )$ has full rank with high probability
([Dietzfelbinger and Walzer 2019b](Z-bibliography.md#ref-dietzfelbinger2019b));
with windows of 64 fixed, the overhead needed grows with $\log n$, and the
build takes $m = n ( 1 + \delta )$ rows with $\delta = \max( 0.01 ,
0.0083 \log_2 n - 0.025 )$, i.e., about 11 per cent at $10^5$ keys and 14
per cent at $10^6$, or 8.9 and 9.1 bits per key for $r = 8$. After each 4
failed seeds it adds $2$ per cent of $n$ rows.

<span id="tab-10-3"></span>**Table 10.3.** The filters of the library,
their operations, their space and the lines of cache that a query reads,
at about 8 bits of fingerprint.

| Filter | Operations | Bits per key | Lines per query |
|---|---|---|---|
| `BloomFilter` | insertions | $1.44 \log_2( 1 / \varepsilon )$ | $k$ |
| `BlockedBloomFilter` | insertions, union | above, by (10.6) | 1 |
| `SpectralBloomFilter` | insertions, removals, multiplicities | a counter per bit | $r$ |
| `CuckooFilter` | insertions, removals | $f / \alpha$, $\varepsilon$ by (10.8) | 2 |
| `PrefixFilter` | insertions, up to $n$ keys | about 11.55, $\varepsilon \approx 0.39$ % | 1, rarely 2 |
| `BinaryFuseFilter` | static | $1.125 b$, $\varepsilon = 2^{-b}$ | 3 |
| `RibbonFilter` | static | $r ( 1 + \delta )$, $\varepsilon = 2^{-r}$ | 2 for a key out of $S$ |

## 10.11 Range filters {#sec-10-11}

A *range filter* of a set $S$ of $n$ keys of the universe
$[ 0 , 2^{64} )$ answers whether a range $[ a , b ]$ may hold a key of $S$:
it answers true for each range that holds one, and for an empty range of
length $\ell = b - a + 1$ with probability $\varepsilon( \ell )$. A filter
of points answers a range by asking its $\ell$ points, i.e., with $\ell$
queries and up to $\ell$ times their false positives. The filters of
ranges built on tries (SuRF, [Zhang et al. 2018](Z-bibliography.md#ref-zhang2018)),
on filters of prefixes (Rosetta, [Luo et al. 2020](Z-bibliography.md#ref-luo2020))
or on learned models (SNARF, [Vaidya et al. 2022](Z-bibliography.md#ref-vaidya2022);
Proteus, [Knorr et al. 2022](Z-bibliography.md#ref-knorr2022)) answer in
fewer steps, but fail on the *correlated* queries, the empty ranges close to
a key, where their false positives go to 1. `GrafiteFilter`, the filter of
[Costa et al. 2024](Z-bibliography.md#ref-costa2024), has a bound that holds
for any set and any query, since its only randomness is that of a hash.

With $B$ bits per key, Grafite reduces the universe to $[ 0 , r )$,
$r = n \, 2^{B-2}$, by the hash

$$
h( x ) = \bigl( q( \lfloor x / r \rfloor ) + x \bigr) \bmod r , \tag{10.11}
$$

where $q$ is a pairwise independent hash of the blocks of $r$ consecutive
keys into $[ 0 , r )$, and keeps the codes $h( S )$ by Elias-Fano
(Chapter 22) in $n B$ bits. Inside a block, (10.11)
is a rotation; hence a range of $\ell \le r$ keys, which meets at most two
blocks, has for image at most two cyclic ranges of $[ 0 , r )$ of $\ell$
codes in all ([Figure 10.11](10-set.md#fig-10-11)), and a successor query
of Elias-Fano answers each in $O( 1 )$.

![Figure 10.11. A range $[ a , b ]$ that meets two blocks of $r$ keys, each rotated into $[ 0 , r )$ by the hash $q$ of its block, and its image, two cyclic ranges of the same total length.](../figures/set-grafite.svg){#fig-10-11}

--8<-- "generated/GrafiteFilter-build.pseudo.md"

```cpp
--8<-- "generated/GrafiteFilter-mayContain.cpp"
```

<span id="lem-10-10"></span>**Lemma 10.10.** *For every set $S$ and every
range of $\ell$ keys,*

$$
\varepsilon( \ell ) \le \min\Bigl( 1 , \frac{n \ell}{r} \Bigr) =
\min\Bigl( 1 , \frac{\ell}{2^{B-2}} \Bigr) . \tag{10.12}
$$

*Proof.* A key of $S$ in the range has its code in the image of the
range, and there is no false negative. A key $x$ of $S$ out of the range,
in a block that the range meets, has its code out of the image of the part
of the range in its block, since (10.11) is one to one on a block. With
respect to the part of the range in the other block, and to all of it when
$x$ is in a block that the range does not meet, the code of $x$ is uniform
on $[ 0 , r )$ given $q$ of the other block, since $q$ is pairwise
independent; it falls in a stretch of at most $\ell$ codes with
probability at most $\ell / r$. The union bound over the $n$ keys gives
(10.12) ([Costa et al. 2024](Z-bibliography.md#ref-costa2024), Theorem
3.4). $\square$

The lemma generalizes the construction of [Goswami et al.
2015](Z-bibliography.md#ref-goswami2015) to the ranges of any length, and
the lower bound of [Costa et al. 2024](Z-bibliography.md#ref-costa2024),
$n \log_2( \ell / \varepsilon ) - O( n )$ bits, is within 2 bits per key of
Grafite. The hash $q$ of the header is the multiply-add-shift of
[Dietzfelbinger 1996](Z-bibliography.md#ref-dietzfelbinger1996) on 128 bits,
pairwise independent, mapped onto $[ 0 , r )$ by a product, whose values
have probabilities within $1 / r \pm 2^{-64}$; this adds at most
$n \ell 2^{-64}$ to (10.12). The paper uses instead the hash of [Wegman and
Carter 1981](Z-bibliography.md#ref-wegman1981) modulo a prime, which would
need a prime above the $2^{64} / r$ blocks. The keys may be any type that
`radixKey()` maps in order onto unsigned words, floating point numbers
included.

`BucketingFilter`, the heuristic of the same paper, cuts the universe into
buckets of $s = \lceil 2^{64} / ( n \, 2^{B-2} ) \rceil$ keys and keeps by
Elias-Fano the buckets that hold a key of $S$; a range may hold a key if a
kept bucket meets it. It needs no hash and is faster to build and to
query, and it is exact on keys and queries drawn independently at random;
however, an empty range inside the bucket of a key, at distance below $s$
from it, is always a false positive, and no bound like (10.12) holds for
every query.

--8<-- "generated/GrafiteFilter-BucketingFilter-mayContain.pseudo.md"

The dynamic range filters after Grafite, Memento ([Eslami and Dayan
2024](Z-bibliography.md#ref-eslami2024)) and Diva ([Eslami et al.
2025](Z-bibliography.md#ref-eslami2025)), keep a similar guarantee while
they take insertions and keys of any length, at the price of more bits and
slower queries; Oasis ([Chen et al. 2024](Z-bibliography.md#ref-chen2024)),
a learned filter of segments, shares the weakness of the learned filters
on correlated queries.

## 10.12 Discussion and references {#sec-10-12}

The realizations of [§10.2](10-set.md#sec-10-2) to
[§10.4](10-set.md#sec-10-4), with their exercises, are those of chapter 8
of [Montresor 2025](Z-bibliography.md#ref-montresor2025). The Roaring
bitmaps are due to [Chambi et al. 2016](Z-bibliography.md#ref-chambi2016),
and their library is described by [Lemire et al.
2018](Z-bibliography.md#ref-lemire2018). The intersections of sorted
sequences follow chapter 6 of [Ferragina
2023](Z-bibliography.md#ref-ferragina2023); the experiments of [Sanders and
Transier 2007](Z-bibliography.md#ref-sanders2007) and of [Barbay et al.
2009](Z-bibliography.md#ref-barbay2009) find the lookup by buckets best up
to lists of similar length, and the two-level skipping better than the
mutual partition for all but the most unequal lists, whose recursion costs
more than its bound suggests. The adaptive algorithms of [Demaine et al.
2000](Z-bibliography.md#ref-demaine2000) extend the galloping to $k$
lists, and [Ding and König 2011](Z-bibliography.md#ref-ding2011) replace
the buckets by bitmaps of a word. The lower bound on the space of the
filters is that of [Carter et al. 1978](Z-bibliography.md#ref-carter1978);
the filter of [Bloom 1970](Z-bibliography.md#ref-bloom1970) and its
variants are surveyed by [Broder and Mitzenmacher
2004](Z-bibliography.md#ref-broder2004), and chapter 8 of [Ferragina
2023](Z-bibliography.md#ref-ferragina2023) has their analysis. The quotient
filters of Bender et al. (which the library does not have) are another dynamic filter of
fingerprints. The comparison of the realizations, of the intersections and
of the filters with the best implementations available (CRoaring, the
libraries of the filters of their authors) belongs to the campaign of
Appendix B; we give no number here.

## 10.13 Exercises {#sec-10-13}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize the SET on a boolean vector, with the operations of two sets in
   $\Theta( N )$ time. Test: `ch08.sec8.1.boolean`.
2. Keep the boolean vector in words of 64 bits, and make the scans and the
   operations of two sets go a word at a time, counting the size of the
   result in the same pass. Test: `ordered.bool-set-words`.
3. Realize the SET on an unsorted list, and compute the intersection and
   the union of two such sets in $O( n m )$ time. Tests:
   `ch08.sec8.2.unsorted-list`, `ch08.ex8.1`, `ch08.ex8.2`.
4. Realize the SET on a sorted list, and compute the union and the
   difference of two such sets by a merge in $O( n + m )$ time. Tests:
   `ch08.sec8.3.sorted-list`, `ch08.ex8.3`, `ch08.ex8.4`.
5. Compute the symmetric difference of two sets in each realization. Test:
   `ch08.ex8.5`.
6. Realize the SET on a hash table and on a Red-Black tree, and bound the
   cost of their operations of two sets. Tests: `ch08.sec8.4.hash`,
   `ch08.sec8.5.tree`, `ch08.ex8.6`, `ch08.ex8.7`.
7. Realize a set whose iteration follows the order of insertion, with each
   operation in $O( 1 )$ expected time. Test: `ch08.ex8.9`.
8. Realize a multiset on a hash table, with a counter per item. Test:
   `ch08.ex8.10`.
9. Realize the compressed bitmaps of Roaring, with the conversions between
   arrays and bitmaps at 4096 items, `rank()` and `select()`. Test:
   `ordered.roaring-bitmap`.
10. Intersect two sorted lists by the merge, the binary searches, the
    mutual partition, the doubling and the two-level storage, and check the
    numbers of comparisons against the bounds. Tests:
    `pearls_io.ch6.intersection-example`,
    `pearls_io.ch6.intersection-random`.
11. Intersect two lists by the permuted buckets of Sanders and Transier,
    and on the disk by the two-level storage. Tests:
    `pearls_io.ch6.permuted-buckets`, `pearls_io.ch6.intersection-ios`.
12. Intersect two sorted lists with no branch, by blocks of 8 items and
    through a window of 64 items. Test: `ordered.vector-intersection`.
13. Build the filter of Bloom and check its false positives against
    (10.5), and its places $h_1 + i h_2 \bmod m$. Tests:
    `pearls_hashing.bloom-analysis`, `taocp.bloom-filter`,
    `taocp.bloom-probes`.
14. Estimate the multiplicities of a multiset by a spectral filter, and
    intersect two sets on two machines by sending a filter. Tests:
    `pearls_hashing.spectral-bloom`, `pearls_hashing.bloom-intersection`.
15. Build the blocked filter of Bloom and compare its false positives with
    (10.6). Test: `hashing.blocked-bloom`.
16. Build the cuckoo filter, with insertions by evictions and removals, and
    measure its largest load. Test: `hashing.cuckoo-filter`.
17. Build the prefix filter, with its pocket dictionaries and its spare.
    Test: `hashing.prefix-filter`.
18. Build the binary fuse filter by peeling and the Ribbon filter by
    banding, and check that their false positives are about $2^{-b}$.
    Tests: `hashing.binary-fuse`, `hashing.ribbon-filter`.
19. Build the range filter Grafite, and check its bound on correlated
    queries, where the bucketing filter fails. Tests:
    `hashing.grafite-filter`, `hashing.bucketing-filter`.

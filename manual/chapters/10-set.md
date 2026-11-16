# 10. Sets and filters {#ch-10}

[Source: `include/ADS/set/BoolSet.h`, `ListSet.h`, `SortedListSet.h`,
`DictionarySet.h`, `SetOperations.h`, `LinkedHashSet.h`, `HashMultiset.h`,
`RoaringBitmap.h`, `SortedIntersection.h`, `VectorIntersection.h`,
`BloomFilter.h`, `BloomVariants.h`, `BlockedBloomFilter.h`,
`CuckooFilter.h`, `PrefixFilter.h`, `BinaryFuseFilter.h`, `RibbonFilter.h`,
`GrafiteFilter.h`, `SpaceEfficientSortedSet.h`; `SetInterfaces.h` and
`Bag.h` are described in [Chapter 1](01-introduction.md#ch-1)]

We are concerned in this chapter with the SET, i.e., a finite collection of
distinct items of a universe $U$, queried for membership and combined with
other sets by the union, the intersection and the differences. The
headers of `ADS/set` realize the SET of [Montresor 2025, chapter
8](Z-bibliography.md#ref-montresor2025) on a boolean vector
([§10.2](10-set.md#sec-10-2)), on an unsorted and on a sorted list
([§10.3](10-set.md#sec-10-3)) and on any dictionary
([§10.4](10-set.md#sec-10-4)), where the sets of Morin and his
space-efficient sorted set on blocks also find their place. Beyond the book, they add three families
that the engineering of the sets has produced. First come the
compressed bitmaps of Roaring ([§10.5](10-set.md#sec-10-5)), and the
second one the intersection of sorted sequences of [Ferragina 2023,
chapter 6](Z-bibliography.md#ref-ferragina2023)
([§10.6](10-set.md#sec-10-6)). Third come the *filters*, i.e., the
sets that may answer yes for an item that they do not hold, in exchange for
a space that does not depend on the items ([§10.7](10-set.md#sec-10-7) to
[§10.10](10-set.md#sec-10-10)), together with the filters of ranges of
[§10.11](10-set.md#sec-10-11). In the sets of
[§1.9](01-introduction.md#sec-1-9), `BinaryFuseFilter.h`,
`BlockedBloomFilter.h`, `CuckooFilter.h`, `GrafiteFilter.h`,
`PrefixFilter.h`, `RibbonFilter.h`, `RoaringBitmap.h` and
`VectorIntersection.h` are advanced and the other headers of this chapter
are base.

## 10.1 The interface {#sec-10-1}

The operations of the SET are those of the concept `Set` of `ADS/ADT.h`:
`contains( x )`, `insert( x )` and `remove( x )`, together with
`forEach( f )`, which calls `f` on each item, and the four operations of two
sets, the union $A \cup B$, the intersection $A \cap B$, the difference
$A \setminus B$ and the symmetric difference
$A \triangle B = ( A \setminus B ) \cup ( B \setminus A )$. They are called
`setUnion()`, `setIntersection()`, `setDifference()` and
`setSymmetricDifference()`, since `union` is a keyword of C++ ([Montresor
2025, §8.1](Z-bibliography.md#ref-montresor2025)). Their meaning is fixed by
the axioms

$$
\mathrm{contains}( \mathrm{insert}( S , x ) , x ) , \qquad
\lnot \mathrm{contains}( \mathrm{remove}( S , x ) , x ) , \tag{10.1}
$$

together with the two axioms that say that `insert( x )` and `remove( x )`
leave the answers for $y \ne x$ as they were, which the contracts of the
realizations check as postconditions. A *multiset* may hold an item more than
once, and the number of its occurrences is its *multiplicity*. Throughout the
chapter $n$ and $m$ are the sizes of the two sets of an operation (with
$m \le n$ for the intersections), $N$ the dimension of a boolean vector, and,
in a hash table, $n_X$ and $m_X$ the items and the size of the table of the
set $X$. [Table 10.1](10-set.md#tab-10-1) summarizes the realizations of the
SET; the bounds of the hash tables are expected, over the choice of the hash
function.

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
| `SpaceEfficientSortedSet` | $n + O( n / b + b )$ items | $O( \log n )$ expected, $O( \log n + b )$ amortized for the changes | $O( n )$, in order | as `TreeSet` |
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
`insert()` requires one in it; `dim()` gives $N$.

We give the declarations of `ADS/set/BoolSet.h` below, in the namespace
`ADS` as all those of the chapter. The example after them intersects two
sets of the universe $[ 0 , 100 )$.

```cpp
// declarations of <ADS/set/BoolSet.h>
class BoolSet
{
 public:
 using Item = long;
 explicit BoolSet( long N );
 std::size_t size( void ) const;
 bool empty( void ) const;
 long dim( void ) const;
 bool contains( long x ) const;
 void insert( long x );
 void remove( long x );
 std::optional< long > min( void ) const;
 std::optional< long > max( void ) const;
 template< class F >
 void forEach( F && f ) const;
 };
BoolSet setUnion( const BoolSet & A , const BoolSet & B );
// and setIntersection(), setDifference(), setSymmetricDifference() alike
```

```cpp
ADS::BoolSet A( 100 ) , B( 100 );
for( long x : { 3 , 7 , 64 } )
 A.insert( x );
for( long x : { 7 , 64 , 99 } )
 B.insert( x );
const ADS::BoolSet C = setIntersection( A , B );  // { 7 , 64 }
const auto lo = C.min();                         // 7
```

![Figure 10.1. A `BoolSet` of dimension $N = 16$ holding $\{ 1 , 3 , 4 , 9 , 14 \}$, before and after `insert(6)`, which sets one bit (in colour) and increments the size.](../figures/set-bool.svg){#fig-10-1}

Scans go one word at a time: `min()` returns $64 k$ plus the trailing
zeros of the first word $W[ k ] \ne 0$, `max()` does the same from the last
word with the leading zeros, and `forEach()` visits the bits set of each
word from the lowest, clearing it at each step by
$w \gets w \,\&\, ( w - 1 )$ (cf. (18.1) of
[Chapter 18](18-combinatorial.md#ch-18)):

--8<-- "generated/BoolSet-forEach.pseudo.md"

Asymptotically the bounds stay $O( N )$, but the loop runs $\lceil N / 64 \rceil$ times
plus once per item, and the time falls by up to 64. The operations of two
sets are one operation on each pair of words: $\vee$ for the union,
$\wedge$ for the intersection, $\wedge \lnot$ for the difference and
$\oplus$ for the symmetric difference, where a word missing in the shorter
vector counts as 0 (the dimension of the result is the larger one for the union and
the symmetric difference, the smaller one for the intersection, and that
of $A$ for the difference).

--8<-- "generated/BoolSet-setUnion.pseudo.md"

<span id="lem-10-1"></span>**Lemma 10.1.** *On a `BoolSet` of dimension
$N$ and $n$ items, `forEach()` makes $\lceil N / 64 \rceil + n$
iterations, `min()` and `max()` read at most $\lceil N / 64 \rceil$ words,
and an operation of two sets of dimensions $N_A$ and $N_B$ makes
$O( \lceil \max( N_A , N_B ) / 64 \rceil )$ word operations and gives a
result whose counter equals its number of items.*

*Proof.* The outer loop of `forEach()` visits each of the
$\lceil N / 64 \rceil$ words once, and the inner loop runs once per bit
set, since $w \,\&\, ( w - 1 )$ clears the lowest bit set of $w$ (cf.
(18.1)); the trailing zeros of a nonzero word give its lowest bit set, and
hence the item visited. Also, the scans of `min()` and `max()` stop at the first
nonzero word from either end. An operation of two sets writes each word of
the result once, by one operation of $\vee$, $\wedge$, $\wedge \lnot$ or
$\oplus$ on the two words at its place (0 for a word past the end of the
shorter vector), and by De Morgan each bit of the result is the operation
on the two bits. The bits past the dimension of the result are cleared in
its last word, and the counter is the sum of the populations of the words
written, i.e., the number of the bits set. $\square$

Two details of the engineering may be worth remarking. First, the size of the result is
counted in the same pass that writes its words; with `ADS_WITH_HIGHWAY`, a
vector of the processor at a time, by the kernel of `Simd.h` and the count
of the lanes of Highway described in
[§18.3](18-combinatorial.md#sec-18-3). Second, the words of the result are written once, and
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
by construction. For instance, the union copies $B$ and adds the items of
$A$ not in $B$, and the symmetric difference adds to $A \setminus B$ the
items of $B$ not in $A$, both with $n$ (or $m$) scans of the other list:

--8<-- "generated/ListSet-setUnion.pseudo.md"

We declare the class and its operations of two sets as follows.

```cpp
// declarations of <ADS/set/ListSet.h>
template< std::equality_comparable T >
class ListSet
{
 public:
 using Item = T;
 ListSet( void ) = default;
 ListSet( std::initializer_list< T > items );
 std::size_t size( void ) const;
 bool empty( void ) const;
 bool contains( const T & x ) const;
 void insert( const T & x );
 void insertNew( const T & x );
 void remove( const T & x );
 std::optional< T > min( void ) const
  requires std::totally_ordered< T >;
 template< class F >
 void forEach( F && f ) const;
 using const_iterator = typename List< T >::const_iterator;
 };
template< class T >
ListSet< T > setUnion( const ListSet< T > & A , const ListSet< T > & B );
// and setIntersection(), setDifference(), setSymmetricDifference() alike
```

Their cost $O( n m )$ cannot be improved when the items can only be
compared for equality.

<span id="lem-10-2"></span>**Lemma 10.2.** *If the items can only be
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

<span id="lem-10-3"></span>**Lemma 10.3.** *The merge makes at most
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
given position, would put $x$ before the last item; we append at
$\mathrm{next}( \mathrm{tail}() )$, the position past the end, instead.
The class also gives `list()`, i.e., the sorted LIST, read only.

Its declarations are the following.

```cpp
// declarations of <ADS/set/SortedListSet.h>
template< std::totally_ordered T >
class SortedListSet
{
 public:
 using Item = T;
 SortedListSet( void ) = default;
 SortedListSet( std::initializer_list< T > items );
 std::size_t size( void ) const;
 bool empty( void ) const;
 bool contains( const T & x ) const;
 void insert( const T & x );
 void insertLast( const T & x );
 void remove( const T & x );
 std::optional< T > min( void ) const;
 std::optional< T > max( void ) const;
 template< class F >
 void forEach( F && f ) const;
 const List< T > & list( void ) const;
 using const_iterator = typename List< T >::const_iterator;
 };
template< class T >
SortedListSet< T > merge( const SortedListSet< T > & A ,
                          const SortedListSet< T > & B , bool a , bool b ,
                          bool both );
template< class T >
SortedListSet< T > setUnion( const SortedListSet< T > & A ,
                             const SortedListSet< T > & B );
// and setIntersection(), setDifference(), setSymmetricDifference() alike
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

In this way, the adaptor makes every dictionary of the library a set of Morin
([Morin 2013, §1.2.3 and §1.2.4](Z-bibliography.md#ref-morin-cpp)), whose
interfaces `USet` and `SSet` are the concepts of `SetInterfaces.h` described
in [§1.3](01-introduction.md#sec-1-3), with their realizations on a vector,
`ArrayUSet` and `ArraySSet`, and the multiset `Bag` (with its `findAll()`) of
[§1.4](01-introduction.md#sec-1-4). Two more concepts of the same header state
what the adaptor needs of a dictionary `D`. A `KeyedDictionary` has the types
`D::Key` and `D::Value`, an `insert( k , v )`, a `remove( k )` and a
`findKey( k )` that gives the address of the key equal to $k$ that it holds
(nil if there is none). Its search may change the dictionary, but not the set
of its keys. An `OrderedDictionary` is a `KeyedDictionary` with
`lowerBoundKey( k )`, the address of the smallest key not smaller than $k$,
nil if none. On the first `DictionarySet< D >` is a `USet`, whose `find( x )`
is `findKey( x )`, and on the second also an `SSet`, whose `lowerBound( x )`
is `lowerBoundKey( x )`, i.e., the successor search; both members are
constrained by `requires`, and hence a hash table, which has no order, gives
no `lowerBound()`. Hash tables are of the first kind, and the search trees,
the skip lists, the B-trees and the sorted array of the second, each at the
cost of its search. Trees keep their `lowerBound()`, which gives the key with
its value as a map does (a pair or an iterator), and the adaptor reads the
address instead, which is the item of the interfaces of Morin. The dictionary
is a `mutable` member of the adaptor; hence a splay tree, whose searches move
its nodes, still splays in the constant `find()` of a `USet` and keeps the
amortized bound of the tree.

Both concepts `USet` and `SSet` require `insert()` and `remove()` to say
whether they changed the set. Hence the adaptor takes the answer of the
dictionary when its `insert()` and `remove()` return a `bool`, and otherwise
first checks whether the item is there ([Algorithm 10.2](10-set.md#alg-10-2));
a new key gets the value `true` when the values are `bool`, and the value made
by default otherwise. Its `contains( x )` is `findKey( x )` on a
`KeyedDictionary`, and the `contains()` of the dictionary on any other, while
`size()` and `empty()` are those of the dictionary, which `dictionary()`
gives, and `forEach( f )` calls $f$ on the keys in the order of the dictionary
(by its `forEach()` when it has one, by its iterators otherwise). When the
dictionary is a range, `begin()` and `end()` make the set a constant range of
its keys, in the same order.

We declare the adaptor and its three aliases as follows, and the example uses the set on the Red-Black tree as an `SSet`.

```cpp
// declarations of <ADS/set/DictionarySet.h>
template< class D >
class DictionarySet
{
 public:
 using Item = typename D::Key;
 template< class... A >
  requires std::constructible_from< D , A... >
 explicit DictionarySet( A &&... a );
 std::size_t size( void ) const;
 bool empty( void ) const;
 bool contains( const Item & x ) const;
 bool insert( const Item & x );
 bool remove( const Item & x );
 const Item * find( const Item & x ) const
  requires KeyedDictionary< D >;
 const Item * lowerBound( const Item & x ) const
  requires OrderedDictionary< D >;
 template< class F >
 void forEach( F && f ) const;
 const D & dictionary( void ) const;
 auto begin( void ) const
  requires std::ranges::forward_range< const D >;
 auto end( void ) const
  requires std::ranges::forward_range< const D >;
 };
template< class T , class H = std::hash< T > >
using ChainedHashSet =
  DictionarySet< ChainedHashTable< T , bool , Chain::FRONT , H > >;
template< class T , class H = std::hash< T > >
using OpenHashSet = DictionarySet< OpenHashTable< T , bool , H > >;
template< std::totally_ordered T >
using TreeSet = DictionarySet< RedBlackTree< T , bool > >;
```

```cpp
ADS::TreeSet< int > S;
for( int x : { 40 , 10 , 30 } )
 S.insert( x );
const int * y = S.lowerBound( 25 );  // *y == 30
const bool fresh = S.insert( 10 );   // false
```

<span id="alg-10-2"></span>**Algorithm 10.2.** The insertion and the
removal of `DictionarySet` ([Montresor 2025,
§8.4](Z-bibliography.md#ref-montresor2025)); "$D.\mathrm{insert}$ answers"
means that the `insert()` of the dictionary returns whether it added the
key.

--8<-- "generated/DictionarySet-insert.pseudo.md"

--8<-- "generated/DictionarySet-remove.pseudo.md"

<span id="lem-10-4"></span>**Lemma 10.4.** *Each operation of
`DictionarySet< D >` costs one operation of `D`, plus one search of `D`
for an `insert()` or a `remove()` of a dictionary that does not say
whether it changed its keys; hence the set has the bounds of the
dictionary, up to a factor of at most 2.*

*Proof.* The operations of the adaptor are those of
[Algorithm 10.2](10-set.md#alg-10-2) and calls of a single member of `D`,
whose answers they pass on; the dictionary keeps its invariant, and
the postconditions of `insert()` and `remove()` hold since the key was
added, or removed, by `D` or was already there, or already absent. $\square$

The operations of two sets of `SetOperations.h` take any SET that can be
copied and has a `forEach()` (the concept `IterableSet` of the header). Thus the union copies $A$ and inserts the items of
$B$, and the intersection copies the smaller set and removes its items not
in the other one; the difference copies $A$ and removes its items in $B$,
and the symmetric difference adds to $A \setminus B$ the items of $B$ not
in $A$.

--8<-- "generated/SetOperations-setIntersection.pseudo.md"

In `SetOperations.h` the concept and the four templates are declared as follows.

```cpp
// declarations of <ADS/set/SetOperations.h>
template< class S >
concept IterableSet =
  Set< S > && std::copy_constructible< S > &&
  requires( const S s )
{ s.forEach( []( const typename S::Item & ) { } ); };
template< IterableSet S >
S setUnion( const S & A , const S & B );
template< IterableSet S >
S setIntersection( const S & A , const S & B );
template< IterableSet S >
S setDifference( const S & A , const S & B );
template< IterableSet S >
S setSymmetricDifference( const S & A , const S & B );
```

Each is a copy, a scan and $n$ or $m$ queries. Hence, on the hash tables
it costs $O( \max( m_A , n_A ) + \max( m_B , n_B ) )$ expected time
(exercise 8.6 of [Montresor 2025](Z-bibliography.md#ref-montresor2025)),
the scans paying the sizes of the tables, and on the trees
$O( n + m \log( n + m ) )$ (exercise 8.7). Realizations that do better
have specific operations, which overload the generic ones (`BoolSet`,
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

![Figure 10.3. A `LinkedHashSet` after the insertions of `c`, `a`, `b` and `d` (in colour, the last): the list of the order, whose positions are $p_1 , \ldots , p_4$, and the table of lists that maps each item to its position, drawn with 5 lists for brevity (the class starts with 7).](../figures/set-linked.svg){#fig-10-3}

--8<-- "generated/LinkedHashSet-insert.pseudo.md"

--8<-- "generated/LinkedHashSet-remove.pseudo.md"

Its class is declared as follows (its copy assignment is deleted).

```cpp
// declarations of <ADS/set/LinkedHashSet.h>
template< class T , class H = std::hash< T > >
class LinkedHashSet
{
 public:
 using Item = T;
 LinkedHashSet( void );
 LinkedHashSet( const LinkedHashSet & o );
 std::size_t size( void ) const;
 bool empty( void ) const;
 bool contains( const T & x ) const;
 void insert( const T & x );
 void remove( const T & x );
 template< class F >
 void forEach( F && f ) const;
 using const_iterator = typename List< T >::const_iterator;
 };
```

Its invariant, checked at the contract level 3, is that each item of the list
is indexed at its position and that the table has no other item. Finally,
`HashMultiset` realizes a multiset on a hash table with lists of overflow that
maps each item to its multiplicity (exercise 8.10): the insertion of a new
item sets its counter to 1 and that of an item already there increments it,
while a removal decrements the counter and removes the item when it reaches 0.
Each operation costs $O( 1 )$ expected time, `count( x )` gives the
multiplicity of $x$, and the invariant is that the size equals the sum of the
counters, none of which is 0. We remark that `ListSet`, `SortedListSet`,
`LinkedHashSet`, `HashMultiset` (whose items are the pairs of an item and its
count), `BoolSet` and `RoaringBitmap` are also constant ranges, with `begin()`
and `end()` (and `cbegin()` and `cend()`), which visit the items in the order
of their `forEach()`.

--8<-- "generated/HashMultiset-remove.pseudo.md"

Its declarations follow; `distinct()` gives the number of the distinct items, and `size()` the sum of their multiplicities.

```cpp
// declarations of <ADS/set/HashMultiset.h>
template< class T , class H = std::hash< T > >
class HashMultiset
{
 public:
 using Item = T;
 HashMultiset( void );
 std::size_t size( void ) const;
 std::size_t distinct( void ) const;
 bool empty( void ) const;
 bool contains( const T & x ) const;
 std::size_t count( const T & x ) const;
 void insert( const T & x );
 void remove( const T & x );
 template< class F >
 void forEach( F && f ) const;
 };
```

Finally, `SpaceEfficientSortedSet` is the space-efficient sorted set of [Morin
2013, exercise 4.13](Z-bibliography.md#ref-morin-cpp) (his `SESSet`), an
`SSet` of ordered items that needs only the space of its $n$ items and
$O( n / b + b )$ more places, $b \ge 3$ a parameter of the constructor (32 by
default, with the seed of the skip list). The items are kept in increasing
order in a list of *blocks*, as in the `SEList` of [Morin 2013,
§3.3](Z-bibliography.md#ref-morin-cpp) (cf. [Chapter 2](02-sequence.md#ch-2)).
Each block is a circular buffer of $b + 1$ places, and each block but the last
holds from $b - 1$ to $b + 1$ items. A `SkipList` ([Chapter
7](07-dictionary.md#ch-7)) indexes the blocks, the key of a block being its
largest item, read in the block at each comparison. The blocks partition the
sorted items into intervals; hence their order never changes when items move
between neighbours, and the skip list sees only the creation and the removal
of a block (and of a largest item of the last one, which is indexed again).
Its `lowerBound( x )` finds the first block whose largest item is not smaller
than $x$ and then $x$ in it by a binary search, and `find( x )` and
`contains( x )` are a `lowerBound( x )` followed by a test of equality:

--8<-- "generated/SpaceEfficientSortedSet-lowerBound.pseudo.md"

An insertion into a block with room moves the shorter side of the block by
one place, at most $b / 2$ items. If the block of $x$ is full, the
insertion looks for a block with room among the next $b - 1$ ones. It then
moves the last item of each full block before it to the front of the next
one, in $O( 1 )$ time per block in a circular buffer, after which the
block of $x$ has room ([Figure 10.4](10-set.md#fig-10-4)); if the list
of the blocks ends before a block with room, a new last block takes the
last item of the old one. If those $b - 1$ blocks are all full too, a new block follows them, and the $b ( b + 1 )$ items of the
$b$ full blocks are spread over the $b + 1$ blocks, $b$ each. A removal
that leaves a block with $b - 2$ items borrows one item from the next
blocks in the same way, up to the first one with more than $b - 1$ items
or to the last one. If the next $b - 1$ blocks hold $b - 1$ items each, the
items of the $b$ blocks are gathered into the first $b - 1$ of them and the
last one is freed ([Algorithm 10.3](10-set.md#alg-10-3)). This class has no
copy, since its blocks are linked by pointers; `capacity()` gives its
places, `size()`, `empty()` and `forEach()` the rest.

Its declarations are the following.

```cpp
// declarations of <ADS/set/SpaceEfficientSortedSet.h>
template< std::totally_ordered T >
 requires std::semiregular< T >
class SpaceEfficientSortedSet
{
 public:
 using Item = T;
 explicit SpaceEfficientSortedSet( std::size_t b = 32 ,
                                   std::uint64_t seed = 1 );
 ~SpaceEfficientSortedSet();
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 bool contains( const T & x ) const;
 const T * find( const T & x ) const;
 const T * lowerBound( const T & x ) const;
 bool insert( const T & x );
 bool remove( const T & x );
 template< class F >
 void forEach( F && f ) const;
 };
```

![Figure 10.4. A `SpaceEfficientSortedSet` with $b = 4$, i.e., blocks of 5 places, and above each block its key in the skip list, its largest item, before and after `insert(9)`: the block $B_0$ of 9 is full, the last items 14 and 31 of the full blocks $B_0$ and $B_1$ move to the front of the next blocks (violet), and 9 then takes its place in $B_0$ (orange).](../figures/set-sesset.svg){#fig-10-4}

<span id="alg-10-3"></span>**Algorithm 10.3.** The insertion and the
removal of `SpaceEfficientSortedSet` ([Morin 2013, exercise
4.13](Z-bibliography.md#ref-morin-cpp)); $B_0 , B_1 , \ldots$ are the
blocks from that of $x$ on.

--8<-- "generated/SpaceEfficientSortedSet-insert.pseudo.md"

--8<-- "generated/SpaceEfficientSortedSet-remove.pseudo.md"

<span id="lem-10-5"></span>**Lemma 10.5.** *A `SpaceEfficientSortedSet`
of $n$ items has at most $n / ( b - 1 ) + 1$ blocks and at most
$2 ( n / ( b - 1 ) + 1 ) + b + 1$ empty places, i.e., it takes the space
of $n + O( n / b + b )$ items; a search costs $O( \log n )$ expected time,
and any sequence of $m$ insertions and removals from the empty set costs
$O( m ( \log n + b ) )$ expected time, $n$ being the largest size reached.*

*Proof.* The blocks but the last hold at least $b - 1$ items each, which
bounds their number; each of them has at most 2 empty places, and the last
at most $b + 1$ (as the test `ods_trees.ods-ex4.13.sesset` checks). A search
is one search of the skip list on at most $n / ( b - 1 ) + 1$ keys, in
$O( \log n )$ expected time, and a binary search in a block, in
$O( \log b )$. An insertion or a removal costs a search, $O( b )$ for the
moves inside a block and the shifts along at most $b$ blocks, $O( 1 )$
operations of the skip list, and possibly a spread or a gather, each of
$O( b^2 )$ moves. To bound the latter, we follow the proof of [Morin 2013,
Lemma 3.1](Z-bibliography.md#ref-morin-cpp) and call a block *fragile* if
it is not the last and holds $b - 1$ or $b + 1$ items. An operation with no
spread or gather changes the number of items only of the blocks at the
ends of its shifts, plus possibly a new last block. There are at most two
rounds of shifts, since after a round the item may belong to the next
block, which is then full; hence it increases the number $\Phi$ of fragile
blocks by at most a constant $c$. A
spread finds $b$ fragile blocks (full), and leaves $b + 1$ blocks of $b$
items; a gather finds $b - 1$ fragile blocks (of $b - 1$ items) after the
one of $b - 2$, and leaves $b - 2$ blocks of $b$ items and one of $b - 1$.
Each thus decreases $\Phi$ by at least $b - 2 \ge b / 3$. Since
$\Phi \ge 0$ starts at 0, the spreads and the gathers are at most
$3 c m / b$, at the cost of $O( b^2 )$ each, i.e., $O( b m )$ in all.
$\square$

Hence $b$ trades space for time: with $b = \Theta( \log n )$ the set takes
the space of its items plus $O( n / \log n )$, and its operations keep the
bound $O( \log n )$ of a skip list, with $n / b$ nodes of the skip list
instead of $n$ (each with its links and its allocation). Whether the
binary search in a block of contiguous items makes the search faster than
that of a skip list on all the items, which misses the cache at each step,
belongs to the campaign of [Appendix B](B-benchmarks.md#app-B).

## 10.5 Compressed bitmaps {#sec-10-5}

A boolean vector takes a bit per integer of the universe, which is usually too
much for a large universe (e.g., the integers of 32 bits), and the sorted list takes a word per
item, which is too much for the dense sets. The Roaring bitmap of [Chambi
et al. 2016](Z-bibliography.md#ref-chambi2016) interpolates between the
two. It cuts the universe into *chunks* of $2^{16}$ integers by their top
16 bits. Each chunk that holds some item is a *container* of the low 16
bits of its items, and the containers are kept in a vector sorted by the
chunks ([Figure 10.5](10-set.md#fig-10-5)). A container is

* an *array container*, the sorted array of the low bits, when the chunk
  holds at most 4096 items (2 bytes per item, at most 8 KB);
* a *bitmap container*, the $2^{16}$ bits of the chunk in 1024 words
  (8 KB), when it holds more.

Note that the two forms take the same space at 4096 items ($4096 \cdot 16 = 2^{16}$
bits), and the threshold chooses the smaller one in either case.

![Figure 10.5. A `RoaringBitmap` with three containers: the chunks $K$, kept apart for the searches, an array container of 3 items, a bitmap container of 5000 items, and the array into which `insert`$( 5 \cdot 2^{16} + 7 )$ puts the low bits 7 (in colour).](../figures/set-roaring.svg){#fig-10-5}

<span id="lem-10-6"></span>**Lemma 10.6.** *No container of a
`RoaringBitmap` takes more than 8 KB, and a set of $n$ items in $c$
containers takes at most $\min( 2 n , 2^{29} ) + O( c )$ bytes, i.e., about
the smaller of the space of a sorted array of 16 bits per item and that of
a boolean vector on the universe.*

*Proof.* An array container of $k \le 4096$ items takes $2 k \le 8192$
bytes, while a bitmap container takes 8192 bytes and holds $k > 4096$ items,
i.e., fewer than $2 k$ bytes. Hence each container takes at most
$\min( 2 k , 8192 )$ bytes for its $k$ items, and summing over the
containers gives at most $2 n$ bytes and at most $2^{16} \cdot 8192 =
2^{29}$ bytes, the bits of the universe. Finally, the keys and the counts of the
containers add $O( c )$. $\square$

A search finds the container by a binary search on the chunks, which are
kept apart in an array, and hence the search reads 2 bytes per step. It
has no branch, since the comparison chooses the half by a conditional move
([Algorithm 10.4](10-set.md#alg-10-4)); the halves are not equal when the
number of chunks is not a power of 2, and the following lemma shows that
the search is still exact.

<span id="lem-10-7"></span>**Lemma 10.7.** *On a sorted array $K$ of
$c \ge 1$ chunks, `find()` of $\kappa$ returns the first place of $K$ whose chunk is
not smaller than $\kappa$ ($c$ if there is none), after
$\lceil \log_2 c \rceil$ iterations.*

*Proof.* We show that the loop keeps the invariant that the chunks before
the place $\beta$ are smaller than $\kappa$ and those from the place
$\beta + n$ on are not (the places past the end counting as not smaller),
which holds at the start with $\beta = 0$ and $n = c$. Let
$h = \lfloor n / 2 \rfloor$. If $K[ \beta + h ] < \kappa$, all the chunks
up to $\beta + h$ are smaller, since $K$ is sorted, and the new
$\beta + h$ and $n - h$ have the same end $\beta + n$. Otherwise the chunks
from $\beta + h$ on are not smaller, and so are those from
$\beta + n - h \ge \beta + h$ on, which is the new end. When $n = 1$ the
answer is $\beta$ or $\beta + 1$, and the last test chooses between them.
Since $n$ becomes $\lceil n / 2 \rceil$ at each iteration, there are
$\lceil \log_2 c \rceil$ of them. $\square$

Furthermore, the search then tests a bit, or searches the array in the same way, in
$O( \log c )$ time plus $O( \log 4096 )$ in an array
([Algorithm 10.4](10-set.md#alg-10-4)). An insertion sets a bit, or shifts
the array to make room (at most 4096 items, a few lines of cache), and an
array that passes 4096 items becomes a bitmap; a removal clears a bit or
shifts the array, removes a container left empty, and turns into an array a
bitmap that falls to 4096 items.

<span id="alg-10-4"></span>**Algorithm 10.4.** The search of a chunk,
the search and the insertion of a Roaring bitmap ([Chambi et al.
2016](Z-bibliography.md#ref-chambi2016)).

--8<-- "generated/RoaringBitmap-find.pseudo.md"

--8<-- "generated/RoaringBitmap-contains.pseudo.md"

--8<-- "generated/RoaringBitmap-insert.pseudo.md"

The operations of two sets merge the two vectors of the containers by
their chunks, as in [Algorithm 10.1](10-set.md#alg-10-1), and combine the
containers of the same chunk; a chunk of one set only is copied, or
dropped, according to the operation.

--8<-- "generated/RoaringBitmap-setIntersection.pseudo.md"

Two containers are combined according to their forms. Two bitmaps are
combined word by word, with the bits of the result counted in the same
pass (a vector at a time with Highway, as in `BoolSet`), and a result of at
most 4096 items is written as an array. Otherwise, the pair is (ii) a bitmap and an array, by testing,
setting, clearing or flipping the bits of the array in a copy of the
bitmap; (iii) two arrays, with Highway, by blocks of $L \le 8$ items, the
lanes of 16 bits of the vectors of the processor. For the intersection
and the difference, a block of each array is compared all against all by
the comparisons with the $L$ rotations of the other block (the method of
[Schlegel et al. 2011](Z-bibliography.md#ref-schlegel2011), who use one
comparison of strings of SSE4.2), the items found (or not found in any
block of the other array) are compressed and stored, and
the block that ends first moves on. For the union, the two blocks are
merged by the network of minima and maxima of [Inoue and Taura
2015](Z-bibliography.md#ref-inoue2015), the $L$ smallest are written but
those equal to the one before them, and the next block comes from the
array with the smaller head; a union or a symmetric difference of more than
4096 items becomes a bitmap. Each pair of containers thus costs
$O( 4096 )$ word operations at most, and often much less.

Also, the class has `rank( x )`, the number of items smaller than $x$. It
sums the counts of the containers before that of $x$ and adds the place of
$x$ in it, in $O( c )$ time plus a search or the counts of at most 1024
words. Also, `select( i )` gives the item of rank $i$, by the same counts; and
`lowerBound( x )`, an iterator on the first item not smaller than $x$.

--8<-- "generated/RoaringBitmap-select.pseudo.md"

We declare it as follows. Besides the members above, `containers()` and
`bitmaps()` count the containers and those of them that are bitmaps,
`bytes()` gives the space and `items()` the sorted items. The constructor
from an array builds the bitmap of $n$ sorted items, and the example makes
a bitmap container of 5000 even integers.

```cpp
// declarations of <ADS/set/RoaringBitmap.h>
class RoaringBitmap
{
 public:
 using Item = std::uint32_t;
 static constexpr std::uint32_t ARRAY = 4096;
 RoaringBitmap( void ) = default;
 RoaringBitmap( const RoaringBitmap & o );
 RoaringBitmap( const std::uint32_t * a , std::size_t n );
 bool contains( std::uint32_t x ) const;
 bool insert( std::uint32_t x );
 bool remove( std::uint32_t x );
 std::size_t size( void ) const;
 bool empty( void ) const;
 void clear( void );
 void swap( RoaringBitmap & o ) noexcept;
 std::size_t containers( void ) const;
 std::size_t bitmaps( void ) const;
 std::size_t bytes( void ) const;
 template< class F >
 void forEach( F && f ) const;
 std::vector< std::uint32_t > items( void ) const;
 std::size_t rank( std::uint32_t x ) const;
 class const_iterator;
 const_iterator lowerBound( std::uint32_t x ) const;
 const_iterator select( std::size_t i ) const;
 };
RoaringBitmap setUnion( const RoaringBitmap & A , const RoaringBitmap & B );
// and setIntersection(), setDifference(), setSymmetricDifference() alike
```

```cpp
ADS::RoaringBitmap R;
for( std::uint32_t x = 0 ; x < 5000 ; ++x )
 R.insert( 2 * x );
const std::size_t r = R.rank( 101 );      // 51 items below
const std::uint32_t z = *R.select( 10 );  // 20
const std::size_t b = R.bitmaps();        // 1: 5000 > 4096
```

It is fair to remark that the library of Roaring has a third form of
container, for the runs of consecutive integers, and the lazy unions of
many bitmaps whose counts are computed at the end ([Lemire et al.
2018](Z-bibliography.md#ref-lemire2018)); we have neither. The
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

<span id="lem-10-8"></span>**Lemma 10.8.** *An algorithm that intersects
by comparisons a sorted array of $n$ items with one of $m \le n$ items
makes, in the worst case, at least*

$$
\log_2 \binom{n}{m} \ge m \log_2 \frac{n}{m} \tag{10.2}
$$

*comparisons.*

*Proof.* When $B \subseteq A$, the algorithm must find out which of the
$\binom{n}{m}$ subsets of $A$ of $m$ items $B$ is. A tree of binary
comparisons with that many leaves has depth at least the logarithm of
their number, and the inequality follows from $\binom{n}{m} \ge
( n / m )^m$. $\square$

In `intersectMerge()`, the merge (the procedure of
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

The doubling reaches the same bound with no recursion
([Algorithm 10.5](10-set.md#alg-10-5)). Each item of $B$ is searched in
$A$ from the place of the previous one, by probes at the distances
$1 , 2 , 4 , \ldots$ and then a binary search in the last gap (the
exponential search of [Bentley and Yao
1976](Z-bibliography.md#ref-bentley1976)).

<span id="alg-10-5"></span>**Algorithm 10.5.** The intersection by
doubling ([Ferragina 2023, chapter 6](Z-bibliography.md#ref-ferragina2023);
[Bentley and Yao 1976](Z-bibliography.md#ref-bentley1976)).

--8<-- "generated/SortedIntersection-intersectDoubling.pseudo.md"

<span id="lem-10-9"></span>**Lemma 10.9.** *Let the $j$-th item of $B$ be
found (or placed) $\Delta_j \ge 0$ places after the previous one in $A$.
Then the intersection by doubling makes
$O( \sum_j ( 1 + \log_2 ( 1 + \Delta_j ) ) )$ comparisons, and*

$$
\sum_{j=1}^m \bigl( 1 + \log_2 ( 1 + \Delta_j ) \bigr) \le
m + m \log_2 \frac{n + m}{m} \le 2 m + m \log_2 \frac{n}{m} ;
\tag{10.3}
$$

*hence it costs $O( m ( 1 + \log( n / m ) ) )$ time, which meets (10.2).*

*Proof.* The probes at the distances $1 , 2 , \ldots , 2^i$ stop at the
first $i$ with $2^i \ge \Delta_j$, i.e., after at most
$1 + \lceil \log_2 ( 1 + \Delta_j ) \rceil$ comparisons, and the binary
search in the last gap, of at most $2^{i-1}$ places, takes at most as
many. Since the places found do not decrease, the gaps $\Delta_j$ sum to
at most $n$, and hence the numbers $1 + \Delta_j$ sum to at most $n + m$.
Since the logarithm is concave, the sum of the logarithms of $m$ positive
numbers of a given sum is largest when they are equal (the inequality of
Jensen). This gives the first inequality of (10.3), and the
second follows from $n + m \le 2 n$. $\square$

The *two-level storage* keeps in an array $A'$ the first item of each
block of $L$ items of $A$; a merge of $A'$ with $B$ finds the block where
each item of $B$ may be, and each block met is merged with its items of
$B$ (`intersectTwoLevel()`), in $O( n / L + m L )$ time, i.e.,
$O( \sqrt{n m} )$ for $L = \sqrt{n / m}$. On the disk, with blocks of $B$
items (cf. Chapter 23), a block of $A$ met costs at most
$\lceil L / B \rceil + 1$ reads. Hence
`intersectTwoLevelExternal()`, after `twoLevelHeaders()` has written the
first item of each block of $A$ into a separate file by one scan, takes
$O( n / L B + m L / B + m )$ I/Os,
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
Note that the lists need not be sorted beforehand, only their images are.

--8<-- "generated/SortedIntersection-PermutedBuckets-intersect.pseudo.md"

<span id="lem-10-10"></span>**Lemma 10.10.** *The two-level intersection
takes $O( n / L + m + \min( n , m L ) )$ time, and
`intersectTwoLevelExternal()` $O( n / ( L B ) + m ( L / B + 2 ) )$ reads
of blocks of $B$ items. The permuted buckets take
$O( m + \min( n , m \lambda ) + k \log k )$ time, where $\lambda$ is the
largest bucket of the longer list and $k$ the size of the result; if the
images of the keys are uniform and independent, a bucket has on the
average at most $L$ items.*

*Proof.* In the pseudocode of `intersectTwoLevel()` above, the index $h$ only increases and walks $A'$ once, in
$O( n / L )$ steps. Also the pointer $p$ only increases, which bounds
its steps by $n$; moreover, for each $b \in B$ it stays in the block
$[ h L , ( h + 1 ) L )$, which bounds its steps by $L$ per item of $B$.
Each item of $B$ costs $O( 1 )$ more, and the result is correct since $b$
can be in $A$ only in the last block whose first item is not larger than
$b$. On the disk, the headers are read once, in $\lceil n / ( L B ) \rceil$
reads, and each item of $B$ reads at most the
$\lceil L / B \rceil + 1$ blocks of the disk that its block of $A$ meets.
In the permuted buckets, the images of the shorter list are visited once,
and for each of them the pointer $j$ scans its bucket of the longer list
from the place of the previous image; $j$ only increases, which bounds its
steps by $n$, and in a bucket it scans at most $\lambda$ places per image.
Sorting the result at the end costs $O( k \log k )$ time. Finally, with
$\ell = \lceil \log_2 ( n / L ) \rceil$ top bits there are
$2^\ell \ge n / L$ buckets, and $n$ uniform images give a bucket at most
$n / 2^\ell \le L$ items on the average. $\square$

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

We declare the functions of `SortedIntersection.h` as follows; the constructor of `PermutedBuckets` takes the list, the width $w$ of the universe, the length $L$ and the constants $a$ and $c$ of $\pi$; `intersect()` stores in `*scanned`, when given, the places scanned, and `values()` gives the list back.

```cpp
// declarations of <ADS/set/SortedIntersection.h>
class PermutedBuckets
{
 public:
 PermutedBuckets( const std::vector< std::uint64_t > & A , unsigned w ,
                  std::size_t L , std::uint64_t a , std::uint64_t c );
 std::size_t size( void ) const;
 std::size_t buckets( void ) const;
 std::vector< std::uint64_t >
 intersect( const PermutedBuckets & other ,
            std::size_t * scanned = nullptr ) const;
 std::vector< std::uint64_t > values( void ) const;
 };
template< class T , class C = std::less<> >
std::vector< T > intersectMerge( const std::vector< T > & A ,
                                 const std::vector< T > & B , C less = C() );
template< class T , class C = std::less<> >
std::vector< T > intersectBinarySearch( const std::vector< T > & A ,
                                        const std::vector< T > & B ,
                                        C less = C() );
template< class T , class C = std::less<> >
std::vector< T > intersectMutualPartition( const std::vector< T > & A ,
                                           const std::vector< T > & B ,
                                           C less = C() );
template< class T , class C = std::less<> >
std::vector< T > intersectDoubling( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    C less = C() );
template< class T , class C = std::less<> >
std::vector< T > intersectTwoLevel( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    std::size_t L ,
                                    C less = C() );
template< class T >
void twoLevelHeaders( const DiskFile< T > & A , std::size_t L ,
                      DiskFile< T > & H );
template< class T , class C = std::less<> >
void intersectMergeExternal( const DiskFile< T > & A ,
                             const DiskFile< T > & B ,
                             DiskFile< T > & out , C less = C() );
template< class T , class C = std::less<> >
void intersectTwoLevelExternal( const DiskFile< T > & A ,
                                const DiskFile< T > & H , std::size_t L ,
                                const DiskFile< T > & B , DiskFile< T > & out ,
                                C less = C() );
```

These bounds count comparisons. On a modern processor, however,
the branches that the comparisons decide may cost more than the
comparisons, since those of a merge of two lists that interleave at random
are mispredicted half of the time. `VectorIntersection.h` therefore
engineers the two regimes. On lists of similar lengths (the longer at most
32 times the shorter) it merges with no branch: an item is written whatever
happens, the count of the output advances on equality, and each list
advances when its item is not larger, three additions in place of the
jumps of the merge (lines 3 to 7 of
[Algorithm 10.6](10-set.md#alg-10-6)).

<span id="lem-10-11"></span>**Lemma 10.11.** *The merge with no branch writes in
$\mathit{out}[ 0 , k )$ exactly the items common to $a$ and $b$, in
increasing order, after at most $n + m$ iterations.*

*Proof.* At each iteration $x = a[ i ]$ and $y = b[ j ]$. If $x < y$, only
$i$ advances and $k$ stays, so that the $x$ just written is overwritten by
the next write; if $y < x$, only $j$ advances, with the same effect; if
$x = y$, both advance and $k$ grows by 1, which keeps $x$ in the output.
These are the moves of the merge of [Algorithm 10.1](10-set.md#alg-10-1)
with only the flag $\mathit{both}$ set, and by
[Lemma 10.3](10-set.md#lem-10-3) the items kept are the common ones, in
increasing order. Each iteration advances $i$ or $j$, which gives the
bound. $\square$

On the integers of 32 bits with Highway the merge goes a block of
$L \le 8$ items at a time on each side, $L$ the lanes of the vectors of the
processor, in at most $( n + m ) / L$ steps. The block of $A$ is compared
with the $L$ rotations of the block of $B$, i.e., $L$ comparisons of
vectors that find all the equal pairs; the items found are compressed to
the front of a vector, and the block whose last item is not larger moves on (both, on
a tie). On lists of very different lengths each item of the shorter one is
looked for in the longer one from the place of the previous one. On any type this is done by galloping
and a binary search with no branch; on the integers of 32 bits with Highway,
through a window of 64 items that moves by
strides of 1024 and then of 64 items, and the item is compared with the 64
of the window at once. An item at a gap of $g$ places then costs
$\lfloor g / 1024 \rfloor + \min( 16 , \lfloor g / 64 \rfloor ) + O( 1 )$
steps, which the processor predicts.

<span id="alg-10-6"></span>**Algorithm 10.6.** The intersection of
`VectorIntersection.h`, on any type: galloping for lists of very different
lengths, the merge with no branch otherwise.

--8<-- "generated/VectorIntersection-intersect.pseudo.md"

Both functions write the common items into `out`, returning their number, or return them in a vector; the example compares them with the doubling.

```cpp
// declarations of <ADS/set/VectorIntersection.h>
template< class T >
std::size_t intersect( const T * a , std::size_t n , const T * b ,
                       std::size_t m ,
                       T * out );
template< class T >
std::vector< T > intersect( const std::vector< T > & A ,
                            const std::vector< T > & B );
```

```cpp
const std::vector< int > A = { 1 , 3 , 5 , 7 , 9 , 11 } , B = { 3 , 9 };
const auto C = ADS::intersectDoubling( A , B );  // { 3 , 9 }
const auto D = ADS::intersect( A , B );          // { 3 , 9 }
```

Comparing blocks all against all is the method of [Schlegel et al.
2011](Z-bibliography.md#ref-schlegel2011) and of [Lemire et al.
2016](Z-bibliography.md#ref-lemire2016), whose library switches to
galloping at a ratio of 50 rather than 32. Asymptotically, the bounds stay those of
[Table 10.2](10-set.md#tab-10-2), and the time should fall by a constant
factor; the comparison belongs to the campaign of Appendix B.

## 10.7 Filters and the filter of Bloom {#sec-10-7}

We now turn to the filters, which trade exactness for space: a filter does
not store its keys, and can therefore take fewer bits than any exact set,
but no fewer than the following bound allows.

<span id="lem-10-12"></span>**Lemma 10.12.** *Let a filter of $s$ bits
map each set $S$ of $n$ keys of a universe $U$ to a state that answers yes
on all of $S$ and on at most $\varepsilon ( | U | - n )$ other keys. Then*

$$
s \ge n \log_2 \frac{1}{\varepsilon} - \frac{n^2 \log_2 e}{\varepsilon | U |} ,
\tag{10.4}
$$

*i.e., $n \log_2 ( 1 / \varepsilon )$ bits up to a term that vanishes when
$n \ll \varepsilon | U |$ ([Carter et al.
1978](Z-bibliography.md#ref-carter1978)).*

*Proof.* There are at most $2^s$ states, and a state answers yes on a set
of at most $M = n + \varepsilon ( | U | - n ) \le n + \varepsilon | U |$
keys, which contains at most $\binom{M}{n}$ sets of $n$ keys. Since each of
the $\binom{| U |}{n}$ sets of $n$ keys is contained in the yes set of its
own state,

$$
2^s \ge \binom{| U |}{n} \Big/ \binom{M}{n} = \prod_{i=0}^{n-1}
\frac{| U | - i}{M - i} \ge \Bigl( \frac{| U |}{M} \Bigr)^n ,
$$

where each factor is at least $| U | / M$ since $| U | \ge M$. Hence
$s \ge n \log_2 ( | U | / ( n + \varepsilon | U | ) ) = n \log_2
( 1 / \varepsilon ) - n \log_2 ( 1 + n / ( \varepsilon | U | ) )$, and
$\log_2 ( 1 + z ) \le z \log_2 e$ gives (10.4). $\square$

A filter whose rate $\varepsilon$ holds on the average over its random hash
functions is covered as well, up to $O( n )$ bits: for some fixed choice
of the hash functions the rate averaged over the sets is at most
$\varepsilon$, at least half of the sets then have a rate at most
$2 \varepsilon$ by the inequality of Markov, and the argument above
applied to them loses $n + 1$ bits.

Filters differ by how close they come to (10.4), by the
operations that they take, and by the lines of cache that a query reads;
[Table 10.3](10-set.md#tab-10-3) in [§10.10](10-set.md#sec-10-10) compares
them. Of these, the oldest is the filter of [Bloom
1970](Z-bibliography.md#ref-bloom1970): `BloomFilter` keeps $m$ bits, an
insertion sets the bits of $k$ places of the key, and a query answers yes
when the $k$ bits of its key are all set
([Algorithm 10.7](10-set.md#alg-10-7) and
[Figure 10.6](10-set.md#fig-10-6)).

![Figure 10.6. A `BloomFilter` of $m = 16$ bits: `insert(x)` sets the $k = 3$ bits $2 , 7 , 12$, i.e., $h_1 + i h_2 \bmod m$ with $h_1 = 2$ and $h_2 = 5$, and `maybe(y)`, whose places are $7 , 12 , 1$, answers false at the bit 1.](../figures/set-bloom.svg){#fig-10-6}

<span id="alg-10-7"></span>**Algorithm 10.7.** The insertion and the query
of the filter of Bloom ([Bloom 1970](Z-bibliography.md#ref-bloom1970);
[Kirsch and Mitzenmacher 2006](Z-bibliography.md#ref-kirsch2006)).

--8<-- "generated/BloomFilter-insert.pseudo.md"

--8<-- "generated/BloomFilter-maybe.pseudo.md"

<span id="lem-10-13"></span>**Lemma 10.13.** *Let a filter of $m$ bits set,
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

*Proof.* A bit stays 0 if none of the $k n$ independent uniform places
hits it, with probability $p = ( 1 - 1 / m )^{k n}$. A key out of $S$ is a
false positive when its $k$ bits are all set, each with probability
$1 - p$. This gives (10.5) up to the dependence of the bits, negligible
for large $m$ (`bloomFalsePositive()` computes the exact rate). We
take $p \approx e^{-k n / m}$, i.e., $k = - ( m / n ) \ln p$, as the
variable, and have $\ln \varepsilon = k \ln ( 1 - p ) = - ( m / n ) f( p )$
with $f( p ) = \ln p \, \ln ( 1 - p ) > 0$; hence $\varepsilon$ is least
where $f$ is largest. One has $f'( p ) = ( \varphi( 1 - p ) - \varphi( p ) )
/ ( p ( 1 - p ) )$ with $\varphi( x ) = x \ln x$. The function
$\psi( p ) = \varphi( p ) - \varphi( 1 - p )$ tends to 0 at $0^+$, is 0 at
$1 / 2$, and has the derivative $2 + \ln( p ( 1 - p ) )$, which is negative
and then positive on $( 0 , 1 / 2 )$; hence $\psi < 0$ there, $f$
increases on $( 0 , 1 / 2 )$, and by the symmetry
$f( p ) = f( 1 - p )$ it is largest at $p = 1 / 2$, i.e., when half of the
bits are set. Then $\varepsilon = 2^{-k}$ with $k = ( m / n ) \ln 2$, and
solving $\log_2 ( 1 / \varepsilon ) = ( m / n ) \ln 2$ for $m$ gives the
space. $\square$

For $\varepsilon = 0.01$ the filter takes about 9.6 bits per key, and
`forItems( n , p )` builds the filter of the lemma for $n$ keys and the
rate $p$, with $m = \lceil - n \ln p / ( \ln 2 )^2 \rceil$ and $k$ the
nearest integer to $( m / n ) \ln 2$. Here the $k$ places of a key are
$h_1 + i h_2 \bmod m$, $i = 0 , \ldots , k - 1$, of two hashes only, both
derived from the hash of the key by `splitmix64()`; [Kirsch and
Mitzenmacher 2006](Z-bibliography.md#ref-kirsch2006) show that this leaves
(10.5) unchanged asymptotically. Moreover, the step $h_2$ is made odd, so that it is
never 0 and, when $m$ is a power of 2, the first $m$ places are distinct,
and each place follows from the previous one by an addition and a
conditional subtraction, with no remainder (both $h_1$ and $h_2$ being
first reduced modulo $m$). Indeed, an odd $h_2$ is invertible modulo a
power of 2, and $i h_2 \equiv j h_2 \pmod{m}$ then gives $i \equiv j$.

We declare the class as follows; `falsePositiveRate()` gives (10.5) at the current number of keys, and `bits()` and `hashes()` give $m$ and $k$. Its example also uses the cuckoo filter of [§10.9](10-set.md#sec-10-9).

```cpp
// declarations of <ADS/set/BloomFilter.h>
template< class T , class H = std::hash< T > >
class BloomFilter
{
 public:
 BloomFilter( std::size_t m , int k , H h = H() );
 static BloomFilter forItems( std::size_t n , double p );
 void insert( const T & x );
 bool maybe( const T & x ) const;
 double falsePositiveRate( void ) const;
 std::size_t bits( void ) const;
 int hashes( void ) const;
 };
```

```cpp
auto F = ADS::BloomFilter< std::string >::forItems( 1000 , 0.01 );
F.insert( "apple" );
const bool yes = F.maybe( "apple" );  // true, always
const int k = F.hashes();             // 7
ADS::CuckooFilter< std::uint64_t > G( 1000 );
const bool in = G.insert( 42 ) && G.maybe( 42 );  // true
G.remove( 42 );
```

In `BloomVariants.h`, each variant changes one assumption at a time.
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
sticks there (`stuck()` counts them), so that a removal never makes a false negative (the counting
filter of [Fan et al. 2000](Z-bibliography.md#ref-fan2000)).

--8<-- "generated/BloomVariants-SpectralBloomFilter-count.pseudo.md"

--8<-- "generated/BloomVariants-SpectralBloomFilter-remove.pseudo.md"

<span id="lem-10-14"></span>**Lemma 10.14.** *Let each removal of a
`SpectralBloomFilter` remove occurrences that were inserted, and let
$\mathit{top}$ be the largest value of a counter. Then `count( x )` is at
least $\min( \mathit{top} , \mu( x ) )$, $\mu( x )$ the multiplicity of $x$,
and in particular the filter has no false negative.*

*Proof.* We show that each counter $C[ j ]$ is $\mathit{top}$ or at least
the sum $\sigma_j$ of the multiplicities of the keys that have $j$ among
their places (counted once per such place). An insertion of $f$
occurrences of $x$ adds $f$ to $\sigma_j$ and to $C[ j ]$ for each place
$j$ of $x$, or sets $C[ j ]$ to $\mathit{top}$ if the sum would exceed it.
A removal subtracts $f$ from $\sigma_j$ and from each $C[ j ] \ne
\mathit{top}$, and leaves a counter at $\mathit{top}$ as it is. Both keep
the claim, which holds at the start, when all is 0. Since
$\sigma_j \ge \mu( x )$ for every place $j$ of $x$, the least counter of
$x$ is at least $\min( \mathit{top} , \mu( x ) )$, which is positive for
$x$ in the multiset. $\square$

Its other functions are the analysis and two applications:
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

We declare them as follows; `counters()` and `hashes()` give $m$ and $r$, and `ones()` the bits set of a partitioned filter.

```cpp
// declarations of <ADS/set/BloomVariants.h>
template< class T , class C = std::uint32_t , class H = std::hash< T > >
class SpectralBloomFilter
{
 public:
 SpectralBloomFilter( std::size_t m , int r , H h = H() );
 void insert( const T & x , C f = 1 );
 void remove( const T & x , C f = 1 );
 C count( const T & x ) const;
 bool maybe( const T & x ) const;
 std::size_t stuck( void ) const;
 std::size_t counters( void ) const;
 int hashes( void ) const;
 };
template< class T , class H = std::hash< T > >
class PartitionedBloomFilter
{
 public:
 PartitionedBloomFilter( std::size_t m , int r , H h = H() );
 void insert( const T & x );
 bool maybe( const T & x ) const;
 std::size_t ones( void ) const;
 std::size_t bits( void ) const;
 };
double bloomFalsePositive( double m , double n , int r );
int bloomBestHashes( double m , double n );
double bloomLowerBound( double n , double epsilon );
double bloomBits( double n , double epsilon );
std::pair< double , double > compressedBloom( double bitsPerKey , int r );
template< class T , class H = std::hash< T > >
std::pair< std::vector< T > , std::vector< T > >
bloomIntersection( const std::vector< T > & A , const std::vector< T > & B ,
                   std::size_t mA );
```

## 10.8 The blocked filter of Bloom {#sec-10-8}

The $k$ bits of a key of `BloomFilter` are in $k$ lines of cache anywhere,
and a query of a key of $S$ therefore misses the cache $k$ times. The
blocked filter of [Putze et al. 2009](Z-bibliography.md#ref-putze2009)
puts them in one block of 256 bits, one line of cache. Thus
`BlockedBloomFilter` cuts its bits into blocks of 8 words of 32 bits, and a
key sets 8 bits of one block, one in each word
([Figure 10.7](10-set.md#fig-10-7)). The block
is $\lfloor h_1 B / 2^{32} \rfloor$ for $B$ blocks (a product instead of a
remainder, as in [Lemire 2019](Z-bibliography.md#ref-lemire2019)), and the
bit of the word $i$ is the top 5 bits of $h_0 s_i \bmod 2^{32}$ for 8 odd
constants $s_i$, where $h_1$ and $h_0$ are the high and the low halves of
the hash of the key.

![Figure 10.7. A block of a `BlockedBloomFilter`, 8 words of 32 bits, and the 8 bits (in colour) that a key sets, one per word.](../figures/set-blocked.svg){#fig-10-7}

--8<-- "generated/BlockedBloomFilter-maybeHash.pseudo.md"

With Highway, the 8 bits are made by one product, one shift and one
variable shift of the vectors of 8 words, and a query tests them by an
and-not and one test of the vectors; on any machine, by 8 products of 32
bits, with the same result
(the mask $M$ of line 2 of the algorithm above has in its word $i$ the
single bit $\lfloor ( h_0 s_i \bmod 2^{32} ) / 2^{27} \rfloor$).

In fact, putting the bits of a key in one block costs some false positives. The blocks
get a Poisson number of keys of mean $\lambda = 256 n / m$, and a block of
$j$ keys, which sets one bit of each of its 8 words per key, answers yes
with probability $( 1 - ( 1 - 1 / 32 )^j )^8$; hence

$$
\varepsilon = \sum_{j \ge 0} e^{-\lambda} \frac{\lambda^j}{j!}
\Bigl( 1 - \Bigl( 1 - \frac{1}{32} \Bigr)^j \Bigr)^8 , \tag{10.6}
$$

which `falsePositiveRate()` computes. Crowded blocks answer yes more
often than the sparse ones save, and (10.6) is above (10.5) with the same
bits: 1.26 per cent against 0.82 (with 7 hashes) at 10 bits per key, and
0.31 against 0.15 per cent at 13.5 bits, the price of one miss of the cache
per query. Splitting the 8 bits into 8 words is that of the filters of
Impala and of the format Parquet ([Apache Parquet
2019](Z-bibliography.md#ref-parquet2019)); the number of blocks is any,
not a power of 2, so that the bits per key are those asked. The filter
also takes the union with a filter of the same number of blocks, by an or
of the blocks (`unite()`), and serves as the spare of the prefix filter of
[§10.9](10-set.md#sec-10-9).

Its constructor takes the number $n$ of the keys and the bits per key (10 by default); `insertHash()` and `maybeHash()` take a hash already computed, `blocks()` gives $B$, and `clear()` empties the filter.

```cpp
// declarations of <ADS/set/BlockedBloomFilter.h>
template< class T , class H = DefaultHash< T > >
class BlockedBloomFilter
{
 public:
 explicit BlockedBloomFilter( std::size_t n , double bitsPerItem = 10 ,
                              H h = H() );
 void insert( const T & x );
 bool maybe( const T & x ) const;
 void insertHash( std::uint64_t h );
 bool maybeHash( std::uint64_t h ) const;
 void unite( const BlockedBloomFilter & o );
 void clear( void );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t bits( void ) const;
 std::size_t blocks( void ) const;
 double falsePositiveRate( void ) const;
 };
```

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
([Figure 10.8](10-set.md#fig-10-8)). If the last fingerprint evicted finds
no place, it is kept in a stash of one entry, the *victim*, that the
queries read too ([Kirsch et al. 2009](Z-bibliography.md#ref-kirsch2009)),
and the next insertion fails: a filter keeps no key and cannot be rehashed,
so an insertion into a full filter returns false and changes nothing.

![Figure 10.8. A `CuckooFilter` of 8 buckets of 4 places: the fingerprint $t$ has the buckets 1 and 6, both full, and takes the place of $u$ in the bucket 1, which moves to its other bucket 3.](../figures/set-cuckoo.svg){#fig-10-8}

--8<-- "generated/CuckooFilter-insertHash.pseudo.md"

Together, the 4 fingerprints of a bucket are one word (32 bits for $f = 8$, 64 for
$f = 16$). A query compares $t$ with the 8 fingerprints of its two buckets
by broadword operations: with $u = w \oplus t \cdot \mathbf{1}$,
$\mathbf{1}$ the word with the bit 0 of each lane set, a lane of $u$ is 0
where the fingerprint equals $t$, and the test of a zero lane is that of
`hasZeroByte()` (cf. [Lemma 18.6](18-combinatorial.md#lem-18-6)). A borrow
may mark a lane above a zero one, but never a word with no zero lane; the
test is therefore exact. The victim $v$ is read only when there is one, a branch taken only
once the filter is full.

--8<-- "generated/CuckooFilter-maybeHash.pseudo.md"

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
removal clears one place holding $t$ in $b_1$ or $b_2$. (The members
`insertHash()` and `removeHash()` take a hash already computed, as
`maybeHash()` does, and `capacity()`, `load()` and `bits()` give the
places, $\alpha$ and the space.) The removal must concern a
key that was inserted, which no contract can check, since the removal of
another key may clear the fingerprint of a key of $S$, a false negative.

Its parameter `F` is `std::uint8_t` or `std::uint16_t`, i.e., $f = 8$ or 16.

```cpp
// declarations of <ADS/set/CuckooFilter.h>
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class CuckooFilter
{
 public:
 explicit CuckooFilter( std::size_t n , H h = H() );
 bool insert( const T & x );
 bool remove( const T & x );
 bool maybe( const T & x ) const;
 bool insertHash( std::uint64_t h );
 bool removeHash( std::uint64_t h );
 bool maybeHash( std::uint64_t h ) const;
 void clear( void );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 double load( void ) const;
 std::size_t bits( void ) const;
 };
```

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
$r = x \bmod 2^8$. A bin has a header of 50 bits, 6 more bits and a body
of 25 bytes. Its header writes in unary the number $c_q$ of the remainders
of each quotient, $1^{c_0} 0 \, 1^{c_1} 0 \cdots 1^{c_{24}} 0$ from the low
bit. The 6 bits hold the quotient of the largest fingerprint of the bin
once it has overflowed, and the body holds the remainders in the order of their
quotients, those of one quotient (its *run*) in increasing order
([Figure 10.9](10-set.md#fig-10-9)).

![Figure 10.9. A bin of a `PrefixFilter`, with 4 quotients instead of 25 for brevity: the header writes the counts 2, 0, 3, 1 of the runs in unary from the low bit, and the body holds the remainders; the run of the quotient 2 (in colour) is the slots $[ z_1 + 1 - 2 , z_2 - 2 )$, $z_q$ the place of the zero of rank $q$ of the header.](../figures/set-prefix.svg){#fig-10-9}

The run of $q$ is the slots $[ z_{q-1} + 1 - q , z_q - q )$ of the body,
where $z_q$ is the place of the zero of rank $q$ of the header (and
$z_{-1} = -1$). With BMI2, one `pdep` of $3 \cdot 2^q$ into the complement
of the header, shifted by one place, gives the two zeros at once (cf. the
`select()` of [§18.3](18-combinatorial.md#sec-18-3)); on any machine, the
lowest bit is cleared $q$ times. A query compares the 32 bytes of the bin
with $r$ at once (by the vectors of Highway, a byte at a time without it)
and stops if
no byte of the body is equal, as happens for about 91 per cent of the keys
out of $S$; otherwise it checks whether one of the equal bytes is in the run
of $q$.

--8<-- "generated/PrefixFilter-maybeHash.pseudo.md"

Bins keep the *prefix* of their fingerprints. Each bin holds the
smallest, in the order of $x = 2^8 q + r$, of the fingerprints that came to
it, at most 25. An insertion into a full bin with $x$ not below the
maximum $x_{\max}$ of the bin sends $( b , x )$ to the spare, and one with
$x < x_{\max}$ evicts $x_{\max}$ to the spare and takes its place; either
way the bin is marked as overflowed. Each fingerprint in the spare is then
at least the maximum of its bin, and a query looks in the spare only if its
bin has overflowed and $x > x_{\max}$. Hence a query of a key of $S$ is
always answered yes ([Lemma 10.15](10-set.md#lem-10-15)), and a query
typically reads one line of cache, against the two of the cuckoo filter.

<span id="lem-10-15"></span>**Lemma 10.15.** *At any time, each
fingerprint of a bin $b$ sent to the spare is at least the largest
fingerprint $x_{\max}$ that $b$ holds; hence the prefix filter has no
false negative.*

*Proof.* A bin sends a fingerprint to the spare only when it is full,
either the new $x \ge x_{\max}$ or its old maximum $x_{\max} > x$, which it
replaces by $x$; in both cases the fingerprint sent is at least the new
maximum of the bin. Later insertions into $b$ never raise its maximum
(a full bin only replaces its maximum by a smaller fingerprint), and the
claim holds by induction. Let now $x$ be the fingerprint of a key of $S$
in the bin $b$. If $b$ holds $x$, the query finds it in its run. Otherwise
$x$ went to the spare, $b$ has overflowed, and $x \ge x_{\max}$ by the
claim, with $x \ne x_{\max}$ since $b$ does not hold $x$; hence the query
queries the spare, a blocked filter of Bloom, which has no false negative.
$\square$

<span id="lem-10-16"></span>**Lemma 10.16.** *Let $n$ keys fall into
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
false positive if its fingerprint is among those of its bin, at most
$\alpha k$ on the average and each equal with probability $1 / s$. It is
also one if the spare answers yes to it, which is queried only when the bin
has overflowed, and the union bound gives the second claim (Theorem 2 of
the same paper).
$\square$

Here we take $\alpha = 0.95$, where $\mathrm{E}[ X ]$ is 5.9 per
cent of the keys, and build the spare for $1.1 \, \mathrm{E}[ X ]$
fingerprints at 12 bits each, as in the paper; when the spare holds
$\mathrm{E}[ X ]$ fingerprints, i.e., 13.2 bits each, (10.6) gives
$\varepsilon' \approx 0.34$ per cent. A blocked filter never fails: a spare that gets more
fingerprints only answers yes more often, and each insertion succeeds until
$n$ keys are in. When the filter is full, the false positive rate is 0.39
per cent (0.37 from the fingerprints and 0.02 from the spare), about the
$1 / 256$ of a filter of fingerprints of 8 bits. The bins take
$256 / ( \alpha k ) = 10.78$ bits per key and the spare about 0.77, i.e.,
11.55 in all. These quantities are reported by the class: its `capacity()` is $n$,
`bins()` is $m$, `load()` the fraction of the $25 m$ places taken,
`spareSize()` the fingerprints sent to the spare, `expectedSpareSize()` the
$\mathrm{E}[ X ]$ of (10.9), `bits()` the space and `falsePositiveRate()`
the expected rate at the current load, the sum over the loads of a bin
that [Lemma 10.16](10-set.md#lem-10-16) bounds.

Its declarations are the following.

```cpp
// declarations of <ADS/set/PrefixFilter.h>
template< class T , class H = DefaultHash< T > >
class PrefixFilter
{
 public:
 explicit PrefixFilter( std::size_t n , H h = H() );
 bool insert( const T & x );
 bool maybe( const T & x ) const;
 bool insertHash( std::uint64_t h );
 bool maybeHash( std::uint64_t h ) const;
 void clear( void );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 double load( void ) const;
 std::size_t bits( void ) const;
 std::size_t bins( void ) const;
 std::size_t spareSize( void ) const;
 double expectedSpareSize( void ) const;
 double falsePositiveRate( void ) const;
 };
```

## 10.10 Static filters {#sec-10-10}

A static filter is built once from $S$, and we can therefore make it solve a linear
system over $\mathbb{F}_2$ whose unknowns are its cells: each key gives an
equation, and a key out of $S$ meets its equation with the probability that
a fixed value equals its fingerprint.

<span id="lem-10-17"></span>**Lemma 10.17.** *Let a filter answer
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
([Figure 10.10](10-set.md#fig-10-10)). This *spatial coupling* lets the
array have only about $1.125 n$ places for large $n$, against the
$1.23 n$ of 3 places anywhere ([Dietzfelbinger and Walzer
2019a](Z-bibliography.md#ref-dietzfelbinger2019a)); we take
$L = 2^{\lfloor \log_{3.33} n + 2.25 \rfloor}$ (at most $2^{18}$) and
about $\max( 1.125 , 0.875 + 0.25 \ln 10^6 / \ln n ) \, n$ places, the
choices of the paper.

![Figure 10.10. The array of a `BinaryFuseFilter`, in segments of $L = 4$ places, and the 3 places of a key (in colour) in 3 consecutive segments.](../figures/set-fuse.svg){#fig-10-10}

The build solves the system by *peeling* ([Algorithm 10.8](10-set.md#alg-10-8)):
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

<span id="alg-10-8"></span>**Algorithm 10.8.** The build of the binary
fuse filter by peeling ([Graf and Lemire
2022](Z-bibliography.md#ref-graf2022)).

--8<-- "generated/BinaryFuseFilter-build.pseudo.md"

<span id="lem-10-18"></span>**Lemma 10.18.** *If the peeling puts all
the keys on the stack $P$, the fingerprints assigned in the reverse order
satisfy $F[ h_0 ] \oplus F[ h_1 ] \oplus F[ h_2 ] = f( h )$ for each key,
and the filter has no false negative.*

*Proof.* The 3 places of a key are in 3 distinct segments, and hence
distinct. A place $p$ with $c[ p ] = 1$ holds in $x[ p ]$ the hash of its
only key and in $s[ p ]$ the index $i$ by which it reaches $p$, since the
exclusive or of a single term is that term. Let the keys be peeled in the
order $k_1 , \ldots , k_n$, by the places $p_1 , \ldots , p_n$. When $k_t$
is peeled, $p_t$ belongs to no other key still unpeeled, i.e., to none of
$k_{t+1} , \ldots , k_n$. Then the assignment pops $k_n$ first and sets
$F[ p_t ]$ so that the equation of $k_t$ holds; the later assignments
write only $p_{t-1} , \ldots , p_1$, and $p_s$, $s < t$, is not a place of
$k_t$, which was unpeeled when $k_s$ was peeled. Hence the equation of $k_t$
still holds at the end, and [Lemma 10.17](10-set.md#lem-10-17) gives no
false negative. $\square$

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
([Figure 10.11](10-set.md#fig-10-11)). A new equation goes to the row of
its start if it is free; otherwise it is reduced by the equation of that
row and shifted to the first column of the result that is 1, until it
finds a free row. An equation that becomes 0 is redundant if its right side
is 0 too (a repeated key), and makes the system unsolvable otherwise, in
which case the build draws a new seed.

![Figure 10.11. The system of a `RibbonFilter` in echelon form, with windows of 4 coefficients instead of 64: each row holds an equation, whose window of 4 coefficients is written from the column of the row, its first coefficient, always 1, in violet, or none (faintest tint), and $f$ is the right side.](../figures/set-ribbon.svg){#fig-10-11}

--8<-- "generated/RibbonFilter-build.pseudo.md"

<span id="lem-10-19"></span>**Lemma 10.19.** *If no equation is
unsolvable, the back substitution of lines 11 and 12 gives a solution $Z$
of (10.10) for each key, and each insertion of an equation ends after at
most 64 reductions.*

*Proof.* Each row $s$ holding an equation has its first coefficient in the
column $s$, i.e., $C[ s ]$ has the bit 0 set. A reduction replaces an
equation by its exclusive or with the equation of the row of its start,
which clears its bit 0, and the shift then moves its start to its first
coefficient left, strictly to the right; since the coefficients span 64
columns, at most 64 reductions empty them or reach a free row. The
equations stored and those inserted thus differ by exclusive ors of
equations, and each inserted equation is the exclusive or of stored ones
(or is redundant, with $c = f = 0$). Finally, the back substitution solves the rows
from the last: the row $i$ with an equation sets $Z_i$ from the
$Z_{i+j}$, $j \ge 1$, already fixed, and hence satisfies its equation, and a
free row takes any value. Every stored equation holds, and therefore every
inserted one. $\square$

Then the rows are solved from the last, keeping for each of the $r$
columns the word of the 64 rows below, so that a row costs $r$ parities of
a word; with AVX-512 the back substitution takes 4 rows at a time (the
last kernel of the filters on the intrinsics of the target, until it is
rewritten on Highway), and a query takes the 8 parities of a line in the
vectors of Highway.
We store the solution interleaved, $r$ words per block of 64 rows. A
query reads the consecutive words of the two blocks of its 64 rows and
makes each bit of its result by a parity. Also, it compares the 8 bits of
each line of cache with those of $f( h )$ before reading the next line,
and a key out of $S$ reads 2 lines in most cases.

--8<-- "generated/RibbonFilter-maybe.pseudo.md"

A system of $n = ( 1 - \delta ) m$ equations with random windows of
length $O( \log n / \delta )$ has full rank with high probability
([Dietzfelbinger and Walzer 2019b](Z-bibliography.md#ref-dietzfelbinger2019b)).
With windows of 64 fixed, one can therefore expect the overhead needed to grow with $\log n$. Thus,
the build takes $m = n ( 1 + \delta )$ rows with $\delta = \max( 0.01 ,
0.0083 \log_2 n - 0.025 )$, i.e., about 11 per cent at $10^5$ keys and 14
per cent at $10^6$, or 8.9 and 9.1 bits per key for $r = 8$. The number of rows is rounded up to a multiple of 64 (and is at least
64), and after each 4 failed seeds the build adds $2$ per cent of $n$ rows,
at least 64. It gives up with an exception after 1000 seeds, and
`trials()` gives the seeds drawn, here and in `BinaryFuseFilter`.

Both static filters are declared as follows, `F` giving $b$ (or $r$) bits and `rows()` the $m$ rows; the example also builds the range filter of [§10.11](10-set.md#sec-10-11).

```cpp
// declarations of <ADS/set/BinaryFuseFilter.h>
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
 requires( std::is_unsigned_v< F > && ( sizeof( F ) <= 4 ) )
class BinaryFuseFilter
{
 public:
 explicit BinaryFuseFilter( const std::vector< T > & keys ,
                            std::uint64_t seed = 1 , H h = H() );
 bool maybe( const T & x ) const;
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t bits( void ) const;
 std::size_t trials( void ) const;
 };
```

```cpp
// declarations of <ADS/set/RibbonFilter.h>
template< class T , class F = std::uint8_t , class H = DefaultHash< T > >
class RibbonFilter
{
 public:
 explicit RibbonFilter( const std::vector< T > & keys ,
                        std::uint64_t seed = 1 ,
                        H h = H() );
 bool maybe( const T & x ) const;
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t bits( void ) const;
 std::size_t rows( void ) const;
 std::size_t trials( void ) const;
 };
```

```cpp
const std::vector< std::uint64_t > keys = { 2 , 3 , 5 , 7 , 11 , 13 };
const ADS::BinaryFuseFilter< std::uint64_t > F( keys );
const bool yes = F.maybe( 7 );  // true, always
const ADS::GrafiteFilter<> G( keys , 16 );
const bool empty = ! G.mayContain( 20 , 30 );  // true, but for a
                                              // false positive
```

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
codes in all ([Figure 10.12](10-set.md#fig-10-12)), and a successor query
of Elias-Fano answers each in $O( 1 )$.

![Figure 10.12. A range $[ a , b ]$ that meets two blocks of $r$ keys, each rotated into $[ 0 , r )$ by the hash $q$ of its block, and its image, two cyclic ranges of the same total length.](../figures/set-grafite.svg){#fig-10-12}

--8<-- "generated/GrafiteFilter-build.pseudo.md"

--8<-- "generated/GrafiteFilter-mayContain.pseudo.md"

A query (the second procedure above, where $m$ is the number of the codes
and $\mathrm{nextGEQ}( x )$ the place of the first code not smaller than
$x$) maps the two ends of the range and checks whether a code lies in each
cyclic range of the image.

<span id="lem-10-20"></span>**Lemma 10.20.** *For every set $S$ and every
range of $\ell$ keys,*

$$
\varepsilon( \ell ) \le \min\Bigl( 1 , \frac{n \ell}{r} \Bigr) =
\min\Bigl( 1 , \frac{\ell}{2^{B-2}} \Bigr) . \tag{10.12}
$$

*Proof.* A key of $S$ in the range has its code in the image of the
range, and there is no false negative. Let the range be empty, and split it
into its at most two parts $P_1$ and $P_2$, each inside one block, of
$\ell_1 + \ell_2 = \ell$ keys; the image of $P_i$ is a cyclic range of
$\ell_i$ codes, determined by $q$ of the block $Y_i$ of $P_i$. Let $x$ be a
key of $S$ in the block $X$. If $X = Y_i$, the code of $x$ is out of the
image of $P_i$, since (10.11) is one to one on a block and $x \notin P_i$.
If $X \ne Y_i$, the values $q( X )$ and $q( Y_i )$ are independent and
uniform on $[ 0 , r )$, since $q$ is pairwise independent; hence the code
$( q( X ) + x ) \bmod r$ is uniform and independent of the image of $P_i$,
and it falls in it with probability $\ell_i / r$. Summing over the two
parts and over the $n$ keys (the union bound) gives at most
$n \ell / r$, and a probability is at most 1, which is (10.12) ([Costa et
al. 2024](Z-bibliography.md#ref-costa2024), Theorem 3.4). $\square$

This lemma generalizes the construction of [Goswami et al.
2015](Z-bibliography.md#ref-goswami2015) to the ranges of any length, and
the lower bound of [Costa et al. 2024](Z-bibliography.md#ref-costa2024),
$n \log_2( \ell / \varepsilon ) - O( n )$ bits, is within 2 bits per key of
Grafite. In the header, $q$ is the multiply-add-shift of
[Dietzfelbinger 1996](Z-bibliography.md#ref-dietzfelbinger1996) on 128 bits,
pairwise independent, mapped onto $[ 0 , r )$ by a product, whose values
have probabilities within $1 / r \pm 2^{-64}$; this adds at most
$n \ell 2^{-64}$ to (10.12). Instead, the paper uses the hash of [Wegman and
Carter 1981](Z-bibliography.md#ref-wegman1981) modulo a prime, which would
need a prime above the $2^{64} / r$ blocks. Keys may be of any type that
`radixKey()` maps in order onto unsigned words, floating point numbers
included.

Besides `mayContain( a , b )` and `maybe( x )`, i.e., `mayContain( x , x )`,
`GrafiteFilter` gives `falsePositiveBound( l )`, the bound (10.12) for a range
of $\ell$ keys, `reduced()`, i.e., $r$, `codes()`, the distinct codes, and
`sizeInBits()`, the bits of the codes and of the directory of `select()` of
Elias-Fano.

`BucketingFilter`, the heuristic of the same paper, cuts the universe into
buckets of $s = \lceil 2^{64} / ( n \, 2^{B-2} ) \rceil$ keys and keeps by
Elias-Fano the buckets that hold a key of $S$; a range may hold a key if a
kept bucket meets it. It needs no hash and is faster to build and to
query. On keys and queries drawn independently at random, a range of
$\ell$ keys meets at most $\ell / s + 2$ buckets, each kept with
probability at most about $n s / 2^{64} \approx 2^{-(B-2)}$. Hence it is
a false positive with probability at most about
$( \ell / s + 2 ) \, 2^{-(B-2)}$ by the union bound;
however, an empty range inside the bucket of a key, at distance below $s$
from it, is always a false positive, and no bound like (10.12) holds for
every query (`bucketWidth()` gives $s$, `buckets()` the kept buckets and
`sizeInBits()` the space).

--8<-- "generated/GrafiteFilter-BucketingFilter-mayContain.pseudo.md"

Both range filters are declared as follows ($B = 16$ bits per key by default).

```cpp
// declarations of <ADS/set/GrafiteFilter.h>
template< RadixKey T = std::uint64_t >
class GrafiteFilter
{
 public:
 explicit GrafiteFilter( const std::vector< T > & keys , int B = 16 ,
                         std::uint64_t seed = 1 );
 bool mayContain( const T & a , const T & b ) const;
 bool maybe( const T & x ) const;
 double falsePositiveBound( double l ) const;
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t codes( void ) const;
 std::uint64_t reduced( void ) const;
 std::size_t sizeInBits( void ) const;
 };
template< RadixKey T = std::uint64_t >
class BucketingFilter
{
 public:
 explicit BucketingFilter( const std::vector< T > & keys , int B = 16 );
 bool mayContain( const T & a , const T & b ) const;
 bool maybe( const T & x ) const;
 std::uint64_t bucketWidth( void ) const;
 std::size_t buckets( void ) const;
 std::size_t sizeInBits( void ) const;
 };
```

After Grafite, the dynamic range filters Memento ([Eslami and Dayan
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
fingerprints. A comparison of the realizations, of the intersections and
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
9. Make each dictionary of the library a USet, and each ordered one an
   SSet, of Morin through one adaptor, and check it against `std::set`.
   Test: `ods_trees.ods1.2.dictionary-sets`.
10. Realize the sorted set of Morin on a list of blocks of $b \pm 1$ items
   indexed by a skip list, in the space of $n + O( n / b + b )$ items
   ([Morin 2013, exercise 4.13](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_trees.ods-ex4.13.sesset`.
11. Realize the compressed bitmaps of Roaring, with the conversions between
   arrays and bitmaps at 4096 items, `rank()` and `select()`. Test:
   `ordered.roaring-bitmap`.
12. Intersect two sorted lists by the merge, the binary searches, the
    mutual partition, the doubling and the two-level storage, and check the
    numbers of comparisons against the bounds. Tests:
    `pearls_io.ch6.intersection-example`,
    `pearls_io.ch6.intersection-random`.
13. Intersect two lists by the permuted buckets of Sanders and Transier,
    and on the disk by the two-level storage. Tests:
    `pearls_io.ch6.permuted-buckets`, `pearls_io.ch6.intersection-ios`.
14. Intersect two sorted lists with no branch, by blocks of 8 items and
    through a window of 64 items. Test: `ordered.vector-intersection`.
15. Build the filter of Bloom and check its false positives against
    (10.5), and its places $h_1 + i h_2 \bmod m$. Tests:
    `pearls_hashing.bloom-analysis`, `taocp.bloom-filter`,
    `taocp.bloom-probes`.
16. Estimate the multiplicities of a multiset by a spectral filter, and
    intersect two sets on two machines by sending a filter. Tests:
    `pearls_hashing.spectral-bloom`, `pearls_hashing.bloom-intersection`.
17. Build the blocked filter of Bloom and compare its false positives with
    (10.6). Test: `hashing.blocked-bloom`.
18. Build the cuckoo filter, with insertions by evictions and removals, and
    measure its largest load. Test: `hashing.cuckoo-filter`.
19. Build the prefix filter, with its pocket dictionaries and its spare.
    Test: `hashing.prefix-filter`.
20. Build the binary fuse filter by peeling and the Ribbon filter by
    banding, and check that their false positives are about $2^{-b}$.
    Tests: `hashing.binary-fuse`, `hashing.ribbon-filter`.
21. Build the range filter Grafite, and check its bound on correlated
    queries, where the bucketing filter fails. Tests:
    `hashing.grafite-filter`, `hashing.bucketing-filter`.

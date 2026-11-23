# 2. Sequences {#ch-2}

[Source: `include/ADS/sequence/ArraySequence.h`, `SmallVector.h`,
`DualArrayDeque.h`, `Treque.h`, `ContiguousDeque.h`,
`RootishArrayStack.h`, `TieredVector.h`, `CubishArrayStack.h`,
`SinglyList.h`, `List.h`, `ListSort.h`, `CursorList.h`, `SEList.h`,
`XorList.h`, `SkipListSequence.h`, `TreapList.h`,
`OrderMaintenanceList.h`, `IndexedList.h`, `SequenceRange.h`,
`SequenceAlgorithms.h`, `Rearrangements.h`; `ADS/queue/RingDeque.h` for
its operations by rank]

We are concerned in this chapter with the *sequence*, i.e., a finite list
$s_0 , s_1 , \ldots , s_{n-1}$ of $n \ge 0$ items, possibly repeated, in
which the order matters. A program may reach its items in two ways: through
*positions*, which the realization chooses and which lead from an item to
the next one, or through *ranks*, the indices $0 , \ldots , n - 1$. All the
structures of the following chapters are built on one of two representations
of a sequence: the vector, whose items are contiguous and are reached by
index in constant time, and the linked list, whose items are in cells that
point to each other. In `ADS/sequence` the headers realize both in several
forms, which differ in the cost of an insertion in the middle, in the space
wasted and in the operations that come for free. In the sets of
[§1.9](01-introduction.md#sec-1-9), `ListSort.h`, `SmallVector.h` and
`TieredVector.h` are advanced and the other headers of this chapter are
base.

After the interfaces ([§2.1](02-sequence.md#sec-2-1)), we describe the
vector that doubles and halves ([§2.2](02-sequence.md#sec-2-2)) and the
circular vector read by rank ([§2.3](02-sequence.md#sec-2-3)). The deques
made of two stacks or of two circular vectors, and the deque in one
contiguous piece, follow in [§2.4](02-sequence.md#sec-2-4) and
[§2.5](02-sequence.md#sec-2-5); the arrays that waste only $O( \sqrt{n} )$
places, i.e., the rootish, the tiered and the cubish ones, are in
[§2.6](02-sequence.md#sec-2-6) to [§2.8](02-sequence.md#sec-2-8). Then come
the lists: the singly and the doubly linked one
([§2.9](02-sequence.md#sec-2-9) and [§2.10](02-sequence.md#sec-2-10)), the
lists with cursors ([§2.11](02-sequence.md#sec-2-11)), the space-efficient
list of blocks ([§2.12](02-sequence.md#sec-2-12)) and the XOR list
([§2.13](02-sequence.md#sec-2-13)). The sequences that reach a rank in
logarithmic time, on a skip list and on a treap, and the order maintenance
of a list are in [§2.14](02-sequence.md#sec-2-14) and
[§2.15](02-sequence.md#sec-2-15). Finally, the chapter ends with the
algorithms that work on any sequence through its positions, or on any LIST
through its ranks ([§2.16](02-sequence.md#sec-2-16)), and with the
rearrangements of a vector in place ([§2.17](02-sequence.md#sec-2-17)).
Together, these sections cover chapters 2 and 3 and section 4.3 of [Morin
2013](Z-bibliography.md#ref-morin-cpp), with the answers of their exercises,
and the sequences of chapter 4 of [Montresor
2025](Z-bibliography.md#ref-montresor2025).

## 2.1 Positions and ranks {#sec-2-1}

The SEQUENCE of [Montresor 2025, §3.2.1](Z-bibliography.md#ref-montresor2025)
reaches its items through values of a type `Pos` that the realization
chooses; [§1.3](01-introduction.md#sec-1-3) states its axioms, and the
concept `Sequence` of `ADS/ADT.h` its operations. The position
$\mathit{pos}_i$ leads to $s_i$ for $0 \le i < n$, and two more positions,
$\mathit{pos}_{-1}$ and $\mathit{pos}_n$, stand before the first item and
after the last one, so that a scan reads

$$
p \gets \mathrm{head}();\ \textbf{while not}\ \mathrm{finished}( p )
\ \textbf{do}\ \ldots\ \mathrm{read}( p ) \ldots ;\ p \gets
\mathrm{next}( p ) . \tag{2.1}
$$

What a position is decides the cost of each operation. An index costs
nothing to move along, but it makes an insertion shift the items after it; a
pointer to a cell, instead, makes an insertion change two links, but it
gives no access by index. In C++, `range()` of
`ADS/sequence/SequenceRange.h` turns (2.1) into the `for` on a range. Its
iterator holds a position, and its end is an iterator marked as such, which
compares equal to any iterator on a finished position. Hence no
$\mathit{pos}_n$ needs to be computed (on a singly linked list `tail()`
costs $O( n )$), while the range stays common, as the algorithms of C++17
and of Boost want it. This range is a view, i.e., a pointer to the sequence,
which must not change during the visit, and a step costs what `next()`
costs; as an adapter with no state, it has no figure.

Those realizations with positions name the type of their items `Item` and
that of their positions `Pos`, and the linked ones also have a nested
`Iterator` for the algorithms of the standard library, as do `TieredVector`
and `SEList`. Furthermore, each container of the area that can be
traversed has the aliases of a container of the standard library
(`value_type`, `size_type`, `difference_type`, `reference`,
`const_reference`, `iterator` and `const_iterator`, and also
`reverse_iterator` and `const_reverse_iterator` for `ArraySequence`,
`SmallVector`, `ContiguousDeque` and `List`, whose iterators go backwards),
with `begin()` and `end()`. `ArraySequence`, `SmallVector`, `List`,
`SinglyList`, `CursorList`, `TieredVector`, `SkipListSequence` and
`TreapList` also have `cbegin()` and `cend()`, and the four classes with
reverse iterators have `rbegin()` and `rend()`; hence the algorithms and the
ranges of the standard library run on them, and we give the kind of each
iterator below with its class.

The LIST of [Morin 2013, §1.2.2](Z-bibliography.md#ref-morin-cpp) reaches the
items by their rank instead, by `get( i )`, `set( i , x )`, `add( i , x )` and
`remove( i )`, and the DEQUE of his §1.2.1 changes a sequence at its two ends.
In turn, the concepts `IndexedList` and `IndexedDeque` of
`ADS/sequence/IndexedList.h` state them with the names of the standard library
(`operator[]`, `insert( i , x )`, `erase( i )`, `push_front()` and so on), as
[§1.3](01-introduction.md#sec-1-3) explains. Throughout the chapter $n$ is the
number of items, $c$ the capacity of a vector (the items it has room for),
$\alpha = n / c$ its load, and $\mu = \min\{ i , n - i \}$ the distance of the
rank $i$ from the nearer end. When the items are in blocks, $b$ is the size of
a block. Also, $A[ 0 .. n - 1 ]$ is a vector of the code, indexed from 0, with
$A[ i .. j )$ its places from $i$ to $j - 1$, and $n / 2$ in a test of the
code is the integer $\lfloor n / 2 \rfloor$. [Table
2.1](02-sequence.md#tab-2-1) gives the realizations with positions, and [Table
2.2](02-sequence.md#tab-2-2) those by rank.

<span id="tab-2-1"></span>**Table 2.1.** The realizations of the
SEQUENCE with positions, and the vectors by index of
[Montresor 2025, §4.1 and §4.2](Z-bibliography.md#ref-montresor2025); the
costs at the end are amortized.

| Realization | Position | Space | `next()`, `read()` | `prev()`, `tail()` | `insert()`, `remove()` | by index |
|----------------|----------------|------------|------------|------------|--------------------|------------|
| `ArraySequence` | the index | $\Theta( n )$ | $O( 1 )$ | $O( 1 )$ | $O( n - i )$, $O( 1 )$ at the end | $O( 1 )$ |
| `List` | cell of the item | $\Theta( n )$ | $O( 1 )$ | $O( 1 )$ | $O( 1 )$ | $O( 1 + \mu )$ |
| `SinglyList` | cell before the item | $\Theta( n )$ | $O( 1 )$ | $O( n )$ | $O( 1 )$ | $O( 1 + i )$ |
| `CursorList` | cursor of the cell before | $\Theta( n )$ shared | $O( 1 )$ | $O( n )$ | $O( 1 )$ | $O( 1 + i )$ |
| `SmallVector` | none | $O( n + N )$ | none | none | $O( 1 )$ at the end | $O( 1 )$ |

<span id="tab-2-2"></span>**Table 2.2.** The realizations of the LIST by
rank and the cost of their operations, amortized where the realization
resizes or rebuilds, and expected for the last two; the column of Morin
gives his class, or his exercise in the C++ edition.

| Realization | Morin | `operator[]` | `insert()`, `erase()` | space beyond $n$ items |
|---|---|---|---|---|
| `ArraySequence` | `ArrayStack`, `FastArrayStack` (§2.1, §2.2) | $O( 1 )$ | $O( 1 + n - i )$ | $\le 3 n$ places |
| `RingDeque` | `ArrayQueue`, `ArrayDeque` (§2.3, §2.4) | $O( 1 )$ | $O( 1 + \mu )$ | $< 2 \hat{n} + 8$ places in all, $\hat{n}$ the largest $n$ |
| `DualArrayDeque` | `DualArrayDeque` (§2.5) | $O( 1 )$ | $O( 1 + \mu )$ | those of its two stacks |
| `Treque` | exercise 2.3 | $O( 1 )$ | $O( 1 + \min\{ \mu , \lvert n / 2 - i \rvert \} )$ | those of two `RingDeque` |
| `ContiguousDeque` | exercise 2.8 | $O( 1 )$ | $O( 1 + \mu )$ | $\le 8 n + 24$ places |
| `RootishArrayStack` | `RootishArrayStack` (§2.6) | $O( 1 )$ | $O( 1 + n - i )$ | $O( \sqrt{n} )$ |
| `DualArrayDeque` on `RootishArrayStack` | exercise 2.9 | $O( 1 )$ | $O( 1 + \mu )$ | $O( \sqrt{n} )$ |
| `TieredVector` | exercises 2.10, 2.11 | $O( 1 )$ | $O( 1 + \min\{ \mu , \sqrt{n} \} )$ | $O( \sqrt{n} )$ |
| `CubishArrayStack` | exercise 2.12 | $O( 1 )$ | $O( n^{1/3} )$ | $O( n^{2/3} )$ |
| `SinglyList` | `SLList` (§3.1) | $O( 1 + i )$ | $O( 1 + i )$ | $n$ pointers |
| `List` | `DLList` (§3.2) | $O( 1 + \mu )$ | $O( 1 + \mu )$ | $2 n$ pointers |
| `SEList` | `SEList` (§3.3), exercises 3.17, 3.18 | $O( 1 + \mu / b )$ | $O( b + \mu / b )$ | $O( b + n / b )$ |
| `XorList` | §3.4 | none | $O( 1 )$ at the ends | $n$ words |
| `SkipListSequence` | `SkiplistList` (§4.3) | $O( \log n )$ | $O( \log n )$ | $5 n$ words, expected |
| `TreapList` | exercise 7.11 | $O( \log n )$ | $O( \log n )$ | $4 n$ words |

We cite the exercises of chapter 2 of Morin with the numbers of the C++
edition of his book, which the library and the names of its tests follow.
Note that the Java edition has one more exercise at the start of the chapter
(on the item left behind by a removal, see [§2.2](02-sequence.md#sec-2-2)),
and numbers the following ones one higher.

## 2.2 The vector that doubles {#sec-2-2}

`ArraySequence` keeps the $n$ items in the places $0 , \ldots , n - 1$ of a
vector $A$ of $c$ places, and its position is the index, $\mathit{pos}_i =
i$, with $\mathit{pos}_{-1} = -1$ and $\mathit{pos}_n = n$. It is the vector
of [Montresor 2025, §4.2](Z-bibliography.md#ref-montresor2025) and the
`ArrayStack` of [Morin 2013, §2.1](Z-bibliography.md#ref-morin-cpp), whose
`get( i )`, `set( i , x )`, `add( i , x )` and `remove( i )` are `read()`,
`write()`, `insert()` and `remove()` here. Moving along the sequence,
reading and writing take $O( 1 )$ time, while an insertion at the index
$p$ moves the $n - p$ items after it one place on, and a removal moves them
back ([Algorithm 2.1](02-sequence.md#alg-2-1)). When an insertion finds the
vector full, the vector is replaced by one of twice its size, into which
the items are moved. When a removal leaves it with $n \le c / 4$ items, it
is replaced by one of half its size; the empty sequence has no vector at
all (the first insertion makes one of one place). [Figure
2.1](02-sequence.md#fig-2-1) follows the operations of the figure of [Morin
2013, §2.1](Z-bibliography.md#ref-morin-cpp) on an `ArraySequence`.

![Figure 2.1. The operations of the first figure of Morin's chapter 2 on an `ArraySequence`: the vector through `add(2,e)`, `add(5,r)`, `add(5,e)`, three `remove(4)` and `set(2,i)`, with $n$ and $c$ at the right of each row; the star marks the doubling from $c = 4$ to $c = 8$ (no halving follows, since $n > c / 4$), each item written is in orange, the items that each operation moves are in violet, and the free places are in the faintest tint.](../figures/sequence-arraystack.svg){#fig-2-1}

<span id="alg-2-1"></span>**Algorithm 2.1.** The insertion and the removal
of `ArraySequence` ([Montresor 2025, §4.2 and
§4.2.1](Z-bibliography.md#ref-montresor2025)); $A[ 0 .. c - 1 ]$ is the
vector.

--8<-- "generated/ArraySequence-insert.pseudo.md"

--8<-- "generated/ArraySequence-remove.pseudo.md"

A single insertion thus costs $\Theta( n )$ in the worst case,
when it doubles the vector, but that of a sequence of insertions at the
end is linear in their number.

<span id="lem-2-1"></span>**Lemma 2.1.** *Starting from the empty
sequence, $n$ insertions at the end of an `ArraySequence` cost in all*

$$
C( n ) = \sum_{i=1}^n c_i \le n + \sum_{k=0}^{\lfloor \log_2 n \rfloor}
2^k \le n + 2^{\lfloor \log_2 n \rfloor + 1} - 1 \le 3 n , \tag{2.2}
$$

*where $c_i = i$ when the $i$-th insertion finds the vector full, i.e.,
when $i - 1$ is a power of 2, and $c_i = 1$ otherwise; hence an insertion
at the end costs $O( 1 )$ amortized time.*

*Proof.* The capacities after the doublings are $1 , 2 , 4 , \ldots$, and
the $i$-th insertion doubles the vector exactly when the $i - 1$ items
already there fill it, i.e., when $i - 1 = 2^k$; it then moves $2^k$ items
and writes one. The doublings among $n$ insertions are those with $2^k \le
n - 1$, whose moves sum to less than $2^{\lfloor \log_2 n \rfloor + 1}
\le 2 n$, and each insertion writes one item more. $\square$

Both constants of the code, the factor 2 of the growth and the threshold $c
/ 4$ of the halving, are needed for a constant amortized cost, as the next
lemma shows on the two obvious alternatives (a vector that only grows,
instead, keeps the space of its largest size).

<span id="lem-2-2"></span>**Lemma 2.2.** *(i) A vector of $d \ge 1$
places that grows by $d$ places when it is full moves $d q ( q + 1 ) / 2$
items in $n$ insertions at its end, $q = \lceil n / d \rceil - 1$, i.e.,
$\Theta( n^2 / d )$ items for $n \ge 2 d$. (ii) A vector that doubles when
it is full and halves when a removal leaves it with $n \le c / 2$ items
moves $c$ items at each operation of the sequence that alternates an
insertion and a removal, starting from a full vector of $c \ge 1$ places.*

*Proof.* (i) The vector is full, and grows, at the insertions of rank $j d +
1$ for $j = 1 , \ldots , q$, the $j$-th growth moving the $j d$ items already
there; these moves sum to $d q ( q + 1 ) / 2$, and $n / ( 2 d ) \le n / d - 1
\le q \le n / d$ for $n \ge 2 d$. (ii) The insertion into the full vector of
$c$ places doubles it, moving $c$ items, and leaves $c + 1$ items in $2 c$
places; the removal that follows leaves $c = 2 c / 2$ items, and hence it
halves the vector back to $c$ places, moving $c$ items, after which the
vector is full as at the start. $\square$

With the halving at a quarter, instead, $\Omega( c )$ operations separate
two resizes, and the potential of the following proof measures this
distance.

<span id="lem-2-3"></span>**Lemma 2.3.** *Any sequence of $m$ insertions
and removals at the end of an `ArraySequence`, starting from the empty
one, costs $O( m )$ time, i.e., $O( 1 )$ amortized time per operation.*

*Proof.* We take as potential

$$
\Phi = \begin{cases} 2 n - c & \text{if } \alpha \ge 1 / 2 , \\
c / 2 - n & \text{if } \alpha < 1 / 2 , \end{cases} \tag{2.3}
$$

which is never negative and is 0 when the vector is half full. An
operation that does not resize changes $n$ by 1 and $\Phi$ by at most 2,
and hence its amortized cost, i.e., its cost plus the change of $\Phi$, is
at most 3. A doubling happens at $n = c$, where $\Phi = n$, and a halving at
$n = c / 4$, where $\Phi = c / 4 = n$. In both cases the $n$ moves are paid
by the potential, which falls to $O( 1 )$ after the resize, the vector
being half full ([Cormen et al. 2009,
§17.4](Z-bibliography.md#ref-cormen2009), after [Tarjan
1985](Z-bibliography.md#ref-tarjan1985)). $\square$

<span id="thm-2-1"></span>**Theorem 2.1.** *`ArraySequence` realizes the
SEQUENCE and the LIST; `read()`, `write()`, `next()` and `prev()` take
$O( 1 )$ time, and `insert()` and `remove()` at the index $i$ take $O( 1
+ n - i )$ amortized time. The vector has at most $4 n$ places once a
removal has been made, and fewer than $2 n$ after insertions only.*

*Proof.* A shift moves $n - i$ items, and the resizes cost $O( 1 )$
amortized by [Lemma 2.3](02-sequence.md#lem-2-3), whose argument does not
depend on where the item is inserted or removed. Also, the halving keeps
$n > c / 4$ after each removal, and the doubling $n > c / 2$ after every
insertion that finds the vector full. $\square$

We remark that the halving of [Morin 2013,
§2.1.2](Z-bibliography.md#ref-morin-cpp) is at $3 n \le c$ and ours at
$4 n \le c$. Hence the argument is the same, with another constant, and the
band of sizes between a doubling and the next halving is wider with the
second. His
`FastArrayStack` (§2.2) shifts the items by `std::copy()` instead of one by
one; ours shifts them by `std::move_backward()` and `std::move()`, which
become one `memmove()` for the items trivially copyable, and it moves the
items in a resize instead of copying them. In C++ a removal leaves in the
place $n$ the item that the shift moved out of it, i.e., the husk of a move,
until an insertion or a resize overwrites it. This husk keeps its resources
only if its move does not take them (a `std::string` or a `std::vector`
usually gives them away). This is the answer to the first exercise of the
chapter in the Java edition of Morin's book, where the place keeps a
reference that the garbage collector cannot reclaim.

Inserting the $k$ items of a range at the index $p$ by $k$ calls of
`insert()` moves the $n - p$ items after $p$ $k$ times, in $\Theta( k ( n -
p ) )$ time. A single shift by $k$ places after a single resize, instead,
moves them once, in $O( 1 + n - p + k )$ amortized time ([Morin 2013,
exercise 2.1](Z-bibliography.md#ref-morin-cpp), and [Lemma
2.4](02-sequence.md#lem-2-4) below). This is the range `insert()` of
`ArraySequence` ([Algorithm 2.2](02-sequence.md#alg-2-2)), which first
copies the range, since it may hold items of the sequence itself.

<span id="alg-2-2"></span>**Algorithm 2.2.** The insertion of $k$ items
at once ([Morin 2013, exercise 2.1](Z-bibliography.md#ref-morin-cpp));
$C[ 0 .. k - 1 ]$ holds the items.

--8<-- "generated/ArraySequence-insertAll.pseudo.md"

<span id="lem-2-4"></span>**Lemma 2.4.** *The range `insert()` of $k$
items at the index $p$ of an `ArraySequence` takes $O( 1 + n - p + k )$
amortized time with the potential (2.3), and $O( n + k )$ time in the worst
case; hence [Lemma 2.3](02-sequence.md#lem-2-3) holds for any mix of single
and range insertions and of removals.*

*Proof.* The copy of the range costs $O( k )$, i.e., one vector of $k$
places for a range that can be measured and the doublings of [Lemma
2.1](02-sequence.md#lem-2-1) otherwise. Then the shift moves $n - p$ items
and the copy back writes $k$, while $\Phi$ grows by $2 k$ at most if there
is no resize. A resize happens when $n + k > c$, and it moves the $n$ items
into $c'$ places, $c'$ the least $2^j c$ with $c' \ge n + k$ ($c$ read as 1
for the empty vector). Since $c' / 2 < n + k$, the vector is then more than
half full, with $\Phi = 2 ( n + k ) - c' \le n + k$. If $k \le n$, we have
$c' = 2 c$, since $2 c \ge 2 n \ge n + k$, and the vector was at least half
full; hence $\Phi$ falls from $2 n - c > n - k$ to $2 ( n + k ) - 2 c \le
2 k$, and the $n$ moves cost at most $n + 2 k - ( n - k ) = 3 k$ amortized.
If $k > n$, the resize moves $n < k$ items and raises $\Phi$ by $n + k < 2
k$ at most, i.e., less than $3 k$ amortized again. $\square$

Furthermore, the class gives `data()`, `operator[]`, `size()`, `capacity()`
(the $c$ of the analysis), `push_back()` for `std::back_inserter`, `==`,
and the iterators of the standard library, which are pointers since the
items are contiguous. Note that a change made through an iterator bypasses
the contracts of `write()`, which check that the item is read back and that
the others are where they were (the latter an audit of cost $O( n )$).

In `ADS/sequence/ArraySequence.h`, the declarations of `ArraySequence` that
a user calls are

```cpp
// declarations of <ADS/sequence/ArraySequence.h>
template< class T >
class ArraySequence
{
 public:
 using Item = T;
 using Pos = long;
 ArraySequence( void );
 ArraySequence( std::initializer_list< T > items );
 template< std::input_iterator I , std::sentinel_for< I > S >
 ArraySequence( I first , S last );
 bool empty( void ) const;
 bool finished( Pos p ) const;
 Pos head( void ) const;
 Pos tail( void ) const;
 Pos next( Pos p ) const;
 Pos prev( Pos p ) const;
 Pos insert( Pos p , const T & v );
 template< std::input_iterator I , std::sentinel_for< I > S >
 Pos insert( Pos p , I first , S last );
 Pos remove( Pos p );
 const T & read( Pos p ) const;
 void write( Pos p , const T & v );
 Pos size( void ) const;
 Pos capacity( void ) const;
 bool operator==( const ArraySequence & o ) const;
 T * data( void );
 const T * data( void ) const;
 T & operator[]( Pos i );
 const T & operator[]( Pos i ) const;
 void push_back( const T & v );
 };
```

### 2.2.1 A vector with room inside itself {#sec-2-2-1}

Each of the many short vectors of a structure (the children of a node, the
arcs of a vertex, the items of a bucket) costs an allocation, and it usually
lies away from the object that holds it. `SmallVector< T , N >` is the vector
that doubles whose first $N$ items ($N = 8$ by default) lie in the object
itself, so that a vector of at most $N$ items needs no memory of the
allocator. Past $N$ items it moves its items to the allocator and doubles
there, as `std::vector` does ([Figure 2.2](02-sequence.md#fig-2-2) and
[Algorithm 2.3](02-sequence.md#alg-2-3)). Its invariant is $n \le c$ and
$c \ge N$, $c$ the capacity, and its costs follow from those of the vector
that doubles.

<span id="lem-2-5"></span>**Lemma 2.5.** *Starting from the empty
`SmallVector< T , N >`, $n$ calls of `push_back()` or `emplace_back()` take no
memory of the allocator if $n \le N$; otherwise they move fewer than $2 n$
items and leave a capacity $c < 2 n$, i.e., at most $2 n + N$ places with
those inside the object. Moving a `SmallVector` of $m$ items takes $O( 1 )$
time when its items are on the allocator, and moves its $m \le N$ items
otherwise.*

*Proof.* Starting at $N$, the capacity doubles only when an insertion
finds $n = c$, moving the $c$ items. After $J \ge 1$ doublings it is $N 2^J$,
and the last doubling found $N 2^{J-1} < n$ items; hence the moves are $N (
1 + 2 + \cdots + 2^{J-1} ) < N 2^J = c < 2 n$. A move exchanges the pointer,
the capacity and the size of the other vector when its items are on the
allocator, and moves its items into the places inside the object
otherwise, i.e., when $m \le N$. $\square$

The bound on the space holds for a vector that has grown by pushes alone:
`pop_back()` and `clear()` keep the capacity, as those of `std::vector` do,
and `reserve()` may set any larger one. Furthermore, the class has the
interface of a vector (`push_back()`, `emplace_back()`, `pop_back()`,
`front()`, `back()`, `operator[]`, `data()`, `reserve()`, `resize()`,
`clear()`, `capacity()` and the iterators), and no positions. With
`ADS_WITH_BOOST` defined (by the option of CMake of the same name, whenever
the headers of Boost 1.83 or later are found), the items are those of
`boost::container::small_vector` with the growth factor 2 of its options,
i.e., the same vector with the same doubling, so that the interface, the
iterators (the pointers to the items) and Lemma 2.5 still hold
(`ADS_SMALLVECTOR_BOOST` defined to 0 keeps the code of ADS).

![Figure 2.2. A `SmallVector` with $N = 4$: the four places inside the object hold `a b c d`, and `push_back(e)` moves them (in violet) to a block of $2 N = 8$ places of the allocator and writes `e` (in orange), the places inside being no longer used.](../figures/sequence-smallvector.svg){#fig-2-2}

<span id="alg-2-3"></span>**Algorithm 2.3.** The insertion at the end of
`SmallVector`.

--8<-- "generated/SmallVector-emplace_back.pseudo.md"

In `ADS/sequence/SmallVector.h`, the declarations of `SmallVector` that a
user calls are

```cpp
// declarations of <ADS/sequence/SmallVector.h>
template< class T , std::size_t N = 8 , class A = std::allocator< T > >
class SmallVector
{
 public:
 SmallVector( void ) noexcept;
 explicit SmallVector( const A & alloc ) noexcept;
 explicit SmallVector( std::size_t n , const T & x = T() );
 SmallVector( std::initializer_list< T > items );
 template< std::input_iterator I , std::sentinel_for< I > S >
 SmallVector( I first , S last );
 bool empty( void ) const;
 std::size_t size( void ) const;
 std::size_t capacity( void ) const;
 bool small( void ) const;
 T * data( void );
 const T * data( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 T & front( void );
 const T & front( void ) const;
 T & back( void );
 const T & back( void ) const;
 void push_back( const T & x );
 void push_back( T && x );
 template< class... Args >
 T & emplace_back( Args &&... args );
 void pop_back( void );
 void reserve( std::size_t n );
 void resize( std::size_t n , const T & x = T() );
 void clear( void );
 bool operator==( const SmallVector & o ) const;
 };
```

## 2.3 The circular vector by rank {#sec-2-3}

A vector whose items are removed at the front, as in a queue, either shifts
all the items at each removal or leaves the front places unused. The
circular vector of the `ArrayQueue` of [Morin 2013,
§2.3](Z-bibliography.md#ref-morin-cpp) keeps instead the items in the
consecutive places $h , h + 1 , \ldots , h + n - 1$ taken modulo the
capacity $m$; the item of rank $i$ is thus at the place $( h + i ) \bmod m$,
and both ends move in $O( 1 )$ time. `RingDeque` of `ADS/queue`, the deque
of chapter [4](04-queue.md#ch-4), is this vector with a capacity that is a
power of 2 (8 at least); hence, the place of the rank $i$ is $( h + i )
\mathbin{\&} ( m - 1 )$, a mask instead of a division ([Morin 2013, exercise
2.7](Z-bibliography.md#ref-morin-cpp)). When it is full, the vector doubles
and moves the items in rank order to the front of the new vector. It does
not shrink unless `shrink_to_fit()` is called, and therefore a queue whose
size oscillates never reallocates. [Figure 2.3](02-sequence.md#fig-2-3)
follows the operations of Morin's figure of the `ArrayQueue` on it: with the
8 places of the least capacity, his sequence needs no doubling, but its last
two insertions wrap around the end of the vector.

![Figure 2.3. The operations of Morin's figure of the `ArrayQueue` (§2.3) on a `RingDeque` of 8 places: the arrow marks the front place $h$ ($h$ and $n$ at the right), the item added is in orange and the free places, the one left by a removal included, are in the faintest tint; `add(g)` and `add(h)` wrap around to the places 0 and 1.](../figures/sequence-ringqueue.svg){#fig-2-3}

The `ArrayDeque` of [Morin 2013, §2.4](Z-bibliography.md#ref-morin-cpp) adds
the insertion and the removal at any rank, which `RingDeque` has too. Since
both ends move in $O( 1 )$ time, an insertion at the rank $i$ shifts the
items of the shorter side: the $i$ items before it one place back when $i <
\lfloor n / 2 \rfloor$, and the $n - i$ items from it one place on
otherwise. Also, the removal does the converse ([Algorithm
2.4](02-sequence.md#alg-2-4) and [Figure 2.4](02-sequence.md#fig-2-4)). Each
shift runs by `std::move()` on the stretches of the items that are
contiguous in the vector, three at most since the items lie in at most two
pieces. This means one `memmove()` each for the items trivially copyable
([Morin 2013, exercise 2.6](Z-bibliography.md#ref-morin-cpp), the
`System.arraycopy()` of the Java edition); a doubling moves them by
`memcpy()`.

<span id="alg-2-4"></span>**Algorithm 2.4.** The insertion and the removal
at a rank of `RingDeque` ([Morin 2013, §2.4](Z-bibliography.md#ref-morin-cpp));
$D[ k ]$ is the item of rank $k$.

--8<-- "generated/RingDeque-insert.pseudo.md"

--8<-- "generated/RingDeque-erase.pseudo.md"

![Figure 2.4. The operations of Morin's figure of the `ArrayDeque` (§2.4) on a `RingDeque`: the star marks the doubling from $m = 8$ to $m = 16$, which moves the items in rank order to the front of the new vector, after which the shift of the front side puts `a` in the place 15; the items added are in orange, the items that each operation moves (the shorter side) in violet, and the free places in the faintest tint.](../figures/sequence-ringdeque.svg){#fig-2-4}

<span id="lem-2-6"></span>**Lemma 2.6.** *On a `RingDeque` of $n$ items,
`operator[]` takes $O( 1 )$ time, the four operations at the ends take
$O( 1 )$ amortized time, and an insertion or a removal at the rank $i$
takes $O( 1 + \min\{ i , n - i \} )$ amortized time. Also, $n$ insertions
move fewer than $2 n$ items in the doublings, and the places are fewer
than $2 n + 8$ for $n$ items at the largest.*

*Proof.* The shift of [Algorithm 2.4](02-sequence.md#alg-2-4) moves $i - 1$
items on the front side, which it takes when $i < n / 2$, and $n - i - 1$
on the back side otherwise, i.e., at most $\min\{ i , n - i \}$ in both
cases. A doubling of a vector of $2^j$ places moves its $2^j$ items, and
the doublings up to $n$ items move $8 + 16 + \cdots + 2^j < 2^{j+1} \le 2
n$ items, with $2^j < n$ for the last one (or none, for $n \le 8$); the
vector of the largest size has $2^{j+1} < 2 n$ places, or 8. $\square$

A rotation of a LIST moves the item of rank $i$ to the rank $( i + r )
\bmod n$ ([Morin 2013, exercise 2.5](Z-bibliography.md#ref-morin-cpp)). On
a `RingDeque` it moves the last $r$ items to the front, or the first $n -
r$ to the back, whichever are fewer, in $O( 1 + \min\{ r , n - r \} )$
time. On a full vector it moves no item at all, since the ranks are read
from $h$, and moving $h$ back by $r$ places is the rotation
([Algorithm 2.5](02-sequence.md#alg-2-5)).

<span id="alg-2-5"></span>**Algorithm 2.5.** The rotation of a
`RingDeque` by $r$ ranks, $r$ reduced modulo $n$ ([Morin 2013, exercise
2.5](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/RingDeque-rotate.pseudo.md"

<span id="lem-2-7"></span>**Lemma 2.7.** *On a `RingDeque` of $n \ge 1$
items, `rotate( r )`, with $r$ reduced modulo $n$, takes $O( 1 + \min\{ r ,
n - r \} )$ time in the worst case, and $O( 1 )$ time with no item moved
when the vector is full.*

*Proof.* On a full vector, $m = n$, the item of rank $i$ is at the place $(
h + i ) \bmod n$; moving $h$ and the end back by $r$ places gives this item
the rank $( i + r ) \bmod n$, which is the rotation. Otherwise the loop makes
$\min\{ r , n - r \}$ steps, each a removal at one end and an insertion at
the other. Since $n < m$, the insertion finds the place that the removal
freed, and it never doubles the vector; both take $O( 1 )$ time. $\square$

Morin's `ArrayDeque` has the bound of [Lemma 2.6](02-sequence.md#lem-2-6),
but his vector halves when $3 n \le m$ and reduces the place by `%`, while
ours never halves and reduces it by a mask. The costs of the operations at
the ends, in terms of the queues and the deques that use them, are in
chapter [4](04-queue.md#ch-4).

## 2.4 A deque made of two stacks {#sec-2-4}

The `DualArrayDeque` of [Morin 2013, §2.5](Z-bibliography.md#ref-morin-cpp)
gets the bound of [Lemma 2.6](02-sequence.md#lem-2-6) out of two stacks on
vectors, each of which is "fast" at one end only. In it, the items are in
two stacks placed back to back, $F$ with the first items from its top down
and $B$ with the others from its bottom up, so that the item of rank $i$ is

$$
F[ |F| - 1 - i ] \ \text{ if } i < |F| , \qquad
B[ i - |F| ] \ \text{ otherwise} , \tag{2.4}
$$

and an insertion or a removal at the rank $i$ moves the items between $i$
and the top of its stack ([Algorithm 2.6](02-sequence.md#alg-2-6)). This is
cheap as long as neither stack is much smaller than the other. When an
operation leaves $3 |F| < |B|$ or $3 |B| < |F|$ (with two items at least),
`balance()` deals the items again, $F$ keeping $\lfloor n / 2 \rfloor$ of
them and $B$ the other $\lceil n / 2 \rceil$. [Figure
2.5](02-sequence.md#fig-2-5) follows the operations of Morin's figure of
the `DualArrayDeque` on ours, from the state that four calls of
`push_back()` leave.

<span id="alg-2-6"></span>**Algorithm 2.6.** The insertion, the removal
and the balance of `DualArrayDeque` ([Morin 2013,
§2.5](Z-bibliography.md#ref-morin-cpp)); $G$ is the larger stack and $S$
the smaller one.

--8<-- "generated/DualArrayDeque-insert.pseudo.md"

--8<-- "generated/DualArrayDeque-erase.pseudo.md"

--8<-- "generated/DualArrayDeque-balance.pseudo.md"

![Figure 2.5. The operations of Morin's figure of the `DualArrayDeque` (§2.5) on ours: the front stack $F$ is drawn reversed at the left of the dashed divider and the back stack $B$ at its right, so that the ranks read from left to right, the indices of each stack starting at 0 at the divider; a star marks a balance, the items added are in orange, and the items moved by the operation or by the balance (`b`, `d`, `c`) are in violet.](../figures/sequence-dualdeque.svg){#fig-2-5}

<span id="lem-2-8"></span>**Lemma 2.8.** *Starting from the empty
`DualArrayDeque`, any $m \ge 1$ insertions and removals spend $O( m )$ time
in the balances, and an operation at the rank $i$ shifts at most $3 \min\{
i , n - i \}$ items.*

*Proof.* After a balance of $n$ items the two stacks differ by one item at
most, and an operation changes $|F| - |B|$ by one. The next balance, of
$n'$ items, needs the larger stack to hold more than three times the
smaller one, i.e., $\bigl| |F| - |B| \bigr| > n' / 2$; hence at least $n' /
2 - 1$ operations separate the two balances, and they pay for the $O( n' )$
moves of the second. As for the shifts, an operation at the rank $i < |F|$
moves the $i$ items above it in $F$, by (2.4), and one at $i \ge |F|$ the
$n - i$ items above it in $B$. Before the operation the stacks are balanced
(or $n < 2$), and thus $|F| \le 3 |B| = 3 ( n - |F| )$, i.e., $|F| \le 3 n
/ 4$. An operation at $i < |F|$ has therefore $i < 3 n / 4$, i.e., $i < 3 (
n - i )$, which gives $i \le 3 \min\{ i , n - i \}$; the case of $B$ is the
mirror, with $n - i$ in place of $i$. $\square$

<span id="thm-2-2"></span>**Theorem 2.2.** *`DualArrayDeque` realizes the
LIST; `operator[]` takes $O( 1 )$ time, and `insert()` and `erase()` at
the rank $i$ take $O( 1 + \min\{ i , n - i \} )$ amortized time.*

*Proof.* [Lemma 2.8](02-sequence.md#lem-2-8), and [Lemma
2.3](02-sequence.md#lem-2-3) for the resizes of the two stacks. $\square$

Here the size of $F$ is `frontSize()`, and the two stacks are of a type `S`,
`std::vector< T >` by default, which needs `size()`, `operator[]`,
`push_back()`, `pop_back()` and either the `insert()` and `erase()` of a rank
or those of an iterator. Morin's balance builds two new stacks and copies the
items into them, which holds the places of $2 n$ items at the peak; ours is
done in place. The larger stack $G$ gives its $t = \bigl| |F|
- \lfloor n / 2 \rfloor \bigr|$ bottom items to the bottom of the smaller
one $S$, which grows by $t$ places and moves its items $t$ places up. It
moves them from its top down, so that no item is overwritten before it is
read; then $G$ moves its other items $t$ places down and gives back its $t$
top places. The rotation of [Algorithm 2.7](02-sequence.md#alg-2-7), instead, moves the
items one at a time from one end to the other, the shorter way round
([Morin 2013, exercise 2.5](Z-bibliography.md#ref-morin-cpp)).

<span id="alg-2-7"></span>**Algorithm 2.7.** The rotation of a
`DualArrayDeque` by $r$ ranks.

--8<-- "generated/DualArrayDeque-rotate.pseudo.md"

<span id="lem-2-9"></span>**Lemma 2.9.** *The balance in place of a
`DualArrayDeque` of $n$ items makes $n$ moves, and the two stacks hold at
most $n + t \le \lceil 3 n / 2 \rceil$ items at once, where $t \le \lceil n
/ 2 \rceil$, and $t < n / 2$ unless $S$ was empty. Also, `rotate( r )`, with
$r$ reduced modulo $n$, takes $O( 1 + \min\{ r , n - r \} )$ amortized
time.*

*Proof.* $S$ receives $t$ items by `push_back()` and moves each of its
$|S|$ items once, and $G$ moves once each of the $|G| - t$ items it keeps
before it gives back its $t$ top places, which then hold only the husks of
moves. The moves are thus $|S| + t + |G| - t = n$, and before the pops the
stacks hold $|S| + t + |G| = n + t$ items. If $G = F$, then $t = |F| -
\lfloor n / 2 \rfloor \le \lceil n / 2 \rceil$, with equality only for $|F|
= n$; if $G = B$, then $t = \lfloor n / 2 \rfloor - |F| \le n / 2$, with
equality only for $|F| = 0$. As for the rotation, it makes $\min\{ r , n - r
\}$ steps, each a removal at one end and an insertion at the other end.
Between the two the deque is balanced, and hence neither stack is empty if
two items are left. The insertion thus goes to the top of a stack and shifts
no item (one item at most if $n = 2$), and both operations cost $O( 1 )$
amortized time by [Lemma 2.8](02-sequence.md#lem-2-8) and [Lemma
2.3](02-sequence.md#lem-2-3). $\square$

With `S = RootishArrayStack< T >` ([§2.6](02-sequence.md#sec-2-6)) the deque
wastes $O( \sqrt{n} )$ places in each stack outside a balance ([Lemma
2.12](02-sequence.md#lem-2-12)), and it inserts and removes at the rank $i$ in
$O( 1 + \min\{ i , n - i \} )$ amortized time by [Lemma
2.8](02-sequence.md#lem-2-8), which answers [Morin 2013, exercise
2.9](Z-bibliography.md#ref-morin-cpp). There the balance in place matters,
since its peak is the places of $n + t \le \lceil 3 n / 2 \rceil$ items plus
the $O( \sqrt{n} )$ of the blocks, where two new stacks would hold those of
$2 n$ items.

In `ADS/sequence/DualArrayDeque.h`, the declarations of `DualArrayDeque`
that a user calls are

```cpp
// declarations of <ADS/sequence/DualArrayDeque.h>
template< class T , class S = std::vector< T > >
class DualArrayDeque
{
 public:
 DualArrayDeque( void );
 DualArrayDeque( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 const T & front( void ) const;
 const T & back( void ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_front( const T & x );
 void push_back( const T & x );
 void pop_front( void );
 void pop_back( void );
 void rotate( std::size_t r );
 void clear( void );
 std::size_t frontSize( void ) const;
 bool operator==( const DualArrayDeque & o ) const;
 };
```

## 2.5 The treque and the contiguous deque {#sec-2-5}

The *treque* (triple ended queue) of [Morin 2013, exercise
2.3](Z-bibliography.md#ref-morin-cpp) is a LIST whose insertions and
removals are "fast" near either end and near the middle. `Treque` splits
the items in the middle into two circular deques, $L$ with the ranks $0 ,
\ldots , |L| - 1$ and $R$ with the others, whose sizes differ by one at
most. An operation at the rank $i < |L|$ is one at the rank $i$ of $L$,
and otherwise one at the rank $i - |L|$ of $R$; after it, one item at most
passes between the back of $L$ and the front of $R$ to restore the balance
([Algorithm 2.8](02-sequence.md#alg-2-8) and
[Figure 2.6](02-sequence.md#fig-2-6)).

<span id="alg-2-8"></span>**Algorithm 2.8.** The insertion, the removal
and the balance of `Treque`.

--8<-- "generated/Treque-insert.pseudo.md"

--8<-- "generated/Treque-erase.pseudo.md"

--8<-- "generated/Treque-balance.pseudo.md"

![Figure 2.6. The two circular deques $L$ and $R$ of a `Treque`, each drawn in rank order, through `insert(4,x)`, `erase(1)` and `insert(0,y)`; in `erase(1)` the items `c` and `d` move one place and the balance moves `x` from the front of $R$ to the back of $L$ (all three in violet), and the items added are in orange.](../figures/sequence-treque.svg){#fig-2-6}

<span id="lem-2-10"></span>**Lemma 2.10.** *With $\lfloor n / 2 \rfloor \le
|L| \le \lceil n / 2 \rceil$, an insertion or a removal at the rank $i$ of a
`Treque` takes $O( 1 + \min\{ i , n - i , | n / 2 - i | \} )$ amortized
time, with no amortization but that of the doublings of the two deques.*

*Proof.* An operation at $i < |L|$ shifts $\min\{ i , |L| - i \}$ items of
$L$ by [Lemma 2.6](02-sequence.md#lem-2-6), i.e., $\min\{ i , n / 2 - i \}$
up to a constant, and one at $i \ge |L|$ shifts $\min\{ i - |L| , n - i \}$,
i.e., $\min\{ i - n / 2 , n - i \}$ up to a constant. Together they are $O(
1 + \min\{ i , n - i , | n / 2 - i | \} )$. Also, the balance moves one item
between two ends, in $O( 1 )$ amortized time, and it restores $\bigl| |L| -
|R| \bigr| \le 1$, which an operation breaks by one at most. $\square$

The `ContiguousDeque` of [Morin 2013, exercise
2.8](Z-bibliography.md#ref-morin-cpp) does without modular arithmetic. Its
items are in order in one contiguous piece $A[ h .. h + n - 1 ]$ of a vector
of $c$ places, so that the item of rank $i$ is $A[ h + i ]$, the iterators
are pointers, and `data()` gives the items to any function that wants an
array. An insertion at the rank $i$ moves the items of the shorter side, as
in `RingDeque`. When it needs a place beyond an end of the vector,
`rebuild()` puts the items in the middle of a vector of $3 n + 8$ places,
with at least $n + 4$ places free on each side ([Algorithm
2.9](02-sequence.md#alg-2-9) and [Figure 2.7](02-sequence.md#fig-2-7)). A
removal that leaves more than $9 n + 24$ places rebuilds too, which keeps
the space linear.

<span id="alg-2-9"></span>**Algorithm 2.9.** The insertion, the removal
and the rebuild of `ContiguousDeque` ([Morin 2013, exercise
2.8](Z-bibliography.md#ref-morin-cpp)); $B$ is a vector of $c'$ places.

--8<-- "generated/ContiguousDeque-insert.pseudo.md"

--8<-- "generated/ContiguousDeque-erase.pseudo.md"

--8<-- "generated/ContiguousDeque-rebuild.pseudo.md"

![Figure 2.7. A `ContiguousDeque`, whose items are one contiguous piece of the vector: `push_back(e)` reaches the end of the 8 places and rebuilds the piece into $c = 3 n + 8 = 20$ places at $h = 8$ (the items moved, `a` to `d`, in violet, `e` in orange); `push_front(z)` sets $h = 7$, and `insert(2,x)` moves the shorter front side (`z` and `a`, in violet) one place back, to $h = 6$, and writes `x` (in orange).](../figures/sequence-contiguous.svg){#fig-2-7}

<span id="lem-2-11"></span>**Lemma 2.11.** *A `ContiguousDeque` takes $O( 1 )$
amortized time for each operation at an end and $O( 1 + \min\{ i , n - i \}
)$ amortized time for an insertion or a removal at the rank $i$, and at most
$9 n + 24$ places for $n$ items after a removal (at most $8 n + 24$ beyond
the items).*

*Proof.* A rebuild of $n_0$ items leaves the vector with $c \ge 3 n_0 + 8$
places, whether it allocates one of $3 n_0 + 8$ or keeps the old one in
place, and hence each side has at least $( c - n_0 ) / 2 - 1 \ge n_0 + 3$
free places. An operation takes one place of one side at most, and thus at
least $n_0 + 3$ operations come before the next rebuild for lack of room;
they pay for its $O( n_0 )$ moves. A rebuild for an excess of room is paid
by the removals since the last vector was allocated. That vector had $3
n' + 8$ places for the $n'$ items of its rebuild, and a rebuild in place
keeps the places, while the excess needs $3 n' + 8 > 9 n + 24$, i.e., $n <
n' / 3$. Hence more than $2 n' / 3$ removals came after that allocation,
and they pay for the $O( n' )$ moves of the rebuild and for no other
rebuild for an excess. Finally, the shift moves $\min\{ i , n - i \}$
items, as in [Lemma 2.6](02-sequence.md#lem-2-6). $\square$

We remark that the hint of the exercise suggests a rebuild that leaves room
for $n / 2$ operations, while ours leaves room for $n + 3$ at least. A
rebuild in the same vector, by `memmove()`, costs no allocation when the
vector is already of a fitting size (at least $3 n + 8$ places and at most
$9 n + 24$) and the items are trivially copyable; this is the common case of
a queue that runs along the vector. Compared with `RingDeque`, the
contiguous deque pays the rebuilds that the circular vector avoids when it
is used as a queue, and it saves the mask of each access and the split of
the items into two pieces.

In `ADS/sequence/Treque.h`, the declarations of `Treque` that a user calls
are

```cpp
// declarations of <ADS/sequence/Treque.h>
template< class T >
class Treque
{
 public:
 Treque( void );
 Treque( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 const T & front( void ) const;
 const T & back( void ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_front( const T & x );
 void push_back( const T & x );
 void pop_front( void );
 void pop_back( void );
 void clear( void );
 bool operator==( const Treque & o ) const;
 };
```

In `ADS/sequence/ContiguousDeque.h`, the declarations of `ContiguousDeque`
that a user calls are

```cpp
// declarations of <ADS/sequence/ContiguousDeque.h>
template< class T >
class ContiguousDeque
{
 public:
 ContiguousDeque( void );
 ContiguousDeque( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 std::size_t capacity( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 T & front( void );
 T & back( void );
 T * data( void );
 const T * data( void ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_front( const T & x );
 void push_back( const T & x );
 void pop_front( void );
 void pop_back( void );
 void clear( void );
 bool operator==( const ContiguousDeque & o ) const;
 };
```

## 2.6 The rootish array stack {#sec-2-6}

A vector that doubles wastes up to $c - n$ places, i.e., up to $3 n$ after a
removal and $n$ after insertions only, and it copies each item once on
average. The `RootishArrayStack` of [Brodnik et al.
1999](Z-bibliography.md#ref-brodnik1999), in the form of [Morin 2013,
§2.6](Z-bibliography.md#ref-morin-cpp), keeps instead its items in $r$
blocks of $1 , 2 , \ldots , r$ places, which together have

$$
1 + 2 + \cdots + r = \frac{ r ( r + 1 ) }{ 2 } \tag{2.5}
$$

places ([Figure 2.9](02-sequence.md#fig-2-9) proves (2.5) by two copies of
the staircase). A block is added when the places are full, and thus a
vector that grows never moves its items; the item of rank $i$ is in the
block

$$
b( i ) = \Bigl\lceil \frac{ -3 + \sqrt{ 9 + 8 i } }{ 2 } \Bigr\rceil ,
\tag{2.6}
$$

i.e., the least $b$ with $( b + 1 ) ( b + 2 ) / 2 > i$, at the place $i - b
( b + 1 ) / 2$ of it. An insertion or a removal at the rank $i$ moves the $n
- i$ items after it through these places ([Algorithm
2.10](02-sequence.md#alg-2-10)). Conversely, the last block is freed while
$( r - 2 ) ( r - 1 ) / 2 \ge n$, which keeps one empty block at most for the
next insertions (cf. [Lemma 2.12](02-sequence.md#lem-2-12)). [Figure
2.8](02-sequence.md#fig-2-8) follows the operations of Morin's figure on a
`RootishArrayStack`. Note that the places are raw memory, the items being
built in them when inserted and destroyed when removed.

<span id="alg-2-10"></span>**Algorithm 2.10.** The insertion and the
removal of `RootishArrayStack` ([Morin 2013,
§2.6](Z-bibliography.md#ref-morin-cpp)), and the block of a rank by
(2.6); $B_0 , \ldots , B_{r-1}$ are the blocks.

--8<-- "generated/RootishArrayStack-insert.pseudo.md"

--8<-- "generated/RootishArrayStack-erase.pseudo.md"

--8<-- "generated/RootishArrayStack-block.pseudo.md"

![Figure 2.8. The operations of Morin's figure of the `RootishArrayStack` (§2.6) on ours: the blocks of 1, 2, 3 and 4 places with the ranks 0 to 9 below them, through `add(2,x)`, `remove(1)`, `remove(7)` and `remove(6)`, with $r$ and $n$ at the right; the item added is in orange, the items shifted by an operation in violet and the free places in the faintest tint, and the empty last block is kept, since $( r - 2 ) ( r - 1 ) / 2 = 3 < n$.](../figures/sequence-rootish.svg){#fig-2-8}

![Figure 2.9. For $r = 5$, a grid of $r$ columns and $r + 1$ rows split into two staircases of $1 , 2 , \ldots , r$ squares, the one from the bottom in the light tint and the one from the top in violet, which shows that $1 + 2 + \cdots + r = r ( r + 1 ) / 2$ (2.5).](../figures/sequence-gauss.svg){#fig-2-9}

<span id="lem-2-12"></span>**Lemma 2.12.** *A `RootishArrayStack` of $n \ge
1$ items has $( r - 2 ) ( r - 1 ) / 2 < n \le r ( r + 1 ) / 2$, hence $r
\le \sqrt{ 2 n } + 2$ and at most $2 r - 1 \le \sqrt{ 8 n } + 3$ places
unused, besides the $r$ pointers to the blocks; the blocks added and freed
cost $O( 1 )$ amortized time per operation.*

*Proof.* The upper bound on $n$ holds since a block is added when the
places are full, the lower one since the last block is freed while $( r - 2
) ( r - 1 ) / 2 \ge n$. Out of the $r ( r + 1 ) / 2$ places, those unused
are fewer than $r ( r + 1 ) / 2 - ( r - 2 ) ( r - 1 ) / 2 = 2 r - 1$. From
$( r - 2 )^2 < ( r - 2 ) ( r - 1 ) < 2 n$ we have $r < \sqrt{ 2 n } + 2$. A
block of $r$ places is added or freed after $r - 1$ operations at least
since the last change of $r$, which pay for its allocation, as in
[Lemma 2.3](02-sequence.md#lem-2-3). $\square$

This waste is optimal up to a constant. In fact, [Brodnik et al.
1999](Z-bibliography.md#ref-brodnik1999) show that, after $n$ insertions
into any array that grows and shrinks by one item in $O( 1 )$ time, some
sequence of operations leaves $\Omega( \sqrt{n} )$ places wasted or
allocated and not yet used ([Morin 2013,
§2.6.2](Z-bibliography.md#ref-morin-cpp)). The test
`ods_lists.ods-2.6.rootish-array-stack` checks the bound $2 r + 1$ (the
block kept for the next insertions included) after every operation of $10^5$
insertions and of $10^5$ removals.

<span id="thm-2-3"></span>**Theorem 2.3.** *`RootishArrayStack` realizes
the LIST; `operator[]` takes $O( 1 )$ time, `insert()` and `erase()` at the
rank $i$ take $O( 1 + n - i )$ amortized time, and the places not used are
$O( \sqrt{n} )$.*

*Proof.* [Lemma 2.12](02-sequence.md#lem-2-12), and (2.6) computed in $O( 1
)$ time as below. $\square$

[Morin 2013, §2.6.4](Z-bibliography.md#ref-morin-cpp) computes (2.6) with
no floating point, by a table of the square roots of the integers below
$2^{16}$ and a table of the logarithms, after a lemma that bounds the error
of the approximate root they give. `RootishArrayStack::block()` takes the
square root of a `double` instead, and it corrects the result by the two
comparisons with $b ( b + 1 ) / 2$ and $( b + 1 ) ( b + 2 ) / 2$
([Algorithm 2.10](02-sequence.md#alg-2-10)).

<span id="lem-2-13"></span>**Lemma 2.13.** *For any rank $i < 2^{52}$,
`block( i )` returns the block $b$ with $b ( b + 1 ) / 2 \le i < ( b + 1 ) ( b
+ 2 ) / 2$, i.e., (2.6), after at most one step of each correction.*

*Proof.* The two loops stop only when both inequalities hold, and the blocks
are a partition of the ranks; hence, the result is (2.6) whatever the first
estimate. In a `double` the rank $i$ is exact, and so is $8 i$; the sum $9 +
8 i < 2^{56}$ is exact for $i < 2^{50}$, and otherwise it is rounded with a
relative error of at most $2^{-53}$. Since the root is correctly rounded by
IEEE 754 and $\sqrt{ 9 + 8 i } < 2^{28}$, the estimate before the ceiling
differs from the exact value by far less than one (about $2^{-25}$ at most,
the subtraction of 3 and the halving adding errors of the same order). Its
ceiling thus differs from (2.6) by one at most. Each loop thus runs at most
once, and the products $b ( b + 1 )$ and $( b + 1 ) ( b + 2 )$, below
$2^{57}$, do not overflow a 64-bit word. $\square$

Morin's tables cost two reads that may miss the cache, and the root of a
`double` a latency of a few tens of cycles at most on the current
processors; we leave the choice to the benchmarks. We remark that Morin's
code computes $9 + 8 i$ in an `int`, which overflows for $i > 2^{28}$. Also,
his blocks are arrays of `new T[ b + 1 ]`, whose items are built by default
and assigned later; ours build an item only when it is inserted.

In `ADS/sequence/RootishArrayStack.h`, the declarations of
`RootishArrayStack` that a user calls are

```cpp
// declarations of <ADS/sequence/RootishArrayStack.h>
template< class T , class A = std::allocator< T > >
class RootishArrayStack
{
 public:
 RootishArrayStack( void );
 explicit RootishArrayStack( const A & alloc );
 RootishArrayStack( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 std::size_t capacity( void ) const;
 std::size_t blocks( void ) const;
 static std::size_t block( std::size_t i );
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 const T & front( void ) const;
 const T & back( void ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_back( const T & x );
 void push_back( T && x );
 template< class... Args >
 T & emplace_back( Args &&... args );
 void pop_back( void );
 void clear( void );
 bool operator==( const RootishArrayStack & o ) const;
 };
```

## 2.7 The tiered vector {#sec-2-7}

The tiered vector of [Goodrich and Kloss
1999](Z-bibliography.md#ref-goodrich1999) trades the shift of $O( n )$ items
of a vector for one of $O( \sqrt{n} )$, while it keeps the access by index
in $O( 1 )$ time. `TieredVector` keeps its items in blocks of $b$ places,
$b$ a power of 2, each a circular buffer with the offset of its first item.
All the blocks are full but the first, with $g < b$ places free at its front
(the *gap*), and the last; the item of index $i$ is thus the item $( g + i )
\bmod b$ of the block $\lfloor ( g + i ) / b \rfloor$, an addition, a shift,
a mask and two reads. An insertion at $i \ge n / 2$ moves by one place the
items of its block after $i$, fewer than $b$. Then, from the last block back
to the block after that of $i$, it moves the last item of each block to the
front of the next one, in $O( 1 )$ per block, since the front of a circular
buffer moves by decreasing its offset ([Algorithm
2.11](02-sequence.md#alg-2-11)). An insertion at $i < n / 2$ moves instead
toward the front, the gap of the first block playing the part of the free
places of the last one. A block is added at the front when $g = 0$, and
removed when it empties. [Figure 2.10](02-sequence.md#fig-2-10) shows both
directions with $b = 4$, while the code keeps $b \ge 64$: $b$ is the least
power of 2 not below $\sqrt{n}$, and 64 at least, and the items are moved
into the blocks of the new $b$ when $n$ leaves $[ b^2 / 4 , 4 b^2 ]$.

<span id="alg-2-11"></span>**Algorithm 2.11.** The insertion and the
removal of `TieredVector` with the shifts toward the back ([Goodrich and
Kloss 1999](Z-bibliography.md#ref-goodrich1999)), and the insertion with
the shifts toward the front; $B_j$ is the block $j$, $B_j.\mathit{off}$
its offset, the places taken modulo $b$, and the gap $g = 0$ in the first
two (with a gap, the indices are those of $g + i$).

--8<-- "generated/TieredVector-insert.pseudo.md"

--8<-- "generated/TieredVector-erase.pseudo.md"

--8<-- "generated/TieredVector-insertFront.pseudo.md"

![Figure 2.10. A `TieredVector` with blocks of $b = 4$ places (each circular, drawn in rank order) and a gap of $g$ free places before the first item, through `insert(5,x)`, `insert(1,y)` and `insert(0,z)`, with $n$ and $g$ at the right: `insert(5,x)` shifts toward the back (`g` passes to the front of the third block and `f` moves one place on, both in violet), `insert(1,y)` moves `a` into the gap (in violet), and `insert(0,z)` finds no gap left and opens a new block at the front with $g = 3$; the items added are in orange.](../figures/sequence-tiered.svg){#fig-2-10}

<span id="lem-2-14"></span>**Lemma 2.14.** *Let a `TieredVector` keep its
items in blocks of $b$ places with $b^2 / 4 \le n \le 4 b^2$ (or $b = 64$),
all full but the first, with a gap $g < b$, and the last, and let an
insertion or a removal at the rank $i$ shift toward the back when $i \ge n
/ 2$ and toward the front otherwise. Then it moves*

$$
O\bigl( 1 + \min\{ \mu , b \} + \mu / b \bigr)
= O\bigl( 1 + \min\{ i , \sqrt{n} , n - i \} \bigr) \tag{2.7}
$$

*items, $\mu = \min\{ i , n - i \}$, plus $O( 1 )$ amortized for the blocks
added and removed and for the rebuilds when $b$ changes, and it wastes $O(
\sqrt{n} )$ places.*

*Proof.* A shift toward the back moves the items of the block of $i$ after
$i$, at most $\min\{ b , n - i \}$. Then it moves one item between each two
consecutive blocks after it, at most $( n - i ) / b + 1$ of them; the shift
toward the front is the mirror, with $\min\{ b , i \} + i / b + 1$ items.
With $b = \Theta( \sqrt{n} )$, one has $\mu / b = O( \sqrt{n} )$ and $\min\{
\mu , b \} + \mu / b = O( \min\{ \mu , \sqrt{n} \} )$. A block is added at
the front only when the gap is 0, after which $b$ insertions at the front
come before the next one; inserting the pointer to it in front of the $n /
b$ others costs $O( n / b ) = O( b )$, i.e., $O( 1 )$ amortized, and the
blocks at the back are the same. A rebuild for $n$ items takes the least
power of 2 whose square is not below $n$ (64 at least), and hence it leaves
$b^2 / 4 < n \le b^2$ (or $b = 64$). A rebuild for growth thus comes after
more than $3 b^2$ insertions since the previous one, i.e., $\Omega( n )$,
which pay for its $O( n )$ moves. It takes the block $4 b$, since $( 2 b )^2 <
4 b^2 + 1$, and leaves $n$ just above $( 4 b )^2 / 4$; hence a rebuild for
shrinking may follow after two removals, which takes the block $2 b$ with $n =
( 2 b )^2 - 1$, and which we charge to the same insertions. Any other
rebuild for shrinking follows one for shrinking, which left $n = b^2 - 1$
for its block $b$, and hence it comes after more than $3 b^2 / 4$ removals
(or the rebuild of a copy, whose own $O( n )$ time pays for it). Finally, the waste is the gap, the free places of the last block and the $O(
n / b )$ pointers of the directory, i.e., $O( b + n / b ) = O( \sqrt{n} )$.
$\square$

Note that the bound of [Morin 2013,
exercise 2.10](Z-bibliography.md#ref-morin-cpp), $O( 1 + \min\{ \sqrt{n} ,
n - i \} )$, is (2.7) without the shifts toward the front, and that that of
his exercise 2.11, $O( 1 + \min\{ i , \sqrt{n} , n - i \} )$, is (2.7)
itself. His hint, the blocks of the `SEList`
([§2.12](02-sequence.md#sec-2-12)), leads to the same structure with the
blocks in a list instead of a directory, which loses the access by index in
$O( 1 )$ time. [Bille et al. 2017](Z-bibliography.md#ref-bille2017)
engineer the tiered vector further, with more levels and a careful layout
in memory; `CubishArrayStack` below is the tiered vector with three levels.

In `ADS/sequence/TieredVector.h`, the declarations of `TieredVector` that a
user calls are

```cpp
// declarations of <ADS/sequence/TieredVector.h>
template< class T , class A = std::allocator< T > >
class TieredVector
{
 public:
 TieredVector( void );
 explicit TieredVector( const A & alloc );
 bool empty( void ) const;
 std::size_t size( void ) const;
 std::size_t block( void ) const;
 std::size_t capacity( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 const T & front( void ) const;
 const T & back( void ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_back( const T & x );
 void pop_back( void );
 void push_front( const T & x );
 void pop_front( void );
 void clear( void );
 };
```

## 2.8 The cubish array stack {#sec-2-8}

The cubish array stack of [Morin 2013, exercise
2.12](Z-bibliography.md#ref-morin-cpp) is a LIST in three levels whose
`operator[]` takes $O( 1 )$ time, and whose insertions and removals take $O(
n^{1/3} )$ amortized time, with $O( n^{2/3} )$ places wasted.
`CubishArrayStack` is the tiered vector with three levels. With $b$ a power
of 2 and $b^3 / 8 \le n \le 8 b^3$ (or $b = 16$, the least block of the
code), the items are in *leaves* of $b$ places,
each a circular buffer with the offset of its first item. In turn, the
leaves are in *middle blocks* of $b^2$ items each, all full but the last; a
middle block is a circular buffer of $2 b$ leaves whose leaves are all full
but the first, with a gap $g < b$ at its front, and the last. Hence the item
of rank $i$ is the item $r = i \bmod b^2$ of the middle block $\lfloor i /
b^2 \rfloor$, and the item of rank $r$ of a middle block is the item $( g +
r ) \bmod b$ of its leaf $\lfloor ( g + r ) / b \rfloor$: shifts, masks and
five reads. A middle block takes an item at its front, or gives one from its
back, in $O( 1 )$ time, by its gap and the offsets of its leaves. An
insertion at the rank $i$ moves the last item of each full middle block
after the one of $i$ to the front of the next one, and then it inserts in
its middle block as a tiered vector of $2 b$ leaves does ([Algorithm
2.12](02-sequence.md#alg-2-12) and [Figure 2.11](02-sequence.md#fig-2-11),
with $b = 2$).

<span id="alg-2-12"></span>**Algorithm 2.12.** The insertion of
`CubishArrayStack` ([Morin 2013, exercise
2.12](Z-bibliography.md#ref-morin-cpp)); $M_0 , \ldots , M_t$ are the
middle blocks.

--8<-- "generated/CubishArrayStack-insert.pseudo.md"

![Figure 2.11. The middle blocks $M_0$, $M_1$, $M_2$ of a `CubishArrayStack` with $b = 2$, each made of leaves of 2 places, before and after `insert(1,x)`: $M_1$ passes `h` to the front of $M_2$ and $M_0$ passes `d` to the front of $M_1$ (`d` and `h` in violet), then `x` (in orange) is inserted at the rank 1 of $M_0$, shifting `b` and `c` (in violet); the free places of the leaves are in the faintest tint.](../figures/sequence-cubish.svg){#fig-2-11}

<span id="lem-2-15"></span>**Lemma 2.15.** *A `CubishArrayStack` takes $O( 1
)$ time for `operator[]`, $O( n^{1/3} )$ amortized time for an insertion or
a removal, and wastes $O( n^{2/3} )$ places.*

*Proof.* In the cascade, one item moves for each of the $O( n / b^2 + 1 ) = O( b
)$ middle blocks after the one of $i$, in $O( 1 )$ each. Then the insertion
in the middle block moves $O( b )$ items of its leaf and one item for each
of the $O( b )$ leaves after it, by [Lemma 2.14](02-sequence.md#lem-2-14) with
leaves in place of blocks; with $b = \Theta( n^{1/3} )$ this is $O( n^{1/3}
)$. Each middle block wastes the free places of its first and its last leaf
and has a directory of $2 b$ leaves, i.e., $O( b )$ for each of the $O( n /
b^2 + 1 )$ middle blocks, and $O( n / b ) = O( n^{2/3} )$ in all. Finally,
the rebuilds, when $n$ leaves $[ b^3 / 8 , 8 b^3 ]$, cost $O( n )$ each, and
they cost $O( 1 )$ amortized by the argument of [Lemma
2.14](02-sequence.md#lem-2-14) with cubes in place of squares: a rebuild for
growth takes the block $4 b$, and the one rebuild for shrinking that may
follow it after two removals is charged to the same insertions. $\square$

Thus, the deque of leaves inside each middle block makes the two operations
at its ends cost $O( 1 )$, which the cascade needs. A few leaves freed are
kept for the next ones, since an insertion frees a leaf at the back of a
middle block and takes one at the front of the next. Also, the removal is
the mirror of the insertion (`erase()`, whose pseudocode is in the header),
and `pop_back()`, `push_back()`, `size()`, `capacity()` and `clear()`
complete the class.

In `ADS/sequence/CubishArrayStack.h`, the declarations of
`CubishArrayStack` that a user calls are

```cpp
// declarations of <ADS/sequence/CubishArrayStack.h>
template< class T >
class CubishArrayStack
{
 public:
 CubishArrayStack( void );
 CubishArrayStack( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 std::size_t block( void ) const;
 std::size_t capacity( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_back( const T & x );
 void pop_back( void );
 void clear( void );
 bool operator==( const CubishArrayStack & o ) const;
 };
```

## 2.9 The singly linked list {#sec-2-9}

A linked list keeps each item in a separate cell, which holds the
address of the next cell. An insertion allocates a cell and changes the
links of its neighbors, in $O( 1 )$ time at any place that one holds;
however, there is no access by index, and a scan follows pointers that may
be scattered in memory. `SinglyList` realizes the SEQUENCE as a singly
linked list, not circular and with no sentinel ([Montresor 2025, exercise
4.6](Z-bibliography.md#ref-montresor2025)), and the choice of the position
makes it efficient.

<span id="lem-2-16"></span>**Lemma 2.16.** *In a list whose cells have a
link to the next one only, an insertion before $s_i$ and the removal of
$s_i$ change the link of the cell of $s_{i-1}$ (or the link to the first
cell, for $i = 0$). With $\mathit{pos}_i$ the cell of $s_{i-1}$, and nil
for $i = 0$, both take $O( 1 )$ time, while `prev()` and `tail()` take
$\Theta( n )$ time in the worst case.*

*Proof.* In fact, the link to change is the one that points to the cell of
$s_i$, which is in the cell before it, and no link leads from a cell back to
its predecessor. A position that held the cell of $s_i$ itself would thus
force a scan from the head to find it, i.e., $\Theta( i )$ time. With the
position before the item, a scan is needed only to go backwards, i.e., by
`prev()` and by `tail()`. $\square$

This is the convention of the realizations with pointers and with cursors of
[Aho et al. 1983, §2.2](Z-bibliography.md#ref-aho1983), which `SinglyList`
follows with $\mathit{pos}_n$ the last cell (nil if the list is empty) and
$\mathit{pos}_{-1}$ a distinguished address of the list itself. After
`insert( p , v )` the new item has the position $p$, and after `remove( p )`
its successor has it ([Algorithm 2.13](02-sequence.md#alg-2-13)). Its cells
come from an `ObjectPool` of the list ([§5.5](05-memory.md#sec-5-5)), which
takes back the cell of `remove()` for the next `insert()`. Hence a cell
costs no call to the allocator, and the cells made one after another are
next to each other in memory; the price is that the list keeps the memory of
its largest size until it dies.

<span id="alg-2-13"></span>**Algorithm 2.13.** The operations of
`SinglyList` that use the position before the item, and the scan of
`prev()`.

--8<-- "generated/SinglyList-insert.pseudo.md"

--8<-- "generated/SinglyList-remove.pseudo.md"

--8<-- "generated/SinglyList-prev.pseudo.md"

`SinglyList` is also the `SLList` of [Morin 2013,
§3.1](Z-bibliography.md#ref-morin-cpp). It keeps its last cell and its
size, so that `push_back()` (his `add( x )`), `push_front()` and
`pop_front()` (his `push( x )`, `pop()` and `remove()`), `front()`,
`back()` and `size()` take $O( 1 )$ time; the list is thus both a stack at
its front and a queue from its back to its front ([Figure
2.12](02-sequence.md#fig-2-12), on the data of Morin's figure).

![Figure 2.12. A `SinglyList` used as a stack and as a queue, on the data of Morin's figure (§3.1): the list `a b c d e`, then after `push_back(x)` (his `add(x)`), two calls of `pop_front()` (his `remove()` and `pop()`, which return `a` and `b`) and `push_front(y)` (his `push(y)`), with the pointers to the first and the last cell and $n$ at the right of each row; the new cells and the links written with them are in orange, and the cell that becomes the first after each `pop_front()` is in violet.](../figures/sequence-sllist.svg){#fig-2-12}

<span id="thm-2-4"></span>**Theorem 2.4.** *`SinglyList` realizes the
SEQUENCE, the STACK and the QUEUE: every operation of the three takes $O( 1
)$ time, but `prev()` and `tail()`, which take $O( n )$; the operations by
rank take $O( 1 + i )$ time.*

*Proof.* [Lemma 2.16](02-sequence.md#lem-2-16) for the positions; the ends
are the first and the last cell, which the list keeps, and a rank $i$ is
reached by a walk of $i$ cells. $\square$

Furthermore, the list has the operations of the exercises of chapter 3 of
Morin on the singly linked list. First, `get( i )`, `set( i , x )`,
`add( i , x )` and `erase( i )` walk $i$ cells ([Morin 2013, exercise
3.3](Z-bibliography.md#ref-morin-cpp), the insertion in [Algorithm
2.14](02-sequence.md#alg-2-14)). Then, `secondLast()` stops at the cell whose
successor has no successor, with no use of the size nor of the last cell
(exercise 3.2), and `reverse()` turns each link back while it walks, with
three pointers and no cell made, in $O( n )$ time and with no recursion
(exercise 3.4, [Algorithm 2.15](02-sequence.md#alg-2-15)). Finally,
`checkSize()` counts the cells and throws if their number is not `size()`
(exercise 3.5). A dummy cell before the first one cannot remove the special
cases of `push_front()`, `pop_front()` and `push_back()` (Morin's `push()`,
`pop()`, `remove()` and `add()`), as it does for a doubly linked list
(exercise 3.1). In fact, the queue adds at the back, whose cell must still be
set apart from the dummy when the list is empty. Also, a removal at the back
would need the cell before the last one, which a singly linked list reaches
only by a scan.

<span id="alg-2-14"></span>**Algorithm 2.14.** The insertion by rank of
`SinglyList` ([Morin 2013, exercise 3.3](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/SinglyList-add.pseudo.md"

<span id="alg-2-15"></span>**Algorithm 2.15.** The second last item and
the reversal in place of `SinglyList` ([Morin 2013, exercises 3.2 and
3.4](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/SinglyList-secondLast.pseudo.md"

--8<-- "generated/SinglyList-reverse.pseudo.md"

<span id="lem-2-17"></span>**Lemma 2.17.** *On a `SinglyList` of $n$
items, `secondLast()` (for $n \ge 2$) and `checkSize()` take $O( n )$ time,
and `reverse()` reverses the list in $O( n )$ time and $O( 1 )$ space.*

*Proof.* Both `secondLast()` and `checkSize()` walk the cells once. Before
each step of `reverse()`,
the cells already visited form, from $p$, the list of their items in the
reverse order, and $u$ is the first of the cells not yet visited, in their
order. The step moves the cell $u$ to the front of the first list by one
assignment of its link, and the loop ends when no cell is left, with $p$ the
old last cell; three pointers are used and no cell is made. $\square$

In `ADS/sequence/SinglyList.h`, the declarations of `SinglyList` that a
user calls are

```cpp
// declarations of <ADS/sequence/SinglyList.h>
template< class T >
class SinglyList
{
 public:
 struct Cell;
 using Item = T;
 using Pos = Cell *;
 SinglyList( void );
 SinglyList( std::initializer_list< T > items );
 bool empty( void ) const;
 bool finished( Pos p ) const;
 Pos head( void ) const;
 Pos tail( void ) const;
 Pos next( Pos p ) const;
 Pos prev( Pos p ) const;
 Pos insert( Pos p , const T & v );
 Pos remove( Pos p );
 const T & read( Pos p ) const;
 void write( Pos p , const T & v );
 bool operator==( const SinglyList & o ) const;
 template< std::input_iterator I , std::sentinel_for< I > S >
 SinglyList( I first , S last );
 void push_front( const T & v );
 std::size_t size( void ) const;
 void push_back( const T & v );
 T pop_front( void );
 const T & front( void ) const;
 const T & back( void ) const;
 const T & get( std::size_t i ) const;
 T set( std::size_t i , const T & x );
 void add( std::size_t i , const T & x );
 T erase( std::size_t i );
 const T & secondLast( void ) const;
 void reverse( void );
 void checkSize( void ) const;
 };
```

## 2.10 The doubly linked list {#sec-2-10}

`List` is the LIST of [Montresor 2025,
§4.1](Z-bibliography.md#ref-montresor2025) and the `DLList` of [Morin 2013,
§3.2](Z-bibliography.md#ref-morin-cpp), a bidirectional circular list of
cells with a *sentinel*, i.e., a cell with no item that is the predecessor
of the first cell and the successor of the last one ([Figure
2.13](02-sequence.md#fig-2-13)). A list of $n$ items has thus $n + 1$ cells,
every cell has both neighbors, and the insertion and the removal have no
special case for the empty list or for the ends. The position
$\mathit{pos}_i$ is the cell of $s_i$, and $\mathit{pos}_{-1}$ and
$\mathit{pos}_n$ are both the sentinel, for which `finished()` looks.
`insert( p , v )` puts $v$ before the cell $p$, in four assignments of
links; it is the `addBefore( w )` of Morin ([Algorithm
2.16](02-sequence.md#alg-2-16) and [Figure 2.14](02-sequence.md#fig-2-14)),
which [Morin 2013, exercise 3.6](Z-bibliography.md#ref-morin-cpp) proposes
to rewrite from memory. Then `remove( p )` unlinks the cell $p$ in two
assignments.

![Figure 2.13. A `List` of the items `a b c d e`, i.e., a circular doubly linked list with a sentinel, each cell with its link to the successor above and to the predecessor below; the link of `e` to its successor and that of the sentinel to its predecessor close the circle.](../figures/sequence-dllist.svg){#fig-2-13}

<span id="alg-2-16"></span>**Algorithm 2.16.** The insertion and the
removal of `List` ([Montresor 2025, §4.1](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/List-insert.pseudo.md"

--8<-- "generated/List-remove.pseudo.md"

![Figure 2.14. The insertion of a cell before $w$ (Morin's `addBefore()`, §3.2.1): the cells $u.\mathit{prev}$ and $w$ before `insert(w, v)`, and the new cell $u$ between them after it; $u$ and the four links that change ($u.\mathit{prev}.\mathit{next} = u$, $u.\mathit{next} = w$, $u.\mathit{prev}$ and $w.\mathit{prev} = u$) are in orange, the other links in gray.](../figures/sequence-addbefore.svg){#fig-2-14}

In ADS the sentinel is a member of the list rather than a cell of the heap.
Moving a list thus relinks its first and last cells to the new sentinel in
$O( 1 )$ time and allocates nothing, and it leaves the list moved from empty
and still usable. As in `SinglyList`, the cells come from an `ObjectPool`.
However, a pool is no gain for a list of a few cells, which still takes a
block and a header per block, and a structure of many short lists (e.g., the
chains of `ChainedHashTable`, chapter [7](07-dictionary.md#ch-7)) takes
`List< T , false >`, whose cells come one by one from `new` and `delete`.
Also, the list keeps its size, and it has the operations by rank of a LIST,
`get( i )`, `set( i , x )`, `add( i , x )` and `erase( i )`. These reach the
cell of rank $i$ by `position( i )` from the nearer end, as Morin's
`getNode( i )` does ([Algorithm 2.17](02-sequence.md#alg-2-17)).

<span id="alg-2-17"></span>**Algorithm 2.17.** The cell of rank $i$, from
the nearer end ([Morin 2013, §3.2](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/List-position.pseudo.md"

<span id="thm-2-5"></span>**Theorem 2.5.** *`List` realizes the SEQUENCE,
with every operation in $O( 1 )$ time, and the LIST, with `get()`, `set()`,
`add()` and `erase()` at the rank $i$ in $O( 1 + \min\{ i , n - i \} )$
time.*

*Proof.* An operation on a position changes a constant number of links;
`position( i )` walks $i$ cells forward from the sentinel when $i < n / 2$
and $n - i$ backward otherwise. $\square$

As for the contracts, they check that a position is a cell of the list,
which costs a scan and is therefore an audit, in the operations that change
the list (`insert()`, `remove()` and `write()`). They do not check it in
`next()`, `prev()` and `read()`, where it would make a scan quadratic.
Its invariant is that each cell is the predecessor of its successor, which
also makes the walk from the sentinel come back to it.

The exercises of chapter 3 of Morin on the doubly linked list require
operations that change the links of the existing cells, without making new
ones. Next, `checkSize()` counts the cells (exercise 3.5), and
`isPalindrome()` walks inward from the two ends, with $\lfloor n / 2 \rfloor$
comparisons in all (exercise 3.7). Next, `rotate( r )` moves the sentinel
before the cell of rank $n - r$; this cell becomes the first one, in
$O( 1 + \min\{ r , n - r \} )$ time and with no item moved (exercise 3.8,
[Algorithm 2.18](02-sequence.md#alg-2-18)). In the same way, `truncate( i )`
cuts the chain before the cell of rank $i$ and hangs it on the sentinel of a
new list, in $O( 1 + \min\{ i , n - i \} )$ time (exercise 3.9), and
`absorb( l )` links the chain of the other list after the last cell, in
$O( 1 )$ (exercise 3.10). Finally, `deal()` relinks the cells of odd rank, in
order, into a new list (exercise 3.11), and `reverse()` exchanges the two
links of each cell and of the sentinel (exercise 3.12).

<span id="alg-2-18"></span>**Algorithm 2.18.** The palindrome test, the
rotation, the truncation and the deal of `List` ([Morin 2013, exercises
3.7, 3.8, 3.9 and 3.11](Z-bibliography.md#ref-morin-cpp)); $s$ is the
sentinel.

--8<-- "generated/List-isPalindrome.pseudo.md"

--8<-- "generated/List-rotate.pseudo.md"

--8<-- "generated/List-truncate.pseudo.md"

--8<-- "generated/List-deal.pseudo.md"

In particular, the truncation shows how little a list with a sentinel
needs: the four links that cross the cut are rewritten, and the sizes are
those of the two parts. These operations move cells from a list to another,
which a list of cells from `new` does by relinking them, as Morin's does.
Between pooled lists, however, a cell cannot change pool, unless the whole
pool goes with it: `truncate()`, `deal()` and `takeFirst()` thus move the
items into cells of the other list, while `absorb()` and `merge()` take the
pool of the other list together with its cells (`ObjectPool::adopt()`,
[§5.5](05-memory.md#sec-5-5)), and `mergeSort()` and `reverse()` relink the
cells of one list in both cases.

<span id="lem-2-18"></span>**Lemma 2.18.** *On a `List` of $n$ items,
`isPalindrome()` makes at most $\lfloor n / 2 \rfloor$ comparisons,
`rotate( r )`, with $r$ reduced modulo $n$, takes $O( 1 + \min\{ r , n - r
\} )$ time with no item moved, and `reverse()` reverses the list in $O( n
)$ time. With cells from `new`, `truncate( i )` takes $O( 1 + \min\{ i , n
- i \} )$ time, `absorb()` and `takeFirst()` take $O( 1 )$, `deal()` $O( n
)$, and `merge()` $O( n_1 + n_2 )$ for lists of $n_1$ and $n_2$ items, and
no item is moved. With pooled cells, `truncate( i )` takes $O( 1 + n - i )$
amortized time, `deal()` $O( n )$ and `takeFirst()` $O( 1 )$ amortized, and `absorb()` and `merge()` take,
besides $O( 1 )$ and $O( n_1 + n_2 )$, the time of `ObjectPool::adopt()`,
which is linear in the number of the blocks of the pool that adopts. Both
merges are stable.*

*Proof.* `isPalindrome()` compares the cells of rank $j$ and $n - 1 - j$ for
$j < \lfloor n / 2 \rfloor$, walking inward from both ends. Then `rotate( r )`
unlinks the sentinel and links it again before the cell of rank $n - r$,
found by `position()` in $O( 1 + \min\{ r , n - r \} )$ time
([Theorem 2.5](02-sequence.md#thm-2-5)); this cell becomes the first one,
and the cell of rank $i$ gets the rank $( i + r ) \bmod n$. Last, `reverse()`
exchanges the two links of each of the $n + 1$ cells, which makes each
successor a predecessor, the sentinel's included. With cells from `new`,
`truncate( i )` finds the cell of rank $i$ and rewrites the four links that
cross the cut, `absorb()` and `takeFirst()` relink a constant number of
cells, and `deal()` and `merge()` make one step per cell. With pooled cells,
`truncate( i )` and `takeFirst()` move the $n - i$ items from the cell of
rank $i$ on, and the first item, into new cells of the other list, in $O( 1
)$ amortized time each ([Lemma
5.5](05-memory.md#lem-5-5)), and `deal()` moves the $\lfloor n / 2 \rfloor$
items of odd rank. Finally, a merge takes an item of the second list before
one of the first only if it is smaller, and therefore keeps the order of
the equal items, those of the first list first; the pooled `merge()` adopts
the two pools and inserts each cell of the second list before the first
cell of the first that is larger, in one walk of both. $\square$

In `ADS/sequence/List.h`, the declarations of `List` that a user calls are

```cpp
// declarations of <ADS/sequence/List.h>
template< class T , bool Pooled = true >
class List
{
 public:
 using Item = T;
 struct Cell;
 using Pos = Cell *;
 List( void );
 List( std::initializer_list< T > items );
 template< std::input_iterator I , std::sentinel_for< I > S >
 List( I first , S last );
 bool empty( void ) const;
 bool finished( Pos p ) const;
 Pos head( void ) const;
 Pos tail( void ) const;
 Pos next( Pos p ) const;
 Pos prev( Pos p ) const;
 Pos insert( Pos p , const T & v );
 Pos remove( Pos p );
 const T & read( Pos p ) const;
 void write( Pos p , const T & v );
 bool operator==( const List & o ) const;
 void push_back( const T & v );
 void push_front( const T & v );
 std::size_t size( void ) const;
 Pos position( std::size_t i ) const;
 const T & get( std::size_t i ) const;
 T set( std::size_t i , const T & x );
 void add( std::size_t i , const T & x );
 T erase( std::size_t i );
 void checkSize( void ) const;
 bool isPalindrome( void ) const;
 void rotate( std::size_t r );
 List truncate( std::size_t i );
 void absorb( List & l2 );
 List deal( void );
 void reverse( void );
 void takeFirst( List & l2 );
 template< class C = std::less<> >
 static List merge( List & l1 , List & l2 , C less = C() );
 template< class C = std::less<> >
 void mergeSort( C less = C() );
 };
```

### 2.10.1 Sorting a list {#sec-2-10-1}

The merge sort of [Morin 2013, exercise 3.13](Z-bibliography.md#ref-morin-cpp)
is made of three pieces. First, `takeFirst( l )` moves the first cell of $l$ to
the end of the list, as `push_back( l.erase( 0 ) )` but with no cell made;
`merge( l_1 , l_2 )` empties two sorted lists into a new sorted one by
`takeFirst()`; and `mergeSort()` truncates the list at its middle, sorts
the two halves and merges them ([Algorithm 2.19](02-sequence.md#alg-2-19)).

<span id="alg-2-19"></span>**Algorithm 2.19.** The merge and the merge
sort of a `List` ([Morin 2013, exercise 3.13](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/List-merge.pseudo.md"

--8<-- "generated/List-mergeSort.pseudo.md"

<span id="lem-2-19"></span>**Lemma 2.19.** *The merge sort of a list of $n$
items is stable and takes $O( n \log n )$ time and $O( \log n )$ space.*

*Proof.* The truncation at the middle and the merge of a list of $n$ items
take $O( n )$, and hence $T( n ) = 2 T( n / 2 ) + O( n )$. Also, the merge
takes the item of the first half among equal ones, which keeps their order,
and the recursion is $\lceil \log_2 n \rceil$ deep. $\square$

`List::mergeSort()` runs it on the chain of the cells, the halves being cut
by nil links and no list being made (`sortChain()`); thus, it relinks the
cells of a pooled list as well. The function `listSort()` of
`ADS/sequence/ListSort.h` is its engineered counterpart, stable as well,
which never copies or moves an item, the positions staying on their items
([Algorithm 2.20](02-sequence.md#alg-2-20)). It cuts the list into its
*runs*, i.e., the maximal stretches not decreasing and those strictly
decreasing, which it reverses (strictly, so that the sort stays stable).
Then it merges them bottom up with an array of at most 64 runs, the place
$k$ holding the merge of $2^k$ runs or nothing, as a binary counter; this is
the merge of `std::list::sort`, on the runs of the input instead of its
single cells. Note that the merges follow the successors only, and the
predecessors are set once at the end.

<span id="alg-2-20"></span>**Algorithm 2.20.** The natural merge sort of a
`List` by relinking ([Knuth 1998, §5.2.4](Z-bibliography.md#ref-knuth1998v3),
Algorithm L, on the runs of [McIlroy 1993](Z-bibliography.md#ref-mcilroy1993)).

--8<-- "generated/ListSort-listSort.pseudo.md"

<span id="lem-2-20"></span>**Lemma 2.20.** *`listSort()` sorts a list of
$n$ items and $r$ runs with $O( n \log r )$ comparisons, of which $n - 1 +
r'$ in the cut into runs, $r' \le r$ the runs of two items or more (hence
$n$ comparisons in all on a sorted or a reversed list of $n \ge 2$ items),
and with $O( 1 )$ space beyond the array of 64 runs.*

*Proof.* The cut into runs compares each item with the next one, and the
first two items of each run of two items or more twice, once to choose the
direction of the run and once in the loop that extends it; a sorted or a
reversed list is a single run, which no merge touches. A run takes part in a
merge each time the carry passes over its place, i.e., in at most $\lceil
\log_2 r \rceil + 1$ merges, each of which costs a comparison per item at
most. Finally, the array has room for $2^{64} - 1$ runs, more than any list
in memory. $\square$

On the numbers in their natural order (an arithmetic type and `std::less`),
`listSort()` sorts instead an array of pairs (key, cell) by the stable LSD
radix sort of chapter [12](12-sort.md#ch-12), and it relinks the cells in
one pass, in $O( n )$ time and space for keys of a fixed width ([Theorem
12.9](12-sort.md#thm-12-9)). A merge sort walks the cells $\log r$
times, and each walk may miss the cache (possibly) once per cell when the
cells are scattered, while the radix sort reads them twice.

In `ADS/sequence/ListSort.h`, the functions are declared as

```cpp
// declarations of <ADS/sequence/ListSort.h>
template< class T , bool P , class C = std::less<> >
void listSort( List< T , P > & L , C less = C() );
```

## 2.11 Lists with cursors {#sec-2-11}

A language with no pointers, or a program that wants its lists in one block
of memory, realizes linked lists with *cursors*. The cells of the lists are
the places of a vector, a `CursorSpace`, and a link is the index of the next
cell, 0 being nil (the place 0 is never used). Several lists can share one
space, each being the cursor of its first cell, and the free cells form one
more list in the same vector, the free list. Then `take()` removes its first
cell, and `give()` returns a cell to it, each in $O( 1 )$ time ([Algorithm
2.21](02-sequence.md#alg-2-21) and [Figure 2.15](02-sequence.md#fig-2-15)).
When the free list is empty the vector doubles (it starts with 8 cells,
or with the dimension given), and the new cells form the free list. The
cursors stay valid, since an index does not change when the vector is
moved, while a pointer would ([Aho et al. 1983,
§2.2](Z-bibliography.md#ref-aho1983); [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997)).

<span id="alg-2-21"></span>**Algorithm 2.21.** The free list of a
`CursorSpace`, and the insertion and the removal of `CursorList`.

--8<-- "generated/CursorList-CursorSpace-take.pseudo.md"

--8<-- "generated/CursorList-CursorSpace-give.pseudo.md"

--8<-- "generated/CursorList-insert.pseudo.md"

--8<-- "generated/CursorList-remove.pseudo.md"

<span id="lem-2-21"></span>**Lemma 2.21.** *A `CursorSpace` of dimension
$d$ that has held at most $u$ cells in use at once has at most $\max\{ d , 2
u \}$ cells, and `take()` and `give()` take $O( 1 )$ amortized time.*

*Proof.* `give()` pushes the cell on the free list, and `take()` pops it,
unless the free list is empty, i.e., all the $c$ cells are in use; it then
resizes the vector to $2 c$ cells (and the place 0), in $O( c )$ time. The
cells double only when all the $c$ cells are in use, i.e., when $c \le u$,
and hence they never pass $\max\{ d , 2 u \}$. With $\Phi = \max\{ 0 , 2 u_0 - c \}$, $u_0$ the cells in use, a `take()`
that does not resize raises $\Phi$ by 2 at most and a `give()` does not raise
it, while a resize finds $\Phi = c$ and leaves it at 2, which pays for its
$O( c )$ moves. $\square$

![Figure 2.15. A `CursorSpace` of 9 places, each with its item (upper row) and the cursor of the next place (lower row), the place 0 never used: before, the list $l$ = `a b c` in the places 1 to 3 and the free list 4, 5, 6, 7, 8 (in the faintest tint); after `push_front(x)` and `push_front(y)` on a second list $m$ of the same space and the removal of `b` from $l$, the new items (places 4 and 5) are in orange, the place 1, whose link now skips to 3, is in violet, and the place 2 is back at the head of the free list.](../figures/sequence-cursor.svg){#fig-2-15}

As in `SinglyList`, the position $\mathit{pos}_i$ of `CursorList` is the
cursor of the cell of $s_{i-1}$ for $i > 0$ and 0 for $i = 0$, so that
`insert()` and `remove()` find the link to change at hand ([Lemma
2.16](02-sequence.md#lem-2-16)). Also, $\mathit{pos}_n$ is the cursor of the
last cell (0 if the list is empty) and $\mathit{pos}_{-1}$ is $-1$, while
`tail()` and `prev()` scan the list. As for the space, its invariant is that
the free cells and the cells in use are all the places but 0, the free list
having no cycle; `used()` and `capacity()` count them. A list made with no
space has a space of its own, while the lists made on one space
(`CursorList( S )`) share it through a `std::shared_ptr`, and the space
lives as long as one of them does; `space()` gives it. The list also has
`push_front()`, `first()`, `get()` and an iterator.

In `ADS/sequence/CursorList.h`, the declarations of `CursorSpace` and
`CursorList` that a user calls are

```cpp
// declarations of <ADS/sequence/CursorList.h>
template< class T >
class CursorSpace
{
 public:
 struct Cell;
 explicit CursorSpace( long dim = 8 );
 Cell & operator[]( long c );
 const Cell & operator[]( long c ) const;
 long take( void );
 void give( long c );
 long capacity( void ) const;
 long used( void ) const;
 };
template< class T >
class CursorList
{
 public:
 using Item = T;
 using Pos = long;
 CursorList( void );
 explicit CursorList( std::shared_ptr< CursorSpace< T > > S );
 CursorList( std::initializer_list< T > items );
 bool empty( void ) const;
 bool finished( Pos p ) const;
 Pos head( void ) const;
 Pos tail( void ) const;
 Pos next( Pos p ) const;
 Pos prev( Pos p ) const;
 Pos insert( Pos p , const T & v );
 Pos remove( Pos p );
 const T & read( Pos p ) const;
 void write( Pos p , const T & v );
 bool operator==( const CursorList & o ) const;
 const CursorSpace< T > & space( void ) const;
 long first( void ) const;
 template< std::input_iterator I , std::sentinel_for< I > S >
 CursorList( I first , S last );
 void push_front( const T & v );
 };
```

## 2.12 The space-efficient list {#sec-2-12}

A doubly linked list takes two pointers per item beyond the item, and a
scan reads one cell per item. The `SEList` of [Morin 2013,
§3.3](Z-bibliography.md#ref-morin-cpp) (an *unrolled* list, [Shao et al.
1994](Z-bibliography.md#ref-shao1994)) keeps instead the items in a doubly
linked list of *blocks*, each a circular deque of at most $b + 1$ items, the
blocks between the first and the last holding at least $b - 1$ items each
(the code requires $b \ge 3$).

<span id="lem-2-22"></span>**Lemma 2.22.** *An `SEList` of $n$ items has
at most $n / ( b - 1 ) + 2$ blocks, i.e., $O( n / b )$ pointers, and it
takes $n + O( b + n / b )$ places; the block of
the rank $i$ is reached from the nearer end through $O( 1 + \min\{ i , n
- i \} / b )$ blocks.*

*Proof.* All the blocks but two hold $b - 1$ items at least. A block has
exactly $b + 1$ places, and hence a middle block wastes 2 places at most
and each end block $b$ at most, which with the $n / ( b - 1 ) + 2$ blocks
gives the places. Finally, the walk of `locate( i )` from the nearer end
passes over whole blocks of $b - 1$ items at least. $\square$

`capacity()` gives these places, $b + 1$ for each block in use.

An insertion at the rank $i$ in a full block looks at the next $b$ blocks at
most for one with room ([Algorithm 2.22](02-sequence.md#alg-2-22)). If one
is found, the last item of each block in between passes to the front of the
next one, in $O( 1 )$ per block. If the walk reaches the end, a block is
added there; if the $b$ blocks are all full, `spread()` makes $b + 1$ blocks
of $b$ items out of them, in $O( b^2 )$ time. A removal that leaves a middle
block with $b - 2$ items is the converse. It takes an item from the front of
each block up to the first one with more than $b - 1$ items, or up to the
end. If the $b$ blocks after it hold $b - 1$ items each, `gather()` refills
the block of the removal and the $b - 1$ blocks after it into $b - 1$
blocks, all full but the last, which has $b - 1$ items, and it drops the
block left empty. [Figure 2.16](02-sequence.md#fig-2-16) and [Figure
2.17](02-sequence.md#fig-2-17) show the three cases of each on the data of
Morin's figures, with $b = 3$.

<span id="alg-2-22"></span>**Algorithm 2.22.** The insertion, the removal,
the spread and the gather of `SEList` ([Morin 2013,
§3.3](Z-bibliography.md#ref-morin-cpp)); $|u|$ is the number of items of
the block $u$, and `locate( i )` gives the block of the rank $i$ and the
rank $j$ inside it.

--8<-- "generated/SEList-insert.pseudo.md"

--8<-- "generated/SEList-erase.pseudo.md"

--8<-- "generated/SEList-spread.pseudo.md"

--8<-- "generated/SEList-gather.pseudo.md"

![Figure 2.16. The three cases of `insert(1, x)` in an `SEList` with $b = 3$ (blocks of 4 places), on the data of Morin's figure (§3.3.3): (a) a block with room within $b$ blocks, (b) the end of the list reached and a new last block added, (c) $b$ full blocks spread into $b + 1$ blocks of $b$ items before the insertion; the new item is in orange, the items passed to another block in violet, the new block is marked by an orange bar, and the free places are in the faintest tint.](../figures/sequence-selist-add.svg){#fig-2-16}

![Figure 2.17. The three cases of `erase(1)` in an `SEList` with $b = 3$, on the data of Morin's figure (§3.3.4): (a) a block with more than $b - 1$ items within $b$ blocks, (b) the end of the list reached, (c) the block left with $b - 2$ items and the $b - 1$ blocks of $b - 1$ items after it gathered into $b - 1$ blocks, all full but the last, giving `a c d` and `e f`; the items taken from the next block are in violet, and the free places in the faintest tint.](../figures/sequence-selist-remove.svg){#fig-2-17}

<span id="lem-2-23"></span>**Lemma 2.23.** *Starting from the empty list,
$m$ insertions and removals take $O( b m )$ time in their calls of
`spread()` and `gather()`.*

*Proof.* Let a middle block be *tight* when it holds $b - 1$ or $b + 1$
items, and let $\Phi$ be $b$ times the number of tight blocks. An insertion
or a removal that neither spreads nor gathers changes the size of one block
by one, the blocks it passes an item through keeping their sizes; hence, it
makes one block tight at most, and it raises $\Phi$ by $b$ at most. A spread
is called on $b$ blocks of $b + 1$ items and leaves $b + 1$ blocks of $b$
items, before the insertion makes one of them tight; it lowers $\Phi$ by $b
( b - 1 )$ at least. A gather is called on $b$ blocks, the first of $b - 2$
items and the other $b - 1$ tight with $b - 1$ items, and it leaves $b - 1$
blocks, of which only the last, of $b - 1$ items, is tight; it lowers $\Phi$
by $b ( b - 2 )$. Since $b \ge 3$, both lower $\Phi$ by $b ( b - 2 ) \ge b^2
/ 3$ at least, which pays for their $O( b^2 )$ time. $\square$

<span id="thm-2-6"></span>**Theorem 2.6.** *`SEList` realizes the LIST;
`operator[]` takes $O( 1 + \min\{ i , n - i \} / b )$ time, `insert()` and
`erase()` at the rank $i$ take $O( b + \min\{ i , n - i \} / b )$ amortized
time, and the items take $n + O( b + n / b )$ places.*

*Proof.* [Lemma 2.22](02-sequence.md#lem-2-22) for the walk, the search of
$b$ blocks at most and the shifts of $O( b )$ items in a block, and
[Lemma 2.23](02-sequence.md#lem-2-23) for the spreads and the gathers.
$\square$

Unlike Morin's, the first and the last block of `SEList` may hold any
number of items from 1 to $b + 1$. `push_back()` adds a block when the last
one is full and `pop_back()` drops it when it is empty, and the same holds
at the front; thus, the four operations of a deque never spread nor gather.

<span id="lem-2-24"></span>**Lemma 2.24.** *The operations `push_front()`,
`push_back()`, `pop_front()` and `pop_back()` of `SEList` take $O( 1 )$
time, independent of $b$; with Morin's invariant, where only the last block
is free, `push_back()` and `pop_back()` take $O( 1 )$ amortized time as
well.*

*Proof.* With the relaxed ends, an operation at an end touches the end
block only, and adds or drops it when it is full or empty. One block
emptied is kept for the next one needed, and therefore a queue that crosses
the boundary of a block back and forth allocates nothing. With Morin's
invariant, a push on a full last block adds a block after it, and a pop
that leaves the last block with fewer than $b - 1$ items borrows nothing,
since the last block is free to be small; a sequence of pushes and pops
thus never spreads nor gathers. However, a removal at the front of Morin's
list leaves the first block, a middle one for his invariant, with $b - 2$
items, and it shifts an item out of each of the blocks that follow, up to
$b$ of them; the deque needs the relaxation at the front too. $\square$

[Lemma 2.24](02-sequence.md#lem-2-24) answers [Morin 2013, exercises 3.17
and 3.18](Z-bibliography.md#ref-morin-cpp), and the bound on the space loses
the $2 b$ places of the two ends at most. A block is one allocation, its
header followed by its $b + 1$ places, which are raw memory. A place is
reduced modulo $b + 1$ by a comparison and a subtraction, since the sum of
the offset of the first item and a rank is below $2 ( b + 1 )$, where
Morin's `BDeque`, an `ArrayDeque`, divides by $b + 1$; his blocks also
build $b + 1$ items by default each. Furthermore, the class has the
iterators, `operator[]`, `at()`, `front()`, `back()`, `size()` and
`clear()`, and its invariant (the bound on the items of each block, none
empty, and the count of $n$) is an $O( n / b )$ check.

In `ADS/sequence/SEList.h`, the declarations of `SEList` that a user calls
are

```cpp
// declarations of <ADS/sequence/SEList.h>
template< class T >
class SEList
{
 public:
 explicit SEList( std::size_t b = 63 );
 SEList( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 std::size_t block( void ) const;
 std::size_t blocks( void ) const;
 std::size_t capacity( void ) const;
 T & operator[]( std::size_t i );
 const T & operator[]( std::size_t i ) const;
 const T & front( void ) const;
 const T & back( void ) const;
 void insert( std::size_t i , const T & x );
 void erase( std::size_t i );
 void push_front( const T & x );
 void push_back( const T & x );
 void pop_front( void );
 void pop_back( void );
 void clear( void );
 bool operator==( const SEList & o ) const;
 };
```

## 2.13 The XOR list {#sec-2-13}

The discussion of chapter 3 of [Morin 2013](Z-bibliography.md#ref-morin-cpp)
mentions the XOR list ([Sinha 2004](Z-bibliography.md#ref-sinha2004)),
whose cell keeps one word of links instead of two, i.e., the exclusive or
$\mathit{prev} \oplus \mathit{next}$ of the addresses of its two neighbors
(nil at the ends being 0). A walk that knows the cell it comes from finds
the next one by one exclusive or, in either direction, since

$$
( \mathit{prev} \oplus \mathit{next} ) \oplus \mathit{prev} =
\mathit{next} , \qquad
( \mathit{prev} \oplus \mathit{next} ) \oplus \mathit{next} =
\mathit{prev} , \tag{2.8}
$$

$\oplus$ being associative and commutative with $x \oplus x = 0$. `XorList`
keeps the first and the last cell, so that the four operations of a deque
take $O( 1 )$ time ([Algorithm 2.23](02-sequence.md#alg-2-23)). Also, the
list reversed is the same list read from its other end, which `reverse()`
obtains by exchanging the first and the last cell, in $O( 1 )$ time ([Figure
2.18](02-sequence.md#fig-2-18)). The price is that a cell alone is no
position: an iterator holds the cell and the one before it, and no cell can
be removed from the middle given only its address. Its cells come from an
`ObjectPool`, as those of `LinkedQueue`, and the addresses are turned into
integers of type `std::uintptr_t` and back. C++ defines this for the
addresses of live objects, while a language with garbage collection, such as
Java, cannot do it, since a collector does not see the addresses hidden in
the words.

<span id="alg-2-23"></span>**Algorithm 2.23.** The insertion and the removal
at the front of `XorList` ([Sinha 2004](Z-bibliography.md#ref-sinha2004)).

--8<-- "generated/XorList-pushFront.pseudo.md"

--8<-- "generated/XorList-popFront.pseudo.md"

![Figure 2.18. An `XorList` of four cells `a b c d`, each with its item and one word of links, the exclusive or of the addresses of its neighbors (0 at the ends), and the pointers to the first and the last cell; after `reverse()` the cells are unchanged, and only the two pointers, in violet, are exchanged.](../figures/sequence-xor.svg){#fig-2-18}

Identity (2.8) also swaps two integers with no third ([Morin 2013, exercise
3.19](Z-bibliography.md#ref-morin-cpp)), which `xorSwap()` does by three
exclusive or ([Algorithm 2.24](02-sequence.md#alg-2-24)).

<span id="alg-2-24"></span>**Algorithm 2.24.** The swap by exclusive or
([Morin 2013, exercise 3.19](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/XorList-xorSwap.pseudo.md"

<span id="lem-2-25"></span>**Lemma 2.25.** *On an `XorList` of $n$ items
the four operations at the ends and `reverse()` take $O( 1 )$ time.
`xorSwap( a , b )` exchanges the values of $a$ and $b$ when they are two
different objects, and sets the object to 0 when they are the same.*

*Proof.* The word of a cell at an end is the address of its only neighbor,
since the other one is 0. An insertion at the front thus writes the address
of the old first cell into the new one, and it adds the address of the new
cell to the word of the old first one by an exclusive or. A removal at the
front reads the address of the second cell in the word of the first one,
and by (2.8) it removes the address of the first from the word of the
second; the back is the mirror. Since the word of each cell is symmetric in
its two neighbors, the walk from the last cell reads the list reversed,
and `reverse()` obtains it by exchanging the first and the last cell. As for the swap, with $a = x$ and $b = y$ the three steps leave $a = x
\oplus y$, then $b = ( x \oplus y ) \oplus y = x$, and then $a = ( x \oplus
y ) \oplus x = y$. If $a$ and $b$ are one object, instead, the first step
sets it to $x \oplus x = 0$, and the other two keep it 0. $\square$

In `ADS/sequence/XorList.h`, the declarations of the functions and of
`XorList` that a user calls are

```cpp
// declarations of <ADS/sequence/XorList.h>
template< class T >
class XorList
{
 public:
 XorList( void );
 XorList( std::initializer_list< T > l );
 bool empty( void ) const;
 std::size_t size( void ) const;
 const T & front( void ) const;
 const T & back( void ) const;
 void push_front( const T & x );
 void push_back( const T & x );
 void pop_front( void );
 void pop_back( void );
 void reverse( void );
 void clear( void );
 bool operator==( const XorList & o ) const;
 };
template< std::unsigned_integral U >
constexpr void xorSwap( U & a , U & b );
```

## 2.14 The skip list by rank {#sec-2-14}

The structures of the previous sections either reach a rank in $O( 1 )$ time
and pay a shift for an insertion in the middle, or insert in $O( 1 )$ time
at a cell they hold and pay a walk to reach a rank. The `SkiplistList` of
[Morin 2013, §4.3](Z-bibliography.md#ref-morin-cpp) does both in $O( \log n
)$ expected time, on the skip list of [Pugh
1990](Z-bibliography.md#ref-pugh1990) ([§7.9](07-dictionary.md#sec-7-9))
whose links carry their *width*, i.e., the number of items they jump over
([Pugh 1990b](Z-bibliography.md#ref-pugh1990b)). In `SkipListSequence` the
items are in the order of the sequence on the bottom level, and each node is
in the levels 0 to $\ell - 1$ with probability $2^{-\ell}$. A link from the
node of index $j$ (the head having the index $-1$) to the node of index $k$
has the width $k - j$, a link to nil reaching the index $n$ ([Figure
2.19](02-sequence.md#fig-2-19), on the data of Morin's figure). Note that
the links to nil keep their widths, and hence no level needs a special case,
and each level is a sublist of the level below.

![Figure 2.19. The skip list of the items 0 to 6 ($n = 7$) of Morin's figure of the widths (§4.3), with the head in the levels $L_0$ to $L_5$ and each link labeled with its width, the links to nil included, so that the widths of every level sum to $n + 1 = 8$; the number under each node is its index.](../figures/sequence-skip-widths.svg){#fig-2-19}

<span id="lem-2-26"></span>**Lemma 2.26.** *If the link of each node $x$ on
the level $l$ has the width $\mathrm{index}( x.\mathit{next}[ l ] ) -
\mathrm{index}( x )$, then the widths of each level sum to $n + 1$, and the
search of the index $i$ that sums the widths and goes right while the sum
stays at most $i$ ends at the node of index $i$, in $O( \log n )$ expected
time.*

*Proof.* On each level the widths telescope from the index $-1$ of the head to
the index $n$ of nil. Along the search the sum of the widths followed is the
index of the node reached. Hence the search is that of a skip list whose
keys are the indices, and [Lemma 7.30](07-dictionary.md#lem-7-30) bounds its
expected length by $2 \log_2 n + O( 1 )$. $\square$

An insertion at the index $i$ draws the levels $\ell$ of the new node, and it
follows the search path of the predecessor, the index $i - 1$. On a level
$l \ge \ell$ the link that passes over the new node grows by 1, and on a level
$l < \ell$ the link that the node splits, from $x$ of index $j$ and of width
$w$, becomes two links of widths $i - j$ and $w + 1 - ( i - j )$ ([Algorithm
2.25](02-sequence.md#alg-2-25) and [Figure 2.20](02-sequence.md#fig-2-20)).
The removal is the reverse: the link that reaches the node is replaced by the
one that leaves it, with the sum of their widths less 1, and the links that
pass over it lose 1. [Figure 2.21](02-sequence.md#fig-2-21) and [Figure
2.22](02-sequence.md#fig-2-22) give the two operations of Morin's figures.
[Figure 2.23](02-sequence.md#fig-2-23) and [Figure
2.24](02-sequence.md#fig-2-24) solve his exercises 4.5 and 4.4, which require
to illustrate `add( 3 , x )`, with a node of height 4 (in the levels 0 to 4),
and `remove( 2 )` on the list of [Figure 2.19](02-sequence.md#fig-2-19).

<span id="alg-2-25"></span>**Algorithm 2.25.** The access, the insertion
and the removal by index of `SkipListSequence` ([Morin 2013,
§4.3](Z-bibliography.md#ref-morin-cpp)); $x.\mathit{next}[ l ]$ and $x.w[ l
]$ are the successor of $x$ on the level $l$ and the width of the link, $j$
the index of $x$, and $L$ the number of levels.

--8<-- "generated/SkipListSequence-get.pseudo.md"

--8<-- "generated/SkipListSequence-add.pseudo.md"

--8<-- "generated/SkipListSequence-remove.pseudo.md"

![Figure 2.20. The splice of a new node $w$ of index $i$ into a level, between the node $u$ of index $j$ and the node $z$ that the link of $u$, of width $\ell$, reaches: after the splice the two links, of widths $i - j$ and $\ell + 1 - ( i - j )$, and $w$ itself, are in orange.](../figures/sequence-skip-splice.svg){#fig-2-20}

![Figure 2.21. The skip list of [Figure 2.19](02-sequence.md#fig-2-19) after `add(4,x)` with `x` in the levels $L_0$ to $L_3$ ($n = 8$), Morin's figure of the insertion: the boxes of the search path (the head in $L_5$ to $L_3$, the node 2 in $L_3$, the node 3 in $L_2$ to $L_0$) and the links it follows (head to 2 in $L_3$, 2 to 3 in $L_2$) are in violet, as are the links from the head to 4 in $L_4$ and $L_5$, whose width grows to 6; the new node and its links are in orange.](../figures/sequence-skip-add.svg){#fig-2-21}

![Figure 2.22. The skip list of [Figure 2.19](02-sequence.md#fig-2-19) after `remove(3)` ($n = 6$), Morin's figure of the removal, the column of the node removed left empty: the boxes of the search path (the head in $L_5$ to $L_3$, the node 2 in $L_3$ to $L_0$) and the link from the head to 2 that it follows in $L_3$ are in violet, as are the links whose width changes, from 2 to 4 (which replaces 2, 3, 4 in $L_0$ to $L_3$) and from the head to 4 in $L_4$ and $L_5$, now of width 4.](../figures/sequence-skip-remove.svg){#fig-2-22}

![Figure 2.23. The skip list of [Figure 2.19](02-sequence.md#fig-2-19) after `add(3,x)` with `x` in the levels $L_0$ to $L_4$ ($n = 8$), the solution of Morin's exercise 4.5: the boxes of the search path (the head in $L_5$ to $L_3$, the node 2 in $L_3$ to $L_0$), the link from the head to 2 that it follows in $L_3$ and the link from the head to 4 in $L_5$, whose width grows to 6, are in violet; the new node and its links (from the node 2 in $L_0$ to $L_3$ and from the head in $L_4$) are in orange.](../figures/sequence-skip-ex-add.svg){#fig-2-23}

![Figure 2.24. The skip list of [Figure 2.19](02-sequence.md#fig-2-19) after `remove(2)` ($n = 6$), the solution of Morin's exercise 4.4, the column of the node removed left empty: the boxes of the search path (the head in $L_5$ to $L_0$, the nodes 0 and 1 in $L_0$) and the links it follows in $L_0$ are in violet, as are the links that replace those through the node 2 (1 to 3 in $L_0$, the head to 3 in $L_1$ and $L_2$, the head to 4 in $L_3$) and the links from the head to 4 in $L_4$ and $L_5$, whose width falls to 4.](../figures/sequence-skip-ex-remove.svg){#fig-2-24}

`truncate( i )` cuts the list after the first $i$ items and returns the rest
as a list of its own, and `absorb( l )` appends the items of another list,
which it leaves empty ([Morin 2013, exercises 4.11 and
4.12](Z-bibliography.md#ref-morin-cpp)). Both relink one node per level. In
particular, the truncation cuts, on each level, the link that crosses the
index $i$, and it gives its end to the head of the new list with the width
reduced by $i$. The absorption walks to the last node of each level, which
is the search of the index $n - 1$, and it links that node to the first node
of the same level of the other list ([Algorithm
2.26](02-sequence.md#alg-2-26)).

<span id="alg-2-26"></span>**Algorithm 2.26.** The truncation and the
absorption of `SkipListSequence` ([Morin 2013, exercises 4.11 and
4.12](Z-bibliography.md#ref-morin-cpp)); $r$ is the new list and $s$ the
list absorbed, of $m$ items and $L_s$ levels.

--8<-- "generated/SkipListSequence-truncate.pseudo.md"

--8<-- "generated/SkipListSequence-absorb.pseudo.md"

<span id="thm-2-7"></span>**Theorem 2.7.** *`SkipListSequence` realizes the
LIST with `get()`, `set()`, `add()`, `remove()`, `truncate()` and `absorb()`
in $O( \log n )$ expected time.*

*Proof.* Each operation follows one search path, of $O( \log n )$ expected
length by [Lemma 2.26](02-sequence.md#lem-2-26), and changes $O( 1 )$
links and widths per level, on $O( \log n )$ expected levels
([Lemma 7.33](07-dictionary.md#lem-7-33)). An insertion and a removal keep
the sums of [Lemma 2.26](02-sequence.md#lem-2-26), since the width added or
removed on a level is the one item added or removed below it. $\square$

We remark that the expectations are over the levels of the nodes, drawn
independently of each other and of the operations. This hypothesis fails
for `absorb()` of a list made with the same seed, whose levels are the same
sequence drawn again: absorbing in turn $n$ lists of one item, all made with
the default seed, gives $n$ nodes of the same height, i.e., $\Theta( n )$
time for a search.

A node is one block of memory, i.e., its item and the tower of its links,
each link a pointer and its width side by side. A step of a search thus
reads one line of cache, where Morin's node, made of three blocks (the
node, the array of the successors and that of the widths), may need three.
Also, the widths are words, not `int` (a list may then pass $2^{31}$ items), and
the levels come from a seeded `SplitMix64` instead of `rand()`. By default
each list takes a seed of its own, $\mathrm{splitmix64}( c )$ for the number
$c$ of the lists made before it (`detail::instanceSeed()`), since the lists of
one item made with one seed would all have the same level, and their
absorption into one list would leave it with two levels, i.e., a search of
$\Theta( n )$ steps; a seed given makes the levels reproducible. The class
also has `insert()`, `erase()`, `operator[]`, `push_back()`, `size()` and
the iterators; its invariant, an $O( n \log n )$ check at the level 3 of
the contracts, is that the widths of each level sum to $n + 1$ and that
each level is a sublist of the level below.

In `ADS/sequence/SkipListSequence.h`, the declarations of
`SkipListSequence` that a user calls are

```cpp
// declarations of <ADS/sequence/SkipListSequence.h>
template< class T >
class SkipListSequence
{
 public:
 using Item = T;
 explicit SkipListSequence( std::uint64_t seed = detail::instanceSeed() );
 std::size_t size( void ) const;
 bool empty( void ) const;
 int levels( void ) const;
 const T & get( std::size_t i ) const;
 void set( std::size_t i , const T & v );
 void add( std::size_t i , const T & v );
 const T & operator[]( std::size_t i ) const;
 T & operator[]( std::size_t i );
 void insert( std::size_t i , const T & v );
 void erase( std::size_t i );
 void push_back( const T & v );
 T remove( std::size_t i );
 SkipListSequence truncate( std::size_t i );
 void absorb( SkipListSequence & s );
 };
```

## 2.15 The treap by rank and the order of a list {#sec-2-15}

The `TreapList` of [Morin 2013, exercise
7.11](Z-bibliography.md#ref-morin-cpp) reaches a rank in $O( \log n )$
expected time on a treap ([Seidel and Aragon
1996](Z-bibliography.md#ref-seidel1996); the treaps on the keys are in
chapter [8](08-tree.md#ch-8)) whose symmetric order is the order of the
sequence, and whose nodes keep the size of their subtree. Thus, the index of
a node is the size of the left subtrees passed on the right along its path,
and the keys, i.e., the indices, are never stored (an *implicit* treap). An
insertion at $i$ goes down by the sizes to the leaf between the items $i -
1$ and $i$, and it rotates the new node up while its priority beats its
parent's; a removal rotates the node down until it has one child at most
([Algorithm 2.27](02-sequence.md#alg-2-27)). Also, `truncate( i )` splits
the treap at the index $i$, and `absorb( s )` joins the treap of another
sequence at the end, along two spines, as `SkipListSequence` does on a skip
list ([Figure 2.25](02-sequence.md#fig-2-25)).

<span id="alg-2-27"></span>**Algorithm 2.27.** The access, the insertion,
the split and the join of `TreapList`; $s( t )$ is the size of the subtree
$t$, 0 for nil.

--8<-- "generated/TreapList-get.pseudo.md"

--8<-- "generated/TreapList-add.pseudo.md"

--8<-- "generated/TreapList-split.pseudo.md"

--8<-- "generated/TreapList-join.pseudo.md"

<span id="lem-2-27"></span>**Lemma 2.27.** *Let the priorities of the
nodes of a `TreapList` of $n$ items be distinct, independent and identically
distributed, and independent of the operations. Then the treap is a random
binary search tree on the indices, and `get()`, `set()`, `add()`,
`remove()`, `truncate()` and `absorb()` take $O( \log n )$ expected time.*

*Proof.* With each item replaced by its index, the treap is one on the keys
$0 , \ldots , n - 1$, and it is a random binary search tree by [Lemma
8.34](08-tree.md#lem-8-34); by [Lemma 8.35](08-tree.md#lem-8-35), the path
to a node, and the path of a search that ends between two nodes, have
expected length at most $2 \ln n + O( 1 )$. Both `get()` and `set()` follow
the path to the node, `add()` follows the path to the leaf of the new node and
rotates it back up along it, and `remove()` follows the path to the node and
makes fewer than 2 rotations in expectation ([Lemma
8.37](08-tree.md#lem-8-37)). Last, `truncate( i )` splits along the path of a
search between the indices $i - 1$ and $i$, and `absorb()` joins along the
right spine of the first treap and the left spine of the second, the paths
of a search beyond all the indices of each. $\square$

As for the skip list, the hypothesis fails for `absorb()` of a list made
with the same seed, whose priorities repeat those of this one: absorbing in
turn $n$ lists of one item, all made with the default seed, gives a treap
that is a path of $n$ nodes.

![Figure 2.25. `truncate(3)` on the implicit treap of the items `a` to `g` in symmetric order, with the size of each subtree under its vertex: on the left, the split at the index 3 follows the path `d`, `b`, `c` (in violet); on the right are the two treaps it leaves, rooted at `b` (the ranks 0 to 2) and at `d` (the rest, its left child now nil, dashed), `d` and `b`, whose sizes are rewritten, in violet.](../figures/sequence-treap-split.svg){#fig-2-25}

Morin gives no code for it. In ours, a node holds the item, a priority, a
size and two links in one block of memory, and a search reads one node per
level of the tree, i.e., $O( \log n )$ expected nodes, each on one line of
cache, as the search of the skip list does. Its priorities come from a
seeded `SplitMix64`, by default with a seed of its own per sequence, as for
`SkipListSequence` (with one seed for all, the joins of the treaps of one
item would make a path of depth $n$), and the invariant, an $O( n )$ check at
the level 3, is
the heap order of the priorities and the sizes. The class has the operations
of `SkipListSequence` (`get()`, `set()`, `add()`, `remove()`, `read()`,
`insert()`, `erase()`, `push_back()`, `truncate()`, `absorb()` and
`size()`).

In `ADS/sequence/TreapList.h`, the declarations of `TreapList` that a user
calls are

```cpp
// declarations of <ADS/sequence/TreapList.h>
template< class T >
class TreapList
{
 public:
 using Item = T;
 explicit TreapList( std::uint64_t seed = detail::instanceSeed() );
 std::size_t size( void ) const;
 bool empty( void ) const;
 const T & get( std::size_t i ) const;
 void set( std::size_t i , const T & v );
 void add( std::size_t i , const T & v );
 const T & operator[]( std::size_t i ) const;
 T & operator[]( std::size_t i );
 void insert( std::size_t i , const T & v );
 void erase( std::size_t i );
 void push_back( const T & v );
 T remove( std::size_t i );
 TreapList truncate( std::size_t i );
 void absorb( TreapList & s );
 struct Shape;
 };
```

### 2.15.1 Order maintenance {#sec-2-15-1}

The *order maintenance* problem asks for a list of elements with three
operations: `addAfter( e , v )`, which adds a new element right after $e$
(at the front if $e$ is nil), `remove( e )`, and `testBefore( a , b )`,
whether $a$ comes before $b$ in the list. It is the `Sequence` of [Morin
2013, exercise 8.11](Z-bibliography.md#ref-morin-cpp), and the solution
that the exercise has in mind labels each node of a scapegoat tree by its
path, so that the order is a comparison of two words. The root has the
label $2^{63}$, and the children of a node of label $\ell$, whose lowest
bit set is $2^j$, the labels $\ell \pm 2^{j-1}$, which keeps the labels in
symmetric order for a tree of height up to 63. A rebuild of a subtree of
$s$ nodes relabels it within the $O( s )$ time of the rebuild ([Lemma
8.53](08-tree.md#lem-8-53)), while a rotation would change the labels of a
whole subtree (which is why the exercise sits in the chapter of the partial
rebuilding); an insertion thus costs the $O( \log n )$ amortized time of
the scapegoat tree ([Lemma 8.58](08-tree.md#lem-8-58)).

`OrderMaintenanceList` does better, i.e., $O( 1 )$ amortized time per
insertion and $O( 1 )$ in the worst case for the other two operations, by
the list labeling of [Bender et al. 2002](Z-bibliography.md#ref-bender2002)
on two levels ([Dietz and Sleator 1987](Z-bibliography.md#ref-dietz1987)
gave the first algorithms with these bounds). In it, the elements are cut
into runs of consecutive elements, the *buckets*, of at most $B = 64$, and
each element has a local label below $2^{63}$, which increases along its
bucket. In turn, the buckets have labels below $2^{63}$ too, which increase
along the list of the buckets, and `testBefore()` compares the labels of the
two buckets, or the local labels within one bucket ([Figure
2.26](02-sequence.md#fig-2-26)). A new element takes the middle of the gap
between its neighbors in the bucket, and the bucket is labeled again with
equal gaps, in $O( B )$ time, when that gap is empty; a bucket of $B + 1$
elements splits into two halves ([Algorithm 2.28](02-sequence.md#alg-2-28)).
A new bucket takes the middle of the gap after the one it follows, if the
gap is not empty. Otherwise, for $i = 1 , 2 , \ldots$, the range $R_i$ of
the $2^i$ labels aligned on $2^i$ that holds the label of its predecessor
$b$ is *overflowing* if its buckets, the new one included, are more than $(
2 / T )^i$, i.e., if its density exceeds $T^{-i}$, with $T = 1.4$. The first
range that does not overflow is labeled again with equal gaps, of at least
$2^i / ( 2 / T )^i = T^i \ge 1$.

<span id="alg-2-28"></span>**Algorithm 2.28.** The insertion of
`OrderMaintenanceList`, and that of a bucket by the densities ([Bender et
al. 2002](Z-bibliography.md#ref-bender2002)); $\ell( u )$ is the local label
of $u$ and $L( b )$ the label of the bucket $b$.

--8<-- "generated/OrderMaintenanceList-addAfter.pseudo.md"

--8<-- "generated/OrderMaintenanceList-addBucketAfter.pseudo.md"

![Figure 2.26. The two levels of labels of `OrderMaintenanceList`, on a universe of 16 labels of buckets and 16 local labels (the code uses $2^{63}$ of each): the buckets 2, 7 and 12, with the label of each bucket above it and the local label of each element below. After `addAfter(b,x)` the new element `x` (in orange) takes the local label 6, the middle of the gap between `b` (4) and `c` (8), which are in violet; `testBefore(x,e)` compares only the labels 2 and 7 of the two buckets (in violet), and is true.](../figures/sequence-order.svg){#fig-2-26}

<span id="lem-2-28"></span>**Lemma 2.28.** *The insertion of a bucket writes
at most $1 + 63 \cdot 2 / ( T - 1 ) + 1 = 317$ labels of buckets amortized,
with $T = 1.4$, whatever the removals among the insertions.*

*Proof.* A relabeling of $R_i$ writes $m \le ( 2 / T )^i$ labels, and it
happens because $R_{i-1}$ overflows, with more than $a = ( 2 / T )^{i-1}$
buckets. The last time that a range containing $R_{i-1}$ was labeled again
the gaps were equal, and the half $R_{i-1}$ of a range of level at least
$i$ received at most half of its buckets plus one, i.e., at most $a / T + 1$.
Hence at least $k_i = \max\{ 1 , \lfloor a \rfloor - \lfloor a / T \rfloor
\}$ insertions have gone into $R_{i-1}$ since then (a removal only lowers
its count, and the insertion at hand is one of them), and we charge the
relabeling to them. An insertion lies in one range of each of the 63
levels, and it is charged at most once per level, since the relabeling of
$R_i$ empties the account of each range inside it; it thus pays at most
$c_i = \lfloor ( 2 / T )^i \rfloor / k_i$ labels to the level $i$. The
ratio $c_i$ tends to $( 2 / T ) / ( 1 - 1 / T ) = 2 / ( T - 1 ) = 5$, and
it is 8 at most (at $i = 6$); summing it over $i = 1 , \ldots , 63$ gives
less than 305, which, with the label of the new bucket, is below $1 + 63
\cdot 5 + 1 = 317$. $\square$

Hence, the universe of $2^{63}$ labels holds $( 2 / T )^{63} \approx 5.7
\cdot 10^9$ buckets, the most for which $R_{63}$ does not overflow, which is
a precondition of the contracts.

<span id="lem-2-29"></span>**Lemma 2.29.** *An insertion into an
`OrderMaintenanceList` writes fewer than 15 labels amortized, a constant
independent of $n$; a removal and `testBefore()` take $O( 1 )$ time in the
worst case.*

*Proof.* A bucket is labeled again with equal gaps of more than $2^{56}$,
and its gap for a new element is empty only after 56 halvings of one gap,
i.e., after 56 insertions into the bucket since its last relabeling (the
removals only widen the gaps). It splits into halves of 32 and 33 elements
when it reaches $B + 1 = 65$, which needs 32 insertions into it since it
was made, and the split writes 65 local labels and the labels of
[Lemma 2.28](02-sequence.md#lem-2-28) for the new bucket. Hence an
insertion writes, amortized, its own label, at most $64 / 56$ for the
relabelings of its bucket and $( 65 + 317 ) / 32$ for the splits, fewer
than 15 labels in all; the bound does not depend on $n$, since $\log_2 m <
63 < B$ for the $m$ buckets. A removal unlinks its element, and its bucket
if it empties, with no relabeling. $\square$

`relabelWork()` counts the labels written. The test of the exercise checks,
in four patterns of insertions (at the front, at the end, right after the
first element, and at random places with removals), that their mean per
insertion is at most 8 after $2^{17}$ insertions, and at most one more than
after $2^{12}$. `first()`, `next()`, `value()` and `size()` read the list,
and an element (of the type `Element`, a pointer to its node) stays valid
until it is removed. Testing the order takes two comparisons of words.

In `ADS/sequence/OrderMaintenanceList.h`, the declarations of
`OrderMaintenanceList` that a user calls are the following, where `Element`
is a constant handle of an element:

```cpp
// declarations of <ADS/sequence/OrderMaintenanceList.h>
template< class T >
class OrderMaintenanceList
{
 public:
 using Item = T;
 static constexpr std::size_t B = 64;
 OrderMaintenanceList( void );
 std::size_t size( void ) const;
 Element addAfter( Element e , const T & v );
 void remove( Element e );
 bool testBefore( Element a , Element b ) const;
 static const T & value( Element e );
 Element first( void ) const;
 Element next( Element e ) const;
 std::pair< std::size_t , std::size_t > relabelWork( void ) const;
 };
```

## 2.16 Algorithms on sequences {#sec-2-16}

The functions of `ADS/sequence/SequenceAlgorithms.h` use only the operations
of the SEQUENCE, and therefore they run on any realization (a `List` and an
`ArraySequence` alike), with the cost of the scan (2.1), i.e., $\Theta( n )$
calls of `next()` and `read()`. First, `search()` compares the item with the
items in order and returns its place counted from 0, or $-( n + 1 )$ when it
misses. On a sorted sequence `searchOrdered()` stops at the first larger
item and returns $-c$ when it misses, $c$ the items compared ($-1$ on the
empty sequence); however, the comparisons stay $\Theta( n )$ in the worst
case, since the positions give no access to the middle (the binary search of
chapter [6](06-search.md#ch-6) needs the access by index). Then `lowerBound()`
returns the position of the first item not smaller than $v$ in a sorted
sequence, i.e., where $v$ goes to keep it sorted, together with whether $v$ is
there, so that a search and an insertion share the same scan. Also, `sublist()`
copies the items from a position to another one (the empty sequence being a
sublist of any sequence), and `order()` determines, in one scan, whether a
sequence is empty, constant, increasing, decreasing, nondecreasing,
nonincreasing or none of these (the values of the enumeration `Order`).

<span id="alg-2-29"></span>**Algorithm 2.29.** The searches and the order
of a sequence, through its positions.

--8<-- "generated/SequenceAlgorithms-search.pseudo.md"

--8<-- "generated/SequenceAlgorithms-searchOrdered.pseudo.md"

--8<-- "generated/SequenceAlgorithms-lowerBound.pseudo.md"

--8<-- "generated/SequenceAlgorithms-sublist.pseudo.md"

--8<-- "generated/SequenceAlgorithms-order.pseudo.md"

`mergeSorted()`, the merge of two sorted sequences of lengths $n_1$ and
$n_2$, appends at each step the smaller of their first items not yet
taken, the one of the first sequence among equal ones, and it keeps the
repeated items. `removeDuplicates()` keeps the first copy of each item, by a
scan of the rest of the sequence for each position, and `bubbleSort()` swaps
adjacent items out of order by `read()` and `write()` until a pass has no
swap.

<span id="alg-2-30"></span>**Algorithm 2.30.** The merge, the removal of
the duplicates and the two sorts of a sequence ([Montresor 2025,
§12.2](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/SequenceAlgorithms-mergeSorted.pseudo.md"

--8<-- "generated/SequenceAlgorithms-removeDuplicates.pseudo.md"

--8<-- "generated/SequenceAlgorithms-bubbleSort.pseudo.md"

--8<-- "generated/SequenceAlgorithms-naturalMergeSort.pseudo.md"

<span id="lem-2-30"></span>**Lemma 2.30.** *On sequences of $n$, $n_1$ and
$n_2$ items, `search()` and `lowerBound()` make at most $n$ comparisons,
`searchOrdered()` at most $2 n$ and `order()` at most $2 ( n - 1 )$. The
merge `mergeSorted()` is stable, and it makes at most $n_1 + n_2 - 1$
comparisons (none if a sequence is empty). Also, `removeDuplicates()` makes at
most $n ( n - 1 ) / 2$ comparisons and $n - 1$ calls of `remove()`, and
`bubbleSort()` makes at most $n$ passes of $n - 1$ comparisons each, and as
many swaps as the inversions of the sequence, at most $n ( n - 1 ) / 2$.*

*Proof.* `search()` and `lowerBound()` compare each item once, and
`searchOrdered()` and `order()` twice at most (an equality and an order, or
two orders). Each step of the merge compares two items and appends one, and
the last item is appended with no comparison, after one sequence is
exhausted; an item of the second sequence goes first only if it is smaller,
which keeps the order of the equal items. Finally, `removeDuplicates()`
compares the item at each position with the items after it that are left, and
it removes an item at most once. As for `bubbleSort()`, a pass carries the
largest item of the places it has not yet sorted to the last of them, since
each comparison that meets it moves it one place on; hence after $k$ passes the
$k$ largest items are in their places, and the pass that follows the $( n -
1 )$-th has no swap. A swap of two adjacent items out of order removes one
inversion and creates none, and the sequence with no inversion is sorted.
$\square$

`naturalMergeSort()` distributes the runs of the sequence alternately to
two sequences and merges them in pairs back, until a single run is left
([Table 2.3](02-sequence.md#tab-2-3)); its optional argument returns the
number of passes. A sequence of $r$ runs has $r - 1$ *descents*, i.e.,
places with $s_{i+1} < s_i$.

<span id="tab-2-3"></span>**Table 2.3.** The passes of `naturalMergeSort()`
on `5 8 1 9 3 7 2 6 4`, whose $r = 5$ runs are `5 8`, `1 9`, `3 7`, `2 6`
and `4`; the runs of each sequence are separated by semicolons, and each
pass halves the runs, rounding up.

| Pass | $a$ | $b$ | after the merges | runs |
|---|---|---|---|---|
| 1 | `5 8`; `3 7`; `4` | `1 9`; `2 6` | `1 5 8 9`; `2 3 6 7`; `4` | 3 |
| 2 | `1 5 8 9`; `4` | `2 3 6 7` | `1 2 3 5 6 7 8 9`; `4` | 2 |
| 3 | `1 2 3 5 6 7 8 9` | `4` | `1 2 3 4 5 6 7 8 9` | 1 |

<span id="lem-2-31"></span>**Lemma 2.31.** *A pass of `naturalMergeSort()`
leaves at most $\lceil r / 2 \rceil$ runs; hence the sort of a sequence of
$r$ runs makes $\lceil \log_2 r \rceil$ passes at most and costs $O( n \log
r )$ time, i.e., $O( n \log n )$ in the worst case and $\Theta( n )$ on a
sorted sequence, which one scan finds to be a single run. No sort by
comparisons does better than $\Omega( n \log r )$ in the worst case on the
sequences of $r$ runs of equal lengths.*

*Proof.* The merge of two sorted runs is a sorted stretch, and two runs
that go to the same sequence one after another may only join into one; the
$j$-th merge of the pass therefore covers the runs $2 j - 1$ and $2 j$ of
the pass, and it gives one run or part of one. Each pass costs $\Theta( n
)$ reads and writes. For the lower bound, the permutations whose runs have
the lengths $\ell_1 , \ldots , \ell_r$ are at most $n! / ( \ell_1! \cdots
\ell_r! )$, a multinomial whose logarithm is at most $n \log_2 r$ and is
$\Omega( n \log r )$ when the runs have equal lengths; a sort by
comparisons must tell them apart ([McIlroy
1993](Z-bibliography.md#ref-mcilroy1993)). $\square$

On a `List`, `listSort()` of [§2.10.1](02-sequence.md#sec-2-10-1) does the
same work by relinking the cells, with no copy of an item.

In `ADS/sequence/SequenceAlgorithms.h`, the functions are declared as

```cpp
// declarations of <ADS/sequence/SequenceAlgorithms.h>
template< Sequence S >
long search( const S & s , const typename S::Item & v );
template< Sequence S >
long searchOrdered( const S & s , const typename S::Item & v );
template< Sequence S >
std::pair< typename S::Pos , bool > lowerBound( const S & s ,
                                               const typename S::Item & v );
template< Sequence S >
S sublist( const S & s , typename S::Pos p , typename S::Pos q );
template< Sequence S >
Order order( const S & s );
template< Sequence S >
void bubbleSort( S & s );
template< Sequence S >
S mergeSorted( const S & a , const S & b );
template< Sequence S >
void naturalMergeSort( S & s , long * passes = nullptr );
template< Sequence S >
void removeDuplicates( S & s );
```

### 2.16.1 Rotations {#sec-2-16-1}

`rotateArray( a , r )` of `ADS/sequence/IndexedList.h` moves each item
$a[ i ]$ of an array to $a[ ( i + r ) \bmod n ]$ ([Morin 2013, exercise
2.4](Z-bibliography.md#ref-morin-cpp)). `rotate( l , r )` does the same on
an `IndexedList`, and it calls the `rotate()` of the list when it has one,
in $O( 1 + \min\{ r , n - r \} )$ time on `RingDeque`, `DualArrayDeque` and
`List` ([Morin 2013, exercises 2.5 and 3.8](Z-bibliography.md#ref-morin-cpp);
[Algorithm 2.5](02-sequence.md#alg-2-5), [Algorithm
2.7](02-sequence.md#alg-2-7) and [Algorithm 2.18](02-sequence.md#alg-2-18)).

<span id="alg-2-31"></span>**Algorithm 2.31.** The rotation by three
reversals and the rotation by cycles ([Bentley 2000,
§2.3](Z-bibliography.md#ref-bentley2000)), $r$ reduced modulo $n$.

--8<-- "generated/IndexedList-rotateArray.pseudo.md"

--8<-- "generated/IndexedList-rotate.pseudo.md"

<span id="lem-2-32"></span>**Lemma 2.32.** *`rotateArray()` makes at most $n$
swaps and uses $O( 1 )$ space; the rotation by cycles makes $n + \gcd( n ,
r )$ moves.*

*Proof.* With $a = X Y$ and $|Y| = r$, the reversal of the whole gives $Y^R
X^R$, and reversing each part gives $Y X$, i.e., each item $r$ places on;
the three reversals swap $\lfloor n / 2 \rfloor + \lfloor r / 2 \rfloor +
\lfloor ( n - r ) / 2 \rfloor \le n$ pairs. The permutation $i \mapsto ( i +
r ) \bmod n$ has $\gcd( n , r )$ cycles, each of $n / \gcd( n , r )$ items,
and following a cycle moves each of its items once, plus one move to save
its first item. $\square$

A rotation by cycles reads and writes each item once (cf. [Lemma
2.32](02-sequence.md#lem-2-32)), through `operator[]`, but it jumps by $r$
places at each step, while the reversals read the array forward and
backward, which usually suits the cache better on a large array.

Two more algorithms on sequences live in other chapters. The shuffle of
Fisher and Yates in the form of [Durstenfeld
1964](Z-bibliography.md#ref-durstenfeld1964), which gives each of the $n!$
orders with probability $1 / n!$ in $n - 1$ swaps ([Morin 2013, exercise
7.4](Z-bibliography.md#ref-morin-cpp)), is `shuffle()` and `shuffleFast()`
of [§17.9](17-numeric.md#sec-17-9), whose indices are drawn by the method
of [Lemire 2019](Z-bibliography.md#ref-lemire2019) and never by $g() \bmod
j$. Finally, the sorts of a vector are in chapter [12](12-sort.md#ch-12).

In `ADS/sequence/IndexedList.h`, whose concepts `IndexedList` and
`IndexedDeque` are in [Table 1.1](01-introduction.md#tab-1-1), the
functions are declared as

```cpp
// declarations of <ADS/sequence/IndexedList.h>
template< class T >
void rotateArray( std::span< T > a , std::size_t r );
template< IndexedList L >
void rotate( L & l , std::size_t r );
```

## 2.17 Rearrangements in place {#sec-2-17}

In this section we describe the functions of `ADS/sequence/Rearrangements.h`,
which reorder a vector $A[ 0 .. n - 1 ]$ by swaps, or with a buffer much
smaller than $n$. The partition into three classes of [Dijkstra 1976,
chapter 14](Z-bibliography.md#ref-dijkstra1976) (the Dutch national flag)
puts the items of color 0 first, then those of color 1, then those of color
2, by the invariant

$$
A[ 0 .. i ) \text{ of color 0} , \quad A[ i .. j ) \text{ of color 1} ,
\quad A[ j .. k ) \text{ not examined} , \quad A[ k .. n )
\text{ of color 2} , \tag{2.10}
$$

which holds at the start with $i = j = 0$ and $k = n$ ([Algorithm
2.32](02-sequence.md#alg-2-32) and [Figure 2.27](02-sequence.md#fig-2-27)).

<span id="alg-2-32"></span>**Algorithm 2.32.** The Dutch national flag
([Dijkstra 1976, chapter 14](Z-bibliography.md#ref-dijkstra1976)), the
alternating order, the sort of a $k$-sorted vector and the sort by the
fewest swaps.

--8<-- "generated/Rearrangements-dutchNationalFlag.pseudo.md"

--8<-- "generated/Rearrangements-wiggleSort.pseudo.md"

--8<-- "generated/Rearrangements-sortKSorted.pseudo.md"

--8<-- "generated/Rearrangements-minimumSwapSort.pseudo.md"

![Figure 2.27. The invariant (2.10) of the Dutch national flag on `0 0 1 1 1 0 2 1 2 2`, with $i = 2$, $j = 5$ and $k = 8$ marked below, the places $A\lbrack j .. k )$ not examined yet in the faintest tint; the step on $A[ 5 ] = 0$ swaps it with $A[ 2 ]$ (both in violet), giving `0 0 0 1 1 1 2 1 2 2` with $i = 3$, $j = 6$, $k = 8$.](../figures/sequence-flag.svg){#fig-2-27}

<span id="lem-2-33"></span>**Lemma 2.33.** *A step that examines $A[ j ]$
keeps (2.10); hence `dutchNationalFlag()` partitions the items by color
after $n$ steps, with $n$ reads of a color, at most $n$ swaps and $O( 1 )$
space.*

*Proof.* An item of color 0 is swapped with $A[ i ]$, which is of color 1
(or is $A[ j ]$ itself), and both $i$ and $j$ grow; one of color 1 makes $j$
grow; one of color 2 is swapped with $A[ k - 1 ]$, not examined yet, and
$k$ falls. Each step makes the stretch not examined one shorter, and it
swaps at most once. $\square$

It is the partition of the quicksort with three ways, the keys smaller
than, equal to and larger than the pivot being the three colors (the
partition of Bentley and McIlroy of chapter [12](12-sort.md#ch-12) reaches
the same three classes another way). The *alternating* order $A[ 0 ] \le A[
1 ] \ge A[ 2 ] \le A[ 3 ] \ge \cdots$ takes instead one pass of `wiggleSort()`,
which compares each item with the next one and swaps them when they are in
the wrong order for the parity of the place.

<span id="lem-2-34"></span>**Lemma 2.34.** *`wiggleSort()` puts a vector
of $n \ge 1$ items in alternating order with $n - 1$ comparisons and at most
$n - 1$ swaps; with distinct items the order is strict.*

*Proof.* Before the step $i$ the places $A[ 0 .. i ]$ alternate. If the step
swaps $A[ i ]$ and $A[ i + 1 ]$, it keeps the order of $A[ i - 1 ]$ and $A[
i ]$ by transitivity: e.g., for an odd $i$ with $A[ i - 1 ] \le A[ i ] < A[ i
+ 1 ]$, the swap puts at the place $i$ the larger $A[ i + 1 ]$, which is still
at least $A[ i - 1 ]$, and the case of an even $i$ is the mirror. After the
step, $A[ i ]$ and $A[ i + 1 ]$ are in the order of the parity of $i$, which
is strict when they are distinct. $\square$

With repeated items the order may be weak where a strict one exists: `1 2 2
3` is already alternating, and `wiggleSort()` leaves it, while `2 3 1 2` is
a strict alternating order of the same items.

A vector is $k$-*sorted* when each item is at most $k$ places away from its
place in the sorted order (the stable one, for repeated items). The item of
the place $i$ of the sorted order is then among $A[ i .. i + k ]$, and
`sortKSorted()` keeps the items of this window not yet placed in a binary
heap ([§9.3](09-priority.md#sec-9-3)) of $k + 1$ items at most, out of
which it takes the least.

<span id="lem-2-35"></span>**Lemma 2.35.** *`sortKSorted()` sorts a
$k$-sorted vector of $n$ items in $O( n \log ( k + 2 ) )$ time, with $k + 1$
places besides the vector, while the insertion sort takes $O( n ( k + 1 ) )$
time on it.*

*Proof.* Let the items be ranked in the stable sorted order. Before the step
$i$, the places $A[ 0 .. i )$ hold the items of rank $0 , \ldots , i - 1$,
and the heap holds the other items of the first $\min\{ i + k + 1 , n \}$
places of the input. The item of rank $i$ was at most $k$ places after the
place $i$, and hence it is in the heap, all of whose items have rank $i$ or
more. Hence the least item of the heap has the value of the item of rank
$i$, which the step writes at the place $i$, while the item of the place $i
+ k + 1$ enters the heap. Each step costs one sift of the heap, $O( \log (
k + 2 ) )$ time. As for the insertion sort, an inversion $A[ i ] > A[ j ]$
with $i < j$ needs the rank of $A[ i ]$, at most $i + k$, to exceed that of
$A[ j ]$, at least $j - k$; hence $j - i < 2 k$, the inversions are fewer
than $2 k n$, and [Lemma 12.6](12-sort.md#lem-12-6) gives the time.
$\square$

<span id="lem-2-36"></span>**Lemma 2.36.** *Any sort by comparisons of the
$k$-sorted vectors of $n \ge k$ items makes $\Omega( n \log k )$
comparisons in the worst case.*

*Proof.* There are at least $( k! )^{\lfloor n / k \rfloor}$ $k$-sorted
orders of $n$ items, since the items of each of the $\lfloor n / k \rfloor$ blocks
of $k$ consecutive places may be permuted independently, each moving fewer
than $k$ places. A decision tree with that many leaves has a height of at
least $\lfloor n / k \rfloor \log_2 k! = \Omega( n \log k )$, by $\log_2 k! \ge
( k / 2 ) \log_2 ( k / 2 )$. $\square$

Finally, `minimumSwapSort()` gives the fewest swaps that sort a vector of
distinct items, and the least cost of a sort by swaps when a swap costs the
sum of its two items. Both rest on the cycles of the permutation that sorts
the items, i.e., that takes each item to its place in the sorted order.

<span id="lem-2-37"></span>**Lemma 2.37.** *The fewest swaps that sort a
vector of $n$ distinct items are $n - c$, $c$ the number of cycles of the
permutation that sorts it, and $k - 1$ swaps sort each cycle of $k$ items.*

*Proof.* A swap of two items of one cycle splits it into two cycles, and a
swap of two items of different cycles joins them; the sorted vector has $n$
cycles of one item, and hence any sort takes $n - c$ swaps at least. A swap
that puts an item of a cycle of $k > 1$ items in its place leaves a cycle of
$k - 1$ items and one of one item, and therefore $k - 1$ such swaps sort the
cycle. $\square$

<span id="lem-2-38"></span>**Lemma 2.38.** *When a swap costs the sum of its
two items, a cycle of $k > 1$ items of sum $s$ and least item $m$ is sorted
at the cost*

$$
\min\{ s + ( k - 2 ) m , \ s + m + ( k + 1 ) M \} , \tag{2.11}
$$

*$M$ the least item of the vector, and $s + ( k - 2 ) m$ is the least cost
of the sorts of the cycle by swaps of its own items. If no item is
negative, the sum of (2.11) over the cycles is the least cost of all the
sorts of the vector by swaps, which may also join the cycles.*

*Proof.* Either $m$ is swapped in turn with each other item of the cycle,
which takes part in one swap while $m$ takes part in $k - 1$, at the cost $(
s - m ) + ( k - 1 ) m$. Otherwise, $M$ enters the cycle by a swap with $m$,
sorts the cycle in the same way with $k - 1$ swaps, and is swapped back, at
the cost $2 ( m + M ) + ( s - m ) + ( k - 1 ) M$. As for the bound, a sort
by swaps of the items of the cycle makes at least $k - 1$ swaps by [Lemma
2.37](02-sequence.md#lem-2-37), i.e., $2 ( k - 1 )$ participations of an
item in a swap, and each of the $k$ items takes part once at least, since
none is in its place. The cost, i.e., the sum of the items over the
participations, is therefore at least $s$ for the first participation of
each item, plus $m$ for each of the other $2 ( k - 1 ) - k = k - 2$.

For the last claim, write the cost (2.11) of a cycle $Z$ of $k \ge 1$ items
as $f( Z ) = s - m + \varphi( k , m )$, with $\varphi( k , m ) = \min\{ ( k
- 1 ) m , 2 m + ( k + 1 ) M \}$; for $k = 1$ this is 0, since $m , M \ge 0$.
Let $\Phi$ be the sum of $f$ over the cycles of the permutation that sorts
the vector, which is 0 on the sorted vector. We show that a swap of the
items $x$ and $y$ lowers $\Phi$ by at most $x + y$, so that any sort costs
at least the initial $\Phi$, which the sorts of the cycles above reach.
If $x$ and $y$ are in two cycles $Z_1 \ni x$ and $Z_2 \ni y$, of $k_i$
items, sum $s_i$ and least item $m_i$, with $m_1 \le m_2$, the swap joins
them. Since $x \ge m_1$ and $y \ge m_2$, it is then enough that $\varphi( k_1 ,
m_1 ) + \varphi( k_2 , m_2 ) \le \varphi( k_1 + k_2 , m_1 ) + m_1 + 2 m_2$.
If $\varphi( k_1 + k_2 , m_1 ) = ( k_1 + k_2 - 1 ) m_1$, this follows from
$\varphi( k_1 , m_1 ) \le ( k_1 - 1 ) m_1$ and $\varphi( k_2 , m_2 ) \le 2
m_2 + ( k_2 + 1 ) m_1$, as $M \le m_1$; otherwise from the second terms of
the two minima, whose sum is $2 m_1 + 2 m_2 + ( k_1 + k_2 + 2 ) M \le 3 m_1
+ 2 m_2 + ( k_1 + k_2 + 1 ) M$. If $x$ and $y$ are in one cycle $Z$ of $k$
items and least item $m$, the swap splits it into $Z_1 \ni x$ and $Z_2 \ni
y$, say with $m$ in $Z_1$. It is then enough that $\varphi( k_1 + k_2 , m )
\le \varphi( k_1 , m ) + \varphi( k_2 , m_2 ) + m$. The right side takes
one of four values, by the terms that the two minima take. With the first
terms it is $( k_1 - 1 ) m + ( k_2 - 1 ) m_2 + m \ge ( k_1 + k_2 - 1 ) m$,
since $m_2 \ge m$. The three other values are at least $2 m + ( k_1 + k_2 +
1 ) M$: $k_1 m + 2 m_2 + ( k_2 + 1 ) M$ exceeds it by $k_1 ( m - M ) + 2 (
m_2 - m ) \ge 0$, $3 m + ( k_1 + 1 ) M + ( k_2 - 1 ) m_2$ by $( m - M ) + (
k_2 - 1 ) ( m_2 - M ) \ge 0$, and $3 m + 2 m_2 + ( k_1 + k_2 + 2 ) M$ by
more. $\square$

The hypothesis on the signs cannot be dropped: if two items have a negative
sum, repeating their swap twice lowers any cost, and no least cost exists,
while `minimumSwapSort()` still returns the sum of (2.11). The test
`lib_search.rearrangements` checks the sum against a search of the least
cost (Dijkstra's algorithm on the permutations) on the vectors of up to 6
distinct items.

We remark that the two optima, the fewest swaps and the least cost, may come
from different sequences of swaps. The sort that gives the permutation costs
$O( n \log n )$, and the costs are summed in the type `W` of the template,
`T` by default, which may be wider than `T` to avoid an overflow.

In `ADS/sequence/Rearrangements.h`, the functions are declared as

```cpp
// declarations of <ADS/sequence/Rearrangements.h>
template< class T , class F >
std::pair< std::size_t , std::size_t >
dutchNationalFlag( std::vector< T > & A , F && color ,
                   std::size_t * swaps = nullptr );
template< class T , class C = std::less<> >
void wiggleSort( std::vector< T > & A , C less = C() );
template< class T , class C = std::less<> >
void sortKSorted( std::vector< T > & A , std::size_t k , C less = C() );
template< class T , class W = T >
 requires std::is_arithmetic_v< T >
std::pair< std::size_t , W > minimumSwapSort( const std::vector< T > & A );
```

## 2.18 Discussion and references {#sec-2-18}

The vector that doubles, the list with pointers and the list with cursors
are in most textbooks on data structures ([Aho et al. 1983,
§2.2](Z-bibliography.md#ref-aho1983); [Knuth 1997, §2.2.2 and
§2.2.3](Z-bibliography.md#ref-knuth1997); [Montresor 2025, chapter
4](Z-bibliography.md#ref-montresor2025)). Also, the amortized analysis of
the doubling and the halving is that of [Tarjan
1985](Z-bibliography.md#ref-tarjan1985) in the form of [Cormen et al. 2009,
§17.4](Z-bibliography.md#ref-cormen2009). The array-based lists of [Morin
2013, chapter 2](Z-bibliography.md#ref-morin-cpp) refine the vector in two
directions: the insertions near either end (`RingDeque`, `DualArrayDeque`),
and the space wasted (`RootishArrayStack`, after [Brodnik et al.
1999](Z-bibliography.md#ref-brodnik1999), whose lower bound of $\Omega(
\sqrt{n} )$ places it attains). The tiered vector of [Goodrich and Kloss
1999](Z-bibliography.md#ref-goodrich1999) brings the insertion in the middle
down to $O( \sqrt{n} )$, and to $O( n^{1/l} )$ with $l$ levels, which [Bille
et al. 2017](Z-bibliography.md#ref-bille2017) engineer and compare with the
vector. The linked lists of [Morin 2013, chapter
3](Z-bibliography.md#ref-morin-cpp) are those of [Knuth 1997, §2.2.3 to
§2.2.5](Z-bibliography.md#ref-knuth1997), with the unrolled list of [Shao et
al. 1994](Z-bibliography.md#ref-shao1994) and the XOR list of [Sinha
2004](Z-bibliography.md#ref-sinha2004). The widths of the links of a skip
list are in the cookbook of [Pugh 1990b](Z-bibliography.md#ref-pugh1990b),
and the order maintenance in $O( 1 )$ amortized time goes back to [Dietz and
Sleator 1987](Z-bibliography.md#ref-dietz1987), whose two-level labeling
[Bender et al. 2002](Z-bibliography.md#ref-bender2002) simplify.

None of the rebuilds of this chapter (the balance of `DualArrayDeque`, the
rebuild of `ContiguousDeque`, the change of $b$ of the tiered and cubish
arrays) is spread over the operations, i.e., de-amortized, for two reasons.
First, the bound that matters for a LIST by rank, $O( 1 + \min\{ i , n - i
\} )$, is amortized anyway on these structures, whose insertion in the
middle shifts $\Theta( n )$ items; a rebuild spread over the operations
would only make the operations at the two ends $O( 1 )$ in the worst case.
Second, the global rebuilding that would do it (that of `RealTimeMinDeque`,
chapter [4](04-queue.md#ch-4)) keeps the old representation in use while the
new one is filled. It thus needs copies of the items, not moves, since the
old cells stay readable through `operator[]` (and, for `ContiguousDeque`,
through `data()` and the iterators, which are pointers into the one
contiguous piece). It also needs two representations at once, i.e., the
places of $2 n$ items or more, which the balance in place avoids;
furthermore, each insertion and removal in the middle must be replayed on
the part already copied, and the old cells must be destroyed a few at a time
afterwards. The constant work per operation at the ends would then likely be
several times (cf. [§4.9](04-queue.md#sec-4-9)) that of the amortized
structure. A deque that needs $O( 1 )$ time in the worst case at the ends is
the linked `Dequeue` of chapter [4](04-queue.md#ch-4), which allocates one
cell per insertion.

Exercise 1.6 of [Morin 2013](Z-bibliography.md#ref-morin-cpp) asks for
reference realizations of LIST, USET and SSET on an array, which serve as
the oracles of the tests: `ArraySequence` for the first, and `ArrayUSet` and
`ArraySSet` of `SetInterfaces.h` (chapter [10](10-set.md#ch-10)) for the
others. His exercise 1.7 is on how to make them faster. Its answers are the
shifts of a list by block moves and on the shorter side (`RingDeque`), the
`find()` of an SSET by binary search on the sorted array, and its insertions
by one `memmove()`, as `ArraySSet` does; the other realizations of [Table
2.2](02-sequence.md#tab-2-2) are the answers of his chapters 2 and 3. The
`RandomQueue` of his exercise 2.2 and the `MinStack`, `MinQueue` and
`MinDeque` of his exercises 3.14 to 3.16 are queues and stacks, which
chapters [3](03-stack.md#ch-3) and [4](04-queue.md#ch-4) describe. The
exercises of his chapter 4 on the skip list as a sorted set (4.1 to 4.3, 4.6
to 4.10 and 4.13 to 4.15) are in [§7.9](07-dictionary.md#sec-7-9).

No benchmark is reported here. In particular, the claims of the headers
against Morin's code (the shifts by `memmove()`, the masks of a power of 2,
the raw places built on insertion, the nodes in one block of memory) are to
be measured, on an idle machine, by the campaign of appendix B.

## 2.19 Exercises {#sec-2-19}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`); the names of the tests on chapter 2 of Morin follow the
numbering of the C++ edition, as the text does.

1. Realize the SEQUENCE on a vector that doubles when it is full and halves
   when it is a quarter full, and prove that $m$ operations at the end cost
   $O( m )$ ([Montresor 2025, §4.2](Z-bibliography.md#ref-montresor2025)).
   Tests: `ch04.spec.ArraySequence`, `contract_sequence.copy-move`.
2. Realize the `ArrayStack` and the `FastArrayStack` of Morin, and draw the
   operations of his Figure 2.1 on them ([Figure 2.1](02-sequence.md#fig-2-1)).
   Test: `ods_lists.ods-2.1.array-stack`.
3. Insert the $k$ items of a collection at the rank $i$ in $O( n - i + k )$
   time, and explain why $k$ calls of `add( i , x )` are slower ([Morin
   2013, exercise 2.1](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex2.1.add-all`.
4. Realize a vector whose first $N$ items need no allocation. Test:
   `sequences.small-vector`.
5. Realize the `ArrayQueue` and the `ArrayDeque` of Morin on a circular
   vector, and draw the operations of his Figures 2.2 and 2.3
   ([Figure 2.3](02-sequence.md#fig-2-3) and
   [Figure 2.4](02-sequence.md#fig-2-4)). Tests:
   `ods_lists.ods-2.3.array-queue`, `ods_lists.ods-2.4.array-deque`.
6. Shift the items of the `ArrayDeque` by block moves, and reduce its
   places by a mask instead of `%` with a capacity that is a power of 2
   ([Morin 2013, exercises 2.6 and 2.7](Z-bibliography.md#ref-morin-cpp)).
   Test: `ods_lists.ods-ex2.7.power-of-two`.
7. Realize the `DualArrayDeque`, with a balance in place, and draw the
   operations of Morin's Figure 2.4 on it ([Figure 2.5](02-sequence.md#fig-2-5)).
   Test: `ods_lists.ods-2.5.dual-array-deque`.
8. Realize a queue whose `remove()` removes an item chosen uniformly at
   random, with every operation in $O( 1 )$ amortized time ([Morin 2013,
   exercise 2.2](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex2.2.random-queue`.
9. Realize the treque, with insertions and removals in $O( 1 + \min\{ i , n -
   i , | n / 2 - i | \} )$ time ([Morin 2013, exercise
   2.3](Z-bibliography.md#ref-morin-cpp)). Test: `ods_lists.ods-ex2.3.treque`.
10. Rotate an array by $r$ places in place ([Morin 2013, exercise
    2.4](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.4.rotate-array`.
11. Rotate a LIST by $r$ ranks, in $O( 1 + \min\{ r , n - r \} )$ time on
    an `ArrayDeque` and on a `DualArrayDeque` ([Morin 2013, exercise
    2.5](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.5.rotate-list`.
12. Realize a deque with no modular arithmetic, its items in one contiguous
    piece, with the amortized costs of the `ArrayDeque` ([Morin 2013,
    exercise 2.8](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.8.contiguous-deque`.
13. Realize the `RootishArrayStack`, draw the operations of Morin's Figure
    2.5 on it ([Figure 2.8](02-sequence.md#fig-2-8)), and check that the
    places unused stay $O( \sqrt{n} )$ after every operation. Test:
    `ods_lists.ods-2.6.rootish-array-stack`.
14. Realize a list with $O( \sqrt{n} )$ places wasted and insertions and
    removals in $O( 1 + \min\{ i , n - i \} )$ time ([Morin 2013, exercise
    2.9](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.9.rootish-deque`.
15. Realize a list with $O( \sqrt{n} )$ places wasted and insertions and
    removals in $O( 1 + \min\{ \sqrt{n} , n - i \} )$ time, and then in
    $O( 1 + \min\{ i , \sqrt{n} , n - i \} )$ ([Morin 2013, exercises 2.10
    and 2.11](Z-bibliography.md#ref-morin-cpp)). Tests:
    `ods_lists.ods-ex2.10.tiered-deque`, `ods_lists.ods-ex2.11.tiered-deque`,
    `sequences.tiered-vector`.
16. Realize the `CubishArrayStack`, with `operator[]` in $O( 1 )$ time,
    insertions and removals in $O( n^{1/3} )$ amortized time and $O(
    n^{2/3} )$ places wasted ([Morin 2013, exercise
    2.12](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.12.cubish`.
17. State the LIST and the DEQUE as concepts and check that the
    realizations of this chapter model them. Test:
    `ods_lists.ods-1.2.interfaces`.
18. Realize the reference LIST, USET and SSET on an array ([Morin 2013,
    exercise 1.6](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex1.6.reference-sets`.
19. Realize the SEQUENCE on a singly linked list, with $O( 1 )$ insertions
    and removals, and on its cells from a pool ([Montresor 2025, exercise
    4.6](Z-bibliography.md#ref-montresor2025)). Tests: `ch04.ex4.6`,
    `ch04.ex4.6.pool`.
20. Realize the `SLList` of Morin as a stack and a queue, and draw the
    operations of his Figure 3.1 on it ([Figure 2.12](02-sequence.md#fig-2-12)).
    Test: `ods_lists.ods-3.1.sllist`.
21. Explain why a dummy cell cannot remove the special cases of an
    `SLList`; find its second last item without its size; realize its
    operations by rank in $O( 1 + i )$ time; reverse it in place; check its
    size ([Morin 2013, exercises 3.1 to 3.5](Z-bibliography.md#ref-morin-cpp)).
    Test: `ods_lists.ods-ex3.2-3.5.sllist`.
22. Realize the bidirectional circular list with a sentinel, with its cells
    from a pool ([Montresor 2025, §4.1](Z-bibliography.md#ref-montresor2025)).
    Tests: `ch04.spec.List`, `ch04.sec4.1.pool`, `contract_sequence.list`.
23. Realize the `DLList` of Morin with the access by rank from the nearer
    end. Test: `ods_lists.ods-3.2.dllist`.
24. On a `DLList`, check the size, insert before a cell, test whether it is
    a palindrome, rotate it with no item moved, truncate it, absorb another
    list, deal its cells of odd rank and reverse it, all with no new cell
    ([Morin 2013, exercises 3.5 to 3.12](Z-bibliography.md#ref-morin-cpp)).
    Test: `ods_lists.ods-ex3.5-3.12.dllist`.
25. Sort a `DLList` by merge sort with `takeFirst()` and `merge()` ([Morin
    2013, exercise 3.13](Z-bibliography.md#ref-morin-cpp)), and by the
    natural merge sort that relinks its cells. Tests:
    `ods_lists.ods-ex3.13.merge-sort`, `sequences.list-sort`.
26. Realize lists with cursors in one vector shared by several lists, with
    the free list. Tests: `ch04.spec.CursorList`, `beyond.cursor-list`.
27. Realize the `SEList` of Morin, and draw the three cases of an insertion
    and of a removal of his Figures 3.4 and 3.5 on it
    ([Figure 2.16](02-sequence.md#fig-2-16) and
    [Figure 2.17](02-sequence.md#fig-2-17)). Test: `ods_lists.ods-3.3.selist`.
28. Prove that an `SEList` used as a stack takes $O( 1 )$ amortized time per
    operation independently of $b$, and realize one whose deque operations
    take $O( 1 )$ time independently of $b$ ([Morin 2013, exercises 3.17
    and 3.18](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex3.17-3.18.selist-ends`.
29. Realize the XOR list ([Morin 2013, §3.4](Z-bibliography.md#ref-morin-cpp)).
    Test: `ods_lists.ods-3.4.xor-list`.
30. Swap two integers with no third variable ([Morin 2013, exercise
    3.19](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex3.19.xor-swap`.
31. Realize the `SkiplistList` of Morin; draw `remove( 2 )` and
    `add( 3 , x )`, the latter with a node of height 4, on the list of his
    Figure 4.5 ([Morin 2013, exercises 4.4 and
    4.5](Z-bibliography.md#ref-morin-cpp); [Figure
    2.24](02-sequence.md#fig-2-24) and [Figure
    2.23](02-sequence.md#fig-2-23)). Test: `ods_trees.ods4.3.skiplist-list`.
32. Truncate a `SkiplistList` at the index $i$, and absorb another one, in
    $O( \log n )$ expected time ([Morin 2013, exercises 4.11 and
    4.12](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_trees.ods-ex4.11.truncate-absorb`.
33. Realize a list by rank on a treap ([Morin 2013, exercise
    7.11](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_trees.ods-ex7.11.treap-list`.
34. Maintain the order of a list with `addAfter()`, `remove()` and
    `testBefore()`, the insertion in $O( 1 )$ amortized time ([Morin 2013,
    exercise 8.11](Z-bibliography.md#ref-morin-cpp)). Tests:
    `ods_trees.ods-ex8.11.order-maintenance`,
    `ods_trees.ods-ex8.11.order-constant`.
35. Search, merge, deduplicate and sort a sequence through its positions,
    and tell its order. Tests: `beyond.sequence-search`,
    `beyond.sequence-order`, `beyond.sequence-sorting`,
    `beyond.sequence-merge-dedup`, `contract_sequence.sequence-algorithms`.
36. Visit a sequence with the `for` on a range of C++, and pass its
    iterators to the algorithms of the standard library and of Boost.
    Tests: `beyond.sequence-range`, `interop.sequences`.
37. Partition a vector into three colors, put it in alternating order, sort
    a $k$-sorted vector in $O( n \log k )$ time, and find the fewest swaps
    and the least cost of sorting it. Tests: `lib_search.rearrangements`,
    `sequences.dutch-flag-branchless`, `sequences.k-sorted-sift`.
38. Merge two runs by appending at the end of a vector, as the natural merge
    sort does. Test: `sequences.merge-sort-appender`.

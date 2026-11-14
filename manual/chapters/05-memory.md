# 5. Memory management {#ch-5}

[Source: `include/ADS/memory/Allocators.h`, `Arena.h`, `ObjectPool.h`,
`SizeClassAllocator.h`, `PageHeap.h`, `PoolAllocator.h`,
`ConcurrentPool.h`, `HugePageAllocator.h`, `CellHeap.h`]

We are concerned in this chapter with the *dynamic storage allocation*,
i.e., with the management of a memory from which a program takes blocks
of the sizes it needs, when it needs them, and to which it gives them
back in an order that the allocator does not know in advance. The headers
of `ADS/memory` first realize the general methods of [Knuth 1997,
§2.5](Z-bibliography.md#ref-knuth1997) on a memory simulated by a vector,
whose blocks and tags can thus be inspected
([§5.2](05-memory.md#sec-5-2) and [§5.3](05-memory.md#sec-5-3)). Then
come the allocators of today that descend from them: the arenas and the
pools of objects of one type ([§5.4](05-memory.md#sec-5-4) and
[§5.5](05-memory.md#sec-5-5)), the size classes for one thread
([§5.6](05-memory.md#sec-5-6) and [§5.7](05-memory.md#sec-5-7)) and for
many ([§5.8](05-memory.md#sec-5-8)), and the huge pages
([§5.9](05-memory.md#sec-5-9)). In [§5.10](05-memory.md#sec-5-10) the
program never gives a block back, since a garbage collector finds the
cells it no longer reaches; [§5.11](05-memory.md#sec-5-11) compares the
allocators on what each one is meant for. All the allocators but the
first two and the last one give real memory (those three give indices of a
vector). Most of them serve the structures of the other chapters (e.g.,
the nodes of the lists, of the search trees and of the hash tables).

## 5.1 Definitions and notation {#sec-5-1}

An allocator manages a memory of $N$ words (or bytes) and serves two
operations: `allocate(n)` gives a *block* of at least $n$ contiguous words
that no other block overlaps, and `free()` (or `deallocate()`) takes a
block back. The words not in a block given are *free*, and an allocator
keeps the free blocks on one or more *free lists*, which the free blocks
themselves hold, a word of each free block pointing to the next one (the
list AVAIL of [Knuth 1997, §2.2.3](Z-bibliography.md#ref-knuth1997)). A
block may carry a *header*, i.e., some words that the allocator reads,
such as the size of the block; a *sized* deallocation (as that of
`std::allocator`) gives the size back with the block, which then needs no
header.

We call *fragmentation* the memory that an allocator holds and does not
give for use. It is *internal* when a block is larger than its request
(because of the rounding to a size class or to a power of 2, or because of
a header). It is *external* when the free memory is in pieces too small
for a request, which then fails (or takes new memory from the system)
although the free words would be enough. Merging adjacent free blocks
(*coalescing*) fights the external fragmentation; giving each request a
block of one of a few sizes (a *size class*) removes it within a class, at
the price of an internal one; the allocators of the chapter are points
between the two extremes (cf. [Table 5.1](05-memory.md#tab-5-1)). Throughout
the chapter, $n$ (or $b$) is the size of a request and $a$ its alignment,
$s( p )$ is the number of words of the block at the address $p$, $F$ is a
free list ($F_k$ that of the blocks of $2^k$ words, $F_c$ that of the
class $c$), and $g = 8$ bytes is the step of the size classes of the
pools.

<span id="tab-5-1"></span>**Table 5.1.** The allocators of `ADS/memory`
and the cost of their operations; $f$ is the number of free blocks, $m$
the logarithm of the memory of the buddy system, $k$ the largest number of
objects that a pool has held at once.

| Allocator | Blocks | Allocate | Free | Waste |
|---------------------------|---------------|-------------|----------------|-------------------|
| `BoundaryTagAllocator` | any size | $O( f )$ | $O( 1 )$, merging both neighbors | 2 words of tags, external fragmentation |
| `BuddyAllocator` | powers of 2 | $O( m )$ splits | $O( m )$ merges | less than half of each block |
| `Arena` | objects of one type | $O( 1 )$, a pointer bump | only all at once | part of the last block |
| `ObjectPool` | objects of one type | $O( 1 )$, but for a new block | $O( 1 )$ | at most $2 k + n$ slots |
| `SizeClassAllocator` | 41 classes up to 32768 bytes | $O( 1 )$ | $O( 1 )$, sized | below 20% of a block past 128 bytes |
| `PoolResource` | classes of 8 bytes up to $M$ | $O( 1 )$ | $O( 1 )$ | below $\max( a , g )$ bytes a slot |
| `ConcurrentPoolResource` | as above, any thread | $O( 1 )$ amortized | $O( 1 )$, a CAS per batch of remote frees | as above, and 31 slots per class and thread in transit |
| `HugePageAllocator` | any, aligned to 2 MB from 2 MB | that of the system | that of the system, or kept from 8 MB | address space below a huge page |
| `CellHeap` | cells of two fields | $O( 1 )$, a collection when none is free | by the collector | none |

The general allocators of [Knuth 1997,
§2.5](Z-bibliography.md#ref-knuth1997) serve any size, and pay for it
with a search of the free list and with the external fragmentation. The
pools serve one size, or a few; they pay with the internal fragmentation
and with the memory that a class keeps while another one needs it. The
allocators of today are of the second kind, and they also care about the
locality of the blocks. In fact, the blocks given one after another are usually
next to each other in memory, and a block just freed (hopefully still in
the cache) is the next one given ([Wilson et al.
1995](Z-bibliography.md#ref-wilson1995)).

## 5.2 Boundary tags {#sec-5-2}

`BoundaryTagAllocator` simulates a memory of $N \ge 4$ words by a vector
$M$ of $N + 2$ words, the last two of which are the head of the list of
the free blocks. A block of $s$ words at the place $p$ has a *tag* at both
its ends: $M[ p ] = M[ p + s - 1 ] = +s$ if the block is free, and $-s$
if it is in use (the boundary tags of [Knuth 1997, Algorithm
2.5C](Z-bibliography.md#ref-knuth1997)). A free block also holds its
neighbors on the doubly linked free list, in $M[ p + 1 ]$ and
$M[ p + 2 ]$; hence, a block has at least 4 words. A request of $n$ words
takes $s_0 = \max( n + 2 , 4 )$ words, two of them for the tags, and the
address returned is $p + 1$, the first word after the lower tag
([Algorithm 5.1](05-memory.md#alg-5-1)).

A request chooses its block on the free list by one of three rules. The *first
fit* stops at the first block large enough (Algorithm 2.5A), and the
*best fit* reads all the list and takes the smallest such block. The
*next fit* starts where the last search stopped (the roving pointer of
Knuth), in order that the small pieces left by the splits do not gather
at the head of the list. We choose the rule by the argument `fit` of the constructor, one of `Fit::FIRST` (the default), `Fit::BEST` and
`Fit::NEXT`. When the rest of the block chosen has room for a free block (4
words at least), the block is split and the request gets its *upper* part;
hence, the lower part keeps its place on the list, and only its tags
change, while the rover moves to the block after it on the list. A request
that no free block can serve returns $-1$, and so does at once a request of
more than $N$ words, for which $n + 2$ might not even fit in a `long`.

![Figure 5.1. A `BoundaryTagAllocator` of 24 words with the blocks at 0 and at 12 free (6 and 8 words) and those at 6 and at 20 in use (6 and 4 words), before and after `free(7)`; the tags hold $+s$ for a free block and $-s$ for a block in use, `nx` and `pv` are the links of a free block, the violet block is the one freed, those in the faintest tint are free.](../figures/memory-tags.svg){#fig-5-1}

A freed block reads the tag just below it, which is the upper tag of the
block below, and the tag just above it, which is the lower tag of the
block above. These two reads give whether the neighbors are free and how
large they are. Since the list is doubly linked, each free neighbor leaves
it in constant time and merges with the block, and the merged block goes
at the head of the list ([Figure 5.1](05-memory.md#fig-5-1)).

<span id="alg-5-1"></span>**Algorithm 5.1.** The operations of
`BoundaryTagAllocator` ([Knuth 1997, Algorithms 2.5A and
2.5C](Z-bibliography.md#ref-knuth1997)); $s( p )$ is the size of the block
at $p$, read in its tags.

--8<-- "generated/Allocators-BoundaryTagAllocator-allocate.pseudo.md"

--8<-- "generated/Allocators-BoundaryTagAllocator-free.pseudo.md"

<span id="lem-5-1"></span>**Lemma 5.1.** *After each operation of
`BoundaryTagAllocator` the blocks tile the memory $[ 0 , N )$, the free
list holds exactly the free blocks, and no two free blocks are adjacent.
Hence `free()` costs $O( 1 )$ time, and `allocate()` costs $O( f )$ time
on a list of $f$ free blocks.*

*Proof.* Initially, the three properties hold for the single free block
of the memory. A split replaces a free block by a free block and a block
in use that tile the same words, and a block taken with no split leaves
the list and becomes a block in use. When a block is freed, its neighbors
are free only if their tags are positive; if so, they are unlinked and
merged with it. By induction, the neighbors of a free neighbor were in
use, and the merged block is thus bordered by blocks in use (or by the
ends of the memory). There are at most two merges, each one an unlink in
$O( 1 )$ time from the doubly linked list. The search of `allocate()`
reads each free block at most once (cf. line 3 of
[Algorithm 5.1](05-memory.md#alg-5-1)); the next fit reads the list from
the rover to its end, then from its head to the rover. $\square$

The audit `tiled()` checks the three properties of
[Lemma 5.1](05-memory.md#lem-5-1) after each operation, in $O( N )$ time,
when the contracts are at their highest level. Besides, the class keeps the set of the blocks in use, for the precondition that `free()` receives an
address given by `allocate()` and still in use. This set costs
$O( \log u )$ time per operation for $u$ blocks in use; it is the price
of the contract, since the method itself does not need it. Two queries,
`freeWords()` and `freeBlocks()`, give the words in free blocks (tags
included) and the number of the free blocks, by a walk of the free list in
$O( f )$ time. Apart from the contracts, the code of `free()` follows
[Algorithm 5.1](05-memory.md#alg-5-1) line by line.

```cpp
--8<-- "generated/Allocators-boundary-free.cpp"
```

Which fit is best cannot be told in general. [Knuth 1997,
§2.5](Z-bibliography.md#ref-knuth1997) reports simulations in which the
first fit was at least as good as the best fit. He also shows, as the
*fifty-percent rule*, that under some assumptions of equilibrium the free
blocks are about half as many as the blocks in use, whatever the fit. In practice, the
best fit leaves the smallest pieces, which are often (but not always) "too
small" for any later request. On the other hand, the next fit spreads the splits over the list, at the
price of a (possibly much) worse locality ([Wilson et al.
1995](Z-bibliography.md#ref-wilson1995)).

## 5.3 The buddy system {#sec-5-3}

`BuddyAllocator` manages a memory of $N = 2^m$ words, $0 \le m \le 40$,
and gives only blocks of $2^k$ words, each one at an address multiple of
its size ([Knowlton 1965](Z-bibliography.md#ref-knowlton1965); [Knuth
1997, Algorithms 2.5R and 2.5S](Z-bibliography.md#ref-knuth1997)). The
free blocks of $2^k$ words are kept in the ordered set $F_k$, and the
blocks in use in a map from their addresses to their sizes. A free thus
needs no size, and a block needs no tag.

<span id="lem-5-2"></span>**Lemma 5.2.** *In the buddy system, a block of
$2^k$ words at the address $a$, $k < m$, was split from the block of
$2^{k+1}$ words at $a \wedge \neg 2^k$, and its* buddy *(the other half of
that block) is at*

$$
a \oplus 2^k . \tag{5.1}
$$

*Proof.* By induction on the splits. A block of $2^{k+1}$ words at an
address multiple of $2^{k+1}$ has the bit $k$ of its address equal to 0.
Its halves are at $a$ and $a + 2^k$, which differ in that bit only and
are multiples of $2^k$; clearing the bit $k$ gives the address of the
block split, and flipping it gives the other half. $\square$

A request of $n$ words takes $k = \lceil \log_2 n \rceil$. It splits the
free block of least size $2^j \ge 2^k$ by halves until it has $2^k$
words, each upper half joining its free list. A freed block finds its
buddy by (5.1); if the buddy is free and whole (i.e., on $F_k$), the block
merges with it and goes on upwards ([Algorithm 5.2](05-memory.md#alg-5-2)
and [Figure 5.2](05-memory.md#fig-5-2)).

<span id="alg-5-2"></span>**Algorithm 5.2.** The operations of
`BuddyAllocator` ([Knuth 1997, Algorithms 2.5R and
2.5S](Z-bibliography.md#ref-knuth1997)); $F_k$ is the set of the free
blocks of $2^k$ words.

--8<-- "generated/Allocators-BuddyAllocator-allocate.pseudo.md"

--8<-- "generated/Allocators-BuddyAllocator-free.pseudo.md"

![Figure 5.2. A `BuddyAllocator` of $2^4$ words before and after `free(0)`: the freed block of $2^2$ words merges with its buddy at $0 \oplus 2^2 = 4$, which is free, and stops at the buddy $0 \oplus 2^3 = 8$ of the merged block, which is in use; the addresses are written in binary below the blocks.](../figures/memory-buddy.svg){#fig-5-2}

<span id="lem-5-3"></span>**Lemma 5.3.** *In `BuddyAllocator` no two
free buddies are ever both on the free lists, and a block given is less
than twice its request, i.e., its internal waste is less than half of the
block. `allocate()` reads at most $m + 1$ free lists and splits at most
$m$ times, `free()` merges at most $m$ times, and each one costs
$O( m \log N )$ time in all.*

*Proof.* A split puts on $F_j$ only an upper half whose lower half is
split further or given, and a free merges the block with its buddy for as
long as the buddy is free; hence, after each operation no free block has
a free buddy. Since $2^{k-1} < n \le 2^k$, the block of $2^k$ words is
smaller than $2 n$, and the waste $2^k - n$ is less than $2^{k-1}$. A
request looks at the sets $F_k , \ldots , F_m$, i.e., at most $m + 1$
of them, after computing $k \le m$ by at most $m$ doublings, and splits
$j - k \le m$ times; a free merges at most $m - k$ times, since each merge
raises $k$ by one and stops at $m$. Each step is an operation on an
ordered set of at most $N$ addresses (or on the map of the blocks in use),
in $O( \log N )$ time. $\square$

In the code, the merge loop of `free()` is that of
[Algorithm 5.2](05-memory.md#alg-5-2), with the buddy computed by an
exclusive or.

```cpp
--8<-- "generated/Allocators-buddy-free.cpp"
```

The buddy system has no external fragmentation in the sense of the
boundary tags, since a free block of $2^k$ words serves any request up to
$2^k$. Yet, two free blocks of $2^k$ words that are not buddies cannot
merge, and a request may therefore fail with enough free memory in total.
Its audit `merged()` checks that the free blocks are aligned to their
sizes, that no two free buddies exist, and that the free blocks and the
blocks in use tile the memory, in $O( N \log N )$ time. As for the
boundary tags, a request returns $-1$ when no block fits (at once if
$n > 2^m$), and two queries read the state: `freeWords()`, the words in
free blocks, and `blockSize(a)`, the size $2^k$ of the block in use at
$a$. Finally, the constructor requires $0 \le m \le 40$, and that $2^m$ fits in a
`long` (i.e., $m \le 30$ where a `long` has 32 bits).

## 5.4 Arenas {#sec-5-4}

An *arena* (also called a *region*) gives the objects of one type from
large blocks by the bump of a pointer, and frees them all at once with
the arena ([Hanson 1990](Z-bibliography.md#ref-hanson1990)). `Arena<T,
A>` keeps the vector of its blocks, each of $n$ places for objects of type
`T` taken from the allocator `A` (by default $n = 2^{12}$ and `A` is
`std::allocator<T>`), and the number $\mathit{used}$ of the places used in
the last block. Then `make(args)` builds the object in the
next place, after appending a new block when the last one is full
([Algorithm 5.3](05-memory.md#alg-5-3) and
[Figure 5.3](05-memory.md#fig-5-3)).

![Figure 5.3. An `Arena` of blocks of $n = 4$ objects: the vector of the blocks, the first block full and the second one with $\mathit{used} = 2$ before `make(g)` and $\mathit{used} = 3$ after it; the orange place is the one built.](../figures/memory-arena.svg){#fig-5-3}

<span id="alg-5-3"></span>**Algorithm 5.3.** The construction of an object
in an `Arena` ([Hanson 1990](Z-bibliography.md#ref-hanson1990)).

--8<-- "generated/Arena-make.pseudo.md"

<span id="lem-5-4"></span>**Lemma 5.4.** *After $z \ge 1$ calls of
`make()` that return an object, and none that throws, an `Arena` of blocks
of $n$ objects holds $\lceil z / n \rceil$ blocks, i.e., fewer than $n$
places not used, and each object keeps the address that `make()` returned
until the arena is destroyed. Each `make()` costs $O( 1 )$ time, but for
the allocation of a block and the amortized append to the vector of the
blocks.*

*Proof.* A block is appended only when the last one is full (or there is
none), and only the last block may thus have places not used; since each
call fills one place, the blocks are $\lceil z / n \rceil$. The vector
holds pointers to the blocks; hence, no block is moved or freed while the
arena lives, although the vector itself may be moved by an append. Besides the allocation and the append, the work of `make()` is a placement
`new` and two increments. $\square$

If the constructor of an object throws, the object is not counted, and
$\mathit{used}$ does not change. A block appended by that call stays, with
no object in it, and the next call of `make()` uses it; this is why the
lemma excludes the calls that throw, each one of which may leave a block
more. If instead the allocation of the block, or its append, throws, the
block is given back to `A` and the arena is as it was. In all cases the
destructor destroys exactly the objects built, $n$ in each block but the
last one and $\mathit{used}$ in the last one, the invariant checked after
each `make()` being that `size()` equals $( \#\mathit{blocks} - 1 ) n +
\mathit{used}$.

An object of an arena costs no header and no search, and the objects made
one after another are next to each other. Their addresses never change,
which matters to the structures that point to their objects (e.g., the
nodes of a search, which keep a pointer to the node they come from). The
arena also gives a random access iterator over the objects in the order
in which they were made: the object of index $j$ is in the place
$j \bmod n$ of the block $\lfloor j / n \rfloor$, and a new object only
moves the end of the sequence. The iterators, however, read the vector of
the blocks through its first element; hence, they stay valid only as long
as no `make()` appends a block, since an append may move that vector, whereas the pointers that `make()` returns
stay valid for the whole life of the arena. As a container in the sense of
the standard library, the arena has the types `value_type`,
`size_type`, `difference_type`, `reference`, `const_reference`, `iterator`
and `const_iterator` (the second a random access iterator over constant
objects, built also from the first), and the functions `begin()`, `end()`,
`cbegin()` and `cend()`. Besides, `size()` is the number of the objects made, and
`bytes()` the memory of the blocks, $\#\mathit{blocks} \cdot n \cdot
\mathrm{sizeof}( T )$. An arena can be neither copied nor moved, since
its objects are reached by their addresses. The price of all this is that
an object cannot be freed by itself: the destructor of the arena destroys
all the objects and frees the blocks.

## 5.5 Pools of objects {#sec-5-5}

`ObjectPool<T, A>` adds to the arena the free of one object. `make(args)`
builds an object in a slot; `free(x)` destroys it and puts its slot at the
head of a free list $F$ (the list AVAIL of [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997)), from which the next `make()`
takes it again. The last slot freed is thus the first one used again, and
its memory is likely still in the cache. A slot is a union of the room of
an object and of the pointer to the next free slot; it needs no header,
and takes $\max( \mathrm{sizeof}( T ) , \mathrm{sizeof}( \mathrm{void} * )
)$ bytes, rounded up to a multiple of the larger of the alignments of `T`
and of a pointer. The slots never used are cut
from the last block, from its place $\mathit{next}$ to its end
([Algorithm 5.4](05-memory.md#alg-5-4)).

The blocks are taken from the allocator `A`; the first block has one slot,
and each next one has twice the slots of the last, up to $n$ (by default
$2^{24}$). A block begins with a header, which holds its size in slots
and the address of the block before it. It takes the first
$H = \lceil 2 \, \mathrm{sizeof}( \mathrm{void} * ) / \mathrm{sizeof}(
\mathit{slot} ) \rceil$ slots of the block (on a machine of 64 bits, two
slots of 8 bytes, or one of 16 or more); the pool reaches all its blocks
from the last one, and needs no vector for them. In fact, the pool is six
words (the free list, the place $\mathit{next}$ and the end of the last
block, the last block, $n$ and the number of objects alive), besides its
allocator, which takes no room when it has no state (as
`HugePageAllocator`); it allocates only its blocks
([Figure 5.4](05-memory.md#fig-5-4)).

![Figure 5.4. An `ObjectPool` with blocks of 1, 2 and 4 slots, each one after its header `H` (linked to the header of the block before), before and after `free(e)`; the slots are numbered below the cells, a free slot holds the number of the next free slot (`-` for nil), and the last block has its place $\mathit{next}$ at the slot 6.](../figures/memory-pool.svg){#fig-5-4}

<span id="alg-5-4"></span>**Algorithm 5.4.** The operations of
`ObjectPool` (the list AVAIL of [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997)); $F$ is the list of the free
slots and $[ \mathit{next} , \mathit{end} )$ the slots never used of the
last block, of $k$ slots.

--8<-- "generated/ObjectPool-make.pseudo.md"

--8<-- "generated/ObjectPool-free.pseudo.md"

The slot of `make()` comes from one function, which also doubles the
blocks; `allocate()` gives a raw slot from the same function, for an
object that the caller builds (as with `std::allocator`).

```cpp
--8<-- "generated/ObjectPool-slot.cpp"
```

If the constructor of the object throws, `make()` pushes the slot back on
$F$ before passing the exception on, and the pool is then as it was, but
for the order of its free list.

<span id="lem-5-5"></span>**Lemma 5.5.** *An `ObjectPool` with blocks of
at most $n$ slots that has held at most $k \ge 1$ objects at once (raw
slots of `allocate()` included) has at most
$\max( 2 k - 1 , k + n - 1 ) \le 2 k + n$ slots besides the headers, in
$O( \log \min( k , n ) + k / n )$ blocks. Each operation costs $O( 1 )$
time, but for the allocation of a block.*

*Proof.* A block is taken only when no slot is free, i.e., when all the
slots of the pool hold objects, and the new object goes into it. While
the blocks double, they are of $1 , 2 , \ldots , 2^j$ slots, and the last
one was taken with $2^j - 1$ objects alive plus the new one; hence,
$k \ge 2^j$, and the slots are $2^{j+1} - 1 \le 2 k - 1$. Once the
blocks have reached $n$ slots, the last block was taken with all the
slots before it in use. Then $k$ is at least the number of these slots
plus one, and the slots are at most $k - 1 + n$. The blocks double
$O( \log \min( k , n ) )$ times, and those of $n$ slots are at most
$k / n + 1$. Each operation is a pop or a push of $F$, or the bump of
$\mathit{next}$, besides the construction or the destruction of the
object. $\square$

The bound counts as held the object of a `make()` whose constructor
throws, whose slot was taken before the exception; with this convention it
also holds for a pool whose operations throw. It does not hold, instead,
for a pool that has adopted the blocks of another one (see below), which
keeps those blocks whatever it holds.

We remark that the slots of a pool go back to `A` only with the pool; a structure that
shrinks thus keeps the memory of its largest size. This is the price of a
free list with no header in the slots, since a block could be given back
only when all its slots are free, which the pool does not count. Also, the pool does not destroy the objects still
alive when it dies.
Their owner destroys them before (or need not, if their destructor is
trivial), as the containers of the library do with their nodes. By
default, the blocks come from `HugePageAllocator`
([§5.9](05-memory.md#sec-5-9)), which puts those of 2 MB or more on huge
pages, where the nodes of a "large" linked structure, reached in no
useful order, miss the translation lookaside buffer less often.

Unlike the `Arena`, the pool may be moved and swapped (the objects stay
where they are): the move constructor leaves the other pool empty, the
move assignment frees the blocks of the pool before taking those of the
other one, and `swap()` exchanges the slots of two pools in $O( 1 )$ time.
Furthermore, `adopt(other)` takes the blocks of `other` with the objects
alive in them, which become objects of this pool, to be freed by it, and
leaves `other` empty; the lists of the library use it to take the nodes of
another list with no copy (`absorb()` in [§2.10](02-sequence.md#sec-2-10)). If this
pool has no block, it is a `swap()`, which brings here also the limit $n$
of `other`, and the free slots of `other` stay free here. Otherwise, the chain of the blocks of `other` is hung under the
first block of this pool, which is found by a walk of the chain, in time
linear in the number of the blocks of this pool (cf.
[Lemma 5.5](05-memory.md#lem-5-5)); the largest block becomes the larger
of the two limits, and the free slots of `other`, and those it never used,
are no longer given until the pool dies. In `adopt()` the two allocators must be equal, since the blocks of `other` go back to `A` with this pool. Finally,
`size()` is the number of the objects alive, and `bytes()` the memory of
the blocks, headers included, computed by a walk of the chain.

## 5.6 Size classes {#sec-5-6}

A *size class* serves all the requests between two sizes with blocks of
the larger one. `SizeClassAllocator` serves the requests of
$1 \le n \le 32768$ bytes from 41 classes: the class of 8 bytes, the
multiples of 16 up to 128 (8 classes), and four classes for each power of
2 above,

$$
s = 2^e + j \, 2^{e - 2} , \qquad j = 1 , \ldots , 4 , \quad
7 \le e \le 14 , \tag{5.2}
$$

i.e., 160, 192, 224, 256, 320, 384, and so on up to 32768 (the constant
`LARGEST`). A request of more than 32768 bytes goes to `operator new`. The deallocation is sized,
since the caller gives back the size of the request (as `std::allocator`
does), and a block thus needs no header.

Each class $c$ has a free list $F_c$ of the blocks given back, the last
one first, and a page whose rest $[ \mathit{next}_c , \mathit{end}_c )$ is
cut a block at a time. A request takes the first block of $F_c$, or the
next one of the page, or a new page of $\max( 2^{16} , s + 16 )$ bytes
from `operator new`. The first 16 bytes of a page hold its size and link
it to the pages taken before ([Algorithm 5.5](05-memory.md#alg-5-5) and
[Figure 5.5](05-memory.md#fig-5-5)); since $s \le 2^{15}$, every page has
in fact $2^{16}$ bytes (the constant `PAGE`). A deallocation pushes the
block on $F_c$. The blocks are aligned to 16 bytes, but for those of the
class of 8 bytes, which are aligned to 8, as long as `operator new` aligns
its memory to 16 bytes (as it does on the usual platforms of 64 bits),
since the header and the sizes of the other classes are multiples of 16.

<span id="alg-5-5"></span>**Algorithm 5.5.** A request to
`SizeClassAllocator` (the lists of available blocks of [Knuth 1997,
§2.5](Z-bibliography.md#ref-knuth1997), by size class); $F_c$ holds the
blocks given back of the class $c$ and $[ \mathit{next}_c ,
\mathit{end}_c )$ is the rest of its page.

--8<-- "generated/SizeClassAllocator-allocate.pseudo.md"

![Figure 5.5. The page of the class 2 (blocks of 32 bytes) of a `SizeClassAllocator`, after its header: before, the block 1 is on $F_2$ and the blocks from 3 on were never given; `allocate(20)` pops the block 1, and `allocate(25)` cuts the block 3 from the page. The violet cell is the block given, those in the faintest tint are free.](../figures/memory-sizeclass.svg){#fig-5-5}

<span id="lem-5-6"></span>**Lemma 5.6.** *A request of $n > 128$ bytes
to `SizeClassAllocator` wastes less than 20% of its block, and less than
25% of the request; one of $n \le 128$ bytes wastes less than 16 bytes.
The class of $n$ is computed in $O( 1 )$ time with no table and no loop,
and both operations cost $O( 1 )$ time, but for the allocation of a
page.*

*Proof.* Let $2^e < n \le 2^{e+1}$, $e \ge 7$. The classes of (5.2)
between $2^e$ and $2^{e+1}$ are $2^{e-2}$ apart, and $n$ gets the least
one not below it, i.e., $2^e + j \, 2^{e-2}$ with
$j = \lceil ( n - 2^e ) / 2^{e-2} \rceil$. The waste is less than
$2^{e-2}$, while the block is at least $5 \cdot 2^{e-2}$ and the request
is larger than $2^e$, whence the two bounds. Up to 128 bytes the classes
are 16 bytes apart (8 bytes for the first one). The exponent $e$ is
$\lfloor \log_2 ( n - 1 ) \rfloor$, the position of the leading bit of
$n - 1$, which the processor gives in one instruction (through
`std::bit_width`), and $j$ is a shift. Each operation is then the pop or
the push of a list, or the bump of a pointer. $\square$

Indeed, the code follows the proof step by step. Conversely,
`classSize(c)` gives 8 for $c = 0$, $16 c$ for $1 \le c \le 8$, and
$2^e + j \, 2^{e-2}$ with $e = 7 + \lfloor ( c - 9 ) / 4 \rfloor$ and
$j = ( c - 9 ) \bmod 4 + 1$ for $9 \le c \le 40$.

```cpp
--8<-- "generated/SizeClassAllocator-sizeClass.cpp"
```

When the rest of the current page of a class is shorter than a block, the
class takes a new page and the rest is lost (less than one block a page).
The pages go back to the system only with the allocator, or by
`release()`, which invalidates all the blocks given; `bytes()` is the
memory of the pages taken. This allocator cannot be copied, since its
blocks are in its pages, but it can be moved: the move leaves the other
allocator with no page, and the move assignment releases the pages of the
allocator before taking those of the other one. While the allocator
lives, its memory is thus not returned, as in a pool; however, the blocks
given back serve any later request of their class. This is the scheme of
TLSF ([Masmano et al. 2004](Z-bibliography.md#ref-masmano2004)) and of
the allocators of today, which choose the block by its size class instead
of by a fit, in constant time.

## 5.7 Free lists by page {#sec-5-7}

The single free list of a class (as that of `ObjectPool` or of
`SizeClassAllocator`) gives the blocks back in the order in which they
were freed. After frees in a scrambled order (e.g., those of a hash table
or of a search tree that is emptied), the next blocks given are scattered
over the memory of the class, and the nodes of a structure built again are
far from each other. The free lists *sharded* by page of mimalloc ([Leijen
et al. 2019](Z-bibliography.md#ref-leijen2019)) give them back a page at a
time instead.

`PageHeap<C>` (in the namespace `ADS::detail`) keeps $C$ size classes of
slots of $( c + 1 ) g$ bytes, $c = 0 , \ldots , C - 1$. It takes
*segments* of $2^{21}$ bytes, aligned to their size and advised for the
huge pages ([§5.9](05-memory.md#sec-5-9)), and cuts them into *pages* of
$2^{16}$ bytes. A page holds the slots of one class after a header of 64
bytes. The header keeps the free list of the page, its first slot never
given, the end of its slots, its class and its owner; it also keeps the
link of the page on the list of the pages of its class with free slots,
and whether the page is *listed* (current, or on that list). Since the
pages are aligned to their size, the page of a slot $p$ is found by
masking the low 16 bits of its address,

$$
\mathrm{pageOf}( p ) = p \wedge \neg ( 2^{16} - 1 ) , \tag{5.3}
$$

and a free needs neither a header in the slot nor its size. In the class, the constants `GRAIN`, `PAGE`, `SEGMENT` and `HEADER` are $g = 8$,
$2^{16}$, $2^{21}$ and 64, the static function `pageOf(p)` is (5.3), and
two queries give the slots given but not yet freed (`live()`) and the memory
of the segments (`bytes()`). Each heap takes as argument an *owner*, which
it writes in the header of each of its pages, and it can be neither copied
nor moved, since its pages name it.

![Figure 5.6. A `PageHeap`: a segment cut into pages of one class each, the current page P3 of the class 1 and its list $A_1$ of the pages with free slots, before and after the free of a slot of the exhausted page P1, which puts P1 back on $A_1$; below, a page with its header, the slot $p$ finding its page by (5.3).](../figures/memory-pageheap.svg){#fig-5-6}

A class allocates from its current page, by the pop of the free list of
the page or by the bump of its pointer. When the page is exhausted, it is
marked as not listed, and the class moves to the next page of its list
$A_c$, or to a new page cut from the segment (a new segment when it is
used up). A free pushes the slot on the list of its page, and puts the
page back on $A_c$ if it was not listed
([Algorithm 5.6](05-memory.md#alg-5-6) and
[Figure 5.6](05-memory.md#fig-5-6)). These are the slabs of [Bonwick
1994](Z-bibliography.md#ref-bonwick1994), whose slabs of one size keep a
free list each and whose partly free slabs are on a list apart.

<span id="alg-5-6"></span>**Algorithm 5.6.** The operations of `PageHeap`
(the free lists by page of [Leijen et al.
2019](Z-bibliography.md#ref-leijen2019) and the slabs of [Bonwick
1994](Z-bibliography.md#ref-bonwick1994)); $P$ is the current page of the
class $c$ and $A_c$ the list of its other pages with free slots.

--8<-- "generated/PageHeap-local.pseudo.md"

--8<-- "generated/PageHeap-more.pseudo.md"

--8<-- "generated/PageHeap-give.pseudo.md"

```cpp
--8<-- "generated/PageHeap-give.cpp"
```

<span id="lem-5-7"></span>**Lemma 5.7.** *A page on the list $A_c$ has at
least one free slot. Hence `more()` makes at most two rounds, and each
operation of `PageHeap` costs $O( 1 )$ time in the worst case, but for
the allocation of a segment (one every 32 pages), which also appends it to the
vector of the segments; a page of the class $c$ wastes its header and less
than $( c + 1 ) g$ bytes at its end.*

*Proof.* A page enters $A_c$ only in `give()`, when a slot is pushed on
its free list while the page is not listed. A page is not listed only
after `more()` has found it exhausted, when it was current; from then on,
no slot of it is given until it is popped from $A_c$ and becomes current
again, while the frees only add to its list. A new page has at least 8
slots, since the classes are at most $( 2^{16} - 64 ) / 8$ bytes (a static
assertion checks it). Therefore, the round of `more()` after a page was
popped from $A_c$, or cut new, finds a slot. (At the start, every class
has as current page a sentinel with no slot, which the first round leaves
at once.) A page enters $A_c$ only when it is not listed, and is listed
from then on; hence, no page is twice on $A_c$, nor on $A_c$ while it is
current, and each operation on $A_c$ is the pop or the push of a stack.
A page has $\lfloor ( 2^{16} - 64 ) / b \rfloor$ slots of $b$ bytes, and
the rest is less than $b$. $\square$

The vector of the segments grows by one place at a time, and the append of
the $s$-th segment may thus copy the $s - 1$ addresses before it; this cost
belongs to the allocation of a segment (cf. [Lemma 5.7](05-memory.md#lem-5-7)), and is
negligible next to the 32 pages that each segment serves.

`PoolResource<M>` serves the requests of $1 \le b \le M$ bytes aligned to
$a \le 16$ (the test `pooled(b, a)`) from a `PageHeap` of $M / 8$
classes, where $M$ is a multiple of 16 between 16 and 4096 (256 by
default); the constants `GRAIN`, `ALIGN`, `MAX` and `CLASSES` are $g = 8$,
16, $M$ and $M / 8$. A request aligned to at most
8 bytes takes a slot of $8 \lceil b / 8 \rceil$ bytes, and one aligned to
16 takes a slot of $16 \lceil b / 16 \rceil$ bytes, whose class has its
slots aligned to 16 (the header has 64 bytes, and the pages are aligned to
$2^{16}$). Any other request, larger or more aligned, goes to `operator
new` ([Algorithm 5.7](05-memory.md#alg-5-7)), with the alignment $a$ when
it exceeds that of a plain `operator new`. The internal waste is thus less
than 8 bytes a slot for $a \le 8$ and less than 16 for $a = 16$, i.e., less
than $\max( a , g )$, and the nodes of the containers, aligned to 8, waste
no byte when their size is a multiple of 8. A request takes one of two
forms: `allocate<b, a>()` and `deallocate<b, a>(p)`, whose size and
alignment are known at compile time, so that the test and the class are
computed by the compiler, and `allocate(b, a)` and `deallocate(p, b, a)`,
which compute them at each call. In both, $a$ is by default the alignment
of `std::max_align_t`, i.e., 16 on the usual platforms of 64 bits, and a
request that does not give it thus takes the classes of 16 bytes. As a precondition, the deallocation checks that the page of $p$ belongs to
the resource and has the class of the request. Two queries give the
objects alive in the classes (`size()`) and the memory of the segments
(`bytes()`); the resource cannot be copied (nor moved), since its objects
and its allocators point to it, and it frees its segments when it dies,
without destroying the objects still alive. The heap is kept apart
from the resource, behind a pointer: its state, read and written at each
operation, then never shares a line of cache with the variables around
the resource (e.g., on a stack).

<span id="alg-5-7"></span>**Algorithm 5.7.** A request to
`PoolResource`; $c$ is the class of the request.

--8<-- "generated/PoolAllocator-PoolResource-allocate.pseudo.md"

`PoolAllocator<T, R>` is the allocator of the standard library over the
resource `R`, which is `PoolResource` or the `ConcurrentPoolResource` of
[§5.8](05-memory.md#sec-5-8). It holds a pointer to its resource, never
nil, given to the constructor `PoolAllocator(r)`, or taken from `R::global()`
by the default constructor, which exists only when `R` has that function
(i.e., for `ConcurrentPoolResource`); `resource()` returns it. Two
allocators, also of different types, are equal when they share their
resource, and `is_always_equal` is therefore false. It is rebound to
any type by `std::allocator_traits`, e.g., to a node of `std::list`,
`std::map` or `std::unordered_map`, or to the control block of
`std::allocate_shared` (which then holds the counts and the object in one
slot). A container copies, moves and swaps its allocator with its
contents, since the three `propagate_on_container` traits are true; a
node therefore always goes back to the pool it came from. A request of
one object (the case of the node containers) finds its class at compile
time, the size and the alignment of `T` being constants, and costs the pop
of a free list; a request of $n$ objects (e.g., the buckets of
`std::unordered_map`) finds it from $n$. Such a request throws
`std::bad_array_new_length` if $n \, \mathrm{sizeof}( T )$ would overflow a
`std::size_t` (a request of no object goes to `operator new`, as any
request out of the classes). An allocator of a type `U` is built from that
of `T` over the same resource, as `rebind` and `std::allocator_traits`
require. Neither the resource nor its
allocators synchronize: one thread at a time uses a `PoolResource`, which
must outlive the objects and the allocators made from it.

## 5.8 A pool shared by threads {#sec-5-8}

`ConcurrentPoolResource<M>` has the classes of `PoolResource`, and an
object made by one thread may be freed by any other, e.g., a message made
by the thread that changes a model and released by the thread of a solver
that has read it. Each thread that uses the resource has a *heap*, i.e., a
`PageHeap` whose pages name the heap as their owner (the heaps of the
threads of Hoard, [Berger et al. 2000](Z-bibliography.md#ref-berger2000)).
For each class, a heap also has a *batch* of the slots that its thread has
freed for another heap, and a stack of the *returns*, i.e., of the slots
that other threads have freed into it. A thread finds its heap through a
cache in its thread-local storage, at the price of one comparison with
the identity of the resource. The first time, it finds the heap on a list
of the heaps, which grows by a compare-and-swap (CAS) and is read with no
lock; the heaps leave the list only when the resource dies.

A free of a pointer $p$ finds its page by (5.3), and then the owner of
the page. If the owner is the heap of the calling thread, the slot goes on
the free list of its page with no atomic operation; a thread that frees
what it made thus pays what it pays in `PoolResource`. Otherwise, the
slot joins the batch of the calling thread for its class, which holds
slots of one owner only. The batch goes back to its owner by one CAS,
which links it on the top of the stack of the returns of that class (a
stack of [Treiber 1986](Z-bibliography.md#ref-treiber1986)). This happens
when the batch has 32 slots, when a slot of another owner arrives, or when
the current page of the class of the calling thread is exhausted or that
thread calls `collect()`
([Algorithm 5.8](05-memory.md#alg-5-8) and
[Figure 5.7](05-memory.md#fig-5-7)). When the current page of a class is
exhausted, the owner takes all the stack at once, by exchanging its top
with nil, and puts each slot back on the list of its page. These are the
batches of snmalloc ([Lietar et al.
2019](Z-bibliography.md#ref-lietar2019)), on the free lists by page of
mimalloc ([Leijen et al. 2019](Z-bibliography.md#ref-leijen2019)).

![Figure 5.7. The batch of the thread B for the class $c$, with two slots of the heap of A, and the stack of the returns of A for $c$, before and after `send()`, which links the batch on the top of the stack by one CAS (the orange slots).](../figures/memory-concurrent.svg){#fig-5-7}

<span id="alg-5-8"></span>**Algorithm 5.8.** The operations of
`ConcurrentPoolResource` (the heaps of the threads of [Berger et al.
2000](Z-bibliography.md#ref-berger2000) and the batches of [Lietar et al.
2019](Z-bibliography.md#ref-lietar2019)); $H$ is the heap of the calling
thread, $c$ the class of the request and $B$ the batch of $H$ for $c$.

--8<-- "generated/ConcurrentPool-ConcurrentPoolResource-allocate.pseudo.md"

--8<-- "generated/ConcurrentPool-ConcurrentPoolResource-deallocate.pseudo.md"

--8<-- "generated/ConcurrentPool-ConcurrentPoolResource-collect.pseudo.md"

Since the CAS of `send()` has the order *release*, the links written into
the slots of the batch are visible to the owner, which takes the stack
with the order *acquire*.

```cpp
--8<-- "generated/ConcurrentPool-give.cpp"
```

```cpp
--8<-- "generated/ConcurrentPool-send.cpp"
```

<span id="lem-5-8"></span>**Lemma 5.8.** *In `ConcurrentPoolResource`,
(i) a free by the owner of the slot makes no atomic operation, and the
frees by the other threads make one successful CAS per batch; (ii) each
thread holds at most 31 slots per class that it has freed and that are
not yet back to their owner; (iii) the stacks of the returns are free of
the ABA problem; (iv) an allocation costs $O( 1 )$ amortized time. In
(iv), we leave out the allocation of a segment and the search of the heap
on the list, which a thread makes at its first call and after each call to
another resource of the same $M$.*

*Proof.* (i) and (ii): a batch is sent when it reaches 32 slots, and
otherwise holds at most 31. (iii): a CAS of a sender replaces the top $t$
that it has read with the head of its batch, after linking the tail of
the batch to $t$. If the top is $t$ again at the time of the CAS (possibly
after the owner has taken the stack and other senders have pushed), the
link to $t$ is still correct, since the new value does not depend on the
next field of $t$; the only removal is that of all the stack by an
exchange, which reads no link. (iv): when the cache of the thread holds the
resource, the heap is found by one comparison. The slots taken from a
stack are pushed on the lists of their pages in time linear in their
number, and each slot is taken from a stack at most once per free; we
thus charge this work to the frees. Apart from this, there remain a send in $O( 1 )$ time and
at most one call of `more()`, which costs $O( 1 )$ time but for a segment
by [Lemma 5.7](05-memory.md#lem-5-7). $\square$

The memory orders complete (iii). A sender writes the links of its batch
with plain stores, before its CAS with the order *release*; a CAS that
fails reads the top with the order *relaxed*, and the loop writes the link
of the tail again before the next attempt. In turn, the successful CAS of the senders are read-modify-write operations on the same top, and they form a
release sequence; hence, the exchange of the owner, with the order
*acquire*, synchronizes with every sender whose batch it takes, and sees
all the links of the stack. The list of the heaps grows in the same way,
by a CAS with the order *release*, and is read with the order *acquire*.

A heap is never destroyed before the resource. A thread that ends leaves
its heap, whose returns keep coming, to the next thread that has the same
identity (`std::thread::id`); its memory goes back to the system with the
resource. The slots freed into the heap of a thread that has ended thus
stay there until such a thread comes. Also, the cache of the thread-local
storage holds one resource for each value of $M$. A thread that
alternates between two resources of the same $M$ walks the list of the
heaps at each change, in time linear in the number of heaps. Since the
identities of the resources come from a counter and are never reused, the
cache never points to the heap of a resource that has died. A thread that
ends also leaves in its batches up to 31 slots per class, which go back to
their owners only when the next thread with the same identity sends
them. Finally, `global()` gives a resource that lives as long as the
program (it is allocated once and never destroyed), the default one of
`PoolAllocator<T, ConcurrentPoolResource<>>`.

Like `PoolResource`, this resource has the constants, the test `pooled()` and the two forms of `allocate()` and
`deallocate()`, and also `SEGMENT`, the $2^{21}$ bytes of a segment of
its heaps. Its query `heaps()` is the number of its heaps, i.e., of
the identities of the threads that have used it, and `bytes()` the memory
of their segments, both read by a walk of the list of the heaps while no
other thread uses the resource. It cannot be copied, and its destructor
frees the heaps and their segments, without destroying the objects still
alive.

## 5.9 Huge pages {#sec-5-9}

A structure whose nodes are reached in no useful order (e.g., the trees of
a heap or the buckets of a hash table) misses the translation lookaside
buffer (TLB) at nearly each step, when its memory spans more pages than
the TLB has entries. A TLB of $E$ entries covers $4 E$ KB of pages of
4 KB, and $2 E$ MB of huge pages of 2 MB, i.e., 512 times more. On Linux,
the kernel gives the huge pages to a region aligned to 2 MB and advised
with `madvise(MADV_HUGEPAGE)` (the transparent huge pages of [Navarro et
al. 2002](Z-bibliography.md#ref-navarro2002)), although it may ignore the
advice. `HugePageAllocator<T>` gives a request of at least one huge page
($P = 2^{21}$ bytes) a block aligned to $P$, rounded up to whole huge
pages and advised for the huge pages; a smaller request goes to
`std::allocator` ([Algorithm 5.9](05-memory.md#alg-5-9) and
[Figure 5.8](05-memory.md#fig-5-8)). Elsewhere than on Linux, the large
requests are only aligned; in any case, the allocators have no state, and
are all equal, also when they are of different types, the allocator of
`T` being built from that of any `U`. Its constant `PAGE` is $P$, and a
request of $n$ objects throws `std::bad_array_new_length` when its
rounding to whole huge pages would overflow a `std::size_t`.

<span id="alg-5-9"></span>**Algorithm 5.9.** A request to
`HugePageAllocator` ([Navarro et al.
2002](Z-bibliography.md#ref-navarro2002)); $P = 2^{21}$ is the size of a
huge page in bytes.

--8<-- "generated/HugePageAllocator-allocate.pseudo.md"

![Figure 5.8. A request of $b = 5$ MB to `HugePageAllocator`: the block is aligned to a huge page of $P = 2$ MB and rounded up to $r = 6$ MB, whose last MB of address space is never touched.](../figures/memory-hugepage.svg){#fig-5-8}

A block of 8 MB or more given back is kept by its thread, up to 16 blocks
and 64 MB in all, and is given again to the next request of the same
rounded size, as the allocators by size class keep their freed spans
([Berger et al. 2002](Z-bibliography.md#ref-berger2002); [Leijen et al.
2019](Z-bibliography.md#ref-leijen2019)). Its pages are already faulted
in and made huge. A structure that is built and destroyed again and
again, or that grows through the same sizes (a vector that doubles, a pool
whose blocks double), therefore pays the faults of its pages once. A
smaller block goes back to the allocator of the system, which serves it
again from its heap. The blocks kept go back to the system when the
thread ends, after which the blocks given back by the thread go to the
system at once (the objects of static storage may die later than the
cache). As any memory of an allocator, a block given again holds what its
last owner left in it.

<span id="lem-5-9"></span>**Lemma 5.9.** *A block of `HugePageAllocator`
of $b \ge P$ bytes takes less than $P$ bytes of address space beyond
$b$, none of which is touched by the allocator, and each thread keeps at
most 16 blocks and 64 MB given back.*

*Proof.* The rounding gives $r = P \lceil b / P \rceil < b + P$ bytes,
and the allocator writes nothing in the block (the advice to the kernel
reads no page). The cache refuses a block when it has 16 already, when the
bytes kept would exceed 64 MB, or when the block is smaller than 8 MB.
$\square$

It is fair to remark that the aligned `operator new` of the C library may
reserve some more address space for the alignment, depending on its
implementation. This memory is not touched either, and costs no physical
memory.

## 5.10 Garbage collection {#sec-5-10}

In `CellHeap` the program never frees a block. The memory is a vector of
$n$ cells of two fields, `car` and `cdr` (the memory of a LISP), each
field holding nil, an *atom* (the integer $k \ge 0$, kept as $-2 - k$) or
the address of a cell (an index $\ge 0$). A set of *roots* names the cells
that the program holds; it is a multiset, since a cell may be a root more
than once, and the cells not reachable from the roots are *garbage*.
`cons(a, d)` takes a cell from the free list, which is linked through the
fields `car`, and calls `collect()` when the list is empty. In turn,
`collect()` marks the cells reachable from the roots and sweeps the
others onto the free list ([Algorithm 5.10](05-memory.md#alg-5-10)).

A field is of the type `CellHeap::Value` (a `long`); the constant `nil` is
$-1$, `atom(k)` gives the atom $k \ge 0$ and `isCell(v)` is true when $v$ is the address of a cell. We read the fields of a cell in use by `car(c)`
and `cdr(c)`, and write them by `setCar(c, v)` and `setCdr(c, v)`, whose
precondition is that $c$ is a cell of the heap in use; `addRoot(c)`
adds one more occurrence of the cell $c$ to the roots, and `removeRoot(c)`
removes one of them, if there is one (and does nothing otherwise).
Finally, `freeCells()` is the number of the free cells, and `collect()`
returns the number of the cells that it frees, i.e., of the cells in use before it that are unreachable. The heap relies on the program for what it
does not check: the fields of the cells in use and the roots hold only nil,
atoms and cells in use (a free cell named by a root, for instance, would
bring the free list into the marking).

A marking by a visit in depth needs a stack as deep as the longest path of
cells, which a collector (run when the memory is exhausted) cannot
allocate. The marking of Deutsch, Schorr and Waite ([Schorr and Waite
1967](Z-bibliography.md#ref-schorr1967); [Knuth 1997, Algorithm
2.3.5E](Z-bibliography.md#ref-knuth1997)) keeps the path of the visit in
the fields of the cells themselves. It reverses each pointer that it
follows and restores it on the way back, with one bit per cell
($\mathit{side}$) for the field that holds the reversed pointer
([Figure 5.9](05-memory.md#fig-5-9)).

![Figure 5.9. Three cells $a$, $b$, $d$ reachable from a root, and the same cells during the marking by pointer reversal, when the visit has gone down the `car` of $a$ and of $b$ and stands at the atom 1 (the current value $c$) with $b$ as the previous cell ($p$); the dashed violet arrows are the reversed pointers, i.e., the path back to the root, and the violet cells are marked.](../figures/memory-cellheap.svg){#fig-5-9}

<span id="alg-5-10"></span>**Algorithm 5.10.** The operations of
`CellHeap` ([Knuth 1997, §2.3.5 and Algorithm
2.3.5E](Z-bibliography.md#ref-knuth1997)); $F$ is the free list, linked
through the field `car`, and $\mathit{side}( x )$ is true when the
reversed pointer of the cell $x$ is in its `cdr`.

--8<-- "generated/CellHeap-cons.pseudo.md"

--8<-- "generated/CellHeap-collect.pseudo.md"

<span id="lem-5-10"></span>**Lemma 5.10.** *During the marking by pointer
reversal, let $\mathrm{back}( x )$ be the field of the cell $x$ named by
$\mathit{side}( x )$. Then the sequence*

$$
p , \; \mathrm{back}( p ) , \; \mathrm{back}( \mathrm{back}( p ) ) ,
\; \ldots \tag{5.4}
$$

*ends with nil and is, in reverse, the path of the visit from the root to
$p$, the cell whose field holds the current value $c$ (a cell, an atom or
nil). Each cell $x$ of the path held, before the visit, the next cell of
the path (or $c$, for $x = p$) in its field $\mathrm{back}( x )$, and the
other fields of the cells of the path, and all the fields of the other
cells, hold their values before the visit.*

*Proof.* By induction on the three moves of the marking (cf.
[Algorithm 5.10](05-memory.md#alg-5-10)). Going down the `car` (line
12), the visit stores $p$ in the `car` of $c$, with $\mathit{side}( c )$
false, and makes $c$ the new $p$; the path grows by $c$, whose `car` held
the new current cell. Going up from a `cdr` (line 14), the visit restores
in the `cdr` of $p$ the cell it comes from and moves to
$\mathrm{back}( p )$; the path loses its last cell, whose fields are now
both restored. Moving from the `car` to the `cdr` of $p$ (line 17), the
visit restores the `car` of $p$, puts $\mathrm{back}( p )$ in its `cdr`
and sets $\mathit{side}( p )$. The path is then the same, and the field
that holds the next cell is now the `cdr`. Initially, $p$ is nil and $c$ is
the root, so that the sequence (5.4) is empty and the statement holds. Finally, the three moves are the only writes of the marking, and each one changes only
the fields named above, which proves the last part. $\square$

<span id="thm-5-11"></span>**Theorem 5.11.** *`collect()` marks exactly
the cells reachable from the roots, leaves their fields as they were, and
frees the others. The marking takes time linear in the cells reached,
with no space besides one mark bit and one side bit per cell, and a
collection costs $O( n + r )$ time on a heap of $n$ cells with $r$
occurrences of roots.*

*Proof.* By [Lemma 5.10](05-memory.md#lem-5-10), the marking of a root
ends when $p$ is nil, i.e., with an empty path, and hence with all the
fields restored. A cell is marked when the visit enters it, and a marked
cell is never entered again; hence, only reachable cells are marked, since
the visit only follows fields. A cell entered is left towards its `cdr`
once and left upwards once (the side bit distinguishes the second time from the first), and each move costs $O( 1 )$ time; the marking thus takes time
linear in the cells marked, besides one step for each root. Conversely,
the visit leaves a cell upwards only after it has followed both its
fields, the `car` by the first loop and the `cdr` by the move of line 17;
hence, when it ends, each field of a marked cell holds nil, an atom or a
marked cell. By induction on the length of a shortest path from a root,
each cell reachable from a root is thus marked (by this marking or by that
of an earlier root). Finally, the sweep reads the $n$ cells, unmarks the
marked ones and pushes the others on the free list, from the last cell to
the first, so that the free list is in the order of the addresses. A
collection thus costs $O( n + r )$ time for $r$ occurrences of roots,
i.e., $O( n )$ when the roots are $O( n )$. $\square$

The code of the marking is made of three loops, one for each move of the
induction.

```cpp
--8<-- "generated/CellHeap-markReversal.cpp"
```

A `cons()` costs $O( 1 )$ time, but for the collection. If each
collection frees at least a fraction $\alpha$ of the heap, the $O( n )$
time of the collection is spread over the $\alpha n$ calls of `cons()`
that follow it, i.e., $O( 1 / \alpha )$ amortized time per call. When the
reachable cells fill the heap, `cons()` fails its precondition after the
collection, since no cell can be freed. For the comparison, the class also keeps the
marking with an explicit stack (`collect(false)`). At the highest level
of the contracts, `collect()` checks that it frees exactly the cells that
are not reachable (found by a plain visit), and that it leaves the fields
of the others as they were.

## 5.11 The allocators compared {#sec-5-11}

The allocators of the chapter are not interchangeable. Those of
[§5.2](05-memory.md#sec-5-2) and [§5.3](05-memory.md#sec-5-3) work on a
simulated memory, and are meant (rather) for the study of the methods (the fits,
the fifty-percent rule, the buddies) more than for serving a program; the
`CellHeap` of [§5.10](05-memory.md#sec-5-10) is in the same position for
the garbage collection. Among the others, the choice depends on how the
objects die. If they die all together, an `Arena` costs no header and no work per
free. If they die one at a time and are all of one type, an
`ObjectPool` costs six words, with no header in its slots. If they are of
a few sizes and die one at a time in the thread that made them,
`SizeClassAllocator` serves them, and `PoolResource` (through
`PoolAllocator`) serves the containers of the standard library, giving the
slots back a page at a time after scrambled frees. If another thread frees
them, `ConcurrentPoolResource` is needed. Finally, `HugePageAllocator` is
the allocator of the blocks under the pools and the vectors of the large
structures. [Berger et al. 2002](Z-bibliography.md#ref-berger2002) found
that most custom allocators of real programs are not faster than a good
general allocator, with the exception of the regions (our arenas); one can
thus expect the gains of the pools to come from the locality more than
from the cost of the operations. The
comparison of our allocators with the allocator of the system and with
mimalloc, on the workloads of the containers of the library, belongs to
Appendix B, where it will be made on an idle machine.

## 5.12 Discussion and references {#sec-5-12}

The dynamic storage allocation of [Knuth 1997,
§2.5](Z-bibliography.md#ref-knuth1997), with the fits, the boundary tags,
the buddy system of [Knowlton 1965](Z-bibliography.md#ref-knowlton1965)
and the fifty-percent rule, is the root of the chapter. The survey of
[Wilson et al. 1995](Z-bibliography.md#ref-wilson1995) is the reference
for what came after it; among other things, it argues that the
simulations with random sizes and lifetimes are a poor model of real
programs, which usually allocate few sizes and free their blocks in
patterns. The arenas are those of [Hanson
1990](Z-bibliography.md#ref-hanson1990), the free list of the pools is the
list AVAIL of [Knuth 1997, §2.2.3](Z-bibliography.md#ref-knuth1997), and
the size classes with constant-time operations are in TLSF ([Masmano et
al. 2004](Z-bibliography.md#ref-masmano2004)). The structure of
[§5.7](05-memory.md#sec-5-7) and [§5.8](05-memory.md#sec-5-8) comes from
the slabs of [Bonwick 1994](Z-bibliography.md#ref-bonwick1994), the heaps
per thread of Hoard ([Berger et al.
2000](Z-bibliography.md#ref-berger2000)), the free lists sharded by page
of mimalloc ([Leijen et al. 2019](Z-bibliography.md#ref-leijen2019)) and
the batches of remote frees of snmalloc ([Lietar et al.
2019](Z-bibliography.md#ref-lietar2019)), on the lock-free stack of
[Treiber 1986](Z-bibliography.md#ref-treiber1986). The transparent huge
pages are those of [Navarro et al. 2002](Z-bibliography.md#ref-navarro2002),
and the marking by pointer reversal is that of [Schorr and Waite
1967](Z-bibliography.md#ref-schorr1967), in the form of [Knuth 1997,
Algorithm 2.3.5E](Z-bibliography.md#ref-knuth1997). We do not treat here
the copying and the generational collectors, nor the collectors that run
concurrently with the program, since the library has none of them.

## 5.13 Exercises {#sec-5-13}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize the dynamic storage allocation with boundary tags, with the
   first, best and next fit, in which a free merges with both its free
   neighbors in $O( 1 )$ time; realize the buddy system on $2^m$ words.
   Check on random sequences of requests and frees that the memory is
   one free block again at the end. Test: `taocp.allocators`.
2. Realize a mark-and-sweep collector on cells of two fields, whose
   marking uses no stack (the pointer reversal of Deutsch, Schorr and
   Waite), and check on random structures with cycles and sharing that it
   frees the same cells as a marking with a stack. Test:
   `taocp.cell-heap`.
3. Realize an arena of objects of one type whose objects never change
   their addresses, and which destroys them all with the arena, also when
   a constructor throws. Test: `engineered.arena`.
4. Realize a pool of objects of one type, with blocks that double up to a
   limit, in which the last slot freed is the first one used again and the
   slot of a constructor that throws stays free. Tests: `engineered.pool`;
   for the raw slots of `allocate()` and `deallocate()`,
   `memory.pool-raw-slots`.
5. Realize an allocator by size classes whose class is computed with no
   table and no loop, and whose waste is less than 20% of a block past 128
   bytes. Test: `memory.size-class-allocator`.
6. Realize a standard allocator over pages with free lists by page, and
   use it in `std::list`, `std::map`, `std::unordered_map` and
   `std::allocate_shared`, with the allocator copied, moved and swapped
   with the containers. Tests: `engineered_memory.pool-resource`,
   `engineered_memory.pool-allocator-containers`,
   `engineered_memory.pool-allocator-propagation`,
   `engineered_memory.pool-allocate-shared`.
7. Realize a pool whose objects may be freed by a thread other than the
   one that made them, with one CAS per batch of remote frees. Tests:
   `engineered_memory.concurrent-pool`,
   `engineered_memory.concurrent-pool-threads`.
8. Realize an allocator that gives the large blocks aligned to a huge
   page, and keeps the large blocks given back for the next request of the
   same size. Tests: `engineered.huge-page-allocator`,
   `memory.huge-page-cache`.

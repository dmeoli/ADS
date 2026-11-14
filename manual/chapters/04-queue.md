# 4. Queues and deques {#ch-4}

[Source: `include/ADS/queue/ArrayQueue.h`, `LinkedQueue.h`, `Dequeue.h`,
`RingDeque.h`, `UniqueQueue.h`, `RandomQueue.h`, `MinQueue.h`,
`MinDeque.h`, `RealTimeMinQueue.h`, `RealTimeMinDeque.h`,
`QueueAlgorithms.h`]

We are concerned in this chapter with the *queue*, i.e., a sequence to
which items are added at one end (its *back*) and from which they are
removed at the other (its *front*): the first item added is the first one
removed (FIFO, for "first in, first out"). We also treat the *double ended
queue*, or *deque*, which is changed at both ends. A computation needs a
queue when it serves its requests in the order of their arrival, as the
visit in breadth of a graph does with the nodes it reaches
([§14.5.2](14-graph.md#sec-14-5-2)); it needs a deque when some requests
must be served first, as in the algorithm of Pape and D'Esopo
([§14.8.1](14-graph.md#sec-14-8-1)). In `ADS/queue` we realize
the queue on a circular vector of fixed capacity
([§4.2](04-queue.md#sec-4-2)) and on a singly linked list
([§4.3](04-queue.md#sec-4-3)), and the deque on a bidirectional list
([§4.4](04-queue.md#sec-4-4)) and on a circular vector that doubles, which
also reads and writes its items by rank ([§4.5](04-queue.md#sec-4-5)).
Then [§4.6](04-queue.md#sec-4-6) and [§4.7](04-queue.md#sec-4-7) change
the discipline of the queue (no duplicates, and the item removed drawn at
random). In [§4.8](04-queue.md#sec-4-8) and [§4.9](04-queue.md#sec-4-9)
the queue and the deque also give their least item, first in constant
amortized time and then in constant time in the worst case;
[§4.10](04-queue.md#sec-4-10) collects the algorithms that reach a queue
through its operations only, and [§4.11](04-queue.md#sec-4-11) the
engineered variants. [Table 4.1](04-queue.md#tab-4-1) summarizes the
costs.

## 4.1 The interfaces {#sec-4-1}

The QUEUE of [Montresor 2025, §4.3.1](Z-bibliography.md#ref-montresor2025)
has four operations: `enqueue(v)` adds the item $v$ at the back,
`dequeue()` removes the item at the front and returns it, `top()` reads
it, and `empty()` is true when there is no item. Their meaning is fixed by
the axioms

$$
\begin{aligned}
& \mathrm{top}( \mathrm{enqueue}( \mathrm{create} , v ) ) = v , \qquad
\mathrm{top}( \mathrm{enqueue}( Q , v ) ) = \mathrm{top}( Q ) , \\
& \mathrm{dequeue}( \mathrm{enqueue}( Q , v ) ) =
\mathrm{enqueue}( \mathrm{dequeue}( Q ) , v ) ,
\end{aligned} \tag{4.1}
$$

the last two for $Q$ not empty, together with
$\mathrm{empty}( \mathrm{create} ) = \mathbf{true}$; `top()` and
`dequeue()` are undefined on the empty queue (cf. the concept `Queue` of
`ADS/ADT.h`, which requires the four operations and nothing else). In
words, an item added to a queue that is not empty does not change what
comes out next, and it comes out only after the items that were there
before it. Compared with (3.1), where the item just pushed is the one read
([§3.1](03-stack.md#sec-3-1)), the item just enqueued is read only when
the queue was empty; this is the whole difference between LIFO and FIFO.

In [Morin 2013, §1.2.1](Z-bibliography.md#ref-morin-cpp) a `Queue` has only
`add(x)` and `remove()`, and a *queueing discipline* decides which item
`remove()` takes. This chapter is about the FIFO discipline; the LIFO one
that of the stacks of [chapter 3](03-stack.md#ch-3), and the priority one,
which removes the least item, that of the priority queues of [chapter
9](09-priority.md#ch-9); `UniqueQueue` ([§4.6](04-queue.md#sec-4-6)) and
`RandomQueue` ([§4.7](04-queue.md#sec-4-7)) are two more. The DEQUE of the
same section has `addFirst(x)`, `removeFirst()`, `addLast(x)` and
`removeLast()`. It is a stack when only the first two are used, and a queue
when items enter by `addLast(x)` and leave by `removeFirst()`. In the
library these operations have the names of the standard library
(`push_front()`, `pop_front()`, `push_back()` and `pop_back()`, read by
`front()` and `back()`), and the concept `IndexedDeque` of
`ADS/sequence/IndexedList.h` requires them. The DEQUEUE of [Montresor 2025,
exercise 4.9](Z-bibliography.md#ref-montresor2025) calls them
`insertHead()`, `removeHead()`, `insertTail()` and `removeTail()`, read by
`head()` and `tail()`. Finally, the LIST of [Morin 2013,
§1.2.2](Z-bibliography.md#ref-morin-cpp) reads, writes, inserts and removes
the item of rank $i$, i.e., the item with $i$ items before it (the concept
`IndexedList`). Most of its realizations are in [chapter 2](#ch-2);
`RingDeque` is described here, since it is also a deque whose operations at
the ends cost $O( 1 )$.

Every class of the area names the type of its items `Item`, which the
concept `Queue` requires. Those that can be traversed also have the aliases
of a container of the standard library (`value_type`, `size_type`,
`difference_type`, `reference`, `const_reference`, `iterator` and
`const_iterator`, and `reverse_iterator` and `const_reverse_iterator` where
the iterators go backwards), with `begin()`, `end()`, `cbegin()` and
`cend()` (and `rbegin()` and `rend()` for `Dequeue` and `RingDeque`); hence
the algorithms and the ranges of the standard library run on them, and the
kind of each iterator is given below with its class. `LinkedQueue`,
`RingDeque` and the two real-time classes of [§4.9](04-queue.md#sec-4-9)
declare the five special members: the destructor destroys the items, a copy
is made item by item, and a move takes the representation in $O( 1 )$ time.
Their assignment `operator=` is a copy (or a move) followed by `swap()`,
which exchanges two representations in $O( 1 )$ time, and `operator==`,
where it is defined, compares the items in order. Each of the other classes
copies and moves as its members do (item by item for those on a list or on
stacks), but for `ArrayQueue`, whose vector can only be moved.

Throughout the chapter $n$ is the number of items, $m$ the number of
places of a vector, $h$ the place of the front in a circular vector, and
$A[ 0 .. m - 1 ]$ a circular vector, i.e., one in which the place 0
follows the place $m - 1$.

<span id="tab-4-1"></span>**Table 4.1.** The realizations of the QUEUE
and of the DEQUE in `ADS/queue` and the cost of their operations
(A: amortized; W: in the worst case); $n$ is the number of items, $m$ the
capacity.

| Realization | Representation | Space | Operations at the ends | Capacity |
|---|---|---|---|---|
| `ArrayQueue` | a circular vector | $\Theta( m )$ | $O( 1 )$ W | $m$, fixed |
| `LinkedQueue` | a singly linked list, with its first and last cells | $\Theta( n )$ | $O( 1 )$ W | none |
| `Dequeue` | a bidirectional list with a sentinel | $\Theta( n )$ | $O( 1 )$ W, at both ends | none |
| `RingDeque` | a circular vector of $2^j$ places that doubles | fewer than $2 n + 8$ places | $O( 1 )$ A at both ends, reads by rank $O( 1 )$ W | none |
| `UniqueQueue` | a bidirectional list with no duplicates | $\Theta( n )$ | $O( n )$ to enqueue, $O( 1 )$ to dequeue | none |
| `RandomQueue` | a vector, the item out drawn uniformly | $\Theta( n )$ | $O( 1 )$ A | none |
| `MinQueue` | two stacks with their minima | $\Theta( n )$ | $O( 1 )$ A, `min()` $O( 1 )$ W | none |
| `MinDeque` | two stacks with their minima, back to back | $\Theta( n )$ | $O( 1 )$ A at both ends, `min()` $O( 1 )$ W | none |
| `RealTimeMinQueue` | arrays with their minima, rotated by parts | $\Theta( n )$ | $O( 1 )$ W, `min()` $O( 1 )$ W | none |
| `RealTimeMinDeque` | arrays with their minima, rebuilt by parts | $\Theta( n )$ | $O( 1 )$ W at both ends, `min()` $O( 1 )$ W | none |

## 4.2 A queue on a circular vector {#sec-4-2}

A queue whose items stayed in the places from 0 on of a vector would move
all of them at each `dequeue()`, i.e., it would take $\Theta( n )$ time;
for the same reason an `ArrayStack` ([§3.2](03-stack.md#sec-3-2)) is a
poor queue. Instead, the solution of [Montresor 2025,
§4.3.3](Z-bibliography.md#ref-montresor2025) (and of [Knuth 1997,
§2.2.2](Z-bibliography.md#ref-knuth1997)) lets the front move instead.
`ArrayQueue` keeps the $n \le m$ items in a circular vector
$A[ 0 .. m - 1 ]$, with the front at the place $h$ and the item of rank
$i$ from the front, for $0 \le i < n$, at the place

$$
( h + i ) \bmod m . \tag{4.2}
$$

Hence the back is at the place $( h + n - 1 ) \bmod m$, and the next item
enters at $( h + n ) \bmod m$. A `dequeue()` advances $h$ by one place and
decrements $n$, while an `enqueue()` writes the place after the back and
increments $n$ ([Algorithm 4.1](04-queue.md#alg-4-1)). Thus the items run
through the vector as a band that wraps around its end ([Figure
4.1](04-queue.md#fig-4-1), which replays the operations of the figure of
[Morin 2013, §2.3](Z-bibliography.md#ref-morin-cpp) on $m = 6$ places). When
$n = m$ the queue is full, and its precondition refuses `enqueue()`. Indeed,
the capacity is fixed when the queue is built, which allocates the $m$
places once (and builds $m$ items by their default constructor). We remark
that the count $n$ is not redundant: a vector that kept only the places of
the front and of the back could not tell the full queue from the empty one,
since both have the two places equal. A common alternative leaves one place
always empty, and hence wastes it.

![Figure 4.1. An `ArrayQueue` of $m = 6$ places under the operations of the figure of [Morin 2013, §2.3](Z-bibliography.md#ref-morin-cpp), from the queue `a`, `b`, `c` at the places 2 to 4: the place that each `enqueue()` fills is in orange, the one that `dequeue()` reads in violet, the free places in the faintest tint; `enqueue(e)` wraps around to the place 0, and after `enqueue(g)` the queue is full, so that the `add(h)` of that figure, which there doubles the vector, is refused here.](../figures/queue-array.svg){#fig-4-1}

<span id="alg-4-1"></span>**Algorithm 4.1.** The operations of
`ArrayQueue` ([Montresor 2025, §4.3.3](Z-bibliography.md#ref-montresor2025));
$A[ 0 .. m - 1 ]$ is the circular vector.

--8<-- "generated/ArrayQueue-enqueue.pseudo.md"

--8<-- "generated/ArrayQueue-dequeue.pseudo.md"

<span id="lem-4-1"></span>**Lemma 4.1.** *If the items of an
`ArrayQueue` are at the places (4.2) before an operation, they are there
after it, with $h$ and $n$ updated; hence the queue satisfies the axioms
(4.1), and each operation takes $O( 1 )$ time.*

*Proof.* After `enqueue(v)`, the item of rank $i < n$ is still at the
place $( h + i ) \bmod m$, and $v$, of rank $n$, is at $( h + n ) \bmod
m$, which is free since $n < m$. After `dequeue()`, the item of old rank
$i \ge 1$ has rank $i - 1$ and is at the place $( ( h + 1 ) + ( i - 1 ) )
\bmod m$. Hence the front is always the oldest item still in the
queue, as (4.1) requires. $\square$

The code computes the modulo of (4.2) without a division. Since $h < m$
and $n \le m$, the sum $h + n$ is below $2 m$, and its remainder is the sum
itself, or the sum minus $m$ when it reaches $m$. In the same way,
advancing $h$ is an increment followed by a comparison:

```cpp
--8<-- "generated/ArrayQueue-enqueue.cpp"
```

```cpp
--8<-- "generated/ArrayQueue-dequeue.cpp"
```

The item of `dequeue()` is moved out of its place, as in `ArrayStack`.
Besides `size()` and `capacity()`, the class has `contains(v)`, which
scans the items from the front in $O( n )$ time. It also gives a read-only
view of the items from the front to the back, whose iterator
(`CircularIterator` of `ADS/Iterators.h`) applies (4.2) at each access;
the iterator is therefore of random access, but not contiguous. The
contracts check the preconditions of (4.1), that `enqueue()` finds the
queue not full, and the invariant $0 \le n \le m$ with $h < m$.

<span id="rem-4-1"></span>**Remark 4.1.** The `ArrayQueue` of [Morin
2013, §2.3](Z-bibliography.md#ref-morin-cpp) has no capacity. When the
vector is full, `add(x)` moves the items to a vector of $2 n$ places, in
the order of their ranks from the place 0; `remove()` shrinks the vector
in the same way when $3 n$ does not exceed its size. By the argument of
[Lemma 3.1](03-stack.md#lem-3-1), this costs $O( 1 )$ amortized time per
operation. In the library, the variant that grows is `RingDeque`
([§4.5](04-queue.md#sec-4-5)), at both ends.

## 4.3 A queue on a linked list {#sec-4-3}

A singly linked list is a queue when the items enter after its last cell
and leave from its first one, since neither the removal of the first cell
nor the insertion after the last one needs the predecessor of a cell
([Montresor 2025, exercise 4.10](Z-bibliography.md#ref-montresor2025)).
`LinkedQueue` keeps a pointer $\mathit{front}$ to the first cell and a
pointer $\mathit{back}$ to the last one, both nil in the empty queue, and
the count $n$ of the items. An `enqueue(v)` links a new cell after the
back, or makes it the front when the queue is empty. A `dequeue()` unlinks
the front, and resets the back when it unlinks the last cell
([Algorithm 4.2](04-queue.md#alg-4-2) and
[Figure 4.2](04-queue.md#fig-4-2)). Both cost $O( 1 )$, and the queue has
no bound on its size, at the price of one cell (an item and a pointer) per
item.

![Figure 4.2. A `LinkedQueue` with the items `a`, `b`, `c`, after `enqueue(d)` and after `dequeue()`: the cell added and the links that `enqueue(d)` writes are in orange, the cells and the link that the operations read or write in violet; `dequeue()` returns `a` and gives its cell back to the pool of the queue.](../figures/queue-linked.svg){#fig-4-2}

<span id="alg-4-2"></span>**Algorithm 4.2.** The operations of `LinkedQueue`
([Montresor 2025, exercise 4.10](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/LinkedQueue-enqueue.pseudo.md"

--8<-- "generated/LinkedQueue-dequeue.pseudo.md"

In the code, the test of line 3 of `enqueue()` (whether the new cell is
linked from the front or from the last cell) becomes a single assignment
through a reference. Its cells come from an `ObjectPool` of the queue
([§5.5](05-memory.md#sec-5-5)):

```cpp
--8<-- "generated/LinkedQueue-enqueue.cpp"
```

```cpp
--8<-- "generated/LinkedQueue-dequeue.cpp"
```

The cell freed by a `dequeue()` is the one that the next `enqueue()` takes.
A queue in a steady state (as many insertions as removals) therefore calls
no allocator, and its cells usually stay in the cache (at least when the
queue is "small"). Besides, the class gives `size()` in $O( 1 )$ time, the
equality of two queues (the same items in the same order), and a forward,
read-only view of the items from the front to the back. A copy is made cell
by cell, in $O( n )$ time; a move takes the cells and the pool in $O( 1 )$
time, and leaves the queue moved from empty and usable. The contracts check
the preconditions of (4.1), and that `enqueue(v)` leaves $v$ at the front
when the queue was empty.

## 4.4 A deque on a bidirectional list {#sec-4-4}

A deque also removes its last item, and on a singly linked list this needs
the predecessor of the last cell, i.e., a scan of the list. `Dequeue`
therefore realizes the DEQUEUE of [Montresor 2025, exercise
4.9](Z-bibliography.md#ref-montresor2025) on the LIST of
`ADS/sequence/List.h` (see [chapter 2](#ch-2)), a bidirectional circular
list with a sentinel. Every cell has a pointer to the next cell and one to
the previous cell, and the sentinel, which holds no item, closes the
circle: the first cell follows it and the last one precedes it. An
insertion or a removal at either end then changes four pointers and tests
no special case ([Figure 4.3](04-queue.md#fig-4-3)). Its four operations
are those of the list at its head and before its sentinel
([Algorithm 4.3](04-queue.md#alg-4-3)), and each takes $O( 1 )$ time.

![Figure 4.3. A `Dequeue` with the items `a`, `b`, `c` on a circular list with a sentinel (the cell on the left, which holds no item), after `insertHead(x)` and after `removeTail()`; the upper link of each pair goes to the next cell, the lower one to the previous cell, and the two arcs close the circle through the sentinel; the cell added and its links are in orange, and the links that `removeTail()` rewrites in violet, with the cell removed, whose item it returns.](../figures/queue-dequeue.svg){#fig-4-3}

<span id="alg-4-3"></span>**Algorithm 4.3.** The operations of `Dequeue`
([Montresor 2025, exercise 4.9](Z-bibliography.md#ref-montresor2025));
$L$ is the list.

--8<-- "generated/Dequeue-insertHead.pseudo.md"

--8<-- "generated/Dequeue-insertTail.pseudo.md"

--8<-- "generated/Dequeue-removeHead.pseudo.md"

--8<-- "generated/Dequeue-removeTail.pseudo.md"

Note that the insertion at the tail is an insertion before the position
that follows the last item, i.e., before the sentinel. Also, the class
reads the two ends by `head()` and `tail()`, and gives the bidirectional
iterators of the list (forward and backward, with the items writable
through them). Its `push_back()` and `push_front()` are other names of the
two insertions, by which `std::back_inserter` and `std::front_inserter`
fill it. As the other lists of the library, it takes its cells from a pool,
and a deque in a steady state calls no allocator. This is the structure of
the algorithm of Pape and D'Esopo ([Pape
1974](Z-bibliography.md#ref-pape1974)), which puts a node at the tail the
first time it is reached and at the head afterwards
([§14.8.1](14-graph.md#sec-14-8-1)).

## 4.5 A deque on a circular vector that doubles {#sec-4-5}

`RingDeque` is the circular vector of `ArrayQueue` with three changes.
Its number of places $m$ is a power of 2, and the modulo of (4.2) is
therefore a mask; it doubles when it is full, and hence it has no
capacity; and it adds and removes items at both ends. It is thus the
`ArrayQueue` and the `ArrayDeque` of [Morin 2013, §2.3 and
§2.4](Z-bibliography.md#ref-morin-cpp), with the mask of [Morin 2013,
exercise 2.8](Z-bibliography.md#ref-morin-cpp) in place of the modulo, and
the deque of [Knuth 1997, §2.2.2](Z-bibliography.md#ref-knuth1997) on a
vector that grows.

**Representation.** The front and the back are two counters $h$ and $t$
modulo $2^{64}$ (unsigned words), whose difference $t - h$ is the number
$n$ of the items. The place of a counter $c$ is $c \mathbin{\&} ( m - 1 )$,
and the item of rank $i$ is at the place

$$
( h + i ) \mathbin{\&} ( m - 1 ) . \tag{4.3}
$$

This is (4.2) for $m = 2^j$, since $2^{64}$ is a multiple of $m$: the
counters can wrap around $2^{64}$ with no item changing its place. An
insertion at the back and a removal from the front change one counter each
($t$ and $h$), and so do an insertion at the front (which decrements $h$)
and a removal from the back (which decrements $t$). Thus the deque is empty
when $h = t$, and full when $t - h = m$. Its places are raw memory, in which
an item is built when it is inserted and destroyed when it is removed.
Unlike `ArrayQueue`, the type of the items thus needs no default
constructor, and a free place holds no item. With no places the mask is $m -
1 = 2^{64} - 1$, i.e., `capacity()` is 0.

**Growth.** When the vector is full, an insertion first moves the $n = m$
items to a new vector of $\max\{ 2 m , 8 \}$ places, in the order of their
ranks from the place 0, and frees the old one
([Algorithm 4.4](04-queue.md#alg-4-4)). The items of the old vector lie in
at most two contiguous pieces, $A[ h \mathbin{\&} ( m - 1 ) .. m - 1 ]$ and
the part that wrapped around to $A[ 0 .. ]$. Each piece is moved by one
`memcpy()` when the items can be copied bit by bit; otherwise each item is
built in the new vector by its move constructor (by its copy constructor
if moving could throw) and destroyed in the old one. After the move $h = 0$
and $t = n$, and an insertion at the back writes the place $n$. An
insertion at the front of a full deque puts the new item at the last place
of the new vector, which (4.3) makes the place before 0: it sets $h = m -
1$ and $t = n + m$, which keeps $t - h$ equal to the number of items
([Figure 4.4](04-queue.md#fig-4-4)).

![Figure 4.4. A full `RingDeque` of 8 places, the front `a` at the place 5, whose items lie in two pieces (all read by the growth, in violet), and the vector of 16 places after `push_back(i)` and, from the same state, after `push_front(z)`: the items move to the places 0 to 7 in the order of their ranks, and the item added (in orange) goes to the place 8 or to the last place, which (4.3) puts before the place 0.](../figures/queue-ring-grow.svg){#fig-4-4}

<span id="alg-4-4"></span>**Algorithm 4.4.** The insertions at the two
ends of `RingDeque` (the circular queue of [Knuth 1997,
§2.2.2](Z-bibliography.md#ref-knuth1997), with the doubling); $h$ and
$t$ are the counters of the front and of the back.

--8<-- "generated/RingDeque-emplace_back.pseudo.md"

--8<-- "generated/RingDeque-emplace_front.pseudo.md"

In the common case an insertion is a test, a construction in place and an
increment. The growth is a call out of line, behind a branch marked as
unlikely:

```cpp
--8<-- "generated/RingDeque-emplace_back.cpp"
```

```cpp
--8<-- "generated/RingDeque-relocate.cpp"
```

```cpp
--8<-- "generated/RingDeque-growFront.cpp"
```

The new item is built in a temporary before the growth, since the argument
of `push_back()` may be an item of the same deque (say,
`d.push_back( d.front() )`), which the growth moves and destroys; the
temporary is then moved into its place. Each of the removals `pop_front()` and
`pop_back()` destroys the item at its end and changes one counter, in
$O( 1 )$ time, and `clear()` destroys all the items and keeps the places.
In a queue the insertions write $t$ and the removals write $h$, and the
two chains of dependent instructions therefore do not wait for each other.

<span id="lem-4-2"></span>**Lemma 4.2.** *Starting from an empty
`RingDeque`, any sequence of operations that reaches at most $N$ items
moves fewer than $2 N$ items in all the growths, and the deque never has
more than $\max\{ 8 , 2 ( N - 1 ) \} < 2 N + 8$ places; hence each
insertion at an end costs $O( 1 )$ amortized time, and each removal
$O( 1 )$ time.*

*Proof.* A growth happens only when the deque is full, and it moves its
$m$ items to $2 m$ places (the first one, from no places to 8, moves
nothing). Since the vector never shrinks on its own, the growths happen
at the sizes $8 , 16 , \ldots , 2^k$, all below $N$, since the insertion
that causes the growth at the size $2^k$ brings the deque to $2^k + 1 \le
N$ items. They move $8 + 16 + \cdots + 2^k < 2^{k+1} \le 2 ( N - 1 )$
items, and leave $2^{k+1} \le 2 ( N - 1 )$ places. If no growth from 8
places happens, the deque has at most 8 places. $\square$

Unlike a vector that grows at one end only, the places freed at one end
serve the insertions at the other. A deque used as a queue in a steady state
thus never grows, and never moves its items once it has reached its largest
size. Neither does it shrink, and its space remains that of its largest size
until `shrink_to_fit()` moves the items to the least power of 2 places (and
8 at least) that holds them, or frees the vector of an empty deque.
Furthermore, the constructors take a list of items, and copy a deque item by
item; a move takes the vector in $O( 1 )$ time, and leaves the deque moved
from with no places. Finally, `reserve(n)` doubles the vector until it has
$n$ places at least.

**Operations by rank.** The item of rank $i$ is read and written by
`operator[]` in $O( 1 )$ time through (4.3), and the random-access iterators
of the class (`MaskIterator`) do the same; one can thus run, e.g.,
`std::sort` on a deque that wraps around its vector. The LIST of Morin also
inserts and removes at a rank, and his `ArrayDeque` does it by moving the
items on the shorter side of the rank. If $i < \lfloor n / 2 \rfloor$,
`insert(i, v)` puts a copy of the front item before it and moves the items
of the ranks $1 , \ldots , i - 1$ one place toward the front. Otherwise it
puts a copy of the back item after it, and moves the items of the ranks $i ,
\ldots , n - 2$ one place toward the back; `erase(i)` does the converse
([Algorithm 4.5](04-queue.md#alg-4-5)). [Figure 4.5](04-queue.md#fig-4-5)
replays the operations of the figure of [Morin 2013,
§2.4](Z-bibliography.md#ref-morin-cpp) on 16 places. We remark that its last
insertion, at the rank 4 of a deque of 9 items, moves the back side, since 4
is not below $\lfloor 9 / 2 \rfloor = 4$. His code does the same, with the
integer division of `i < n/2`, whereas the figure of his book moves the
front side.

![Figure 4.5. A `RingDeque` of 16 places under the operations of the figure of [Morin 2013, §2.4](Z-bibliography.md#ref-morin-cpp) (`remove(i)` and `add(i,x)` there are `erase(i)` and `insert(i,x)` here) and then `insert(0,w)`: the items that each operation moves are in violet, the one it inserts in orange; `erase(2)` and `insert(3,y)` move the front side, `insert(4,x)` and `insert(4,z)` the back side, and `insert(0,w)` takes the last place, before the place 0.](../figures/queue-ring-rank.svg){#fig-4-5}

<span id="alg-4-5"></span>**Algorithm 4.5.** The insertion and the
removal at a rank of `RingDeque` (the `add(i,x)` and `remove(i)` of the
`ArrayDeque` of [Morin 2013, §2.4](Z-bibliography.md#ref-morin-cpp)); $D[
0 .. n - 1 ]$ are the items by rank.

--8<-- "generated/RingDeque-insert.pseudo.md"

--8<-- "generated/RingDeque-erase.pseudo.md"

<span id="lem-4-3"></span>**Lemma 4.3.** *`insert(i, v)` moves at most
$\min\{ i , n - i \} + 2$ items, and takes
$O( 1 + \min\{ i , n - i \} )$ amortized time; `erase(i)` moves
$\min\{ i , n - 1 - i \}$ items, and takes $O( 1 + \min\{ i , n - i
\} )$ time.*

*Proof.* For $i < \lfloor n / 2 \rfloor$, `insert()` moves the front item
to a new place and $i - 1$ items, and then writes $v$; since then $i <
n - i$, these are $\min\{ i , n - i \} + 1$ moves. Otherwise it moves the
back item and $n - 1 - i$ items, and then writes $v$, i.e., $n - i + 1$
moves; here $n - i \le \min\{ i , n - i \} + 1$, because $i \ge \lfloor n
/ 2 \rfloor$ gives $n - i \le \lceil n / 2 \rceil \le i + 1$. In the same
way, `erase()` moves $i$ items when $i < \lfloor n / 2 \rfloor$, and $n -
1 - i$ otherwise, which is then at most $i$. Only an insertion at an end may
grow the vector, and its cost is amortized by
[Lemma 4.2](04-queue.md#lem-4-2). $\square$

The moves of the two sides are made on runs of places that are contiguous
in the vector, as [Morin 2013, exercise 2.7](Z-bibliography.md#ref-morin-cpp)
asks. Indeed, the ranks $\mathit{lo} , \ldots , \mathit{hi}$ of the items to
move occupy at most two pieces of the vector, and the places one rank away at
most two more. A run stops at the first end of the vector that either the
source or the destination meets, and there are therefore three runs at
most, each moved by one `std::move` (one `memmove()` for the items that
can be copied bit by bit):

```cpp
--8<-- "generated/RingDeque-shiftDown.cpp"
```

The function `shiftUp()` is the mirror, by `std::move_backward`, and
`insert()` and `erase()` are short on top of the two:

```cpp
--8<-- "generated/RingDeque-insert.cpp"
```

```cpp
--8<-- "generated/RingDeque-erase.cpp"
```

The argument $v$ of `insert()` is copied before anything moves, since it
may be an item of the deque; the insertions at the ranks 0 and $n$ are
those at the ends.

**Rotation.** `rotate(r)` moves the item of rank $i$ to the rank $( i + r )
\bmod n$, for every $i$ ([Morin 2013, exercise
2.6](Z-bibliography.md#ref-morin-cpp)). When the vector is full, the items
already sit in a circle, and the rotation only moves the two counters back
by $r$, in $O( 1 )$ time. Otherwise the deque moves $\min\{ r , n - r \}$
items from one end to the other, one at a time
([Algorithm 4.6](04-queue.md#alg-4-6)); this never grows the vector, since
a place is free after each removal.

<span id="alg-4-6"></span>**Algorithm 4.6.** The rotation of
`RingDeque` ([Morin 2013, exercise 2.6](Z-bibliography.md#ref-morin-cpp)),
$r$ reduced modulo $n$.

--8<-- "generated/RingDeque-rotate.pseudo.md"

```cpp
--8<-- "generated/RingDeque-rotate.cpp"
```

On a `RingDeque`, the function `rotate(l, r)` of
`ADS/sequence/IndexedList.h` calls this member; on the lists that have no
`rotate()` it follows the $\gcd( n , r )$ cycles of the permutation by
`operator[]` (see [chapter 2](#ch-2)). Its contracts check that `front()`,
`back()` and the removals find the deque not empty, that `operator[]` reads
a rank below $n$, and the invariant that the number of places is 0 or a
power of 2 not smaller than $n$.

## 4.6 A queue with no duplicates {#sec-4-6}

`UniqueQueue` is a queue that never holds two equal items, with one of two
policies (the enumeration `Duplicate`) for an item that is already in the
queue. With `IGNORE_NEW`, `enqueue(v)` does nothing, and each item keeps the
place of its first arrival, as the visits of a graph want it. With
`FORGET_OLD`, `enqueue(v)` removes the old copy and enqueues $v$ at the
back; each item then keeps the place of its last arrival, as a cache of the
least recently used items wants it ([Figure 4.6](04-queue.md#fig-4-6)).
Either way, `enqueue()` must first find the copy ([Algorithm
4.7](04-queue.md#alg-4-7)). In a `List` (see [chapter 2](#ch-2)), where the
items are kept, the copy is removed in $O( 1 )$ time but found by a scan.

![Figure 4.6. A `UniqueQueue` with the items `1`, `2`, `3` in its list (the front on the left, the sentinel of the list not drawn) and `enqueue(2)` under the two policies: with `IGNORE_NEW` the copy is found (in violet) and the queue is left as it is, with `FORGET_OLD` the copy is removed and `2` is enqueued at the back (in orange).](../figures/queue-unique.svg){#fig-4-6}

<span id="alg-4-7"></span>**Algorithm 4.7.** The insertion of
`UniqueQueue`; $L$ is the list of the items.

--8<-- "generated/UniqueQueue-enqueue.pseudo.md"

Hence `enqueue()` takes $O( n )$ comparisons in the worst case (about $n /
2$ on average when a copy is in the queue, at a place drawn uniformly), and
so does `contains(v)`, while `dequeue()`, `top()` and `size()` take $O( 1 )$
time. Note that the invariant that no item occurs twice (an $O( n^2 )$ check
of the contracts) requires the equality of the items to be an equivalence.
An item unequal to itself, such as a NaN, would be enqueued again at each
`enqueue()`; the postcondition, which checks that $v$ is in the queue,
refuses it. With `FORGET_OLD`, the item $v$ is copied before the old copy is
removed, since $v$ may refer to that copy (as in `q.enqueue( q.top() )`).
Also, the view of the items from the front to the back is read-only, since a
change through it could create a duplicate. A dictionary from the items to
their cells would bring `enqueue()` to $O( 1 )$ expected time (see [chapter
7](07-dictionary.md#ch-7)), at the price of a hash of the items and of a
second structure. We kept the specification of [Montresor 2025,
§4.3](Z-bibliography.md#ref-montresor2025), in which the items need only an
equality.

## 4.7 A random queue {#sec-4-7}

The `RandomQueue` of [Morin 2013, exercise
2.3](Z-bibliography.md#ref-morin-cpp) is a bag: `push(x)` puts an item in,
and `pop()` takes out one chosen uniformly at random among those inside,
both in $O( 1 )$ amortized time. Its items are the first $n$ places of a
vector that doubles, in no order. To remove one, `pop()` draws a place $j$
uniformly in $\{ 0 , \ldots , n - 1 \}$, exchanges its item with the last
one and removes the last place ([Algorithm 4.8](04-queue.md#alg-4-8) and
[Figure 4.7](04-queue.md#fig-4-7)); no item moves but these two.

![Figure 4.7. `pop()` on a `RandomQueue` of six items, with the place $j = 2$ drawn: the items of the place drawn and of the last place (in violet) are exchanged, and the last place is removed, returning `c`.](../figures/queue-random.svg){#fig-4-7}

<span id="alg-4-8"></span>**Algorithm 4.8.** The removal of
`RandomQueue` ([Morin 2013, exercise 2.3](Z-bibliography.md#ref-morin-cpp));
$A[ 0 .. n - 1 ]$ are the items.

--8<-- "generated/RandomQueue-pop.pseudo.md"

```cpp
--8<-- "generated/RandomQueue-pop.cpp"
```

<span id="lem-4-4"></span>**Lemma 4.4.** *Whatever the sequence of
operations before it, a `pop()` on a queue of $n$ items returns each of
them with probability $1 / n$, provided that the draw of the place is
uniform and independent of the previous ones.*

*Proof.* The place $j$ is drawn uniformly among the $n$ places, each of
which holds one of the items, and the draw does not depend on how the
items came to be in those places. $\square$

The order of the vector thus never matters, and the exchange with the last
place, which changes it, is harmless; `sample()` reads an item drawn in the
same way, without removing it. Emptying a queue after $n$ pushes gives the
items in a uniform random order. This is the shuffle of [Fisher and Yates
1938](Z-bibliography.md#ref-fisher1938) in the form of [Durstenfeld
1964](Z-bibliography.md#ref-durstenfeld1964), where the place drawn is
exchanged with the last one, which then leaves.

The place is drawn by `uniform(n)`, after [Lemire
2019](Z-bibliography.md#ref-lemire2019). Let $w$ be a word of 64 bits of
the generator SplitMix64 ([Steele, Lea and Flood
2014](Z-bibliography.md#ref-steele2014); see
[§17.9](17-numeric.md#sec-17-9)), which the constructor seeds for a
reproducible run, and let $w \cdot n = 2^{64} h + l$ be its product with
$n$ on 128 bits. The draw returns the high half $h$, unless $l$ is below
the threshold $\theta = ( 2^{64} - n ) \bmod n = 2^{64} \bmod n$, in which
case it draws another word ([Algorithm 4.9](04-queue.md#alg-4-9)).

<span id="alg-4-9"></span>**Algorithm 4.9.** The uniform draw of a place
in $[ 0 , n )$ ([Lemire 2019](Z-bibliography.md#ref-lemire2019)).

--8<-- "generated/RandomQueue-uniform.pseudo.md"

```cpp
--8<-- "generated/RandomQueue-uniform.cpp"
```

<span id="lem-4-5"></span>**Lemma 4.5.** *If the words $w$ are uniform
in $[ 0 , 2^{64} )$, `uniform(n)` returns each $h \in [ 0 , n )$ with
probability $1 / n$; each word is rejected with probability $\theta /
2^{64} < n / 2^{64}$, and the division that computes $\theta$ is made
only when $l < n$, i.e., with probability at most $n / 2^{64}$.*

*Proof.* Let $2^{64} = q n + \theta$ with $0 \le \theta < n$. The words
$w$ whose product has the high half $h$ are those with $w n \in [ 2^{64} h
, 2^{64} ( h + 1 ) )$. Their low halves $l = w n - 2^{64} h$ form an
arithmetic progression of difference $n$ in $[ 0 , 2^{64} )$, whose first
term $l_0$ is below $n$. If $l_0 \ge \theta$, the progression has
$\lfloor ( 2^{64} - 1 - l_0 ) / n \rfloor + 1 = q$ terms, all accepted,
since $2^{64} - 1 - l_0 = q n - ( l_0 + 1 - \theta )$ with $1 \le l_0 +
1 - \theta \le n$. If $l_0 < \theta$, it has $q + 1$ terms, of which only the
first is rejected. Each $h$ is thus the image of $q$ accepted words, out of
the $2^{64} - \theta = q n$ words accepted in all. The words rejected have
$l < \theta$, and there are $n \theta$ of them, a fraction $\theta /
2^{64}$; finally, $l < \theta$ implies the test $l < n$ that comes before
the division. $\square$

For a queue of fewer than $2^{32}$ items, a word is thus rejected with
probability below $2^{-32}$. In practice the draw costs one multiplication
and one comparison on all but a negligible fraction of the calls, whereas a
draw by $w \bmod n$ would cost a division at each call (and would be
biased). Also, the class has `size()`, `empty()`, `clear()` and `items()`,
the vector of the items in no order. The contracts check that `pop()` and
`sample()` find the queue not empty, and that `uniform(n)` receives $n > 0$.

## 4.8 Queues and deques with the minimum {#sec-4-8}

The `MinQueue` and the `MinDeque` of [Morin 2013, exercises 3.15 and
3.16](Z-bibliography.md#ref-morin-cpp) are a queue and a deque that also
give their least item by `min()`, in $O( 1 )$ amortized time per
operation, under an order `C` (a strict weak order, `std::less<>` by
default). Both are built on the `MinStack` of `ADS/stack/MinStack.h`
([§3.5](03-stack.md#sec-3-5)), the stack of [Morin 2013, exercise
3.14](Z-bibliography.md#ref-morin-cpp) that keeps, beside the vector of
its items, a second vector of the record lows, whose top is the least item
of the stack ([Algorithm 3.3](03-stack.md#alg-3-3) and
[Lemma 3.2](03-stack.md#lem-3-2)); each of its operations costs $O( 1 )$
time, but for the doublings of the two vectors.

**A queue on two stacks.** `MinQueue` enqueues on a stack $I$ (the back, the
last item on top) and dequeues from a stack $O$ (the front, the first item
on top). When $O$ is empty, `dequeue()` first moves all the items of $I$ to
$O$, popping them from $I$ and pushing them on $O$. This reverses their
order, and puts the oldest item on top of $O$ ([Algorithm
4.10](04-queue.md#alg-4-10) and [Figure 4.8](04-queue.md#fig-4-8)). As for
the least item, it is the lesser of the minima of the two stacks, found in
$O( 1 )$ time (cf. `min()` below).

![Figure 4.8. A `MinQueue` into which `4`, `6`, `2`, `7`, `5` were enqueued, its two stacks drawn from the bottom (left) to the top, each with its minima; `dequeue()` finds $O$ empty, moves the items of $I$ to $O$ (in orange) in the reverse order, and pops the front `4` (in violet); the least item is `2` before and after.](../figures/queue-minqueue.svg){#fig-4-8}

<span id="alg-4-10"></span>**Algorithm 4.10.** The removal of
`MinQueue` ([Morin 2013, exercise 3.15](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/MinQueue-dequeue.pseudo.md"

```cpp
--8<-- "generated/MinQueue-transfer.cpp"
```

```cpp
--8<-- "generated/MinQueue-min.cpp"
```

<span id="lem-4-6"></span>**Lemma 4.6.** *The items of a `MinQueue`
leave it in the order in which they entered; any sequence of $s$
operations from the empty queue takes $O( s )$ time, and `min()` returns
the least item in $O( 1 )$ time.*

*Proof.* The items of $O$ are all older than those of $I$, since $I$ is
moved to $O$ only when $O$ is empty. Since the move reverses $I$, the items
of $O$ are in the order of their age from the top. Hence the oldest
item is the top of $O$, or the bottom of $I$ when $O$ is empty (the item
that `top()` reads). Each item is pushed on $I$ once, popped from $I$ and
pushed on $O$ at most once, and popped from $O$ at most once: these are
four operations of the stacks per item, and the $s$ operations enqueue at
most $s$ items. Every item is in one of the two stacks, whose minima are
their least items. $\square$

The bound is on the sequence, not on the operation. A `dequeue()` that finds
$O$ empty moves all of $I$, i.e., it takes $\Theta( n )$ time, and a queue
that alternates long runs of insertions with long runs of removals may pay
it once per run; [§4.9](04-queue.md#sec-4-9) removes this worst case.

**A deque on two stacks.** `MinDeque` keeps a stack $F$ with the front on
top and a stack $B$ with the back on top, back to back as in the
`DualArrayDeque` of [Morin 2013, §2.5](Z-bibliography.md#ref-morin-cpp) (see
[chapter 2](#ch-2)). In the order of the deque, the items are those of $F$
from the top down, and then those of $B$ from the bottom up. An insertion or
a removal at an end is that of its stack, but for a removal from an empty
side, which first *deals* the $k$ items of the other side again. The $\lceil
k / 2 \rceil$ items nearer to the empty end (i.e., the bottom items of the
other stack) go to the empty side, in the reverse order, and the other
$\lfloor k / 2 \rfloor$ stay ([Algorithm 4.11](04-queue.md#alg-4-11) and
[Figure 4.9](04-queue.md#fig-4-9)). A deal takes $O( k )$ time (one may
argue that this is rather wasteful, but it happens rarely, cf. [Lemma
4.7](04-queue.md#lem-4-7)), through a vector that holds the items popped.

![Figure 4.9. A `MinDeque` with the items `3`, `8`, `1`, `9`, `4`, `6`, all on $B$, the two stacks drawn back to back in the order of the deque; `pop_front()` finds $F$ empty and deals the three items nearer to the front to it (in orange), then pops `3` (in violet); the least item is `1`.](../figures/queue-mindeque.svg){#fig-4-9}

<span id="alg-4-11"></span>**Algorithm 4.11.** The removal from the
front of `MinDeque` and the deal ([Morin 2013, exercise
3.16](Z-bibliography.md#ref-morin-cpp)); $s_0 , \ldots , s_{k-1}$ are the
items of the stack $s$ from its bottom.

--8<-- "generated/MinDeque-popFront.pseudo.md"

--8<-- "generated/MinDeque-deal.pseudo.md"

```cpp
--8<-- "generated/MinDeque-deal.cpp"
```

<span id="lem-4-7"></span>**Lemma 4.7.** *Any sequence of $s$ operations
on a `MinDeque` from the empty deque takes $O( s )$ time, and `min()`
returns the least item in $O( 1 )$ time.*

*Proof.* Let a deal of $k$ items cost at most $c k$, for a constant $c$
(it pops $k$ items and pushes $k$), and let $\Phi = c \, | \, |F| - |B| \,
|$, which is 0 for the empty deque and never negative. An operation that
does not deal costs $O( 1 )$ and changes $\Phi$ by $c$ at most. A deal is
made when one side is empty and the other has $k$ items, i.e., when $\Phi =
c k$. It leaves the two sides with $\lceil k / 2 \rceil$ and $\lfloor k /
2 \rfloor$ items, i.e., $\Phi \le c$, and the drop of $\Phi$, at least $c (
k - 1 )$, pays for the deal but for $c$; the removal that follows changes
$\Phi$ by $c$ at most. Hence the amortized cost of each operation (its cost
plus the change of $\Phi$) is $O( 1 )$. The minimum is the lesser of those
of $F$ and $B$, as in `MinQueue`. $\square$

The deal splits the items in two halves, instead of moving them all to the
empty side, and the bound holds at both ends for this reason. A deal that
moved all the $k$ items would leave the other side empty; a sequence that
removed alternately at the two ends would then move all the items at each
operation. Here too a single operation may take $\Theta( n )$ time. Both
classes also have `size()`, `empty()` and `clear()`, and `MinDeque` reads
its ends by `front()` and `back()` (the bottom of the other stack when a
side is empty). In both, the contracts check that the removals, the reads
and `min()` find the structure not empty.

## 4.9 The minimum in constant worst-case time {#sec-4-9}

[Lemma 4.6](04-queue.md#lem-4-6) and [Lemma 4.7](04-queue.md#lem-4-7) bound
sequences, not operations: one `dequeue()` of `MinQueue` may move the whole
back, and one removal of `MinDeque` may deal a whole side.
`RealTimeMinQueue` and `RealTimeMinDeque` spread that work over the
operations that come before the moment it is needed. Every operation,
`min()` included, then takes $O( 1 )$ time in the worst case, at the price
of larger constants and of a more delicate state. For the queue the idea is
the *real-time* queue of [Hood and Melville
1981](Z-bibliography.md#ref-hood1981); for the deque it is the *global
rebuilding* of [Overmars 1983](Z-bibliography.md#ref-overmars1983), as
[Gajewska and Tarjan 1986](Z-bibliography.md#ref-gajewska1986) apply it to
the deques with heap order.

**Arrays with the minimum.** Both classes keep their stacks in arrays of
fixed capacity (`detail::MinArray`), allocated with their final size when
the work that fills them starts. Its live cells are the range
$[ \mathit{lo} , \mathit{hi} )$, and a cell holds an item and the place of
the least item among those of the places from 0 up to it. Hence the minimum of
the array is read at the cell of its top, and a push builds a cell on
top with its least place:

```cpp
--8<-- "generated/RealTimeMinQueue-MinArray-push.cpp"
```

A cell is removed from the top by `pop()`, and from the bottom by
`popBottom()`, which destroys it and advances $\mathit{lo}$. However, the least
places of the cells above it are not updated, and they may therefore point
to a cell no longer live (a case that `RealTimeMinQueue` handles below). No
operation of the array copies, moves or destroys more than one item, and
the allocation of an array of raw cells is counted as $O( 1 )$.

**The rotation of Hood and Melville.** `RealTimeMinQueue` keeps, as
`MinQueue` does, a front $F$ (the first item on top) and a back $R$ (the
last item on top), with $|R| \le |F|$ between the operations that come
outside a rotation. When an operation makes $|R| = |F| + 1$, a *rotation*
starts: $R$ is frozen as $R'$, and a new empty back takes the insertions. A
new front $N$ is then built from its bottom, by three moves per operation,
first with the items of $R'$ from its top and then with those of $F$ from
its bottom, while the removals still take the top of $F$. When the moves of
$F$ meet its top the rotation ends, and $N$ becomes the front ([Algorithm
4.12](04-queue.md#alg-4-12) and [Figure 4.10](04-queue.md#fig-4-10)). Here
the items are moved, not copied, since an item is in only one of the arrays
at a time.

![Figure 4.10. A rotation of `RealTimeMinQueue` (the queue `a`, ..., `f` before, `a` the front), each array drawn from its bottom (left) to its top: `enqueue(g)` makes $|R| = |F| + 1$, freezes $R$ as $R'$ and moves three items of it to the new front $N$ (in orange); the next `dequeue()` returns `a`, moves `d`, `c`, `b` and finds the moves of $F$ at its top, so that $N$ becomes the front.](../figures/queue-rtqueue.svg){#fig-4-10}

<span id="alg-4-12"></span>**Algorithm 4.12.** The removal of
`RealTimeMinQueue` and the moves of a rotation ([Hood and Melville
1981](Z-bibliography.md#ref-hood1981)).

--8<-- "generated/RealTimeMinQueue-dequeue.pseudo.md"

```cpp
--8<-- "generated/RealTimeMinQueue-dequeue.cpp"
```

```cpp
--8<-- "generated/RealTimeMinQueue-steps.cpp"
```

Ending the rotation costs no move, and it is checked even when the
three moves are spent; hence $F$ is never left empty with the rotation
still going on. A rotation allocates its arrays when it starts, $N$
with $|F| + |R'|$ cells and the new back with $|F| + |R'| + 2$. Its old
front, whose cells have all been moved or removed by then, is freed when
the rotation ends, together with $R'$, which is empty.

<span id="lem-4-8"></span>**Lemma 4.8.** *Let a rotation start with $f =
|F|$ and $|R'| = f + 1$. Then $F$ does not empty before the rotation
ends, which happens within $\lceil ( 2 f + 1 ) / 3 \rceil$ operations
(the one that starts it included); at the end $|R| \le |F|$, and the arrays
allocated at the start are large enough until the next rotation. Hence
every operation of `RealTimeMinQueue` takes $O( 1 )$ time in the worst
case.*

*Proof.* The moves are at most the $f + 1$ of $R'$ and the $f$ of $F$ (fewer
if some items of $F$ are removed first), three per operation, and hence at
most $\lceil ( 2 f + 1 ) / 3 \rceil$ operations make moves. The $f + 1$
moves of $R'$ are done when the operation that starts the rotation and the
$k$ after it have made $3 ( k + 1 ) \ge f + 1$ moves, i.e., at $k = \lceil (
f + 1 ) / 3 \rceil - 1$. Until then at most $k$ items have been removed from
$F$, and $f - k > 0$ for $f \ge 1$ (for $f = 0$, $R'$ holds one item, and
the rotation ends in the operation that starts it). After that, the moves of
$F$ go up from its bottom and the removals down from its top, and the
rotation ends when they meet, before a removal could find $F$ empty. At the
end, $N$ holds the items left of $F$ above those of $R'$, in the order of
the queue; after $d$ removals these are $2 f + 1 - d$ items, which fit in
its $2 f + 1$ cells. Meanwhile, the new back has received the insertions
made after the start, at most $\lceil ( 2 f + 1 ) / 3 \rceil - 1 - d < 2 f +
1 - d$ of them, and hence $|R| \le |F|$. Then the next rotation starts as
soon as $|R| = |F| + 1 \le 2 f + 2$, within the $2 f + 3$ cells of the back.
Every operation thus makes one push or one pop, three moves, and at most one
start and one end, each of which allocates or frees arrays with no live
cells. $\square$

The least item is the least of the minima of the four parts, $F$, $R'$, $N$
and the back, each read at the cell of its top. The minimum of $F$ needs
some care, since the least place of its top cell may be a cell already
moved to $N$. In fact, the items of $F$ go to $N$ from its bottom, in
order, after the $|R'|$ items of the frozen back; the item of the place $a$
of $F$ is therefore at the place $|R'| + a$ of $N$. A minimum of $F$ found
in $N$ is counted twice (in $F$ and in $N$), which does not change the
least of the four.

```cpp
--8<-- "generated/RealTimeMinQueue-min.cpp"
```

Furthermore, the class visits the items from the front to the back by
`forEach(f)`, in $O( n )$ time; the order is the top of $F$ down to its
first place not moved, the items of $F$ in $N$, the frozen back from its
bottom, its items in $N$, and the back. It is used by the copy constructor;
a move exchanges the state in $O( 1 )$ time, and `clear()` replaces the
state by that of an empty queue. The contracts check the preconditions of
(4.1), and the invariant that $F$ is not empty when the queue is not.

**The global rebuilding of a deque.** `RealTimeMinDeque` keeps the stacks
$F$ and $B$ of `MinDeque`, and avoids the deals by keeping the two sides
within a factor 3 of each other. A *rebuild* starts when one side $G$ has
more than $3 |S| + 1$ items, $S$ being the other side, or when a side fills
half its array. Let $n_0$ be the number of items at the start, and $t =
|G| - \lceil n_0 / 2 \rceil$: the $\lceil n_0 / 2 \rceil$ items nearest the
top of $G$ stay on its side, and the $t$ at its bottom pass to the other.
Two new arrays of $2 n_0 + 16$ cells are filled from their bottom with
copies, twelve per operation. They are $S'$, with $G[ t - 1 ] , \ldots ,
G[ 0 ]$ and then the items of $S$ from its bottom, and $G'$, with $G[ t ] ,
G[ t + 1 ] , \ldots$ up to the top ([Algorithm 4.13](04-queue.md#alg-4-13)
and [Figure 4.11](04-queue.md#fig-4-11)). Meanwhile, the operations go on
on the old stacks, as on `MinDeque` but with no deal. Each copy keeps a
prefix of its old stack, and a removal that goes below the end of a prefix
also removes the top of the copy. When both prefixes reach their tops, the
new arrays replace the old ones, which the operations that follow destroy
twelve cells at a time.

![Figure 4.11. A rebuild of `RealTimeMinDeque` on the deque `f0`, `b0`, ..., `b4`, the two stacks drawn back to back in the order of the deque: $B$ has more than $3 |F| + 1$ items, and its $t = 2$ bottom items (in violet) pass to the front; the copies (in orange) fill $S'$ with `b1`, `b0` and then `f0`, and $G'$ with `b2`, `b3`, `b4`, after which the new arrays replace the old. A deque this small is rebuilt by the code within one operation; the figure shows the order of the copies.](../figures/queue-rtdeque.svg){#fig-4-11}

<span id="alg-4-13"></span>**Algorithm 4.13.** The rebuild of
`RealTimeMinDeque` (the global rebuilding of [Overmars
1983](Z-bibliography.md#ref-overmars1983), after [Gajewska and Tarjan
1986](Z-bibliography.md#ref-gajewska1986)).

--8<-- "generated/RealTimeMinDeque-rebuild.pseudo.md"

The code of the test that starts a rebuild, of the start, of the copies and
of the removal that keeps the prefixes is the following:

```cpp
--8<-- "generated/RealTimeMinDeque-due.cpp"
```

```cpp
--8<-- "generated/RealTimeMinDeque-start.cpp"
```

```cpp
--8<-- "generated/RealTimeMinDeque-steps.cpp"
```

```cpp
--8<-- "generated/RealTimeMinDeque-pop.cpp"
```

<span id="lem-4-9"></span>**Lemma 4.9.** *Let a rebuild start with $n_0
\ge 32$ items. Then it ends within $n_0 / 11 + 2$ operations, during which
neither side of the old stacks empties and $G$ keeps its $t$ bottom items;
every operation of `RealTimeMinDeque` takes $O( 1 )$ time in the worst
case, and the next rebuild starts $\Omega( n_0 )$ operations later.*

*Proof.* The copies are at most $n_0$ plus one per insertion during the
rebuild (a removal only shortens a prefix), twelve per operation. The
operation that starts the rebuild and the $k$ after it finish them once
$12 ( k + 1 ) \ge n_0 + k$, i.e., for some $k \le n_0 / 11 + 1$. When a
rebuild starts by the sizes, the operation before it left $|G| \le 3 |S| +
1$ and changed one side by one item; hence $3 |S| + 2 \le |G| \le 3 |S| +
4$, i.e., $|S| \ge ( n_0 - 4 ) / 4 \ge 7$. When it starts by the room, the
sizes were within the factor 3, i.e., $|S| \ge ( n_0 - 1 ) / 4$. Either
way $|S|$ exceeds the at most $n_0 / 11 + 2$ removals from $S$ during the
rebuild, since $( n_0 - 4 ) / 4 > n_0 / 11 + 2$ for $n_0 > 132 / 7$.
Also, $G$ has $\lceil n_0 / 2 \rceil \ge 16$ items above its bottom $t$,
more than the removals from it, and the half of each array that is free
holds the insertions. Since the old stacks are copied, not moved, they
keep all their cells below their tops; their minima stay right, and the
operations act on them as on `MinDeque`. At the end each side has $\lceil
n_0 / 2 \rceil$ or $\lfloor n_0 / 2 \rfloor$ items, changed by at most $n_0
/ 11 + 2$, with room for $n_0 + 8$ before half its array. A new rebuild
needs $|G| - 3 |S|$ to grow from about $- n_0$ to 2, by 3 at most per
operation, or a side to grow by about $n_0 / 2$; either takes $\Omega( n_0
)$ operations, more than those that destroy the at most $n_0 + n_0 / 11 +
2$ live cells of the old arrays, twelve per operation. A deque of fewer
than 32 items is rebuilt within the operation that starts it, in $O( 1 )$
time, and its old arrays are freed at once. $\square$

The items are copied, whereas the queue moves them, since the old stacks
stay in use during the rebuild; the type of the items must therefore be
copyable. A removal from an empty side happens only when the deque has one
item, which the other side holds (with sides within the factor 3, or with
a rebuild going on, neither side is empty when $n \ge 2$); `front()` and
`back()` read it there. Besides `forEach(f)`, the class has `size()`,
`empty()` and `clear()`; it copies item by item and moves in $O( 1 )$
time.
The contracts check the preconditions of the removals, and the invariant
that a rebuild never reads a cell that a removal has destroyed.

## 4.10 Algorithms on a queue {#sec-4-10}

The functions of `ADS/queue/QueueAlgorithms.h` act on a queue through
`enqueue()`, `dequeue()`, `top()` and `empty()` only, and are therefore
written once for all the models of the concept `Queue`. Moving the positive
items of a queue $q$, in their order, to the back of another queue `out`
dequeues each item of $q$ once (`extractPositive()`), in $O( n )$ time, and
leaves $q$ empty. Copying them instead, with $q$ left as it was
(`copyPositive()`), enqueues each item in an auxiliary queue as it is
dequeued, and moves the auxiliary queue back to $q$ at the end. By the FIFO
order of both queues this restores the order of $q$, in $O( n )$ time and
space ([Algorithm 4.14](04-queue.md#alg-4-14)). Note that the auxiliary
queue is a `LinkedQueue`, whatever the type of $q$, since an `ArrayQueue`
would need a capacity that the interface does not give.

<span id="alg-4-14"></span>**Algorithm 4.14.** The positive items of a
queue, moved and copied.

--8<-- "generated/QueueAlgorithms-extractPositive.pseudo.md"

--8<-- "generated/QueueAlgorithms-copyPositive.pseudo.md"

One could restore $q$ with no auxiliary queue by rotating it, i.e., by
enqueueing each item again in $q$ as soon as it is dequeued, but only
knowing how many items $q$ holds, which the QUEUE does not say. (A mark
enqueued at the end would do, if the type of the items had a value unequal
to all the others.) Both functions check, as preconditions, that the two
queues are distinct objects; `copyPositive()` also checks, as an audit,
that $q$ holds its old items in their old order.

A queue also reverses a stack, since it keeps the order in which the items
are popped: this is `reverseWithQueue()` of `ADS/stack/StackAlgorithms.h`
([Morin 2013, exercise 1.4](Z-bibliography.md#ref-morin-cpp)), described
with the algorithms on a stack ([Algorithm 3.7](03-stack.md#alg-3-7)).

Three of the problems on the lines of a text of [Morin 2013, exercise
1.1](Z-bibliography.md#ref-morin-cpp) call for a queue. The first writes,
for each blank line after the 42nd, the line 42 lines before it, keeping 43
lines at most; a queue whose length is kept at 43 by a `dequeue()` after
each `enqueue()` beyond it solves it (on a `RingDeque`, the vector stops at
64 places). Writing the lines of even rank before those of odd rank takes
two queues, filled in turn. Finally, writing the lines in a random order
takes a `RandomQueue`, emptied after all the lines are in (cf.
[Lemma 4.4](04-queue.md#lem-4-4)).

## 4.11 Engineered variants {#sec-4-11}

The realizations of the book are kept as they are written, but for three
changes that do not alter their interface. First, `ArrayQueue` computes
the modulo of (4.2) by a comparison and a subtraction, which is correct
because the sum of two places is below $2 m$ ([§4.2](04-queue.md#sec-4-2)).
Second, `LinkedQueue` and `Dequeue` take their cells from a pool
([§5.5](05-memory.md#sec-5-5)); a queue in a steady state reuses the cell
just freed, which is likely still in the cache, and calls no allocator.
Third, the item removed is moved out of its place, not copied, which
matters when the items own memory.

`RingDeque` differs from the `ArrayDeque` of Morin in four points: (i) the
mask of a power of 2 replaces the division of `%`, which on most
processors costs tens of cycles, against one for the mask; (ii) the places
are raw memory, and a growth moves the items by `memcpy()` on at most two
pieces, where the `ArrayDeque` of Morin builds its new array by default and
assigns the items one by one; (iii) the moves by rank are made by runs
([Lemma 4.3](04-queue.md#lem-4-3) and what follows it); and (iv) the deque
does not shrink, so that a queue whose size oscillates never reallocates,
where the `ArrayDeque` of Morin resizes whenever $3 n \le m$. Furthermore,
the two counters modulo $2^{64}$ make the insertions at the back and the
removals from the front two chains of instructions that do not depend on
each other. Also, the operations at the ends are inlined, and the growth is
kept out of line.

Compared with `std::deque`, which keeps the items in blocks of 512 bytes
reached by a map of pointers, `RingDeque` has no indirection and no
allocation but at the doublings, and its items lie in at most two contiguous
pieces. Unlike `boost::circular_buffer`, its vector grows when full; unlike
`boost::container::devector`, the places freed at one end serve the
insertions at the other, and a queue used as a FIFO never moves its items.
Its price is the space of the largest size, at most $2 N + 8$ places for $N$
items ([Lemma 4.2](04-queue.md#lem-4-2)), until `shrink_to_fit()`. We leave
the running times against these libraries to the campaign of Appendix B, and
no number is given here.

`RandomQueue` draws its places with no division in the common case
([Lemma 4.5](04-queue.md#lem-4-5)), whereas the obvious $w \bmod n$ costs
a division at each draw and is biased. We expect the real-time structures of
[§4.9](04-queue.md#sec-4-9) to be somewhat slower than `MinQueue` and
`MinDeque` over a whole sequence, since their constants are larger (three
moves per operation for the queue, twelve copies for the deque, and more
arrays). They are the choice when the time of each operation counts, e.g.,
in a loop with a deadline, or when the items are expensive to move in
bulk.

## 4.12 Discussion and references {#sec-4-12}

The queue on a circular vector, the queue on a linked list and the deque are
in most textbooks on data structures (see, e.g., [Knuth 1997, §2.2.1 and
§2.2.2](Z-bibliography.md#ref-knuth1997), which also has the deques
restricted on input or on output, and [Montresor 2025,
§4.3](Z-bibliography.md#ref-montresor2025)). [Morin 2013, §2.3 and
§2.4](Z-bibliography.md#ref-morin-cpp) adds the vector that grows and
shrinks and the operations by rank on the shorter side, and calls these
structures folklore. The deque of the algorithm of Pape and D'Esopo is in
[Pape 1974](Z-bibliography.md#ref-pape1974), and the queue on two stacks is
folklore as well. Its worst case was removed by [Hood and Melville
1981](Z-bibliography.md#ref-hood1981), whose queue is real-time and purely
functional (no cell is ever changed); [Okasaki
1995](Z-bibliography.md#ref-okasaki1995) obtained simpler real-time queues
and deques by lazy evaluation, and [Kaplan and Tarjan
1999](Z-bibliography.md#ref-kaplan1999) added the concatenation of two
deques in $O( 1 )$ worst-case time. The deque with the minimum in $O( 1 )$
worst-case time per operation is that of [Gajewska and Tarjan
1986](Z-bibliography.md#ref-gajewska1986). It applies to the two stacks of a
deque the global rebuilding of [Overmars
1983](Z-bibliography.md#ref-overmars1983), i.e., the construction of a new
copy of a structure in the background while the old one stays in use. The
thresholds of `RealTimeMinDeque` (the factor 3, the twelve copies per
operation, and the size 32 below which a rebuild is done at once) are those
of the library, chosen so that the proof of [Lemma 4.9](04-queue.md#lem-4-9)
goes through with simple constants; other choices are possible, and possibly
better in practice. Finally, the draw of a place without division is that of
[Lemire 2019](Z-bibliography.md#ref-lemire2019).

A typical use of a queue with its minimum is the minimum over a sliding
window, since the window gains an item at one end and loses one at the other
at each step. There, a deque of the indices whose values increase, which
drops from its back the indices that a new one dominates, answers in $O( 1
)$ amortized time with no stack, as in `maxSumSegmentLength()`
([§13.5](13-algorithm.md#sec-13-5)). [Chapter 24](24-concurrent.md#ch-24)
has the queues shared by threads (with one or many producers and consumers,
on a circular vector or on a list), and [chapter 9](09-priority.md#ch-9) the
priority queues. Also, the other deques of chapter 2 of Morin
(`DualArrayDeque`, the `Treque` and the deques of blocks) are in [chapter
2](#ch-2).

## 4.13 Exercises {#sec-4-13}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize the QUEUE on a circular vector of $m$ places, with each
   operation in $O( 1 )$ time and no division, and check that the queue
   refuses an item when full and still answers `contains()` when its items
   wrap around the end of the vector ([Montresor 2025,
   §4.3.3](Z-bibliography.md#ref-montresor2025)). Tests:
   `ch04.spec.ArrayQueue`, `beyond.queue-contains-sequence-equal`.
2. Check the axioms (4.1) on random sequences of operations, for a queue on
   a vector and for one on a list. Tests: `contract_queue.queue-axioms`,
   `contract_queue.array-queue`, `contract_queue.linked-queue`.
3. Realize the QUEUE on a singly linked list with pointers to its first and
   last cells, so that it also works after it has been emptied, and give it
   a copy and a move ([Montresor 2025, exercise
   4.10](Z-bibliography.md#ref-montresor2025)). Test: `ch04.ex4.10`.
4. Realize the DEQUEUE with every operation in $O( 1 )$ time, and explain
   why a singly linked list does not suffice ([Montresor 2025, exercise
   4.9](Z-bibliography.md#ref-montresor2025)). Tests: `ch04.ex4.9`,
   `contract_queue.dequeue`.
5. Give the output of the sequence `FIRS*T*IN***FI*RS***T*OUT*****` on a
   queue, a letter meaning `enqueue()` and an asterisk `dequeue()`, and the
   least capacity of an `ArrayQueue` that runs it. Test:
   `beyond.lifo-fifo-traces`.
6. Using only a queue, reverse the order of the items of a stack that has
   only `push()` and `pop()` ([Morin 2013, exercise
   1.4](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex1.4.reverse-stack`.
7. Solve the problems 3, 8 and 9 of [Morin 2013, exercise
   1.1](Z-bibliography.md#ref-morin-cpp) (the line 42 lines before each
   blank line with 43 lines kept, the lines of even rank before those of
   odd rank, the lines in a random order) with a queue, two queues and a
   random queue. Test: `ods_lists.ods-ex1.1.lines`.
8. Realize the `ArrayQueue` and the `ArrayDeque` of [Morin 2013, §2.3 and
   §2.4](Z-bibliography.md#ref-morin-cpp) on one circular vector that
   doubles, so that a queue in a steady state never grows, and the
   insertion and the removal at the rank $i$ take $O( 1 + \min\{ i , n - i
   \} )$ time; replay the operations of the two figures of those sections
   ([Figure 4.1](04-queue.md#fig-4-1) and [Figure 4.5](04-queue.md#fig-4-5)).
   Tests: `ods_lists.ods-2.3.array-queue`, `ods_lists.ods-2.4.array-deque`.
9. Make the moves of `insert(i, v)`, `erase(i)` and of the growth of the
   previous exercise by blocks of contiguous places rather than item by
   item, and check them when the items to move wrap around the end of the
   vector ([Morin 2013, exercise 2.7](Z-bibliography.md#ref-morin-cpp)).
   Test: `ods_lists.ods-2.4.array-deque`.
10. Replace the modulo of the previous exercises by the mask of a vector
    of $2^j$ places ([Morin 2013, exercise
    2.8](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.8.power-of-two`.
11. Rotate a LIST so that the item of rank $i$ goes to the rank $( i + r )
    \bmod n$, in $O( 1 + \min\{ r , n - r \} )$ time on a deque on a
    circular vector, and in $O( 1 )$ when its vector is full ([Morin 2013,
    exercise 2.6](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.6.rotate-list`.
12. Realize a deque on a circular vector that grows, with items that own
    memory or that can only be moved, an item of the deque inserted into it
    when it must grow, and random-access iterators, against `std::deque`.
    Tests: `engineered_queues.ring-deque`,
    `engineered_queues.ring-deque-items`,
    `engineered_queues.book-queues-engineered`.
13. Design a `RandomQueue`, whose `remove()` takes an item chosen uniformly
    at random among those inside, with `add(x)` and `remove()` in $O( 1 )$
    amortized time, and draw its places with no bias ([Morin 2013,
    exercise 2.3](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex2.3.random-queue`.
14. Realize a queue with no duplicates under the two policies of
    [§4.6](04-queue.md#sec-4-6), and move or copy the positive items of a
    queue using only its operations. Tests: `beyond.queue-algorithms`,
    `contract_queue.unique-queue`.
15. Design a `MinQueue` with `add(x)`, `remove()`, `size()` and `min()` in
    $O( 1 )$ amortized time ([Morin 2013, exercise
    3.15](Z-bibliography.md#ref-morin-cpp)), and then one in which every
    operation takes $O( 1 )$ time in the worst case. Tests:
    `ods_lists.ods-ex3.15.min-queue`,
    `ods_lists.ods-ex3.15.real-time-min-queue`.
16. Design a `MinDeque` with the four operations of the DEQUE, `size()`
    and `min()` in $O( 1 )$ amortized time ([Morin 2013, exercise
    3.16](Z-bibliography.md#ref-morin-cpp)), and then one in which every
    operation takes $O( 1 )$ time in the worst case. Tests:
    `ods_lists.ods-ex3.16.min-deque`,
    `ods_lists.ods-ex3.16.real-time-min-deque`.

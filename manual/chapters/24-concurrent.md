# 24. Concurrency {#ch-24}

[Source: `include/ADS/concurrent/SPSCQueue.h`, `MPSCQueue.h`,
`MPSCInbox.h`, `MPMCQueue.h`, `WorkStealingDeque.h`,
`ConcurrentUnionFind.h`, `ConcurrentCuckooTable.h`,
`ConcurrentFlatTable.h`, `MultiQueue.h`, `EventCount.h`, `Backoff.h`,
`StealablePriorityQueue.h`, and their `impl/*.tpp`]

We are concerned in this chapter with the structures that several threads
of one process read and change at the same time, without a lock or with as
few locks as possible. They are the queues between producers and
consumers, the deque of the scheduling by work stealing, the union-find and
the hash tables that the threads share, the relaxed priority queue, the
event count on which a thread sleeps until a condition holds, and the
priority queues of the workers of a parallel search, from which the idle
workers steal. These structures are the bricks of the parallel algorithms
and of the runtime of the workers of chapter 25, and of the parallel
searches of the library. Each of them gives, in a precise sense, the
result of a sequential structure of the earlier chapters; its cost,
however, depends on how the threads meet in memory more than on the number
of its operations.

The structure of the chapter is as follows. In
[§24.1](24-concurrent.md#sec-24-1) we fix the model of the threads and of
the memory, and the meaning of correct and of nonblocking, on which every
later claim rests; [§24.2](24-concurrent.md#sec-24-2) summarizes the
structures and their costs. Then [§24.3](24-concurrent.md#sec-24-3) and
[§24.4](24-concurrent.md#sec-24-4) treat the queues,
[§24.5](24-concurrent.md#sec-24-5) the deque of work stealing,
[§24.6](24-concurrent.md#sec-24-6) the union-find, and
[§24.7](24-concurrent.md#sec-24-7) and [§24.8](24-concurrent.md#sec-24-8)
the two hash tables. We treat the MultiQueue in
[§24.9](24-concurrent.md#sec-24-9), the ways in which a thread waits in
[§24.10](24-concurrent.md#sec-24-10), and the priority queues that are
stolen in [§24.11](24-concurrent.md#sec-24-11). The file `impl/Spin.tpp`
holds only two helpers that the other headers share (the pause of the
processor in a loop that spins, and the prefetch of a line of cache), and
we describe them where they are used.

## 24.1 The model of the threads and of the memory {#sec-24-1}

A *shared object* is a structure whose operations are called by several
threads, each thread being a sequence of calls. An operation takes time,
from its *invocation* to its *response*, and two operations of two threads
may overlap. A sequential specification (e.g., the axioms of the QUEUE of
chapter 4) does not specify overlapping operations, and one needs a
condition that relates the runs of the shared object to those of the
sequential one. Throughout the chapter we use the condition of
[Herlihy and Wing 1990](Z-bibliography.md#ref-herlihy1990). We say that a
run is *linearizable* if each operation seems to take effect at one instant
between its invocation and its response, its *linearization point*, and the
responses are those of the sequential object on the operations taken in
the order of their points; an object is linearizable if all its runs are.
A thread may then reason on a linearizable object as on the sequential
structure, and the headers name the point of each operation (e.g., the
compare-and-swap that claims a place, or the store that publishes an
item). [Figure 24.1](24-concurrent.md#fig-24-1) shows a run of three
operations on a queue. The push of `b` ends before the pop starts, while
the push of `a` overlaps both; the answer `b` of the pop is explained by
the order of the points push(`b`), push(`a`), pop.

<span id="fig-24-1"></span>
![Figure 24.1. Three operations of three threads on a FIFO queue, each an interval from its invocation to its response, with its linearization point (the dot); the order of the points, push(b), push(a), pop, explains the answer b of the pop.](../figures/concurrent-linearizability.svg)

We call an implementation *blocking* if a thread stopped in the middle of an
operation (descheduled by the system, or simply "slow") may keep the other
threads from completing theirs, as a thread that holds a lock does. We call
it *lock-free* if, whatever the threads that stop, some thread that keeps
running completes an operation in a finite number of its steps. Thus the
system as a whole makes progress, though a single thread may starve. It is
*wait-free* if each thread that keeps running completes each of its
operations in a finite number of its steps ([Herlihy
1991](Z-bibliography.md#ref-herlihy1991)). A loop that retries a
compare-and-swap is lock-free, since the compare-and-swap of a thread fails
only because that of another thread succeeded; it is not wait-free,
however, since the same thread may lose every round. We write
$\mathrm{CAS}( x , e , d )$ for the compare-and-swap, one atomic
read-modify-write that sets $x \gets d$ and answers true if $x = e$, and
answers false otherwise (in the code it also writes the value found into
$e$).

The memory is that of C++, which the headers use through `std::atomic`
([Boehm and Adve 2008](Z-bibliography.md#ref-boehm2008)). A *data race* is
a pair of accesses of two threads to one location, one of them a write,
neither of them atomic and neither ordered before the other. A program with
a data race has no defined meaning, and a word that two threads may access
at once is therefore always an atomic. Each atomic access carries an
*order*. A *relaxed* access is atomic and nothing more. A store with
*release* and a load with *acquire* that reads its value *synchronize*: the
writes that the storing thread made before the store *happen before* the
reads that the loading thread makes after the load. This is how an item
written by plain stores is handed to another thread, by publishing
afterwards an index or a pointer with release. A *sequentially consistent*
access is acquire and release and, furthermore, takes a place in a single
total order $S$ of all such accesses (and of the sequentially consistent
fences), consistent with the order of each thread.

Between threads, the orders come only from these. A relaxed access orders
no other access, and two threads that each write one word and then read
the word of the other, with release and acquire, may both miss the write
of the other; only $S$ excludes it. Several lemmas below (the last item of
a deque, the growth of a hash table, the sleep of a waiter) rest on this
pattern, and we prove them by placing four accesses in $S$.

We now recall two facts of the hardware that decide the costs. A write to a line of cache
takes the line away from the caches of the other cores, which then miss on
it. Two words written by two threads on one line (*false sharing*) make the
line travel at each write; the headers therefore put each such word on a
separate line, often on a pair of lines, since many processors of the x86
family fetch the lines two at a time. Also, a read-modify-write (e.g., a
compare-and-swap or an exchange) costs several times a plain access, and it
orders the memory around it. A structure is "fast" when its common case has
none, and [Table 24.1](24-concurrent.md#tab-24-1) states, for each operation, how many it pays. Finally,
we recall the *ABA problem* of a compare-and-swap on a pointer. A thread
reads the top $A$ of a stack and its successor $B$; another thread pops
$A$ and $B$ and pushes $A$ again; then the compare-and-swap of the first
thread from $A$ to $B$ succeeds, and puts back on the stack a cell that is
no longer there. In ADS the headers avoid it by construction rather than by
counters of versions (cf. [Lemma 24.3](24-concurrent.md#lem-24-3)).

Throughout the chapter $p$ is the number of the threads that use an object
and $c$ the capacity of a bounded queue, a power of 2, so that the index
$i$ lies at the place $i \bmod c$, computed by a mask. The head $h$ and the
tail $t$ of a queue are integers that only grow, and the items are those of
the indices in $[ h , t )$.

## 24.2 The structures and their costs {#sec-24-2}

In [Table 24.1](24-concurrent.md#tab-24-1) we list the structures of the area,
the threads that each admits, the progress it guarantees and the cost of
its operations, which the following sections prove one by one.

<span id="tab-24-1"></span>**Table 24.1.** The structures of
`ADS/concurrent`, the threads they admit, their progress and the cost of
their operations; $m$ is the number of the pairs of a queue or of a heap,
$n$ that of the elements of the union-find or of the queues of the
MultiQueue, and $k$ that of the messages taken at once.

| Structure | Threads | Progress | Cost of an operation |
|---|---|---|---|
| `SPSCQueue` | one producer, one consumer | wait-free | $O( 1 )$, no read-modify-write |
| `MPSCQueue` | many producers, one consumer | wait-free push but for the allocation of a block | $O( 1 )$ amortized, no read-modify-write but per block |
| `MPSCInbox` | many producers, one consumer | lock-free push, wait-free take | one CAS per push, one exchange per take, plus $O( k )$ |
| `MPMCQueue` | many producers, many consumers | lock-free but for a thread stopped between claim and publication | one CAS per operation without contention |
| `WorkStealingDeque` | one owner, many thieves | wait-free push, lock-free take and steal | $O( 1 )$ amortized for the owner, a CAS only on the last item; one per steal |
| `ConcurrentUnionFind` | any | lock-free | $O( \log_{2 + m / n} n )$ amortized on one thread |
| `ConcurrentCuckooTable` | any | blocking (a lock per region), lookups without locks for atomic keys and values | $O( 1 )$ expected, $O( 1 )$ amortized for the doublings |
| `ConcurrentFlatTable` | any | wait-free lookups but across a growth, insertions blocking only during a growth | $O( 1 )$ expected, $O( 1 )$ amortized for the growths; no removals |
| `MultiQueue` | any, by a handle each | blocking (a lock per queue, taken at the first try) | $O( \log m )$ amortized, $q / ( q - 1 )$ draws expected; relaxed |
| `StealablePriorityQueue` | one owner, many thieves | wait-free for the owner, lock-free steal | $O( \log m )$ for the owner, no read-modify-write; two exchanges per steal |
| `EventCount` | any | a waiter sleeps, a notifier never waits | a fence and a read when nobody waits |

The queues of one producer or of one consumer are cheaper than the general
one because each of their indices has a single writer. Such an index is
written by plain stores with release and read by loads with acquire, and no
read-modify-write is needed; many writers of one index, instead, must agree
by a compare-and-swap, and contend on its line. This remark runs through
the whole chapter. Indeed, the structures are designed so that the "common" case of
an operation writes only words that no other thread writes, and they pay a
read-modify-write only where two threads may really want the same thing
(e.g., the last item of a deque, a free place of a table, or an offer of a
queue that is stolen, cf. [§24.11](24-concurrent.md#sec-24-11)).

## 24.3 Queues of one producer {#sec-24-3}

`SPSCQueue` is the circular vector of the queue of chapter 4 shared by two
threads. Only the producer writes the tail $t$, and only the consumer
writes the head $h$; each index is thus an atomic written with release and
read with acquire, and no lock is needed (the queue of [Lamport
1983](Z-bibliography.md#ref-lamport1983)). The queue is full when
$t - h = c$ and empty when $h = t$. Since the indices only grow and are
reduced
by the mask at each access, and the two tests therefore hold also when the
integers wrap around. Each thread keeps besides a copy of the index of the
other one, $\mathit{seen}_P \le h$ for the producer and
$\mathit{seen}_C \le t$ for the consumer, and reads the index again only
when the queue looks full or empty by the copy ([Lee, Bu and Chandranmenon
2010](Z-bibliography.md#ref-lee2010)). A stale copy is never ahead of the
index it copies; hence it can only make the queue look fuller or emptier
than it is. [Figure 24.2](24-concurrent.md#fig-24-2) shows the vector
before and after a push, with both copies stale and neither of them read.

<span id="fig-24-2"></span>
![Figure 24.2. An `SPSCQueue` of capacity 8 with the items `b`, `c`, `d`, before and after `push(e)`; the producer writes the place $t \bmod 8$ and then the tail, and the copies $\mathit{seen}_P$ of the head and $\mathit{seen}_C$ of the tail, both stale, are not read, since the queue looks neither full to the producer nor empty to the consumer.](../figures/concurrent-spsc.svg)

<span id="alg-24-1"></span>**Algorithm 24.1.** The push and the pop of
`SPSCQueue` ([Lamport 1983](Z-bibliography.md#ref-lamport1983), with the
copies of [Lee et al. 2010](Z-bibliography.md#ref-lee2010) and the wait for
a batch of [Wang et al. 2013](Z-bibliography.md#ref-wang2013));
$A[ 0 .. c - 1 ]$ is the vector.

--8<-- "generated/SPSCQueue-emplace.pseudo.md"

--8<-- "generated/SPSCQueue-tryPop.pseudo.md"

We view the correctness of the queue as the transfer of each item, and of each
place, between the two threads in the order of happening before. Two pairs
of a release and an acquire give it.

<span id="lem-24-1"></span>**Lemma 24.1.** *Let the producer write the item
of the index $t$ and then store $t + 1$ in the tail with release, and let
the consumer load the tail with acquire, read the items of the indices
below it, and then store the new head with release, which the producer
loads with acquire before it reuses a place. Then the consumer reads each
item whole, after it was written, and the producer never overwrites an item
not yet read:*

$$
\mathrm{write}( A[ i ] ) \to \mathrm{read}( A[ i ] ) \to
\mathrm{write}( A[ i + c ] ) \quad \forall i , \tag{24.1}
$$

*where $\to$ is the order of happening before. The queue is wait-free, and
it is linearizable with the point of a push at its store of the tail and
that of a pop at its store of the head (at its load of the tail if it
finds the queue empty).*

*Proof.* We follow the item of the index $i$. Its write precedes in the producer the release store
of a tail larger than $i$, which the consumer reads by an acquire load
before it reads $A[ i ]$. These two accesses synchronize, whence the
first arrow of (24.1). Similarly, the second arrow follows from the
argument on
the head, since the producer writes $A[ i + c ]$ only after it has read a
head larger than $i$, i.e., after it has found $t - h < c$. A stale copy can
only make the test of full (of empty) succeed where the index would make it
fail, and the copy is read again before the queue is declared full (empty);
thus no push and no pop fails wrongly. Each operation runs a constant
number of steps, whatever the other thread does. At its store of the tail a
push makes its item visible at the end of the queue, and at its store of
the head a pop removes the first item, which it has read; the order of
these points is the order of the operations of the sequential
queue. $\square$

The code follows [Algorithm 24.1](24-concurrent.md#alg-24-1) line by line. Here the item is built in
place,
and the comparison with the copy comes first, so that the common case reads
no line that the other thread writes:

```cpp
--8<-- "generated/SPSCQueue-emplace.cpp"
```

```cpp
--8<-- "generated/SPSCQueue-tryPop.cpp"
```

Two "details" of engineering change the time of a queue between two cores
several times over, without changing its argument. First, the two indices
and the two copies lie on four separate lines of cache, grouped in pairs of
lines. A thread that spins on a full or empty queue rewrites its copy, and
this layout keeps it from invalidating the line that the other thread
writes, also through the prefetch of the adjacent line. Also, the places
are raw memory with a line of padding at each end, and the first and the last
items share no line with other data. Second, a consumer that is right
behind a "streaming" producer, and looks again at once, would take from it
at each item the lines of the tail and of the item it is writing. When the
last pop took an item and the consumer finds fewer than
$\min( 64 , c / 2 )$ new ones, it therefore waits 64 pauses of the
processor for a batch before it takes them (the backtracking of B-Queue,
[Wang et al. 2013](Z-bibliography.md#ref-wang2013)). Then the lines travel
once per batch. After a pop that found no item, instead, the consumer takes
at once what comes, and a single item after a "silence" costs no wait.
Furthermore, a consumer that finds the queue empty prefetches the line of
its next place before it reads the tail again. When the item has come, the two
lines travel together, and a round trip between two threads usually costs
one miss a side instead of two in a row.

Two batch operations publish several items by one store of the tail and
free several places by one store of the head, which is the "cheapest" way
in which one can move many items between two threads. Note that `consume( f , n )` calls
$f$ on each item in place, and dequeues the items called also when a call
throws.

--8<-- "generated/SPSCQueue-pushMany.pseudo.md"

--8<-- "generated/SPSCQueue-consume.pseudo.md"

`MPSCQueue` serves many producers and one consumer, and is unbounded. Each
producer has a *lane*, i.e., a queue of one producer and one consumer, and
the producers never compete with one another. Its consumer takes the lanes
in turn, and drains the one it is on before it moves to the next. A lane is
a list of blocks of $B$ places (1024 by default, a power of 2), after the
list of bounded buffers of [Aldinucci et al.
2012](Z-bibliography.md#ref-aldinucci2012). Each producer writes the places
of its last block in order and publishes, after each item, their count $f$
in the block, by a store with release and no read-modify-write. When the
block is full it links a new one after it, by a store with release of the
link. In turn, the consumer reads the count of its block once for all the items that
it shows, and takes them. When it has taken the $B$ items of a block and
finds the next one linked, it gives the block back to its lane, on a stack
that the producer empties at once, by an exchange, when it needs a block
and before it allocates one ([Figure 24.3](24-concurrent.md#fig-24-3)). A
producer is a handle that `producer()` returns, used by one thread at a
time. All the lanes form a list built by the push of Treiber of
[§24.4](24-concurrent.md#sec-24-4), and they live as long as the queue,
also when their producer is gone and its items are still to be taken.

<span id="fig-24-3"></span>
![Figure 24.3. An `MPSCQueue` with two lanes of blocks of 4 places; $f$ is the count of the items written in a block, the consumer reads from the place below its arrow and the producer writes at the place under its own; the first lane has a block that the consumer gave back, on the stack from which the producer takes its next block.](../figures/concurrent-mpsc.svg)

<span id="alg-24-2"></span>**Algorithm 24.2.** The operations of
`MPSCQueue` (the list of bounded buffers of [Aldinucci et al.
2012](Z-bibliography.md#ref-aldinucci2012), the wait for a batch of [Wang
et al. 2013](Z-bibliography.md#ref-wang2013), and the stack of [Treiber
1986](Z-bibliography.md#ref-treiber1986) for the lanes).

--8<-- "generated/MPSCQueue-emplace.pseudo.md"

--8<-- "generated/MPSCQueue-popInto.pseudo.md"

--8<-- "generated/MPSCQueue-producer.pseudo.md"

Each lane is a queue of Lamport whose block holds the count of its items in
place of the tail, and [Lemma 24.1](24-concurrent.md#lem-24-1) applies (cf. (24.1)) to it place by place: the item is
written before its count is published, and the consumer reads the count
with acquire before the items. A block is reused by its producer only after
the consumer has read all its $B$ items and has pushed it, with release, on
the stack of the blocks given back. Its producer takes that stack by an
exchange with acquire, and therefore (24.1) holds for each place of a block
across its reuses. This stack has one pusher, the
consumer, and one taker, which takes it all at once; it has thus no ABA
problem (cf. [Lemma 24.3](24-concurrent.md#lem-24-3)).

<span id="lem-24-2"></span>**Lemma 24.2.** *A lane of `MPSCQueue` that has
held at most $m$ items at once keeps at most $\lceil m / B \rceil + 1$
blocks. A push costs $O( 1 )$ time and no read-modify-write, but for the
push that fills a block, which costs an exchange, or an allocation when
the lane has fewer blocks than it needs.*

*Proof.* At any moment the blocks of a lane in use are those from the block
of the consumer to that of the producer, which hold the items present; they
are at most $\lceil m / B \rceil + 1$, since the first and the last may be
partly empty. A block out of them lies in one of the two stacks of blocks
to reuse, and a producer allocates a block only when both stacks are empty,
i.e., when all the blocks of the lane are in use. Hence the lane never
holds more blocks than the largest number in use at once. A push writes its
item and a count, two plain stores, unless it fills the block; then it
takes a block from its spare stack, or all the stack given back by an
exchange, or a new block. $\square$

We conclude that in a steady state a lane cycles over the same blocks, which stay
in the caches. Here the count and the link of a block share a line that only the
producer writes, and the consumer touches the lines of the producer once
per batch, with the same wait for a batch as `SPSCQueue` (when a call took
some items and the next one finds fewer than $\min( B , 64 )$ in the
block). Furthermore, the consumer of either queue may sleep until an item comes,
by `waitPop()`, on the event count of [§24.10](24-concurrent.md#sec-24-10);
the producers then call `notify()` after their pushes.

## 24.4 Queues of many producers or of many consumers {#sec-24-4}

`MPSCInbox` is the "simplest" of the queues of many producers, e.g., the
inbox of a worker of a parallel search, into which the other workers send
their messages. A producer pushes a cell on a stack by a loop of
$\mathrm{CAS}( \mathit{top} , \mathit{next} , \mathit{cell} )$ (the stack of
[Treiber 1986](Z-bibliography.md#ref-treiber1986)). Its consumer takes all
the stack at once, by $\mathrm{exchange}( \mathit{top} , \mathbf{nil} )$,
and reverses it; the messages of each producer thus come out in the order
in which it sent them ([Figure 24.4](24-concurrent.md#fig-24-4)). Hence
`take()` costs one atomic operation however many messages there are, plus
$O( k )$ time for the $k$ messages taken, and the consumer never competes
with the producers on single items. This inbox suits the messages that
usually come in bursts, at the price of an allocation per message.

<span id="fig-24-4"></span>
![Figure 24.4. The stack of an `MPSCInbox` before and after `push(d)`, whose compare-and-swap links the new cell above the top it read, and after `take()`, which exchanges the top with nil and returns the messages reversed, in the order of sending.](../figures/concurrent-inbox.svg)

<span id="alg-24-3"></span>**Algorithm 24.3.** The push and the take of
`MPSCInbox` ([Treiber 1986](Z-bibliography.md#ref-treiber1986)).

--8<-- "generated/MPSCInbox-push.pseudo.md"

--8<-- "generated/MPSCInbox-take.pseudo.md"

```cpp
--8<-- "generated/MPSCInbox-push.cpp"
```

<span id="lem-24-3"></span>**Lemma 24.3.** *Let the cells of a stack of
Treiber be pushed one at a time, by a compare-and-swap whose new value is a
cell of the pushing thread, and be taken only all at once, by an exchange.
Then the ABA problem does not occur: each successful push links its cell
above the true top, and each take returns all the cells pushed before it
that are not yet taken.*

*Proof.* The ABA problem needs a compare-and-swap whose expected value was
read before the cell it names was removed and put back. Here we have only
one compare-and-swap, that of a push, whose expected value is the top read by
the pusher. If it succeeds, the top is that value at that instant, whatever
happened in between (e.g., a take, then pushes that brought the top back to
the same address after a reuse of the memory); the cell becomes the new top
above the cell that is really there, since a push never reads the
successor of the top. No cell is ever popped singly. An exchange is one
atomic step, and the cells it returns are those of the pushes that
precede it in the order of the modifications of $\mathit{top}$. Finally, a
push publishes its cell with release and the take reads the top with acquire,
and therefore the contents of the cells are seen. $\square$

The same argument holds for the list of the lanes of `MPSCQueue` and for
its stacks of blocks given back. A push is lock-free but not wait-free,
since a pusher may lose every round to other pushers; a take is wait-free.

`MPMCQueue` is the bounded queue of many producers and many consumers of
[Vyukov 2010](Z-bibliography.md#ref-vyukov2010), on a circular vector of $c$
places. Each place carries a *sequence* number besides the room of an item:
the place of the index $i$ holds the sequence $i$ when it waits for the
push of $i$, $i + 1$ when it holds the item of $i$, and $i + c$ when that
item has been taken and the place waits for the push of $i + c$. A producer
reads the tail $p$ and the sequence $s$ of its place. If $s = p$ it claims
the index by $\mathrm{CAS}( t , p , p + 1 )$, builds the item and publishes
it by storing the sequence $p + 1$ with release; if $s < p$ the queue is
full; and if $s > p$ another producer took $p$, and it reads the tail
again. A consumer does the same on the head $q$ with the sequence $q + 1$,
and frees the place by storing $q + c$
([Figure 24.5](24-concurrent.md#fig-24-5)). Its tail and its head lie on
separate lines, and producers and consumers meet only at the places they
share, with one read-modify-write per operation.

<span id="fig-24-5"></span>
![Figure 24.5. An `MPMCQueue` of capacity 4 with the items of the indices 5 and 6, before and after a push that claims the index 7: the place 3 held the sequence 7 (free for the index 7), and the push writes the item and then the sequence 8; the place 0 holds the sequence 8, i.e., it waits for the push of the index 8.](../figures/concurrent-mpmc.svg)

<span id="alg-24-4"></span>**Algorithm 24.4.** The push and the pop of
`MPMCQueue` ([Vyukov 2010](Z-bibliography.md#ref-vyukov2010)); $C[ i ]$ is
the place $i \bmod c$ and $C[ i ].s$ its sequence.

--8<-- "generated/MPMCQueue-emplace.pseudo.md"

--8<-- "generated/MPMCQueue-tryPop.pseudo.md"

```cpp
--8<-- "generated/MPMCQueue-emplace.cpp"
```

We remark that the code compares the sequence with the index on their difference, taken as
a signed integer; the test thus holds across the wrap-around of the
integers, as the tests of `SPSCQueue` do.

<span id="lem-24-4"></span>**Lemma 24.4.** *In `MPMCQueue`, each index is
claimed by one producer and by one consumer. The consumer of the index $i$
reads the item that the producer of $i$ wrote, and the producer of $i + c$
writes the place only after the consumer of $i$ has read it; a failed
compare-and-swap means that another thread claimed the index.*

*Proof.* We note first that a compare-and-swap of the tail from $p$ to $p + 1$ succeeds for one
thread only, since the tail never goes back, and the same holds for the
head. In fact, the producer of $i$ publishes by storing $i + 1$ with release after
it has written the item, and the consumer claims $i$ only after an acquire
load that read $i + 1$, whence the first arrow of (24.1). In turn, the
consumer stores $i + c$ with release after it has read the item, and the producer of
$i + c$ claims its index only after it has read the sequence $i + c$ with
acquire, whence the second one. A producer that reads a sequence below $p$
finds the item of $p - c$ not yet taken, i.e., the queue full, and one that
reads a sequence above $p$ was overtaken, and reads the tail again. Also,
the failure of its compare-and-swap means that another producer claimed $p$,
which is progress of the queue. Consumers are symmetric. $\square$

The queue is lock-free in the common case, but not in the strict sense of
[§24.1](24-concurrent.md#sec-24-1). In fact, a thread stopped between its
compare-and-swap and its publication holds up the threads that come round
to its place, which find a sequence that means neither "free" nor "full". For
the same reason a pop may answer that the queue is empty while a push that
has claimed its index has not yet published it, which is consistent with a
linearization point of the push at its publication. The rings of [Nikolaev
2019](Z-bibliography.md#ref-nikolaev2019), with a cycle per place, are
lock-free in the strict sense, at the price of a somewhat more complex
protocol. In chapter 25 the farm takes its tasks from an `MPMCQueue`; there
such a stop happens only when the system deschedules a thread, and it then
delays the other threads only until that thread runs again.

## 24.5 The deque of work stealing {#sec-24-5}

In the scheduling by work stealing each worker pushes and takes its tasks
at the bottom of its deque, as on a stack, which keeps the tasks it made
last, and their data, in its cache. An idle worker steals the oldest task,
at the top of the deque of another worker; in a computation by fork and
join this task is often the root of a "large" subtree of work ([Blumofe and
Leiserson 1999](Z-bibliography.md#ref-blumofe1999); the bound on the time
of this schedule is in chapter 25). `WorkStealingDeque` is the deque of
[Chase and Lev 2005](Z-bibliography.md#ref-chase2005). Its owner thread
pushes and takes at the bottom, and any other thread steals from the top,
without locks, on a circular vector of $2^k$ places that the owner doubles
when it is full. The bottom $b$ is written by the owner only, and the top
$t$, which only grows, only by a compare-and-swap; the items are those of
$[ t , b )$, at the places of their indices modulo the size of the vector
([Figure 24.6](24-concurrent.md#fig-24-6)). Its items are kept in atomics
of relaxed order, and their type must therefore be trivially copyable
(e.g., an index or a pointer to a task).

<span id="fig-24-6"></span>
![Figure 24.6. A `WorkStealingDeque` of 8 places with the items `a`, `b`, `c` in $[ t , b ) = [ 2 , 5 )$, after `take()` of the owner, which takes `c` at the bottom, and after `steal()` of a thief, which takes `a` at the top; the shaded cell is the place that the operation empties.](../figures/concurrent-deque.svg)

<span id="alg-24-5"></span>**Algorithm 24.5.** The operations of
`WorkStealingDeque` ([Chase and Lev 2005](Z-bibliography.md#ref-chase2005),
with the orders of memory of [Lê et al. 2013](Z-bibliography.md#ref-le2013));
$A$ is the vector of $|A|$ places, the index $i$ at its place
$i \bmod |A|$.

--8<-- "generated/WorkStealingDeque-push.pseudo.md"

--8<-- "generated/WorkStealingDeque-take.pseudo.md"

--8<-- "generated/WorkStealingDeque-steal.pseudo.md"

We first remark that, when more than one item is left, the owner and the
thieves take different items (the owner $b - 1$ and a thief $t$), and only the thieves race among
themselves, on the compare-and-swap of $t$. The somewhat "delicate" case is
the last item, which the owner and a thief must not both take, nor both leave. Here
the orders of memory matter, and [Lê et al.
2013](Z-bibliography.md#ref-le2013) placed two fences that make the deque
correct in the model of C11. In ADS, the header writes those fences as sequentially
consistent accesses to the bottom and to the top, since the sanitizer of
threads models these accesses but not the fences themselves.

<span id="lem-24-5"></span>**Lemma 24.5.** *Let `take()` store $b - 1$ in
the bottom and then load the top, and `steal()` load the top and then the
bottom, all four accesses sequentially consistent. Then, when one item is
left, the owner and a thief do not both take it, and they do not both leave
it while it is there.*

*Proof.* We place the four accesses in the total order $S$ of
[§24.1](24-concurrent.md#sec-24-1). If the load of the bottom of the thief
follows in $S$ the store of the owner, the thief either sees $t \ge b$ and
gives up, or it sees the item; in the second case both threads race for the
item by a compare-and-swap of $t$ from the same value, which only one of
them wins. Otherwise the load of the bottom of the thief precedes the store
of the owner, and hence the load of the top of the thief precedes the load
of the top of the owner. If the thief has already taken the item, the owner
sees the top raised and finds the deque empty; if not, the owner sees
$t = b - 1$ and races for the item by the compare-and-swap, as the thief
does. In each case at most one compare-and-swap of $t$ from the same value
succeeds, and when both threads race one of them wins. $\square$

We remark that with release and acquire only both threads could read the old
value of the word written by the other, and both take the item. In the
code, the two sequentially consistent pairs are these:

```cpp
--8<-- "generated/WorkStealingDeque-take.cpp"
```

```cpp
--8<-- "generated/WorkStealingDeque-steal.cpp"
```

A thief reads the item before the compare-and-swap that claims it, from the
vector that it reads after the bottom; the owner publishes a new vector with
release before it publishes a bottom that needs it. A doubling copies the
items of $[ t , b )$ to the same indices of the new vector, and a thief that
read the old vector thus reads from it an item that the copy did not
change; its compare-and-swap then decides whether the item is its. All the
old vectors are kept until the deque is destroyed, and no read is ever of freed
memory.

<span id="lem-24-6"></span>**Lemma 24.6.** *The doublings of a
`WorkStealingDeque` of initial capacity $c_0$ cost $O( 1 )$ amortized time
per push, and the vectors kept occupy less than twice the vector in use.*

*Proof.* We charge each doubling to the pushes before it. A doubling from
$2^k$ places copies $b - t = 2^k$ items. It happens
only after at least $2^{k - 1}$ pushes since the previous doubling (or $c_0$
pushes for the first), since a push adds one item and the vector was at
most half full after the previous doubling. Hence the copies are at most
twice the pushes, as for the vector of [Lemma 3.1](03-stack.md#lem-3-1).
The sizes of the vectors form the geometric series
$c_0 , 2 c_0 , \ldots , 2^j c_0$, whose sum is less than
$2^{j + 1} c_0$. $\square$

Thus `push()` and `take()` cost $O( 1 )$ time and no read-modify-write but
when `take()` reaches the last item, and `steal()` costs one
compare-and-swap. All three operations are linearizable: a push at its store of
the bottom, a take at its store of $b - 1$ (or at its compare-and-swap on
the last item), and a steal at its compare-and-swap. `push()` is
wait-free, while `take()` and `steal()` are lock-free; a thief fails only
when another thread took the item it wanted, and `steal()` then returns
false, so that the caller may try another victim. This deque is that of the
pool of threads of chapter 25. Its argument returns in
[§24.11](24-concurrent.md#sec-24-11), where the queues of a search are
stolen without making their owner race for each item.

## 24.6 A union-find shared by threads {#sec-24-6}

`ConcurrentUnionFind` realizes the MFSET of chapter
11 on the elements $0 , \ldots , n - 1$, for threads
that unite and find at once ([Jayanti and Tarjan
2021](Z-bibliography.md#ref-jayanti2021)). It keeps a forest of parents
$p[ 0 .. n - 1 ]$, a root being its own parent, each parent an atomic word
changed only by a compare-and-swap (with no lock). Thus `find( x )` climbs
to the root by *splitting*: each element on the way is given the parent of
its parent, by a compare-and-swap whose failure is "harmless", and the paths
halve their lengths as they are walked. This is the concurrent form of the
path splitting of [Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984). Then `unite( x , y )` finds the
two roots and hooks the larger one under the smaller, by a compare-and-swap
of the parent of the larger ([Figure 24.7](24-concurrent.md#fig-24-7)). The
compare-and-swap fails only if another thread hooked that root first, and
it is then tried again from the roots found anew.

<span id="fig-24-7"></span>
![Figure 24.7. A `ConcurrentUnionFind` before and after `unite(6, 5)`: the splitting of `find(6)` gives 6 its grandparent 3, `find(5)` finds the root 0, and the larger root 3 is hooked under the smaller root 0; the shaded elements are those whose parent changed.](../figures/concurrent-unionfind.svg)

<span id="alg-24-6"></span>**Algorithm 24.6.** The operations of
`ConcurrentUnionFind`: the splitting and the linking by index of [Jayanti
and Tarjan 2021](Z-bibliography.md#ref-jayanti2021).

--8<-- "generated/ConcurrentUnionFind-find.pseudo.md"

--8<-- "generated/ConcurrentUnionFind-unite.pseudo.md"

--8<-- "generated/ConcurrentUnionFind-sameSet.pseudo.md"

--8<-- "generated/ConcurrentUnionFind-compress.pseudo.md"

We now show that the correctness rests on an invariant of the order of the
elements, which no interleaving of the operations can break.

<span id="lem-24-7"></span>**Lemma 24.7.** *If a root $x$ is hooked only
under a root $y < x$, by $\mathrm{CAS}( p[ x ] , x , y )$, and a parent
$p[ x ]$ is otherwise only replaced by an ancestor of $x$, then at every
moment*

$$
p[ x ] \le x \quad \forall x , \tag{24.2}
$$

*the forest has no cycle, and the root of each tree is its least element;
hence the label $\mathrm{find}( x )$ of a set does not depend on the order
of the operations, once they have ended.*

*Proof.* We argue by induction on the successful compare-and-swaps. A hook succeeds
only if $x$ is still a root, and sets $p[ x ] = y < x$. The splitting of
`find()` replaces $p[ x ] = u$ by $g = p[ u ] \le u$, by a compare-and-swap
that fails if $p[ x ]$ changed in the meantime; the parent of $u$ only
moves up the tree of $u$, so $g$ is an ancestor of $x$, and $g \le u \le x$
keeps (24.2). Along a path the elements decrease strictly (but at the
root), which excludes a cycle and makes the root the least element of its
tree. `compress()` sets $p[ x ]$ from $u$ to the root found from $u$, an
ancestor of $x$, and the same argument applies. $\square$

```cpp
--8<-- "generated/ConcurrentUnionFind-find.cpp"
```

```cpp
--8<-- "generated/ConcurrentUnionFind-unite.cpp"
```

A failed compare-and-swap of a splitting only leaves a longer path, and a
failed hook means that another thread hooked $x$ first, which is progress;
hence the structure is lock-free. All the operations are linearizable. A
`unite()` takes effect at its successful hook (or, when it answers false,
at the second `find()` that found the same root), and a `find()` at the
read that finds a root. A `sameSet()` that answers false takes effect at
its second `find()`, provided that the root of $x$ is still a root after
it, which the pseudocode checks. Each hook uses the orders acquire and
release, and therefore a thread that follows a parent sees the writes that
preceded the hook. After the compression of all the
elements, with no union running, `parent( x )` is the label of the set of
$x$, the least element; the parallel connected components of chapter 25
use it in this way.

<span id="lem-24-8"></span>**Lemma 24.8.** *On one thread,
`ConcurrentUnionFind` performs $m \ge n$ operations on $n$ elements in
$O( m \log_{2 + m / n} n )$ steps.*

*Proof.* On one thread the compare-and-swaps all succeed, and the structure
is the union-find with the linking by index and the path splitting, whose
bound is that of [Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984) for any linking that does not depend
on the sizes. $\square$

The linking by index has the advantage of canonical labels, at the price of
a somewhat weaker bound, logarithmic rather than nearly constant. The randomized
linking of [Jayanti and Tarjan 2021](Z-bibliography.md#ref-jayanti2021)
hooks by a random order of the elements instead of their indices. It bounds
the expected work on $p$ threads by $O( m ( \alpha( n , m / ( n p ) ) +
\log( n p / m + 1 ) ) )$, but its labels are not the least elements. Neither
the constructor nor `size()` is concurrent with the other operations, and $n < 2^{31}$, since the parents
are words of 32 bits.

## 24.7 A cuckoo table under locks per region {#sec-24-7}

`ConcurrentCuckooTable` is the cuckoo hashing with buckets of 4 places and
tags of 8 bits of `BucketCuckooTable` (chapter
7), made concurrent with one lock per *region* of
buckets and, for keys and values that the processor reads atomically,
lookups that take no lock at all. A key of hash $h$ has two buckets: $b_1$,
given by the top bits of $h$, and

$$
b_2 = b_1 \oplus \left( \left\lfloor g( \tau ) / 2^{64 - r} \right\rfloor
\vee 1 \right) , \tag{24.3}
$$

where $\tau$ is the tag of $h$, $g$ a mixing of the tag (the partial-key
cuckoo hashing of [Fan, Andersen and Kaminsky
2013](Z-bibliography.md#ref-fan2013)), and $R = 2^r$ the buckets of a
region. Thus one can compute the other bucket of a key from its bucket and
its tag, and both lie in the same region. The $B$ buckets form $P = \min( 2048 ,
\max( 1 , B / 256 ) )$ regions of $R = B / P$ buckets each. A region has
one lock, i.e., a counter of versions (on a separate line), odd while the
lock is held (the lock striping of [Herlihy and Shavit
2008, chapter 13](Z-bibliography.md#ref-herlihy2008), with a stripe per
region instead of a stripe of scattered buckets). A lock is taken by a
compare-and-swap from an even value after a spin on plain loads, with the
pause of the processor and, after 64 rounds, a yield to the system
([Figure 24.8](24-concurrent.md#fig-24-8)).

<span id="fig-24-8"></span>
![Figure 24.8. A `ConcurrentCuckooTable` of 16 buckets in 4 regions of 4 buckets, each with the version $v$ of its lock (odd, as that of the region 1, while a writer holds it); the two buckets $b_1$ and $b_2$ of a key lie in its region, so that an operation on the key takes one lock.](../figures/concurrent-cuckoo.svg)

An operation on a key takes one lock, that of its region. The cuckoo path
of an insertion never leaves the region; it is searched in breadth, and its
keys are moved, under that lock only, with no state to check again. By
contrast, [Li et al. 2014](Z-bibliography.md#ref-li2014) search without
locks and lock two buckets per move. Only the doubling, `reserve()`, `clear()`,
`forEach()` and `consistent()` take more locks.

<span id="alg-24-7"></span>**Algorithm 24.7.** The lookup and the insertion
of `ConcurrentCuckooTable`: the seqlock of [Boehm
2012](Z-bibliography.md#ref-boehm2012) on the partial-key cuckoo hashing of
[Fan et al. 2013](Z-bibliography.md#ref-fan2013), with the search in
breadth of [Li et al. 2014](Z-bibliography.md#ref-li2014); $v_R$ is the
version of the region $R$ and $n_R$ the number of its pairs.

--8<-- "generated/ConcurrentCuckooTable-find.pseudo.md"

--8<-- "generated/ConcurrentCuckooTable-insert.pseudo.md"

--8<-- "generated/ConcurrentCuckooTable-update.pseudo.md"

--8<-- "generated/ConcurrentCuckooTable-erase.pseudo.md"

We prove the table correct by an argument on the locks, one on the lookups
without locks and one on the doubling.

<span id="lem-24-9"></span>**Lemma 24.9.** *The locks of
`ConcurrentCuckooTable` cannot deadlock.*

*Proof.* An operation on a key holds one lock and waits for no other while
it holds it. The operations that take more than one lock take all those in
use in increasing order, from the stripe 0, and the doubling takes the
locks of the new regions in the same order, before the new table is seen.
Hence a thread that holds the lock $i$ and waits for the lock $j$ has
$i < j$, and a cycle of waits would give
$i_1 < i_2 < \cdots < i_k < i_1$, which is impossible. $\square$

Let the keys and the values be trivially copyable, and let
`std::atomic_ref` read and write them without locks (e.g., integers and
pointers; the constant `optimistic` of the class is then true). A lookup is
then a read under the version of the region, i.e., a *seqlock*. It reads
the even version $v$, then the table in use, the tags and the places of the
key with relaxed atomic loads; then it issues an acquire fence and loads
the version again. If the version is still $v$ it returns what it read;
otherwise it tries again, and it takes the lock after 4 tries. A writer makes the version odd by its compare-and-swap and
issues a release fence before its relaxed stores, and makes the version
even again by a store with release.

<span id="lem-24-10"></span>**Lemma 24.10.** *A lookup without locks that
reads the same even version $v$ before and after its reads returns a state
of the region of its key, i.e., what a lookup under the lock would have
returned at the moment of its first read of the version.*

*Proof.* We suppose that the lookup read a value stored by a writer $w$ that
held the lock after the version $v$ was written. This writer stored it
after its release fence, which followed its compare-and-swap of the version from
an even value $v' \ge v$ to $v' + 1$; the lookup read the stored value
before its acquire fence. By the rules of the fences of C++, the fence of
the writer synchronizes with that of the lookup, and the second load of the
version, which follows the acquire fence, reads $v' + 1$ or a later value,
i.e., not $v$ ([Boehm 2012](Z-bibliography.md#ref-boehm2012)). Hence, when
the second load reads $v$, every value read was stored before the release
that made the version $v$, by writers that had all released the lock, and
no writer held it in between; the reads see one state of the region. No
access is a data race, since all of them are atomic. $\square$

```cpp
--8<-- "generated/ConcurrentCuckooTable-peek.cpp"
```

A lookup without locks thus costs no read-modify-write and no store to a
shared line. Furthermore, the tables replaced by a doubling are kept until the table is
destroyed, since a reader may still be on one; with $B_0 < \cdots < B_k$
buckets they total $\sum_{j < k} B_j < B_k$, less than the table in use.
Other keys and values (e.g., `std::string`) are read under the lock, and
the tables replaced are freed at once.

We now turn to the insertion, which puts the new key in a free place of its two buckets if there
is one. Otherwise it searches in breadth, from the two buckets, a path of at
most $L = 5$ moves that ends in a bucket with a free place, and moves the
keys of the path from its end, each into its other bucket, which (24.3)
computes from its tag only.

<span id="lem-24-11"></span>**Lemma 24.11.** *A search in breadth over
buckets of $b$ places that looks at $M$ places finds a path of at most*

$$
L = \left\lceil \log_b \left( \frac{M}{2} - \frac{M}{2 b} + 1 \right)
\right\rceil \tag{24.4}
$$

*moves when one exists among them, against the $M / 2$ moves of a search
in depth; for $M = 2000$ and $b = 4$, $L = 5$.*

*Proof.* We count the places of the two trees of the search, rooted at the
two buckets of the key: they have $2 b^l$ places at the depth $l$, since each place of a bucket names
one other bucket. Hence the places within $L$ moves are
$\sum_{l = 1}^{L} 2 b^l = 2 b ( b^L - 1 ) / ( b - 1 )$, which is at least
$M$ exactly when $b^L \ge M / 2 - M / ( 2 b ) + 1$, whence (24.4). For
$M = 2000$ and $b = 4$ the argument of the logarithm is 751, and
$\log_4 751 \approx 4.78$. A search in depth follows one path, which may be
as long as half the places it looks at ([Li et al.
2014](Z-bibliography.md#ref-li2014)). $\square$

The search stops at $L = 5$, or at 1024 buckets, all in the region, whose
tags form a block of $8 R$ bytes. The table doubles when a region would
pass the load 0.98, or when no path is found. The thread that doubles it
takes all the locks in use, and those of the new regions too before the new
table is seen; then it checks that no other thread has doubled the table in
the meantime, and places each pair again in the new table
("stop-the-world"). With 2 buckets per key and 4
places per bucket, the cuckoo hashing of $n$ random keys succeeds with high
probability while $n / ( 4 B ) < 0.9803$, the threshold of the
4-orientability of the random graphs ([Cain, Sanders and Wormald
2007](Z-bibliography.md#ref-cain2007)), here in each region. A region of $4
R$ places holds a binomial number of the $n$ keys, of mean $\mu = n / P$
and standard deviation $\sqrt{ \mu ( 1 - 1 / P ) }$. This is a relative
deviation of about $1 / \sqrt{ 4 R \alpha }$ at the load $\alpha$ (3.3% for
$R = 256$ and $\alpha = 0.9$), and therefore the fullest region of a large
table reaches 0.98 at a mean load of about 0.88 to 0.92. A restriction
of the choices to a part of the table (there, a page) is also
studied by [Dietzfelbinger, Mitzenmacher and Rink
2011](Z-bibliography.md#ref-dietzfelbinger2011). As in [Lemma 24.6](24-concurrent.md#lem-24-6) (cf. also
[Lemma 24.14](24-concurrent.md#lem-24-14)), the doublings cost $O( 1 )$ amortized per insertion, by the argument of [Lemma 24.6](24-concurrent.md#lem-24-6). A doubling that
would leave a table of at least 256 places less than 1/16 full throws
`std::length_error`, which happens only when more than 8 keys share both
buckets at every size (e.g., by equal hashes), and leaves the table
unchanged.

A lookup touches two buckets. It fetches ahead, at once, the tags of both
and the line of the places of the first, and the misses overlap (the 4
places of a bucket share a line of cache when their size is a power of 2 up
to 64 bytes). Those of the second bucket, where fewer keys usually are, are read only when a tag matches there. The tags and the places of a
table of 2 MB or more lie on transparent huge pages, and the random
accesses then miss the TLB once per 2 MB instead of once per 4 KB ([Navarro et
al. 2002](Z-bibliography.md#ref-navarro2002)). With uniform keys two
operations contend for a lock with probability $1 / P$. Each region counts
its pairs under its lock, and `size()` is the sum of $P$ counters read
without locks, exact when no operation runs and otherwise an estimate. The
operations of a single key are linearizable at the moment they hold the
lock of its region, or at the second read of the version for a lookup
without locks. Also, `forEach()` visits the pairs of one moment, since it
holds all the locks.

## 24.8 Linear probing that grows {#sec-24-8}

`ConcurrentFlatTable` gives up what `ConcurrentCuckooTable` needs its
versions for: a word of its vector, once written, never changes nor moves
within its vector, since there are no removals and the keys are never
displaced. It is open addressing with linear probing on a vector of $2^l$
atomic words of 64 bits, i.e., the *folklore* table that [Maier, Sanders
and Dementiev 2019](Z-bibliography.md#ref-maier2019) found the "fastest" to
read among the hash tables shared by threads, made to grow while the
threads work and to keep its pairs where they are. It is meant as the
CLOSED set of a parallel search. Indeed, `findOrInsert( k , h , v )` takes a
key with its hash of 64 bits, computed once by the caller, and either finds
the key or inserts it with the value $v$; in both cases it returns the pair
that the table holds. The threads that reach the same state at once thus
agree on one pair, and only one of them is told that it inserted it.

A word is 0 when its place is free; it is written once, by a
compare-and-swap from 0, and it is never changed afterwards but by
`update()`. We distinguish the forms in which the pairs are kept, chosen by
their types and by the processor when compiling. (i) When the key and the value are
trivially copyable and fit in 8 bytes together (e.g., a key of 32 bits with
a value of 32 bits, or a key of 64 bits with no value, i.e., a set), the
word is the pair itself (the constant `packed` of the class). Pairs whose
key is made of zero bytes, whose word could be 0, are kept apart in
one atomic word. (ii) When they do not fit together but each fits in 8
bytes, and the processor has a compare-and-swap of 16 bytes without locks
(`cmpxchg16b` on x86-64, the pairs of registers of AArch64), a place is two
words, the key and the value, written together by that instruction
(`inlined`). (iii) Otherwise the word holds the index of a node, where the
pair lies with its hash, and 24 bits of the hash; a lookup follows the
index only when those bits match, which happens once in $2^{24}$ times for
another key. Nodes never move, and the pair returned is a reference to
the node, valid as long as the table. The nodes are cut from segments of
$2^{12} , 2^{13} , \ldots$ nodes, of which each thread reserves 64 at a
time; an insertion thus takes no lock, and calls the allocator once per 64
nodes at most.

The place of a key of hash $h$ is $\lfloor h / 2^{64 - l} \rfloor$, and the
key is looked for from there on, cyclically, up to its word or to a word 0.
When the pairs lie in the words, each place has besides a *tag* of a byte,
0 or 7 bits of the hash of its key with the high bit set, as the metadata
of the tables of Swiss and F14 kind. Tags are kept apart from the words, 8 to an atomic word. A lookup reads the tags from the place on, 8 at
a time, compares them all with the tag of the key in one step, reads only the words of
the places whose tag matches, and stops at the first tag 0. An absent key
thus costs one line of the tags, ($1/8$ of the words or less), which the
cache usually keeps. A tag is written, with release, after the word it
tags, and always with the same value, by the insertion that wrote the word
or by any insertion that passes the place while it looks for a place;
the insertion takes effect when the tag of its word is written
([Figure 24.9](24-concurrent.md#fig-24-9)). The pairs in nodes have no
tags, since a tag would add a third miss to the two of a key found in its
node.

<span id="fig-24-9"></span>
![Figure 24.9. The tags (upper row, 0 for a place free or not yet tagged) and the words (lower row) of a `ConcurrentFlatTable` of 12 places, before and after the insertion of a key $k$ whose place is 5: the insertion passes the places 5 to 7, which hold other keys, writes $k$ into the free place 8 by a compare-and-swap, and then its tag.](../figures/concurrent-flat.svg)

<span id="alg-24-8"></span>**Algorithm 24.8.** The insertion and the
lookup of `ConcurrentFlatTable` (the folklore table of [Maier et al.
2019](Z-bibliography.md#ref-maier2019), with the growth below); $W$ is the
vector in use, $c_t$ the counter of the thread and $g$ the flag of the
growth.

--8<-- "generated/ConcurrentFlatTable-findOrInsert.pseudo.md"

--8<-- "generated/ConcurrentFlatTable-find.pseudo.md"

The lookups take no lock, write no word, and check no version, since a word
or a tag, once written, stays. We now show why they are
correct while insertions run.

<span id="lem-24-12"></span>**Lemma 24.12.** *Let the words and the tags of
a vector be written only from 0, once each, and only while the vector is in
use, and let an insertion write the tags of the places it passes before it
writes its own. Let a lookup of $k$ in the vector $T$ read the tag 0 at the
place $j$, after tags at the places from that of $k$ to $j - 1$ whose words
hold other keys, and then find $T$ still in use. Then no insertion of $k$
had taken effect when the lookup read the tag of $j$.*

*Proof.* We follow an insertion of $k$, which writes the first word 0 of the sequence of
places of $k$, and before its own tag the tags of all the places it passed.
We suppose that it had taken effect, i.e., written its tag, before the lookup
read the tag of $j$. The place $j$ comes before its word, or is its word,
and it would have had its tag then, since a tag is written once and stays;
but the lookup read 0 there. $\square$

A lookup that finds $k$ returns a pair that stays, and it is linearized at
the read of its word (of its tag, when the places have tags). One that
finds the vector replaced starts over on the new one, which happens once
per growth. Also, the lookups read with acquire. Hence the word of a tag read is
read whole, and so is the pair of a node, built before the compare-and-swap
with release that published its index; $p$ threads that only read
therefore scale as $p$ independent sequential tables (cf. [Table 24.1](24-concurrent.md#tab-24-1)). Note that an
insertion that finds $k$ in a word whose tag is still 0 writes the tag
before it answers, and its answer thus comes after the insertion of $k$ has
taken effect. For the pairs in nodes, which have no tags, the lemma holds
with the words in place of the tags, an insertion taking effect at its
compare-and-swap.

We let the table grow when its pairs pass half of its places. Insertions (and
`update()`) exclude a growth, and the growth excludes them, by a protocol
of Dekker over 64 counters on separate lines, as in the growth of [Maier et
al. 2019](Z-bibliography.md#ref-maier2019) that synchronizes the threads. A
thread adds 1 to its counter and then reads the flag $g$ of the growth,
while the thread that grows the table raises $g$ and then reads the
counters:

```cpp
--8<-- "generated/ConcurrentFlatTable-enter.cpp"
```

<span id="lem-24-13"></span>**Lemma 24.13.** *Let a writer raise its
counter and then read the flag $g$, and the thread that grows the table
raise $g$ and then read the counters, all these accesses sequentially
consistent; and let a writer that reads $g$ raised lower its counter and
retry after the growth. Then no word of a vector is written once its copy
has begun.*

*Proof.* The four accesses lie in the single order $S$. If the read of $g$
by the writer follows the raise of $g$, the writer backs off. Otherwise the
read of $g$ precedes the raise, and hence the raise of the counter of the
writer precedes the read of the counters by the grower. Then the grower
sees the counter and waits for it to drop, i.e., for the writer to leave the
vector (the mutual exclusion of Dekker, which [Dijkstra
1965](Z-bibliography.md#ref-dijkstra1965) published). Thus the copy starts
only when no writer is inside the old vector, and no writer enters it until
the growth ends. $\square$

The grower allocates a vector twice as large. Meanwhile, the threads that
come to insert copy blocks of 4096 words of the old vector, claimed by a
counter, instead of waiting. Each word is placed at the first free place of
its sequence in the new vector, which no other thread reads yet, and the
new vector is published, with release, once all the blocks are copied.
Meanwhile the lookups never wait: they read the old vector, which no longer
changes, until the new one is in use. The counters of the threads also
count the pairs, in their upper 48 bits, and an insertion counts its pair
in the same read-modify-write by which it leaves. Their sum, which reads 64
lines, is taken once per 1024 insertions of a counter (more often in a
vector of less than $2^{20}$ places). An insertion that reads more than 128
words also grows the table, if it is at least one eighth full. A table less
full than that, which typically only keys of equal hash make so "crowded", is read on
to its first free place instead.

<span id="lem-24-14"></span>**Lemma 24.14.** *With the load $\alpha$ of the
table at most $1/2$ and random hashes, a lookup reads on average*

$$
\frac{1}{2} \left( 1 + \frac{1}{1 - \alpha} \right) \le 1.5
\quad \text{and} \quad
\frac{1}{2} \left( 1 + \frac{1}{( 1 - \alpha )^2} \right) \le 2.5
\tag{24.5}
$$

*places when it succeeds and when it fails, respectively, and the growths
cost $O( 1 )$ amortized time per insertion.*

*Proof.* We take the two averages of linear probing ([Knuth 1998,
§6.4](Z-bibliography.md#ref-knuth1998)), increasing in $\alpha$, taken at
$\alpha = 1/2$. A growth copies each word of the old vector once, and the
sizes double; hence the copies are at most twice the insertions, as in
[Lemma 24.6](24-concurrent.md#lem-24-6). $\square$

Most of the places of (24.5) lie on one line of 8 words, which the lookup
fetches while it reads the tags, and a key that is there typically costs
about one miss. An insertion is blocking only during a growth, and the
lookups never are; a lookup is thus wait-free when the table does not grow.
Where the pairs are `inlined`, the copy of a growth moves the two words of
a place at once, and a lookup reads the key with acquire and then the value
from the same line. [Lemma 24.12](24-concurrent.md#lem-24-12) then holds unchanged for the key, since
`update()` changes the value by the same instruction and never the key; the
lookup reads a value written with that key, or a later one of `update()`.
We remark that `clear()`, `forEach()` and `consistent()` may not run with
the other operations. Vectors replaced are kept until the table is
destroyed, and with sizes that double they occupy less than the vector in
use. For a `packed` set (with no value), `findOrInsertKey()` returns the key
that the table holds, which is equal to $k$ by `Eq`, though not necessarily
by its bytes. In a set of pointers to objects compared as the objects, the
threads that insert at once pointers to equal objects therefore all get the
pointer of the one that inserted it; this is the intrusive set of the nodes
of a parallel search.

## 24.9 Relaxed priority queues {#sec-24-9}

A priority queue shared by $p$ threads under one lock serializes them. A
*relaxed* priority queue returns instead a pair that need not be the least:
the *rank* of a popped pair is 1 plus the number of the pairs present with
a smaller key, and its *rank error* the rank less 1, which is 0 for an exact
queue. A search that takes its states from a relaxed OPEN expands a few
states that an exact one would not, and in exchange its threads do not wait
for one another. `MultiQueue` is the MultiQueue of [Rihani, Sanders and
Dementiev 2015](Z-bibliography.md#ref-rihani2015), with the stickiness and
the buffers of [Williams, Sanders and Dementiev
2021](Z-bibliography.md#ref-williams2021), on pairs $( k , x )$ of a key
(an integer or a real, the least first by `less`) and an item.

The $n$ queues are sequential priority queues, each with a separate lock
and with the key of its minimum, $\mathit{top}_j$, kept apart in an atomic
word that the other threads read without the lock. A push puts the pair in
a random queue whose lock it takes at the first try, and draws another one
when the lock is held. A pop reads the minima of two distinct random
queues, locks the queue with the smaller one if it is free (draws two other
queues otherwise), and takes its minimum
([Figure 24.10](24-concurrent.md#fig-24-10)). Let there be $n = q p$
queues. At most $p - 1$ of them are held by other threads, and a queue
drawn at random is held with probability less than $1 / q$; hence a lock is
taken after at most $q / ( q - 1 )$ draws on average (2 for the default $q = 2$),
and no thread waits for a lock. With $n = 1$ the queue is exact, a
priority queue under one lock, which is typically used for testing and for
comparison.

<span id="fig-24-10"></span>
![Figure 24.10. A `MultiQueue` of four queues, each with its minimum $\mathit{top}$, read without locks, and its deletion buffer $D$ (the least pair on the right); a pop that draws the queues 1 and 3 takes the key 4 from the better one, while the keys 2 and 3 are in the other queues: its rank error is 2.](../figures/concurrent-multiqueue.svg)

<span id="alg-24-9"></span>**Algorithm 24.9.** The push and the pop of
`MultiQueue::Handle` ([Rihani et al. 2015](Z-bibliography.md#ref-rihani2015),
with the stickiness and the buffers of [Williams et al.
2021](Z-bibliography.md#ref-williams2021)); $D_j$ and $I_j$ are the buffers
of the queue $j$, $H_j$ its heap, and $b = 16$.

--8<-- "generated/MultiQueue-push.pseudo.md"

--8<-- "generated/MultiQueue-tryPop.pseudo.md"

We measure the quality of the pops by the expected rank error, for which
the sequential model of the MultiQueue has an exact analysis. In this model
the pairs are spread over the $n$ queues uniformly at random, and then
popped one at a time, each from the queue whose least key is the $j$-th of
the $n$ with a probability $\sigma_j$; we write $\sigma_{\le i} = \sigma_1
+ \cdots + \sigma_i$.

<span id="thm-24-15"></span>**Theorem 24.15.** *([Walzer and Williams
2025](Z-bibliography.md#ref-walzer2025).) If $\sigma_{\le i} > i / n$ for
$i < n$, the rank error of a pop converges in distribution as the pops go
on, and the mean of its limit is*

$$
E[ R ] = \sum_{i = 1}^{n - 1} \frac{\sigma_{\le i} ( 1 - \sigma_{\le i} )}
{\sigma_{\le i} - i / n} ; \tag{24.6}
$$

*for the better of two queues drawn with replacement,
$\sigma_{\le i} = 1 - ( 1 - i / n )^2$ and*

$$
E[ R ] = \sum_{i = 1}^{n - 1} \left( 2 - \frac{i}{n} \right)
\left( 1 - \frac{i}{n} \right) = \frac{5}{6} n - 1 + \frac{1}{6 n} ,
\tag{24.7}
$$

*and the maximum over $\mathrm{poly}( n )$ pops is $O( n \log n )$ with high
probability.*

We do not reproduce the proof, whose idea is about the queues ordered by
their least keys. Here the $i$ best queues hold a share $i / n$ of the pairs
that come next in the order of the keys, since the pairs were spread at
random, and a pop takes from them with the probability $\sigma_{\le i}$.
When $\sigma_{\le i} > i / n$ they are emptied faster than their share, and
the number of pairs that separate the $i$-th least key from the $( i + 1
)$-th is a geometric variable whose mean depends only on $\sigma_{\le i}$
and $i / n$. Walzer and Williams prove that the ranks of
the least keys of the queues converge to sums of such independent
variables, whence (24.6). With one draw, $\sigma_{\le i} = i / n$, the best
queues are emptied only as fast as they are filled, and the rank error
grows without bound. Two draws that fall on the same queue are such a draw.
Hence `MultiQueue` draws two distinct queues, for which
$\sigma_{\le i} = 1 - ( n - i ) ( n - i - 1 ) / ( n ( n - 1 ) )$ and
$\sigma_{\le i} - i / n = i ( n - i ) / ( n ( n - 1 ) )$, so that (24.6)
becomes

$$
E[ R ] = \sum_{i = 1}^{n - 1} \sigma_{\le i} \, \frac{n - i - 1}{i} ,
\tag{24.8}
$$

somewhat less than (24.7) ([Table 24.2](24-concurrent.md#tab-24-2)).

<span id="tab-24-2"></span>**Table 24.2.** The expected rank error of the
limit of the sequential model, for two queues drawn with replacement, by
(24.7), and for two distinct queues, by (24.8).

| $n$ | with replacement (24.7) | distinct (24.8) |
|---|---|---|
| 8 | 5.69 | 4.63 |
| 32 | 25.67 | 24.53 |

The earlier bounds of [Alistarh et al. 2017](Z-bibliography.md#ref-alistarh2017),
$O( n )$ for the mean and $O( n \log n )$ for the maximum, hold for pushes
and pops mixed as well, but for a comparison and a pop done atomically. As
[Williams et al. 2021](Z-bibliography.md#ref-williams2021) do, `MultiQueue`
reads the least keys of the two queues without their locks, possibly stale.
In a run of many threads its rank errors are therefore measured rather than
bounded, and the test `engineered_queues.multi-queue-rank` compares
the measured mean with (24.8) on one thread.

We adopt two devices of [Williams et al.
2021](Z-bibliography.md#ref-williams2021), which save most of the misses of the cache. A thread keeps the two queues it drew
for $s$ operations (the *stickiness*), and their lines stay in its cache;
it draws two others after $s$ operations or when a lock is held. A push
goes to one of the two at random, a pop to the better one. Also, each queue
is an 8-ary heap behind two buffers of 16 pairs. The deletion buffer $D$ is
sorted by decreasing keys and holds the least pairs of the queue, and a pop
takes its pair from its end without moving the others. The insertion buffer
$I$ is unsorted; a push fills it, and it goes into the heap only when it is
full or when $D$ runs out.

<span id="lem-24-16"></span>**Lemma 24.16.** *In each queue, let $D$ be
empty only if the queue is, and let no pair of $I$ or of the heap $H$ be
less than a pair of $D$. Then the least pair of $D$ is the least of the
queue, and the push and the pop of [Algorithm 24.9](24-concurrent.md#alg-24-9) keep both properties.*

*Proof.* We note that the first statement is the second property. A push of a pair not
greater than the greatest of $D$ puts it into $D$. Then the greatest of a
full $D$ moves into $I$ (with $I$ into $H$ when $I$ is full), and it is not
less than the rest of $D$. A greater pair goes into $I$, or into $D$ when
$I$ and $H$ are empty, and the pairs of a full $I$ go into $H$; neither
move changes the two properties. A pop takes the least of $D$ and, if $D$ becomes empty, moves
$I$ into $H$ and the least $\min( 16 , |H| )$ pairs of $H$ into $D$, which
are not greater than the rest of $H$; $D$ is then empty only if $H$
is. $\square$

Each queue is therefore exact, the relaxation is only in the choice of the
queue, and [Theorem 24.15](24-concurrent.md#thm-24-15) applies unchanged. Its heap is touched by batches
of 16 pairs, at a cost of $O( \log m )$ amortized per operation on a queue
of $m$ pairs, and the lines of $D$ and $I$ usually stay in the cache of the
thread.
The stickiness has no theory yet: the measures of [Williams et al.
2021](Z-bibliography.md#ref-williams2021) show rank errors that grow about
linearly in $s$, which is why $s$ is a parameter, 1 for the bounds above.
Furthermore, `pushAll()` puts a range of pairs into one queue under one
lock. A thread that makes several pairs at once (e.g., the successors of a
node) thus takes one lock instead of one per pair, at the price of a
relaxation like that of a stickiness of the number of the pairs. A pop that
finds both queues empty scans all the queues, each under its lock. When no
push runs at the same time, a scan that finds no pair proves the structure
empty, which a parallel search needs in order to stop. Its locks are
test-and-test-and-set on a byte, taken by an exchange after a plain read;
a pop holds one lock, a push one, and the scan one at a time, so that no
deadlock is possible. Each operation of a single queue is linearizable at
its lock, while the structure as a whole is relaxed, i.e., it is not a
linearizable priority queue but for $n = 1$. The greatest key under `less`
(the infinity of a real) stands for an empty queue in the atomic minima,
and a pair with that key is popped only by the scan.

## 24.10 Waiting until a condition holds {#sec-24-10}

A thread that waits for an item could look at the structure again and
again, which wastes a core, or sleep for fixed lengths, which delays the
item. The *event count* of [Reed and Kanodia
1979](Z-bibliography.md#ref-reed1979) lets it sleep until another thread
signals that the condition may have become true, with no lock and with
nearly no cost for the signal when nobody sleeps. `EventCount` holds an
epoch $e$ and the number $w$ of the waiters, the two halves of one word of
64 bits (on a separate line). It also holds a copy $E$ of the epoch of 32
bits, the size of a futex, on which the waiters sleep. A waiter announces
itself and reads the epoch in one read-modify-write (`prepareWait()`, which
adds 1 to $w$), and looks at the condition once more. If the condition is
still false, it sleeps while $E = e$ (`commitWait( e )`, by
`std::atomic::wait`); otherwise it withdraws (`cancelWait( e )`). A
notifier makes the condition true, and then `notify()` reads the word after
a fence. Only if there are waiters does it advance the epoch and set $w$ to
0, in one compare-and-swap, and wake them ([Figure
24.11](24-concurrent.md#fig-24-11)). Later notifications, while
the woken threads are still on their way, find no thread to wake and cost
no system call.

<span id="fig-24-11"></span>
![Figure 24.11. The word of the epoch $e$ and of the waiters $w$ of an `EventCount`, and the copy $E$ of the epoch, after `prepareWait()` of a waiter, which then sleeps while $E = 5$, and after `notify()`, which advances the epoch, withdraws the waiter and wakes it.](../figures/concurrent-eventcount.svg)

<span id="alg-24-10"></span>**Algorithm 24.10.** The operations of
`EventCount` (after [Reed and Kanodia 1979](Z-bibliography.md#ref-reed1979));
$( e , w )$ is the word of the epoch and of the waiters, and $E$ the copy
of the epoch.

--8<-- "generated/EventCount-prepareWait.pseudo.md"

--8<-- "generated/EventCount-cancelWait.pseudo.md"

--8<-- "generated/EventCount-commitWait.pseudo.md"

--8<-- "generated/EventCount-notify.pseudo.md"

--8<-- "generated/EventCount-await.pseudo.md"

The fences are asymmetric where the system has them, as `membarrier(2)` on
Linux ([Ladan-Mozes, Lee and Vyukov
2011](Z-bibliography.md#ref-ladanmozes2011)). A waiter, which is about to
sleep anyway, runs the heavy fence, a system call that makes each running
thread of the process run a full barrier; the notifier runs the light one,
which only stops the compiler. When no thread waits, a notification thus
costs the read of a line that stays shared. A full fence, instead, would
wait at each item for the stores of the item to reach the caches of the
other threads. Elsewhere both fences are full ones.

<span id="lem-24-17"></span>**Lemma 24.17.** *Let the read-modify-write of
the waiter be followed by a full fence, and the write that makes the
condition true be followed by a full fence before the notifier reads the
waiters. Then a waiter does not sleep through a notification of a
condition that is true.*

*Proof.* We place the two fences in $S$ (a pair of asymmetric fences orders as a
pair of full ones). If that of the notifier comes first, the waiter, which
looks at the condition after its fence, sees it true and does not
sleep. If that of the waiter comes first, the notifier reads its
announcement, $w \ge 1$, and advances the epoch by a release on the same
word; then either the waiter has not yet compared $E$ with $e$, and finds
it changed, or it sleeps on $E = e$ and is woken by the notification that
follows the change. A "spurious" wake-up only makes the waiter look
again. $\square$

```cpp
--8<-- "generated/EventCount-notify.cpp"
```

We need one more detail to keep the lemma true with several notifiers. Two notifiers
that advanced the epoch of the word to $e - 1$ and to $e$ may reach $E$ in
the other order. A store of $e - 1$ after $e$ would then put back the epoch
of a waiter that the second notifier withdrew, and that waiter would sleep
with the condition true and no thread to wake it. Hence the copy is only
raised, by a compare-and-swap with release that compares the epochs
modulo $2^{32}$: the epoch $e$ is ahead of the copy $E$ when $0 < ( e - E )
\bmod 2^{32} < 2^{31}$. Under the sanitizer of threads,
which does not model the fences, the notifier reads the word by a
read-modify-write instead, which reads the last value in the order of its
modifications.

Finally, `await( p )` packs the loop of [Algorithm 24.10](24-concurrent.md#alg-24-10). It returns when
`p()` is true, $p$ being called by the waiting thread only (which may thus
take the item in $p$). It first looks again and again for 20 $\mu$s (a
parameter), with a pause of the processor between the looks. An item that
comes within that time costs no sleep and no wake-up, each of which takes
about the time of a system call and of a switch of threads. We use it in
the queues of [§24.3](24-concurrent.md#sec-24-3) and
[§24.4](24-concurrent.md#sec-24-4) for their operations that sleep (`waitPop()`, `waitTake()`).

Spinning is not "free" either, since the loads and the failed
compare-and-swaps of the threads that spin on one line keep moving it
between their caches. `Backoff` is the exponential backoff of [Anderson
1990](Z-bibliography.md#ref-anderson1990) (see also [Mellor-Crummey and
Scott 1991](Z-bibliography.md#ref-mellorcrummey1991)). Its call $k$ from
the last `reset()` runs $2^k$ pauses of the processor (`pause` on x86,
`yield` on AArch64) while $k \le S$, and from then on it yields the
processor to the system. Note that `spinning()` is true when the next call
still spins; a waiter can then move on to sleep, e.g., on an `EventCount`,
once it yields. With $S = 6$ (the default) the spinning lasts $2^7 - 1 = 127$
pauses, a few microseconds.

--8<-- "generated/Backoff-pause.pseudo.md"

<span id="lem-24-18"></span>**Lemma 24.18.** *A thread that looks at a
condition before each call of `pause()`, from a `reset()`, makes its look
$k + 1$ after $2^{k + 1} - 1$ pauses, for $k \le S$; hence a condition
that becomes true after $t < 2^{S + 1} - 1$ pauses is seen after fewer than
$2 t + 1$ pauses, with $O( \log t )$ looks.*

*Proof.* The calls $0 , \ldots , k$ run
$1 + 2 + \cdots + 2^k = 2^{k + 1} - 1$ pauses. If the condition becomes
true after $t$ pauses, and the last look that saw it false was the look
$k + 1$, after $T = 2^{k + 1} - 1 < t$ pauses, the next look comes after
$2^{k + 2} - 1 = 2 T + 1 < 2 t + 1$ pauses; the looks are $k + 2$, i.e.,
$O( \log t )$. $\square$

The looks of $p$ threads at one line thus thin out geometrically, while the
delay added to a wait is less than the wait itself (plus one pause), and a
wait longer than $2^{S + 1}$ pauses gives the processor back to the system.
The test `engineered_search.backoff-threads` checks the steps and four
threads that wait with it for a flag.

## 24.11 Priority queues that are stolen {#sec-24-11}

A best-first search on $p$ workers (e.g., a branch-and-bound by bounds, or
an A* search) may keep its OPEN in one queue shared by all, exact under a
lock or relaxed as the MultiQueue of [§24.9](24-concurrent.md#sec-24-9).
Alternatively, it may keep a queue per worker, from which the idle workers
steal. The second choice keeps the order of each worker exact. A search
needs this when it counts the nodes of each worker against a limit, or when
it wants the same run from the same steals. It loses the global order,
however, and a worker may expand a node worse than the best of another
worker; the steals (hopefully) keep this loss "small" when they move "good"
nodes.
`StealablePriorityQueue` is such a queue, and `StealingFrontier` holds $p$
of them with the detection of the end of the search. Its design has to
settle how many pairs a thief takes, which ones, and at what price for the
owner.

As to how many, a thief takes half of the pairs, as in the queues of
[Hendler and Shavit 2002](Z-bibliography.md#ref-hendler2002) and in the
priority queues stolen by [Wimmer et al. 2014](Z-bibliography.md#ref-wimmer2014).
A steal of one pair usually sends the thief back at once, while a steal of half
spreads the pairs made by one worker over $p$ workers in about $\log_2 p$
rounds of steals. The Stealing Multi-Queue of [Postnikova et al.
2022](Z-bibliography.md#ref-postnikova2022) steals instead a batch of a few
of the best tasks. Its owner sets them apart in a buffer and skips them in
the meantime, and its order is thus relaxed by that buffer.

As to which ones, the "better" half sends the victim, which made those pairs,
to "worse" ones, and the next steal brings the good pairs back the other way.
In turn, the worse half gives the thief the pairs that a best-first search expands
last, and that a bound improved in the meantime is likely to prune. An
"interleaved" half, instead, gives each worker a share of the best pairs as
of the others. This is the reason behind several balancing rules of the
parallel branch-and-bound. That of [Tschöke, Lüling and Monien
1995](Z-bibliography.md#ref-tschoke1995) is best when each of the $p$
processors holds one of the $p$ subproblems of least bound, and it balances
the least bounds of the heaps besides their sizes. Also, [Karp and Zhang
1993](Z-bibliography.md#ref-karp1993) send each child to a random
processor, and the randomized priority queue of [Sanders
1998](Z-bibliography.md#ref-sanders1998) spreads its items at random over
the processors.

The queue therefore deals its pairs to two 4-ary heaps $H_0$ and $H_1$ of
its owner, which no other thread reads. A push goes into the smaller heap,
and into each in turn when they are as large. Hence the pairs pushed one
after the other (e.g., the children of a node) go to both heaps, and either
heap holds, for each range of keys, about half of the pairs in it. A pop
takes the better of the two least pairs. A thief takes the heap whose least
pair is not the least of the queue, and the owner thus keeps its best pair
and its next pop ([Figure 24.12](24-concurrent.md#fig-24-12)).

<span id="fig-24-12"></span>
![Figure 24.12. A `StealablePriorityQueue` whose owner pushed the keys 1 to 8, dealt to its two heaps (in the order of a 4-ary heap); `serve()` moves into the offer the heap whose least key 2 is not the least of the queue, and a thief claims it by an exchange and deals its pairs to two heaps of its own.](../figures/concurrent-spq.svg)

As to the price, a queue whose pairs a thief may take at any time makes its
owner race the thieves at each pop. By the laws of order of [Attiya et al.
2011](Z-bibliography.md#ref-attiya2011), a linearizable pop of such an
object needs a read-modify-write or a fence, as `take()` of the deque of
[§24.5](24-concurrent.md#sec-24-5) does. Here, instead, a thief raises the
flag `hungry` of the queue, which the owner reads with no order at its next
push or pop (or at `serve()`). Its owner answers by moving a heap into the
*offer*, an atomic pointer published with release, which a thief claims by
an exchange with null; these are the receiver-initiated steals of the
private deques of [Acar, Charguéraud and Rainey
2013](Z-bibliography.md#ref-acar2013). Thus the common case of the owner
costs no read-modify-write and no fence. A thief, in exchange, waits
(hopefully briefly) for the next operation of the owner, as long as a `Backoff` spins, before it tries
another victim.

<span id="alg-24-11"></span>**Algorithm 24.11.** The operations of
`StealablePriorityQueue`; $O$ is the offer, $k_O$ its least key and
$\mathit{offered}$ the knowledge of the owner that it made it.

--8<-- "generated/StealablePriorityQueue-push.pseudo.md"

--8<-- "generated/StealablePriorityQueue-pop.pseudo.md"

--8<-- "generated/StealablePriorityQueue-serve.pseudo.md"

--8<-- "generated/StealablePriorityQueue-steal.pseudo.md"

Until a thief claims the offer, its pairs still belong to the owner. A pop
whose least local key is greater than $k_O$ (or whose heaps are empty)
takes the offer back by the same exchange, and whoever of the two threads
exchanges first gets all of it.

<span id="lem-24-19"></span>**Lemma 24.19.** *Let the owner take the offer
back, by an exchange with null, before a pop whose two heaps are empty or
whose least key is greater than $k_O$. Then each pop of the owner returns a
pair of least key among those that the owner holds at the moment of the
pop, the pairs of an offer not yet claimed included.*

*Proof.* We first note that the heaps of the owner change only by its
operations. The offer,
once published, does not change until one exchange takes it, by a thief or
by the owner and never by both, since the exchanges on one word are totally
ordered and only the first one finds the pointer. If the least local key is
not greater than $k_O$, the local pair is a least pair of all, whether a
thief claims the offer before the pop or not. Otherwise the owner
exchanges; either it gets the offer back and pops from the pairs joined, or
a thief had claimed it before, and the pairs were no longer of the owner
when it popped. $\square$

Each pop is thus exact at the moment of its exchange, or of its last change
of the heaps. Otherwise, the only pairs that leave the owner are those of an
offer, a choice of the owner that keeps its least pair. For $m$ pairs we
have the following costs. A push and a pop cost $O( \log m )$ on a 4-ary heap of about $m / 2$ pairs,
half as deep as a binary heap, plus a load of the flag. Answering a thief
costs $O( 1 )$, i.e., the store of a pointer. An exception arises when the
two heaps differ by more than a factor of 4 (the heap left after a
steal, before the pushes refill the other); the owner then deals all its
pairs again to two heaps, in $O( m )$. A steal costs the thief two
exchanges and the deal of the $s$ pairs taken to its two heaps, in
$O( s )$ by the construction of Floyd. A pair is copied only by a deal, and
the pairs of an offer move with their heap, by its pointer.

`StealingFrontier` holds the queues of $p$ workers, one per call $f( i )$ of
the `run( p , f )` of an executor (see chapter 25). Its `pop( i )` returns
the least pair of the queue of the worker $i$. When that queue is empty, it
returns the least pair of what it steals from victims drawn at random; it
returns no pair only when no worker holds one, or after `stop()`.

<span id="alg-24-12"></span>**Algorithm 24.12.** The pop of a worker of
`StealingFrontier`; $C$ is the counter of the workers at work and of the
thieves trying, $p$ at the start.

--8<-- "generated/StealablePriorityQueue-StealingFrontier-pop.pseudo.md"

<span id="lem-24-20"></span>**Lemma 24.20.** *In `StealingFrontier`, let
the counter $C$ start at $p$, and let a worker subtract 1 when its pop finds
its heaps and its offer empty. Let a thief add 1 before the exchange that
claims an offer and subtract 1 if the exchange fails, and let only a worker
that holds a pair (or the program before the run) push. Then no pair exists
when $C = 0$, and none exists afterwards.*

*Proof.* We observe that a pair lies in the heaps or in the offer of a worker that has not
left $C$, since a worker leaves only when it holds no pair; or it lies in
the offer claimed by a thief, which entered $C$ before its exchange. The
victim that finds its offer claimed has read the null of the exchange of
the thief, and the two exchanges synchronize (a release read by an
acquire). Hence the entry of the thief happens before the exit of the
victim, and $C$, whose modifications are totally ordered, cannot reach 0 in
between; hence, a pair in transit is always counted. When $C = 0$ no
pair exists, and no pair can be created, since only a worker that holds one
pushes. Thieves that still try enter and leave $C$ with no pair to
find, and see $C = 0$ again. $\square$

When we give a type of bounds, each pair carries a bound besides its key, e.g., the
key of selection of a branch-and-bound by depth first or by best estimate,
and the bound by which it prunes. Then `minBound()` gives the least bound
$\beta$ of the pairs of the owner, and when $\beta$ cannot improve the
incumbent beyond the tolerance, `drain()` drops the entire queue. Each heap
keeps its least bound as long as no pop takes a pair of that bound (a push
lowers it, if its bound is less), and finds it again by a scan of its pairs
when one does. With bounds independent of the keys, a pop takes the least
of $m$ bounds with probability about $1 / m$, and the scans cost $O( 1 )$
expected per pop. With bounds that go with the keys, as in a search by best
first, each pop takes it, and each scan costs $\Theta( m )$. Hence, when
the pairs scanned exceed 16 times the pops and the pairs held, plus
$2^{16}$ for the start, on a heap of 64 pairs at least, the heap keeps
instead, in a vector apart, the least bound $\mu( i )$ of the pairs of the
subtree of each node $i$:

$$
\mu( i ) = \min\{ b( i ) , \mu( 4 i + 1 ) , \ldots , \mu( 4 i + 4 ) \} ,
\tag{24.9}
$$

where $b( i )$ is the bound of the pair of $i$, and the children that do
not exist are left out; the least bound of the heap is then $\mu( 0 )$. A
push moves the hole of a new leaf up to the place $i$ of the new pair. The
nodes passed by the hole took the pair of their parent, and get (24.9)
again from their children, from the leaf up. The subtree of $i$, and of
each node above it, gained the new bound $b$ only, and $\mu$ is lowered to
$b$ from $i$ up as long as $b$ is less, since $\mu$ does not increase
towards the root. A pop moves the last pair $z$ down from the root along a
path $P$. A node off $P$ on the path of the old last leaf lost $z$ only; it
keeps its $\mu$ if $\mu$ is less than the bound of $z$, and so do all those
above it off $P$. The rest of that path, and then $P$ from its end up, get
(24.9) again. Each operation thus costs $O( \log m )$, and a read of $\mu(
0 )$ costs $O( 1 )$.

We remark that two alternatives cost more on both kinds of bounds. The first keeps the
bounds of the pushes and those of the pops in two multisets, the least
bound being the least of the first once their equal tops are taken out; it
adds a push into one of them per operation, on heaps that grow with the
pops until they are made again. The second keeps a heap by the bounds,
indexed by the places of the pairs, and it adds a second sift per operation
and an update of the index at each move of a pair.

--8<-- "generated/StealablePriorityQueue-minBound.pseudo.md"

<span id="lem-24-21"></span>**Lemma 24.21.** *Let the owner keep, when it
makes an offer, the least bound $b_O$ of the heap that it moves into the
offer. Let `minBound()` return the least of the least bounds of its heaps
that are not empty and, if the offer that it made is still there at an
acquire load of the pointer, of $b_O$. Then the value returned is the least
bound of the pairs that the owner holds at the moment of that load.*

*Proof.* The heaps of the owner change only by its operations, and the
least bound of each is exact. In fact, the one known stays the least while
no pop takes a pair of that bound, a push lowers it when its bound is less,
a scan computes it, and $\mu( 0 )$ is it by (24.9). The pairs of an offer
are fixed from its publication until an exchange takes it, and $b_O$ is
their least bound. At the load, the offer is either still there, and then
its pairs are of the owner (cf. [Lemma 24.19](24-concurrent.md#lem-24-19): a thief that claims it later
takes pairs that the owner held at the load), or claimed, and then its
pairs belong to the thief from its exchange on. $\square$

The pairs stolen keep their keys and their bounds: the thief adopts the
heap as it is, deals its pairs to two heaps, and rebuilds (24.9) on them.
Note that `drain()` takes the offer back first, if no thief has claimed it,
and empties both heaps; a queue pruned by its $\beta$ holds no pair
afterwards. `StealingFrontier::run()` calls the $p$ workers by
`ex.run( p , f )`. An exception of a worker stops the others, through
`stop()`, and the first one is thrown again once all the workers have
returned; afterwards,
`leftovers()` hands to a function the pairs left in the queues, which bound
the optimum of what was not explored.

## 24.12 Discussion and references {#sec-24-12}

We follow the model of [Herlihy and Shavit
2008](Z-bibliography.md#ref-herlihy2008), whose book covers
linearizability ([Herlihy and Wing 1990](Z-bibliography.md#ref-herlihy1990)),
the hierarchy of the progress conditions ([Herlihy
1991](Z-bibliography.md#ref-herlihy1991)), the locks that spin ([Anderson
1990](Z-bibliography.md#ref-anderson1990); [Mellor-Crummey and Scott
1991](Z-bibliography.md#ref-mellorcrummey1991)) and the striping of locks.
For the memory of C++ we follow [Boehm and Adve
2008](Z-bibliography.md#ref-boehm2008), and [Williams
2019](Z-bibliography.md#ref-williams2019) explains it for the programmer,
with the total order of the sequentially consistent operations and the
fences.

The queue of one producer and one consumer goes back to [Lamport
1983](Z-bibliography.md#ref-lamport1983), and its engineering for the
caches to [Lee et al. 2010](Z-bibliography.md#ref-lee2010) and to B-Queue
([Wang et al. 2013](Z-bibliography.md#ref-wang2013)). Our lanes of blocks
recycled are those of the unbounded queues of FastFlow ([Aldinucci et al.
2012](Z-bibliography.md#ref-aldinucci2012)), and the stack of the inbox is
that of [Treiber 1986](Z-bibliography.md#ref-treiber1986). The bounded
queue of many producers and consumers is that of [Vyukov
2010](Z-bibliography.md#ref-vyukov2010), which [Nikolaev
2019](Z-bibliography.md#ref-nikolaev2019) made lock-free in the strict
sense. Work stealing uses the deque of [Chase and Lev
2005](Z-bibliography.md#ref-chase2005), with the orders of memory proved
correct by [Lê et al. 2013](Z-bibliography.md#ref-le2013); the schedule of
[Blumofe and Leiserson 1999](Z-bibliography.md#ref-blumofe1999) rests on
it. The union-find is that of [Jayanti and Tarjan
2021](Z-bibliography.md#ref-jayanti2021), with the analysis of the
sequential splitting of [Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984).

Among the hash tables, we built the cuckoo table after MemC3 ([Fan et al.
2013](Z-bibliography.md#ref-fan2013)) and the search in breadth of [Li et
al. 2014](Z-bibliography.md#ref-li2014), with the seqlock analyzed by
[Boehm 2012](Z-bibliography.md#ref-boehm2012). Our table of linear probing
that grows follows the folklore table and the growth of [Maier et al.
2019](Z-bibliography.md#ref-maier2019), who also compare the concurrent
hash tables, among them those that grow without locks. The tables of open
addressing without locks of [Purcell and Harris
2005](Z-bibliography.md#ref-purcell2005) also remove keys, at the price of
a protocol of states per place. The phase-concurrent tables of [Shun and
Blelloch 2014](Z-bibliography.md#ref-shun2014) give deterministic contents
by separating the phases of insertion and of lookup; the table of
[§24.8](24-concurrent.md#sec-24-8) does not need them, since its insertions
of one key agree on the pair kept.

The MultiQueue is that of [Rihani et al. 2015](Z-bibliography.md#ref-rihani2015),
with the engineering of [Williams et al.
2021](Z-bibliography.md#ref-williams2021) (and the later account of
[Williams and Sanders 2025](Z-bibliography.md#ref-williams2025)). Its rank
errors are bounded by [Alistarh et al. 2017](Z-bibliography.md#ref-alistarh2017)
and, exactly in the sequential model, by [Walzer and Williams
2025](Z-bibliography.md#ref-walzer2025). For the event count we follow [Reed
and Kanodia 1979](Z-bibliography.md#ref-reed1979), with the asymmetric
fences of [Ladan-Mozes et al. 2011](Z-bibliography.md#ref-ladanmozes2011).
The priority queues that are stolen combine the steal of half of [Hendler
and Shavit 2002](Z-bibliography.md#ref-hendler2002) with the
receiver-initiated steals of [Acar et al. 2013](Z-bibliography.md#ref-acar2013),
which the laws of order of [Attiya et al.
2011](Z-bibliography.md#ref-attiya2011) motivate, and with the balancing of
the best subproblems of the parallel branch-and-bound ([Tschöke et al.
1995](Z-bibliography.md#ref-tschoke1995); [Karp and Zhang
1993](Z-bibliography.md#ref-karp1993); [Sanders
1998](Z-bibliography.md#ref-sanders1998)). [Wimmer et al.
2014](Z-bibliography.md#ref-wimmer2014) and [Postnikova et al.
2022](Z-bibliography.md#ref-postnikova2022) study other ways of stealing
priority work. A comparison of these structures with those of other
libraries (e.g., oneTBB, Folly, the queues of FastFlow) belongs to the
benchmarks of appendix B, made on an idle machine, and it is not reported
here.

## 24.13 Exercises {#sec-24-13}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize a bounded queue between one producer thread and one consumer
   thread with no lock and no read-modify-write, and show that the copies
   of the indices kept by each thread never make an operation fail
   wrongly. Tests: `engineered.spsc-queue`,
   `engineered_queues.spsc-queue-items`.
2. Realize an unbounded queue of many producers and one consumer in which
   the producers never write a common word, and bound the blocks that a
   lane keeps. Test: `engineered_queues.mpsc-queue`.
3. Realize an inbox of many producers whose consumer takes all the messages
   by one atomic operation, in the order of sending of each producer, and
   explain why the ABA problem cannot occur. Test:
   `engineered.mpsc-inbox`.
4. Realize a bounded queue of many producers and many consumers with one
   compare-and-swap per operation, and find the step at which a stopped
   thread holds up the others. Test: `engineered_queues.mpmc-queue`.
5. Realize the deque of Chase and Lev, and show that with release and
   acquire only the owner and a thief may both take the last item. Test:
   `engineered_queues.work-stealing-deque`.
6. Realize a union-find without locks whose labels are the least elements
   of the sets, whatever the order of the operations. Test:
   `engineered_parallel.concurrent-union-find`.
7. Make a cuckoo table concurrent with one lock per region and lookups
   without locks under the versions of the regions, and prove that it
   cannot deadlock. Tests: `engineered.concurrent-cuckoo`,
   `engineered.concurrent-cuckoo-threads`.
8. Realize a table of linear probing shared by threads that grows while
   they insert, with lookups that never wait, and with the pairs in the
   words, in places of two words, or in nodes. Tests:
   `engineered.concurrent-flat-table`,
   `engineered.concurrent-flat-table-threads`,
   `engineered.concurrent-flat-table-words`.
9. Realize the MultiQueue with buffers, check that it is exact with one
   queue, and compare its mean rank error with (24.8). Tests:
   `engineered_queues.multi-queue`, `engineered_queues.multi-queue-rank`,
   `engineered_queues.multi-queue-threads`.
10. Realize an event count with which no wake-up is lost, also with
    several notifiers at once. Tests: `engineered_queues.event-count`,
    `engineered_queues.event-count-notifiers`.
11. Realize the exponential backoff of a waiting thread, and let four
    threads wait with it for a flag. Test:
    `engineered_search.backoff-threads`.
12. Realize a priority queue whose owner pops in the exact order of its
    keys with no read-modify-write while thieves take half of its pairs,
    and keep the least bound of its pairs exact. Tests:
    `engineered_queues.stealable-pq`,
    `engineered_queues.stealable-pq-threads`,
    `engineered_queues.stealable-pq-bound`,
    `engineered_queues.stealable-pq-bound-threads`.
13. Detect the end of a best-first search of $p$ workers that steal from
    one another, also when it is stopped or a worker throws. Tests:
    `engineered_runtime.stealing-frontier`,
    `engineered_runtime.stealing-frontier-stop`,
    `engineered_runtime.stealing-frontier-bound`.

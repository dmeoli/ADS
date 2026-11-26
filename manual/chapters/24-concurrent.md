# 24. Concurrency {#ch-24}

[Source: `include/ADS/concurrent/SPSCQueue.h`, `MPSCQueue.h`,
`MPSCInbox.h`, `MPMCQueue.h`, `WorkStealingDeque.h`,
`ConcurrentUnionFind.h`, `ConcurrentCuckooTable.h`,
`ConcurrentFlatTable.h`, `MultiQueue.h`, `EventCount.h`, `Backoff.h`,
`StealablePriorityQueue.h`, `LockFreePriorityQueue.h`, `Reclaim.h`,
`EpochReclaim.h`, `HazardPointer.h`, `LockFreeStack.h`,
`LockFreeQueue.h`, and their `impl/*.tpp`]

We are concerned in this chapter with the structures that several threads of
one process read and change at the same time, without a lock or with as few
locks as possible. They are the queues between producers and consumers, the
deque of the scheduling by work stealing, the union-find and the hash tables
that the threads share, the relaxed priority queue, the event count on which a
thread sleeps until a condition holds, the priority queues of the workers of a
parallel search, from which the idle workers steal, an exact priority queue
without locks, and the reclamation of the memory of the nodes of the
structures that any thread may unlink, with the stack and the queue that
take it. These structures are the bricks of the parallel algorithms and
of the runtime of the workers of [chapter 25](25-parallel.md#ch-25), and of the
parallel searches of the library. Each of them gives, in a precise sense, the
result of a sequential structure of the earlier chapters; its cost, however,
depends on how the threads meet in memory more than on the number of its
operations. In the sets of [§1.9](01-introduction.md#sec-1-9), the headers of
this chapter are all advanced.

We organize the chapter as follows. In
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
[§24.10](24-concurrent.md#sec-24-10), the priority queues that are
stolen in [§24.11](24-concurrent.md#sec-24-11), the exact priority
queue without locks in [§24.12](24-concurrent.md#sec-24-12), the
structures of one writer in [§24.13](24-concurrent.md#sec-24-13), and the
reclamation of the memory of the nodes, with the stack and the queue
without locks, in [§24.14](24-concurrent.md#sec-24-14). The file
`impl/Spin.tpp`
holds only two helpers that the other headers share (the pause of the
processor in a loop that spins, and the prefetch of a line of cache), and
we describe them where they are used.

## 24.1 The model of the threads and of the memory {#sec-24-1}

We call *shared object* a structure whose operations are called by several
threads, each thread being a sequence of calls. An operation takes time, from
its *invocation* to its *response*, and two operations of two threads may
overlap. A sequential specification (e.g., the axioms of the QUEUE of [chapter
4](04-queue.md#ch-4)) does not specify overlapping operations, and one needs a
condition that relates the runs of the shared object to those of the sequential
one. Throughout the chapter we use the condition of [Herlihy and Wing
1990](Z-bibliography.md#ref-herlihy1990). We say that a run is *linearizable*
if each operation seems to take effect at one instant between its invocation
and its response, its *linearization point*, and the responses are those of the
sequential object on the operations taken in the order of their points; an
object is linearizable if all its runs are. A thread may then reason on a
linearizable object as on the sequential structure, and the headers name the
point of each operation (e.g., the compare-and-swap that claims a place, or the
store that publishes an item). [Figure 24.1](24-concurrent.md#fig-24-1) shows a
run of three operations on a queue. The push of `b` ends before the pop starts,
while the push of `a` overlaps both; the answer `b` of the pop is explained by
the order of the points push(`b`), push(`a`), pop.

![Figure 24.1. Three operations of three threads on a FIFO queue, each an interval from its invocation to its response, with its linearization point (the dot); the order of the points, push(b), push(a), pop, explains the answer b of the pop.](../figures/concurrent-linearizability.svg){#fig-24-1}

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

The memory is that of C++, which the headers use through `std::atomic` ([Boehm
and Adve 2008](Z-bibliography.md#ref-boehm2008)). We call *data race* a pair of
accesses of two threads to one location, one of them a write, neither of them
atomic and neither ordered before the other. A program with a data race has no
defined meaning, and a word that two threads may access at once is therefore
always an atomic. Each atomic access carries an *order*. A *relaxed* access is
atomic, and it orders no other access. A store with *release* and a load with
*acquire* that reads its value *synchronize*: the writes that the storing
thread made before the store *happen before* the reads that the loading thread
makes after the load. This is how an item written by plain stores is handed to
another thread, by publishing afterwards an index or a pointer with release. A
*sequentially consistent* access is acquire and release and, furthermore, takes
a place in a single total order $S$ of all such accesses (and of the
sequentially consistent fences), consistent with the order of each thread.

Between threads, the orders come only from these. A relaxed access orders
no other access, and two threads that each write one word and then read
the word of the other, with release and acquire, may both miss the write
of the other; only $S$ excludes it. Several lemmas below (the last item of
a deque, the growth of a hash table, the sleep of a waiter) rest on this
pattern, and we prove them by placing four accesses in $S$.

We now recall two facts of the hardware that decide the costs. Indeed, a write
to a line of cache takes the line away from the caches of the other cores,
which then miss on it. Two words written by two threads on one line (*false
sharing*) make the line travel at each write; the headers therefore put each
such word on a separate line, often on a pair of lines, since many processors
of the x86 family fetch the lines two at a time. Also, a read-modify-write
(e.g., a compare-and-swap or an exchange) costs several times a plain access,
and it orders the memory around it. A structure is "fast" when its common case
has none, and [Table 24.1](24-concurrent.md#tab-24-1) states, for each
operation, how many it pays. Finally, we recall the *ABA problem* of a
compare-and-swap on a pointer. A thread reads the top $A$ of a stack and its
successor $B$; another thread pops $A$ and $B$ and pushes $A$ again; then the
compare-and-swap of the first thread from $A$ to $B$ succeeds, and puts back on
the stack a cell that is no longer there. In ADS the headers avoid it by
construction rather than by counters of versions (cf. [Lemma
24.3](24-concurrent.md#lem-24-3)).

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
| `MPSCInbox` | many producers, one consumer | lock-free push but for the allocation of a block, lock-free take | one CAS per push; per take, one exchange, a fetch-and-add per run of messages of one block, a CAS per block given back, and $O( k )$ |
| `MPMCQueue` | many producers, many consumers | lock-free but for a thread stopped between claim and publication | one CAS per operation without contention |
| `WorkStealingDeque` | one owner, many thieves | wait-free calls, a steal may fail (abort); retried steals lock-free | $O( 1 )$ amortized for the owner, a CAS only on the last item; one per steal |
| `ConcurrentUnionFind` | any | lock-free | $O( \log_{2 + m / n} n )$ amortized on one thread |
| `ConcurrentCuckooTable` | any | blocking (a lock per region), lookups without locks for atomic keys and values | $O( 1 )$ expected, $O( 1 )$ amortized for the doublings |
| `ConcurrentFlatTable` | any | wait-free lookups but across a growth, insertions blocking only during a growth (and the renewal of a group, with epochs) | $O( 1 )$ expected, $O( 1 )$ amortized for the growths; no removals; `clear()` $O( 1 )$ amortized with epochs |
| `MultiQueue` | any, by a handle each | blocking (a lock per queue, taken at the first try) | $O( \log m )$ amortized, $q / ( q - 1 )$ draws expected; relaxed |
| `StealablePriorityQueue` | one owner, many thieves | wait-free for the owner, lock-free steal | $O( \log m )$ for the owner, no read-modify-write; one exchange per steal |
| `LockFreePriorityQueue` | any, by a handle each | lock-free but for the allocation of a node; reclamation blocking | $O( \log m )$ expected insertion, two CAS on average; one FAO per DeleteMin, plus the prefix walked; exact |
| `LockFreeStack` | any, by a handle each | lock-free but for the allocation of a node and for the reclamation | one CAS per push and per pop without contention, plus the reclamation |
| `LockFreeQueue` | any, by a handle each | lock-free but for the allocation of a node and for the reclamation | two CAS per enqueue, one per dequeue without contention, plus the reclamation |
| `EpochDomain` (`FraserEpochs`) | any, by a record each | blocking: a thread stopped in a guard stops the frees | one exchange per guard; $O( N )$ per $R$ retirements, for $N$ records |
| `HazardDomain` (`HazardPointers`, `hazard_pointer`) | any, by a record or a hazard pointer each | lock-free; fewer than $B ( 2 H + 64 )$ nodes waiting | one exchange per node protected; $O( \log H )$ amortized per node retired, for $H$ slots and $B$ bins |
| `EventCount` | any | a waiter sleeps, a notifier never waits | a fence and a read when nobody waits |
| `SingleWriterFlatTable` | one writer, many readers | wait-free for the writer but for the allocation of a growth, wait-free lookups but across a growth | $O( 1 )$ expected, no read-modify-write; $O( 1 )$ amortized for the growths |
| `MinBoard` | one writer per slot, many readers | wait-free | a load and at most a store per publish, $p$ loads per `min()`; coherence only |

We remark that the queues of one producer or of one consumer are cheaper than
the general one because each of their indices has a single writer. Such an
index is written by plain stores with release and read by loads with acquire,
and no read-modify-write is needed; many writers of one index, instead, must
agree by a compare-and-swap, and contend on its line. This remark runs through
the whole chapter. Indeed, the structures are designed so that the "common"
case of an operation writes only words that no other thread writes, and they
pay a read-modify-write only where two threads may really want the same thing
(e.g., the last item of a deque, a free place of a table, or an offer of a
queue that is stolen, cf. [§24.11](24-concurrent.md#sec-24-11)).

Each structure exposes its template parameters as member types, `Item` for
the items of the queues and of the deque, `Key` and `Item` for the pairs of
the priority queues (and, in `StealablePriorityQueue`, `Entry` for the
pair kept and `Bound` for its bound, see
[§24.11](24-concurrent.md#sec-24-11)), and `Key` and `Value` for the
tables; we name them in the following sections only where they matter.

## 24.3 Queues of one producer {#sec-24-3}

`SPSCQueue` is the circular vector of the queue of [chapter
4](04-queue.md#ch-4) shared by two threads. Only the producer writes the tail
$t$, and only the consumer writes the head $h$; each index is thus an atomic
written with release and read with acquire, and no lock is needed (the queue of
[Lamport 1983](Z-bibliography.md#ref-lamport1983)). The queue is full when $t -
h = c$ and empty when $h = t$. Since the indices only grow and are reduced by
the mask at each access, the two tests hold also when the integers wrap around.
Each thread keeps besides a copy of the index of the other one,
$\mathit{seen}_P \le h$ for the producer and $\mathit{seen}_C \le t$ for the
consumer, and reads the index again only when the queue looks full or empty by
the copy ([Lee, Bu and Chandranmenon 2010](Z-bibliography.md#ref-lee2010)). A
stale copy is never ahead of the index it copies; hence it can only make the
queue look fuller or emptier than it is. [Figure
24.2](24-concurrent.md#fig-24-2) shows the vector before and after a push, with
both copies stale and neither of them read.

![Figure 24.2. An `SPSCQueue` of capacity 8 with the items `b`, `c`, `d`, before and after `push(e)`; the producer writes the place $t \bmod 8$ and then the tail, and the copies $\mathit{seen}_P$ of the head and $\mathit{seen}_C$ of the tail, both stale, are not read, since the queue looks neither full to the producer nor empty to the consumer.](../figures/concurrent-spsc.svg){#fig-24-2}

The class gives the producer `push()` and `emplace()`, the consumer `pop()`
and `tryPop()`, and both of them the batch operations and the wait of
[§24.10](24-concurrent.md#sec-24-10); its public members are

```cpp
// declarations of <ADS/concurrent/SPSCQueue.h>
template< class T >
class SPSCQueue
{
 public:
 explicit SPSCQueue( std::size_t capacity );
 bool push( const T & v );
 bool push( T && v );
 template< class... Args >
 bool emplace( Args &&... args );
 std::optional< T > pop( void );
 bool tryPop( T & x );
 template< class It >
 std::size_t pushMany( It first , std::size_t n );
 template< class F >
 std::size_t consume( F && f , std::size_t n = std::size_t( -1 ) );
 void notify( void ) noexcept;
 void waitPop( T & x );
 std::size_t size( void ) const;
 std::size_t capacity( void ) const;
 };
```

and two threads use it as follows, the producer with `push()`, which
answers false on a full queue, and the consumer with `waitPop()`:

```cpp
ADS::SPSCQueue< int > Q( 1024 );  // c = 1024
std::thread producer( [ & ]() {
 for( int i = 0 ; i < 100 ; ++i ) {
  while( ! Q.push( i ) ) {
   }  // full: try again
  Q.notify();
  }
 } );
long sum = 0;
for( int i = 0 ; i < 100 ; ++i ) {
 int x;
 Q.waitPop( x );  // the items 0, 1, ..., 99
 sum += x;
 }
producer.join();  // sum is 4950
```

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

*Proof.* We follow the item of the index $i$. Its write precedes in the
producer the release store of a tail larger than $i$, which the consumer reads
by an acquire load before it reads $A[ i ]$. These two accesses synchronize,
whence the first arrow of (24.1). Similarly, the second arrow follows from the
argument on the head, since the producer writes $A[ i + c ]$ only after it has
read a head larger than $i$, i.e., after it has found $t - h < c$. A stale copy
can only make the test of full (of empty) succeed where the index would make it
fail, and the copy is read again before the queue is declared full (empty);
thus no push and no pop fails wrongly. Each operation runs a constant number of
steps, whatever the other thread does. At its store of the tail a push makes
its item visible at the end of the queue, and at its store of the head a pop
removes the first item, which it has read; the order of these points is the
order of the operations of the sequential queue. $\square$

The implementation follows [Algorithm 24.1](24-concurrent.md#alg-24-1)
line by line; it builds the item in place, and it compares the index with
the copy before it reads the index of the other thread (lines 3 to 5 of
`emplace()`), so that the common case reads no line that the other thread
writes.

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
last pop took an item and the consumer finds some new ones, but fewer than
$\min( 64 , c / 2 )$, it therefore waits 64 pauses of the
processor for a batch before it takes them (the backtracking of B-Queue,
[Wang et al. 2013](Z-bibliography.md#ref-wang2013)). Then the lines travel
once per batch. After a pop that found no item, instead, the consumer takes
at once what comes, and a single item after a "silence" costs no wait.
Furthermore, a consumer that finds the queue empty prefetches the line of
its next place before it reads the tail again. When the item has come, the two
lines travel together, and a round trip between two threads usually costs
one miss a side instead of two in a row.

Two batch operations publish several items by one store of the tail and free
several places by one store of the head, which is the "cheapest" way in which
one can move many items between two threads. Note that `consume( f , n )` calls
$f$ on each item in place, and dequeues the items called also when a call
throws.

--8<-- "generated/SPSCQueue-pushMany.pseudo.md"

--8<-- "generated/SPSCQueue-consume.pseudo.md"

`MPSCQueue` serves many producers and one consumer, and is unbounded. Each
producer has a *lane*, i.e., a queue of one producer and one consumer, and the
producers never compete with one another. Its consumer takes the lanes in turn,
and drains the one it is on before it moves to the next. We build a lane as a
list of blocks of $B$ places (1024 by default, a power of 2), after the list of
bounded buffers of [Aldinucci et al.
2012](Z-bibliography.md#ref-aldinucci2012). Each producer writes the places of
its last block in order and publishes, after each item, their count $f$ in the
block, by a store with release and no read-modify-write. When the block is full
it links a new one after it, by a store with release of the link. In turn, the
consumer reads the count of its block once for all the items that it shows, and
takes them. When it has taken the $B$ items of a block and finds the next one
linked, it gives the block back to its lane, on a stack that the producer
empties at once, by an exchange, when it needs a block and before it allocates
one ([Figure 24.3](24-concurrent.md#fig-24-3)). A producer is a handle of class
`Producer` that `producer()` returns, used by one thread at a time. All the
lanes form a list built by the push of Treiber of
[§24.4](24-concurrent.md#sec-24-4), and they live as long as the queue, also
when their producer is gone and its items are still to be taken.

![Figure 24.3. An `MPSCQueue` with two lanes of blocks of 4 places; $f$ is the count of the items written in a block, the consumer reads from the place below its arrow and the producer writes at the place under its own; the first lane has a block that the consumer gave back, on the stack from which the producer takes its next block.](../figures/concurrent-mpsc.svg){#fig-24-3}

Its public members are those below; a producer pushes through its handle,

```cpp
// declarations of <ADS/concurrent/MPSCQueue.h>
template< class T , std::size_t B = 1024 >
class MPSCQueue
{
 public:
 class Producer
 {
  public:
  template< class... Args >
  void emplace( Args &&... args );
  void push( const T & v );
  void push( T && v );
  void notify( void );
  };
 Producer producer( void );
 bool tryPop( T & x );
 std::size_t popInto( T * out , std::size_t n );
 void waitPop( T & x );
 std::size_t waitPopInto( T * out , std::size_t n );
 bool empty( void );
 };
```

e.g., four threads that each send 1000 items to one consumer:

```cpp
ADS::MPSCQueue< int > Q;
std::vector< std::thread > T;
for( int k = 0 ; k < 4 ; ++k )
 T.emplace_back( [ & , k ]() {
  auto P = Q.producer();  // one lane per handle
  for( int i = 0 ; i < 1000 ; ++i )
   P.push( 1000 * k + i );
  P.notify();
  } );
int x , got = 0;
while( got < 4000 ) {
 Q.waitPop( x );  // in order within each lane
 ++got;
 }
for( auto & t : T )
 t.join();
```

<span id="alg-24-2"></span>**Algorithm 24.2.** The operations of
`MPSCQueue` (the list of bounded buffers of [Aldinucci et al.
2012](Z-bibliography.md#ref-aldinucci2012), the wait for a batch of [Wang
et al. 2013](Z-bibliography.md#ref-wang2013), and the stack of [Treiber
1986](Z-bibliography.md#ref-treiber1986) for the lanes).

--8<-- "generated/MPSCQueue-emplace.pseudo.md"

--8<-- "generated/MPSCQueue-popInto.pseudo.md"

--8<-- "generated/MPSCQueue-producer.pseudo.md"

We may thus see each lane as a queue of Lamport whose block holds the count of
its items in place of the tail, and [Lemma 24.1](24-concurrent.md#lem-24-1)
applies (cf. (24.1)) to it place by place: the item is written before its count
is published, and the consumer reads the count with acquire before the items. A
block is reused by its producer only after the consumer has read all its $B$
items and has pushed it, with release, on the stack of the blocks given back.
Its producer takes that stack by an exchange with acquire, and therefore (24.1)
holds for each place of a block across its reuses. We observe that this stack
has one pusher, the consumer, and one taker, which takes it all at once; it has
thus no ABA problem (cf. [Lemma 24.3](24-concurrent.md#lem-24-3)).

<span id="lem-24-2"></span>**Lemma 24.2.** *A lane of `MPSCQueue` that has
held at most $m$ items at once keeps at most $\lceil m / B \rceil + 1$
blocks. A push costs $O( 1 )$ time and no read-modify-write, but for the
push that finds its block full, which costs at most an exchange (none when
its spare stack has a block), or an allocation when the lane has fewer
blocks than it needs.*

*Proof.* At any moment the blocks of a lane in use are those from the block
of the consumer to that of the producer, which hold the items present; they
are at most $\lceil m / B \rceil + 1$, since the first and the last may be
partly empty. A block out of them lies in one of the two stacks of blocks
to reuse, and a producer allocates a block only when both stacks are empty,
i.e., when all the blocks of the lane are in use. Hence the lane never
holds more blocks than the largest number in use at once. A push writes its
item and a count, two stores with no read-modify-write, unless it finds
its block full; then it
takes a block from its spare stack, or all the stack given back by an
exchange, or a new block. $\square$

We conclude that in a steady state a lane cycles over the same blocks, which
stay in the caches. Here the count and the link of a block share a line that
only the producer writes, and the consumer touches the lines of the producer
once per batch, with the same wait for a batch as `SPSCQueue` (when a call took
some items and the next one finds fewer than $\min( B , 64 )$ in the block).
Furthermore, the consumer of either queue may sleep until an item comes, by
`waitPop()`, on the event count of [§24.10](24-concurrent.md#sec-24-10) (and
the consumer of `MPSCQueue` by `waitPopInto( out , n )`, the `popInto()` that
sleeps until at least one item comes); the producers then call `notify()` after
their pushes.

## 24.4 Queues of many producers or of many consumers {#sec-24-4}

`MPSCInbox` is the "simplest" of the queues of many producers, e.g., the
inbox of a worker of a parallel search, into which the other workers send
their messages. A producer pushes a cell on a stack by a loop of
$\mathrm{CAS}( \mathit{top} , \mathit{next} , \mathit{cell} )$ (the stack of
[Treiber 1986](Z-bibliography.md#ref-treiber1986)). Its consumer takes all
the stack at once, by $\mathrm{exchange}( \mathit{top} , \mathbf{nil} )$,
and reverses it; the messages of each producer thus come out in the order
in which it sent them ([Figure 24.4](24-concurrent.md#fig-24-4)), and the
consumer never competes with the producers on single items. The cells come
from blocks of 64 that each producing thread owns, and a producer takes the
cells of its block in order, with no atomic operation. A consumer adds to a
count of the block the messages it has taken from it, by one fetch-and-add
per run of messages of the block, and the one that brings the count to 64
gives the block back to its owner, by a compare-and-swap on a stack that the
owner empties by an exchange when it needs a block; a producer in a steady
state thus makes no allocation. Hence `take()` costs one exchange however
many messages there are, a fetch-and-add per run and a loop of
compare-and-swap per block given back, and $O( k )$ time for the $k$
messages taken (cf. [Lemma 24.3](24-concurrent.md#lem-24-3)). Also, a push
wakes
a sleeping consumer only when it finds the stack empty, since the pushes
onto a stack that is not empty are taken with the one that found it empty.
This inbox suits the messages that usually come in bursts.

![Figure 24.4. The stack of an `MPSCInbox` before and after `push(d)`, whose compare-and-swap links the new cell above the top it read, and after `take()`, which exchanges the top with nil and returns the messages reversed, in the order of sending.](../figures/concurrent-inbox.svg){#fig-24-4}

Its public members are

```cpp
// declarations of <ADS/concurrent/MPSCInbox.h>
template< class T >
class MPSCInbox
{
 public:
 void push( T v );
 std::vector< T > take( void );
 std::size_t takeInto( std::vector< T > & out );
 std::vector< T > waitTake( void );
 bool empty( void ) const;
 };
```

where `takeInto()` appends the messages of `take()` to a vector that keeps
its room from call to call, and `waitTake()` is the `take()` that sleeps
until a message comes, on the event count of
[§24.10](24-concurrent.md#sec-24-10).

<span id="alg-24-3"></span>**Algorithm 24.3.** The push and the take of
`MPSCInbox` ([Treiber 1986](Z-bibliography.md#ref-treiber1986)).

--8<-- "generated/MPSCInbox-push.pseudo.md"

--8<-- "generated/MPSCInbox-take.pseudo.md"

<span id="lem-24-3"></span>**Lemma 24.3.** *Let the cells of a stack of
Treiber be pushed one at a time, by a compare-and-swap whose new value is a
cell of the pushing thread, and be taken only all at once, by an exchange;
and let the cells come from blocks given back to their owner as in
`MPSCInbox`. Then (i) the ABA problem does not occur: each successful push
links its cell above the true top, and each take returns all the cells
pushed before it that are not yet taken; (ii) the owner writes a cell again
only after the consumer that took it has moved its message out and read its
link, and a cell is never in two stacks at once; (iii) the push and the take
are lock-free (the push but for the allocation of a block), and a take of
$k$ cells costs one exchange, at most one fetch-and-add per run of cells of
one block in the order of sending (hence at most $k$), one loop of
compare-and-swap per block that it gives back, and $O( k )$ time besides.*

*Proof.* (i) We recall that the ABA problem needs a compare-and-swap whose
expected value was read before the cell it names was removed and put back.
Here the only compare-and-swap on $\mathit{top}$ is that of a push, whose
expected value is the top read by the pusher. If it succeeds, the top is
that value at that instant, whatever happened in between (e.g., a take,
then pushes that brought the top back to the same address after a reuse of
the cell); the cell becomes the new top above the cell that is really
there, since a push never reads the successor of the top. No cell is ever
popped singly. An exchange is one atomic step, and the cells it returns are
those of the pushes that precede it in the order of the modifications of
$\mathit{top}$. Finally, a push publishes its cell with release and the take
reads the top with acquire, and therefore the contents of the cells are
seen.

(ii) The owner gives out each cell of a block once per use of the block,
in order, and each cell given out is pushed onto one inbox and taken by
one take (or by the destructor of the inbox); a thread that ends adds to
the count the cells it never gave out. The consumer reverses the cells it
took before it moves any message, reads the link of each cell before it
moves the message of that cell, and adds a run to the count only after the
last message of the run is moved out and destroyed. Hence the count of a
block reaches 64 once per use, at the fetch-and-add that adds its last
cells. These fetch-and-adds are read-modify-writes, with acquire and
release, of one atomic object, and each of them therefore reads the value
of the one before it in the order of the modifications; hence all the
accesses of the consumers to the cells of the block happen before the last
fetch-and-add. Its thread pushes the block on the stack `given` of the
owner by a compare-and-swap with release, and the owner takes the stack by
an exchange with acquire before it writes a cell of the block again; the
accesses of the consumers thus happen before the next writes of the owner.
A block that the owner fills is a new one or one that it took back from
`given`, and a cell taken by an exchange is no
longer reachable from $\mathit{top}$; hence a cell is in one stack at most,
and a reused cell is pushed as a new one, as in (i). The stack `given` is a
stack of Treiber too, pushed by compare-and-swap and emptied whole by an
exchange, and (i) holds for it; once its owner has ended, its top is a value
that no block has, and the consumer that brings a count to 64 then frees
the block instead of pushing it.

(iii) The compare-and-swap of a push fails only if the top changed after
the pusher read it, i.e., only if another push or a take succeeded in
between, and that of a block given back fails only if another block was
given back to the same owner or the owner took its stack in between; hence
some thread completes an operation at each failure. The exchange and the
fetch-and-adds never fail, and the reversal and the moves touch only the
$k$ cells taken, which no other thread can reach any more. The cells are
gone through in the order of sending, with one fetch-and-add each time the
block changes and once at the end, and a block can be given back only at
one of these fetch-and-adds, which gives the cost. $\square$

The same argument holds for the list of the lanes of `MPSCQueue` and for
its stacks of blocks given back. A push is not wait-free, however, since a
pusher may lose every round to other pushers, and for the same reason
neither is a take that gives a block back.

`MPMCQueue` is the bounded queue of many producers and many consumers of
[Vyukov 2010](Z-bibliography.md#ref-vyukov2010), on a circular vector of $c$
places. Each place carries a *sequence* number besides the room of an item: the
place of the index $i$ holds the sequence $i$ when it waits for the push of
$i$, $i + 1$ when it holds the item of $i$, and $i + c$ when that item has been
taken and the place waits for the push of $i + c$. We let a producer read the
tail $p$ and the sequence $s$ of its place. If $s = p$ it claims the index by
$\mathrm{CAS}( t , p , p + 1 )$, builds the item and publishes it by storing
the sequence $p + 1$ with release; if $s < p$ the queue is full; and if $s > p$
another producer took $p$, and it reads the tail again. A consumer does the
same on the head $q$ with the sequence $q + 1$, and frees the place by storing
$q + c$ ([Figure 24.5](24-concurrent.md#fig-24-5)). Its tail and its head lie
on separate lines, and producers and consumers meet only at the places they
share, with one read-modify-write per operation.

![Figure 24.5. An `MPMCQueue` of capacity 4 with the items of the indices 5 and 6, before and after a push that claims the index 7: the place 3 held the sequence 7 (free for the index 7), and the push writes the item and then the sequence 8; the place 0 holds the sequence 8, i.e., it waits for the push of the index 8.](../figures/concurrent-mpmc.svg){#fig-24-5}

Its public members are

```cpp
// declarations of <ADS/concurrent/MPMCQueue.h>
template< class T >
class MPMCQueue
{
 public:
 explicit MPMCQueue( std::size_t capacity );
 template< class... Args >
 bool emplace( Args &&... args );
 bool push( const T & v );
 bool push( T && v );
 bool tryPop( T & x );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 };
```

where `push()` answers false on a full queue and `tryPop()` on an empty
one, and `size()` is exact only when no operation runs.

<span id="alg-24-4"></span>**Algorithm 24.4.** The push and the pop of
`MPMCQueue` ([Vyukov 2010](Z-bibliography.md#ref-vyukov2010)); $C[ i ]$ is
the place $i \bmod c$ and $C[ i ].s$ its sequence.

--8<-- "generated/MPMCQueue-emplace.pseudo.md"

--8<-- "generated/MPMCQueue-tryPop.pseudo.md"

We remark that the implementation compares the sequence with the index on their
difference, taken as a signed integer; the test thus holds across the
wrap-around of the integers, as the tests of `SPSCQueue` do.

<span id="lem-24-4"></span>**Lemma 24.4.** *In `MPMCQueue`, each index is
claimed by one producer and by one consumer. The consumer of the index $i$
reads the item that the producer of $i$ wrote, and the producer of $i + c$
writes the place only after the consumer of $i$ has read it; a failed
compare-and-swap means that another thread claimed the index.*

*Proof.* We note first that a compare-and-swap of the tail from $p$ to $p + 1$
succeeds for one thread only, since the tail never goes back, and the same
holds for the head. In fact, the producer of $i$ publishes by storing $i + 1$
with release after it has written the item, and the consumer claims $i$ only
after an acquire load that read $i + 1$, whence the first arrow of (24.1). In
turn, the consumer stores $i + c$ with release after it has read the item, and
the producer of $i + c$ claims its index only after it has read the sequence
$i + c$ with acquire, whence the second one. A producer that reads a sequence
below $p$ finds the item of $p - c$ not yet taken, i.e., the queue full, and
one that reads a sequence above $p$ was overtaken, and reads the tail again.
Also, the failure of its compare-and-swap means that another producer claimed
$p$, which is progress of the queue. Consumers are symmetric. $\square$

The queue is lock-free in the common case, but not in the strict sense of
[§24.1](24-concurrent.md#sec-24-1). In fact, a thread stopped between its
compare-and-swap and its publication holds up the threads that come round to
its place, which find a sequence that means neither "free" nor "full". For the
same reason a pop may answer that the queue is empty while a push that has
claimed its index has not yet published it, which is consistent with a
linearization point of the push at its publication. We remark that the rings of
[Nikolaev 2019](Z-bibliography.md#ref-nikolaev2019), with a cycle per place,
are lock-free in the strict sense, at the price of a somewhat more complex
protocol. In [chapter 25](25-parallel.md#ch-25) the farm takes its tasks from
an `MPMCQueue`; there such a stop happens only when the system deschedules a
thread, and it then delays the other threads only until that thread runs again.

## 24.5 The deque of work stealing {#sec-24-5}

In the scheduling by work stealing each worker pushes and takes its tasks at
the bottom of its deque, as on a stack, which keeps the tasks it made last, and
their data, in its cache. An idle worker steals the oldest task, at the top of
the deque of another worker; in a computation by fork and join this task is
often the root of a "large" subtree of work ([Blumofe and Leiserson
1999](Z-bibliography.md#ref-blumofe1999); the bound on the time of this
schedule is in [chapter 25](25-parallel.md#ch-25)). `WorkStealingDeque` is the
deque of [Chase and Lev 2005](Z-bibliography.md#ref-chase2005). Its owner
thread pushes and takes at the bottom, and any other thread steals from the
top, without locks, on a circular vector of $2^k$ places that the owner doubles
when it is full. In it, the bottom $b$ is written by the owner only, and the
top $t$, which only grows, only by a compare-and-swap; the items are those of
$[ t , b )$, at the places of their indices modulo the size of the vector
([Figure 24.6](24-concurrent.md#fig-24-6)). Its items are kept in atomics of
relaxed order, and their type must therefore be trivially copyable (e.g., an
index or a pointer to a task).

![Figure 24.6. A `WorkStealingDeque` of 8 places with the items `a`, `b`, `c` in $\lbrack t , b ) = \lbrack 2 , 5 )$, after `take()` of the owner, which takes `c` at the bottom, and after `steal()` of a thief, which takes `a` at the top; the violet cell is the place that the operation empties.](../figures/concurrent-deque.svg){#fig-24-6}

Its public members are

```cpp
// declarations of <ADS/concurrent/WorkStealingDeque.h>
template< class T >
class WorkStealingDeque
{
 public:
 explicit WorkStealingDeque( std::size_t capacity = 1024 );
 void push( T x );                 // the owner
 std::optional< T > take( void );  // the owner
 bool steal( T & x );              // any thief
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 };
```

in which `push()` doubles the vector when it is full, so that only `take()`
and `steal()` may fail, the latter also when it loses a race.

<span id="alg-24-5"></span>**Algorithm 24.5.** The operations of
`WorkStealingDeque` ([Chase and Lev 2005](Z-bibliography.md#ref-chase2005),
with the orders of memory of [Lê et al. 2013](Z-bibliography.md#ref-le2013));
$A$ is the vector of $|A|$ places, the index $i$ at its place
$i \bmod |A|$.

--8<-- "generated/WorkStealingDeque-push.pseudo.md"

--8<-- "generated/WorkStealingDeque-take.pseudo.md"

--8<-- "generated/WorkStealingDeque-steal.pseudo.md"

We first remark that, when more than one item is left, the owner and the
thieves take different items (the owner $b - 1$ and a thief $t$), and only the
thieves race among themselves, on the compare-and-swap of $t$. The somewhat
"delicate" case is the last item, which the owner and a thief must not both
take, nor both leave. Here the orders of memory matter, and [Lê et al.
2013](Z-bibliography.md#ref-le2013) placed two fences that make the deque
correct in the model of C11. In ADS, the header writes those fences as
sequentially consistent accesses to the bottom and to the top, since the
sanitizer of threads models these accesses but not the fences themselves.

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
value of the word written by the other, and both take the item. The two
sequentially consistent pairs are the store of the bottom and the load of the
top of `take()` (lines 2 and 3 of its pseudocode in [Algorithm
24.5](24-concurrent.md#alg-24-5)) and the loads of the top and of the bottom of
`steal()` (its line 2); the other accesses are relaxed, or acquire for the
vector.

We note that a thief reads the item before the compare-and-swap that claims it,
from the vector that it reads after the bottom; the owner publishes a new
vector with release before it publishes a bottom that needs it. A doubling
copies the items of $[ t , b )$ to the same indices of the new vector, and a
thief that read the old vector thus reads from it an item that the copy did not
change; its compare-and-swap then decides whether the item is its. All the old
vectors are kept until the deque is destroyed, and no read is ever of freed
memory.

<span id="lem-24-6"></span>**Lemma 24.6.** *The doublings of a
`WorkStealingDeque` of initial capacity $c_0$ cost $O( 1 )$ amortized time
per push, and the vectors kept occupy less than twice the vector in use.*

*Proof.* We charge each doubling to the pushes before it. A doubling from
$2^k$ places copies $b - t = 2^k$ items. It happens
only after at least $2^{k - 1}$ pushes since the previous doubling (or $c_0$
pushes for the first), since a push adds one item and the vector was at
most half full after the previous doubling. Hence the copies are at most
twice the pushes, as for the vector of [Lemma 3.3](03-stack.md#lem-3-3).
The sizes of the vectors form the geometric series
$c_0 , 2 c_0 , \ldots , 2^j c_0$, whose sum is less than
$2^{j + 1} c_0$. $\square$

Thus `push()` and `take()` cost $O( 1 )$ time and no read-modify-write but
when `take()` reaches the last item, and `steal()` costs one
compare-and-swap. The sequential specification against which the deque is
correct lets a steal fail (an *abort*, as in [Chase and Lev
2005](Z-bibliography.md#ref-chase2005)): a thief fails when another thread
took the item it wanted, also while other items remain, and `steal()` then
returns false, so that the caller may try another victim.

<span id="lem-24-7"></span>**Lemma 24.7.** *`WorkStealingDeque` is
linearizable with respect to a deque whose steal may abort, with the point
of a push at its store of the bottom, that of a take at its store of
$b - 1$ (at its compare-and-swap on the last item, at its load of the top
when it finds the deque empty), and that of a steal at its
compare-and-swap (at its load of the bottom when it finds the deque empty).
Each call runs a constant number of steps, but for the copy of a doubling;
a thief that retries until it gets an item is lock-free.*

*Proof.* We observe first that the items present at any instant are those of $[
t , b )$ by the values of $t$ and $b$ in the order of their modifications, and
the points above are the instants at which these values change, or are read. A
push writes its item before its store of $b + 1$ and becomes the new bottom
item there. A take with more than one item left takes $b - 1$, which no thief
can reach, since a thief claims only $t < b - 1$ by its compare-and-swap of $t$
after it read the new bottom (it read $b$ after $t$, cf. [Lemma
24.5](24-concurrent.md#lem-24-5)); on the last item, take and steal race by a
compare-and-swap of $t$, and [Lemma 24.5](24-concurrent.md#lem-24-5) gives that
exactly the winner takes it, if anyone reads it there. A successful steal takes
the item $t$ at the instant at which $t$ grows, which is the top item of the
sequential deque then; a failed one changes nothing, and is an abort. Each
operation is a fixed sequence of loads, stores and at most one
compare-and-swap, with no loop but the copy of a doubling. A compare-and-swap
of $t$ fails only because another one on $t$ succeeded, i.e., another operation
completed; hence a thief that retries completes, or some other thread does, at
each of its failures. $\square$

This deque is that of the
pool of threads of [chapter 25](25-parallel.md#ch-25). Its argument returns in
[§24.11](24-concurrent.md#sec-24-11), where the queues of a search are
stolen without making their owner race for each item.

## 24.6 A union-find shared by threads {#sec-24-6}

`ConcurrentUnionFind` realizes the MFSET of [chapter 11](11-mfset.md#ch-11) on
the elements $0 , \ldots , n - 1$, for threads that unite and find at once
([Jayanti and Tarjan 2021](Z-bibliography.md#ref-jayanti2021)). It keeps a
forest of parents $p[ 0 .. n - 1 ]$, a root being its own parent, each parent
an atomic word changed only by a compare-and-swap (with no lock). Thus `find( x
)` climbs to the root by *splitting*: each element on the way is given the
parent of its parent, by a compare-and-swap whose failure is "harmless", and
the paths halve their lengths as they are walked. This is the concurrent form
of the path splitting of [Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984). Then `unite( x , y )` finds the two
roots and hooks the larger one under the smaller, by a compare-and-swap of the
parent of the larger ([Figure 24.7](24-concurrent.md#fig-24-7)). The
compare-and-swap fails only if another thread hooked that root first, and it is
then tried again from the roots found anew.

![Figure 24.7. A `ConcurrentUnionFind` before and after `unite(6, 5)`: the splitting of `find(6)` gives 6 its grandparent 3, `find(5)` finds the root 0, and the larger root 3 is hooked under the smaller root 0; the violet elements are those whose parent changed.](../figures/concurrent-unionfind.svg){#fig-24-7}

Its public members are

```cpp
// declarations of <ADS/concurrent/ConcurrentUnionFind.h>
class ConcurrentUnionFind
{
 public:
 explicit ConcurrentUnionFind( std::size_t n );
 std::size_t size( void ) const;
 std::int32_t find( std::int32_t x );
 std::int32_t parent( std::int32_t x ) const;
 void compress( std::int32_t x );
 bool unite( std::int32_t x , std::int32_t y );
 bool sameSet( std::int32_t x , std::int32_t y );
 };
```

where `unite()` answers whether it joined two sets, and `parent()` reads
$p[ x ]$ without changing it.

<span id="alg-24-6"></span>**Algorithm 24.6.** The operations of
`ConcurrentUnionFind`: the splitting and the linking by index of [Jayanti
and Tarjan 2021](Z-bibliography.md#ref-jayanti2021).

--8<-- "generated/ConcurrentUnionFind-find.pseudo.md"

--8<-- "generated/ConcurrentUnionFind-unite.pseudo.md"

--8<-- "generated/ConcurrentUnionFind-sameSet.pseudo.md"

--8<-- "generated/ConcurrentUnionFind-compress.pseudo.md"

We now show that the correctness rests on an invariant of the order of the
elements, which no interleaving of the operations can break.

<span id="lem-24-8"></span>**Lemma 24.8.** *If a root $x$ is hooked only
under a root $y < x$, by $\mathrm{CAS}( p[ x ] , x , y )$, and a parent
$p[ x ]$ is otherwise only replaced by an ancestor of $x$, then at every
moment*

$$
p[ x ] \le x \quad \forall x , \tag{24.2}
$$

*the forest has no cycle, and the root of each tree is its least element;
hence the label $\mathrm{find}( x )$ of a set does not depend on the order
of the operations, once they have ended.*

*Proof.* We argue by induction on the successful compare-and-swaps. A hook
succeeds only if $x$ is still a root, and sets $p[ x ] = y < x$. The splitting
of `find()` replaces $p[ x ] = u$ by $g = p[ u ] \le u$, by a compare-and-swap
that fails if $p[ x ]$ changed in the meantime; the parent of $u$ only moves up
the tree of $u$, so $g$ is an ancestor of $x$, and $g \le u \le x$ keeps
(24.2). Along a path the elements decrease strictly (but at the root), which
excludes a cycle and makes the root the least element of its tree. `compress()`
sets $p[ x ]$ from $u$ to the root found from $u$, an ancestor of $x$, and the
same argument applies. $\square$

<span id="lem-24-9"></span>**Lemma 24.9.** *`ConcurrentUnionFind` is
lock-free and linearizable: a `find()` takes effect at the read that finds a
root, a `unite()` at its successful hook (or, when it answers false, at the
read of its second `find()` that found the same root), and a `sameSet()`
at the read by which its second `find()` found the root of $y$, whether it
answers true or false.*

*Proof.* A failed compare-and-swap of a splitting only leaves a longer
path, and it fails because another thread changed $p[ x ]$, by a hook or by
a splitting, both successful steps; a failed hook means that another
thread hooked $x$ first. Each loop of the operations thus repeats only
after a successful step of another thread, and since by [Lemma
24.8](24-concurrent.md#lem-24-8) every path decreases strictly, a thread
that runs alone ends after a finite number of steps; this is lock-freedom.
A root, once hooked, is never a root again, and the parents only move up
the trees, so that the sets only merge. At the read that finds a root $r$
from $x$, $r$ is the root of the tree of $x$, hence the answer of the
sequential `find()` then. A successful hook of $x$ under $y$ merges the two
sets at that instant, which is the effect of the sequential `unite()`; a
`unite()` that answers false read at that instant the same root from both
elements. A `sameSet()` that answers true read the root of $y$ equal to a
root of $x$ found before; since sets only merge, $x$ and $y$ are in one set
at the second read. One that answers false found the root $r_y \ne r_x$ of
$y$ and then saw the root $r_x$ of $x$ still a root (line 5 of its
pseudocode); since a root that is hooked never becomes a root again, $r_x$
was a root at the read that found $r_y$, and two distinct roots then
headed the trees of $x$ and of $y$, i.e., two distinct sets. If $r_x$ is no
longer a root, the loop starts again, after a successful hook of another
thread. Each hook uses the orders acquire
and release, and a thread that follows a parent thus sees the writes that
preceded the hook. $\square$

After the compression of all the elements, with no union running, `parent( x )`
is the label of the set of $x$, the least element; the parallel connected
components of [chapter 25](25-parallel.md#ch-25) use it in this way.

<span id="lem-24-10"></span>**Lemma 24.10.** *On one thread,
`ConcurrentUnionFind` performs $m \ge n$ operations on $n$ elements in
$O( m \log_{2 + m / n} n )$ steps.*

*Proof.* On one thread every compare-and-swap succeeds, since no other
thread changes a parent between the read and the swap. Then `find()` is the
find with path splitting, and `unite()` is two finds followed by a link of
one root under the other, chosen by the indices only, i.e., the linking
that [Tarjan and van Leeuwen 1984](Z-bibliography.md#ref-tarjan1984) call
naive (it ignores the sizes and the ranks); `sameSet()` is two finds, and
each operation thus makes at most two finds and one link. For path
splitting with naive linking, Tarjan and van Leeuwen prove that $m'$ finds
and at most $n - 1$ links on $n$ elements, $m' \ge n$, take
$O( m' \log_{1 + m' / n} n )$ steps. Here $m' \le 2 m$, and for $m \ge n$
we have $1 + m / n \ge 2$, whence $\log( 2 + m / n ) \le \log_2 3 \cdot
\log( 1 + m / n )$ and $\log_{1 + 2m / n} n \le \log_{1 + m / n} n \le
\log_2 3 \cdot \log_{2 + m / n} n$; the bound follows. $\square$

The linking by index has the advantage of canonical labels, at the price of a
somewhat weaker bound, logarithmic rather than nearly constant. We remark that
the randomized linking of [Jayanti and Tarjan
2021](Z-bibliography.md#ref-jayanti2021) hooks by a random order of the
elements instead of their indices. It bounds the expected work on $p$ threads
by $O( m ( \alpha( n , m / ( n p ) ) + \log( n p / m + 1 ) ) )$, but its labels
are not the least elements. Note also that the constructor is not concurrent
with the other operations (`size()` gives the constant $n$), and $n < 2^{31}$,
since the parents are words of 32 bits.

## 24.7 A cuckoo table under locks per region {#sec-24-7}

`ConcurrentCuckooTable` is the cuckoo hashing with buckets of 4 places and tags
of 8 bits of `BucketCuckooTable` ([chapter 7](07-dictionary.md#ch-7)), made
concurrent with one lock per *region* of buckets and, for keys and values that
the processor reads atomically, lookups that take no lock at all. A key of hash
$h$ has two buckets: $b_1$, given by the top bits of $h$, and

$$
b_2 = b_1 \oplus \left( \left\lfloor g( \tau ) / 2^{64 - r} \right\rfloor
\vee 1 \right) , \tag{24.3}
$$

where $\tau$ is the tag of $h$, $g$ a mixing of the tag (the partial-key cuckoo
hashing of [Fan, Andersen and Kaminsky 2013](Z-bibliography.md#ref-fan2013)),
and $R = 2^r$ the buckets of a region. Thus one can compute the other bucket of
a key from its bucket and its tag, and both lie in the same region. The $B$
buckets form $P = \min( 2048 , \max( 1 , B / 256 ) )$ regions of $R = B / P$
buckets each. We give a region one lock, i.e., a counter of versions (on a
separate line), odd while the lock is held (the lock striping of [Herlihy and
Shavit 2008, chapter 13](Z-bibliography.md#ref-herlihy2008), with a stripe per
region instead of a stripe of scattered buckets). A lock is taken by a
compare-and-swap from an even value after a spin on plain loads, with the pause
of the processor and, after 64 rounds, a yield to the system ([Figure
24.8](24-concurrent.md#fig-24-8)).

![Figure 24.8. A `ConcurrentCuckooTable` of 16 buckets in 4 regions of 4 buckets, each with the version $v$ of its lock (odd, as that of the region 1, while a writer holds it); the two buckets $b_1$ and $b_2$ of a key lie in its region, so that an operation on the key takes one lock.](../figures/concurrent-cuckoo.svg){#fig-24-8}

An operation on a key takes one lock, that of its region. The cuckoo path
of an insertion never leaves the region; it is searched in breadth, and its
keys are moved, under that lock only, with no state to check again. By
contrast, [Li et al. 2014](Z-bibliography.md#ref-li2014) search without
locks and lock two buckets per move. Only the doubling, `reserve()`, `clear()`,
`forEach()` and `consistent()` take more locks.

Its public members are

```cpp
// declarations of <ADS/concurrent/ConcurrentCuckooTable.h>
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > >
class ConcurrentCuckooTable
{
 public:
 explicit ConcurrentCuckooTable( std::size_t n = 0 , H h = H() ,
                                 Eq eq = Eq() );
 std::optional< V > find( const K & k ) const;
 bool contains( const K & k ) const;
 bool insert( const K & k , const V & v );
 bool insertOrAssign( const K & k , const V & v );
 template< class F >
 bool update( const K & k , F && f );
 bool erase( const K & k );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 std::size_t rehashes( void ) const;
 std::size_t stashed( void ) const;
 void reserve( std::size_t n );
 void clear( void );
 template< class F >
 void forEach( F && f );
 bool consistent( void ) const;
 static constexpr bool optimistic = refLockFree< K >() && refLockFree< V >();
 };
```

and several threads count words in it as follows, `update()` applying its
function to the value under the lock of the region:

```cpp
ADS::ConcurrentCuckooTable< std::uint64_t , int > C( 1 << 16 );
auto count = [ & ]( std::uint64_t w ) {
 if( ! C.update( w , []( int & c ) { ++c; } ) )
  if( ! C.insert( w , 1 ) )  // inserted meanwhile
   C.update( w , []( int & c ) { ++c; } );
 };
auto work = [ & ]() {
 for( int i = 0 ; i < 1000 ; ++i )
  count( i % 7 );
 };
std::thread a( work ) , b( work );
a.join();
b.join();  // C.find( 0 ) holds 286
```

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

<span id="lem-24-11"></span>**Lemma 24.11.** *The locks of
`ConcurrentCuckooTable` cannot deadlock.*

*Proof.* An operation on a key holds one lock and waits for no other while
it holds it but the mutex of the stash, which is innermost: a thread takes
it holding no lock, the lock of one region or all the locks, and while it
holds it waits for no other lock, so that no cycle of waits goes through
it. The operations that take more than one lock take all those in
use in increasing order, from the stripe 0, and the doubling takes the
locks of the new regions in the same order, before the new table is seen.
Hence a thread that holds the lock $i$ and waits for the lock $j$ has
$i < j$, and a cycle of waits would give
$i_1 < i_2 < \cdots < i_k < i_1$, which is impossible. A thread migrates
parts of a doubling only while it waits for a lock and holds none. A part
takes no lock of a region, but only, for a pair that finds no place in the
new table (rarely), the spin lock of the list of such pairs, which is the
innermost one: a thread takes it holding no other lock, appends one pair
and releases it, and does not wait while it holds it, so that no cycle
of waits goes through it. Hence the thread that doubles, which waits for
the parts claimed, waits for no thread that waits for a lock of a region.
$\square$

Let the keys and the values be trivially copyable, and let
`std::atomic_ref` read and write them without locks (e.g., integers and
pointers; the constant `optimistic` of the class is then true). A lookup is
then a read under the version of the region, i.e., a *seqlock*. It reads
the table $T$ in use, then the even version $v$ of the region of its key in
$T$, then the table in use again, which must still be $T$ (a doubling
changes the table and then the versions), then the tags and the places of
the key, all with atomic loads that acquire, and then the version again. If the
version is still $v$ it returns what it read; otherwise it tries again,
and it takes the lock after 4 tries. A writer makes the version odd by its
compare-and-swap, stores with release, and makes the version even again by
a store with release.

<span id="lem-24-12"></span>**Lemma 24.12.** *A lookup without locks that
reads the same even version $v$ before and after its reads returns a state
of the region of its key, i.e., what a lookup under the lock would have
returned at the moment of its first read of the version.*

*Proof.* We suppose that the lookup read a value stored by a writer $w$ that
held the lock after the version $v$ was written. This writer stored it with
release, after its compare-and-swap of the version from an even value
$v' \ge v$ to $v' + 1$, and the lookup read it with acquire, before its
second load of the version. The store synchronizes with the load, so that
the compare-and-swap happens before the second load of the version, which
therefore reads $v' + 1$ or a later value, i.e., not $v$. [Boehm
2012](Z-bibliography.md#ref-boehm2012) writes the same order by a release
fence after the compare-and-swap and an acquire fence before the second
load, with relaxed accesses between them; we order the accesses themselves
(the sanitizer of threads does not model the fences, and on x86-64 the
accesses remain plain loads and stores). Hence, when
the second load reads $v$, every value read was stored before the release
that made the version $v$, by writers that had all released the lock, and
no writer held it in between; the reads see one state of the region. No
access is a data race, since all of them are atomic. $\square$

A lookup without locks thus costs no read-modify-write and no store to a shared
line. Furthermore, the tables replaced by a doubling are kept until the table
is destroyed, since a reader may still be on one; with $B_0 < \cdots < B_k$
buckets they total $\sum_{j < k} B_j < B_k$, less than the table in use. Other
keys and values (e.g., `std::string`) are read under the lock, and the tables
replaced are freed at once.

We now turn to the insertion, which puts the new key in a free place of its two
buckets if there is one. Otherwise it searches in breadth, from the two
buckets, a path of at most $L = 5$ moves that ends in a bucket with a free
place, and moves the keys of the path from its end, each into its other bucket,
which (24.3) computes from its tag only.

<span id="lem-24-13"></span>**Lemma 24.13.** *A search in breadth over
buckets of $b$ places that looks at $M$ places finds a path of at most*

$$
L = \left\lceil \log_b \left( \frac{M}{2} - \frac{M}{2 b} + 1 \right)
\right\rceil \tag{24.4}
$$

*moves when one exists among them, against the $M / 2$ moves of a search
in depth; for $M = 2000$ and $b = 4$, $L = 5$.*

*Proof.* We count the places of the two trees of the search, rooted at the
two buckets of the key, which have the depth 0. Each place of a bucket
names one other bucket, and the trees therefore have $2 b^l$ buckets, i.e.,
$2 b^{l+1}$ places, at the depth $l$. A path of at most $L$ moves moves the
keys of places at the depths 0 to $L - 1$, each to a bucket of the next
depth, and the places that a search for such a path looks at are therefore
$\sum_{l = 0}^{L - 1} 2 b^{l+1} = \sum_{l = 1}^{L} 2 b^l = 2 b ( b^L - 1 )
/ ( b - 1 )$, which is at least $M$ exactly when $b^L \ge M / 2 - M / ( 2
b ) + 1$, whence (24.4). For
$M = 2000$ and $b = 4$ the argument of the logarithm is 751, and
$\log_4 751 \approx 4.78$. A search in depth follows one path, which may be
as long as half the places it looks at ([Li et al.
2014](Z-bibliography.md#ref-li2014)). $\square$

The search stops at $L = 5$, or at 1024 buckets, all in the region, whose tags
form a block of $8 R$ bytes. We double the table when a region would pass the
load 0.98, or when no path is found. The thread that doubles it takes all the
locks in use, and those of the new regions too before the new table is seen;
then it checks that no other thread has doubled the table in the meantime, and
places each pair again in the new table, while no other thread changes the old
one. The first bucket of a key being the top bits of its hash, the pairs of the
old buckets $[ l , h )$ go to the new buckets $[ 2 l , 2 h )$, which hold whole
regions and receive no other pair. The old table is thus cut in parts of whole
regions, of at least 4096 buckets, and the threads that wait for a lock claim
parts, by a compare-and-swap of a counter that carries the number of the
doubling (so that a thread late on a doubling claims no part of the next), and
migrate them with the thread that doubles: each makes its part of the new table
empty, places its pairs there and counts the pairs of its new regions (the
cooperative migration of [Maier, Sanders and Dementiev
2019](Z-bibliography.md#ref-maier2019), here under the locks). The thread that
doubles waits for all the parts, and only then is the new table seen. With 2
buckets per key and 4 places per bucket, the cuckoo hashing of $n$ random keys
succeeds with high probability while $n / ( 4 B ) < 0.9803$, the threshold of
the 4-orientability of the random graphs ([Cain, Sanders and Wormald
2007](Z-bibliography.md#ref-cain2007)), here in each region. Now, a region of
$4 R$ places holds a binomial number of the $n$ keys, of mean $\mu = n / P$ and
standard deviation $\sqrt{ \mu ( 1 - 1 / P ) }$. This is a relative deviation
of about $1 / \sqrt{ 4 R \alpha }$ at the load $\alpha$ (3.3% for $R = 256$ and
$\alpha = 0.9$), and therefore the fullest region of a large table reaches 0.98
at a mean load of about 0.88 to 0.92. A restriction of the choices to a part of
the table (there, a page) is also studied by [Dietzfelbinger, Mitzenmacher and
Rink 2011](Z-bibliography.md#ref-dietzfelbinger2011). The cost of the doublings
is in [Lemma 24.14](24-concurrent.md#lem-24-14) below. A doubling that would
leave a table of at least 256 places less than 1/16 full is not made, since the
key that finds no place then shares both buckets with 8 others at every size
(e.g., by equal hashes). The key goes instead to a *stash* ([Kirsch et al.
2009](Z-bibliography.md#ref-kirsch2009)), a vector of pairs under a separate
mutex, after the bit of overflow of its first bucket for its tag is set (see
below), as for the stash of `BucketCuckooTable` in
[§7.7](07-dictionary.md#sec-7-7). A pair enters the stash only under all the
locks, and leaves it or changes there under the lock of its region and the
mutex; a lookup reads the stash, under the mutex, only after its two buckets
and only while the stash is not empty, which it is with any but a degenerate
hash (a lookup without locks also only when that bit is set, and within its two
reads of the version). A doubling places the pairs of the stash again with the
others, and those that find no place, as those that the parts of a doubling
cannot place, go to the new stash. The table thus has at most $\max( P_0 , 512
, 32 n )$ places, $P_0$ those of the empty table and $n$ the most pairs that
its buckets have held, since it doubles only when it is fuller than $1 / 16$ or
smaller than 256 places.

Each bucket has 4 bits of *overflow*, in a separate vector of $B / 2$
bytes, i.e., $1 / 8$ of the tags. The bit $j$ of the bucket $b$ is set
when a key with $b$ as its first bucket, and with $j$ in the top 2 bits of
its tag, goes into its other bucket, by an insertion or by a move along a
cuckoo path, and it is cleared only by a doubling or by `clear()` (the
remap entries of the Horton tables of [Breslow et al.
2016](Z-bibliography.md#ref-breslow2016), here reduced to bits). Hence a
key in its other bucket has its bit set in its first one, and a lookup
reads the second bucket only when that bit is set: an absent key costs the
tags of one bucket and a bit from a vector that the cache keeps. The bits
set by keys removed since stay, and make the lookups read the second
bucket in vain until the next doubling.

A lookup without locks that reads, with acquire, the tags of the first
bucket $b_1$ of its key $k$ in the table $T$ (the read $R_1$), finds no
tag of $k$ there, then finds the bit of $b_1$ for $k$ clear (the read
$R_2$, with acquire), and then finds $T$ still in use (the read $R_3$),
returns that $k$ is absent without reading the version of the region. We
argue that no write that places $k$ in a table and happens before $R_1$
escapes it. The writers change $T$ only while it is in use, under the lock
of the region, and $k$ enters its second bucket either by an insertion,
which sets the bit before it stores the tag, or by a move from $b_1$, which
copies it, sets the bit, and then clears its tag in $b_1$ by a store with
release. If the store of a tag of $k$ in $b_1$ happens before $R_1$, then
$R_1$ reads it or a later store of that word, by the coherence of the
accesses to one object, and a later store drops the tag only by a removal
or by a move of $k$. If the bit is set by a write $W$ that happens before
$R_1$, either directly or through a move whose clearing of the tag $R_1$
reads (and therefore acquires), then $W$ happens before $R_2$, which is
sequenced after $R_1$, and $R_2$ reads $W$ or a later store; bits are
cleared only in a table not yet in use, or by `clear()`, which first
clears the tags and thus removes $k$. Finally, a write into a table that
replaces $T$ follows the publication of that table, which it acquires;
were it to happen before $R_1$, the publication would happen before $R_3$,
which would read it or a later table, and not $T$. Hence the lookup can
be ordered at $R_1$, after every write of $k$ that happens before it. Only
the lookups that read places need the version.

A lookup fetches ahead, at once, the tags and the line of the places of the
first bucket, and the misses overlap (the 4 places of a bucket share a line of
cache when their size is a power of 2 up to 64 bytes); an insertion fetches the
tags of the second bucket too, where the key goes if the first is full. The
places of the second bucket are read only when its bit is set and a tag matches
there. Also, the tags and the places of a table of 2 MB or more lie on
transparent huge pages, and the random accesses then miss the TLB once per 2 MB
instead of once per 4 KB ([Navarro et al.
2002](Z-bibliography.md#ref-navarro2002)). With uniform keys two operations
contend for a lock with probability $1 / P$. Each region counts its pairs under
its lock, and `size()` is the sum of $P$ counters read without locks, exact
when no operation runs and otherwise an estimate.

<span id="lem-24-14"></span>**Lemma 24.14.** *The operations of
`ConcurrentCuckooTable` on a single key are linearizable, at an instant at
which they hold the lock of the region of the key in the table in use, or at
the second read of the version for a lookup without locks (at its read of the
tags of the first bucket for one that returns from there an absent key);
`forEach()` visits the pairs of one instant; and if each doubling to $2 B$
buckets follows at least $\epsilon B$ insertions since the previous doubling,
for a constant $\epsilon > 0$, the doublings cost $O( 1 / \epsilon )$ amortized
time per insertion.*

*Proof.* An operation that holds the lock of the region of its key in the
table in use is alone on the two buckets of the key, which lie in that
region by (24.3), and a doubling, which takes all the locks, cannot run
meanwhile; its effect is therefore that of the sequential operation at any
instant within its critical section. An operation that finds, after taking
the lock, that the table was replaced releases it and starts again on the
new table, and it has had no effect. A lookup without locks returns, by
[Lemma 24.12](24-concurrent.md#lem-24-12), the state of the region at its
first read of the version, which no writer changed before the second read;
any instant between the two reads is thus a point, and a lookup that
returns an absent key from the tags of the first bucket is right at their
read, as shown above. `forEach()` holds all
the locks while it visits, so that no operation takes effect meanwhile. A
doubling to $2 B$ buckets places again the at most $4 B$ pairs present, in
$O( B )$ time, which we charge to the at least $\epsilon B$ insertions
since the previous doubling, $O( 1 / \epsilon )$ each; every insertion is
charged by one doubling only. $\square$

The hypothesis of the last claim is not a property of the code but of the
keys: a doubling halves the load, to at most $0.49$, and with random hashes
the fullest region of a large table reaches $0.98$ only at a mean load of
about $0.88$ (see above), i.e., after about $0.39 \cdot 4 B$ insertions,
while keys that share both buckets make the doublings come with no
insertion, until the table is less than $1 / 16$ full and the keys go to
the stash.

Also, besides these operations, `contains( k )` is the
lookup that returns only whether $k$ is there, `insertOrAssign( k , v )`
is the insertion that, for a key found, assigns $v$ to its value before
the unlock, and `rehashes()` counts the doublings of the table.

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

We let a word be 0 when its place is free; it is written once, by a
compare-and-swap from 0, and it is never changed afterwards but by `update()`.
We distinguish the forms in which the pairs are kept, chosen by their types and
by the processor when compiling. (i) When the key and the value are trivially
copyable and fit in 8 bytes together (e.g., a key of 32 bits with a value of 32
bits, or a key of 64 bits with no value, i.e., a set), the word is the pair
itself (the constant `packed` of the class). Pairs whose key is made of zero
bytes, whose word could be 0, are kept apart in one atomic word. (ii) When they
do not fit together but each fits in 8 bytes, and the compare-and-swap of 16
bytes of Boost.Atomic is free of locks (`cmpxchg16b` on x86-64, the pairs of
registers of AArch64, as Boost finds them under the flags of the compiler), a
place is two words, the key and the value, written together by that instruction
(`inlined`); without Boost, the pairs are in the form (iii). (iii) Otherwise
the word holds the index of a node, where the pair lies with its hash, and 24
bits of the hash; a lookup follows the index only when those bits match, which
happens once in $2^{24}$ times for another key. Nodes never move, and the pair
returned is a reference to the node, valid as long as the table; the member
type `Ref` is what an operation returns of a pair, i.e., a copy of the value in
the forms (i) and (ii) and a reference to the node (of the type `Entry`) in the
form (iii). The nodes are cut from segments of $2^{12} , 2^{13} , \ldots$
nodes, of which each thread reserves 64 at a time; an insertion thus takes no
lock, and calls the allocator once per 64 nodes at most.

Here the place of a key of hash $h$ is $\lfloor h / 2^{64 - l} \rfloor$, and
the key is looked for from there on, cyclically, up to its word or to a word 0.
When the pairs lie in the words, each place has besides a *tag* of a byte, 0 or
7 bits of the hash of its key with the high bit set, as the metadata of the
tables of Swiss and F14 kind. Tags are kept apart from the words, 8 to an
atomic word. A lookup reads the tags from the place on, 8 at a time, compares
them all with the tag of the key in one step, reads only the words of the
places whose tag matches, and stops at the first tag 0. An absent key thus
costs one line of the tags, ($1/8$ of the words or less), which the cache
usually keeps. A tag is written, with release, after the word it tags, and
always with the same value, by the insertion that wrote the word or by any
insertion that passes the place while it looks for a place; the insertion takes
effect when the tag of its word is written ([Figure
24.9](24-concurrent.md#fig-24-9)). We give the pairs in nodes no tags, since a
tag would add a third miss to the two of a key found in its node.

![Figure 24.9. The tags (upper row, 0 for a place free or not yet tagged) and the words (lower row) of a `ConcurrentFlatTable` of 12 places, before and after the insertion of a key $k$ whose place is 5: the insertion passes the places 5 to 7, which hold other keys, writes $k$ into the free place 8 by a compare-and-swap, and then its tag.](../figures/concurrent-flat.svg){#fig-24-9}

Its public members, but for the variants that take the hash already
computed (each operation on a key $k$ has one, with the hash $h$ as a
second argument), are

```cpp
// declarations of <ADS/concurrent/ConcurrentFlatTable.h>
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > , class C = ZeroingClear >
class ConcurrentFlatTable
{
 public:
 static constexpr bool packed =
   std::is_trivially_copyable_v< K > && std::is_trivially_copyable_v< V > &&
   plainKey() && ( sizeof( K ) + valueBytes() <= 8 );
 static constexpr bool inlined =
   ( ADS_FLAT_CAS16 != 0 ) && ! packed && std::is_trivially_copyable_v< K > &&
   std::is_trivially_copyable_v< V > && plainKey() && ( sizeof( K ) <= 8 ) &&
   ( sizeof( V ) <= 8 );
 struct Entry
 {
  const K key;
  V value;
  };
 using Ref = std::conditional_t< packed || inlined , V , Entry & >;
 explicit ConcurrentFlatTable( std::size_t n = 0 , H h = H() , Eq eq = Eq() );
 std::uint64_t hash( const K & k ) const;
 std::pair< Ref , bool > findOrInsert( const K & k , const V & v );
 bool insert( const K & k , const V & v );
 std::optional< V > find( const K & k ) const;
 std::optional< K > findKey( const K & k ) const;
 bool contains( const K & k ) const;
 void prefetch( std::uint64_t h ) const;
 template< class F >
 bool update( const K & k , F && f )
  requires( packed || inlined );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 std::size_t rehashes( void ) const;
 void reserve( std::size_t n );
 void clear( void );
 std::uint64_t epoch( void ) const;
 template< class F >
 void forEach( F && f ) const;
 bool consistent( void ) const;
 };
```

besides `findOrInsertKey( k , h )` of a `packed` set and `entry( k , h )`,
the node of a key of a table that is neither `packed` nor `inlined`. The
lookup `findKey( k )` returns a copy of the key that the table holds equal
to $k$ by `Eq`, or no key, and it inserts none; in a set of pointers to
nodes compared by the states they hold, a thread thus reads the pointer
stored for a state equal to that of its node, while the owner inserts,
with no second copy of the pointer in the value. It is the lookup of
`find()`, with its progress and its point of linearization, and it reads
the key of a node as `find()` reads its value.

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

<span id="lem-24-15"></span>**Lemma 24.15.** *Let the words and the tags of
a vector be written only from 0, once each, and only while the vector is in
use, and let an insertion write the tags of the places it passes before it
writes its own. Let a lookup of $k$ in the vector $T$ read the tag 0 at the
place $j$, after tags at the places from that of $k$ to $j - 1$ whose words
hold other keys, and then find $T$ still in use. Then no insertion of $k$
had taken effect when the lookup read the tag of $j$.*

*Proof.* We follow an insertion of $k$, which writes the first word 0 of the
sequence of places of $k$, and before its own tag the tags of all the places it
passed. We suppose that it had taken effect, i.e., written its tag, before the
lookup read the tag of $j$. The place $j$ comes before its word, or is its
word, and it would have had its tag then, since a tag is written once and
stays; but the lookup read 0 there. $\square$

A lookup that finds $k$ returns a pair that stays, and it is linearized at the
read of its word (of its tag, when the places have tags). One that finds the
vector replaced starts over on the new one, which happens once per growth.
Also, the lookups read with acquire. Hence the word of a tag read is read
whole, and so is the pair of a node, built before the compare-and-swap with
release that published its index; $p$ threads that only read therefore scale as
$p$ independent sequential tables (cf. [Table
24.1](24-concurrent.md#tab-24-1)). Note that an insertion that finds $k$ in a
word whose tag is still 0 writes the tag before it answers, and its answer thus
comes after the insertion of $k$ has taken effect. For the pairs in nodes,
which have no tags, the lemma holds with the words in place of the tags, an
insertion taking effect at its compare-and-swap.

We let the table grow when its pairs pass half of its places. Insertions (and
`update()`) exclude a growth, and the growth excludes them, by a protocol
of Dekker over 64 counters on separate lines, as in the growth of [Maier et
al. 2019](Z-bibliography.md#ref-maier2019) that synchronizes the threads. A
thread adds 1 to its counter and then reads the flag $g$ of the growth,
while the thread that grows the table raises $g$ and then reads the
counters (lines 3 and 4, and 19 and 20, of `findOrInsert()` in [Algorithm
24.8](24-concurrent.md#alg-24-8)).

<span id="lem-24-16"></span>**Lemma 24.16.** *Let a writer raise its
counter and then read the flag $g$, and the thread that grows the table
raise $g$ and then read the counters, all these accesses sequentially
consistent; and let a writer that reads $g$ raised lower its counter and
retry after the growth. Then no word of a vector is written once its copy
has begun.*

*Proof.* We place the four accesses in the single order $S$. If the read of $g$
by the writer follows the raise of $g$, the writer backs off. Otherwise the
read of $g$ precedes the raise, and hence the raise of the counter of the
writer precedes the read of the counters by the grower. Then the grower
sees the counter and waits for it to drop, i.e., for the writer to leave the
vector (the mutual exclusion of Dekker, which [Dijkstra
1965](Z-bibliography.md#ref-dijkstra1965) published). Thus the copy starts
only when no writer is inside the old vector, and no writer enters it until
the growth ends. $\square$

The grower allocates a vector twice as large. Meanwhile, the threads that come
to insert copy blocks of 4096 words of the old vector, claimed by a counter,
instead of waiting. Each word is placed at the first free place of its sequence
in the new vector, which no other thread reads yet, and the new vector is
published, with release, once all the blocks are copied. Meanwhile the lookups
never wait: they read the old vector, which no longer changes, until the new
one is in use. We let the counters of the threads also count the pairs, in
their upper 48 bits, and an insertion counts its pair in the same
read-modify-write by which it leaves. Their sum, which reads 64 lines, is taken
once per 1024 insertions of a counter (more often in a vector of less than
$2^{20}$ places). An insertion that reads more than 128 words also grows the
table, if it is at least one eighth full. A table less full than that, which
typically only keys of equal hash make so "crowded", is read on to its first
free place instead.

<span id="lem-24-17"></span>**Lemma 24.17.** *With the load $\alpha$ of the
table at most $1/2$ and random hashes, a lookup reads on average*

$$
\frac{1}{2} \left( 1 + \frac{1}{1 - \alpha} \right) \le 1.5
\quad \text{and} \quad
\frac{1}{2} \left( 1 + \frac{1}{( 1 - \alpha )^2} \right) \le 2.5
\tag{24.5}
$$

*places when it succeeds and when it fails, respectively. The growths that
the load triggers cost $O( 1 )$ amortized time per insertion, at most
$18 / 7$ copies of a word each, provided that the load stays below $9/16$.*

*Proof.* We recall that the two averages are those of linear probing with
random hashes ([Knuth 1998, §6.4](Z-bibliography.md#ref-knuth1998v3)),
increasing in $\alpha$, taken at $\alpha = 1/2$. A growth from $2^l$ places
copies the at most $( 9/16 ) 2^l$ words in use, each once. The previous growth
left a load of at most $( 9/16 ) / 2 = 9/32$ in the vector of $2^l$ places, and
a growth triggered by the load happens only when the pairs pass $2^l / 2$;
hence at least $( 1/2 - 9/32 ) 2^l = ( 7/32 ) 2^l$ insertions lie between the
two, and the copies are at most $( 9/16 ) / ( 7/32 ) = 18/7$ per insertion.
$\square$

We remark that the load may pass $1/2$ before a growth, since each of the 64
counters triggers the sum only once per period of $2^{\min( \max( l - 10 , 0 )
, 10 )}$ of its insertions: between two sums the pairs grow by less than $64
\cdot 2^{l - 10} = 2^l / 16$, and hence $\alpha < 9/16$, which is the proviso
of [Lemma 24.17](24-concurrent.md#lem-24-17), and where the two averages of
(24.5) are below 1.65 and 3.12. A growth for a crowded insertion (more than 128
words read at a load of at least $1/8$) is outside the lemma: with random
hashes it is rare, and with keys of equal hash it may follow the previous
growth with few insertions. Most of the places of (24.5) lie on one line of 8
words, which the lookup fetches while it reads the tags, and a key that is
there typically costs about one miss. The progress of the operations is that of
[Lemma 24.18](24-concurrent.md#lem-24-18) below. Where the pairs are `inlined`,
the copy of a growth moves the two words of a place at once, and a lookup reads
the key with acquire and then the value from the same line. [Lemma
24.15](24-concurrent.md#lem-24-15) then holds unchanged for the key, since
`update()` changes the value by the same instruction and never the key; the
lookup reads a value written with that key, or a later one of `update()`. We
remark that `clear()`, `forEach()` and `consistent()` may not run with the
other operations. Also, `contains( k )` (or `contains( k , h )` with the hash
given) is the lookup that returns only whether $k$ is there, and `rehashes()`
counts the growths of the table. Vectors replaced are kept until the table is
destroyed, and with sizes that double they occupy less than the vector in use.
For a `packed` set (with no value), `findOrInsertKey()` returns the key that
the table holds, which is equal to $k$ by `Eq`, though not necessarily by its
bytes. In a set of pointers to objects compared as the objects, the threads
that insert at once pointers to equal objects therefore all get the pointer of
the one that inserted it; this is the intrusive set of the nodes of a parallel
search.

<span id="lem-24-18"></span>**Lemma 24.18.** *A lookup of
`ConcurrentFlatTable` never waits, and it ends after a number of its steps
bounded by the places of the vector times one plus the growths that
complete while it runs; an insertion waits only for a growth that runs, or
for the renewal of a group by another insertion.*

*Proof.* A lookup reads the vector in use and its words, never a lock or a
counter, and in one vector it reads at most all its places before it finds the
key or a free place (the vector is never full, since it grows at half of its
places). It starts over only when it finds the vector replaced, i.e., after a
growth completed; each start reads at most the places of the vector then in
use. An insertion that finds the flag $g$ raised helps the copy, whose blocks
it claims by a counter, and waits only for the blocks claimed by the other
threads and for the publication of the new vector; outside a growth it runs the
probe of a lookup and one compare-and-swap per free place it tries, which fails
only when another insertion wrote that place, and it then reads on. With the
policy `EpochClear` described next, it may also wait for the store of the epoch
of a group that another insertion renews. $\square$

`clear()` writes 0 to every place, in time linear in the capacity, which a
table cleared often and filled little in between (the restarts of an
incremental search) pays at each clear. With the policy `EpochClear< S >`, the
fifth template parameter (`ZeroingClear`, the default, leaves the table as
above; the concept `FlatClear` admits these two policies only), `clear()`
takes $O( 1 )$ amortized time instead. Each group of 8 places has a stamp of
the unsigned type $S$ (32 bits by default, i.e., half a byte per place), and
the group is in use only if its stamp is the epoch of the table, which
`clear()` raises, so that all the groups fall out of use at once. A lookup
reads the tags of a group, or a word of a table without tags, only if the
stamp of its group is the epoch, and a group out of use is free, where the
lookup stops. An insertion that reaches a group out of use renews it first: it
takes the group by a compare-and-swap of its stamp to a mark, writes 0 to its
words and tags, and stores the epoch with release:

--8<-- "generated/ConcurrentFlatTable-renew.pseudo.md"

The other insertions that reach the group meanwhile wait for that store, a few
words long, while the lookups see the group free until then, which is right,
since no insertion of the new epoch has taken effect in it; once the stamp is
the epoch, the group is a group of a new vector, and [Lemma
24.15](24-concurrent.md#lem-24-15) holds for it unchanged. A renewal runs
within the critical section of its insertion, so that by [Lemma
24.16](24-concurrent.md#lem-24-16) no growth runs with it; the copy of a growth
takes only the groups in use, into a vector whose groups all have the epoch.
The stamps of the old epochs are less than the epoch, and when the epoch would
reach the mark, the largest value of $S$, `clear()` writes 0 to the whole
vector and starts again from the epoch 1: once per $2^{|S|} - 2$ clears, so
that no stamp left from an old epoch can equal a new one. Of course, the pairs
of a table in nodes are then dropped with no destructor, their nodes reused by
the next epoch, and the policy therefore requires keys and values trivially
destructible; `epoch()` gives the epoch (test
`engineered.concurrent-flat-table-epochs`).

## 24.9 Relaxed priority queues {#sec-24-9}

We observe that a priority queue shared by $p$ threads under one lock
serializes them. A *relaxed* priority queue returns instead a pair that need
not be the least: the *rank* of a popped pair is 1 plus the number of the pairs
present with a smaller key, and its *rank error* the rank less 1, which is 0
for an exact queue. A search that takes its states from a relaxed OPEN expands
a few states that an exact one would not, and in exchange its threads do not
wait for one another. `MultiQueue` is the MultiQueue of [Rihani, Sanders and
Dementiev 2015](Z-bibliography.md#ref-rihani2015), with the stickiness and the
buffers of [Williams, Sanders and Dementiev
2021](Z-bibliography.md#ref-williams2021), on pairs $( k , x )$ of a key (an
integer or a real, the least first by `less`) and an item.

In it, the $n$ queues are sequential priority queues, each with a separate lock
and with the key of its minimum, $\mathit{top}_j$, kept apart in an atomic
word that the other threads read without the lock. A push puts the pair in
a random queue whose lock it takes at the first try, and draws another one
when the lock is held. A pop reads the minima of two distinct random
queues, locks the queue with the smaller one if it is free (draws two other
queues otherwise), and takes its minimum
([Figure 24.10](24-concurrent.md#fig-24-10)). With $n = 1$ the queue is
exact, a priority queue under one lock, which is typically used for testing
and for comparison.

<span id="lem-24-19"></span>**Lemma 24.19.** *Let there be $n = q p$ queues,
$q > 1$, used by $p$ threads, and let each draw of a queue be uniform and
independent of the locks held. Then a push takes a lock after fewer than
$q / ( q - 1 )$ draws in expectation (2 for $q = 2$, i.e., the $n = 2 p$
queues that the header suggests), and no thread waits for a lock.*

*Proof.* A thread that draws holds no lock, and each of the other $p - 1$
threads holds at most one; hence at most $p - 1$ of the $n$ queues are
held at each draw, and a draw finds a held lock with probability at most
$( p - 1 ) / ( q p ) < 1 / q$. The number of draws up to the first free
lock is thus dominated by a geometric variable of success probability
$1 - 1 / q$, whose mean is $q / ( q - 1 )$. A held lock is never waited
for: the thread draws again instead. $\square$

The independence of the draws from the locks is that of the generator of
each handle, which no other thread reads; a pop draws two queues per
round and takes a lock with at least the same probability, since it fails
only if the better queue is held.

![Figure 24.10. A `MultiQueue` of four queues, each with its minimum $\mathit{top}$, read without locks, and its deletion buffer $D$ (the least pair on the right); a pop that draws the queues 1 and 3 takes the key 4 from the better one, while the keys 2 and 3 are in the other queues: its rank error is 2.](../figures/concurrent-multiqueue.svg){#fig-24-10}

Its public members, and those of the handle of a thread, are

```cpp
// declarations of <ADS/concurrent/MultiQueue.h>
template< class K , class T , class Less = std::less< K > ,
          template< class , class > class H = MultiQueueHeap >
class MultiQueue
{
 public:
 struct Entry
 {
  K key;
  T item;
  };
 class Handle
 {
  public:
  void push( K k , T x );
  template< class It >
  void pushAll( It first , It last );
  std::optional< Entry > tryPop( void );
  };
 explicit MultiQueue( std::size_t n , unsigned s = 16 ,
                      std::uint64_t seed = 1 ,
                      Less less = Less() );
 Handle handle( void );
 std::size_t queues( void ) const;
 unsigned stickiness( void ) const;
 std::size_t size( void ) const;
 bool empty( void ) const;
 bool consistent( void ) const;
 };
```

where $s$ is the stickiness and `tryPop()` answers nothing only when the
queues it looked at were empty; the constant `decrease_key` of the class
declares, in the vocabulary of [§1.14](01-introduction.md#sec-1-14), that
a key is lowered lazily, by a new pair. Each thread takes a handle:

```cpp
ADS::MultiQueue< int , int > M( 8 );  // n = 8 queues
std::vector< std::thread > T;
for( int k = 0 ; k < 4 ; ++k )
 T.emplace_back( [ & , k ]() {
  auto h = M.handle();
  for( int i = 0 ; i < 100 ; ++i )
   h.push( 4 * i + k , i );
  while( auto e = h.tryPop() ) {
   }  // small keys first, roughly
  } );
for( auto & t : T )
 t.join();
```

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
the $n$ with a probability $\sigma_j$; we write $\sigma_{\le i} =
\sigma_1 + \cdots + \sigma_i$.

<span id="thm-24-1"></span>**Theorem 24.1.** *([Walzer and Williams
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

The earlier bounds of [Alistarh et al.
2017](Z-bibliography.md#ref-alistarh2017), $O( n )$ for the mean and $O( n \log
n )$ for the maximum, hold for pushes and pops mixed as well, but for a
comparison and a pop done atomically. As [Williams et al.
2021](Z-bibliography.md#ref-williams2021) do, `MultiQueue` reads the least keys
of the two queues without their locks, possibly stale. In a run of many threads
its rank errors are therefore measured rather than bounded, and the test
`engineered_queues.multi-queue-rank` compares the measured mean with (24.8) on
one thread.

We adopt two devices of [Williams et al.
2021](Z-bibliography.md#ref-williams2021), which save most of the misses of the
cache. A thread keeps the two queues it drew for $s$ operations (the
*stickiness*), and their lines stay in its cache; it draws two others after $s$
operations or when a lock is held. A push goes to one of the two at random, a
pop to the better one. Also, each queue is an 8-ary heap behind two buffers of
16 pairs. We keep the deletion buffer $D$ sorted by decreasing keys; it holds
the least pairs of the queue, and a pop takes its pair from its end without
moving the others. The insertion buffer $I$ is unsorted; a push fills it, and
it goes into the heap only when it is full or when $D$ runs out.

<span id="lem-24-20"></span>**Lemma 24.20.** *In each queue, let $D$ be empty
only if the queue is, and let no pair of $I$ or of the heap $H$ be less than a
pair of $D$. Then the least pair of $D$ is the least of the queue, and the push
and the pop of [Algorithm 24.9](24-concurrent.md#alg-24-9) keep both
properties, at a cost of $O( \log m )$ amortized time per operation on a queue
of $m$ pairs.*

*Proof.* We note that the first statement is the second property. A push of a
pair not greater than the greatest of $D$ puts it into $D$. Then the greatest
of a full $D$ moves into $I$ (with $I$ into $H$ when $I$ is full), and it is
not less than the rest of $D$. A greater pair goes into $I$, or into $D$ when
$I$ and $H$ are empty, and the pairs of a full $I$ go into $H$; neither move
changes the two properties. A pop takes the least of $D$ and, if $D$ becomes
empty, moves $I$ into $H$ and the least $\min( 16 , |H| )$ pairs of $H$ into
$D$, which are not greater than the rest of $H$; $D$ is then empty only if $H$
is. As to the cost, a push or a pop changes $D$ or $I$ in $O( b )$ time, $b =
16$; a full $I$ goes into $H$ by $b$ pushes, in $O( b \log m )$ time, at most
once per $b$ pairs put into $I$, and $D$ is refilled by at most $b$ pops of
$H$, in $O( b \log m )$ time, at most once per $b$ pairs taken from $D$, but
for the refills of fewer than $b$ pairs, which happen only when $H$ empties and
cost $O( |H| \log m )$, charged to the pairs that entered $H$. $\square$

Each queue is therefore exact, the relaxation is only in the choice of the
queue, and [Theorem 24.1](24-concurrent.md#thm-24-1) applies unchanged. Its
heap is touched by batches of 16 pairs, and the lines of $D$ and $I$ usually
stay in the cache of the thread. The lemma asks of $H$ only an exact queue, and
`MultiQueue` takes it as its template parameter `H`, given the type of the
pairs and their order, by default `MultiQueueHeap`, the 8-ary heap: with the
keys of `packFG()`, the pairs $( f , g )$ of a best-first search with integer
costs, $H$ can be the bucket queue of `BucketHeap` (cf.
[§9.9](09-priority.md#sec-9-9)), whose pushes and pops take $O( 1 )$ amortized
time with bounded keys, so that the cost of an operation no longer grows with
$m$ (test `engineered_queues.multi-queue-buckets`). The Multi Bucket Queue of
[Zhang, Posluns and Jeffrey 2024](Z-bibliography.md#ref-zhang2024) shares the
buckets among the threads, each bucket a separate MultiQueue; here each queue
keeps its buckets under its lock, and the choice of the queue is the one of
[Theorem 24.1](24-concurrent.md#thm-24-1). We know of no theory of the
stickiness yet: the measures of [Williams et al.
2021](Z-bibliography.md#ref-williams2021) show rank errors that grow about
linearly in $s$, which is why $s$ is a parameter of the constructor (16 by
default, given by `stickiness()`); the bounds above are those of $s = 1$.
Furthermore, `pushAll()` puts a range of pairs into one queue under one lock. A
thread that makes several pairs at once (e.g., the successors of a node) thus
takes one lock instead of one per pair, at the price of a relaxation like that
of a stickiness of the number of the pairs. A pop that finds both queues empty
scans all the queues, each under its lock. When no push runs at the same time,
a scan that finds no pair proves the structure empty, which a parallel search
needs in order to stop. Its locks are test-and-test-and-set on a byte, taken by
an exchange after a plain read; a pop holds one lock, a push one, and the scan
one at a time, so that no deadlock is possible. Each operation of a single
queue is linearizable at its lock, while the structure as a whole is relaxed,
i.e., it is not a linearizable priority queue but for $n = 1$. The greatest key
under `less` (the infinity of a real) stands for an empty queue in the atomic
minima, and a pair with that key is popped only by the scan.

## 24.10 Waiting until a condition holds {#sec-24-10}

A thread that waits for an item could look at the structure again and again,
which wastes a core, or sleep for fixed lengths, which delays the item. The
*event count* of [Reed and Kanodia 1979](Z-bibliography.md#ref-reed1979) lets
it sleep until another thread signals that the condition may have become true,
with no lock and with nearly no cost for the signal when nobody sleeps.
`EventCount` holds an epoch $e$ and the number $w$ of the waiters, the two
halves of one word of 64 bits (on a separate line). It also holds a copy $E$ of
the epoch of 32 bits, the size of a futex, on which the waiters sleep. A waiter
announces itself and reads the epoch in one read-modify-write (`prepareWait()`,
which adds 1 to $w$ and returns $e$, of the member type `Key`), and looks at
the condition once more. If the condition is still false, it sleeps while $E =
e$ (`commitWait( e )`, by `std::atomic::wait`); otherwise it withdraws
(`cancelWait( e )`). A notifier makes the condition true, and then `notify()`
reads the word after a fence. Only if there are waiters does it advance the
epoch and set $w$ to 0, in one compare-and-swap, and wake them ([Figure
24.11](24-concurrent.md#fig-24-11)). Later notifications, while the woken
threads are still on their way, find no thread to wake and cost no system call.

![Figure 24.11. The word of the epoch $e$ and of the waiters $w$ of an `EventCount`, and the copy $E$ of the epoch, after `prepareWait()` of a waiter, which then sleeps while $E = 5$, and after `notify()`, which advances the epoch, withdraws the waiter and wakes it.](../figures/concurrent-eventcount.svg){#fig-24-11}

Its public members are

```cpp
// declarations of <ADS/concurrent/EventCount.h>
class EventCount
{
 public:
 using Key = std::uint32_t;
 Key prepareWait( void ) noexcept;
 void cancelWait( Key k ) noexcept;
 void commitWait( Key k ) noexcept;
 void notify( void ) noexcept;
 template< class P >
 void await( P && p , std::int64_t spin = 20000 );
 std::uint32_t waiters( void ) const noexcept;
 };
```

and a thread waits for a flag that another one raises as follows:

```cpp
ADS::EventCount E;
std::atomic< bool > ready{ false };
std::thread w( [ & ]() {
 E.await( [ & ]() { return( ready.load() ); } );  // spins, then sleeps
 } );
ready.store( true );  // the condition first,
E.notify();           // then the notification
w.join();
```

<span id="alg-24-10"></span>**Algorithm 24.10.** The operations of
`EventCount` (after [Reed and Kanodia 1979](Z-bibliography.md#ref-reed1979));
$( e , w )$ is the word of the epoch and of the waiters, and $E$ the copy
of the epoch.

--8<-- "generated/EventCount-prepareWait.pseudo.md"

--8<-- "generated/EventCount-cancelWait.pseudo.md"

--8<-- "generated/EventCount-commitWait.pseudo.md"

--8<-- "generated/EventCount-notify.pseudo.md"

--8<-- "generated/EventCount-await.pseudo.md"

The notifier and the waiter each run a full fence of the standard. When no
thread waits, a notification thus costs the fence and the read of a line
that stays shared. With `ADS_EVENTCOUNT_RMW` defined, the notifier reads
the waiters by a read-modify-write that adds 0 instead, and neither side
runs a fence: the read-modify-write reads the last value in the order of the
modifications of the word, so that it either sees the waiter or is seen by
it through the release sequence; this is also the form that the sanitizer
of threads checks, since it does not model the fences, and the one that
the header takes when the build defines `ADS_SANITIZE_THREAD`, as CMake
and the makefile do when the options of the compiler turn the sanitizer
on. Which of the two
costs less depends on the processor (a full fence against a line taken
exclusive at each notification), and we leave the choice of the default to
the campaign of [Appendix B](B-benchmarks.md#app-B). It is worth noting that
the asymmetric fences of the system (`membarrier( 2 )` on Linux, [Ladan-Mozes,
Lee and Vyukov 2011](Z-bibliography.md#ref-ladanmozes2011)), which would let
the notifier run only a fence of the compiler, have no portable form.

<span id="lem-24-21"></span>**Lemma 24.21.** *Let the read-modify-write of
the waiter be followed by a full fence, and the write that makes the
condition true be followed by a full fence before the notifier reads the
waiters. Then a waiter does not sleep through a notification of a
condition that is true.*

*Proof.* We place the two fences in $S$. If that of the notifier comes first,
the waiter, which looks at the condition after its fence, sees it true and does
not sleep. If that of the waiter comes first, the notifier reads its
announcement, $w \ge 1$, and advances the epoch by a release on the same word;
then either the waiter has not yet compared $E$ with $e$, and finds it changed,
or it sleeps on $E = e$ and is woken by the notification that follows the
change. A "spurious" wake-up only makes the waiter look again. $\square$

We need one more detail to keep the lemma true with several notifiers. Two
notifiers that advanced the epoch of the word to $e - 1$ and to $e$ may reach
$E$ in the other order. A store of $e - 1$ after $e$ would then put back the
epoch of a waiter that the second notifier withdrew, and that waiter would
sleep with the condition true and no thread to wake it. Hence the copy is only
raised, by a compare-and-swap with release that compares the epochs modulo
$2^{32}$: the epoch $e$ is ahead of the copy $E$ when $0 < ( e - E ) \bmod
2^{32} < 2^{31}$. Under the sanitizer of threads, which does not model the
fences, the notifier reads the word by a read-modify-write instead, which reads
the last value in the order of its modifications.

Finally, `await( p )` packs the loop of [Algorithm
24.10](24-concurrent.md#alg-24-10). It returns when `p()` is true, $p$ being
called by the waiting thread only (which may thus take the item in $p$). It
first looks again and again for 20 $\mu$s (the parameter `spin`, in
nanoseconds), with a pause of the processor between the looks and a read of the
clock every 64 looks; then it waits by `prepareWait()`, `cancelWait()` and
`commitWait()` as above. Also, `waiters()` gives the number $w$ of the waiters
announced, a relaxed read. An item that comes within that time costs no sleep
and no wake-up, each of which takes about the time of a system call and of a
switch of threads. We use it in the queues of
[§24.3](24-concurrent.md#sec-24-3) and [§24.4](24-concurrent.md#sec-24-4) for
their operations that sleep (`waitPop()`, `waitTake()`).

Spinning is not "free" either, since the loads and the failed compare-and-swaps
of the threads that spin on one line keep moving it between their caches.
`Backoff` is the exponential backoff of [Anderson
1990](Z-bibliography.md#ref-anderson1990) (see also [Mellor-Crummey and Scott
1991](Z-bibliography.md#ref-mellorcrummey1991)). Its call $k$ from the last
`reset()` runs $2^k$ pauses of the processor (`pause` on x86, `yield` on
AArch64) while $k \le S$, and from then on it yields the processor to the
system. Note that `spinning()` is true when the next call still spins; a waiter
can then move on to sleep, e.g., on an `EventCount`, once it yields. With $S =
6$ (the default; the constructor requires $0 \le S \le 16$) the spinning lasts
$2^7 - 1 = 127$ pauses, a few microseconds, and `step()` gives the number $k$
of the calls since the last `reset()` (up to $S + 1$).

Its public members are

```cpp
// declarations of <ADS/concurrent/Backoff.h>
class Backoff
{
 public:
 explicit Backoff( int S = 6 );
 void pause( void ) noexcept;
 void reset( void ) noexcept;
 bool spinning( void ) const noexcept;
 int step( void ) const noexcept;
 };
```

--8<-- "generated/Backoff-pause.pseudo.md"

<span id="lem-24-22"></span>**Lemma 24.22.** *A thread that looks at a
condition before each call of `pause()` and once after the last one, from
a `reset()`, makes its look $j \ge 1$ after $2^{j - 1} - 1$ pauses, for
$j \le S + 2$; hence a condition that becomes true after
$0 < t \le 2^{S + 1} - 1$ pauses is seen after fewer than $2 t + 1$ pauses,
with $O( \log t )$ looks.*

*Proof.* The look $j$ follows the calls $0 , \ldots , j - 2$, which run
$1 + 2 + \cdots + 2^{j - 2} = 2^{j - 1} - 1$ pauses while $j - 2 \le S$.
If the condition becomes true after $t > 0$ pauses, the look 1, after no
pause, sees it false; let the last look that sees it false be the look
$j$, after $T = 2^{j - 1} - 1 < t$ pauses. Since
$t \le 2^{S + 1} - 1$, we have $j \le S + 1$, and the next look comes
after $2^j - 1 = 2 T + 1 < 2 t + 1$ pauses; the looks are $j + 1 \le
\log_2 ( t + 1 ) + 2$, i.e., $O( \log t )$. $\square$

The looks of $p$ threads at one line thus thin out geometrically, while the
delay added to a wait is less than the wait (plus one pause), and a
wait longer than $2^{S + 1}$ pauses gives the processor back to the system.
The test `engineered_search.backoff-threads` checks the steps and four
threads that wait with it for a flag.

## 24.11 Priority queues that are stolen {#sec-24-11}

A best-first search on $p$ workers (e.g., a branch-and-bound by bounds, or an
A* search) may keep its OPEN in one queue shared by all, exact under a lock or
relaxed as the MultiQueue of [§24.9](24-concurrent.md#sec-24-9). Alternatively,
it may keep a queue per worker, from which the idle workers steal. We adopt the
second choice, which keeps the order of each worker exact. A search needs this
when it counts the nodes of each worker against a limit, or when it wants the
same run from the same steals. It loses the global order, however, and a worker
may expand a node worse than the best of another worker; the steals (hopefully)
keep this loss "small" when they move "good" nodes. `StealablePriorityQueue` is
such a queue, and `StealingFrontier` holds $p$ of them with the detection of
the end of the search. Its design has to settle how many pairs a thief takes,
which ones, and at what price for the owner.

As to how many, a thief takes half of the pairs, as in the queues of [Hendler
and Shavit 2002](Z-bibliography.md#ref-hendler2002) and in the priority queues
stolen by [Wimmer et al. 2014](Z-bibliography.md#ref-wimmer2014). A steal of
one pair usually sends the thief back at once, while a steal of half spreads
the pairs made by one worker over $p$ workers in about $\log_2 p$ rounds of
steals. The Stealing Multi-Queue of [Postnikova et al.
2022](Z-bibliography.md#ref-postnikova2022) steals instead a batch of a few of
the best tasks. Its owner sets them apart in a buffer and skips them in the
meantime, and its order is thus relaxed by that buffer.

As to which ones, the "better" half sends the victim, which made those pairs,
to "worse" ones, and the next steal brings the good pairs back the other way.
In turn, the worse half gives the thief the pairs that a best-first search
expands last, and that a bound improved in the meantime is likely to prune. An
"interleaved" half, instead, gives each worker a share of the best pairs as of
the others. This is the reason behind several balancing rules of the parallel
branch-and-bound. That of [Tschöke, Lüling and Monien
1995](Z-bibliography.md#ref-tschoke1995) is best when each of the $p$
processors holds one of the $p$ subproblems of least bound, and it balances the
least bounds of the heaps besides their sizes. Also, [Karp and Zhang
1993](Z-bibliography.md#ref-karp1993) send each child to a random processor,
and the randomized priority queue of [Sanders
1998](Z-bibliography.md#ref-sanders1998) spreads its items at random over the
processors.

We therefore let the queue deal its pairs to two 4-ary heaps $H_0$ and $H_1$ of
its owner, which no other thread reads. A push goes into the smaller heap,
and into each in turn when they are as large. Hence the pairs pushed one
after the other (e.g., the children of a node) go to both heaps, and either
heap holds, for each range of keys, about half of the pairs in it. A pop
takes the better of the two least pairs. A thief takes the heap whose least
pair is not the least of the queue, and the owner thus keeps its best pair
and its next pop ([Figure 24.12](24-concurrent.md#fig-24-12)).

![Figure 24.12. A `StealablePriorityQueue` whose owner pushed the keys 1 to 8, dealt to its two heaps (in the order of a 4-ary heap); `serve()` moves into the offer the heap whose least key 2 is not the least of the queue, and a thief claims it by an exchange and deals its pairs to two heaps of the thief.](../figures/concurrent-spq.svg){#fig-24-12}

As to the price, a queue whose pairs a thief may take at any time makes its
owner race the thieves at each pop. By the laws of order of [Attiya et al.
2011](Z-bibliography.md#ref-attiya2011), a linearizable pop of such an object
needs a read-modify-write or a fence, as `take()` of the deque of
[§24.5](24-concurrent.md#sec-24-5) does. Here, instead, a thief raises the flag
`hungry` of the queue, which the owner reads with no order at its next push or
pop (or at `serve()`). Its owner answers by moving a heap into the *offer*, an
atomic pointer published with release, which a thief claims by an exchange with
null; these are the receiver-initiated steals of the private deques of [Acar,
Charguéraud and Rainey 2013](Z-bibliography.md#ref-acar2013). Thus the common
case of the owner costs no read-modify-write and no fence. A thief, in
exchange, waits (hopefully briefly) for the next operation of the owner, as
long as a `Backoff` spins, before it tries another victim.

Its public members are

```cpp
// declarations of <ADS/concurrent/StealablePriorityQueue.h>
template< class K , class T , class Less = std::less< K > , class Bd = void ,
          class BdLess = std::less<> >
class StealablePriorityQueue
{
 public:
 static constexpr bool bounded = ! std::is_void_v< Bd >;
 using Bound = std::conditional_t< bounded , Bd , detail::SpqNoBound >;
 using Entry = detail::SpqEntry< K , T , Bd >;  // key, (bound,) item
 explicit StealablePriorityQueue( Less less = Less() , BdLess bl = BdLess() );
 void push( K k , T x )
  requires( ! bounded );
 void push( K k , Bound b , T x )
  requires bounded;
 std::optional< Bound > minBound( void ) const
  requires bounded;
 std::optional< Entry > pop( void );               // the owner
 const Entry & top( void );                        // the owner
 void serve( void );                               // the owner
 std::size_t steal( StealablePriorityQueue & v );  // a thief, into v
 template< class F >
 std::size_t drain( F && f );
 std::size_t size( void ) const;
 bool empty( void ) const;
 bool stealable( void ) const;
 bool offering( void ) const;
 std::pair< std::size_t , std::size_t > halves( void ) const;
 bool consistent( void ) const;
 };
```

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

<span id="lem-24-23"></span>**Lemma 24.23.** *Let the owner take the offer
back, by an exchange with null, before a pop whose two heaps are empty or
whose least key is greater than $k_O$. Then each pop of the owner returns a
pair of least key among those that the owner holds at the moment of the
pop, the pairs of an offer not yet claimed included.*

*Proof.* We first note that the heaps of the owner change only by its
operations. Now, the offer, once published, does not change until one exchange
takes it, by a thief or by the owner and never by both, since the exchanges on
one word are totally ordered and only the first one finds the pointer. If the
least local key is not greater than $k_O$, the local pair is a least pair of
all, whether a thief claims the offer before the pop or not. Otherwise the
owner exchanges; either it gets the offer back and pops from the pairs joined,
or a thief had claimed it before, and the pairs were no longer of the owner
when it popped. $\square$

Each pop is thus exact at the moment of its exchange, or of its last change
of the heaps. Otherwise, the only pairs that leave the owner are those of an
offer, a choice of the owner that keeps its least pair.

<span id="lem-24-24"></span>**Lemma 24.24.** *For an owner that holds $m$
pairs, a push and a pop cost $O( \log m )$ time and a load of the flag
`hungry`, besides the answer to a thief that they may give; the answer
costs $O( 1 )$ time, or $O( m )$ when the heap $j$ to offer is unbalanced
($5 | H_j | < m$ or $5 | H_j | > 4 m$) and the owner first deals the $m$
pairs again; and a steal of $s$
pairs costs the thief one exchange, a store of the flag `hungry`, and
$O( s )$ time.*

*Proof.* Each heap holds at most $m$ pairs, and a push or a pop sifts one
pair along a path of a 4-ary heap, of length $O( \log_4 m )$; the choice of
the heap and the reading of the flag are $O( 1 )$. A deal puts the $m$
pairs in turn into two vectors and makes each a heap by the construction
of Floyd, which costs time linear in its size. Answering a thief is the
store of a pointer to a heap with release, and the heap moves with its
pointer, with no copy of a pair. A thief raises the flag by a relaxed
store, claims the offer by one exchange and, if it got it, moves the heap
into a heap of the thief, with no copy, and deals its $s$ pairs to its two
heaps, again
by Floyd's construction. $\square$

`StealingFrontier` holds the queues of $p$ workers, one per call $f( i )$ of
the `run( p , f )` of an executor (see [chapter 25](25-parallel.md#ch-25)). Its
`pop( i )` returns the least pair of the queue of the worker $i$. When that
queue is empty, it returns the least pair of what it steals from victims drawn
at random; it returns no pair only when no worker holds one, or after `stop()`
(`terminated()` tells whether no worker holds a pair, i.e., $C = 0$, and
`stopped()` whether `stop()` was called). The owner of a
`StealablePriorityQueue` learns from `offering()` whether one of its offers
waits for a thief.

Its public members are

```cpp
// declarations of <ADS/concurrent/StealablePriorityQueue.h>
template< class K , class T , class Less = std::less< K > , class Bd = void ,
          class BdLess = std::less<> >
class StealingFrontier
{
 public:
 using Queue = StealablePriorityQueue< K , T , Less , Bd , BdLess >;
 using Entry = typename Queue::Entry;
 using Bound = typename Queue::Bound;
 explicit StealingFrontier( std::size_t p , std::uint64_t seed = 1 ,
                            Less less = Less() , BdLess bl = BdLess() );
 std::size_t workers( void ) const;
 Queue & queue( std::size_t i );
 void push( std::size_t i , K k , T x )
  requires( ! Queue::bounded );
 void push( std::size_t i , K k , Bound b , T x )
  requires Queue::bounded;
 std::optional< Bound > minBound( std::size_t i ) const
  requires Queue::bounded;
 template< class F >
 std::size_t drain( std::size_t i , F && f );
 void serve( std::size_t i );
 std::optional< Entry > pop( std::size_t i );
 void stop( void );
 bool stopped( void ) const;
 bool terminated( void ) const;
 std::size_t steals( void ) const;
 template< class E , class F >
 void run( E & ex , F && f );
 template< class F >
 std::size_t leftovers( F && f );
 };
```

and $p$ workers of the pool of [§25.3](25-parallel.md#sec-25-3) run a
search on it as follows, each worker $i$ popping from its queue (or
stealing, inside `pop( i )`) until the frontier is empty everywhere:

```cpp
#include "ADS/parallel/ThreadPool.h"

ADS::ThreadPool pool( 4 );
ADS::StealingFrontier< int , int > F( 4 );
F.push( 0 , 0 , 1 );  // the root, key 0, at worker 0
std::atomic< long > nodes{ 0 };
F.run( pool , [ & ]( std::size_t i ) {
 while( auto e = F.pop( i ) ) {  // nothing once terminated
  ++nodes;
  if( e->item < 1000 )  // two children
   for( int c : { 2 * e->item , 2 * e->item + 1 } )
    F.push( i , e->key + 1 , c );
  }
 } );  // nodes is 1999
```

<span id="alg-24-12"></span>**Algorithm 24.12.** The pop of a worker of
`StealingFrontier`; $C$ is the counter of the workers at work and of the
thieves trying, $p$ at the start.

--8<-- "generated/StealablePriorityQueue-StealingFrontier-pop.pseudo.md"

<span id="lem-24-25"></span>**Lemma 24.25.** *In `StealingFrontier`, let
the counter $C$ start at $p$, and let a worker subtract 1 when its pop finds
its heaps and its offer empty. Let a thief add 1 before the exchange that
claims an offer and subtract 1 if the exchange fails, and let only a worker
that holds a pair (or the program before the run) push. Then no pair exists
when $C = 0$, and none exists afterwards.*

*Proof.* We observe that a pair lies in the heaps or in the offer of a worker
that has not left $C$, since a worker leaves only when it holds no pair; or it
lies in the offer claimed by a thief, which entered $C$ before its exchange.
The victim that finds its offer claimed has read the null of the exchange of
the thief, and the two exchanges synchronize (a release read by an acquire).
Hence the entry of the thief happens before the exit of the victim, and $C$,
whose modifications are totally ordered, cannot reach 0 in between; hence, a
pair in transit is always counted. When $C = 0$ no pair exists, and no pair can
be created, since only a worker that holds one pushes. Thieves that still try
enter and leave $C$ with no pair to find, and see $C = 0$ again. $\square$

When we give a type of bounds, each pair carries a bound besides its key, e.g.,
the key of selection of a branch-and-bound by depth first or by best estimate,
and the bound by which it prunes. Then `minBound()` gives the least bound
$\beta$ of the pairs of the owner, and when $\beta$ cannot improve the
incumbent beyond the tolerance, `drain()` drops the entire queue. Each heap
keeps its least bound as long as no pop takes a pair of that bound (a push
lowers it, if its bound is less), and finds it again by a scan of its pairs
when one does. With bounds independent of the keys, a pop takes the least of
$m$ bounds with probability about $1 / m$, and the scans cost $O( 1 )$ expected
per pop. With bounds that go with the keys, as in a search by best first, each
pop takes it, and each scan costs $\Theta( m )$. Hence, when the pairs scanned
exceed 16 times the pops and the pairs held, plus $2^{16}$ for the start, on a
heap of 64 pairs at least, the heap keeps instead, in a vector apart, the least
bound $\mu( i )$ of the pairs of the subtree of each node $i$:

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
toward the root. A pop moves the last pair $z$ down from the root along a
path $P$. A node off $P$ on the path of the old last leaf lost $z$ only; it
keeps its $\mu$ if $\mu$ is less than the bound of $z$, and so do all those
above it off $P$. The rest of that path, and then $P$ from its end up, get
(24.9) again.

<span id="lem-24-26"></span>**Lemma 24.26.** *A heap of $m$ pairs that keeps
its least bound by scans spends $O( c )$ expected time on the scans per pop
when the pair popped carries the least bound with probability at most
$c / m$. A heap that keeps $\mu$ satisfies (24.9) at every node after each
push and each pop, which cost $O( \log m )$ time, and $\mu( 0 )$ is its least
bound.*

*Proof.* A scan reads the $m$ pairs, and it runs only when the pop takes a
pair of the least bound, whence $O( m \cdot c / m )$ expected per pop. By
induction on the depth, (24.9) at every node makes $\mu( i )$ the least
bound of the subtree of $i$, and $\mu( 0 )$ that of the heap. A push
changes the pairs only on the path from the new leaf to the place $i$ of
the new pair: the nodes below $i$ on that path hold the pairs of their
parents, and recomputing (24.9) from the leaf up restores it there, since
their children are already right; above $i$, each subtree gained the bound
$b$ only, and $\mu$ becomes $\min\{ \mu , b \}$, which stops changing at
the first node where $\mu \le b$, since $\mu$ does not increase toward the
root. A pop removes the last leaf, whose pair $z$ moves down from the root
along a path $P$; off $P$, only the nodes on the path of the old last leaf
lost a pair, $z$, and their $\mu$ changes only if it was the bound of $z$,
in which case recomputing (24.9) from below restores it; the nodes of $P$
are recomputed from the end of $P$ up. Each recomputation reads at most 5
values, along $O( 1 )$ paths of length $O( \log_4 m )$. $\square$

A read of $\mu( 0 )$ thus costs $O( 1 )$.

We remark that two alternatives cost more on both kinds of bounds. We discard
both. The first keeps the bounds of the pushes and those of the pops in two
multisets, the least bound being the least of the first once their equal tops
are taken out; it adds a push into one of them per operation, on heaps that
grow with the pops until they are made again. The second keeps a heap by the
bounds, indexed by the places of the pairs, and it adds a second sift per
operation and an update of the index at each move of a pair.

--8<-- "generated/StealablePriorityQueue-minBound.pseudo.md"

<span id="lem-24-27"></span>**Lemma 24.27.** *Let the owner keep, when it
makes an offer, the least bound $b_O$ of the heap that it moves into the
offer. Let `minBound()` return the least of the least bounds of its heaps
that are not empty and, if the offer that it made is still there at an
acquire load of the pointer, of $b_O$. Then the value returned is the least
bound of the pairs that the owner holds at the moment of that load.*

*Proof.* The heaps of the owner change only by its operations, and the least
bound of each is exact. In fact, the one known stays the least while no pop
takes a pair of that bound, a push lowers it when its bound is less, a scan
computes it, and $\mu( 0 )$ is it by (24.9). The pairs of an offer are fixed
from its publication until an exchange takes it, and $b_O$ is their least
bound. At the load, the offer is either still there, and then its pairs are of
the owner (cf. [Lemma 24.23](24-concurrent.md#lem-24-23): a thief that claims
it later takes pairs that the owner held at the load), or claimed, and then its
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

## 24.12 An exact priority queue without locks {#sec-24-12}

The MultiQueue of [§24.9](24-concurrent.md#sec-24-9) relaxes the order of
the pops, and the queues of [§24.11](24-concurrent.md#sec-24-11) keep it
exact only within each worker. Some parallel searches, however, want one
OPEN shared by all the threads in the exact order of the keys (e.g., the
SPA* of [Burns et al. 2010](Z-bibliography.md#ref-burns2010), whose
threads take the best open node of the search). A heap under one lock
serializes them, and its root is a line that they all write.
`LockFreePriorityQueue` is the skiplist of [Lindén and Jonsson
2013](Z-bibliography.md#ref-linden2013), which is linearizable and
lock-free, and whose DeleteMin pays one read-modify-write in the common
case. The earlier queues on skiplists ([Lotan and Shavit
2000](Z-bibliography.md#ref-lotan2000); [Sundell and Tsigas
2005](Z-bibliography.md#ref-sundell2005)) unlink each node that they
delete. A DeleteMin then writes the pointers of the first nodes of the
list (one CAS to mark the node and one per level to unlink it), and the
other DeleteMin, which read those pointers, usually miss on them; Sundell and
Tsigas, moreover, keep at most one deleted node in the list, and make the
threads that meet it help to unlink it. Lindén and Jonsson leave instead the
deleted nodes in the list, and cut them off in batches.

The pairs are the nodes of a skiplist ([Pugh
1990](Z-bibliography.md#ref-pugh1990)) of $L = 32$ levels between a head and a
tail, a node of height $h$ (drawn with probability $2^{-h}$) being in the lists
of the levels $0 , \ldots , h - 1$. The keys are made distinct by a stamp of
the insertion, $\kappa = ( k , s )$, where $s$ is the count of the insertions
of the handle and its number, so that two pairs of equal keys come out in the
order of their stamps (the device of [Sundell and Tsigas
2005](Z-bibliography.md#ref-sundell2005)); we compare the nodes by $\kappa$,
the tail having the greatest. The pointer of level 0 of a node $p$ carries in
its lowest bit, as in the lists of [Harris
2001](Z-bibliography.md#ref-harris2001), a mark, which we write $d( p )$; note
that the mark tells that the node *after* $p$ is deleted, and a node is
*deleted* when the word that reaches it at level 0 is marked. An insertion
swings that word by a CAS from its value unmarked, and therefore never links a
node in front of a deleted one. A DeleteMin walks the list of level 0 from the
head over the marked words and marks the first word not marked by a
fetch-and-or (FAO), which deletes the node that the word reaches. A DeleteMin
that walked more than $b$ nodes (the parameter `offset` of the constructor, $b
\ge 1$) moves the word of level 0 of the head past the deleted nodes by one CAS
(a *cut*), which also sets in that word the *bit of the cut*, then the upper
words of the head after them (the *restructure*), then clears the bit of the
cut by a fetch-and-and, and hands the nodes cut off to the reclamation by
epochs described below. We write $x \prec y$ when the node $x$ comes before $y$
in the list of level 0.

<span id="lem-24-28"></span>**Lemma 24.28.** *The words of level 0 change
only by the CAS of an insertion, which links a new node between two
adjacent ones, by the FAO of a DeleteMin, which sets a mark, by the cut,
which moves the word of the head forward with its mark and the bit of the
cut set, and by the fetch-and-and with which the thread of the cut then
clears that bit, which changes neither the pointer nor the mark. Hence $\prec$
is a fixed order of the nodes ever linked, and at each moment (i) the
marked words form a prefix of the list, i.e., the deleted nodes are a prefix
of it, (ii) after the first deletion the word of the head is marked, and
(iii) the nodes not deleted have increasing $\kappa$ along the list.*

*Proof.* No other statement writes a word of level 0 once its node is linked,
and none of the four moves a node or unlinks one but the cut, which unlinks
only from the head (the fetch-and-and leaves the node that the word reaches,
and its mark, as they are). As to (i), a FAO marks $x.\mathit{next}[ 0 ]$ where
$x$ is the head or a node reached by a marked word, and hence extends the
prefix, while the CAS of an insertion succeeds only on an unmarked word, and
writes an unmarked word to the new node, whose word, $\langle S_0 , 0 \rangle$,
is unmarked as well. Property (ii) holds since the first FAO marks the word of
the head, the cut writes it with its mark set, and the fetch-and-and clears
only the bit of the cut. As to (iii), the walk of $\mathrm{locate}( \kappa )$
stops at level 0 at a node $S_0$ that is the tail or has a greater key. Its
predecessor $P_0$ is the head, a node of key less than $\kappa$, or a deleted
node, since the walk passes a node only for its key, for a mark on the word
that reaches it, or for a mark on its own word, i.e., when its successor is
deleted (the start $P_1$ of the walk of level 0 falls in the same cases, by
induction on the levels). The CAS succeeds only if $P_0.\mathit{next}[ 0 ]$ is
still $\langle S_0 , 0 \rangle$; hence $S_0$ is not deleted at that moment, and
the new node lies after the deleted nodes and between two nodes not deleted of
smaller and greater keys. $\square$

We now turn to the upper lists. They are only shortcuts, and need not
hold all the nodes (an insertion links its node there from the bottom up,
after the CAS of level 0, and stops at the first level where its test
fails); we need, however, that the pointers of all the levels lead forward
in $\prec$, since the walks and the safety of the reclamation rest on it.
Lindén and Jonsson try to ensure it by stopping the insertion when the
successor $S_i$ found at the level $i$ is deleted, which they detect by
$d( S_i )$, or, for the last deleted node, whose $d$ is not set, by
comparing $S_i$ with the last deleted node met by the walk of level 0. This
walk, however, starts from $P_1$, and it misses $S_i$ when $P_1$ lies past
it. This may happen when the list of level 1 has a branch: a node linked at
level 1 after a node that the head has already passed is reachable at
level 1 only from that node, and yet it may be linked at level 2 after the
head. The insertion would then link its node $n$ before $S_i \prec n$, i.e.,
backwards, and a later walk may reach through it the nodes freed. Hence we
use instead the test $\mathrm{behind}( S_i , n )$ of [Algorithm
24.13](24-concurrent.md#alg-24-13), which reads the successor of $S_i$.

<span id="lem-24-29"></span>**Lemma 24.29.** *Let $n$ be linked at level
0, and let $c$ be the successor found at an upper level by a walk for
$\kappa( n )$ made after the CAS that linked $n$ (or by the walk before it,
when $c$ is compared after that CAS). If $\mathrm{behind}( c , n )$ is
false, then $n \prec c$.*

*Proof.* Let $c \prec n$, and let us see that the test is true. The tail
comes after all the nodes, and therefore $c$ is not the tail, and
$\kappa( c ) > \kappa( n )$, since the walk did not pass $c$ at its
level. Let $P_0$ be
the predecessor of $n$ found by the walk of level 0; then $c \preceq P_0$.
If $c$ were not deleted when $n$ was linked, then neither would $P_0$ be,
by [Lemma 24.28](24-concurrent.md#lem-24-28) (i), and $P_0$ would have a key
less than $\kappa( n )$, as in its proof; by (iii) $\kappa( c ) \le \kappa(
P_0 ) < \kappa( n )$, which is false. Hence $c$ is deleted, and so is it at
the read of $c.\mathit{next}[ 0 ]$ in $\mathrm{behind}$. If the word read is
marked, the test is true. Otherwise $c$ is the last deleted node and its
successor $f$ is the first node not deleted; $n$ is not deleted, since
$c \prec n$ and the deleted nodes are a prefix, whose last is $c$, and
therefore $f \preceq n$. Either $f = n$, or $f \prec n$, and then
$\kappa( f ) < \kappa( n )$ by (iii), and $f$ is not the tail; in both cases
the test is true. $\square$

The test may be true when $n \prec c$ as well, but only if $n$ is deleted
by then (cf. the last case of the proof), and the insertion then just stops
its tower, as it does anyway when $d( n )$ is set.

<span id="lem-24-30"></span>**Lemma 24.30.** *A pointer of any level of a
node, when not null, reaches a node after it in $\prec$, and each
walk of [Algorithm 24.13](24-concurrent.md#alg-24-13) moves forward.*

*Proof.* The words of the head and of level 0 do so by [Lemma
24.28](24-concurrent.md#lem-24-28). An upper pointer of a node is written
only by an insertion, either $n.\mathit{next}[ i ] \gets S_i$ after the
test of line 7, which gives $n \prec S_i$ by [Lemma
24.29](24-concurrent.md#lem-24-29), or by the CAS that sets
$P_i.\mathit{next}[ i ]$ to $n$. In the latter case, by induction on the
steps, the walk of $\mathrm{locate}$ moves forward, and $P_i \preceq P_{i -
1} \preceq \cdots \preceq P_0$, while $P_0$ is the predecessor of $n$: in the
first round by the CAS of level 0, and after a new $\mathrm{locate}$ by the
test $S_0 = n$ of line 10. Hence $P_i \prec n$. $\square$

<span id="lem-24-31"></span>**Lemma 24.31.** *Let the abstract queue be the
sequence of the pairs of the nodes not deleted, in the order of the list of
level 0. Then `insert()` and `tryDeleteMin()` are linearizable as the
operations of a priority queue at the points of the header: `insert()` at
the CAS that links its node at level 0, `tryDeleteMin()` at the FAO that
marks the word that reaches the node it returns, or, when it returns
nothing, at the read of a pointer to the tail; `tryMinKey()` at the read of
the first unmarked word.*

*Proof.* By [Lemma 24.28](24-concurrent.md#lem-24-28) (iii) the abstract
queue is ordered by $\kappa$, and each point lies between the invocation
and the response of its operation. At the CAS of `insert()` the new node
joins the nodes not deleted at the place of its $\kappa$, and nothing else
changes. A DeleteMin applies its FAO to the word of a node $x$ that is the
head or a deleted node, since it reached $x$ by marked words. If the word
was not marked, the node $y$ that it reaches is the first not deleted, by
(i), i.e., the pair of least key; the FAO removes it from the abstract
queue, and no other DeleteMin returns it, since only one FAO finds the word
unmarked. If it was marked, another DeleteMin deleted $y$ in the meantime,
nothing changes, and the walk goes on. A DeleteMin that reads a pointer to
the tail from the head or from a deleted node $x$ finds no node after $x$,
and hence an empty abstract queue. Note that no FAO marks a pointer to the
tail: the FAO follows a read of the same word that did not reach the tail,
and such a word never comes to reach it (an insertion links a node before
its old successor). The same argument gives the point of
`tryMinKey()`. The responses are therefore those of the sequential queue
on the operations in the order of their points. $\square$

<span id="lem-24-32"></span>**Lemma 24.32.** *Let a cut move the word of
level 0 of the head from the node $o$ to the node $\nu$. Then (i) the cuts
and their restructures run one at a time; (ii) no node being inserted lies
before $\nu$; (iii) once the restructure of the cut is over, each word of
the head reaches $\nu$ or a node after it, and does so ever after; (iv) a
thread whose reads of the list follow the end of that restructure reaches no
node before $\nu$.*

*Proof.* (i) The CAS of a cut expects a word without the bit of a cut and
writes it with the bit, which only the thread of the cut clears, after its
restructure, and no other statement writes the word of the head once it is
marked. (ii) The values of the word of the head never repeat, since they
move forward; hence the walk of the DeleteMin that cut read the word as
$o$, and visited as its node $x$ each node from $o$ to $\nu$, the last
excluded. Then $\nu$ is the first of them whose flag $\mathit{inserting}$
it read set, or the node that it deleted; a node before $\nu$ was read
with its flag clear, which an insertion clears once and at its end. (iii)
Each node before $\nu$ has $d$ set at the cut: those visited were passed by
marked words, and those before $o$ were cut before by induction. The
restructure, alone by (i), reads each upper word $h$ of the head after the
cut, leaves it if $d( h )$ is clear, and then $\nu \preceq h$, or writes it
by a CAS from $h$ to the first node $c$ with $d( c )$ clear that it finds,
and then $\nu \preceq c$. The only other writes of an upper word of the
head are those of the restructures of the later cuts, which by the same
argument write nodes after their own $\nu' \succeq \nu$, and the CAS of
the insertions with $P_i$ the head, which write a node being inserted,
after $\nu$ by (ii). (iv) Follows from (iii) and from [Lemma
24.30](24-concurrent.md#lem-24-30), since the pointers lead forward.
$\square$

The memory of the nodes cut off is reclaimed by a policy of
[§24.14](24-concurrent.md#sec-24-14), owned by the queue: by default the epochs
of [Fraser 2004](Z-bibliography.md#ref-fraser2004) (see also [Hart et al.
2007](Z-bibliography.md#ref-hart2007)), `FraserEpochs< 64 >`, or those of any
other $R$, while the hazard pointers do not fit, since the walk of a DeleteMin
goes on from nodes that may have been cut off, and the queue refuses them when
compiling (the epochs of `ConcurrentFlatTable`, on the other hand, clear a
table, and free no memory). Each handle holds a record of the domain, whose
guard announces the epoch $E$ by a sequentially consistent exchange before the
operation reads the list, whose words it then reads with sequential consistency
too. The thread of a cut retires the nodes cut off after the cut and the
restructure, stamps them with $E$ read then, and tries to raise $E$ at the end
of its DeleteMin once $R$ nodes have been retired since its last attempt; a
node stamped $r$ is freed once its thread reads $E \ge r + 2$. By [Lemma
24.32](24-concurrent.md#lem-24-32) (iv) the last CAS of the cut and of its
restructure *unlinks* the nodes cut off, in the sense of
[§24.14](24-concurrent.md#sec-24-14): a thread whose reads follow it in the
total order $S$ of [§24.1](24-concurrent.md#sec-24-1) reaches no node cut off.
Hence, by [Lemma 24.37](24-concurrent.md#lem-24-37), no node cut off is freed
while an operation that reached it runs. Of course, a relaxed store of the
announce followed by a sequentially consistent fence, with reads that acquire,
gives the same order through the rules of the fences. We prefer the exchange,
since the sanitizer of threads does not model the fences, and on x86-64 it is
one instruction where the store and the fence are two (a sequentially
consistent load is there the same plain load as one that acquires). No node is
leaked: the destructor frees those still in the list, and the domain those that
wait for their epoch. The reclamation is blocking, as in the paper: a thread
stopped inside an operation keeps $E$ from growing, and the nodes cut off
afterwards from being freed, while the operations of the other threads go on.

<span id="lem-24-33"></span>**Lemma 24.33.** *The operations are
lock-free but for the allocation of the nodes: in an infinite run of
$p$ threads, some operation completes infinitely often.*

*Proof.* Each operation completes after a bounded number of its own
successful read-modify-writes: an insertion after at most $L$, a DeleteMin
after one FAO and, if it cuts, one CAS, at most $L - 1$ in its restructure
and the clear of the bit. Let no operation complete after some moment of an
infinite run; then each of the $p$ operations running performs finitely
many successful read-modify-writes, and after a later moment none at all.
From then on no word of the list changes, and therefore no CAS fails, no
FAO finds a mark set by another, and no walk passes more than the finitely
many nodes of the list. Some operation, however, takes infinitely many
steps, since the run is infinite and the threads are $p$, and it then
performs a successful read-modify-write or completes, a contradiction. We
remark that no step waits for another thread (the flag
$\mathit{inserting}$ and the bit of a cut only keep a cut from moving past
a node, or from starting), and that a restructure retries a level only
after a CAS of an insertion on the same word, since no other restructure
runs at the same time, by [Lemma 24.32](24-concurrent.md#lem-24-32) (i).
$\square$

A thread stopped inside an insertion (with its flag set), or inside a cut
(with the bit set), keeps the head where it is, and the prefix then grows
with the deletions, which all the later DeleteMin walk; Lindén and Jonsson
remark the first case. Without such a thread, the prefix is bounded as
follows.

<span id="lem-24-34"></span>**Lemma 24.34.** *In a run of one thread, a
DeleteMin walks at most $b + 1$ nodes (the head included), the list holds at
most $b$ deleted nodes between two operations, and the deleted nodes are cut
off once per $b$ DeleteMin that delete a node, from the $( b + 1 )$-th on.*

*Proof.* Let $j$ be the number of the deleted nodes in the list before a
DeleteMin that deletes a node. Its walk visits the head and the $j$ deleted
nodes, $o = j + 1$ nodes, and deletes one more; if $o > b$, its cut leaves
only the node just deleted, the new first node of the head, and $j = 1$
afterwards, and otherwise $j + 1$. From $j = 0$ the values after each
DeleteMin are $1 , 2 , \ldots , b$, then the cut gives 1, and the cycle
repeats with $b$ DeleteMin per cut. An insertion does not change $j$, and a
DeleteMin that finds the queue empty walks the same $j + 1 \le b + 1$ nodes
and changes nothing. $\square$

With $p$ threads the walk is (possibly) longer by the deletions of the
others that take place during it. Also, a cut may fail because another one
started since its DeleteMin read the head, and then the next DeleteMin that
walks more than $b$ nodes cuts. The test `engineered_queues.lockfree-pq`
checks the bound of the lemma. Furthermore, each handle counts in
`counts()` its read-modify-writes, those that failed, the nodes walked and
the cuts, by which two queues can be compared on a busy machine without
timing them. The public members of the queue are

```cpp
// declarations of <ADS/concurrent/LockFreePriorityQueue.h>
template< class K , class T , class Less = std::less< K > ,
          reclamation_policy R = FraserEpochs< 64 > >
class LockFreePriorityQueue
{
 public:
 struct Entry
 {
  K key;
  T item;
  };
 struct Counts
 {
  std::uint64_t rmw = 0;
  std::uint64_t failed = 0;
  std::uint64_t walked = 0;
  std::uint64_t cuts = 0;
  };
 class Handle
 {
  public:
  void insert( K k , T x );
  std::optional< Entry > tryDeleteMin( void );
  std::optional< K > tryMinKey( void );
  bool empty( void );
  const Counts & counts( void ) const;
  };
 explicit LockFreePriorityQueue( unsigned offset = 32 ,
                                 std::uint64_t seed = 1 ,
                                 Less less = Less() );
 Handle handle( void );
 unsigned offset( void ) const;
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t prefix( void ) const;
 bool consistent( void ) const;
 const typename R::Domain & domain( void ) const;
 };
```

where `size()`, `empty()`, `prefix()` and `consistent()` of the queue are
called with no operation running, `domain()` gives the counters of the
reclamation, while `empty()` of a handle is
linearizable, as `tryMinKey()`; as in the MultiQueue, the constant
`decrease_key` declares that a key is lowered by a new pair. Each thread
takes a handle, which holds its record of the reclamation:

```cpp
ADS::LockFreePriorityQueue< int , int > Q;  // b = 32
std::vector< std::thread > T;
for( int k = 0 ; k < 4 ; ++k )
 T.emplace_back( [ & , k ]() {
  auto h = Q.handle();
  for( int i = 0 ; i < 100 ; ++i )
   h.insert( 4 * i + k , i );
  } );
for( auto & t : T )
 t.join();
auto h = Q.handle();
while( auto e = h.tryDeleteMin() ) {
 }  // the keys 0, 1, 2, ...
```

<span id="alg-24-13"></span>**Algorithm 24.13.** The insertion and the
DeleteMin of `LockFreePriorityQueue::Handle` ([Lindén and Jonsson
2013](Z-bibliography.md#ref-linden2013), with the test $\mathrm{behind}$
and the bit of a cut of ours); $P$ and $S$ are the predecessors and the
successors at each level, $d( p )$ the mark of $p.\mathit{next}[ 0 ]$, and
$b$ = `offset()`.

--8<-- "generated/LockFreePriorityQueue-insert.pseudo.md"

--8<-- "generated/LockFreePriorityQueue-tryDeleteMin.pseudo.md"

As to the cost, an insertion is the search of a skiplist, $O( \log m )$
expected on $m$ nodes, plus the deleted prefix that its walk of level 0 may
pass, and one CAS per level of its tower, two on average. A DeleteMin walks
the prefix, $O( b )$ nodes on one thread by [Lemma
24.34](24-concurrent.md#lem-24-34), and pays one FAO, plus one cut per $b$
deletions, whose restructure walks, at each level, the nodes of that level
among the $O( b )$ cut off. We remark that the nodes of the prefix are read
by all the DeleteMin and written by none but for the word that each
DeleteMin marks. Indeed, this is the reason of the design: the reads of
lines that nobody writes are "cheap", while a write to the first nodes (as
in the queues that unlink each node) makes the other DeleteMin miss on
them. The FAO on the first unmarked word, however, is still one word
that all the DeleteMin contend for, and at most one of them succeeds per
deletion. With many threads the queue therefore (typically)
serves the DeleteMin one at a time at that word, which the relaxed queue of
[§24.9](24-concurrent.md#sec-24-9) avoids at the price of the order.

## 24.13 Structures of one writer {#sec-24-13}

A worker of a parallel search keeps state that only it changes and that
the others read: its CLOSED set, which the others look up to prune the
states they reach, and the least $f$ of its OPEN list, which bounds the
work of all. A structure of one writer needs no read-modify-write, since
no two threads may want to write the same word (cf.
[§24.2](24-concurrent.md#sec-24-2)), and the two of this section are
built on that remark.

**One writer and many readers.** `SingleWriterFlatTable` is the linear probing
of [§24.8](24-concurrent.md#sec-24-8) with the pairs in the words (one word for
a set, two for a key and a value, each trivially copyable and of at most 8
bytes), filled by one thread. An insertion reads the words from the place of
its key and, at the first free one, stores the value and then the key, the key
by a store with release. A growth fills a vector twice as large while no reader
can see it, and then stores it as the vector in use with release; the old one
is kept for the readers still on it. A lookup is that of `ConcurrentFlatTable`:
it reads the vector in use and the words with acquire, and, when it ends at a
free place, reads the vector in use again and starts over if the table grew
meanwhile. The counters of the threads and the protocol of Dekker of [Lemma
24.16](24-concurrent.md#lem-24-16), i.e., two sequentially consistent
read-modify-writes per insertion, are thus gone, and an insertion is made of
plain loads and stores on x86-64. With `EpochClear` the writer renews a group
out of use when an insertion first reaches it, writing 0 to its words and then
its stamp with release, so that `clear()` takes $O( 1 )$ as in
[§24.8](24-concurrent.md#sec-24-8). Its public members, but for the variants
that take the hash already computed, are

```cpp
// declarations of <ADS/concurrent/SingleWriterFlatTable.h>
template< class K , class V , class H = DefaultHash< K > ,
          class Eq = std::equal_to< K > , class C = ZeroingClear >
class SingleWriterFlatTable
{
 public:
 explicit SingleWriterFlatTable( std::size_t n = 0 , H h = H() ,
                                 Eq eq = Eq() );
 std::uint64_t hash( const K & k ) const;
 std::pair< V , bool > findOrInsert( const K & k , const V & v );
 bool insert( const K & k , const V & v );
 template< class F >
 bool update( const K & k , F && f )
  requires( valueBytes() > 0 );
 std::optional< V > find( const K & k ) const;
 std::optional< K > findKey( const K & k ) const;
 bool contains( const K & k ) const;
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t capacity( void ) const;
 std::size_t rehashes( void ) const;
 void reserve( std::size_t n );
 void clear( void );
 std::uint64_t epoch( void ) const;
 template< class F >
 void forEach( F && f ) const;
 bool consistent( void ) const;
 };
```

besides `findOrInsertKey( k , h )` of a set, which returns the key held
equal to $k$. One thread at a time, the writer, calls `findOrInsert()`,
`findOrInsertKey()`, `insert()`, `update()`, `reserve()` and `clear()`,
and the role may pass to another thread by any synchronization. Any thread
calls the lookups; `size()` gives the writer its exact count, and the
others a count that the table had since the last `clear()`.

<span id="alg-24-14"></span>**Algorithm 24.14.** The insertion, with the
growth, and the lookup of `SingleWriterFlatTable`; $W$ is the vector in
use, of $2^l$ places, and $n$ the number of the pairs.

--8<-- "generated/SingleWriterFlatTable-findOrInsert.pseudo.md"

--8<-- "generated/SingleWriterFlatTable-find.pseudo.md"

<span id="lem-24-35"></span>**Lemma 24.35.** *Let one thread make all the
insertions, the updates and the growths of a `SingleWriterFlatTable`, and
let any thread make lookups. Then (i) a lookup that reads a key word
$w \ne 0$ reads with it the value stored at its insertion or by a later
update, and everything that the writer wrote before the insertion; (ii) a
lookup of $k$ that reads a free place of the vector $T$ (or a group out of
use) and then $T$ again as the vector in use is linearized at that read,
which no insertion of $k$ happens before; (iii) a vector replaced by a
growth is no longer written, and holds every pair inserted before the
growth. Hence the lookups are wait-free between two growths and start over
at most once per growth.*

*Proof.* For (i), the store of the value and every earlier write of the
writer are sequenced before the store of the key with release, which the
load of the key with acquire synchronizes with; an update stores one
atomic word with release, so that the value read is whole. For (ii), let
the insertion of $k$ into $T$ write the place $j$. The writer found full
the places from that of $k$ to $j - 1$, renewing with `EpochClear` each
group out of use among them (its words, then its stamp with release), and
these writes are sequenced before the store of $k$. If that store happened
before the read of a place $i < j$ of the sequence, the write of $i$ (or of
the stamp of its group) would happen before the read as well, and the
coherence of the reads would forbid reading the free place. If $k$ was
inserted into a later vector, its store follows the store of that vector
as the one in use, and if it happened before the read of $i$, it would
happen before the second read of the vector in use, which then could not
read $T$. For (iii), the writer writes only the vector in use, and the new
one before it is published, after the copy has read every pair of the old
one; the vectors replaced are freed only by `clear()` or by the
destruction, which run with no reader. A lookup restarts only when it
reads a vector other than the one it searched, i.e., once per growth.
$\square$

The insertions of one writer thus cost the loads of their probe and one or
two stores, against the two sequentially consistent read-modify-writes on
the counter of a thread of `ConcurrentFlatTable`, which several writers
need (cf. [Lemma 24.16](24-concurrent.md#lem-24-16)). The price is that the
table has one writer:
the threads of a search keep one table each, and a thread that looks for
a state reads the table of the worker that may hold it.

**A board of least keys.** `MinBoard< K >` keeps $p$ keys, one per thread,
each in an atomic word on a line of cache that no other slot shares. `publish(
i , k )`,
called by the owner of the slot $i$ only, reads the slot and stores $k$
only if it differs, both relaxed; a worker whose key stays thus leaves
its line shared in the caches of the readers. `min()` reads the $p$
slots, relaxed, and returns the least key read ([Algorithm
24.15](24-concurrent.md#alg-24-15)).

<span id="alg-24-15"></span>**Algorithm 24.15.** The publish of the
owner of the slot $i$ and the least key of a `MinBoard` of $p$ slots
$S$.

--8<-- "generated/MinBoard-publish.pseudo.md"

--8<-- "generated/MinBoard-min.pseudo.md"

Its public members are

```cpp
// declarations of <ADS/concurrent/MinBoard.h>
template< class K >
 requires( std::totally_ordered< K > && std::is_trivially_copyable_v< K > )
class MinBoard
{
 public:
 explicit MinBoard( std::size_t p , K k = std::numeric_limits< K >::max() );
 std::size_t size( void ) const;
 void publish( std::size_t i , K k );
 K at( std::size_t i ) const;
 K min( void ) const;
 void reset( K k = std::numeric_limits< K >::max() );
 };
```

<span id="lem-24-36"></span>**Lemma 24.36.** *Let each slot of a
`MinBoard` be written by its owner only. Then (i) a read of the slot $i$
reads a key published there (or the initial one), not older in the order
of the publishes of $i$ than any publish of $i$ that happens before the
read, and two reads of $i$ by one thread never read the second older than
the first; (ii) `min()` returns $\min_i x_i$ of keys $x_i$ so read, one
per slot, which need not be the least key of the board at any instant;
(iii) if the keys published in each slot never increase, the value
returned lies between the least key of the board at the end of the call
and the least of the keys published before the call (those whose publish
happens before it), and the values returned to one thread never increase.*

*Proof.* (i) is the coherence of the atomic objects of C++, which holds
for relaxed accesses, with the publishes of a slot made in the program
order of its owner; a publish that skips its store finds the key already
there, and leaves the slot as if it had stored it. (ii) holds since
`min()` reads each slot once, at different times. For (iii), with the keys
of each slot never increasing, a key read is at most the last one
published before the call, by (i), and at least the key that the slot
holds at the end of the call; the minimum over the slots keeps both
bounds, and two calls by one thread read each slot in the order of its
publishes. $\square$

The board orders no other access: a thread that reads $k$ in a slot is not
thereby ordered after the writes that the owner made before publishing $k$,
which need a separate synchronization, and it uses the board as a bound
only,
e.g., the least $f$ of the workers below which a search may prune. A
publish costs a load of a line that the owner holds, and a store only
when the key changes; `min()` costs $p$ loads of lines that only their
owners write.

## 24.14 The reclamation of the memory of the nodes {#sec-24-14}

We now turn to the structures of nodes that any thread may unlink: the
stack of [Treiber 1986](Z-bibliography.md#ref-treiber1986), the queue of
[Michael and Scott 1996](Z-bibliography.md#ref-michael1996) and the
skiplist of [§24.12](24-concurrent.md#sec-24-12). In them a thread that
has unlinked a node cannot free it at once, since another thread may have
reached it just before and may still read it. The queues of the earlier
sections avoid the problem by construction (the blocks of `MPSCQueue` and
`MPSCInbox` go back to their producer, and the places of `MPMCQueue` are
never freed), while `WorkStealingDeque` and `ConcurrentFlatTable` keep the
vectors they replace until they are destroyed, which costs less than the
vector in use. A list of nodes, however, may hold any number of them over
time (typically, far more than it holds at once), and it needs a
*reclamation*. We say that a node is *retired* when
the thread that unlinked it hands it over, and the reclamation frees it
once no thread may still hold it. Note that the reclamation also removes
the ABA problem of [§24.1](24-concurrent.md#sec-24-1) from these
structures: a node that is protected is not freed, and therefore it is not
allocated again and linked anew while a compare-and-swap may still expect
it (cf. [Lemma 24.40](24-concurrent.md#lem-24-40)).

The reclamations of `Reclaim.h` are *policies*, i.e., parameters of the
template of a structure. They are `FraserEpochs< R >`, the epochs of [Fraser
2004](Z-bibliography.md#ref-fraser2004), and `HazardPointers`, the hazard
pointers of [Michael 2004](Z-bibliography.md#ref-michael2004). A policy
gives a domain, records and guards. The *domain* is the state that the threads
of one structure share; the structure owns it, and its destructor frees the
nodes still retired. The *record* of a thread on the domain (`Local< K >`, with
$K$ slots under the hazard pointers) is held by the handle of the thread. The
*guard* is the span of one operation on a record, and its `protect( i , s )`
reads the pointer of the atomic $s$ and keeps the node it reaches from being
freed. A node derives from `Retirable`, the link by which it waits in a list of
the retired, together with the function that frees it. Under the epochs a guard
protects all the nodes that its operation reaches (`kRegions` is true), whereas
under the hazard pointers it protects only the node in each of its $K$ slots,
the last one that `protect()` read into the slot. The hazard pointers therefore
do not fit a structure whose walk goes on from a node that may have been
retired, as the walk of the deleted prefix of the skiplist does, and
`LockFreePriorityQueue` refuses them when compiling. The default of the
skiplist (cf. [§24.12](24-concurrent.md#sec-24-12)), of the stack and of the
queue is `FraserEpochs< 64 >`. In all three, the
structure frees the nodes still in it and the domain those retired, and no
node is leaked.

Throughout the section we say that a sequentially consistent write $u$
*unlinks* a node $x$ when no thread reaches $x$ by a read that follows $u$
in the total order $S$ of [§24.1](24-concurrent.md#sec-24-1). In the stack,
$u$ is the compare-and-swap of the pop that swings the top past $x$, which
is never linked again. In the queue, it is the compare-and-swap of the
dequeue that swings the head past $x$. In the skiplist, it is the last
compare-and-swap of the cut of $x$ and of its restructure, by [Lemma
24.32](24-concurrent.md#lem-24-32) (iv). A node is retired after $u$ by the
thread of $u$, and the reads by which a thread reaches a node are all
sequentially consistent (the `protect()` of the policies, and the reads
`kWalk` of the skiplist); we recall that on x86-64 these are the plain
loads of an acquire, and on AArch64 the same `ldar`.

**The epochs.** The domain keeps an epoch $E$, from 2, and a list of
records. A record announces $2 e + 1$ while its thread runs a guard of the
epoch $e$, and 0 otherwise; it is pushed on the list by a sequentially
consistent compare-and-swap, and it may be given back by its thread and
taken by another, but it is freed only with the domain. A guard reads $E$
and makes its announce by a sequentially consistent exchange, before its
thread reads the structure. A node retired waits in one of three lists of
its record, the *limbo* of its *label* modulo 3. The thread of the record
frees it at the start of a later guard, or when the limbo of the same
residue receives a node of a newer label.

Under `FraserEpochs< R >` the label of a node is $E$, read after its
unlink, and the node is freed once its thread reads $E \ge \ell + 2$ for
the label $\ell$. A guard that ends after $R$ retirements of its record
since the last attempt reads all the records, and raises $E$ by a CAS from
$e$ to $e + 1$ if no record announces an odd value other than $2 e + 1$.
With $R = 1$ the attempt follows each retirement (in
`LockFreePriorityQueue`, in which only a cut retires, each cut), and the
read of all the $N$ records is paid at each one; a larger $R$ shares it
among $R$ retirements, at the price of more nodes waiting.

<span id="alg-24-16"></span>**Algorithm 24.16.** The retirement of a node
and the guard of an operation under the epochs of `EpochDomain`
(`Local::retire()`, and the constructor and the destructor of
`Local::Guard`).

--8<-- "generated/EpochReclaim-EpochDomain-retire.pseudo.md"

--8<-- "generated/EpochReclaim-EpochDomain-enter.pseudo.md"

<span id="lem-24-37"></span>**Lemma 24.37.** *Under `FraserEpochs< R >`,
let a node $x$ be unlinked by $u$ and retired with the label $\ell$, and
let a guard $g$ reach $x$. Then $x$ is not freed before $g$ ends.*

*Proof.* The reads of $g$ follow its announce in the order of its thread,
and hence in $S$. If the announce followed $u$ in $S$, so would the read by
which $g$ reaches $x$, against the definition of $u$; thus the announce
precedes $u$, which precedes the read of $E = \ell$ by the retirement of
$x$. The read of $E$ by which $g$ made its announce $2 a + 1$ precedes the
announce, and therefore $a \le \ell$. The CAS that raises $E$ to $\ell + 2$
follows a scan of the records that read $E = \ell + 1$, after the CAS from
$\ell$ to $\ell + 1$, which follows in $S$ the read of $E = \ell$. The scan
reads the first record after it, hence after the push of the record of $g$
(which precedes the announce of $g$); it therefore reaches that record and
reads the announce of $g$ or a later value of it. Since
$2 a + 1 \ne 2 ( \ell + 1 ) + 1$, the scan finds an older announce, and it
does not raise $E$ while $g$ is open. Finally, a node of the label $\ell$
is freed only by a guard or by a retirement that reads $E \ge \ell + 2$,
and none does so before $g$ ends. $\square$

The epochs are thus blocking. Indeed, a thread stopped inside a guard keeps
$E$ from growing past its epoch plus 1, and all the nodes retired
afterwards from being freed, while the operations of the other threads go
on. The nodes waiting then grow without bound (the test
`engineered_queues.hazard-pointers-stalled` shows it), and they are freed
once the thread leaves its guard.

**The hazard pointers.** The domain of `HazardPointers` keeps a list of
*slots* and a list of *bins*. A slot is a word that holds a node protected
by its thread, or nil, and a bin is the list of the nodes retired by one
thread; both are pushed by a sequentially consistent compare-and-swap,
given back and taken again, and freed only with the domain, and we write
$H$ for the number of the slots made so far. A guard protects the node
reached by the atomic $s$ in its slot $h$ by reading $p$ from $s$, writing
$p$ in $h$ and reading $s$ again, until the two reads agree; the write and
the second read are sequentially consistent. A node retired goes into the
bin of its thread. When the bin holds $2 H + 64$ nodes, the thread reads
all the slots into a vector, sorts it, and frees each node of its bin that
the vector does not hold (the *scan*). The same domain, as a static object
of the program (`HazardDomain::global()`), serves the interface of
`std::hazard_pointer` of C++26: there `retire()` puts the object into the
bin of its thread, which the thread keeps in its thread-local storage and
gives back, after a scan, when it ends.

<span id="alg-24-17"></span>**Algorithm 24.17.** The protection of a
node and the retirement with the scan under the hazard pointers
(`hazard_pointer::protect()`, which is also the `protect()` of a guard,
and `Local::retire()` of `HazardDomain`).

--8<-- "generated/HazardPointer-hazard_pointer-protect.pseudo.md"

--8<-- "generated/HazardPointer-HazardDomain-retire.pseudo.md"

<span id="lem-24-38"></span>**Lemma 24.38.** *Let a thread write $x$ in
its slot $h$ by the store $s$, and then make a read $r$ that precedes in
$S$ the write $u$ that unlinks $x$, after which $x$ is retired. Then no scan
frees $x$ before $h$ is written again.*

*Proof.* The store $s$ precedes $r$ in $S$, being sequenced before it, and
$r$ precedes $u$. In turn, $u$ happens before each scan that may free $x$:
the scans of the thread that retired $x$ are sequenced after $u$, and those
of a thread that took its bin later follow the release of the bin, which
their acquire read. Such a scan reads the first slot after $s$ in $S$, and
hence after the push of $h$, which precedes $s$; it therefore reaches $h$,
and reads it after $s$. A sequentially consistent read that preceded a
write in the order of the modifications of its word would precede it in
$S$ too; thus the scan reads the value of $s$ or a later one. As long as
$h$ is not written again, the vector of the scan holds $x$, and the scan
keeps it. $\square$

In `protect()` the read $r$ is the second read of $s$, which returns $x$.
In the stack and in the queue, $s$ never holds $x$ again once $u$ has
swung it, and $r$ therefore precedes $u$. In the dequeue of the queue, the
successor $n$ of the head $h$ is protected by reading $h.\mathit{next}$,
which may still hold $n$ after $n$ has been retired. There $r$ is the read
of the head that returns $h$ again: it precedes the compare-and-swap from
$h$ to $n$, and hence the one that unlinks $n$.

<span id="lem-24-39"></span>**Lemma 24.39.** *Let no deleter retire, and
let the vector of each scan fit in memory. Then, between two operations of
its thread, each bin holds fewer than $2 H + 64$ nodes, where $H$ is the
number of the slots made so far. Hence fewer than $B ( 2 H + 64 )$ nodes of
$B$ bins wait to be freed, whatever the other threads do (a stopped one
included). Each scan frees at least $H + 64$ nodes in
$O( H \log H )$ time, i.e., in $O( \log H )$ per node freed.*

*Proof.* The count $H$ grows by 1 before each slot is pushed, and a scan
thus reads at most $H$ slots. A retirement adds one node to its bin, and
scans it if the bin then holds at least $2 H' + 64$ nodes, for the count
$H' \le H$ that it read. The scan keeps only the nodes whose address the
vector holds, at most one per slot read (the addresses of distinct nodes
alive are distinct), and hence at most $H$ nodes, fewer than $2 H + 64$. A
bin changes only by the retirements and the scans of the thread that holds
it, and $H$ only grows; hence the bound holds after each retirement, and
the sum follows (a bin given back keeps at most $H$ nodes after the scan of
its release, for the next thread that takes it). Finally, a scan that
starts from $2 H' + 64$ nodes and keeps at most $H'$ frees at least
$H' + 64$ of them, after reading $H'$ slots, sorting them and making one
binary search per node, i.e., in $O( H' \log H' )$ time. $\square$

The hazard pointers are therefore lock-free. A thread stopped anywhere (even
between the write of its slot and the second read) keeps at most the $K$ nodes
of its slots and the nodes of its bin from being freed. The price is a
sequentially consistent write per node protected (an exchange on x86-64), where
a guard of the epochs pays one per operation, plus the scans of all the slots.

**The stack and the queue.** `LockFreeStack` is the stack of Treiber. A
push links a new node above the top it read, by a CAS of the top; a pop
protects the top $t$, swings the top from $t$ to the successor of $t$,
moves the item out of $t$ and retires it. `LockFreeQueue` is the queue of
Michael and Scott, with a dummy node first. An enqueue protects the tail
$t$, links its node after $t$ by a CAS of $t.\mathit{next}$ from nil, and
swings the tail to it; a thread that finds the tail behind the last node
swings it first. A dequeue protects the head $h$ in its slot 0 and the
successor $n$ in its slot 1, and reads the head again. It then swings the
tail first if the tail is $h$; otherwise it swings the head from $h$ to $n$,
moves the item out of $n$ (now the dummy) and retires $h$.

<span id="alg-24-18"></span>**Algorithm 24.18.** The push and the pop of
`LockFreeStack::Handle`, where $\mathrm{protect}$ and $\mathrm{retire}$
are those of the policy.

--8<-- "generated/LockFreeStack-push.pseudo.md"

--8<-- "generated/LockFreeStack-tryPop.pseudo.md"

<span id="alg-24-19"></span>**Algorithm 24.19.** The enqueue and the
dequeue of `LockFreeQueue::Handle`, with the slots 0 and 1 of the policy.

--8<-- "generated/LockFreeQueue-enqueue.pseudo.md"

--8<-- "generated/LockFreeQueue-tryDequeue.pseudo.md"

<span id="lem-24-40"></span>**Lemma 24.40.** *Under either policy, a
pop of `LockFreeStack` whose compare-and-swap succeeds from
$t$ writes the successor that $t$ had when it was pushed, and $t$ is the
top at that moment. Hence the stack is linearizable: a push at its
compare-and-swap, and a pop at its compare-and-swap or, when it finds the
stack empty, at its read of the top.*

*Proof.* The pop protected $t$ by a read of the top that returned $t$ after the
write of its slot, or inside its guard. By [Lemma
24.38](24-concurrent.md#lem-24-38) (the top never holds $t$ again once a pop
has swung it), or by [Lemma 24.37](24-concurrent.md#lem-24-37), $t$ is not
freed before the pop ends. A node not freed is not allocated again, and a push
always links a new node; hence the top equals $t$ at the CAS only if no pop has
swung it from $t$ since $t$ was pushed. Also, $t.\mathit{next}$ is written
once, before the push, and it is still the node below $t$. The operations then
act on the sequence of the nodes from the top as the push and the pop of a
stack. $\square$

<span id="lem-24-41"></span>**Lemma 24.41.** *In `LockFreeQueue`, under
either policy, (i) the head never passes the tail, and the
node of the tail is therefore never retired while it is the tail; (ii) the
node whose item a dequeue moves out is touched by no other dequeue until
it is retired. The queue is linearizable: an enqueue at the
compare-and-swap that links its node, and a dequeue at the one that swings
the head or, when it finds the queue empty, at its read of a nil
successor.*

*Proof.* (i) A dequeue swings the head from $h$ only after reading the tail
different from $h$, which it reads after the head $h$, while the head was at
$h$ and the tail at or after it. The tail only moves forward, and the head thus
moves to $n$, at or before the tail; the node of the tail is retired only after
the head has passed it, hence after the tail has. (ii) After the CAS from $h$
to $n$, $n$ is the dummy, whose item no dequeue reads. It is retired only by
the dequeue that swings the head past it, and then the guard of the first
dequeue still protects it, by [Lemma 24.38](24-concurrent.md#lem-24-38) (with
the reads of the head discussed above) or by [Lemma
24.37](24-concurrent.md#lem-24-37). As in the stack, a protected node is not
allocated again, and the CAS of the head, of the tail and of the successors
find the nodes that they read; the linearizability is then that of [Michael and
Scott 1996](Z-bibliography.md#ref-michael1996). $\square$

Both structures are lock-free but for the allocation of a node per push
and for the reclamation, which is blocking under the epochs. Each thread
uses them through a handle, which holds its record of the policy:

```cpp
// declarations of <ADS/concurrent/LockFreeQueue.h>
//                 <ADS/concurrent/LockFreeStack.h>
template< class T , reclamation_policy R = FraserEpochs< 64 > >
class LockFreeQueue
{
 public:
 class Handle
 {
  public:
  void enqueue( T x );
  std::optional< T > tryDequeue( void );
  };
 LockFreeQueue( void );
 Handle handle( void );
 std::size_t size( void ) const;
 bool empty( void ) const;
 const Domain & domain( void ) const;
 };
template< class T , reclamation_policy R = FraserEpochs< 64 > >
class LockFreeStack
{
 public:
 class Handle
 {
  public:
  void push( T x );
  std::optional< T > tryPop( void );
  };
 LockFreeStack( void ) = default;
 Handle handle( void );
 std::size_t size( void ) const;
 bool empty( void ) const;
 const Domain & domain( void ) const;
 };
```

where `size()` and `empty()` of the structure are called with no operation
running, and `domain()` gives the counters of the reclamation (among them
`pending()`, the number of the nodes retired that wait to be freed). The
policies are interchangeable:

```cpp
ADS::LockFreeQueue< int , ADS::HazardPointers > Q;
std::vector< std::thread > T;
for( int k = 0 ; k < 4 ; ++k )
 T.emplace_back( [ & , k ]() {
  auto h = Q.handle();
  for( int i = 0 ; i < 100 ; ++i ) {
   h.enqueue( 4 * i + k );
   ( void )h.tryDequeue();
   }
  } );
for( auto & t : T )
 t.join();
// fewer than B ( 2 H + 64 ) nodes wait, by Lemma 24.39
std::size_t waiting = Q.domain().pending();
```

The interface of C++26, on the domain of the program, is that of
`std::hazard_pointer`:

```cpp
// declarations of <ADS/concurrent/HazardPointer.h>
class hazard_pointer
{
 public:
 hazard_pointer( void ) noexcept = default;
 bool empty( void ) const noexcept;
 template< class T >
 T * protect( const std::atomic< T * > & src ) noexcept;
 template< class T >
 bool try_protect( T *& ptr , const std::atomic< T * > & src ) noexcept;
 template< class T >
 void reset_protection( const T * ptr ) noexcept;
 void reset_protection( std::nullptr_t = nullptr ) noexcept;
 void swap( hazard_pointer & o ) noexcept;
 };
hazard_pointer make_hazard_pointer( void );
template< class T , class D = std::default_delete< T > >
class hazard_pointer_obj_base
{
 public:
 void retire( D d = D() ) noexcept;
 };
```

As to the costs, the benchmark `concurrent` measures them (case `reclaim`)
against the queue of Michael and Scott of xenium, under its hazard pointers,
its epochs and its DEBRA, and against `boost::lockfree::queue`, which keeps its
nodes on a free list and frees none. We expect the epochs to be (somewhat)
faster when no thread stops, since a guard pays one exchange per operation. The
hazard pointers pay one exchange per node protected (two per dequeue) and the
scans; however, they are the only choice when a thread may stop inside an
operation and the memory must stay bounded. Under `FraserEpochs< 1 >` the stack
and the queue read all the records at each pop, and their default $R = 64$
amortizes that read at the price of more nodes waiting.

## 24.15 Discussion and references {#sec-24-15}

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

We take the queue of one producer and one consumer from [Lamport
1983](Z-bibliography.md#ref-lamport1983), and its engineering for the
caches from [Lee et al. 2010](Z-bibliography.md#ref-lee2010) and from B-Queue
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
it. We take the union-find from [Jayanti and Tarjan
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

The MultiQueue is that of [Rihani et al.
2015](Z-bibliography.md#ref-rihani2015), with the engineering of [Williams et
al. 2021](Z-bibliography.md#ref-williams2021) (and the later account of
[Williams and Sanders 2025](Z-bibliography.md#ref-williams2025)). Its rank
errors are bounded by [Alistarh et al.
2017](Z-bibliography.md#ref-alistarh2017) and, exactly in the sequential model,
by [Walzer and Williams 2025](Z-bibliography.md#ref-walzer2025). For the event
count we follow [Reed and Kanodia 1979](Z-bibliography.md#ref-reed1979); the
asymmetric fences are those of [Ladan-Mozes et al.
2011](Z-bibliography.md#ref-ladanmozes2011). The priority queues that are
stolen combine the steal of half of [Hendler and Shavit
2002](Z-bibliography.md#ref-hendler2002) with the receiver-initiated steals of
[Acar et al. 2013](Z-bibliography.md#ref-acar2013), which the laws of order of
[Attiya et al. 2011](Z-bibliography.md#ref-attiya2011) motivate, and with the
balancing of the best subproblems of the parallel branch-and-bound ([Tschöke et
al. 1995](Z-bibliography.md#ref-tschoke1995); [Karp and Zhang
1993](Z-bibliography.md#ref-karp1993); [Sanders
1998](Z-bibliography.md#ref-sanders1998)). [Wimmer et al.
2014](Z-bibliography.md#ref-wimmer2014) and [Postnikova et al.
2022](Z-bibliography.md#ref-postnikova2022) study other ways of stealing
priority work. The exact queue without locks is that of [Lindén and Jonsson
2013](Z-bibliography.md#ref-linden2013), on the skiplists of [Pugh
1990](Z-bibliography.md#ref-pugh1990) and the marked pointers of [Harris
2001](Z-bibliography.md#ref-harris2001), after the queues of [Lotan and Shavit
2000](Z-bibliography.md#ref-lotan2000) and of [Sundell and Tsigas
2005](Z-bibliography.md#ref-sundell2005); its reclamation by epochs is that of
[Fraser 2004](Z-bibliography.md#ref-fraser2004), compared with the hazard
pointers and the other schemes by [Hart et al.
2007](Z-bibliography.md#ref-hart2007). The hazard pointers are those of
[Michael 2004](Z-bibliography.md#ref-michael2004), with the interface that
C++26 adopted as `std::hazard_pointer`; the queue of nodes is that of
[Michael and Scott
1996](Z-bibliography.md#ref-michael1996), and the stack that of [Treiber
1986](Z-bibliography.md#ref-treiber1986). A comparison of these structures with
those of other libraries (e.g., oneTBB, Folly, the queues of FastFlow) belongs
to the benchmarks of appendix B, made on an idle machine, and it is not
reported here.

## 24.16 Exercises {#sec-24-16}

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
   `engineered.concurrent-cuckoo-threads`,
   `engineered.concurrent-cuckoo-growth`.
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
14. Realize the skiplist of Lindén and Jonsson with its memory reclaimed by
    epochs, check that every pair comes out once with $p$ threads, and find
    a linearization of short histories of the operations by the search of
    Wing and Gong. Tests: `engineered_queues.lockfree-pq`,
    `engineered_queues.lockfree-pq-threads`,
    `engineered_queues.lockfree-pq-linearizable`.
15. Realize the hazard pointers and the epochs of Fraser as
    policies of the stack of Treiber and of the queue of Michael and Scott,
    and show that a thread stopped inside an operation keeps the nodes
    waiting bounded under the former and unbounded under the latter.
    Tests: `engineered_queues.hazard-pointers`,
    `engineered_queues.hazard-pointers-stalled`,
    `engineered_queues.epoch-reclaim`,
    `engineered_queues.lockfree-stack-threads`,
    `engineered_queues.lockfree-queue-threads`,
    `engineered_queues.reclaim-races`.

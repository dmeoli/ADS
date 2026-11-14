# 26. Distributed computing {#ch-26}

[Source: `include/ADS/distributed/DistributedSort.h`, `MPIExecutor.h`,
`MPIMailbox.h`]

We are concerned in this chapter with the computations whose data are
spread over the processes of MPI ([Message Passing Interface Forum
2023](Z-bibliography.md#ref-mpi2023)), each with a separate memory, which exchange them only by messages. In `ADS/distributed`, the headers
are compiled only with `ADS_WITH_MPI` (the option of CMake of the same
name, AUTO by default, i.e., on whenever an implementation of MPI such as
Open MPI or MPICH is found, and the macro of the makefiles). They give a
sort of the items of all the processes ([§26.2](26-distributed.md#sec-26-2)),
and the runtime that lets the processes act as the workers of one
computation: an executor and a distributed minimum
([§26.3](26-distributed.md#sec-26-3)), and a mailbox
([§26.4](26-distributed.md#sec-26-4)) that also detects the termination of
the computation ([§26.5](26-distributed.md#sec-26-5)). As for the threads inside a process, and the mailbox of the threads of which the one of the
processes is the counterpart, are those of chapter 25.

## 26.1 The model {#sec-26-1}

A distributed computation runs on $P$ processes, numbered by their
*ranks* $0 , \ldots , P - 1$ in a communicator, each with a separate memory and a part of the input; a process reads the data of another only
by receiving a message. A *collective* operation is one that all the
processes of the communicator call, each with its part, e.g., the
gathering of a value of each process in all of them (`MPI_Allgather`), or
the exchange in which each process sends a piece of its data to each of the others (`MPI_Alltoall`, and `MPI_Alltoallv` for pieces of different
sizes). We take the cost of a message of $b$ bytes as $\alpha + \beta b$,
where $\alpha$ is the latency and $\beta$ the time per byte. We then take as the cost of an algorithm its local work, its number of messages and its volume
of communication, all per process, i.e., along its slowest process.
Throughout the chapter $n_i$ is the number of the items of the process $i$
and $n = \sum_i n_i$.

The model leaves out the topology of the network, the contention of the
messages on the links, and the overlap of the communication with the work
(which the operations that do not block try to obtain, cf.
[§26.3](26-distributed.md#sec-26-3) to [§26.5](26-distributed.md#sec-26-5)); it is nevertheless the usual one for the analysis of the
collective operations, and the costs below are stated in it. We leave the measurements on a cluster to the benchmarks of the appendix B.

## 26.2 Sorting by regular sampling {#sec-26-2}

A sort of the items spread over the processes leaves in the process of
rank $j$ a sorted vector whose items are not smaller than those of the
ranks before $j$; the vectors in the order of the ranks are then the
sorted items of all, and their sizes change. Before the sort, a process
typically holds items of all the parts of the final order, and hence any
distributed sort moves most items to another process: the volume of
communication is at least about $n / P$ per process. Hence the aim is to send each item once, in one exchange, and to end with parts of about
$n / P$ items.

`sort(comm, A, less)` of `DistributedSort.h` is the parallel sort by
regular sampling of [Shi and Schaeffer 1992](Z-bibliography.md#ref-shi1992),
in one round of communication ([Algorithm 26.1](26-distributed.md#alg-26-1)).
Each process sorts its $n_i$ items and takes $P - 1$ samples at the ranks
$\lfloor ( j + 1 ) n_i / P \rfloor$, one every $w = n_i / P$ items. The samples, gathered in all the processes by `MPI_Allgather` and sorted, give the
$P - 1$ splitters, every $( P - 1 )$-th of them. Each process then cuts
its sorted items at the splitters by binary searches, sends to the process
$j$ its piece $j$ by one `MPI_Alltoallv` (after an `MPI_Alltoall` of the
sizes of the pieces), and merges the $P$ sorted pieces that it receives by
`multiwayMerge()` of `ADS/sort`. Since the items travel as bytes, their type
must be trivially copyable. [Figure 26.1](26-distributed.md#fig-26-1)
follows the algorithm on three processes.

<span id="alg-26-1"></span>**Algorithm 26.1.** Sort by regular sampling
([Shi and Schaeffer 1992](Z-bibliography.md#ref-shi1992)), on an executor
$\mathit{ex}$ inside each process, and without one.

--8<-- "generated/DistributedSort-sort.pseudo.md"

--8<-- "generated/DistributedSort-sort-2.pseudo.md"

![Figure 26.1. Sort by regular sampling on $P = 3$ processes of nine items each: the samples at the places 3 and 6 of each process (shaded) give the sorted samples 8, 10, 15, 17, 19, 24, and hence the splitters 15 and 19; each process cuts its items before 15 and before 19 (thick lines), and after the exchange and the merge the processes hold 14, 4 and 9 items, fewer than $2 n / P = 18$ each.](../figures/distributed-sort.svg){#fig-26-1}

Since all the processes cut at the same splitters, the items received by
the process $j$ are those of all the processes in its interval, and their
merge is sorted; hence the vectors in the order of the ranks are the sorted
items. Regular sampling owes its name to the bound on the sizes of the parts.

<span id="lem-26-1"></span>**Lemma 26.1 (regular sampling).** *With
$n / P$ items in each process and distinct keys, each process receives
fewer than $2 n / P$ items.*

*Proof.* The interval of a process holds the $P - 1$ samples between two
consecutive splitters; let $t_i$ be those of the process $i$, so that
$\sum_i t_i \le P - 1$. The items of the process $i$ in the interval lie
between its sample before the first of its $t_i$ samples and its sample
after the last one, i.e., in at most $t_i + 1$ of its gaps of
$w = n / P^2$ items. Hence the interval holds at most $\sum_i ( t_i + 1 )
w \le ( 2 P - 1 ) n / P^2 < 2 n / P$ items. $\square$

The bound is tight up to a constant, but in practice the parts are
usually far closer to $n / P$ ([Shi and Schaeffer
1992](Z-bibliography.md#ref-shi1992)). With repeated keys the lemma may
fail, since the items of one key all fall in one interval; `sort()`
therefore cuts the items equal to a run of $k$ equal splitters into
$k + 1$ parts of the same size (line 10), and a key repeated many times is
thus spread over as many processes as it fills. Also, the merge keeps the result sorted, since the parts of the run are of equal items.

```cpp
--8<-- "generated/DistributedSort-cuts.cpp"
```

A process with no items takes no samples (as its flag, gathered first, records), and the splitters come from the samples of the others; when no process
has items, all the cuts are at the end. Finally, the last loop makes the cuts nondecreasing, which the equal runs of different processes may otherwise
violate by one place.

<span id="lem-26-2"></span>**Lemma 26.2.** *With $n / P \ge P^2$ items
in each process, `sort()` makes $O( ( n / P ) \log n )$ work per process,
and its communication takes*

$$
O( \alpha P + \beta ( n / P + P^2 ) ) \tag{26.1}
$$

*time with a direct all-to-all, i.e., $O( \alpha P + \beta n / P )$.*

*Proof.* The work is the local sort, $O( ( n / P ) \log( n / P ) )$, the
sort of the $P ( P - 1 )$ samples, $O( P^2 \log P )$, the $P - 1$ pairs of
binary searches, and the merge of $P$ pieces of $O( n / P )$ items in all
(by [Lemma 26.1](26-distributed.md#lem-26-1)), $O( ( n / P ) \log P )$;
with $n / P \ge P^2$ the sum is $O( ( n / P ) \log n )$. As for the volume of the communication, it is that of the samples, $P^2$ items to each process, of the
sizes of the pieces, one integer for each pair of processes, and of the
pieces, each item sent once, i.e., $O( n / P )$ items per process by
[Lemma 26.1](26-distributed.md#lem-26-1). In a direct all-to-all each
process sends $P - 1$ messages, which gives (26.1). $\square$

Besides the gathering of the samples, the exchange of the sizes and that of
the pieces, which carry the volume, `sort()` makes two small collective
operations: the gathering of the flags of the empty processes, and a
reduction that checks that the bytes of each piece fit the counts of MPI
(the `int` before MPI 4, whose `MPI_Alltoallv_c` has counts of 64 bits).
If they do not fit in some process, all the processes throw
`std::length_error`, instead of passing MPI a number wrapped around or
leaving the others to wait in the exchange. An executor of threads
(chapter 25) may also be given: the local sort and
the merge then run on its threads, with MPI between the nodes and threads
within each, the hybrid scheme of the clusters of today.

Beyond some thousands of processes, the latency $\alpha P$ and the $P^2$
samples dominate (26.1). In this regime, the multi-level sorts of [Axtmann et al.
2015](Z-bibliography.md#ref-axtmann2015) group the processes and
exchange the items in $O( \log P )$ levels, each item sent once per level,
which trades the $P$ messages of each process for $O( P^{1 / \ell} )$
messages per level over $\ell$ levels; ADS does not realize them.

## 26.3 Processes as workers {#sec-26-3}

`MPIEnvironment` starts MPI for the life of the object, by
`MPI_Init_thread()` at the funneled level unless it is already started,
and ends it by `MPI_Finalize()` if it started it; the funneled level (only
the thread that started MPI calls it) suffices for the mailbox.
`MPIExecutor` holds a communicator and gives the rank of the process and
the number of the processes, and its `run(f)` makes the call `f(rank)`,
i.e., the worker of the process. A program written for the workers of
`run()` of the executors of threads thus runs with one worker per process, all the processes calling `run()`; we remark that `run()` returns
when the worker of the calling process returns, and `barrier()` is the
synchronization of all of them.

A search in which each process improves a bound (e.g., the value of the
best solution found so far) needs the minimum over the processes of a
value of each. `DistributedMin` starts a reduction of the minimum that
does not block, `MPI_Iallreduce` ([Hoefler et al.
2007](Z-bibliography.md#ref-hoefler2007)), and tests it while the process
works ([Algorithm 26.2](26-distributed.md#alg-26-2)); `test()` gives the
minimum when the reduction has completed, and `wait()` waits for it.
Hence a process learns the bounds of the others at most one reduction
late, with no message for each improvement. Since a reduction is a
collective operation, all the processes must start the same number of them,
and destroy the object after the last one has completed, which are the
preconditions of the class.

<span id="alg-26-2"></span>**Algorithm 26.2.** Start of a reduction of
`DistributedMin`.

--8<-- "generated/MPIExecutor-DistributedMin-start.pseudo.md"

## 26.4 The mailbox {#sec-26-4}

In a distributed computation each process typically owns a part of the
data, e.g., the items whose keys hash to its rank (`ownerOf(k, P)` of
`Mailbox.h` in `ADS/parallel`), and it sends to the owner what concerns
them, as the nodes of a search that the other processes generate. Such
messages are many, small, and sent at unpredictable times, and a message
of MPI for each of them would pay the latency $\alpha$ each time. The
mailbox of `MPIMailbox.h` therefore gathers the messages of a process for
each rank into an outbox $O_{\mathit{to}}$, and sends it as one message
of MPI when it holds `batch` messages ([Algorithm 26.3](26-distributed.md#alg-26-3)):
$k$ messages of $b$ bytes then cost $\alpha + \beta k b$ instead of
$k ( \alpha + \beta b )$. Its price is the latency of a single message,
which grows by the time its batch takes to fill; `flush()` sends the
outboxes that are not empty, and cuts it.

<span id="alg-26-3"></span>**Algorithm 26.3.** Send, post, flush and poll
of `MPIMailbox`; $s$ and $r$ count the messages sent and received by the
process.

--8<-- "generated/MPIMailbox-send.pseudo.md"

--8<-- "generated/MPIMailbox-flush.pseudo.md"

--8<-- "generated/MPIMailbox-poll.pseudo.md"

A batch is posted by `MPI_Isend`, which does not block: its buffer stays,
with its request, in a list of the sends in flight until MPI has completed
it, which `MPI_Testsome` finds at each poll ([Figure 26.2](26-distributed.md#fig-26-2)).
Hence a process never waits for a receiver. Incoming batches are found by `MPI_Iprobe` from any source and received by `MPI_Recv`, at
most 64 batches per poll, and `poll(f)` calls `f(x)` on each of their
messages; the messages of a process to itself do not go through MPI. Note that the mailbox duplicates the communicator, and its messages and reductions
therefore never meet those of the program. This is the asynchronous
mailbox of the library YGM of [Priest et al.
2019](Z-bibliography.md#ref-priest2019); the exchange NBX of [Hoefler et
al. 2010](Z-bibliography.md#ref-hoefler2010) solves instead a single round
of sparse messages, by synchronous sends and a barrier that does not
block.

![Figure 26.2. The outboxes of the process 1 of three, with `batch` = 4, before and after `send(2, z)`, which fills the outbox of the rank 2: the batch is posted by `MPI_Isend`, and its buffer stays on the list of the sends in flight until MPI completes it.](../figures/distributed-outbox.svg){#fig-26-2}

**Forms of the messages.** `MPIMailbox<Msg>` of a trivially copyable
`Msg` sends its batches as arrays of messages, which MPI copies as bytes
(256 messages per batch by default); several kinds of such messages can
travel as one `std::variant`, which is trivially copyable as well. Messages of other
types, or of lengths that vary (e.g., the nodes of a search with a state
of variable size), are the records of `MPIByteMailbox`: each one a tag of
32 bits, a length of 32 bits and the bytes, padded to a multiple of 8,
packed into a batch until it holds `batch` bytes (65536 by default),
[Figure 26.3](26-distributed.md#fig-26-3) and
[Algorithm 26.4](26-distributed.md#alg-26-4). Finally, `MPIMailbox<Msg>`
of a type that is not trivially copyable is an `MPIByteMailbox` whose
bytes are those of `MPICodec<Msg>`, the customization point that encodes
and decodes a message, given for `std::string` and for the vectors of
trivially copyable items. Whatever their form, the messages are counted
one by one, and the waves of [§26.5](26-distributed.md#sec-26-5) see all
of them.

![Figure 26.3. Two records of a batch of `MPIByteMailbox`: a message of tag 7 and 5 bytes, padded to 8, and an empty message of tag 2.](../figures/distributed-records.svg){#fig-26-3}

<span id="alg-26-4"></span>**Algorithm 26.4.** Send and poll of
`MPIByteMailbox`.

--8<-- "generated/MPIMailbox-MPIByteMailbox-send.pseudo.md"

--8<-- "generated/MPIMailbox-MPIByteMailbox-poll.pseudo.md"

## 26.5 Termination {#sec-26-5}

A process is *active* while it has work and *passive* otherwise; it
becomes active only by receiving, and it sends only while it is active.
The computation has terminated when all the processes are passive and no
message is on its way, a global condition that no single process can see. Each process counts the messages it has sent, $s_i$, and those it
has received, $r_i$, both of which only grow. A *wave* reads the counters
of all the processes and sums them, $S = \sum_i s_i$ and $R = \sum_i r_i$;
here it is a reduction that does not block (`MPI_Iallreduce`), to which a
process contributes the pair $( s_i , r_i )$ copied when it is passive,
i.e., from `quiescent()`. While the wave is in flight the process keeps
receiving and working, and it starts a wave only after the previous one
has completed in it, which happens only after all the processes have
contributed to it ([Algorithm 26.5](26-distributed.md#alg-26-5)).

<span id="alg-26-5"></span>**Algorithm 26.5.** The detection of the
termination in `quiescent()`, where $R'$ is the sum of the receipts of the
previous wave and $\sigma$ the number of the processes stopped.

--8<-- "generated/MPIMailbox-quiescent.pseudo.md"

<span id="lem-26-3"></span>**Lemma 26.3 (four counters).** *Let the wave
$k - 1$ read the receipts $R_{k - 1}$ and the next wave $k$ the sends
$S_k$. If*

$$
S_k = R_{k - 1} , \tag{26.2}
$$

*the computation had terminated when the last process contributed to the
wave $k - 1$; conversely, once the computation has terminated, (26.2)
holds at most two waves later, and all the processes stop at the same
wave.*

*Proof.* Let $T$ be the moment of the last contribution to the wave
$k - 1$, which precedes all the contributions to the wave $k$, and $S( T )$,
$R( T )$ the messages sent and received by all at $T$. The counters only
grow, the receipts of the wave $k - 1$ were read at or before $T$ and the
sends of the wave $k$ at or after it, and a message is received after it
is sent; hence $R_{k - 1} \le R( T ) \le S( T ) \le S_k$, and (26.2) makes
them all equal. Thus at $T$ no message was on its way ($R( T ) = S( T )$),
and no process had received a message after it contributed to the wave
$k - 1$ ($R_{k - 1} = R( T )$). Each process was passive when it
contributed, and a passive process becomes active only by receiving;
hence all were passive at $T$, which is the termination. Conversely, after the termination each process eventually finds no work and contributes to
each wave, and the counters stop growing; two waves later (26.2) holds.
Since all the processes get the same sums, all the processes stop at the same
wave. $\square$

This is the method of the four counters of [Mattern
1987](Z-bibliography.md#ref-mattern1987). A wave costs a reduction of a
few integers, $O( \alpha \log P )$ time, and the termination is therefore
known $O( \alpha \log P )$ after it happens, with no message per message
sent (the acknowledgments of [Dijkstra and Scholten
1980](Z-bibliography.md#ref-dijkstra1980)) and no process that coordinates
the waves. We remark that a single wave would not do: a message sent by a
process before its contribution and received by another after the contribution of the latter is counted in $S$ but not in $R$, while one received before
and sent after is counted in neither, and the two errors may cancel.
[Table 26.1](26-distributed.md#tab-26-1) follows the waves on a small
computation.

<span id="tab-26-1"></span>**Table 26.1.** Waves on two processes: the
process 0 sends one message to the process 1 and becomes passive, while
the process 1, passive, contributes to the first wave before the message
arrives; the test (26.2) fails at the second wave and holds at the third.

| Wave | $( s_0 , r_0 )$ | $( s_1 , r_1 )$ | $( S , R )$ | (26.2) |
|---|---|---|---|---|
| 1 | $( 1 , 0 )$ | $( 0 , 0 )$ | $( 1 , 0 )$ | (no previous wave) |
| 2 | $( 1 , 0 )$ | $( 0 , 1 )$ | $( 1 , 1 )$ | $S_2 = 1 \ne R_1 = 0$ |
| 3 | $( 1 , 0 )$ | $( 0 , 1 )$ | $( 1 , 1 )$ | $S_3 = 1 = R_2$: end |

At the core of the test is the function `wave()`, which `quiescent()` calls
after it has flushed the outboxes:

```cpp
--8<-- "generated/MPIMailbox-wave.cpp"
```

**The stop.** A process may also end the whole computation before its
termination (e.g., by a limit on the work, or by a bound that closes the
search), by `stop()`. It sends a *note*, an empty batch, to each other
process, and from then on it neither posts nor receives; a process whose
poll finds a note stops in the same way, and leaves its work for
`quiescent()`. A stopped process contributes to the waves the triple
$( s_i , r_i , 1 )$, the others $( s_i , r_i , 0 )$, and the third sum
$\sigma$ of a wave counts the processes stopped when they contributed to
it.

<span id="lem-26-4"></span>**Lemma 26.4 (the stop).** *If a process calls
`stop()`, all the processes see the end at the same wave, at most two waves
after the last one has received the note; if $\sigma = P$ for a wave, all
the processes had stopped when they contributed to it.*

*Proof.* The notes are messages of MPI, which delivers them; a process
that has not stopped polls while it works, or in `quiescent()`
while it waits, and stops when it receives its note. Once stopped, a
process stays stopped, and since it receives no more messages its work does not grow, and it calls `quiescent()` until the end. Hence the first wave started by all after the last of them has stopped has $\sigma = P$, and each process, which has the same sums, sees the end at that wave. The
waves are started by each process only after the previous one has
completed, and they are thus the same collective operations in the same
order on all the processes, also when the end comes from (26.2) instead
of from $\sigma = P$. $\square$

**The leftovers.** After the end, `leftovers(f)` hands to $f$ the
messages that no poll has handed: those still on their way to the
process, those to itself and those of its outboxes
([Algorithm 26.6](26-distributed.md#alg-26-6)). Those on their way are known by counting. Each process keeps the messages $m_{i , q}$ and the notes
$n_{i , q}$ that it has sent by MPI to each process $q$, and one
reduction that scatters its sums (`MPI_Reduce_scatter_block`) gives each
process $q$ the totals

$$
M_q = \sum_i m_{i , q} , \qquad N_q = \sum_i n_{i , q} \tag{26.3}
$$

of what was sent to it. Then the process receives, by blocking calls,
until it has received $M_q$ messages and $N_q$ notes in all, and waits for the completion of its sends, which their receivers are receiving in the same way.

<span id="alg-26-6"></span>**Algorithm 26.6.** The messages left after
the end.

--8<-- "generated/MPIMailbox-leftovers.pseudo.md"

<span id="lem-26-5"></span>**Lemma 26.5 (each message once).** *After
`leftovers()` on all the processes, each message given to `send()` on any
process has been handed to a function exactly once, by a `poll()` or by
`leftovers()`.*

*Proof.* A message given to `send()` is in an outbox until its batch is
posted, then either among the messages of the process to itself or on its
way by MPI, and finally it is handed by the poll that receives it. A stopped process posts no batch, and what it holds in its outboxes or for
itself is handed by its `leftovers()`. Furthermore, the messages posted by MPI to $q$ are
counted in $M_q$ of (26.3), and $q$ receives until it has received $M_q$
messages (those of its polls included), i.e., those of the batches posted
to it, since MPI delivers each message once. $\square$

The cost is one reduction of $2 P$ integers and the messages left, i.e.,
$O( \alpha \log P + \beta P )$ time besides them, paid once at the end;
during the computation it costs only the counters of each send. If
`leftovers()` is not called, the destructor makes the same exchange and
drops the messages, so that no send stays in flight when the duplicate of
the communicator is freed.

## 26.6 Discussion and references {#sec-26-6}

The sort by regular sampling is that of [Shi and Schaeffer
1992](Z-bibliography.md#ref-shi1992), and its multi-level successors for
many thousands of processes are those of [Axtmann et al.
2015](Z-bibliography.md#ref-axtmann2015). The detection of the termination
by waves of counters is one of the algorithms of [Mattern
1987](Z-bibliography.md#ref-mattern1987), which collects the earlier ones,
among which that of [Dijkstra and Scholten
1980](Z-bibliography.md#ref-dijkstra1980) for the diffusing computations;
here the waves are reductions that do not block ([Hoefler et al.
2007](Z-bibliography.md#ref-hoefler2007)), which spares the process that
coordinates them in the original. The mailbox with batches follows that of
YGM ([Priest et al. 2019](Z-bibliography.md#ref-priest2019)), and the
exchange of a single round of sparse messages, which ADS does not realize,
is the NBX of [Hoefler et al. 2010](Z-bibliography.md#ref-hoefler2010).
Our tests run each program on 1 to 4 processes, which checks the
correctness of the protocols (including the stop and the leftovers on a
random traffic) but it does not measure their scaling; the latter is left to
the benchmarks of the appendix B.

## 26.7 Exercises {#sec-26-7}

Each exercise names the test of the library that solves it; the tests run
on 1 to 4 processes, as `engineered_mpi.<test>.np<P>`, and exist only when
MPI is found (run them with `ctest -R <name>`).

1. Sort the items of $P$ processes by regular sampling, also when some
   processes are empty, when all the items are on one process, and when
   the keys are repeated many times, with and without an executor of
   threads inside each process. Test: `engineered_mpi.distributed-sort.np4`.
2. Check that the sizes of the pieces fit the counts of MPI, and stop all
   the processes when one of them does not. Test:
   `engineered_mpi.distributed-sort-counts.np4`.
3. Run one worker per process, and synchronize the processes by a barrier.
   Test: `engineered_mpi.mpi-executor.np4`.
4. Send messages of a fixed size in batches between random processes, and
   detect the end of the traffic. Test: `engineered_mpi.mpi-mailbox.np4`.
5. Visit a tree whose nodes are sent to their owners, and detect the
   termination by the four counters. Test:
   `engineered_mpi.mpi-termination.np4`.
6. Send messages of variable length with a tag, and messages of a type
   that is not trivially copyable through a codec. Tests:
   `engineered_mpi.mpi-bytes.np4`, `engineered_mpi.mpi-codec.np4`.
7. Stop a computation from one process, whatever the messages on their
   way, and hand each message left exactly once. Test:
   `engineered_mpi.mpi-stop.np4`.
8. Keep the minimum over the processes of a bound that each improves, by
   reductions that do not block. Test: `engineered_mpi.mpi-min.np4`.

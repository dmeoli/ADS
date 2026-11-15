# Appendix B. The benchmarks {#app-B}

[Source: `bench/*.cpp`, `bench/Bench.h`, `bench/Texts.h`, `bench/run.sh`,
`bench/tables.py`, `bench/summary.py`, the section Performance of the
README, `results/README.md`]

We are concerned in this appendix with the programs of `bench/`, which
time the structures and the algorithms of ADS against those of the
standard library and of the libraries of reference in each field, and
with the method by which they are run. No chapter of this manual gives a
measured time: wherever a comparison belongs, the chapter leaves it to a
campaign on an idle machine. This appendix fixes what that campaign runs,
how a cell is measured and how its noise is estimated, how a campaign is
reproduced, and where each chapter reports its numbers. At the time of
writing the campaign has not been run; hence the tables below give the
structure of the results, and their values will be filled in from the
files that `bench/run.sh` writes in `results/`
([§B.7](B-benchmarks.md#sec-B-7)). We gave in
[§A.7](A-writing-structure.md#sec-A-7) the rules by which a new structure
enters a benchmark.

## B.1 The programs {#sec-B-1}

There are 45 programs of timing in `bench/`, one per family of structures or of
algorithms, each a file `bench/<name>.cpp` built as `bench-<name>` (by
`make bench`, or with the CMake option `-DADS_BENCHMARKS=ON`, which builds
all of them but `compression`, `succinct` and `mpi`). Each
program states in its `@file` comment the cases, the sizes, the unit of a
cell and what the clock excludes. It also lists the competitors,
each with the macro or the variable of the makefile that compiles it in
when its library is found. [Table B.1](B-benchmarks.md#tab-B-1)
summarizes them, with the chapter whose comparison each program measures.
The two further sources `bench/zerocost.cpp` and `bench/zerocost_asm.cpp`
measure no time; the script `bench/zerocost.py` compiles them at `-O2` with
each compiler found, counts by callgrind the instructions of one change of
a structure that publishes its changes, and compares the machine code of each
change with that of the plain structure byte by byte, so that the claim of
[§1.13](01-introduction.md#sec-1-13) that a change nobody listens to costs no instruction is checked by counts and by code, which do not depend on the load
of the machine. With `--before REV` the script also compiles them on the
headers of an earlier revision, and compares there and here the count of
each workload with no observer and the code of every function, e.g., of
`IndexedDaryHeap` ([§1.15.1](01-introduction.md#sec-1-15-1)) and of
`JoinTree` ([§8.12.7](08-tree.md#sec-8-12-7)).

Two auxiliary headers serve the programs. One is `bench/Bench.h`, with the
timing, the options and the output that all of them share
([§B.2](B-benchmarks.md#sec-B-2)), the selection of the structures of
[§B.4](B-benchmarks.md#sec-B-4), and `keys( n , seed )`, which gives $n$
distinct random keys of 63 bits (from `std::mt19937_64`, so that they fit a
signed key too); while `bench/Texts.h` holds the texts of the benchmarks on
strings, each of $n$ bytes from a seed: random bytes over alphabets of 4, 26
and 256 symbols, words of a Zipf vocabulary with the frequencies of the
English letters, the Fibonacci word, a highly repetitive text, and the
repeated byte on which the naive search takes its worst case.

<span id="tab-B-1"></span>**Table B.1.** The programs of `bench/`: what each
one measures, the structures of other libraries it is compared with (those of
Boost whenever its headers are found, those in parentheses only when their
library is given to `make bench`, as stated in the `@file` comment of the
program), and the chapter where its comparison belongs.

| Program | What it measures | Compared with | Chapter |
|--------|--------------|----------------|------|
| `allocators` | `PoolAllocator` in the node containers and in `std::allocate_shared`, also freed by another thread | `std::allocator`, `std::pmr` pools, `boost::fast_pool_allocator`, (mimalloc) | [§5.11](05-memory.md#sec-5-11) |
| `arrays` | lower bound in sorted arrays, selection by rank, range minima | `std::lower_bound`, Eytzinger layout, `std::nth_element`, (PGM-index, miniselect, sdsl-lite) | [§6.10](06-search.md#sec-6-10) |
| `assembly` | sparse matrices by columns from triplets, rows of cuts appended and removed | `std::map`, a vector of rows, (Eigen) | [§17.11](17-numeric.md#sec-17-11) |
| `assignment` | the assignment from scratch and after the change of a row, `maxWeightMatching()` | Boost `maximum_weighted_matching`, (LEMON `NetworkSimplex`) | [§14.14](14-graph.md#sec-14-14) |
| `backtrack` | dancing links on exact covers; clause learning, A* and subset sum on the structures of each library | `ExactCover` of the book, `std`, Boost, (DLX1, DLX2, SSXCC of Knuth) | [§19.8](19-backtrack.md#sec-19-8) |
| `bestfirst` | the closed set, the addressable frontier, the ordered frontier and the packed arrays of a best-first search | `std::unordered_set`, `std::set`, `std::vector< bool >`, Boost.Heap, `boost::unordered_flat_set`, `boost::dynamic_bitset` | [7](07-dictionary.md#ch-7), [8](08-tree.md#ch-8), [§9.13](09-priority.md#sec-9-13), [22](22-succinct.md#ch-22) |
| `bignum` | products, quotients, conversion, gcd and square roots of long integers, sums of rationals | `boost::multiprecision::cpp_int`, `boost::rational`, (GMP, FLINT) | [§17.11](17-numeric.md#sec-17-11) |
| `combinatorial` | population counts, Morton codes, permutations and combinations | `std::popcount`, `std::next_permutation`, `boost::dynamic_bitset`, (libpopcnt, libmorton, Hinnant's combinations) | [§18.10](18-combinatorial.md#sec-18-10) |
| `compression` | entropy coders, codes of integers, BWT, FM-index and r-index, LZ parses, space | (huff0, FSE, Stream VByte, FastPFor, sdsl-lite, libsais, LZ4, ryg_rans, r-index of Prezza) | [§21.13](21-compression.md#sec-21-13) |
| `concurrent` | queues between two threads, work-stealing deques, the wait of a consumer | Boost.Lockfree, `std::deque` under a mutex, (rigtorp, moodycamel, folly) | [§24.14](24-concurrent.md#sec-24-14) |
| `dictionaries` | insertions, lookups and visits of hash tables and search trees | `std::unordered_map`, `std::map`, `boost::unordered_flat_map`, `boost::container::flat_map` | [§7.6](07-dictionary.md#sec-7-6), [8](08-tree.md#ch-8) |
| `dynamic` | the observers that keep shortest paths, spanning forests and flows after a change; link-cut trees; union-find with rollback; offline connectivity | the algorithms of the book from scratch, a vector of parents, `UnionFind` | [§14.14](14-graph.md#sec-14-14), [8](08-tree.md#ch-8), [§11.6](11-mfset.md#sec-11-6) |
| `edit` | edit distance, longest common subsequence, approximate search | the dynamic programming of the textbook, (edlib, rapidfuzz-cpp) | [§15.9](15-string.md#sec-15-9) |
| `external` | multiway merges, the mergesort of the notes in two-level memory, random sampling | `std::priority_queue`, `__gnu_parallel::multiway_merge`, `std::ranges::sample`, Boost.Heap, (tlx) | [§23.8](23-external.md#sec-23-8) |
| `fingerprints` | hashes of rows of doubles, the comparison of two rows | `std::hash` combined, `std::equal`, `boost::hash_range` | [7](07-dictionary.md#ch-7) |
| `flows` | maximum flows from scratch and after a change of a capacity | Boost `push_relabel_max_flow`, (LEMON `Preflow`) | [§14.14](14-graph.md#sec-14-14) |
| `geometry` | convex hull, closest pair | Boost.Geometry, (CGAL) | [§16.5](16-geometry.md#sec-16-5) |
| `graphs` | visits, shortest paths, spanning trees, components, Eulerian trails, matchings, union-find, grid maps, each on the structures of `std`, Boost and ADS | the same code on `std` and Boost structures, Boost Graph Library | [§14.14](14-graph.md#sec-14-14), [§9.13](09-priority.md#sec-9-13), [§11.6](11-mfset.md#sec-11-6) |
| `greedy` | parallel Kruskal, spanning forest, independent set and matching by deterministic reservations | the sequential algorithms, (ParlayLib) | [§25.16](25-parallel.md#sec-25-16) |
| `hashattack` | the keys of exercise 5.11 of Morin's book against random keys | `std::unordered_map` | [7](07-dictionary.md#ch-7), [§B.6](B-benchmarks.md#sec-B-6) |
| `hashing` | hash functions, hash maps, filters, range filters, perfect hashing, concurrent maps | `std::hash`, `std::unordered_map`, Boost, (Abseil, unordered_dense, XXH3, wyhash, rapidhash, robin-map, libcuckoo, fastfilter_cpp, xor filters, PTHash, BBHash, RecSplit) | [§7.6](07-dictionary.md#sec-7-6), [§10.12](10-set.md#sec-10-12), [§24.14](24-concurrent.md#sec-24-14) |
| `joins` | union, intersection, difference, batch insertion and building of trees based on join, on $t$ threads | `std::map`, (PAM) | [8](08-tree.md#ch-8), [§25.16](25-parallel.md#sec-25-16) |
| `lce` | longest common extensions | the suffix array of the textbook, direct comparison, (sdsl-lite) | [§15.9](15-string.md#sec-15-9) |
| `learned` | LA-vector, PGM-index, dynamic PGM-index | Elias-Fano, `std::map`, `BTreeMap`, (sdsl-lite, la_vector, PGM-index) | [§6.10](06-search.md#sec-6-10), [§22.13](22-succinct.md#sec-22-13) |
| `matrix` | dense products and transposition | the products of the book, (Eigen, OpenBLAS) | [§17.11](17-numeric.md#sec-17-11) |
| `memory` | arenas, object pools, allocators of blocks of any size | `new`/`delete`, `std::pmr`, Boost.Pool, (mimalloc, jemalloc) | [§5.11](05-memory.md#sec-5-11) |
| `mpi` | the sort and the mailbox over the processes of MPI | none (the scaling over $P$) | [§26.6](26-distributed.md#sec-26-6) |
| `multi` | search of many patterns at once | `std::string_view::find()`, (Vectorscan) | [§15.9](15-string.md#sec-15-9) |
| `numeric` | gcd, modular powers, primality, factorization, generators, deviates, shuffles, alias tables, sampling | `std`, Boost, (GMP, hurchalla, PCG, Random123) | [§17.11](17-numeric.md#sec-17-11) |
| `ordered` | ordered dictionaries: insertions, lookups, lower bounds, removals, scans | `std::map`, `boost::container::flat_map`, Boost.Intrusive trees, (Abseil and tlx B-trees) | [8](08-tree.md#ch-8) |
| `parallel` | parallel sorts, patterns, merges, sparse and dense products, graphs, shortest paths, succinct builds, LCP, concurrent tables and queues, branch and bound, on $t$ threads | GNU parallel mode, OpenMP, (oneTBB, `std::execution` on oneTBB, IPS4o, ParlayLib, FastFlow, tlx, GAP benchmark suite, Eigen, OpenBLAS, sdsl-lite, libcuckoo, growt, MultiQueue, k-LSM, SprayList) | [§25.16](25-parallel.md#sec-25-16) |
| `positions` | indices from the addresses of objects to their positions | `std::unordered_map`, `FlatHashTable`, `boost::unordered_flat_map` | [7](07-dictionary.md#ch-7) |
| `priority` | heaps, sequence heap, bucket queues and radix heap in the hold model, double-ended queues, Dijkstra, best-first searches | `std::priority_queue`, `std::multiset`, Boost.Heap, (tlx, radix-heap) | [§9.13](09-priority.md#sec-9-13) |
| `queues` | queues, deques and stacks | `std::deque`, `std::vector`, `boost::circular_buffer`, `boost::container::devector` | [3](03-stack.md#sec-3-9), [4](04-queue.md#ch-4) |
| `runtime` | the channels, the mailbox and the termination of the long-lived workers | the workers of OpenMP, (oneTBB, FastFlow) | [§25.13](25-parallel.md#sec-25-13) |
| `search` | exact search of a pattern | `std::string_view::find()`, the searchers of `std`, `memmem()`, Boost.Algorithm | [§15.9](15-string.md#sec-15-9) |
| `sequences` | sequences, tiered and small vectors, stacks, rearrangements and sorts of lists | `std::vector`, `std::list`, `std::forward_list`, `std::deque`, `std::stack`, Boost.Container | [2](02-sequence.md#ch-2), [3](03-stack.md#sec-3-9), [§12.15](12-sort.md#sec-12-15) |
| `sets` | intersections of sorted lists, union-find, boolean vectors, compressed bitmaps | `std::set_intersection`, `boost::disjoint_sets`, `boost::dynamic_bitset`, (SIMD intersections of Lemire et al., CRoaring) | [§10.6](10-set.md#sec-10-6), [§10.12](10-set.md#sec-10-12), [§11.6](11-mfset.md#sec-11-6) |
| `sort` | sorts of integers, doubles, strings and records in six patterns | `std::sort`, `std::stable_sort`, Boost.Sort, (IPS4o, IPS2Ra, x86-simd-sort, ska_sort, fluxsort, crumsort, quadsort, vqsort) | [§12.15](12-sort.md#sec-12-15) |
| `sparse` | sparse products by a vector and transposition | the orthogonal lists of the book, (Eigen) | [§17.11](17-numeric.md#sec-17-11) |
| `ssets` | the sorted sets of Open Data Structures (exercise 9.14 of Morin's book) | `std::set`, (the code of Morin) | [7](07-dictionary.md#ch-7), [8](08-tree.md#ch-8), [§B.6](B-benchmarks.md#sec-B-6) |
| `stringdicts` | static dictionaries of strings by rank and by prefix | sorted `std::string`, (HAT-trie, marisa-trie, tx-trie, path-decomposed tries) | [7](07-dictionary.md#ch-7), [§15.9](15-string.md#sec-15-9) |
| `succinct` | rank and select, compressed bit vectors, Elias-Fano, balanced parentheses, wavelet structures, directly addressable codes, space | (sdsl-lite, sux, pasta, succinct) | [§22.13](22-succinct.md#sec-22-13) |
| `suffix` | suffix arrays and LCP arrays | the constructions of the textbook, (libsais, libdivsufsort) | [§15.9](15-string.md#sec-15-9) |
| `tries` | dictionaries of strings with enumeration by prefix | `std::map`, `std::unordered_map`, (Abseil, HAT-trie, libart) | [7](07-dictionary.md#ch-7) |

Three programs have more than one build or mode. On one side, `bignum`
and `matrix` measure `BigInteger`, `Rational`, `gemm()` and
`multiplyStrassen()` as they are built, i.e., on GMP and on a CBLAS where
those are found. On the other, `bench-bignum-own` and `bench-matrix-own`,
built with `ADS_BENCH_OWN` when the backends are on, measure the code of
ADS without them, under the labels `... own`. Also, `external` takes as
its first argument the part to run (`merge`, `book` or `sampling`).
Finally, `mpi` is not among the programs of `make bench`: one builds it
with `mpicxx` (the variable `MPICXX`) by `make build-make/bench-mpi` and
runs it by `mpirun` on $P$ processes, and with `-m` it measures the
mailbox in place of the sort.

Besides the options of `bench/Bench.h` ([§B.2](B-benchmarks.md#sec-B-2)), some
programs take options of their own, all of which default to running
everything. The programs of several threads take `-t`, the number of the
threads (1 by default in `parallel`, `joins`, `greedy` and `mpi`, 4 in
`runtime`); `parallel` also takes `-b`, one of its parts (`sort`, `patterns`,
`merge`, `spmv`, `gemm`, `graphs`, `sssp`, `succinct`, `lcp`, `hash`,
`closed`, `pq`, `rank` and `bnb`), and `-w`, one family of executors (`pool`,
`fastflow`, `ff`, `openmp`, `tbb`, `parlay`, `ips` or `seq`). Variables of the
environment select the rest: `ONLY=a|b|...` the structures whose label
contains one of the words (in `arrays`, `ordered`, `sets` and `tries`); `PART`
one part of `arrays` (`array search`, `select` or `rmq`) or of `sets`
(`intersection`, `mfset`, `boolset` or `bitmap`); `PARTS` the parts of
`hashing` named by their first words (e.g., `PARTS=filters , perfect`), and
`THREADS` the threads of its concurrent maps (`1 , 2` by default);
`ADS_GRAPHS` the benchmarks of `graphs`, separated by commas;
`ADS_BESTFIRST_CASE` one family of `bestfirst` (`closed`, `frontier`,
`ordered` or `packed`). In `parallel`, `ADS_SSSP_DELTA` and `ADS_SSSP_RHO` set
the parameters of the shortest paths (32 and $2^{20}$ by default, 256 and
$2^{12}$ on the grid), `ADS_SSSP_CASE` keeps one graph (e.g., `kronecker20`),
and `ADS_SSSP_DUMP` and `ADS_WAVELET_DUMP` name folders where the graphs and
the sequences are written for the programs of GBBS and of pwm. Finally,
`joins` sets `PARLAY_NUM_THREADS` to the number of its threads unless it is
given, and `numeric` built with the macro `ADS_BENCH_OLD` leaves out what the
previous release lacks.

## B.2 The measure of a cell {#sec-B-2}

A *cell* is one structure on one case at one size. It is measured by the
function `median( prepare , work , r , ops )` of `bench/Bench.h`, which
$r$ times calls `prepare()`, untimed, and then times `work()` on what
`prepare()` returned with `std::chrono::steady_clock`. Each time
is divided by the number `ops` of its operations, and the cell is the
median of the $r$ times, i.e., the element of rank $\lfloor r / 2
\rfloor$ found by `std::nth_element` (the true median for $r$ odd). We
prefer the median to the mean and to the minimum, since it discards both
the runs slowed by an interruption and those that a lucky state of the
caches made faster, and it needs no assumption on the distribution of
the times.

A result that the compiler could drop is added to the `volatile` variable
`sink`. Where a wrong result is possible, each program checks the results
of its structures against each other or against the standard library.
For instance, `sort` and `parallel` give $-1$ to a sort that leaves a
copy out of order (and `parallel` also to any result that differs from
the sequential one), while `joins` does the same with a result that
differs from that of `std::map`. Similarly, `graphs` runs the same
algorithm on three families of structures, and it stops when they differ
in the counts of the nodes extracted, of the arcs examined or of the
decreases of a priority.

In general, the clock excludes what is not the operation measured: the building
of the input and its copy (a sort runs on a fresh copy of the same input at
each run, made in `prepare()`), the conversion to the types of the other
libraries (in `geometry`), the building of the graphs (in `graphs`, `flows` and
`dynamic`) and the first solution of the dynamic algorithms. Each exception is
deliberate, and stated in the comment of its program. Thus, `search` includes
the preparation of the pattern, which a searcher pays at each pattern, and
`backtrack` includes the building of the exact cover from its options (and, for
the programs of Knuth, run as processes, the reading of the file and the start
of the process). Also, `graphs` includes the building of the structures of the
run, e.g., of the priority queue.

Note that the programs of several threads (`parallel`, `runtime` and
`mpi`) make a
first run that is not timed, which starts the threads of the runtime. In
`matrix`, an untimed product of the same library precedes each product of
up to 512 rows, and 20 ms of scalar work precede each cell; thus each
product starts with its operands in the cache and the processor at the
same clock. A cell is printed as one line of five fields separated by
tabs, `( benchmark , case , n , structure , ns/op )`, with four decimals
below one nanosecond and two above. Each program takes the options `-r`
(the runs, 5 by default), `-s` (the seed of the random input) and the
sizes after them.

## B.3 The noise {#sec-B-3}

Two runs of the same code on the same machine do not give the same time,
and a ratio of two structures means something only when it is compared
with that difference. We measure it in two ways. Within a run,
each program but `hashattack` and `ssets` runs its first structure twice,
the second time under its label followed by `A/A` (e.g., `std::vector
A/A` in the stack cells of `queues`). These two rows measure the same
code in the same process, and `bench/tables.py` takes as the noise of a
table the largest relative difference between them over its cells.

Across runs, `bench/summary.py` takes the files of two whole campaigns on
the same machine and computes the relative difference of each cell
between them, of which it reports the median, the 90th percentile and
the largest value; each cell of its summary is then the mean of the two
runs. Hence we report as a tie a ratio of two structures that lies
within the A/A noise of its table. Also, the text gives the comparisons
as ranges (e.g., "1.2 to 1.9 times faster"), and never as a single ratio
with more digits than the noise allows.

## B.4 The machine {#sec-B-4}

A campaign runs on a machine that nobody else uses meanwhile, and we
check its load before starting, by the load average and by the busy time
of the processors in `/proc/stat` (the count of the processes is not a
reliable measure). In the header of its file, `bench/run.sh` records the
model of the processor, the number of cores, the compiler, the load
average at the start and at the end, the date, the core, the runs per
cell, the mode of the processes and the text of the compressors; thus
each file carries the conditions of its measurements. A campaign whose load rose during the run is
repeated, not reported (its A/A noise would usually show it, cf.
[§B.3](B-benchmarks.md#sec-B-3)).

Each program runs pinned by `taskset` on one core (`CORE`, 0 by default). Those
of two threads (`concurrent` and `allocators`) run on `CORE` and on `CORE2`, by
default the core after `CORE`, and `concurrent` pins its two threads to the
first two processors that the process may use. As for the parallel programs
(`parallel`, `joins`, `greedy` and `runtime`), they run on the cores of
`PCORES` (by default only `CORE`), at 1, 2, 4, and so on threads up to
their number (and at most 128); the lines that the GAP benchmark suite
prints directly inside `parallel` are left out. With `ISOLATE=1`, the
default, each structure of each benchmark runs in a separate process
(`ISOLATE=0` runs each benchmark in one process). To
this aim, `run.sh` first obtains from the binary the labels of its structures
(`ADS_BENCH_LIST=1`), and then runs it once per label
(`ADS_BENCH_WHO=<label>`); hence no cell depends on the structures measured
before it in the same process, through the state of the heap, the caches or the
layout of the code.

We compile the benchmarks by the makefile with `-std=c++20 -O3 -march=native
-DNDEBUG -DADS_CONTRACTS=0`. On x86 it adds
`-Wa,-mbranches-within-32B-boundaries` when the assembler accepts it,
which keeps the branches off the boundaries of 32 bytes, where the
erratum JCC of Skylake slows a loop by its alignment only. These flags
are the same for ADS and for the libraries it is compared with, and
therefore a ratio does not depend on where the code falls. Finally,
`queues` keeps the memory freed in the heap of `malloc` (by `mallopt`);
otherwise a structure might pay the faults of pages that another one did
not, depending on the order of the cells.

## B.5 Reproducing a campaign {#sec-B-5}

A campaign is made of two runs of `bench/run.sh` on the same idle
machine, after the programs have been built with the competitors that
are installed:

    make bench GMP=<prefix> HASHING=<dir> ABSL=<prefix> ...
    CORE=2 RUNS=11 bench/run.sh
    CORE=2 RUNS=11 bench/run.sh
    python3 bench/tables.py results/<host>-<date>.tsv
    python3 bench/summary.py results/<run 1>.tsv results/<run 2>.tsv

The variables of `make bench` name the prefix or the sources of each
competitor. They are listed at the head of `bench/run.sh`, and the comment
before `BENCHES` in the makefile gives the benchmark to which each one adds a
competitor; a competitor whose variable is not given is left out, and its
benchmark still builds. Then `run.sh` runs the programs at the sizes of its
lines (e.g., $10^5$ and $10^6$ for the dictionaries, and from 16 to 32768 words
for the long integers), with `RUNS` runs per cell (11 by default), and writes
`results/<host>-<date>.tsv`, whose name it prints at the end. It takes
the programs from the folder `BUILD` (`build-make` by default), runs
`bignum-own` and `matrix-own` only if they were built, and renames the
benchmark of each type of the sorts as `sort <type>` (e.g., `sort
uint32`), its pattern being the case.

Two inputs come from outside the repository when they are given. For instance,
the text of the compressors is the file of `ADS_BENCH_TEXT`, repeated up to
each size (otherwise, words drawn from a Zipf law). Also, the road networks of
`graphs` are those of the 9th DIMACS Implementation Challenge when the folder
`ADS_DIMACS` holds them (otherwise, points of the unit square joined to their
nearest neighbours). Then `bench/tables.py` turns a file into one Markdown
table per benchmark, with a row per structure and a column per case and size,
each cell the time followed, in parentheses, by its ratio to the first
structure (the one run again as A/A), both to 3 and 2 significant digits.
It leaves out the lines of the benchmarks named `<name>:count`, with which
`graphs` records the counts that identify a run, and it notes the tables
that have no A/A row. Finally, `bench/summary.py` turns the two
files into the summary of [Table B.3](B-benchmarks.md#tab-B-3).

Three programs are not in `run.sh`, and we run them by hand in the same
conditions: `ssets` and `hashattack` ([§B.6](B-benchmarks.md#sec-B-6)),
and `mpi`, as `mpirun -np <P> build-make/bench-mpi [-t <threads>] [-m]`
on 1, 2, 4, and so on processes. Their lines have the same form, and they
are appended to the file of the campaign, with `taskset` on the same
core for the first two.

## B.6 The comparison with Morin's code {#sec-B-6}

For the structures of [Morin 2013](Z-bibliography.md#ref-morin-cpp), ADS
has the code of the book to compare with. Several headers claim to be
faster than it, each in a paragraph "Where it is meant to be faster than
the ... of Morin, for the benchmarks to check". In particular, `ssets` is
the experiment of exercise 9.14 of Morin's book. It runs the sorted sets
of ADS (the skip list, the scapegoat tree in three variants, the treap
with its hashed and finger forms, the randomized search tree, the
left-leaning Red-Black tree, the trees of partial rebuilding, the
Red-Black tree and the AVL tree) against `std::set`. Built with
`ODS=<dir>`, the C++ sources of the book (`ODS_DIR` with CMake), it also
runs `SkiplistSSet`, `ScapegoatTree1`, `Treap1` and `RedBlackTree1` of
Morin. Its keys are $n$ distinct random integers of 31 bits, which the
sets of Morin hold by themselves, and those of ADS with a value of 4 bytes
each.

Its cases are those of the exercise: $n$ insertions in random and in
increasing order, $n$ searches of present keys, $n$ searches of the least
key not smaller than an absent one, and the removal of all the keys in
random, increasing and decreasing order. We remark that the code of the
book has two defects, which the comment of the program records, and which
the campaign must patch before it runs. First, the constructor of its
`ScapegoatTree` leaves the bound $q$ uninitialized; hence a tree may
rebuild at random or climb past its root. Second, its `SkiplistSSet`
frees with `delete` the nodes that it allocates with `malloc`. Both
defects crashed runs of 2000 keys. Hence the campaign tests the claims of
[Table B.2](B-benchmarks.md#tab-B-2), each against the class of Morin
that the header names.

<span id="tab-B-2"></span>**Table B.2.** The claims of the headers that
`ssets` tests against the code of Morin, the class of ADS and that of
Morin in the same row, and the reasons that each header gives; the
measured ratios of each case of `ssets` are filled in from the files of
the campaign.

| ADS | Morin | Reasons stated by the header |
|-----------|----------|------------------------------|
| `SkipList` | `SkiplistSSet` | levels from `SplitMix64` instead of `rand()`; $\ell$ links per node instead of $\ell + 1$; no comparison repeated across the levels |
| `ScapegoatTree` | `ScapegoatTree1` | two links per node, the climb on the path kept in a vector; the depth tested against a table of the powers instead of a logarithm; each size counted once (`REUSE`) |
| `Treap` | `Treap1` | priorities from `SplitMix64`; the insertion rotates on the way back up, with no link to the parent; the sizes cost one addition per level |
| `LeftLeaningRedBlackTree` | `RedBlackTree1` | the colour in a byte; nil a null pointer instead of a sentinel node read at each test of a colour |

Other headers make claims of the same kind, which `ssets` does not test
since it runs no code of Morin for them: `SkipListSequence` against his
`SkiplistList`, `BinaryTrie`, `XFastTrie` and `YFastTrie` against his
tries, and `MeldableHeap` and `DaryHeap` against his heaps. Until a
program measures them against the code of the book, these claims remain
reasons why each structure should be faster, rather than results.

Similarly, `hashattack` is the experiment of exercise 5.11 of Morin's book: $n$
integer keys chosen so that a hash table puts most of them in one place,
against $n$ random keys. It searches them in `std::unordered_map`, whose hash
of an integer is the integer itself taken modulo a prime number $P$ of buckets;
hence the multiples of $P$ collide after `reserve( n )`. Then it searches them
in `ChainedHashTable` and `OpenHashTable` with `std::hash` on $2^d$ places,
where the multiples of $2^{20}$ collide while $2^d \le 2^{20}$. Finally, it
searches them in the same two tables with the multiplicative and the tabulation
hashing of [Morin 2013, §5.1.1 and §5.2.3](Z-bibliography.md#ref-morin-cpp).
This attack makes each search in the first two kinds of tables cost $\Theta( n
)$, and therefore the cells of the case `find-attack` should grow linearly with
$n$ (at the sizes 1000, 4000 and 16000 by default). In the randomized tables,
instead, a search should stay at $O( 1 )$ expected, as in the case
`find-random`. We expect the campaign to confirm the shape of this growth,
which the chapter of the dictionaries reports.

## B.7 Where the numbers go {#sec-B-7}

The numbers of the campaign enter the manual in two forms. One is the
summary that `bench/summary.py` writes from the two files of a
campaign, with the structure of [Table B.3](B-benchmarks.md#tab-B-3) and
one row per pair of a structure of ADS and a library in the list `PAIRS`
of the script, as an HTML table. To `PAIRS` the script adds, for each
algorithm of `parallel` and each number of threads, the pool of ADS
against the parallel libraries, against the same algorithm of ADS on the
executors of FastFlow, OpenMP and oneTBB, and against the sequential
algorithm. A cell enters the summary only if both runs measured it with a
positive time; hence a result marked $-1$ as wrong
([§B.2](B-benchmarks.md#sec-B-2)) is left out of the summary. Its rows go in this appendix, and those of the structures
of a chapter also go in the section of that chapter that Table B.1
names. The other is the table of a single benchmark, which
`bench/tables.py` writes from one file, with one row per structure and
one column per case and size. In it each cell is the time in nanoseconds
per operation (or per item, per byte or per arc, as the program states),
followed by its ratio to the first structure, and the caption gives the
A/A noise of the table and the file of results it comes from.

In both, the measured values are filled in from the files of the campaign,
`results/<host>-<date>.tsv`, as the README does for its table, whose
values come from `results/frangioni-20261019-1216.tsv` and
`results/frangioni-20261019-1315.tsv`, an earlier campaign on the
benchmarks of the sorts, of the dictionaries, of the priority queues, of
the sequences and of the long integers, which `results/README.md`
tabulates. No number of this manual is taken from elsewhere.

<span id="tab-B-3"></span>**Table B.3.** The structure of the summary of
a campaign, as `bench/summary.py` writes it: for each benchmark and each
structure of ADS, the library it is compared with, the number of cells
(cases times sizes) where both were measured, and the median and the
range over those cells of the time of the library divided by that of ADS
(above 1 when ADS is faster), each cell the mean of the two runs; when a
library has more than one candidate, a cell takes the fastest of them.
The rows shown are examples of the list `PAIRS`, and the values are those
of the campaign files.

| Benchmark | ADS | Against | Cells | Median | Range |
|---|---|---|---|---|---|
| `sort uint32` | `ADS::sort` | std | | | |
| | | Boost | | | |
| `dictionaries` | `ADS::FlatHashTable` | std | | | |
| | | Boost | | | |
| `priority` | `ADS::DaryHeap d=4` | std | | | |

In its header, the summary gives the machine, the core, the compiler, the
runs per cell, and the load at the start of the first run and at the end
of the second. It also gives the A/A noise across the two runs (its
median, its 90th percentile and its largest value over all the cells);
thus one reads each ratio against the noise of the campaign that
produced it.

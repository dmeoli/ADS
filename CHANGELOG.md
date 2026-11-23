# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `TinyRankSelect` (`BasicTinyRankSelect< L0 >`, with blocks of `L0` =
  512, 1024 or 2048 bits), the rank and select of 3* of Kurpicz,
  Rigi-Luperti and Sanders: an entry of 512 bits per superblock of 32
  blocks, i.e., 0.78125% of the bits for the rank with the default blocks of
  2048 bits, and a select in constant time in the worst case through a tree
  of samples of three levels packed at the widths that its values need, a
  few hundredths of a percent of the bits for each value where the bits are
  spread evenly; its parallel construction on an executor, its persistence,
  and its use as the directory of `HuffmanWaveletTree` (whose parallel
  construction now takes any directory built on an executor); the kernel
  `detail::lineRank()` on several lines and `detail::lineCount()`; the
  tests `engineered_succinct.tiny-rank-select` and `tiny-rank-select-large`
  (on 2^32 + 70 bits), the tiny directories in the tests of the parallel
  construction, of the persistence and of `succinct_top`, the cells
  `ADS::TinyRankSelect` and `ADS::TinyRankSelect<1024>` of the benchmark
  `bitvector` against pasta, sdsl-lite (with the new cell
  `sdsl::rank_support_v5+mcl`) and sux, and the part of section 22.4 of the
  manual on it (Algorithm 22.8, Lemmas 22.7 and 22.8)
- the row `h=2` of the benchmark of the directly addressable codes, at most
  2 levels, which on its integers takes 10.3 to 10.4 bits each, against 9.4
  of the default of 3 levels and 10.1 of the levels of 8 bits
- the events of `Modification.h` from the mutators of `RadixHeap`,
  `DialQueue` and `BucketQueue` (an item inserted or erased with its keys,
  the queue emptied), of `DisjointSets` (the link of a root under another,
  as a change of the new attribute `mod::Parent`), of `LinkCutTree` (the
  nodes, their values and the edges of the forest without its roots), of
  `BTree`, `DenseHashMap` and `PointerTable` (by key, as `JoinTree`), and,
  in overloads that take an observer, the improvements of the incumbent of
  `bbTsp()`, `maxWeightClique()` and `knapsackBB()`, each structure taking
  its subject policy as the last parameter of its type, `NoObservers` by
  default, with which its code is that of the structure before the events;
  `BTree::clear()`, which the tree lacked; the tests
  `engineered_emitters.emit-queues*`, `engineered_emitters_forest` and
  `engineered_emitters_dict`, the functions of these structures in the check
  of zero cost of `bench/zerocost_asm.cpp`, and section 1.15.5 of the manual
  (Table 1.8, Lemmas 1.24 to 1.26)
- the capabilities of the incremental searches on structures of the
  library: `extract( theta , out )` of `IndexedDaryHeap`, `RadixHeap`,
  `DialQueue` and `BucketQueue` (`batch_extract`), `rebase( k )` of
  `RadixHeap` and `DialQueue` (`monotone_extract`), with `peekKey()` of
  `DialQueue`, `rekey( f )` of `IndexedDaryHeap` (`rekeyable`), which sends
  `Rebuilt`, `TableHeap`, the queue of the LaB-PQ that reads its keys from a
  table and observes an `ObservableArray` (`observes_values`), and the
  opt-in `nonmonotone_values` of `ConcurrentCuckooTable`, whose lookups read
  each value whole under the version of its region; the tests
  `engineered_capabilities.cap-extract`, `cap-rebase`, `cap-rekey`,
  `cap-tableheap` and `cap-nonmonotone`
- `PointerTable` takes its places through its policy, `WithPlaces< L , P
  >` for the places `L` and the subject policy `P`: `WidePlaces`, the
  default, the places of 16 bytes of before, or `IndexPlaces< S >`, places
  of 8 bytes that keep the high 32 bits of the hash and the index of the
  object in a store `S` given to the constructor (`S::at()` and
  `S::index()`), half the memory for at most 2^32 - 1 objects; and on an
  allocator that declares `is_zeroed`, as the new `ZeroPageAllocator` of
  `HugePageAllocator.h` (blocks of `std::calloc()`, the large ones aligned
  to a huge page), the table and its growths write no 0 over the places;
  either option takes the places of ADS also with `ADS_WITH_BOOST`, whose
  places stay those of the default
- `bwt()` and `detail::SentinelSuffixArray` take the length `wideFrom`
  from which their suffix array is of entries of 64 bits, by default
  `bwtWideFrom` = 2^31 - 1, so that the path of 64 bits runs, and is
  tested against that of 32 bits, on short texts
- `ADS_SIMD_SCALAR`, 0 by default, which set to 1 with `ADS_WITH_HIGHWAY`
  turns off every vector branch of the headers (the kernels of `Simd.h`
  and of `SimdInline.h`, the matching of the hash tables, the filter of
  the searches of strings, the sort of Highway, the packing of the
  integers), so that one build on Highway tests the scalar code too; the
  branches are taken on `ADS_SIMD`, which `Simd.h` defines, and
  `ADS_FLAT_SIMD`, `ADS_SEARCH_SIMD` and `ADS_VECTOR_SORT` take it as their
  default, while `ADS_BP_SCALAR` and the three, when given, keep their
  meaning
- `ADS_TEST_CONTRACTS`, the level of the contracts at which CMake and the
  makefile build the tests, 3 by default; below 3 the tests pass with the
  checks of the contracts themselves (`CHECK_THROWS` of a
  `contract_violation`, the audits and the counts kept for them) left out
- the pseudocode of the operations of `SequenceStack`, of the updates of
  `HashedTreap`, `ParentTree` and `MaterializedAggregate` and of the
  dispatch of `queue_deleteMin`, and, in chapters 1 to 13 and appendix A
  of the manual, the lemmas of the headers that had none (among them
  `CursorTree`, the dictionaries of chapter 7, `LinkedHashSet`,
  `BlockedBloomFilter`, `CuckooFilter`, `IndexedHeapPair`,
  `SequenceRange`, the prefetch, `ArenaResource` and `PmrResource`) and
  the pseudocodes of the headers that no chapter included

### Changed

- `FlatHashTable` keeps the code of ADS also where `ADS_WITH_BOOST` is
  defined, since on the benchmarks it is on a par with
  `boost::unordered_flat_map` (a median ratio of the times of 0.93 on the
  dictionaries and of 1.03 on the hash maps); `ADS_FLATHASH_BOOST` defined
  to 1 puts it on Boost as before, and the name of the inline namespace of
  the layouts [see `Abi.h`] follows it

- a batch cancels an erasure against the last insertion of the same handle
  with the same key and value among the insertions at the end of its net
  events, so that the events of an item that is several times in a queue
  with no handles coalesce into those of a multiset of pairs, and
  `priority_t< Q >` is `Q::Key` when the queue has no `Priority`
- `DenseHashMap` counts exactly the places that a removal empties in a
  group with some overflow, the only ones that still count as full, adding
  one at a removal only in such a group (whose control bytes the removal
  has read), so that `deleted()` takes O(1) time and an insertion that
  reaches 7 m / 8 of the places rebuilds the index with no scan of the
  groups to count them again
- the tables of the benchmarks in the README and in `results/` are those
  of a new run on the AMD EPYC 9654, built with Highway and, the parallel
  benchmarks, with hwloc, and each comparison of the README carries its
  verdict against the noise of the rows A/A
- the spelling of the comments, the documentation and the manual is the
  American one, and `kOptNeighbours` is now `kOptNeighbors`, the former
  name staying as a deprecated alias (the titles of the cited works keep
  the spelling with which they were published)
- the fingerprint of `BinaryFuseFilter` and of `RibbonFilter` is the high
  bits of the product of the hash by an odd constant, which depend on
  every bit of the hash and not only on the bits that give the places (the
  coefficients of the Ribbon filter fix the hash but for one bit, and the
  places of the binary fuse filter shared bits with the fingerprint from
  segments of 2^15 places), so that the false positive rate of 2^-b holds
  in the model of Lemma 10.17; the files of both filters are of version 2,
  whose bit 63 of the count of the trials marks the new fingerprints, and
  the files of the version 1 (and the archives of cereal written before)
  are still read, their filters keeping the fingerprints of
  h ^ ( h >> 32 ), and hence their answers, also when written again
- a structure of `Persistence.h` may give `persistOldestVersion`, the
  oldest version of its files that `restore()` accepts, and `persist()`
  reads the version of the file being read by `a.version()`
- Lemma 25.5 states what `run()` of `ThreadPool` gives: no thread holds two
  calls of a run at once and every call begins while the others wait, so
  that calls that wait for each other run at the same time on distinct
  threads, while a call that returns before the others have begun may
  leave its worker to claim another one; `Executor.h` and the theory of
  the workers say the same
- Lemma 12.30 states the sample of the learned sort as an assumption of
  the model (its places come from a xorshift seeded by the size), bounds
  the buckets with probability at least 1 - delta for
  eps = sqrt( ln( 2 / delta ) / ( 2 s ) ), a constant probability for
  eps = c / sqrt( s ) and not a high one, and treats the ties: a bucket
  between two splitters holds at most ( 1 / k + 2 eps ) n keys, and one
  that also takes the keys equal to its splitter, when the sample repeats
  no splitter, at most ( 2 / k + 2 eps ) n
- `PartitionedEliasFano` keeps a record of one word per block (two if its
  fields do not fit in one), with the start of the block and the place of
  its code relative to its superblock of 16 blocks and the place of its 1
  of rank 64, so that `access()` reads the record in one load and selects
  in at most 4 words with no branch on the word, and `nextGEQ()` finds the
  block by a table of buckets of the superblocks and a count of 16 records
  in place of the layout of Eytzinger and a scan of the records of fixed
  width; its files are of version 2
- with `ADS_WITH_HIGHWAY`, on the targets whose compress of 64 bytes is
  one instruction (AVX-512 VBMI2), `selectInWord()` reads the place of the
  k-th 1 of a word from the indices of the lanes compressed by the
  mask of the word, by a permutation of the bytes, which shortens the
  select of `RankSelect` and the access of `EliasFano`; elsewhere, where
  the compress of bytes is emulated and slower, it keeps the broadword
  select; `select()` of `FastBitwise.h` is `selectInWord()`
- the tables of the benchmarks in the README and in `results/` are those
  of a run on an AMD EPYC 9654, with the rows "A/A" of every benchmark for
  the noise, and `results/` lists the libraries compared with the commit
  or the checksum of each
- `bench/summary.py` takes one run, with the noise from its rows "A/A", or
  two, and with `--short` gives a row per benchmark against the standard
  library and against the strongest library, each comparison closed by its
  verdict against the 90th percentile of the A/A: + (ADS faster in every
  cell), − (slower in every cell), ≈ (the median within the noise) or ±
  (faster in some cells and slower in others)
- `FastFlowLock::lock()` retries its test-and-set after the exponential
  backoff of `Backoff`, from pauses of the processor to yields of the
  thread, instead of the busy loop of `ff::spin_lock()`, so that the threads
  waiting for a holder preempted by the system do not spin through their
  quanta
- `foldedProduct()`, `hashWord()` and the cases of `hashBytes()` up to 64
  bytes are always inlined, the strings of more than 64 bytes hashed out
  of line as before, since GCC kept them as calls in the loops of the
  tables and of the filters
- `detail::wideProductHalves()` is always inlined and returns the two
  words of the product in a `detail::WordPair` instead of writing the high
  one through a reference, which GCC kept on the stack of a call
- `BinaryFuseFilter` computes the first place of a key once for its 3
  places, in `maybe()` and in the build
- `PrefixFilter` tells a remainder that matches a single byte of its bin by
  the bit of the header at its place and the ones below it, with no
  select, and finds the end of a run by the zeros that follow its start
  instead of a second select; the bins and the answers are the same
- `DenseHashMap` counts its empty places before a growth only after a
  removal, since with none the count cannot avoid the growth
- the scalar kernel `detail::lineRank()` counts the words before the one
  of the place under masks of all 1s or all 0s, and that one under the mask
  of its low bits, with no branch, where the clamp of the length of each
  word compiled into a chain of branches, one of them mispredicted at
  almost every rank
- `DirectAccessCodes` bounds its dynamic program to the widths of at most
  3 levels by default (`Levels`), a few bits more than the optimum, which
  takes up to 8 levels of chunks of 1 or 2 bits, a rank and a branch each;
  `Levels::UNCAPPED` gives the unbounded program
- `InterleavedRankSelect` builds its lines a superblock at a time, first
  the bits of each line, shifted a vector at a time with
  `ADS_WITH_HIGHWAY`, then the counts and the samples, the line read from
  the bit 16 before it so that no word is shifted twice
- `factorize()` splits a composite cofactor of 34 bits or more by the
  elliptic curves of Lenstra, on curves of Montgomery with the parameter of
  Suyama, a stage 1 by the ladder of Montgomery and a stage 2 on the primes
  paired by the giant steps of 60, with bounds that grow with the size of
  the cofactor, and a square by its root, the rho of Pollard and Brent
  remaining below 34 bits and after 64 curves with no factor; the
  divisions go up to the primes below 4096, each a product by the inverse
  of the prime and a comparison
- the pages of `PoolResource` and `ConcurrentPoolResource` count their
  slots given and not freed, and a page whose last slot is freed is cut
  again from its first slot by the bump of its pointer, so that a
  container emptied and filled again takes its nodes in the order of
  their addresses instead of the order of the frees
- `radixSort()` and `radixSortBy()` take the buffer of trivial items from
  the block that `ScratchRoom` keeps between the calls, so that a sort
  does not pay the faults of fresh pages
- `bwt()`, `HuffmanFMIndex`, `RIndex` and `MoveRIndex` build the suffix
  array of the text by the `suffixArray()` of `SuffixSorting.h`, on
  entries of 32 bits below 2^31 - 1 bytes, with the row of the
  sentinel by the arithmetic of the indices instead of an insertion at the
  front; `FMIndex` keeps the suffix array of `SuffixArray.h`
- `intersect()` searches the items of the shorter list in the longer one
  when the longer has more than 4 times the items of the shorter, instead
  of 32, where the search beats the merge with Highway and without
- `binaryPack()` packs the full blocks of unsorted integers from the input,
  with no copy of it, into an output of its exact size, and the
  differences of sorted integers once a block, in a single pass into room
  that is not cleared; `streamVByteEncode()` writes into room that is not
  cleared, and its kernel of Highway makes the control bytes of 8 integers
  at once by a saturated narrowing and the mask of the top bits instead of
  a sum of the lanes per 4
- `BoolSet` combines the words that both sets have in a loop with no test,
  which the compiler vectorizes without Highway, and masks the bits past
  the dimension once at the end
- the idle workers of `ThreadPool` look for work for a budget that follows
  the gaps between the tasks that each of them sees, twice the longest gap
  of the last two windows of 5 `maxSpin`, between `spin` (2 ms) and the new
  parameter `maxSpin` (200 ms), and back to `spin` after a gap longer than
  `maxSpin`, pausing ever longer between the looks and yielding the core
  beyond `spin`; before, they slept after 2 ms, so that a parallel phase
  after a sequential step of more than 2 ms waited for their wake-up and
  ran on cores that the governor of the frequency had slowed down
- the loops and the reductions of `Executor.h` cut their indices into at
  most 64 blocks (`kBlocks`, 256 before), or into at most the blocks of the
  new last parameter of `blocksOf()`, `parallelForBlocks()`,
  `parallelFor()` and `parallelReduce()`, and the partial results of
  `parallelReduce()` take a line of the cache each: a sum of 10^6 integers
  of 64 bits claims 64 blocks of 125 KB instead of 256 of 31 KB, and its
  threads no longer write to the same lines
- `MPSCInbox` takes its cells from blocks of 64 of each producing thread,
  which the consumers give back once they took all their messages, instead
  of allocating a cell per message for the consumer to free, and a push
  wakes the consumer only when it finds the inbox empty; `takeInto()`
  appends the messages taken to a vector that keeps its room
- `ConcurrentCuckooTable` keeps 4 bits of overflow per bucket, one of which
  is set when a key goes into its other bucket, and a lookup reads the
  other bucket only when the bit of its key is set; a lookup of an absent
  key, whose tag is not among those of its first bucket and whose bit is
  clear, returns without reading the version of the region (the remap
  entries of the Horton tables of Breslow et al., as bits)
- `ConcurrentCuckooTable::find()` and `contains()` inline the lookup
  without locks at the call and call the lookup under the lock, so that a
  lookup of keys and values read atomically costs no call, where the
  compiler had left both out of line in a program with many tables
- `ConcurrentCuckooTable` doubles with the threads that wait for a lock:
  the old table is cut in parts of whole regions, which go to buckets of
  the new table that receive no other pair, and the waiting threads claim
  parts by a compare-and-swap of a counter carrying the number of the
  doubling and migrate them with the thread that doubles, instead of
  spinning while it places every pair alone (the cooperative migration of
  Maier, Sanders and Dementiev)
- `cmake/ads_sets.py --test` takes a compiler given with its options
  (e.g., `--cxx "g++ -m32"`), as the makefile passes `$(CXX)`
- `VectorQuickSort.h` includes `Simd.h` before the sort of Highway, so that
  the dispatch of the kernels comes before `hwy/highway.h` in every unit

### Fixed

- `ArrayGraph` has `weighted`, `arcs()`, `arcHeads()` and `arcWeights()`,
  with the meaning of those of `StaticGraph`, so that `CsrInput` of
  `Csr.h` and `view()` of `GraphBLAS.h` take it, as their documentation
  says, where they did not compile on it; `view()` of a graph whose rows
  are not known to be sorted copies it by `copy()`, which now takes any
  graph of compressed rows, if the heads of a row do not increase, since
  GraphBLAS reads them in increasing order
- `DynamicCsr::clear()` sets the first start of its rows to 0 by an
  assignment, where a resize to 1 left it unset on a vector emptied by a
  move (the allocator of the vector does not initialize the numbers) and
  made g++ 13 warn of a write out of the block, through the construction
  of `DefaultInitAllocator`, on the path that grows an empty vector
- on a target of 32 bits the hash tables, the filters and the perfect hash
  functions take the 64 bits of the hash of `FastHash`, which its new
  member `hash64()` gives whatever the width of a `std::size_t`
  (`detail::hash64()`, with `detail::kAvalanching64` for a hash that
  declares `is_avalanching` and gives 64 bits), where they took its
  `std::size_t` of 32 bits as a hash of 64 with the high half 0, which put
  the keys of `BlockedBloomFilter`, `CuckooFilter`, `BucketCuckooTable`,
  `ConcurrentCuckooTable`, `PrefixFilter` and the others into a few
  buckets and left `RecSplit` and `FingerprintPerfectHash` with no seed
  that separates the keys; `BucketQueue` takes its keys in 64 bits, where
  a `std::size_t` of 32 bits cut a key past 2^32; the tests that assumed a
  `long` or a `std::size_t` of 64 bits take their widths, and the m32 job
  of the CI, all of whose tests pass, is now required
- `BucketCuckooTable` puts in a stash, searched after the two buckets, a
  key that finds no place after 2 doublings of its insertion or in a table
  at most half full, and a doubling puts there the pairs that it cannot
  place: more than 8 keys of equal hashes made the table double until the
  memory ran out, and now it has at most max( P0 , 4 n ) places (Lemma
  7.17), a search costing one more test while the stash is empty
- `CuckooHashTable` draws at most 8 pairs of functions per rebuild, and
  puts the keys still homeless in new places at the end of the stash:
  more than s + 2 keys of equal hashes made it rebuild forever
- `ConcurrentCuckooTable` puts in a stash, under a mutex of its own and
  with the bit of overflow of the first bucket set, a key that the table
  less than 1/16 full cannot place, instead of throwing
  `std::length_error`, and a doubling puts there the pairs that it cannot
  place instead of doubling again; `stashed()` counts them
- `BucketingFilter` sizes its buckets on the universe of the words of its
  keys, 2^w for keys of w bits, where it took 2^64 for every type, so that
  on keys of 32 bits (or fewer) a single bucket held them all and every
  query answered true
- `PackedBitWriter::put()` of a string checks that all of it is of '0' and
  '1' before it writes a bit, where it wrote the bits before the first
  other symbol and then failed its precondition
- `gammaLength()`, `deltaLength()`, `riceLength()`, `golombLength()` and
  `denseLength()` check the preconditions of their codes (x >= 1,
  0 <= k < 64, 1 <= b <= 2^63, s >= 1 and c >= 1), where they returned a
  wrong length, divided by zero or shifted a word by 64 bits
- `FlowNetwork::maxFlow()` with `PUSH_RELABEL` leaves the call to `DINIC`
  when the residual capacities out of the source and the excesses left by
  `setCapacity()` sum to more than `LLONG_MAX`, since the preflow pushes
  them all at once and its excesses overflowed a `long long` on networks
  whose maximum flow fits one (e.g., two edges of `LLONG_MAX` out of the
  source into a node)
- `maxDensitySegment()` multiplies the sums of the segments by their
  lengths, in the turns of its hull and in the comparison of the averages,
  on the integers of 64 bits for an integer `T` of at most 32 and on those
  of 128 bits for a wider one, since in `T` the products overflowed on
  sums of segments well within `T` (e.g., items of 10^9 in an `int`),
  which left the result undefined
- `detail::ScratchBuffer` of a type aligned beyond what `operator new`
  gives takes a block of a huge page at least, which is aligned to a huge
  page; before, a buffer of such a type smaller than a huge page was on a
  block aligned to 16 bytes only
- the parallel `multiwayMerge()` checks that its ranges are sorted (an
  audit) before it splits them by binary searches, as the sequential one
  does; before, only the merges of the pieces checked their pieces, after
  the splits had searched the unsorted ranges
- `ConcurrentUnionFind` checks n < 2^31 before it allocates the
  parents, and `parent()` and `compress()` check their element as the
  other operations do
- `ArenaResource::allocate()` throws `std::bad_alloc` when a block of the
  bytes asked plus the alignment, with its header, would not fit in a
  `std::size_t`; before, the size of the block wrapped around, and a
  request of nearly 2^64 bytes was given a pointer into a block of a few
  bytes
- the `swap()` of `FlatHashTable`, `DenseHashMap`, `RootishArrayStack` and
  `BTreeMap` is `noexcept` only when its allocators propagate on a swap or
  are all equal, as the `swap()` of the containers of the standard, so
  that a swap of two containers on unequal allocators that do not
  propagate throws `contract_violation` instead of calling
  `std::terminate()`; `FlatHashTable` checks its allocators before it
  exchanges anything, so that the refused swap leaves both tables as they
  were
- `RecordFile::append()` drops the bytes of a record left partial at the
  end of the file (by a write interrupted, say) before it appends, so that
  the records appended start where the whole ones end; before, they were
  written after the partial bytes, and the scans read every record after
  them shifted by those bytes

- `RankSelect` and `CompactRankSelect` compile where `std::size_t` has 32
  bits, their top levels taken by shifts of 64 bits, and the preconditions
  that bound a size by 2^32 (the perfect hashes, `RecSplit`, the PGM
  indexes, `SellMatrix`, `LAVector`, `RemUnionFind`, `WordRangeMinimum`) hold
  there for every size, which a shift of a `std::size_t` by 32 broke
- the makefile builds the benchmarks on Boost, Highway and hwloc where they
  are found, as the targets of CMake do, keeping the code of ADS where a
  structure would delegate to Boost or to LEMON; before, it built them on
  none, and the structures with a vector kernel or a wide product ran
  their portable code
- `feedbackFarm()` on FastFlow orders the memory between the master and the
  workers with fences, since the queues of FastFlow order the stores of the
  producer only: on arm64 a master that read a task back through a pointer
  of its own, not through the one taken from the queue, could read its old
  contents
- the makefile takes the competitors of AVX2 of the benchmarks again
  (fastfilter_cpp, x86-simd-sort): the probe that compiles their headers
  wrote `\#include`, which make 4.3 and later pass on with the backslash,
  and did not include the standard headers that `simd-block.h` uses
- `bench/parallel.cpp` builds again with ParlayLib, whose `parlay::map()`
  does not take the arcs of `g.out()` as a random access range
- `bench/run.sh` writes only the cells of the benchmarks in the results,
  and `bench/tables.py` leaves out the lines that are not cells, which
  BBHash prints by itself when asked for its size
- `MeldableHeap::erase()` checks that a handle with no parent is the root
  of its own heap before it merges the children, so that the root of
  another heap is refused with `contract_violation` and both heaps stay
  whole; before, the check came after the merge, which had already
  relinked the nodes of the other heap, and the destructors freed one of
  them twice

## [0.1.0] - 2026-11-18

### Added

#### Organization of the library

- a header-only C++20 library of 445 public headers in `include/ADS`, one
  directory per area, each in one of the sets of `cmake/ads_sets.txt`:
  `base` (245 headers, the structures and the algorithms that follow a
  textbook or an academic source), `advanced` (153, those written for speed
  beyond these sources, the concurrent and parallel structures, the
  executors, the distributed layer, the typed modifications and the
  observers), `core` (19, the infrastructure of both) and `interop` (28,
  the adapters to other libraries); the entry points `ADS/core.h`,
  `ADS/base.h`, `ADS/advanced.h` and `ADS/ADS.h`

#### Structures and algorithms of the textbooks

- the abstract data types of A. Montresor, Algoritmi e Strutture Dati
  (2025), each a concept of `ADT.h`, realized after the pseudocode and the
  names of the book (`sequence/`, `stack/`, `queue/`, `tree/`,
  `dictionary/`, `set/`, `priority/`, `mfset/`, `graph/`), with the
  algorithms of its chapters (`algorithm/`, `search/`, `sort/`,
  `optimization/`, `backtrack/`, `complexity/`, `intractable/`) and its
  exercises as tests
- the structures of P. Morin, Open Data Structures, e.g.,
  `RootishArrayStack`, `DualArrayDeque`, `SEList`, `ScapegoatTree`,
  `MeldableHeap`, `XFastTrie` and `YFastTrie`
- from The Art of Computer Programming: the multiple-precision arithmetic
  (`BigInteger`, `Rational`), the sorts and the searches of volume 3
  (`sort/KnuthSorts.h`, `search/KnuthSearches.h`, `LoserTree`), the
  broadword operations and the combinatorial generation (`combinatorial/`),
  the dancing links (`ExactCover`) and the satisfiability solvers
  (`backtrack/SatSolvers.h`)
- from the notes of P. Ferragina on algorithm engineering: the two-level
  memory and its algorithms (`external/`), the integer codes, `PGMIndex`,
  rank and select, Elias-Fano and the suffix arrays

#### Engineered structures

- sequences and dictionaries: `SmallVector`, `TieredVector`, `RingDeque`,
  `Fenwick`; `FlatHashTable` and `PointerTable` (open addressing on groups
  of tags compared by vector instructions), `DenseHashMap`,
  `BucketCuckooTable`, `FastHash`, the minimal perfect hash functions
  `RecSplit` and `MinimalPerfectHash`, `AdaptiveRadixTree`,
  `SuccinctPatriciaTrie` and `BTreeMap`
- trees: the balanced trees whose only balancing is join
  (`tree/JoinTree.h`), with split, union, intersection and difference in
  the optimal work, batches and augmented maps; `SplayTree`, `LinkCutTree`,
  `IntervalTree` and `RangeTree`
- sets and priority queues: `BlockedBloomFilter`, `CuckooFilter`,
  `RibbonFilter`, `BinaryFuseFilter`, `PrefixFilter`, the range filter
  `GrafiteFilter`, `RoaringBitmap`; `DaryHeap`, `IndexedDaryHeap`,
  `PairingHeap`, `RadixHeap`, `BucketQueue`, `MinMaxHeap`, `SequenceHeap`;
  `UnionFind`, `RemUnionFind` and `RollbackUnionFind`
- sorting and searching: `sort()` and `stableSort()` (`sort/Sort.h`), which
  choose among a pdqsort partitioned by blocks, the radix sorts of integers
  and floating-point numbers and the sorts of strings; the sample sort, the
  learned sort and the vector quicksort; `DynamicPGMIndex`,
  `CompressedPGMIndex`, `search/StaticSearch.h` and
  `search/RangeMinimum.h`
- strings, compression and succinct structures: `AhoCorasick`,
  `SuffixAutomaton`, `string/SuffixSorting.h`; rANS and tANS
  (`ransCompress()`, `tansCompress()`), `streamVByteEncode()`, the r-index
  (`RIndex`, `MoveRIndex`); `RRRBitVector`, `PartitionedEliasFano`,
  `WaveletMatrix`, `LAVector`, `K2Tree`, `BVGraph` and `PackedArray`
- graphs and numerics: `StaticGraph` (compressed rows), `GridGraph`,
  `GraphBuilder`, the maximum flow of `FlowNetwork` by push-relabel,
  `Assignment`, the matchings of Edmonds; `DenseMatrix`, `CsrMatrix`,
  `SellMatrix`, `DynamicCsr` and `gemm()`
- memory: `Arena`, `ObjectPool`, `ConcurrentPoolResource`, `PageHeap`,
  `PoolAllocator`, `SizeClassAllocator`, `HugePageAllocator`, and the files
  of records of `RecordFile` for the external memory

#### Parallelism, concurrency and distribution

- the executors of one concept (`parallel/Executor.h`): `Sequential`,
  `ThreadPool` (work stealing on `std::thread`) and, as options,
  `OpenMPExecutor`, `TBBExecutor` and `FastFlowExecutor`, so that a program
  that already has one of these runtimes does not start a second one; the
  loops, the reductions (whose blocks do not depend on the number of
  threads) and the prefix sums on them
- on any executor: the parallel sorts and merges, the products of sparse
  and dense matrices, the visit in breadth that optimizes its direction,
  the components by Afforest, the shortest paths by Delta-stepping and
  rho-stepping, the LCP array, the greedy algorithms,
  `speculativeFor()`, the builds of the succinct structures, the
  long-lived workers of `run()`, the channels of `parallel/Channels.h`,
  `feedbackFarm()` and the `Mailbox` that detects the termination
- the concurrent structures of `concurrent/`, among them `SPSCQueue`,
  `MPMCQueue`, `WorkStealingDeque`, `EventCount`, `MultiQueue`,
  `StealablePriorityQueue`, `LockFreePriorityQueue`, `ConcurrentFlatTable`,
  `ConcurrentCuckooTable` and `ConcurrentUnionFind`
- with `ADS_WITH_MPI`, the processes of MPI as workers (`MPIExecutor`,
  `MPIMailbox`, with the termination by the four counters of Mattern,
  `DistributedMin`) and the sort by regular sampling of
  `distributed/DistributedSort.h`, also on a communicator of Boost.MPI

#### Typed modifications and reoptimization

- the typed modifications of `Modification.h`: the events of `ADS::mod`
  (`Inserted`, `Erased`, `Changed`, `BulkInserted`, `Rebuilt`, and so on),
  declared by each structure as a type list, the subject policies
  `NoObservers`, `Static`, `Dynamic` and `Both`, the groups of `batch()`
  coalesced into their net effect, `quiet()` and `UndoLog`; with
  `NoObservers` a structure compiles to the code of the plain one
- `Observable` and `ObservedGraph`, and the observers of
  `graph/Reoptimization.h` (`ShortestPathsObserver`,
  `SpanningForestObserver`, `MaxFlowObserver`, `AssignmentObserver`), which
  keep a solution with a certificate of its optimality across the changes
  of a graph and tell by `outcome()` whether the last solve was certified,
  warm or from scratch, as `RatioPolicy` decides
- the warm starts of LEMON (`interop/lemon/Warm.h`, `ADS_LEMON_SHIM`):
  `runWarm()` on `NetworkSimplex`, `CostScaling`, `CycleCanceling`,
  `CapacityScaling` and `Preflow`, the changes of one arc or node in
  constant time (`warmCost()`, `warmUpper()`, `warmSupply()`), and
  `PreflowEngine`, `NetworkSimplexAssignment` and `MinCostFlowObserver`
  on them

#### Contracts, capabilities and persistence

- design by contract on the whole library (`Contract.h`): `ADS_PRE`,
  `ADS_POST`, `ADS_INVARIANT` and `ADS_AUDIT`, evaluated up to the level
  `ADS_CONTRACTS` chosen at compile time (1 by default, 0 with `NDEBUG`),
  a violation throwing `ADS::contract_violation`
- the capabilities of `Capabilities.h`, which the algorithms query at
  compile time: the concepts `splittable`, `addressable` and `undoable`,
  the opt-in `enable_stable_references`, `query()`, `decrease_key_of()`
  and the customization points `reserve`, `prefetch` and `insert_bulk`
- the interface of the standard containers, with iterators of the
  strongest category that each structure allows (`Iterators.h`), so that
  the algorithms of `std::ranges` and of Boost run on them
- `store()`, `restore()` and `restoreMapped()` of the static structures
  that model `Persistent` (`external/Persistence.h`), in a versioned
  format with a checksum; `restoreMapped()` maps the file (`MappedFile`)
  and opens a structure of any size in constant time, with no copy
- the primitives that the compilers spell each in their own way in one
  form (`Portable.h`, `Wide.h`), with no test on the platform, the
  compiler or the architecture in any header; the vector kernels (the
  hash tables, the filters, the edit distance, the search of many
  patterns, the suffix sorting, the integer codes) on Google Highway with
  scalar fallbacks (`Simd.h`, `SimdInline.h`), and the layouts of the hash
  tables under an inline namespace (`Abi.h`), so that units built with
  different options link into one program

#### Interoperation

- GMP for `BigInteger` and `Rational`, a CBLAS for `gemm()`, the headers
  of Boost, Highway and hwloc, each `AUTO` by default (`ADS_WITH_GMP`,
  `ADS_WITH_BLAS`, `ADS_WITH_BOOST`, `ADS_WITH_HIGHWAY`,
  `ADS_WITH_HWLOC`), the code of ADS running without them
- Boost.Graph, Boost.Heap and Boost.Unordered (`interop/boost/`): the
  algorithms of Boost.Graph on the graphs of ADS and vice versa, the heaps
  of ADS in its Dijkstra, those of Boost.Heap as frontiers of ADS, and
  `FastHash` as a hasher; LEMON (`interop/lemon/`, `ADS_WITH_LEMON`, the
  target `ADS::LEMON`): the heaps and the maps of ADS in its algorithms,
  the graphs both ways, its notifiers told to the observers of ADS
- the executors on the runtimes of a program (`interop/parallel/`): the
  parallel algorithms of the standard library, stdexec, Taskflow and Kokkos
  (`ADS_WITH_STDPAR`, `ADS_WITH_STDEXEC`, `ADS_WITH_TASKFLOW`,
  `ADS_WITH_KOKKOS`)
- Eigen, METIS, KaHIP, SuiteSparse:GraphBLAS, NetworKit, CRoaring,
  sdsl-lite 3 and cereal (`ADS_WITH_EIGEN`, `ADS_WITH_METIS`,
  `ADS_WITH_KAHIP`, `ADS_WITH_GRAPHBLAS`, `ADS_WITH_NETWORKIT`,
  `ADS_WITH_CROARING`, `ADS_WITH_SDSL`, `ADS_WITH_CEREAL`), the C Data
  Interface of Apache Arrow, `std::flat_map` and `std::pmr`
  (`interop/std/`); each target `ADS::<name>` is linked only by a program
  that asks for it
- the C++20 modules `ads`, `ads.core`, `ads.base` and `ads.advanced`
  (`ADS_BUILD_MODULE`, with CMake 3.28, Ninja and Clang 16 or GCC 15, or
  `make ADSMOD=1`), as the targets `ADS::ADS_module` and those of the sets
- the Python package `ads-python` (`python/`) on nanobind, with the
  arrays of the structures as read-only views of NumPy, `restore_mapped()`
  and `nx-ads`, a backend of NetworkX; one wheel on the stable ABI of
  Python 3.12 and later

#### Build, packaging and requirements

- a compiler of C++20; the CI tests g++ and clang++ on Linux and macOS,
  the g++ and the clang++ of MinGW-w64 and MSVC (by CMake) on Windows, and
  g++ on Linux on ARM64
- CMake 3.21 or later (`find_package(ADS)` with `ADS::ADS`, or
  `add_subdirectory()`), pkg-config (`ads.pc`), the makefiles of `lib/`,
  Bazel (`MODULE.bazel`), Meson by WrapDB, and a Nix flake
- the recipes of `packaging/` for Debian, Homebrew, vcpkg, conda-forge,
  Conan, Spack, the AUR, MSYS2, xmake, Nix, WrapDB and the Bazel Central
  Registry
- the makefiles take the adapters of Eigen, METIS, KaHIP,
  SuiteSparse:GraphBLAS and NetworKit as CMake does, each by its variable
  `ADS_WITH_<NAME>` (`auto`, `0` or `1`) and its prefix `<Name>_ROOT` in
  `extlib/`, and nanoarrow (`ADS_WITH_NANOARROW`) for the test of the
  columns of Arrow, which CMake also finds as a shared library
- the wheels of `ads-python` built by cibuildwheel (configured in
  `python/pyproject.toml`) on Linux (manylinux, x86_64 and aarch64), macOS
  (arm64 and x86_64) and Windows (AMD64 and ARM64), one per platform on
  the stable ABI of Python 3.12, each tested by the tests of the package,
  and its sdist, which carries the sources of ADS in `libads/` and is
  tested by installing it, in `.github/workflows/wheels.yml`; at each tag
  they are attached to the release, and published on PyPI by trusted
  publishing once the repository variable `PYPI_PUBLISH` is true

#### Tests, benchmarks and documentation

- the tests of CTest from the exercises of the book, the exams of its
  course, the examples of Knuth and Ferragina, the axioms of each abstract
  data type and the edge cases of each engineered structure, run also
  under AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer
  (`ADS_SANITIZE_ADDRESS` and `ADS_SANITIZE_THREAD` defined by the build
  from the options of the compiler, the suppressions of the runtimes that
  ThreadSanitizer does not instrument in `test/tsan.supp`), with no fence
  in the concurrent structures that ThreadSanitizer does not model
- the benchmarks of `bench/` against the standard library, Boost and the
  libraries of each area, with the tables of `results/`
- the documentation at https://dmeoli.github.io/ADS/ (the theory of each
  area and the reference of the API) and the manual, whose PDF is attached
  to the release
- the CI installs the libraries of the adapters of `include/ADS/interop`
  and requires them: those that the package managers have in the jobs
  `test` on Linux, macOS and MSYS2, and all of them in the job `interop`,
  by CMake and by the makefile, the libraries that the runners lack or
  have too old (sdsl-lite 3, CRoaring, nanoarrow, GraphBLAS 10, KaHIP and
  NetworKit) built from their sources by `.github/interop-libs.sh` and
  kept in the cache; and the job `python` builds the wheel of `ads-python`
  and runs its tests, the backend `nx-ads` of NetworkX among them, on
  Linux, macOS and Windows, and on Linux the package built by CMake

[Unreleased]: https://github.com/dmeoli/ADS/compare/v0.1.0...main
[0.1.0]: https://github.com/dmeoli/ADS/releases/tag/v0.1.0

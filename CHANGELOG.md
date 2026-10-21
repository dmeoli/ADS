# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- bench/bignum.cpp: the squares, the gcd of two numbers with a common
  factor of 3 and the integer square root of 2n digits, the length of 1024
  digits of 32 bits, and FLINT (fmpz_t, fmpq_t) with FLINT=<prefix>
- BigInteger::sqrtRem() and BigInteger::sqrt(), the integer square root
  by the Karatsuba square root of Zimmermann, two digits by Newton's
  method; BigInteger::sqrtNewton(), Newton's method from above, for the
  audits; the test big-integer-sqrt
- prefixKeySort() (sort/StringSort.h), the stable sort of strings by
  integer keys of 8 of their characters from the prefix that a group
  shares, each with the place of its string in the low bits, sorted by
  vectorQuickSort(): each string read a few times in all instead of once
  per pass or comparison; sort() and stableSort() use it for strings when
  the vector registers are there, but for few distinct strings
- vectorQuickSort(): a counting sort of the keys in the nibbles of the
  lanes, with no comparison, when a sample spans a small range of
  integers (rangeSort()) or holds at most 16 distinct keys of any bits
  (hashSort(), a key found by a shifted window of its bits and checked
  against a table in the registers)
- the merge of two sorted runs in the vector registers (mergeRuns()),
  forward or backward; sort() merges with it the runs of integers of 32
  bits
- test/sorting.cpp: vector-merge, prefix-key-sort, and the patterns of few
  keys at the ends of the type, with a key apart, and of 16 keys of any
  bits

- the algorithms of the book on graphs as templates on the structures they
  use: shortestPaths() on a frontier given as a type (PrototypeFrontier;
  PriorityFrontier on any priority queue of section 10.1, its priority the
  distance or the pair ( distance , node ), QueueFrontier, StackFrontier
  and DequeFrontier), prim() on a priority queue, kruskal() on any MFSET
  with merge(), distance() on a queue, each counting its work
  (FrontierCounts, MergeCounts, VisitCounts); the versions with Frontier
  run on them
- StaticGraph, a GRAPH in compressed rows with each arc next to its
  weight and the rows sorted by head, with transpose(), relabel() and
  bfsOrder(); IndexedDaryHeap, a d-ary heap of the ids [ 0 , n ) with
  decrease-key and a vector of places, also a PRIORITYQUEUE whose handles
  are the ids; DisjointSets, the union-find in one vector by size or by
  rank with the paths halved or compressed, UnionFind the union by size
  with halving; test/engineered_graph.cpp
- bench/graphs.cpp, the algorithms of the book (the visit in breadth, the
  shortest paths on a priority queue and on a queue, Prim, Kruskal, the
  strongly connected components, the union-find) on the structures of the
  standard library, of Boost and of this library, the same arcs in the
  same order, with their counts checked equal; bench/tables.py leaves the
  rows of the counts out
- the references that the documentation of the shortest paths, of the
  maximum flow and of the properties of graphs named without citing them:
  Fredman and Tarjan, Ford, Moore, Karzanov, Malhotra-Kumar-Maheshwari,
  Karmarkar, Konig
- SizeClassAllocator, the allocator of small blocks by size classes for one
  thread (multiples of 16 up to 128 bytes, then 4 classes per power of 2
  up to 32 KB), a free list and a page of 64 KB per class, the class found
  with no table nor loop; ObjectPool::allocate() and deallocate(), the raw
  slots; their tests in test/memory.cpp and the case mixed of
  bench/memory.cpp, blocks of 8 to 4096 bytes, against new and delete,
  mimalloc, jemalloc and std::pmr::unsynchronized_pool_resource
- TieredVector, the tiered vector of Goodrich and Kloss: blocks of a power
  of 2 near sqrt( n ) items, each a circular buffer, the access in O( 1 )
  and the insertion and removal anywhere in O( sqrt( n ) ), 8 times
  faster than std::vector at 10^5 items; its test in test/sequences.cpp
  and its cells against std::vector and std::deque
- DaryHeap< T , Better , D , true >, the pop() from the bottom up (the hole
  down to a leaf by the best children, the last item up from there, Floyd
  and Wegener); the default stays the walk from the top, 1.45 times faster
  in the hold model; its test in test/heaps.cpp
- MinMaxHeap and IntervalHeap, the double-ended queues of Atkinson et al.
  and of van Leeuwen and Wood, whose runs of removals sort the items once
  and then serve both ends in O( 1 ), made a heap again in O( n ) by the
  next push; their test in test/heaps.cpp and their cells against
  std::multiset in bench/priority.cpp
- RadixHeap::extractBelow(), the items of keys below a limit, the buckets
  below it emptied whole and the one across it moved to the rows below;
  DialQueue moves its overflow into the window with it, a bucket of the
  radix heap at a time, 2 times faster on the extractions at 10^6 items
- listSort(), the sort of a List by relinking its cells, stable: the
  numbers in their natural order by the radix sort of an array of pairs
  ( key , cell ) and one pass that relinks them, 4 to 15 times faster than
  std::list::sort; any other order by the bottom-up merge of the natural
  runs (the strictly decreasing ones reversed) with an array of 64 runs,
  1.3 to 2.7 times faster; its test in test/sequences.cpp and its cells in
  bench/sequences.cpp

- PoolAllocator, the standard allocator over size classes of 8 bytes up
  to 256, rebound to any type, with the three propagation traits true, for
  the node containers and std::allocate_shared (the control block and the
  object in one slot); PoolResource, its resource for one thread, and
  ConcurrentPoolResource, the resource shared by threads, a heap per
  thread, the frees of the other threads sent back to the owner in batches
  of up to 32 by one compare-and-swap on a stack of Treiber, which the
  owner takes whole; both on PageHeap, pages of 64 KB of one class each in
  segments of 2 MB on huge pages, with a free list per page; their tests in
  test/engineered_memory.cpp and bench/allocators.cpp, against
  std::allocator, std::pmr, boost::fast_pool_allocator and mimalloc, also
  with the frees in a second thread
- PositionIndex, the index of the positions of objects in a vector on
  FlatHashTable, whose renumber() takes the sorted positions erased, drops
  their keys and moves every other position by its rank among them, in one
  walk over the table with a bit vector and the counts of its words;
  FlatHashTable::forEach() with the values to change in place; its tests in
  test/engineered_sparse.cpp and bench/positions.cpp, against
  std::unordered_map and boost::unordered_flat_map
- compressTriplets(), assembleCsc() and assembleCsr(), the compressed
  sparse columns or rows of triplets into the arrays beg, ind and val of
  the caller, the repetitions summed in the order of the input, by one
  distribution by the major index, a mark per minor index and the sort of
  each major index; DynamicCsr, the compressed sparse rows that grow by
  rows and batches of rows and shrink by sorted sets of rows in one pass;
  their tests in test/engineered_sparse.cpp and bench/assembly.cpp, against
  setFromTriplets() of Eigen, an assembly by std::map and a vector of rows
- rowFingerprint() and sameRow(), the fingerprints of dense and sparse
  rows of doubles on hashBytes(), +0 equal to -0 and every NaN to every
  NaN, and their exact comparison; their tests in
  test/engineered_sparse.cpp and bench/fingerprints.cpp, against std::hash,
  boost::hash_range and std::equal
- fft() of every size, in double and single precision: codelets of the
  sizes 2^a 3^b 5^c up to 25 besides the powers of 2 up to 64, those of
  coprime factors by the prime-factor algorithm of Good and Thomas with no
  powers of omega, the primes 7, 11 and 13 by pairs of symmetric sums,
  their tables in one with that of the powers of 2, the other primes up to
  101 by the same sums in loops, the rounds of Stockham in the fewest
  radices of the codelets, the four steps on any divisor of n with the
  last block of the lanes shorter and codelets of 2^a 3^b 5^c up to 60 on
  their rows and columns, Rader for the large primes and Bluestein for the
  sizes with a prime above 101; the four steps unrolled up to 1024 points
  (2048 in single precision), and in 2 dimensions up to 32 x 32; up to
  2^17 points the fastest of 4 factorizations, timed when the plan is made
  (ADS_FFT_TUNE); the lanes half as wide on few points; the inverse as the
  conjugate of the forward transform of the conjugates, so that each
  kernel is compiled once; the plans kept per thread for each size; their
  tests in test/fft.cpp against the definition in long double
- rfft() and irfft(), the transforms of real data by a complex transform of
  half the points and a pass on the pairs k , h - k: up to 16 points one
  codelet and the pass on its outputs; the inverse of 4098 to 131072 points
  on the four steps with the pass in their first step, the points made as the
  columns read them; elsewhere the pass on the numbers alternated, with 3
  shuffles for a vector of pairs; dct() and dst(), the transforms of the
  cosines and of the sines of types 1 to 4 as those of FFTW (Makhoul for the
  types 2 and 3, the half-length complex transform for the type 4);
  fftMany(), fft2() and fftn(), many transforms and those in more dimensions;
  convolve() in single precision
- convolve() on the four steps makes the products of the transforms as
  the columns of the inverse read them, with no pass of its own; rfft()
  and irfft() of 48 points by the complex transform of 48
- in double precision on the vectors of AVX, the many transforms of 4 to
  32 points of fftMany() and of the rows and the columns of fft2() and
  fftn(), two at a time on the numbers alternated with no transposition,
  and fft() of 8 and 16 points as two halves of the even and the odd
  points so made, then one butterfly; rfft() and irfft() by one codelet
  only up to 16 points
- the lanes of fft() by the intrinsics of SSE2 and AVX on the compilers
  with no vector extensions (MSVC), ADS_FFT_INTRINSICS to take them with
  GCC and Clang too, and ADS_FFT_LEAN, fewer codelets for the compilers
  that take long on them, the default of MSVC
- bench/fft.cpp against FFTW in double and single precision, pocketfft and
  ducc0, on complex, real and single precision data, the cosines of type
  2, many transforms, 2 dimensions and convolutions, with the cost of the
  plans apart; POCKETFFT and DUCC0 in the makefile
- InterleavedRankSelect, the layout of SPIDER of Laws et al.: each line
  of 512 bits holds 496 bits of the vector and the 1s before it since its
  superblock of 128 lines in 16 bits, so that rank reads one line, and
  select guesses the line between two samples by interpolation and
  corrects it a line at a time; its test (also from the words) and its
  cells in the benchmark against the directories of the same space
- MoveStructure, the move structure of Nishimoto and Tabei: a
  permutation that shifts each of its intervals alike, balanced so that
  no image holds 4 starts (an image cut at its third start), with the
  image of a place and its interval in O( 1 ) time; MoveRIndex, the
  r-index on two of them (LF on the runs and phi on the places of their
  starts), after the move-r of Bertram, Fischer and Nalbach; their tests
  and the cells of MoveRIndex in the benchmark rindex
- RIndex, the r-index of Gagie, Navarro and Prezza: the runs of the
  transform as the Elias-Fano sequence of their starts, their heads in a
  wavelet tree of Huffman on CompactRankSelect and the starts of the runs
  in F in one more Elias-Fano sequence, count() by the backward search on
  them, locate() by the toehold and the function phi, from the places of
  the starts of the runs and the samples at their ends; its test against
  a scan, and the benchmark rindex on a repetitive text against the
  r-index of Prezza, the FM-index of Huffman and the csa_wt of sdsl-lite
- binaryPack() and binaryUnpack(), the binary packing SIMD-BP128 of
  Lemire and Boytsov: blocks of 128 integers at the width of their
  largest, laid out over the 4 lanes of a register and packed by an
  unrolled code for each width, with the differences at a distance of 4
  for sorted sequences; patchedPack() and patchedUnpack(), its patched
  form PFor, the width of least cost and the exceptions written apart;
  their tests (also a lane at a time, ADS_BP_SCALAR) and their cells in
  the benchmark against the SIMD-BP128 and SIMD-FastPFor of FastPFor
- PartitionedEliasFano, the partitioned Elias-Fano of Ottaviano and
  Venturini on blocks of 128 items, each full, a vector of bits or
  Elias-Fano relative to the end of the block before, whichever is
  shortest, with records of fixed width holding the last item, the place
  of the code and the kind of each block, and the last items of every
  8-th block in the layout of Eytzinger for nextGEQ; its test against a
  scan on uniform, dense, run and clustered sequences, and its cells in
  the benchmark with a clustered sequence beside the uniform one
- HuffmanWaveletTree, the wavelet tree shaped by the code of Huffman of
  Makinen and Navarro: the bits of the trie of the canonical codes in one
  vector with rank and select (RankSelect or CompactRankSelect), access,
  rank, select and access with the rank of the symbol in time
  proportional to the length of its code, in n ( H0 + 1 ) bits and the
  directory; HuffmanFMIndex, the FM-index on it (the succinct suffix
  array), with the two ranks of a step of the backward search by one
  descent, LF by one descent and the marks of the samples on
  CompactRankSelect; their tests against a scan and their cells in the
  benchmarks against wt_huff_int and csa_wt<wt_huff> of sdsl-lite, on a
  text whose bytes 0 are made 1, as sdsl-lite keeps the 0 for its end
- DialQueue, the monotone bucket queue of Dial on a circular window of W
  buckets with a radix heap for the keys beyond it, the next bucket in use
  found by a mask of W bits; RadixHeap::peekKey(), the least key without
  moving the base of the buckets; their tests in test/heaps.cpp
- SequenceHeap, the sequence heap of Sanders: an insertion heap of 256
  items, groups of up to 64 sorted sequences merged in batches by trees of
  losers with the keys at hand, a buffer per group and a deletion buffer;
  its test in test/heaps.cpp and its cells in bench/priority.cpp
- SmallVector, the vector with room for N items inside itself, which
  moves to the allocator and doubles past N; its tests in
  test/sequences.cpp, with those of the engineered paths of the
  rearrangements and of the natural merge sort
- the benchmark of the sequences also against boost::container::vector,
  devector and small_vector, CursorList against std::forward_list, the
  vectors of 8 items, the stacks against std::stack on a vector and on a
  deque, the Dutch national flag against two passes of std::partition,
  the sort of a k-sorted vector against std::sort and the natural merge
  sort of a List against std::list::sort
- bench/memory.cpp, the benchmark of Arena and ObjectPool against
  new and delete, std::pmr::monotonic_buffer_resource and
  unsynchronized_pool_resource, boost::object_pool and boost::pool,
  mimalloc and jemalloc (MIMALLOC=<prefix> and JEMALLOC=<prefix> in the
  makefile, ADS_MIMALLOC and ADS_JEMALLOC in CMake, which also takes the
  sources of tlx and radix-heap from ADS_PRIORITY)

- RadixHeap, the monotone queue of integer keys of Ahuja, Mehlhorn, Orlin
  and Tarjan with digits of B bits: a key goes to the bucket of the
  highest digit in which it differs from the last key extracted, the
  least bucket in use is found by a mask, and the keys of a bucket of the
  lowest row are equal, so that they come out with no move; PairingHeap,
  the pairing heap with decrease-key, two-pass or auxiliary two-pass, its
  nodes in an ObjectPool and the passes in place on the sibling pointers;
  their tests in test/heaps.cpp
- the benchmark of the priority queues also against the binomial and the
  skew heaps of Boost, the d-ary heaps and the radix heap of tlx and the
  radix heap of radix-heap (PRIORITY=<dir> in the makefile), and in
  Dijkstra's algorithm on a random graph of n nodes and 4 n arcs, the
  queues with handles by decrease-key and the others by lazy deletion
- vectorQuickSort(), the quicksort in the lanes of the vector registers
  for the integers and the floating point numbers of 32 and 64 bits:
  the partition of Bramas by vpcompress on AVX-512 and by a table of
  permutations on AVX2, 8 registers read at a time from the end with fewer
  free places, three ways in one pass when the sample repeats the pivot,
  and the parts of at most 16 registers sorted by a network held by
  columns (the odd-even merge sort of Batcher between registers, the
  bitonic merges with all comparators upwards, a transposition); the
  floating point numbers by the integers of their order, the unsigned
  integers compared as such on AVX-512; the same algorithm on arrays where
  neither AVX-512 nor AVX2 is there, kVectorSort telling which; its tests
  in test/sorting.cpp on every size up to 600 and on the extremes, the
  zeros, the infinities and the NaN, also in the CI on AVX2 and AVX-512
- sampleSort(), the in-place super scalar samplesort of Axtmann, Witt,
  Ferizovic and Sanders for any order: up to 256 buckets by a tree of
  splitters walked with no branch, 4 items at a time, buckets of the items
  equal to a repeated splitter, the classification into blocks of 2 KiB,
  the permutation of the blocks and the cleanup, the buckets of at most 64
  items sorted by insertion at once; its tests in test/sorting.cpp
- in bench/sort.cpp the sequential IPS4o and IPS2Ra, x86-simd-sort,
  vqsort of Highway, ska_sort, fluxsort, crumsort and quadsort, from their
  sources given to the makefile by SORTING and HWY (ADS_SORTING and
  find_package( hwy ) with CMake), the A/A row of std::sort, a check that
  each sort leaves its copy in order, and their pairs in bench/summary.py
- the references of the analysis of Timsort by Auger, Jugé, Nicaud and
  Pivoteau in RunSorts.h, of the ninther of Tukey in
  EngineeredQuickSorts.h and of Boost.Sort in Sort.h
- the engineered structures of the merges of the notes of Ferragina,
  chapter 5, tested in test/engineered_pearls.cpp: FlatLoserTree, the tree
  of losers with the keys in its nodes, whose matches on the integers
  compare key and source as one integer of 128 bits and swap by masks;
  multiwayMerge(), the merge of k sorted ranges by a lazy funnel of binary
  mergers with buffers of 4 KiB, whose stretches check no end and whose
  steps have no jump; multiwayMergeSort() of ExternalSorting.h takes the
  structure that picks the least head of its runs as a parameter, the
  tree of losers of the notes by default, so that the same algorithm runs
  on the trees and the heaps of ADS, of the standard library and of
  Boost, with the same I/Os
- the benchmark bench/external.cpp: multiwayMerge() and FlatLoserTree
  against std::priority_queue, kWayMerge(), __gnu_parallel::
  multiway_merge() and tlx::multiway_merge(), with the comparisons per
  item; the mergesort of the notes on LoserTree, FlatLoserTree,
  std::priority_queue and boost::heap::d_ary_heap, with the I/Os and the
  comparisons; the samplings of n of 10^8 positions of RandomSampling.h
  against std::ranges::sample(); TLX in the makefile, ADS_TLX in CMake,
  the pairs of the summary
- the engineered combinatorial structures, beyond the book, tested in
  test/engineered_combinatorial.cpp and measured by bench/combinatorial.cpp
  and bench/backtrack.cpp: popcount() and popcountAnd(), the counts of
  arrays of words by the carry-save adders of Harley and Seal on AVX-512 or
  AVX2, pdep(), pext(), select() and the Morton codes by BMI2
  (FastBitwise.h); forEachPermutation(), Heap's method with the first
  three places in straight-line code, forEachPermutationLex(), the
  lexicographic permutations of a multiset with the last three places
  written directly, forEachCombination() and forEachCombinationMask(), the
  combinations with the last place in a loop of its own and Gosper's word
  by the trailing zeros (FastGeneration.h); FastExactCover, the exact
  covering with colors on sparse sets, the dancing cells of Knuth's SSXCC
  with the forced moves (FastExactCover.h)
- Bitwise.h cites HAKMEM, Gray and Morton, Generation.h Trotter, Johnson
  and Heap
- the algorithms of the book on the structures of std, Boost and ADS:
  BasicCdcl, the clause learning of SatSolvers.h on any heap of pairs
  (Cdcl is the one on std::priority_queue), solvePuzzle() on any map of
  the levels, subsetSumDP() on any vector of bits, each with the same
  counts on all of them, tested in test/engineered_combinatorial.cpp and
  measured by bench/backtrack.cpp on std::priority_queue, the heaps of
  Boost and DaryHeap, on std::map, std::unordered_map,
  boost::unordered_flat_map and FlatHashTable, and on std::vector< bool >,
  boost::dynamic_bitset and BitVector
- SatSolvers.h cites Davis and Putnam, the restarts of Luby, Sinclair and
  Zuckerman and the phase saving of Pipatsrisawat and Darwiche

- SellMatrix, the sparse format SELL-C-sigma of Kreutzer et al. (the rows
  sorted by length in windows of sigma, chunks of C rows by columns), its
  product in the lanes of the vector registers; the test sell-matrix and
  the cells of bench-sparse
- Philox4x32 and Philox4x64, the counter-based generators Philox4xW-10
  with the sequence of std::philox4x32 and std::philox4x64 of C++26, two
  blocks at once, discard() in O( 1 ); normalZiggurat() and
  exponentialZiggurat(), the ziggurats of Marsaglia and Tsang with 256
  layers, their tables computed in 60 digits (ZigguratTables.tpp); the
  tests philox and ziggurat and the cells normal and exponential of
  bench-numeric, Random123 (ADS_HAS_RANDOM123) among the generators
- MPMCQueue, the bounded queue of many producers and many consumers of
  Vyukov (a sequence number in each place, one compare-and-swap an
  operation); the test mpmc-queue and the cells mpmc of bench-concurrent
- EventCount, the sleep of a thread until a condition on a lock-free
  structure holds (an epoch and the waiters in one word, std::atomic::wait
  on a copy of the epoch, asymmetric fences by membarrier(2) on Linux, 20
  us of looks before the sleep), and with it waitPop() and notify() of
  SPSCQueue, waitPop(), waitPopInto() and Producer::notify() of MPSCQueue,
  waitTake() of MPSCInbox, whose push() notifies; the test event-count and
  the cells wake of bench-concurrent (latency, idle, throughput)
- WorkStealingDeque, the deque of Chase and Lev with the orders of Lê et
  al., its fences as sequentially consistent operations; the test
  work-stealing-deque and the cells steal of bench-concurrent
- COPYING, the text of the GNU GPL v3 that the LGPL v3 of LICENSE
  supplements, as the LGPL asks, installed by the recipes next to LICENSE

- the recipes of the packages, in packaging/ with the steps of each channel
  in packaging/README.md: the vcpkg port, the conda-forge recipe
  (cpp-ads), the Homebrew formula, the Debian package libads-dev with
  deb.sh for a Launchpad PPA, the Conan 2 recipe, the Spack package, the
  Nix derivation and flake.nix, the PKGBUILDs of the AUR and of MSYS2, the
  xmake package; packaging/bump.sh writes the version and the checksums
  of a release into all of them
- the backend of GMP for BigInteger, and so for Rational, behind
  ADS_WITH_GMP (the CMake option of the same name, off by default, links
  GMP to ADS::ADS and fails at configure time without it): the products,
  the squares, the quotients with and without the remainder, the exact
  quotients, the conversions in the bases not powers of 2 and the gcd go
  to the mpn and mpz layers of GMP in the ranges of lengths where these
  were faster on a Xeon Gold 5120 (the thresholds kGmp...), on the digits
  of BigInteger in place; the test big-integer-gmp at the edges of the
  ranges, make test ADS_WITH_GMP=1, the CI job gmp, and bench-bignum-gmp
  with the rows "ADS::BigInteger GMP" and "ADS::Rational GMP"
- MODULE.bazel and BUILD.bazel, ADS as the module ads of Bazel with the
  target @ads//:ads, and packaging/test as a module that uses it; the entry
  of the Bazel Central Registry in packaging/bcr and the port of Meson
  WrapDB in packaging/wrapdb, both filled by packaging/bump.sh, with the
  steps of their submission in packaging/README.md
- the source tarball ADS-X.Y.Z.tar.gz and its sha256, attached by
  release.yml to the release of each tag
- the workflow packaging.yml, which builds and tests the Homebrew formula on
  macOS, the vcpkg port and the MSYS2 package on Windows and the AUR
  package on Arch Linux
- ads.pc, for pkg-config
- the snippets of FetchContent and CPM.cmake in the README

- the engineered hashing, tested in test/hashing.cpp: FastHash, the hashes
  of the words and of the strings by folded products of 128 bits, by
  stripes of 64 bytes in vectors of AVX2 or AVX-512 past 256 bytes with
  the same values as on any machine, and DefaultHash; BlockedBloomFilter,
  the filter of Bloom in blocks of 256 bits with one bit in each of 8
  words; BinaryFuseFilter, the binary fuse filter of Graf and Lemire with
  fingerprints of 8 or 16 bits; MinimalPerfectHash, the minimal perfect
  hash function by pilots of PTHash; BucketCuckooTable, the cuckoo table
  with buckets of 4 places, tags of 8 bits and the search in breadth of
  Li et al.
- RecSplit, the minimal perfect hash function by recursive splitting of
  Esposito, Mueller Graf and Vigna, about 1.83 bits per key with leaves of
  8 keys and buckets of 100 (1.73 with buckets of 2000, 1.73 with leaves
  of 12): the seeds of the splitting trees with the lower and the upper
  aggregation, in codes of Golomb and Rice with the optimal parameter of
  each size, the keys and the places of the buckets in two Elias-Fano
  sequences less their averages, the leaves searched by the rotation
  fitting of Bez, Kurpicz, Lehmann and Sanders; leaves of 1 to 24 keys;
  its tests in test/hashing.cpp, and in bench/hashing.cpp against the
  RecSplit of sux and BBHash
- the benchmark of the hashing, bench/hashing.cpp: the hash functions
  against std::hash, boost::hash, absl::Hash, XXH3, wyhash, rapidhash and
  the hash of unordered_dense; the hash maps against std, Boost (the map,
  the node map and the flat map), absl::flat_hash_map, unordered_dense,
  tsl::robin_map and libcuckoo; the filters against the blocked filter of
  fastfilter_cpp and the xor and binary fuse filters of xor_singleheader;
  the minimal perfect hash functions against PTHash, PHOBIC, BBHash and
  RecSplit; the libraries from their sources given to the makefile by
  HASHING and ABSL (ADS_HASHING and find_package( absl ) with CMake)
- CuckooFilter, the cuckoo filter of Fan et al. with insertions and
  removals, fingerprints of 8 or 16 bits in buckets of 4 places packed in
  one word and compared at once, partial-key cuckoo hashing, 500
  evictions and a victim kept aside, an insertion into a full filter
  failing with nothing changed (loads of 0.95 to 0.97 reached, false
  positives 3.0 % and 0.012 % when full); RibbonFilter, the standard
  Ribbon filter of Dillinger and Walzer with results of 8 or 16 bits,
  built by Gauss elimination on the fly over coefficients of 64 bits and
  back substitution into a solution stored by columns in blocks of 64
  rows, a query being one parity per bit of its result (8.9 to 9.3 bits
  per key for 10^5 to 10^7 keys at 8 bits, false positives 2^-8); their
  tests in test/hashing.cpp, and in bench/hashing.cpp against the cuckoo
  filter and the standard Ribbon filter of RocksDB shipped with
  fastfilter_cpp, with the removals of the cuckoo filters timed too
- DenseHashMap, the hash map whose pairs lie in a dense vector, in the
  order of the insertions, beside an index of open addressing in groups of
  one line of cache, 12 control bytes matched at once and the 12
  positions of their pairs, so that a search costs one miss in the index
  and one in the vector; a removal moves the last pair into the place left
  free, so that a visit of the pairs is a scan of a vector; its tests in
  test/hashing.cpp, and in bench/hashing.cpp against the same layout of
  ankerl::unordered_dense and the other maps
- FingerprintPerfectHash, the minimal perfect hash function by levels of
  fingerprints of Mueller, Sanders, Schulze and Zhou, the method of BBHash
  and FMPH: in each level of gamma bits per key a key alone in its place
  keeps it, the others go to the next level, and the value of a key is the
  rank of its bit, kept by blocks of 512 bits; about 3.5 bits per key at
  gamma 2 (2.9 at gamma 1), a build of two passes per level over vectors
  that stay in the cache; its tests in test/hashing.cpp, and in
  bench/hashing.cpp against BBHash
- ConcurrentCuckooTable, in concurrent/, the cuckoo table with buckets of
  4 places and tags of 8 bits shared by threads: both buckets of a key in
  one region of at least 256 buckets, one lock per region (at most 2048),
  a counter of versions odd while held, so that an operation takes one
  lock and its cuckoo path, searched in breadth within the region, needs
  no second check; lookups without locks under the version of the region
  (a seqlock) when the keys and the values are read atomically without
  locks, with the tables replaced kept until the destruction, and under
  the lock otherwise; the tags of both buckets and the places of the first
  fetched ahead, the 4 places of a bucket on one line of cache, the
  tables on transparent huge pages; the doubling under all the locks in
  use when a region would pass the load 0.98 (at a mean load of 0.88 to
  0.95); find, contains, insert, insertOrAssign, update, erase, reserve,
  clear, forEach under all the locks, size from counters per region, and
  consistent(), the check of the invariant; its tests in
  test/engineered.cpp, on one thread against std::unordered_map and on 2
  to 8 threads, under ThreadSanitizer too, and the part "concurrent maps"
  of bench/hashing.cpp, on 1 and 2 threads (THREADS) against libcuckoo,
  boost::concurrent_flat_map and std::unordered_map under a
  std::shared_mutex
- PrefixFilter, the prefix filter of Even, Even and Morrison: an
  incremental filter for n keys whose bins are pocket dictionaries of 32
  bytes, 25 remainders of 8 bits over 25 quotients with a header of 50
  bits in unary, keeping the 25 smallest fingerprints of each bin and
  sending the others to a spare, a blocked filter of Bloom of 12 bits per
  fingerprint built for 1.1 times their expected number (5.9 % of the
  keys); a query reads one bin, compared at once with AVX-512 or AVX2, and
  the spare only for a fingerprint above the maximum of an overflowed bin,
  the bound folded in with no branch; an insertion moves the bytes of a
  bin within a register; the same answers on every machine (11.55 bits
  per key, false positives 0.39 % when full, as the analysis gives); its
  tests in test/hashing.cpp, and in bench/hashing.cpp against the cuckoo
  filters and the blocked filters of Bloom

- StringSearcher, the engineered exact search of string/StringSearch.h:
  a pattern prepared once in O( 256 + m ), two or four probes of its
  rarest bytes (four only on the patterns that hint at a small alphabet)
  compared with 64, 32 or 16 places of the text at once (AVX-512, AVX2,
  SSE2, memchr() elsewhere), four blocks a step with the verifications
  aside, the shifts of its hashed q-grams for the patterns of 64 bytes or
  more, on four segments of a long text at once with the windows in
  registers, and the Two-Way algorithm of Crochemore and Perrin, its
  scans 8 bytes at a time, once the comparisons with the pattern cost more
  than 16 times the text scanned, so that no text costs more than
  O( n + m ); find(), findAll(), count() and the interface of the
  searchers of std::search(); test engineered_strings.string-search
- suffixArray() and lcpArrayPhi() of string/SuffixSorting.h: SA-IS on 32
  or 64 bits, the types of 64 places at a time from the carries of one
  addition and vector comparisons, the partial induction on the buckets
  split by the type of the suffix before (so that each scan visits only
  the entries it moves) with the names marked during the scans, the
  groups of equal LMS-substrings sorted by the text that follows them in
  keys of 8 bytes (by insertion, or by a radix sort on the bits the keys
  do not share), then by the names that follow, then by the doubling of
  Larsson and Sadakane on few or small groups, and only then by the
  recursion; the scans in functions whose arrays do not overlap, two
  entries a step, with no branch for the complement, the text and, on
  the alphabets beyond the cache, the buckets and the places written
  prefetched; a text that never rises sorted at once; the LCP array by
  the permuted LCP of Karkkainen, Manzini and Puglisi, the text read 8
  bytes at a time after the first 4, the three passes prefetched; test
  engineered_strings.suffix-sorting
- LceIndex, the longest common extensions of a text in O( 1 )
  (string/LongestCommonExtension.h): the first 64 symbols compared
  directly, 32 at a time with AVX2, then the range minimum on LCP in
  blocks of 16 entries, one line each, with the stacks of their suffix
  minima in words, groups of 64 blocks with their sparse tables and a
  sparse table of the groups, so that a query waits for two reads of the
  memory in a row; test engineered_strings.lce
- MultiSearcher, the search of many patterns at once
  (string/MultiSearch.h): up to 64 patterns in 8 buckets by a filter of
  shuffles on the halves of the bytes of their first places (AVX-512,
  AVX2), as many places as bring the probability of a place to pass below
  2^-10; more patterns by the shift-or filter of FDR on super characters
  of 11 bits, a block of 8 places in one register of 16 bytes, of stride
  2 on patterns of 8 bytes or more when selective enough, with a table of
  the prefixes for the checks; one pattern by StringSearcher; the
  automaton of Aho and Corasick from the place where a filter lets too
  much through; AhoCorasick::forEach(); test
  engineered_strings.multi-search
- levenshtein(), lcsLength() and approximateMatches() of
  string/EditDistance.h: the bit vectors of Myers and Hyyro, one word up
  to 64 bytes, the diagonal transitions of Landau and Vishkin for the
  close strings, the band of Ukkonen with the blocks dropped by the cost
  still to pay, and all the blocks four columns at a time in the lanes of
  AVX2 once the band grows too wide; the bit-parallel LCS of Allison and
  Dix in the form of Crochemore et al.; the search of Myers with the
  cut-off of Ukkonen; test engineered_strings.edit-distance
- boyerMoore(), horspool(), shiftOr(), zFunction(), zMatching() and
  twoWay() in string/StringMatching.h, the classic exact matchers, which
  count their comparisons where asked; AhoCorasick, the automaton of Aho
  and Corasick for many patterns at once (string/AhoCorasick.h); the
  Lyndon factorization of Duval and the least rotation
  (string/Lyndon.h); the suffix automaton of Blumer et al.
  (string/SuffixAutomaton.h); the eertree of Rubinchik and Shur and the
  palindromic factorization in O( n log n ) by its series
  (string/Palindromes.h); tests engineered_strings.classic-matching,
  aho-corasick, lyndon, suffix-automaton and eertree
- the benchmarks of the exact search, of the suffix and LCP arrays and of
  the edit distance, LCS and approximate search (bench/search.cpp,
  bench/suffix.cpp, bench/edit.cpp, on the texts of bench/Texts.h)
  against std::search() with its three searchers, std::string::find(),
  memmem(), the searchers of Boost.Algorithm and, when found, libsais,
  libdivsufsort, edlib and rapidfuzz-cpp
- factorize(), the prime factors of a word by the divisions by the primes
  below 256 and the rho of Pollard with the cycles of Brent in Montgomery
  form, the differences multiplied 128 at a time before each gcd;
  NumberTheory.h
- Xoshiro256StarStar, xoshiro256** on the state of Xoshiro256, and
  AliasTable, the alias method of Walker in the construction of Vose,
  which draws an index from one word; RandomGenerators.h
- MPSCQueue, the unbounded queue from many producers to one consumer with
  a lane per producer, which never compete: the queue of Lamport on a
  circular vector that the producer replaces by one twice as large when
  full, so that a lane in a steady state allocates nothing
- multiplyStrassen(), the variant of Winograd of the product of Strassen
  on the views of the quadrants of DenseMatrix, down to gemm() below a
  cutoff, the odd dimensions peeled
- DenseMatrix, the matrices by rows in one vector, with gemm(), the
  product of GotoBLAS and BLIS (the five loops, B and A packed in
  micro-panels, a micro-kernel of 6 x 16 doubles on AVX2, of 12 x 16 on
  AVX-512 with ADS_GEMM_AVX512, portable C++ otherwise, the small
  products with only B packed and the kernels of the shape of the
  borders), multiply() and transpose() by blocks; CsrMatrix, the sparse
  matrices in compressed rows, from triplets or from SparseMatrix, with
  the product by a vector and the transpose by a counting sort, through
  256 buckets of columns when its writes would scatter; bench/matrix.cpp
  and bench/sparse.cpp against Eigen and OpenBLAS
- the registration of bench/numeric.cpp, queues.cpp, concurrent.cpp,
  geometry.cpp, matrix.cpp and sparse.cpp in the makefile, in
  CMakeLists.txt, in bench/run.sh and in the pairs of bench/summary.py
- convexHull(), the octagon of Akl and Toussaint, the radix sort of keys
  of 64 bits and the monotone chain of Andrew; closestPairSweep(), the
  sweep of Hinrichs, Nievergelt and Schorn on the points sorted by the
  radix sort, along y if it meets too many points along x, closestPair()
  if along y too; bench/geometry.cpp against Boost.Geometry and CGAL
- SPSCQueue::emplace() and SPSCQueue::tryPop(); bench/concurrent.cpp, the
  queues between two threads against boost::lockfree, rigtorp's
  SPSCQueue, moodycamel's ReaderWriterQueue and ConcurrentQueue and
  folly's ProducerConsumerQueue
- RingDeque, the double ended queue on a circular vector of 2^k places
  that doubles when full, the places reduced by a mask, the front and the
  back two counters that the operations at the two ends change apart;
  bench/queues.cpp, the queues, the deques and the stacks of ADS against
  std::deque, std::vector, boost::circular_buffer and
  boost::container::devector; test/engineered_queues.cpp
- binaryGcd(), the binary gcd of Stein with the trailing zeros counted by
  one instruction and the swap by conditional moves; Montgomery64, the
  arithmetic modulo an odd word in Montgomery form; NumberTheory.h
- RandomGenerators.h: xoshiro256++ (Xoshiro256, with jump()) and wyrand
  (WyRand), uniformBelow() of Lemire, uniformReal(), and shuffleFast(),
  the shuffle of Fisher and Yates with 2 or 6 indices drawn from one word
  (Brackett-Rozinsky and Lemire)
- reservoirSampleL(), the reservoir sampling of Li's Algorithm L, which
  draws the items to skip; RandomSampling.h
- bench/numeric.cpp, the gcd, the modular powers, the primality, the
  generators, the bounded integers, the shuffle and the sampling against
  std, Boost (Integer, Multiprecision, Random), GMP, hurchalla's Montgomery
  arithmetic and the generators of PCG; test/engineered_numeric.cpp
- the engineered ordered structures, beyond the book: BTreeMap, a B+-tree
  in main memory with nodes of 512 bytes, the keys counted by comparisons
  of vectors, the nodes prefetched and taken from pools on huge pages, and
  iterators that prefetch the leaves ahead through their parents, and with
  the counts of the pairs below each child its rank() and select(), a key
  past the largest one appended to the last leaf with no descent;
  SPlusTree, the static B+-tree of Slotin over a sorted array, and
  branchlessLowerBound(); nthElement(), the selection of Floyd and Rivest
  with the partition of Lomuto with no branch, the one of Hoare on ranges
  in order and the median of the medians past a budget of rounds;
  WordRangeMinimum, the range minima by stacks in a word and a sparse
  table on blocks of 64; intersect(), the intersection of sorted lists by
  blocks of 8 with AVX2 and by galloping; RemUnionFind, the algorithm of Rem
  with splicing; AdaptiveRadixTree, the adaptive radix tree of Leis,
  Kemper and Neumann on strings; PGMIndex, the learned index of Ferragina
  and Vinciguerra on the optimal piecewise linear approximation;
  RoaringBitmap, the compressed bitmap of Chambi, Lemire, Kaser and Godin
  with arrays and bitmaps, the arrays combined by blocks of 8 with SSE4.2,
  and its rank(), select() and lowerBound();
  test/ordered.cpp
- the benchmarks of the ordered dictionaries, of the searches in arrays
  (lower bound, selection, range minima) and of the sets (intersection,
  union of disjoint sets, boolean vectors, compressed bitmaps), against
  the standard library, Boost and, when found, Abseil, tlx, miniselect,
  sdsl-lite, the SIMD intersections of Lemire et al., CRoaring, the
  PGM-index, tsl::htrie_map and libart; bench/ordered.cpp,
  bench/arrays.cpp, bench/sets.cpp, bench/tries.cpp
- the benchmarks of the exact search, of the search of many patterns, of
  the suffix and LCP arrays, of the longest common extensions and of the
  edit distance, LCS and approximate search (bench/search.cpp,
  bench/multi.cpp, bench/suffix.cpp, bench/lce.cpp, bench/edit.cpp, on the
  texts of bench/Texts.h) against std::search() with its three searchers,
  std::string::find(), memmem(), the searchers of Boost.Algorithm, the
  SuffixArray of the textbook and, when found, libsais, libdivsufsort,
  Vectorscan (HYPERSCAN), the range minima of sdsl-lite (SDSL), edlib and
  rapidfuzz-cpp; the benchmarks print 4 decimals below 1 ns
- from Ferragina's Pearls of Algorithm Engineering: the variants of the
  maximum sub-array sum, the random sampling on disk and on streams, the
  list ranking, the two-level memory with the external mergesorts and the
  distribution sort, the loser tree, the sorted set intersections (chapters
  2-6); universal, perfect, minimal ordered perfect and cuckoo hashing, the
  Bloom variants, prefix search, front coding, the Patricia trie and the
  two-level dictionary (8-9); suffix arrays, LCP, range minimum queries and
  suffix trees with their applications (10); the integer codes, the bit
  vectors with rank and select, Elias-Fano, the succinct trees and graphs
  (11, 15); canonical Huffman, arithmetic coding, PPM, the Lempel-Ziv
  compressors, the Burrows-Wheeler transform and the FM-index (12-14);
  test/pearls_*.cpp
- the tests from the course of the book: the chapter and thematic
  collections, the exams from 2011 to 2026 with their official solutions,
  the exercise sessions and the laboratory projects, each against an
  independent oracle; test/exam_*.cpp
- the benchmarks of the dictionaries, of the priority queues, of the
  sequences, of the integers of any length and of the FFT, against the
  standard library, Boost (unordered_flat_map, flat_map, Boost.Heap,
  Multiprecision, Rational) and, when found, GMP and FFTW, with a run
  "A/A" of the reference for the noise; bench/run.sh runs them on one core
  and bench/tables.py writes their tables
- the results of two runs of all the benchmarks on an idle Xeon Gold 5120,
  in results/ with their tables, and their summary at the top of the
  README, written by bench/summary.py with the noise A/A between the runs
- the product of Karatsuba in BigInteger, between Algorithm M and the FFT
- the move constructor of FibonacciHeapPriorityQueue
- BucketQueue::popEntry(), the top item with its keys f and g
- the standard interface on every container: begin() and end() of the same
  type, the member types of the standard library, and iterators of the
  strongest category the structure allows (contiguous, random access,
  bidirectional, forward), mutable where the items can change in place;
  the common iterators in Iterators.h; nodes() and neighbors( u ) on the
  graphs, children(), preorder() and inorder() on the trees of nodes,
  members() on ListMFSet, values() on PreorderArrayTree, rowEntries() and
  colEntries() on SparseMatrix, stack( i ) on MultiStack; constructors from
  a pair of iterators and push_back()/push_front() on the sequences;
  test/interop.cpp, also with Boost.Range and Boost.Sort when installed
- the abstract data types SEQUENCE, SET, DICTIONARY, STACK and QUEUE as
  C++20 concepts (chapter 3); the LIST with pointers, the sequence on a
  doubling vector, the stack on a vector and on any sequence, the queue on
  a circular vector, the binary counter and the towers of Hanoi (chapter 4)
- the exercises of chapters 3 and 4 as tests, one ctest each, listed with
  their source in test/tests.txt, which both CMake and the makefile read
- chapters 1, 2 and 12: the minimum, the binary search, the galloping
  search and the search by interpolation; Selection, Insertion, Merge
  (recursive, hybrid, bottom-up, on lists, counting the inversions),
  Counting and Pigeonhole Sort, Quicksort as in the book, with its
  improvements and on lists, and heapsort; the sum and the
  products of binary integers, Ahmes and Karatsuba, the products of
  polynomials and the algorithm of Strassen; the solutions of the
  recurrences of the Teoremi 2.2-2.5; the exercises 12.1 to 12.13 as tests
- chapter 5: the ordered tree with pointers to parent, first child and
  sibling, its four visits, the vector of parents, the binary tree and its
  visits, the binary representation of an ordered tree and the complete
  d-ary tree in a vector; the exercises 5.1 to 5.13 as tests
- chapter 6: the binary search tree and the Red-Black tree, one class
  whose balancing is a template parameter, as a DICTIONARY; the B+-tree
  with insertion, search and the keys of an interval; the exercises 6.1 to
  6.10 as tests
- chapter 7: the table with direct access, the hash functions of the book
  (extraction, XOR, division by Horner, multiplication in floating point
  and with integers) and the middle square; the hash
  table with lists of overflow, which may bring the key found to the head
  or keep the lists sorted, and the one with open addressing, with linear
  and quadratic inspection and double hashing and the deleted places; both
  may restructure themselves; the exercises 7.1 to 7.10 as tests
- chapter 8: the SET on a boolean vector, on an unsorted and on a sorted
  list, and on any dictionary with a foreach (the hash tables and the
  Red-Black tree), with union, intersection, difference and symmetric
  difference, by merging on the sorted lists; the foreach of the search
  trees and of the hash tables; the set with the order of insertion
  (LinkedHashSet) and the multiset on a hash table; the exercises 8.1 to
  8.10 as tests
- chapter 9: the GRAPH, oriented or not, with weights on the edges and
  labels on the nodes, on lists of adjacency, on an adjacency matrix and on
  vectors of adjacency, and the incidence matrix; the visits bfs and dfs,
  the distances and their paths, the connected components, the schema of
  the dfs with the kinds of the edges, the cycles, the topological order
  (by dfs and by Kahn) and the strongly connected components (Kosaraju);
  the exercises 9.1 to 9.11 as tests
- chapter 10: the priority queue on a d-ary heap, on sorted and unsorted
  lists and on a Red-Black tree, heapBuild() and heapsort(); the MFSET on a
  forest, with the rank and the compression of the paths that can be
  switched off, and on lists; the incremental connected components; the
  exercises 10.1 to 10.9 as tests
- chapter 11: the prototype of the shortest paths with six structures,
  i.e., Dijkstra, Johnson, Fredman-Tarjan on a Fibonacci heap,
  Bellman-Ford-Moore with the detection of the negative cycles, the stack
  and Pape-D'Esopo on a dequeue, and the shortest paths of an acyclic
  graph; the dequeue of exercise 4.9;
  the exercises 11.1 to 11.10 as tests
- chapter 13: the binomial coefficients and Fibonacci, the approximate
  string matching, the edit distance, the LCS with diff, the longest
  common substring and the global, semiglobal and local alignments; the
  weighted independent intervals, the chain of matrices, Floyd-Warshall
  with the insertion of an edge, the knapsack by a table and by
  memoization in a hash table, the longest increasing subsequence, the
  subvector of largest sum and the segmentation into words; the exercises
  13.1 to 13.13 as tests
- chapter 14: the change with coins, greedy and optimal, the real
  knapsack, the independent set, the coloring and the covering of
  intervals, Kruskal (with Counting Sort for small integer weights) and
  Prim on a list, a heap or a Fibonacci heap, the code of Huffman and
  Shortest Job First; the exercises 14.1 to 14.10 as tests
- chapter 15: Shell Sort with the gaps of Knuth, Shell, Pratt and Ciura;
  the maximum flow by Ford-Fulkerson, Edmonds-Karp and Dinic, the minimum
  cut and the circulations with lower bounds; the Eulerian cycle; the
  exercises 15.1 to 15.10 as tests
- chapter 16: the convex hull, of O( n^3 ) and by Graham, with the
  pseudo-angle; the enumeration by backtrack of subsets, permutations,
  derangements and subsets of given sum, the n queens with
  masks of bits, the tour of the knight with the rule of Warnsdorff, the
  Sudoku of any size and the anagrams; the exercises 16.1 to 16.8 as tests
- chapter 17: the selection of Hoare with random pivots and the median of
  medians, the algorithm of Euclid and its extension, the modular powers
  and inverses, Miller-Rabin and the exact primality on 64 bits, a toy
  RSA, the test of the null polynomial expression, Freivalds and the
  half-satisfiability; the exercises 17.1 to 17.5 as tests
- chapter 18: the square and the linear tiling; the certificates and the
  enumerations of the problems of the chapter (clique, coloring, salesman,
  satisfiability, 0/1 programming, partition, knapsack, scheduling, exact
  cover, Hamiltonian circuits, cycles, paths and chains, longest path,
  bounded spanning tree); the nondeterminism simulated by enumeration;
  the reductions of the chapter and of its exercises; the exercises 18.1
  to 18.16 as tests
- chapter 19: the pseudopolynomial algorithms (subset sum, partition,
  scheduling, McNaughton, real-time scheduling, the three thieves); the
  approximations (ff and ffd, the triangular salesman, the gap of
  Teorema 19.2, apSubsetSum, vertex cover, knapsack); the branch-&-bound
  of the salesman with the bound of the text and of the 1-tree, the
  algorithm of Little, the clique of largest weight and the knapsack;
  the heuristics (greedyTsp, 2-opt, min-max-greedy with buckets,
  partition, superstring, star alignment, saturation degree); the fifteen
  puzzle; the exercises 19.1 to 19.19 as tests
- Conclusione: 2-satisfiability, 2-coloring, bipartite matching and the
  cliques of interval graphs, each tested against its NP-hard twin of
  Tabella 19.1
- beyond the book: binary trees in a positional vector and with cursors,
  ordered trees with cursors and in preorder, the heap with pointers, the
  graph with pointers, k stacks in one vector, deep copies of trees, the
  triangular inspection and the double hashing with a second function,
  rectangular matrices, search, order, bubble sort, natural merge sort,
  merge and deduplication of sequences, palindromes and the sorting of a
  stack, naive and KMP string matching, and some classic exercises
- the contracts of all the operations (Contract.h: ADS_PRE, ADS_POST,
  ADS_INVARIANT and ADS_AUDIT, levels 0 to 3), the declarations in include/
  and the definitions in src/, and a suite of tests of the contracts for
  each abstract data type, with the axioms of STACK, QUEUE, SET, DICTIONARY
  and PRIORITYQUEUE checked on every realization
- the specification of an abstract data type in ADT.h: the triple
  ( D , F , A ), the algebraic and the constructive semantics, the kinds of
  the operations, the requirements of abstraction and of protection
- the rule of five on the list (whose sentinel is now a member, so that a
  move allocates nothing), the sequence on a vector (which also halves at a
  quarter) and the stack on a vector, with equality and copy; the singly
  linked list of exercise 4.6 and the linked queue of exercise 4.10 as
  headers
- beyond the book: the list with cursors in a shared space with a free
  list; the range of C++ over a sequence; the search with the place of
  insertion and the sublists; the postfix expressions and the
  shunting-yard; the queue with no duplicates and the positive items of a
  queue; the visit of a stack from the bottom; the sieve of Eratosthenes on
  a set; the dictionary on a sorted vector; the pseudo-random inspection;
  the union by size in the MFSET; the decision tree of the binary search;
  the trees as parenthesized lists; the tree of an expression; the tree
  with lists of children; the heap with a pointer to its last leaf; the
  general visit of a graph, the unilateral connection and the adjacency
  from the incidence; the order of the orders of growth; the general
  schemes of the techniques; the strong failure function of Knuth; the
  gaps of Hibbard; Merge Sort with no merge of parts in order; Insertion
  Sort with a sentinel; the partition around a value; the minimum and the
  maximum together; the greedy and the backtracking knapsack; the iterative
  n queens; the eight puzzle by breadth and by depth with any goal; the
  factorial, the bounds of the binomial and of Fibonacci, and Fibonacci
  with memory between the calls
- engineered structures: the two-level bucket queue for integer keys, the
  d-ary heap of plain items, the hash table of pointers with linear probing
  and Fibonacci hashing, the arena of objects in blocks, SplitMix64,
  Zobrist and simple tabulation hashing, the lock-free queue from one
  producer to one consumer and the lock-free inbox of many producers, with
  their tests on real threads
- the sorts beyond the book: bubble, cocktail shaker, odd-even
  transposition, comb, gnome, cycle, pancake, tree, patience, strand and
  smoothsort, the sorting networks of Batcher, stooge, slow and bogo sort;
  introsort, pdqsort, the dual-pivot and the three-way quicksort, Timsort
  and Powersort, the LSD and the MSD radix sorts, the American flag sort,
  the bucket sort and the multikey quicksort; ADS::sort() and
  ADS::stableSort(), which choose among them; the benchmark bench/sort.cpp
  against the standard library and Boost.Sort
- from The Art of Computer Programming: the Fibonacci search and the
  uniform binary search, the AVL tree, the B-tree with removal, the trie
  and the compact trie, the coalesced hashing with a cellar and Brent's
  double hashing, with the skip list, the treap, the splay tree and the
  Bloom filter (vol. 3); the sorts by binary and list insertion, address
  calculation, radix exchange, tournament and merge insertion, the
  replacement selection and the polyphase merge (vol. 3); the threaded
  tree, the boundary-tag and buddy allocators, the heap of cells with the
  marking of Deutsch, Schorr and Waite, the sparse matrix on orthogonal
  lists (vol. 1); the linear congruential generators with the statistical
  and spectral tests, the shuffling and the sampling, the FFT, the integers
  of any length and the rationals (vol. 2); the broadword tricks and the
  generation of tuples, permutations, combinations and partitions (vol.
  4A); the exact cover by dancing links, DPLL and CDCL (vol. 4B)
- move constructors for SkipList and SplayTree
- a CI job that runs the tests under AddressSanitizer and UBSan
- install and export for find_package(ADS), and the CI on GitHub Actions
- the general algorithms of the exams in the library: strings, dynamic
  programming and optimization, searching and sequences, graphs and trees;
  test/lib_*.cpp
- the edge cases in every test (the empty input, one and two items, ties,
  the extreme values of the types, chains of 10^5 nodes, full and empty
  structures) and a violation of each precondition by CHECK_THROWS
- FlatHashTable, the engineered hash table of ADS: open addressing with
  one control byte per place (empty, or 8 bits of the hash), the places in
  groups of 15 whose 16 bytes, a byte of overflow among them, are matched
  at once by SSE2 on x86-64, by NEON on ARM64 and by SWAR elsewhere (or
  wherever ADS_FLAT_SIMD=0), the byte of overflow ending the searches for
  absent keys and replacing the deleted marks, the hash mixed by a product
  of 128 bits, a power of 2 groups, at most 7 / 8 of the places full, the
  first line of the pairs of a matching group prefetched; the table that
  bench/dictionaries.cpp compares with boost::unordered_flat_map
- ObjectPool, a pool of objects in blocks that double, the slot freed
  last made again first (the list AVAIL of Knuth), so that the nodes of a
  linked structure cost no call to the allocator each and lie next to each
  other; HugePageAllocator, which aligns the blocks of 2 MB or more to the
  huge pages and, on Linux, advises them for the transparent huge pages,
  the default allocator of ObjectPool
- WaveletMatrix, the wavelet matrix of Claude, Navarro and Ordonez: a
  sequence of integers of w bits as w vectors of bits with rank and
  select, with access, rank and select of any symbol, the k-th smallest
  item of a range and the items of a range below a value, in O( w ) time;
  test/engineered_succinct.cpp
- huffmanCompress() and huffmanDecompress(), the code of Huffman of bytes
  as the fast coders write it: optimal lengths of at most 11 bits by
  package-merge (limitedHuffmanLengths()), the codewords from the low bit
  of a word in four streams written and read together, a table of 2^11
  entries that gives one or two symbols a lookup
- ransCompress() and ransDecompress(), rANS of order 0 of Duda with four
  states in turn on one stream of 16 bits words, the frequencies scaled to
  2^12 (scaleFrequencies()), the division of the encoder by a reciprocal
  and the renormalization with no branch
- streamVByteEncode() and streamVByteDecode(), the Stream VByte of
  Lemire, Kurz and Rupp: the lengths of 4 integers in a control byte apart
  from their bytes, moved by one shuffle of SSSE3 (and SSE4.1 in the
  encoder) from a table of 256 masks
- the benchmarks of the succinct structures and of the compressors,
  bench/succinct.cpp and bench/compression.cpp, against sdsl-lite, sux,
  pasta, succinct, huff0 and FSE, Stream VByte, libsais and LZ4 when found
- CompactRankSelect, rank and select in about 3.5% of the bits: an entry
  of 128 bits per 4096 (the rank of the superblock and those of its 8
  blocks of a line each), the line of the block summed by the nibbles
  with AVX2, and the superblock of every 4096-th bit of each value for
  select; test/engineered_succinct.cpp and test/succinct_top.cpp
- tansCompress() and tansDecompress(), tANS of order 0 of Duda with 2^11
  states dealt by the step of Zstandard: two halves of the string, each
  with its own stream of bits read backward and four states, decoded
  together; the states by tables, a symbol by a lookup, a shift and an
  addition
- DirectAccessCodes, the directly addressable codes of Brisaboa, Ladra and
  Navarro: the integers cut in chunks of the widths that minimize the bits
  (their dynamic program on the counts of the lengths), a level per chunk
  with the continuation bits on RankSelect, and access in time
  proportional to the length of the item; levels of a fixed width too
- lz77ParseLinear(), the parse of LZ77 with no window of Karkkainen, Kempa
  and Puglisi in O( n ) time, by the previous and the next smaller values
  of the suffix array, instead of the O( n w ) search of the window
- RRRBitVector, the compressed vector of bits of Raman, Raman and Rao in
  the form of Claude and Navarro: blocks of 63 bits as their class and
  their offset in the combinatorial number system, superblocks of 32
  blocks with their rank and the place of their offsets at their widths,
  in at most m H0 + 0.175 m bits, with access, rank and select, which
  walk the places of the block only down to what they need

### Changed

- suffixArray() of a text of bytes sorts its LMS-suffixes directly, as
  strings, before the induction: the windows of 64 bits of the text packed
  in codes of the ranks of its bytes (1 to 6 bits, or the bytes when more
  than 64 occur), a radix sort on their top bits (by the 256 buckets of the
  top 8 first on large texts) and the next windows for the ties, with no
  partial induction, no names and no recursion; a sample of the windows
  that repeats, or ties beyond a budget, leave the text to the induction.
  On the induction the names are those of the groups the text leaves
  (finer than the LMS-substrings), the marks of the sorted LMS-suffixes
  cleared 8 at a time, the names gathered forward (16 at a time by a
  compress on AVX-512); the final induction marks a suffix by its top bit
  and prefetches by one select, and the kinds of the places are counted
  from counters built by vector operations. suffixArray(), plcpArray() and
  lcpArrayPhi() build their arrays on transparent huge pages (Linux), and
  lcpArrayPhi() reads the permuted LCP from a copy of n bytes on texts of
  2^18 bytes or more and compares one symbol at a time. Against libsais
  2.12 on wolfe, bench/suffix.cpp as built by the makefile, two runs
  alternated with the previous version (ratios libsais / ADS at 10^5,
  10^6, 10^7 bytes): random over 4 bytes 1.30, 1.35, 1.75 (0.83, 0.87,
  0.90 before), over 26 1.07, 1.80, 2.25 (0.85, 1.27, 1.20), over 256
  0.86, 1.03, 1.54 (0.80, 0.87, 0.94), English 1.02, 0.96, 1.12 (0.97,
  0.82, 0.88), Fibonacci 0.84, 0.98, 1.20 (0.82, 0.84, 0.91), repeats
  0.82, 0.99, 1.09 (0.82, 0.85, 0.92); the LCP 1.08 to 1.21 at 10^5, 0.98
  to 1.18 at 10^6, 1.31 to 1.71 at 10^7 (0.91 to 1.18, 0.96 to 1.13, 0.91
  to 1.05). The cells below 1 are those of the alignment of the jumps
  (the JCC erratum of Skylake): with the assembler told to keep the jumps
  within 32 bytes on both libraries the cells of 10^5 are 1.03 to 1.05
  and English at 10^6 0.98 to 1.00; the tests in
  test/engineered_strings.cpp
- vectorQuickSort(): the partition prefetches the blocks 8 U W keys ahead
  of each end, nearly halving a pass over a part out of the cache of the
  second level; on AVX-512 the keys of 64 bits are partitioned by a table
  of 256 permutations and one vpermq instead of two compressions; the
  column network also serves 8 registers of 16 keys, with a masked store
  per block, and reverses the lanes by one permutation; floating point
  numbers are sorted as the signed integers of their bits and the block
  of the negative ones reversed at the end, instead of two conversions of
  the whole vector
- sort() and stableSort(): a vector of 16384 items or more is checked for
  one run first, a strictly decreasing one reversed in the same pass that
  checks it, from both ends, and an ascending one scanned with
  prefetching, in four streams for items of at most 8 bytes past the
  cache of the second level; the scan of the first run goes on from where
  that check stopped

- HugePageAllocator keeps the blocks of 8 MB or more given back, up to 16
  blocks and 64 MB per thread, for the next requests of the same size, which find
  their pages faulted in and huge: an ObjectPool built again and again
  3.5 times faster at 10^6 objects (7 ns against 25, mimalloc 16.5), 1.3
  times in the churn; the test in test/memory.cpp

- SequenceHeap pushes into an empty heap in bulk, an array in the order of
  the pushes, whose best item a first look finds by a scan and which then
  becomes an incremental quicksort (Paredes and Navarro), a further source
  of the deletion buffer; the insertion heap of 4096 items is 4-ary and
  made a heap only when a top() or a pop() needs it, the groups hold 16
  sequences, the new sequence is merged with the two buffers by a merge of
  two with no jump, and the numbers and the pairs of numbers in the order
  of std::less or std::greater are compared in the merges by keys of one
  word and sorted by the radix sort (OrderKey.h): at 10^6 items 1.8 times
  faster than DaryHeap on the insertions, 1.4 times in the hold model and
  1.6 times on the extractions
- fft() takes every size, no longer only the powers of 2; convolve()
  transforms the smallest even size 2^a 3^b 5^c that holds the product,
  or the power of 2 if that size is not below 3 / 4 of it, by rfft() and
  irfft(); the arrays of doubles of ADS_FFT_SIMD=0 are aligned and
  operated on by loops, which the compilers vectorize, and take the four
  steps unrolled too

- EliasFano::nextGEQ() scans at most 8 items of the bucket of x and then
  finds the end of the bucket by select0 and the item by a binary search
  on the low bits, O( log c ) for a bucket of c items instead of O( c )
- DirectAccessCodes reads a chunk of 8 or 16 bits as its byte or its
  pair of bytes, and builds a level by packing its chunks and its
  continuation bits in order, a field of 64 bits at a time, from the
  integers that reach it with no copy of the first level
- CompactRankSelect builds its directory a superblock at a time with no
  test on the words, the counts of the blocks packed by constant shifts,
  and reserves its samples, and a vector from its words copies them as it
  counts them above 2^23 bits, as RankSelect; the tests of the building
  from the words against the one from a BitVector, also with the top
  regions of 2^13 bits
- RankSelect builds the inventory of select of a value at its first
  select, once whatever the threads (std::call_once behind a flag read
  with acquire semantics), so that a directory that only ranks never pays
  for it, and a copy keeps the inventories built (the first call out of
  the line of select, which stays inlined); the directory takes the
  prefix sums of the sideways sums of a block by one product on their
  bytes, in a loop per top block with no test, and a vector from its
  words copies them as it counts them above 2^23 bits; the top level is
  kept only above 2^40 bits; directoryBits() counts the inventories asked
  for, built or not; the benchmark builds it from the words and has a
  cell of the building with a select of each value; the tests of the
  first select from 8 threads, of the copies and of the building from the
  words
- HeapPriorityQueue and LinkedHeapPriorityQueue take their items and
  nodes from an ObjectPool, the slot of deleteMin() taken by the next
  insert(), instead of one allocation each: 3 to 4 times faster on the
  insertions, 1.1 to 2 times on the extractions, 1.2 to 1.6 times in
  Dijkstra's algorithm; both are moved, not copied, as before

- dutchNationalFlag() moves the items of a trivially copyable type with no
  branch, the place of the swap computed from the colour, the same swaps
  as before, 1.5 times faster on random colours; sortKSorted() replaces the
  root of its heap of k + 1 items by the next item with one sift down
  instead of a pop and a push, 1.3 times faster; naturalMergeSort(),
  mergeSorted() and sublist() append at the position they keep after the
  last item instead of finding it by tail(), which scans a singly linked
  list, and the natural merge sort merges the runs straight into the
  result, 2 times faster on a List

- BucketQueue keeps its items in one vector of cells linked in lists, the
  cells freed by pop() taken first by push(), and the heads of the lists
  of g < 4 in the layer itself: a layer costs no allocation unless its g
  reaches 4, and a push or a pop none once the vector has grown, 4 to 8
  times faster in the hold model of bench/priority.cpp
- sort() and stableSort() of the integers and the floating point numbers
  of 32 and 64 bits, when the vector registers are there, go to
  vectorQuickSort() instead of the radix sort and of pdqSort() (but the
  integers of 64 bits whose sample differs in one byte, which go to the
  counting sort of radixSort()), and to the merges of powerSort() only up
  to 8 runs for the keys of 32 bits;
  stableSort() goes the same way on the integers, and on the floating
  point numbers when no -0 is there
- sort() of the items with an order of their own goes to sampleSort():
  from kRadix items if they are copied as bytes and in no few runs or few
  keys, instead of the merge sort through a buffer, and from kRadix items
  otherwise, instead of pdqSort()
- RibbonFilter keeps its solution in lines of cache aligned to 64 bytes,
  8 columns of a block per line, and answers a query 8 columns at a time
  (AVX-512 or AVX2, the parities of the bytes by vpshufb and vpsadbw, a
  popcount per word on any machine), stopping at the first line that
  disagrees, so that a key out of the set reads 2 lines whatever the
  results; it takes the coefficients from a product of 64 bits, prefetches
  the rows of the key 16 places ahead in the banding, and with AVX-512
  solves 4 rows at a time in the back substitution; the answers are the
  same on every machine, its tests in test/hashing.cpp now also for
  results of 32 bits and for the answers of every ISA
- CuckooFilter takes the fingerprint from the high bits of a product of
  64 bits of the hash instead of a folded product of 128 bits, tells
  whether one lane of the two buckets is equal by one test of a zero lane
  and reads the victim only when there is one
- PrefixFilter keeps in the 6 bits above its header the quotient of the
  largest fingerprint of an overflowed bin (63 before), so that the bound
  of a query is two loads and a shift, read only for a key not found in
  the bin, takes the run of a quotient from the two bits of its pdep with
  no count of zeros, and answers the rare queries of the spare out of line
- bench/summary.py compares PrefixFilter with the cuckoo filter of
  fastfilter_cpp only, of the same false positives, not with its blocked
  filter of Bloom

- FlatHashTable removes a key of its first group by writing the 16
  control bytes of the group whole, at an address known from the hash
  alone, and bounds the places left counted by one per removal, counting
  them exactly only when the bound fills the table: the removals of random
  keys overlap their misses as the searches do, 1.6 to 1.9 times faster
  (31 to 35 ns against 55 to 65 per removal from 10^6 keys of 64 bits on
  wolfe, against 35 to 38 of tsl::robin_map and 53 to 74 of
  boost::unordered_flat_map); the rebuilds of FlatHashTable and of
  BucketCuckooTable prefetch the memory of the string keys 8 keys ahead,
  and the doubling of BucketCuckooTable hashes the keys in a first pass
  and prefetches the first bucket of each key 8 keys ahead in the second,
  1.05 to 1.2 times faster insertions of strings; FlatHashTable passes a
  small key by value to the search past the first group, so that a search
  stores nothing
- RecSplit keeps the numbers of keys before the buckets and the places of
  their codes in Elias-Fano sequences of its own, with the place of every
  64th 1 of the high bits, 64 bits per 64 buckets, and takes the numbers of
  keys before a bucket and before the next from one select, and selects
  in a word by PDEP where the machine has BMI2: a query costs 111 ns
  instead of 300 for 10^6 keys on wolfe, the same bits per key
- hashBytes() computes the product of the strings of 4 to 8 bytes and that
  of the shorter ones in their own branches, the same values with one
  multiplication fewer: GCC compiled the product shared by the two
  branches as a product of 128 bits

- SPSCQueue: a consumer whose last pop took an item and that finds only a
  few new ones waits 64 pauses for a batch
- bench-concurrent: the processors read once at the start (the threads
  created after a pin ran on one processor, or on processor 0), and
  moodycamel::ReaderWriterQueue built with the same 1024 places
- SPSCQueue: a consumer that finds the queue empty prefetches its next
  place while it reads the tail again (one miss a side per round trip)
- MPSCQueue: a lane is a list of blocks of B places that the consumer
  gives back to its producer, which reuses them before it allocates, and
  a consumer right behind a streaming producer waits a moment for a batch;
  B is now the places of a block
- binaryGcd() on x86-64 counts the trailing zeros of b - a, so that a
  step is a subtraction, a count and a shift (5 cycles instead of 7)
- powMod() below 2^32 in a Montgomery arithmetic of 32 bits with no
  constant but the inverse of m and no form of 1
- isPrime() below 2^32 by one base, read in a table of 1024 at a hash
  of n (found by an exhaustive search), in a Montgomery arithmetic with
  no constant but the inverse of n
- BigInteger past GMP on the short products: the products by passes of 4
  rows in assembly, the window of 5 places in registers on the two chains
  of carries of ADCX and ADOX, about 1.4 cycles per product on a Xeon Gold
  5120 against the 1.45 to 2.2 of the basecase of GMP 6.3; the squares by
  the same passes across the tiles of 4 digits and the squares of the
  tiles; the products by one digit and the sums with any count of digits
  in assembly, 1, 2 and 4 steps before the loop of 8 with no table of
  jumps, the submultiples of up to 8 digits with no loop; the sum of the
  middle of Karatsuba in two passes on the two chains; Toom-3 with the
  interpolation of Bodrato from kToom3 digits; the short divisions on the
  stack (kDivShort); bench/bignum.cpp repeats the short operations in a
  run, so that the clock of 100 ns of a virtual machine reads them
- BigInteger: the products by the NTT leave the top digits of the longer
  factor to rows of products by one digit when the transform of the rest
  is shorter by more than they cost (1025 digits: 1.07 times GMP instead
  of 0.63); the quotient alone of a divisor longer than it by the top
  m + 2 digits of the divisor and 2 m + 1 of the dividend, checked by the
  remainder of the top digits and, rarely, by one product (a quotient of
  5 digits by 4096: from 140 times slower than GMP to about its time)
- BigInteger: the carries of the NTT by three passes of products by one
  digit on the three residues, each below 2^50 and so one digit; Newton's
  reciprocal with a x_h modulo b^N - 1, N = length( n ) or n, the three
  values of t = b^{n+h} - a x_h told apart by its lowest digit, and the
  correction by the h + 1 digits of t that are not 0 (from 2.9 to 1.8 to
  2.1 products of the same length); Algorithm D by blocks of 4 digits of
  the quotient from kDivBlock digits, each block from the top 6 digits of
  the divisor and subtracted by a pass of 4 rows of the complements
  (subPass4()), also with the divisor cut; the submultiples by blocks of
  4 digits on the chains of ADC and SBB; the conversion to a base by
  fractions below kConvertF digits: the remainder x < P^{2^j} times the
  reciprocal of P^{2^j}, then halves of the chunks by the band of the
  products that the fraction of f P^h needs (mulBand()), rounded up or
  down as the fraction below says, so that a run of 9 or of 0 does not
  carry wrong, the leaves by products by P; the powers and their
  reciprocals kept per thread and base; the figures of a chunk of 10^19
  by pairs from a table
- BigInteger: the passes of 6 rows (window of 7 places, 31 instructions
  for 6 products), about 1.22 cycles per product on long rows and 1.38 on
  32 digits against 1.70 for the basecase of GMP; Algorithm D by blocks
  of 6 digits of the quotient from kDivBlock6 digits of divisor
  (subPass6()), 1.06 to 1.18 times the schoolbook division of GMP from
  128 digits; kKaratsuba 40 and kDivide 96
- BigInteger: the blocks of digits freed kept per thread (DigitPool), 8 of
  each capacity 2^k up to 2^16, the capacities rounded to them, so that
  the temporaries cost no allocation (the product of 8 digits by 8: from
  171 to 129 cycles), the move to the heap and the release out of line,
  so that the numbers in place cost a test (H_n of 1024 terms: from 490
  to 437 thousand cycles); the documentation of the products, the divisions
  and the conversions with their formulas, and the references of
  Karatsuba and Ofman, Toom, Bodrato and Zanoni, Krandick and Jebelean,
  Barrett, Bouvier and Zimmermann, Lehmer; kToom3 120, kHensel 20
- BigInteger::gcd() by Lehmer on two digits and by the half gcd of
  Moeller from kGcdDC 600 digits (hgcd() from kHgcd 64): the steps of
  Euclid on the leading digit alone down to 2^33, their matrix applied
  to the leading 128 bits, and the leading 64 bits of the result again,
  instead of two-digit arithmetic; the matrix applied to both numbers in
  one pass of sums of products (rowsAsm(), the subtraction folded into
  the complement); against mpz_gcd() of GMP 6.3.0 from 0.12 to 0.39 times
  to 1.05 to 1.33 times on 4 to 1024 digits, 0.96 to 0.99 on 2048 and
  4096
- BigInteger: Toom-4 from kToom4 digits, its interpolation from the
  values at 0, 1, -1, 2, -2, 1/2 and infinity on nonnegative numbers
  only; the squares by thresholds of their own (kSqrKaratsuba, kSqrToom3,
  kSqrToom4, kSqrFFT), the square of the school by blocks of 6 digits
  (the passes of 6 rows after each block, tri6Asm() inside it, the
  squares of the digits added in the pass that doubles, sqrDiagAsm()),
  and the NTT of a square with one transform even where the rows take
  the top digits of a product
- BigInteger: the unbalanced products by Toom-3/2 (a up to 7/4 as long
  as b) and Toom-4/2 (up to 5/2) from kToom32 digits, instead of slices
  of a as long as b

- the CMake package ADS installed under share/cmake/ADS instead of
  lib/cmake/ADS (ADS_INSTALL_CMAKEDIR), since a library of headers is the
  same on every architecture

- BigInteger closer to GMP: the loops of the sums and of the products by
  one digit in assembly with MULX, ADCX and ADOX where GCC or Clang build
  for BMI2 and ADX (ADS_BIGINT_NOASM keeps the C++); the NTT on lanes of
  doubles of AVX2 with FMA, one digit per term modulo three primes below
  2^50, on 2^k or 3 2^k terms, two rounds per pass, and the NTT on pieces
  of 32 bits on lanes of AVX2 with the products of Shoup; the trial
  quotients of Algorithm D from three digits by two and the reciprocal of
  a digit with no division (Moeller and Granlund); the division by
  Newton's reciprocal by blocks, the products by the divisor modulo
  b^N - 1, from 768 digits; the quotient alone by the divisor cut digit
  by digit, or with no last remainder; the reciprocals of the powers kept
  by the conversion to a base; the Karatsuba middle sum shared; the
  numbers of one or two digits with no allocation; the thresholds tuned
  against GMP on a Xeon Gold 5120

- Rational: the divisions by a gcd exact by Hensel's division when the gcd
  has one digit, BigInteger::divExact(), and the sum of the products in
  place, BigInteger::addMul()

- FlatHashTable hashes by DefaultHash< K >, FastHash for the integers and
  the strings, std::hash for any other key: the lookups of strings take
  from 1.17 to 1.52 times less than with std::hash, the words as long
- BloomFilter, PartitionedBloomFilter and SpectralBloomFilter reach the
  places h1 + i h2 mod m one from the other by an addition, with no
  remainder past the first two, and PartitionedBloomFilter hashes an item
  once instead of once per part
- closestPairSweep() falls back, after the sweeps along x and y, to the
  randomized grid of Golin, Raman, Schwarz and Smid, O( n ) expected on
  any input, instead of the divide et impera of the book
- RingDeque moves its items into a larger vector by a function that does
  not see the deque, so that the loops of its operations keep it in
  registers
- the documentation of the numeric, random, sampling, geometry, queue and
  concurrent headers touched: the mathematics in LaTeX and the references
  checked against their doi
- SPSCQueue keeps its indices and their copies on four lines of cache,
  in pairs of lines that the adjacent line prefetcher cannot make shared
  between the threads, its places in raw memory padded by a line at each
  end, and builds and destroys the items in them, so that T need not have
  a default constructor
- ArrayQueue reduces its places by a comparison instead of a division,
  and LinkedQueue takes its cells from an ObjectPool
- powMod() multiplies in Montgomery form when the modulus is odd, and
  mulMod() takes the remainder of its product of 128 bits by one division
  of 128 by 64 bits instead of a call to the division of 128 by 128
- isPrime() divides by the primes up to 211 by one product each, all at
  once, and runs the test of Miller and Rabin in Montgomery form (of 32
  bits below 2^32, with lazy reduction below 2^62) on the fewest bases
  known to be exact below n (1 base below 341531, below 2^32 the base 2
  and a second one read by a hash of n in a table of 32, made for the 2314
  strong pseudoprimes to the base 2 there, 7 bases for every word), the
  bases in lockstep and the powers with no branch on the exponent; powMod()
  reduces lazily below 2^62
- the documentation of the succinct structures and of the compressors
  cites every paper whose method or analysis they use, with its doi, and
  writes their bounds in LaTeX; the README lists the compact directory,
  the vector of RRR, tANS, the DACs and the linear parse of LZ77 among the
  engineered structures
- the papers of SplitMix64 in SkipList.h and Treap.h, of the trick of the
  Four Russians in RangeMinimum.h and of Patricia in TwoLevelDictionary.h
- BoolSet keeps its vector as words of 64 bits, and its operations of two
  sets, min(), max() and forEach() go a word at a time, the operations of
  two sets counting the bits of the result in the same pass, 4 words at a
  time with AVX2; contains() tests the bounds with one comparison
- nthElement() checks whether a range is sorted on raw pointers when the
  iterators are contiguous, so that the check is vectorized
- the union of Rem is RemUnionFind, next to UnionFind, and the benchmark
  of the sets measures both
- BigInteger on digits of 64 bits instead of 32: the product of two
  digits and the sums with carry by unsigned __int128, by the intrinsics
  of MSVC on x64, or by halves of 32 bits (forced by ADS_BIGINT_PORTABLE);
  the NTT cuts each digit in two pieces of 32 bits, the conversions to
  base 10 take 19 figures per digit; limbs() counts digits of 64 bits; the
  remainder by one digit below 2^60 folds 4 digits per step; the digits
  are no longer zeroed before they are written; the thresholds tuned again
- BigInteger without its quadratic steps: the recursive division of
  Burnikel and Ziegler above Algorithm D, the conversions of base by
  divide and conquer (and by slices of bits in the bases 2^k), the gcd of
  Lehmer, the left-to-right binary power with squares, Karatsuba on one
  scratch buffer, the divisions by one digit through a reciprocal
  (Moeller and Granlund); Rational skips the divisions by a gcd of 1;
  divModSchool(), toStringSchool(), fromStringSchool() and gcdEuclid() keep
  the quadratic algorithms for the audits and the tests
- OpenHashTable::scan() adds the constant stride of the linear inspection
  and of the double hashing instead of computing H( k , i ) by four
  divisions per probe; the places are the same
- FlatHashTable iterates a group at a time, by the mask of its full places
  and a sentinel group after the last, instead of place by place; its
  rebuild places the pairs by a count of the full places of each new group
  and never reads the control bytes it has just written; an insertion
  takes the place of a new key in the first group of its search when it
  has one; the search past the first group and the rebuild are out of
  line; the groups carry the sentinel
- the licence is the GNU Lesser General Public License version 3.0
- the definitions of the templates move from src/ADS/<area>/X.tpp to
  include/ADS/<area>/impl/X.tpp, as in Boost: include/ is the whole library
  and the only directory to put on the include path
- the option ADS_BUILD_TESTS, by default on only when ADS is the main
  project, so that add_subdirectory() and FetchContent build the interface
  target alone
- the documentation is a site of Sphinx with the theme of Read the Docs,
  its pages written from the README by docs/pages.py, with the API
  reference built by Exhale and Breathe from the XML of Doxygen
- fft() is one unrolled codelet up to 64 points and above it the four
  steps of Bailey, with no reversal of the bits: the columns, then the
  rows, transformed 4 at a time in the lanes of the vectors of AVX (2 on
  SSE2 and NEON, arrays of doubles elsewhere or with ADS_FFT_SIMD=0), by
  codelets of up to 64 points and, above them, by the rounds of Stockham
  on 4 columns or rows gathered at a time with the next rows prefetched;
  the blocks between the steps are transposed in registers, those of the
  outputs start on a line of the cache; the powers of the root of unity
  are computed once per thread for every size and both directions, each
  with its angle taken to [0, pi/4]; convolve() transforms each real
  vector by a complex transform of half its length, and the product back
  by another, 3 half transforms instead of 3 whole ones
- PreorderArrayTree::end( i ), the place after the subtree of i, is now
  subtreeEnd( i ), so that the name end() is left to the standard interface
- SequenceRange is a view whose end() is an iterator, so that the range is
  common
- FlowNetwork, the flows, the b-matching and countInversions() count in
  long long, which has 64 bits also where long has 32 (Windows), and so
  do the ends of a WeightedInterval
- the visits of the trees and of the graphs, their destructors and the
  blocking flow of Dinic use an explicit stack, so that chains of 10^5
  nodes do not overflow the stack
- linked() of BinaryTree and Tree checks the links of the node alone
- sort() and stableSort() count the runs of a long vector of numbers or
  strings and give at most kRuns of them to powerSort(), so that organ
  pipes and few sorted runs merge instead of going through the radix
  sort; one run is left as it is, or reversed when strictly decreasing,
  in the same pass that counts it, and more runs are merged where that
  pass found them, with no second search; sort() gives to pdqSort() the
  doubles with few distinct wide keys
- radixSort() finds in one first sweep the bytes in which the keys
  differ, and counts and passes over those alone; when they differ in one
  byte, a counting sort writes the numbers back from one of each value,
  with no buffer and no pass that moves them
- msdRadixSort() skips the prefix that a group of strings shares, keeps
  the character of each string aside and moves the strings once per pass,
  back and forth between the vector and its buffer; a string that keeps
  the whole prefix costs one memcmp() instead of a comparison per
  character
- pdqSort() and the insertion sort scan by pointers, and the merges of
  timSort() and powerSort() gallop adaptively, with no branch on the order
  for numbers in disorder; the merge one by one keeps its threshold of
  galloping in a local and leaves its loop on the end of the run that
  moved, with no flag tested at each step
- the partitions of pdqSort() bound their first scans only where the
  samples of the pivot do not stop them, and the partition by blocks
  fills its offsets 8 at a time and swaps through two pointers, each
  place found once
- stableSort() of items copied as bytes with an order of their own, and
  sort() of them from kRadix items, count the runs as for the numbers and
  sort few keys of a sample of 64 by counting, stable, and the rest by a
  merge sort through a buffer of n / 2 items, as the spinsort of Boost,
  whose merges go from both ends at once with no branch on the order and
  no check of where a half ends, instead of powerSort() and pdqSort()
- the runs of sort() and stableSort() are counted in blocks of 32 items
  with no branch inside, which vectorize on numbers, so that a sorted or
  reversed vector costs one read at the speed of the memory
- the merges of timSort() and powerSort() copy the shorter run, from the
  back if it is the right one, into a buffer of n / 2 items made once,
  rotate two runs in reverse order, and on numbers in their natural order
  merge by a branch that follows the runs, with no gallop
- radixSort() and radixSortBy() first split a vector larger than the
  cache by its top byte, if its keys differ in 3 bytes or more, so that
  the passes over the other bytes stay in the cache; the buffer is not
  zeroed, and the counts of the parts are of 32 bits
- msdRadixSort() permutes the strings in place at the first character,
  and makes its buffer only if a bucket is not all of equal strings
- DaryHeap::push() compares the item with the parent of the new place
  before storing it, and pop() keeps the last item aside, so that no item
  is stored and read back at once; push(), pop() and their sifts are
  inline, so that the compiler expands them in the loop of the caller
- FibonacciHeapPriorityQueue keeps its nodes in an ObjectPool, the node
  of deleteMin() serving the next insert(); deleteMin() joins the children
  of the minimum to the roots in O( 1 ), their parents and marks cleared by
  the consolidation, which walks the roots where they are with the trees
  by degree in an array on the stack instead of two vectors per call; a
  heap of items with a trivial destructor dies a block at a time
- SinglyList keeps its cells in an ObjectPool, the cell of remove()
  serving the next insert(); a list of items with a trivial destructor
  dies a block at a time
- DaryHeap keeps its vector on HugePageAllocator, so that a large heap
  is faulted in a huge page at a time and its sifts miss the TLB less
- ObjectPool begins with a block of one slot and doubles from there, each
  block with a header that links it to the one before instead of a vector
  of the blocks, so that a pool is six words, allocates nothing but its
  blocks, and a structure of few items takes no more than it holds
- List keeps its cells in an ObjectPool, the cell of remove() serving
  the next insert(); a list of items with a trivial destructor dies a
  block at a time; List< T , false > takes its cells one by one from new
  and delete, as the chains of ChainedHashTable do, which are short
- RankSelect with one word of 64 bits of directory per 256 bits (the
  rank of the block in 40 bits and those of its words 1, 2 and 3 in 8
  bits each), so that rank1() is one load of the directory, one of the
  bits and one sideways sum with no branch, in 25% of the bits instead of
  37.5%; select1() and select0() by the inventory of Vigna's simple
  select, the place of the first bit of each group of 2^l spanning about
  8192 bits and of every 2^(l-5)-th in 16 bits, then a count word by
  word, instead of the binary search between samples of every 4096-th
  bit; the groups that span 2^16 bits or more search the directory; a
  third level of counts above 2^40 bits; BitVector keeps its words in
  whole lines of the cache, aligned to a line (to a huge page from 2 MB
  on), with a word past the last bit, so that get() reads a field with no
  test; selectInWord() by PDEP where BMI2 is found; the tests of the third
  level and of the sparse groups, test/succinct_top.cpp
- RankSelect builds the inventories of select only for the values asked
  for, RankSelect::Selects: JacobsonTree keeps those of the 1s, K2Tree
  none
- EliasFano::nextGEQ() compares the low bits of the items of the bucket of
  x with those of x, instead of an access(), and its select, per item
- BalancedParentheses searches on the excess relative to its start, so
  that a search that ends in the leaf of i needs no rank: the leaves of 512
  bits are scanned 8 bits at a time at any place, by tables of the excess,
  of the least prefix and suffix and of the place of each target of a
  byte, then the leaves of its group of 16 by their excess and least
  excess in 16 bits, then the tree of the groups; about 0.1 bits per
  parenthesis instead of 0.5, and the directory with the select of the 1s
  alone
- FMIndex::rank() counts the symbol in the block 8 bytes at a time, by the
  zero bytes of the word xor the symbol, instead of one byte at a time;
  FMIndex::sizeInBits()
- RankSelect builds the inventories of select from its directory, a block
  at a time, instead of a scan of the words, and its directory with no
  test per word; the third level of counts is kept also when only the
  last block reaches it; BitVector from a span of words
- encodeGamma() writes U( L + 1 ) B( x ) as one field when it fits in a
  word, binaryLength() is the width of the processor, BitReader finds the
  1 of the unary code by the trailing zeros of the processor and turns a
  field around by the swap of its bytes
- the groups of select of RankSelect span about 4096 bits instead of
  8192: a sample every 2 words on average instead of 4, and the 1s of a
  vector of 1% of them each in its offset, for 14% of the bits for each
  value instead of 7%
- BalancedParentheses builds its leaves a byte at a time by its tables of
  the excess, instead of a bit at a time
- streamVByteEncode() finds the lengths of 8 integers at once by
  saturating arithmetic on their bytes and one mask, and
  streamVByteDecode() decodes 32 integers with no test while 12 more
  follow, 8 control bytes taken in a word
- huffmanCompress() joins 4 codes before writing them and writes the
  streams straight into the output, and huffmanDecompress() decodes up to
  3 symbols a lookup and counts the rounds that fit instead of testing
  each; byteHistogram() on eight tables
- BalancedParentheses keeps its bits on CompactRankSelect, since no search
  needs a rank: 1.14 bits per parenthesis in all instead of 1.5
- RankSelect answers select straight from the inventory, with no access
  to the bits, when every bit b of a group has its offset (a value of
  density below about 1%); BitVector copies the words of a span once

### Fixed

- the random graphs of the exams of trees and of the collections of
  exercises with an edge probability of 2.0 / n or 1.5 / n, more than 1
  for n = 1, which std::bernoulli_distribution rejects under the
  assertions of libstdc++ (on by default in makepkg, and in GCC 15 at -O0)
- a PointerTable moved from is an empty table: find() gave an access out
  of its places; the move operations, clear() and reserve() are new
- the lookups of a const OpenHashTable from several threads at once no
  longer race on probes(), which is atomic
- ADS_CONTRACTS is 0 by default when NDEBUG is defined, as for assert, so
  that a release build checks no precondition unless it asks to
- the product of BigInteger by the FFT in doubles rounded wrong from about
  2^17 digits of 32 bits; it is now an exact number theoretic transform
  modulo three primes
- the cases shown by the edge cases of the tests, about 60 in all: an
  overflow at the extreme values of the types (DirectTable, hash_division,
  the allocators, the knapsack, refuelingStops, evalPostfix, cross(),
  pseudoAngle(), the Bloom filters), a NaN converted to an integer
  (interpolation and address calculation sort), the containers that took
  one of their own items or themselves while growing (MultiStack, the
  array and preorder trees), the lists of children that repeat a node
  (Jacobson, LOUDS and BP trees), the reading of malformed trees and
  expressions, the empty inputs of searchOrdered(), tsp() and the grid
  paths, the stability of stableSort() between -0 and +0, the exponent of
  the master theorem near an integer and linear() for 1 < a < 2, the
  equality of the contracts with NaN
- knapsack() and zaino() take the objects of volume 0, which the
  precondition admits: the column of capacity 0 follows the same maximum
- selectionSampling() draws its uniform numbers by std::generate_canonical,
  so that a generator of 32 bits no longer gives the first n items always
- the search by interpolation computes its differences in double, since
  v - A[ i ] could overflow the keys and send the probe out of the vector
- the bound of sortToHull() is floor( sqrt( LONG_MAX ) ), so that x^2 fits
  in a long also where long has 32 bits (Windows)
- the fractional knapsack no longer gives a negative fraction when the
  residual capacity rounds below 0

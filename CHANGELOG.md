# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

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
- the benchmark of the hashing, bench/hashing.cpp: the hash functions
  against std::hash, boost::hash, absl::Hash, XXH3, wyhash, rapidhash and
  the hash of unordered_dense; the hash maps against std, Boost (the map,
  the node map and the flat map), absl::flat_hash_map, unordered_dense,
  tsl::robin_map and libcuckoo; the filters against the blocked filter of
  fastfilter_cpp and the xor and binary fuse filters of xor_singleheader;
  the minimal perfect hash functions against PTHash, PHOBIC, BBHash and
  RecSplit; the libraries from their sources given to the makefile by
  HASHING and ABSL (ADS_HASHING and find_package( absl ) with CMake)

- StringSearcher, the engineered exact search of string/StringSearch.h:
  a pattern prepared once, two or four probes of its rarest bytes compared
  with 64, 32 or 16 places of the text at once (AVX-512, AVX2, SSE2,
  memchr() elsewhere), the shifts of its hashed q-grams for the patterns
  of 64 bytes or more, on the two halves of a long text at once, and the
  Two-Way algorithm of Crochemore and Perrin once the comparisons with
  the pattern cost more than 16 times the text scanned, so that no text
  costs more than O( n + m ); find(), findAll(), count() and the
  interface of the searchers of std::search(); test
  engineered_strings.string-search
- suffixArray() and lcpArrayPhi() of string/SuffixSorting.h: SA-IS on 32
  or 64 bits without an array of the types, the LMS-substrings named by
  their lengths and memcmp(), the scans of the induction two entries a
  step with the text, the buckets and the places written prefetched, the
  reduced string sorted by the doubling of Larsson and Sadakane on the
  groups of equal names when few names repeat, a text that never rises
  sorted at once; the LCP array by the permuted LCP of Karkkainen,
  Manzini and Puglisi, the text read 8 bytes at a time; test
  engineered_strings.suffix-sorting
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

### Changed

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

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- the tables of the benchmarks in the README and in `results/` are those
  of a run on an AMD EPYC 9654, with the rows "A/A" of every benchmark for
  the noise, and `results/` lists the libraries compared with the commit
  or the checksum of each
- `bench/summary.py` takes one run, with the noise from its rows "A/A", or
  two, and with `--short` gives a row per benchmark against the standard
  library and against the strongest library
- `FastFlowLock::lock()` retries its test-and-set after the exponential
  backoff of `Backoff`, from pauses of the processor to yields of the
  thread, instead of the busy loop of `ff::spin_lock()`, so that the threads
  waiting for a holder preempted by the system do not spin through their
  quanta

### Fixed

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

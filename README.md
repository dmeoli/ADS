# Algorithms and Data Structures

[![CI](https://github.com/dmeoli/ADS/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/dmeoli/ADS/actions/workflows/ci.yml)
[![Coverage Status](https://coveralls.io/repos/github/dmeoli/ADS/badge.svg?branch=main)](https://coveralls.io/github/dmeoli/ADS?branch=main)
[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://dmeoli.github.io/ADS/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen)](https://github.com/dmeoli/ADS#use)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)](https://github.com/dmeoli/ADS/actions/workflows/ci.yml)
[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v3-blue.svg)](https://www.gnu.org/licenses/lgpl-3.0)

ADS is a header-only C++20 library of the algorithms and the data
structures of the (Italian) textbook

> A. Montresor, *Algoritmi e Strutture Dati*, 2025,
> <https://algoritmicamente.it> (CC BY-NC 4.0)

(a revision of A. A. Bertossi and A. Montresor, *Algoritmi e Strutture di
Dati*, 3rd edition, Città Studi, 2014), in which each exercise of the book
is a test. It also has the structures and the algorithms that a course on
the subject usually adds, and a few engineered ones meant for speed. An
abstract data type is a C++20 concept whose realizations follow the
pseudocode and the names of the book (`empty()`, `head()`, `next()`,
`read()`, `push()`, `enqueue()`, and so on), and hence the code can be read
side by side with the text. Where the book leaves a choice open, or
its pseudocode has a defect (a few figures, too, disagree with the formulae
they illustrate), the header states the choice made and the reason for
it.

## Performance

The table below compares the engineered structures of ADS with those of
the standard library (libstdc++), of Boost 1.83, of GMP 6.3.0 and of
FFTW 3.3.10, as measured by the benchmarks in [`bench/`](bench/) (`make
bench`). These were run twice on an otherwise idle Intel Xeon Gold 5120
at 2.2 GHz, on one core pinned by `taskset`, with g++ 13.3 at `-O3
-march=native`; GMP was configured for that processor, and FFTW was built
with SSE2, AVX, AVX2 and AVX-512. Each cell of a benchmark is the median
of 11 runs, and the clock does not include the building of the input nor
the planning of FFTW. A row gives the time of the library divided by that
of ADS (above 1 when ADS is faster), as the median and the range over the
cells of the benchmark, i.e., its cases (e.g., random, sorted or few
distinct keys for the sorts) and its sizes ($10^5$ and $10^6$ items for
the containers). When a library has more than one candidate, the time of
a cell is that of the fastest of them (e.g., the faster of
`boost::pdqsort` and `boost::spreadsort`). The same code measured twice
differs by 1.1% in the median cell and by 4.3% at the 90th percentile,
over the 1544 cells of the two runs; the largest differences, up to 50%,
are in cells of a few hundred nanoseconds, where they are the resolution
of the clock. Hence, a ratio between 0.95 and 1.05 is usually a tie.

<table>
<tr><th>benchmark</th><th>ADS</th><th>against</th><th>cells</th><th>median</th><th>range</th></tr>
<tr><td rowspan="4">sort double</td><td rowspan="2"><code>ADS::sort</code></td><td>std</td><td align="right">12</td><td align="right">8.83</td><td align="right">1.95 to 32</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">2.20</td><td align="right">0.93 to 14</td></tr>
<tr><td rowspan="2"><code>ADS::stableSort</code></td><td>std</td><td align="right">12</td><td align="right">4.35</td><td align="right">1.67 to 25</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">1.20</td><td align="right">0.88 to 3.01</td></tr>
<tr><td rowspan="4">sort int64</td><td rowspan="2"><code>ADS::sort</code></td><td>std</td><td align="right">12</td><td align="right">14</td><td align="right">1.95 to 39</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">3.52</td><td align="right">0.84 to 17</td></tr>
<tr><td rowspan="2"><code>ADS::stableSort</code></td><td>std</td><td align="right">12</td><td align="right">13</td><td align="right">2.33 to 32</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">1.40</td><td align="right">0.71 to 26</td></tr>
<tr><td rowspan="4">sort record</td><td rowspan="2"><code>ADS::sort</code></td><td>std</td><td align="right">12</td><td align="right">2.33</td><td align="right">1.10 to 6.60</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">1.00</td><td align="right">0.98 to 1.02</td></tr>
<tr><td rowspan="2"><code>ADS::stableSort</code></td><td>std</td><td align="right">12</td><td align="right">5.53</td><td align="right">0.75 to 32</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">0.98</td><td align="right">0.57 to 1.16</td></tr>
<tr><td rowspan="4">sort string</td><td rowspan="2"><code>ADS::sort</code></td><td>std</td><td align="right">12</td><td align="right">3.36</td><td align="right">1.67 to 5.15</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">1.45</td><td align="right">0.90 to 2.20</td></tr>
<tr><td rowspan="2"><code>ADS::stableSort</code></td><td>std</td><td align="right">12</td><td align="right">2.37</td><td align="right">1.68 to 6.10</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">2.76</td><td align="right">1.34 to 5.62</td></tr>
<tr><td rowspan="4">sort uint32</td><td rowspan="2"><code>ADS::sort</code></td><td>std</td><td align="right">12</td><td align="right">15</td><td align="right">3.90 to 36</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">3.93</td><td align="right">0.75 to 16</td></tr>
<tr><td rowspan="2"><code>ADS::stableSort</code></td><td>std</td><td align="right">12</td><td align="right">13</td><td align="right">1.34 to 41</td></tr>
<tr><td>Boost</td><td align="right">12</td><td align="right">1.03</td><td align="right">0.63 to 36</td></tr>
<tr><td rowspan="3">dictionaries</td><td rowspan="2"><code>ADS::FlatHashTable</code></td><td>std</td><td align="right">8</td><td align="right">5.47</td><td align="right">4.32 to 16</td></tr>
<tr><td>Boost</td><td align="right">8</td><td align="right">0.93</td><td align="right">0.75 to 1.16</td></tr>
<tr><td rowspan="1"><code>ADS::RedBlackTree</code></td><td>std</td><td align="right">8</td><td align="right">1.45</td><td align="right">1.00 to 2.38</td></tr>
<tr><td rowspan="3">priority</td><td rowspan="2"><code>ADS::DaryHeap d=4</code></td><td>std</td><td align="right">6</td><td align="right">1.62</td><td align="right">1.38 to 2.17</td></tr>
<tr><td>Boost</td><td align="right">6</td><td align="right">1.04</td><td align="right">0.87 to 1.68</td></tr>
<tr><td rowspan="1"><code>ADS::FibonacciHeapPriorityQueue</code></td><td>Boost</td><td align="right">6</td><td align="right">0.61</td><td align="right">0.54 to 0.86</td></tr>
<tr><td rowspan="3">sequences</td><td rowspan="1"><code>ADS::ArraySequence</code></td><td>std</td><td align="right">6</td><td align="right">1.05</td><td align="right">1.00 to 1.12</td></tr>
<tr><td rowspan="1"><code>ADS::List</code></td><td>std</td><td align="right">6</td><td align="right">1.02</td><td align="right">0.99 to 1.10</td></tr>
<tr><td rowspan="1"><code>ADS::SinglyList</code></td><td>std</td><td align="right">6</td><td align="right">0.98</td><td align="right">0.86 to 1.03</td></tr>
<tr><td rowspan="4">bignum</td><td rowspan="2"><code>ADS::BigInteger</code></td><td>Boost</td><td align="right">12</td><td align="right">4.04</td><td align="right">1.00 to 91</td></tr>
<tr><td>GMP</td><td align="right">12</td><td align="right">0.33</td><td align="right">0.20 to 1.25</td></tr>
<tr><td rowspan="2"><code>ADS::Rational</code></td><td>Boost</td><td align="right">3</td><td align="right">4.74</td><td align="right">1.93 to 6.03</td></tr>
<tr><td>GMP</td><td align="right">3</td><td align="right">0.42</td><td align="right">0.40 to 0.46</td></tr>
<tr><td rowspan="1">fft</td><td rowspan="1"><code>ADS::fft</code></td><td>FFTW</td><td align="right">3</td><td align="right">0.50</td><td align="right">0.34 to 0.78</td></tr>
</table>

`ADS::sort()` and `ADS::stableSort()` are faster than the standard library
in almost all the cells, by 2 to 15 times in the median (the radix sorts
accounting for most of it), and they are often as fast as Boost or faster.
However, `boost::spinsort` is 1.4 to 1.8 times faster on the stable sort
of records, of sorted 32-bit keys and of reversed 64-bit keys at $10^5$,
and `boost::spreadsort` is 1.3 times faster on sorted 32-bit keys.
`FlatHashTable` is 4 to 16 times faster than `std::unordered_map`, and it
is on a par with `boost::unordered_flat_map` on the lookups, but for the
misses among $10^5$ keys (where Boost is 1.15 times faster). Boost is also
1.3 times faster on the insertions and the visits of $10^5$ keys, and 1.16
times slower on the insertions of $10^6$. It is fair to remark that the
libraries built on hand-written kernels stay ahead. GMP is 2 to 5 times
faster than `BigInteger` on products, quotients and decimal conversion
(twice as fast already at 16 limbs, where both use the schoolbook
algorithms), and `mpq_class` is 2.2 to 2.5 times faster than `Rational`;
FFTW is 1.3 to 3 times faster than `ADS::fft`, whose radix-4 transform is
vectorized only by the compiler. Also, the Fibonacci heap of Boost is 1.2
to 1.9 times faster than that of ADS, which follows the pseudocode of the
book. The tables of all the structures, those of the book included, are
in [`results/`](results/README.md) together with the raw files of the two
runs.

## Layout

Headers are grouped by abstract data type, one header per realization,
and `include/ADS/ADT.h` has the concepts of the book. The folders of the
types (`sequence/`, `stack/`, `queue/`, `tree/`, `dictionary/`, `set/`,
`priority/`, `mfset/` and `graph/`) also hold the algorithms on them, e.g.,
the operations on sets or the visits of a graph. The other folders group
the algorithms by technique or by problem:

- `search/` and `sort/`, the searches and the sorts;
- `numeric/`, the products of integers, polynomials and matrices;
- `string/`, the algorithms on strings;
- `optimization/`, the problems of optimization (knapsack, intervals);
- `geometry/`, the convex hull;
- `backtrack/`, the enumerations by backtrack;
- `complexity/`, the problems of NP and their reductions;
- `intractable/`, the techniques for the intractable problems;
- `memory/` and `concurrent/`, the allocators, the arena and the queues
  between threads;
- `external/`, the two-level memory and the algorithms on it;
- `compression/` and `succinct/`, the codes, the compressors and the
  compressed data structures;
- `combinatorial/`, the broadword tricks and the combinatorial generation;
- `algorithm/`, the rest.

A header is included by its path (e.g., `#include "ADS/graph/ListGraph.h"`)
and includes what it needs. It declares its classes and functions, with
their contracts, and the definitions of its templates are in the folder
`impl/` next to it (e.g., `ADS/graph/impl/ListGraph.tpp`), included at the
end of the header, as in Boost; hence, `include/` is the whole library.

## Where each thing comes from

Each header and each test cites the section, the example or the exercise
of the book that it implements, and the tests are listed with their source
in [`test/tests.txt`](test/tests.txt); the solutions of the exercises are
in the tests, one function per exercise. Each one is checked on the
example of the book, on its edge cases and (where an enumeration is
possible) on random instances against the enumeration. Since the license
of the book does not allow its text in a repository under the LGPL, the
code cites it instead of reproducing it.

| Chapter | Section | What | Header | Tests |
|---|---|---|---|---|
| [1](https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf) | 1.2 | minimum, binary search | [`search/Minimum.h`](include/ADS/search/Minimum.h), [`search/BinarySearch.h`](include/ADS/search/BinarySearch.h) | `ch01.sec1.2.*` |
| | 1.4 | multiplication of Ahmes | [`numeric/BinaryInteger.h`](include/ADS/numeric/BinaryInteger.h) | `ch01.sec1.4.ahmes` |
| [2](https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf) | 2.6 | sum and products of binary integers, Karatsuba | [`numeric/BinaryInteger.h`](include/ADS/numeric/BinaryInteger.h) | `ch02.sec2.6.*` |
| | 2.7 | Selection, Insertion, Merge, Counting Sort, lower bound | [`sort/`](include/ADS/sort) | `ch02.sec2.7.*` |
| | 2.9 | solutions of the recurrences (Teoremi 2.2–2.5) | [`algorithm/MasterTheorem.h`](include/ADS/algorithm/MasterTheorem.h) | `ch02.sec2.9.recurrences` |
| [3](https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf) | 3.2 | SEQUENCE, SET, DICTIONARY | [`ADT.h`](include/ADS/ADT.h) | `ch03.*` |
| [4](https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf) | 4.1 | LIST, bidirectional circular with sentinel | [`sequence/List.h`](include/ADS/sequence/List.h) | `ch04.spec.List` |
| | 4.2 | sequence on a vector that doubles | [`sequence/ArraySequence.h`](include/ADS/sequence/ArraySequence.h) | `ch04.spec.ArraySequence`, `ch04.sec4.2.doubling` |
| | 4.2.1 | binary counter, amortized analysis | [`algorithm/BinaryCounter.h`](include/ADS/algorithm/BinaryCounter.h) | `ch04.sec4.2.1.counter` |
| | 4.3 | STACK, QUEUE | [`ADT.h`](include/ADS/ADT.h) | |
| | 4.3.1 | stack simulated with a sequence | [`stack/SequenceStack.h`](include/ADS/stack/SequenceStack.h) | `ch04.sec4.3.1.stack-on-list` |
| | 4.3.2 | stack on a vector | [`stack/ArrayStack.h`](include/ADS/stack/ArrayStack.h) | `ch04.spec.ArrayStack` |
| | 4.3.3 | queue on a circular vector | [`queue/ArrayQueue.h`](include/ADS/queue/ArrayQueue.h) | `ch04.spec.ArrayQueue` |
| | 4.4 | towers of Hanoi, recursive and iterative | [`algorithm/Hanoi.h`](include/ADS/algorithm/Hanoi.h) | `ch04.sec4.4.hanoi` |
| | 4.6 | exercises 4.1–4.11; the singly linked list of 4.6 and the linked queue of 4.10 | [`test/ch04.cpp`](test/ch04.cpp), [`sequence/SinglyList.h`](include/ADS/sequence/SinglyList.h), [`queue/LinkedQueue.h`](include/ADS/queue/LinkedQueue.h) | `ch04.ex4.*` |
| [5](https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf) | 5.1, 5.3 | ordered TREE, parent/first child/sibling | [`tree/Tree.h`](include/ADS/tree/Tree.h) | `ch05.spec.Tree` |
| | 5.2 | previsit, postvisit, invisit, by levels | [`tree/Tree.h`](include/ADS/tree/Tree.h) | `ch05.sec5.2.visits` |
| | 5.4 | vector of parents | [`tree/ParentTree.h`](include/ADS/tree/ParentTree.h) | `ch05.sec5.4.parents` |
| | 5.5 | binary TREE and its visits, ordered tree as binary | [`tree/BinaryTree.h`](include/ADS/tree/BinaryTree.h) | `ch05.spec.BinaryTree`, `ch05.sec5.5.*` |
| | 5.6 | complete d-ary tree in a vector | [`tree/BinaryTree.h`](include/ADS/tree/BinaryTree.h) | `ch05.sec5.6.dary` |
| | 5.8 | exercises 5.1–5.13 | [`test/ch05.cpp`](test/ch05.cpp) | `ch05.ex5.*` |
| [6](https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf) | 6.1 | binary search tree (ABR), DICTIONARY | [`tree/SearchTree.h`](include/ADS/tree/SearchTree.h) | `ch06.sec6.1.*` |
| | 6.2 | Red-Black tree, Teoremi 6.1–6.3 | [`tree/SearchTree.h`](include/ADS/tree/SearchTree.h) | `ch06.spec.RedBlackTree`, `ch06.sec6.2.*` |
| | 6.3 | B+-tree, insertion, search, interval | [`tree/BPlusTree.h`](include/ADS/tree/BPlusTree.h) | `ch06.sec6.3.*` |
| | 6.4 | exercises 6.1–6.10 | [`test/ch06.cpp`](test/ch06.cpp) | `ch06.ex6.*` |
| [7](https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf) | 7 | table with direct access | [`dictionary/DirectTable.h`](include/ADS/dictionary/DirectTable.h) | `ch07.sec7.direct-access` |
| | 7.1 | extraction, XOR, division, multiplication, middle square | [`dictionary/HashFunctions.h`](include/ADS/dictionary/HashFunctions.h) | `ch07.sec7.1.*` |
| | 7.2 | lists of overflow, open addressing, deleted places | [`dictionary/ChainedHashTable.h`](include/ADS/dictionary/ChainedHashTable.h), [`dictionary/OpenHashTable.h`](include/ADS/dictionary/OpenHashTable.h) | `ch07.sec7.2.*` |
| | 7.3 | I(α) and S(α) measured against the formulas | [`dictionary/OpenHashTable.h`](include/ADS/dictionary/OpenHashTable.h) | `ch07.sec7.3.complexity` |
| | 7.4 | restructuring, amortized cost | [`dictionary/TableSize.h`](include/ADS/dictionary/TableSize.h) | `ch07.sec7.4.restructure` |
| | – | exercises 7.1–7.10 | [`test/ch07.cpp`](test/ch07.cpp) | `ch07.ex7.*` |
| [8](https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf) | 8.1 | SET on a boolean vector | [`set/BoolSet.h`](include/ADS/set/BoolSet.h) | `ch08.sec8.1.boolean` |
| | 8.2 | SET on an unsorted list | [`set/ListSet.h`](include/ADS/set/ListSet.h) | `ch08.sec8.2.unsorted-list` |
| | 8.3 | SET on a sorted list, operations by merging | [`set/SortedListSet.h`](include/ADS/set/SortedListSet.h) | `ch08.sec8.3.sorted-list` |
| | 8.4 | SET on hash tables | [`set/DictionarySet.h`](include/ADS/set/DictionarySet.h), [`set/SetOperations.h`](include/ADS/set/SetOperations.h) | `ch08.sec8.4.hash` |
| | 8.5 | SET on Red-Black trees | [`set/DictionarySet.h`](include/ADS/set/DictionarySet.h) | `ch08.sec8.5.tree` |
| | 8.7 | exercises 8.1–8.10, LinkedHashSet, multiset | [`set/LinkedHashSet.h`](include/ADS/set/LinkedHashSet.h), [`set/HashMultiset.h`](include/ADS/set/HashMultiset.h), [`test/ch08.cpp`](test/ch08.cpp) | `ch08.ex8.*` |
| [9](https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf) | 9.1 | definitions, the case of the duke McPollock | [`test/ch09.cpp`](test/ch09.cpp) | `ch09.sec9.1.*` |
| | 9.2, 9.4 | GRAPH, lists and vectors of adjacency | [`graph/Graph.h`](include/ADS/graph/Graph.h), [`graph/ListGraph.h`](include/ADS/graph/ListGraph.h), [`graph/ArrayGraph.h`](include/ADS/graph/ArrayGraph.h) | `ch09.spec.ListGraph`, `ch09.sec9.3.matrices` |
| | 9.3 | adjacency matrix, incidence matrix | [`graph/MatrixGraph.h`](include/ADS/graph/MatrixGraph.h) | `ch09.spec.MatrixGraph`, `ch09.sec9.3.matrices` |
| | 9.5 | bfs, distances, dfs, components, dfs schema, cycles, topological order, strong components | [`graph/Visit.h`](include/ADS/graph/Visit.h) | `ch09.sec9.5.*` |
| | 9.7 | exercises 9.1–9.11 | [`test/ch09.cpp`](test/ch09.cpp) | `ch09.ex9.*` |
| [10](https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf) | 10.1 | PRIORITYQUEUE on a d-ary heap, on lists, on a Red-Black tree | [`priority/`](include/ADS/priority) | `ch10.spec.*PriorityQueue`, `ch10.sec10.1.3.heap` |
| | 10.1.4 | heapBuild(), heapsort() | [`sort/Heapsort.h`](include/ADS/sort/Heapsort.h) | `ch10.sec10.1.4.heapsort` |
| | 10.2 | MFSET on a forest (rank, compression of the paths) and on lists, incremental components | [`mfset/`](include/ADS/mfset) | `ch10.sec10.2.*` |
| | 10.4 | exercises 10.1–10.9 | [`test/ch10.cpp`](test/ch10.cpp) | `ch10.ex10.*` |
| [11](https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf) | 11.1–11.2 | shortest paths, theorem of Bellman, the prototype | [`graph/ShortestPaths.h`](include/ADS/graph/ShortestPaths.h) | `ch11.sec11.1.bellman`, `ch11.sec11.2.prototype` |
| | 11.3–11.5 | Dijkstra, Johnson, Fredman-Tarjan on a Fibonacci heap | [`graph/ShortestPaths.h`](include/ADS/graph/ShortestPaths.h), [`priority/FibonacciHeapPriorityQueue.h`](include/ADS/priority/FibonacciHeapPriorityQueue.h) | `ch11.sec11.3.dijkstra`, `ch11.sec11.5.fibonacci` |
| | 11.6–11.8 | Bellman-Ford-Moore, the stack, Pape-D'Esopo on a dequeue | [`graph/ShortestPaths.h`](include/ADS/graph/ShortestPaths.h), [`queue/Dequeue.h`](include/ADS/queue/Dequeue.h) | `ch11.sec11.6.*`, `ch11.sec11.7.stack`, `ch11.sec11.8.pape` |
| | 11.10 | exercises 11.1–11.10 | [`test/ch11.cpp`](test/ch11.cpp) | `ch11.ex11.*` |
| [12](https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf) | 12.1 | search by interpolation | [`search/InterpolationSearch.h`](include/ADS/search/InterpolationSearch.h) | `ch12.sec12.1.interpolation` |
| | 12.2 | Quicksort, its improvements, on lists | [`sort/QuickSort.h`](include/ADS/sort/QuickSort.h) | `ch12.sec12.2.*` |
| | 12.3 | products of polynomials, Karatsuba | [`numeric/Polynomial.h`](include/ADS/numeric/Polynomial.h) | `ch12.sec12.3.polynomials` |
| | 12.4 | Strassen | [`numeric/Matrix.h`](include/ADS/numeric/Matrix.h) | `ch12.sec12.4.strassen` |
| | 12.6 | exercises 12.1–12.13 | [`test/ch12.cpp`](test/ch12.cpp) | `ch12.ex12.*` |
| [13](https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf) | 13 | binomial coefficients, Fibonacci | [`numeric/Combinatorics.h`](include/ADS/numeric/Combinatorics.h) | `ch13.sec13.binomial`, `ch13.sec13.4.fibonacci` |
| | 13.1 | approximate string matching, edit distance | [`string/ApproximateMatching.h`](include/ADS/string/ApproximateMatching.h) | `ch13.sec13.1.matching` |
| | 13.2 | weighted independent intervals | [`optimization/WeightedIntervals.h`](include/ADS/optimization/WeightedIntervals.h) | `ch13.sec13.2.intervals` |
| | 13.3 | chain of matrices | [`numeric/MatrixChain.h`](include/ADS/numeric/MatrixChain.h) | `ch13.sec13.3.matrix-chain` |
| | 13.5 | Floyd-Warshall, insertion of an edge | [`graph/AllPairsShortestPaths.h`](include/ADS/graph/AllPairsShortestPaths.h) | `ch13.sec13.5.floyd-warshall` |
| | 13.6 | knapsack, by a table and by memoization | [`optimization/Knapsack.h`](include/ADS/optimization/Knapsack.h) | `ch13.sec13.6.knapsack` |
| | 13.7 | LCS, diff, longest common substring, alignments | [`string/`](include/ADS/string) | `ch13.sec13.7.lcs` |
| | 13.8 | exercises 13.1–13.13 | [`test/ch13.cpp`](test/ch13.cpp), [`algorithm/Subsequences.h`](include/ADS/algorithm/Subsequences.h) | `ch13.ex13.*` |
| [14](https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf) | 14 | change with coins | [`optimization/CoinChange.h`](include/ADS/optimization/CoinChange.h) | `ch14.sec14.change` |
| | 14.1 | real knapsack | [`optimization/FractionalKnapsack.h`](include/ADS/optimization/FractionalKnapsack.h) | `ch14.sec14.1.real-knapsack` |
| | 14.2 | independent set of intervals, coloring, towers | [`optimization/Intervals.h`](include/ADS/optimization/Intervals.h) | `ch14.sec14.2.intervals` |
| | 14.3 | Kruskal and Prim, Teorema 14.1 | [`graph/MinimumSpanningTree.h`](include/ADS/graph/MinimumSpanningTree.h) | `ch14.sec14.3.*` |
| | 14.4 | code of Huffman | [`string/Huffman.h`](include/ADS/string/Huffman.h) | `ch14.sec14.4.huffman` |
| | 14.5 | exercises 14.1–14.10, Shortest Job First | [`test/ch14.cpp`](test/ch14.cpp), [`optimization/ShortestJobFirst.h`](include/ADS/optimization/ShortestJobFirst.h) | `ch14.ex14.*` |
| [15](https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf) | 15 | local search of the minimum spanning tree | [`test/ch15.cpp`](test/ch15.cpp) | `ch15.sec15.local-search` |
| | 15.1 | Shell Sort, four sequences of gaps | [`sort/ShellSort.h`](include/ADS/sort/ShellSort.h) | `ch15.sec15.1.*` |
| | 15.2 | maximum flow (Ford-Fulkerson, Edmonds-Karp, Dinic), minimum cut, circulations | [`graph/MaxFlow.h`](include/ADS/graph/MaxFlow.h) | `ch15.sec15.2.max-flow` |
| | 15.4 | exercises 15.1–15.10, Eulerian cycle | [`test/ch15.cpp`](test/ch15.cpp), [`graph/EulerianCycle.h`](include/ADS/graph/EulerianCycle.h) | `ch15.ex15.*` |
| [16](https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf) | 16.1 | convex hull, O(n³) and Graham | [`geometry/ConvexHull.h`](include/ADS/geometry/ConvexHull.h) | `ch16.sec16.1.*` |
| | 16.2 | enumeration: subsets, permutations, derangements, subset sum | [`backtrack/Enumeration.h`](include/ADS/backtrack/Enumeration.h) | `ch16.sec16.2.subsets` |
| | 16.3 | Sudoku | [`backtrack/Sudoku.h`](include/ADS/backtrack/Sudoku.h) | `ch16.sec16.3.sudoku` |
| | 16.4 | exercises 16.1–16.8, n queens, tour of the knight, anagrams | [`backtrack/`](include/ADS/backtrack), [`test/ch16.cpp`](test/ch16.cpp) | `ch16.ex16.*` |
| [17](https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf) | 17 | Monte Carlo: null polynomial, Freivalds, half-satisfiability | [`algorithm/MonteCarlo.h`](include/ADS/algorithm/MonteCarlo.h) | `ch17.sec17.monte-carlo`, `ch17.ex17.4`, `ch17.ex17.5` |
| | 17.1 | selection of Hoare, median of medians | [`search/Selection.h`](include/ADS/search/Selection.h) | `ch17.sec17.1.selection` |
| | 17.2 | Euclid, powers, Miller-Rabin, primality on 64 bits | [`numeric/NumberTheory.h`](include/ADS/numeric/NumberTheory.h) | `ch17.sec17.2.primality`, `ch17.ex17.1`–`ex17.3` |
| | 17.3 | RSA | [`numeric/RSA.h`](include/ADS/numeric/RSA.h) | `ch17.sec17.3.rsa` |
| [18](https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf) | 18 | the problems in NP, certificates and enumerations | [`complexity/Problems.h`](include/ADS/complexity/Problems.h), [`complexity/Domino.h`](include/ADS/complexity/Domino.h) | `ch18.sec18.examples` |
| | 18.2–18.3 | nondeterminism simulated by enumeration | [`complexity/Nondeterminism.h`](include/ADS/complexity/Nondeterminism.h) | `ch18.sec18.2.nondeterminism` |
| | 18.5–18.7 | reductions, linear tiling in P, Teoremi 18.2–18.9 | [`complexity/Reductions.h`](include/ADS/complexity/Reductions.h) | `ch18.sec18.5.*`, `ch18.sec18.7.*` |
| | 18.9 | exercises 18.1–18.16 | [`test/ch18.cpp`](test/ch18.cpp) | `ch18.ex18.*` |
| [19](https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf) | 19.1 | pseudopolynomial algorithms, scheduling | [`intractable/Pseudopolynomial.h`](include/ADS/intractable/Pseudopolynomial.h) | `ch19.sec19.1.subset-sum` |
| | 19.2 | approximation: ff, ffd, triangular salesman, apSubsetSum | [`intractable/Approximation.h`](include/ADS/intractable/Approximation.h) | `ch19.sec19.2.*` |
| | 19.3 | branch-&-bound: bbTsp, 1-tree, Little, clique, knapsack | [`intractable/BranchAndBound.h`](include/ADS/intractable/BranchAndBound.h) | `ch19.sec19.3.bb-tsp` |
| | 19.4 | heuristics: greedyTsp, min-max-greedy, local search | [`intractable/Heuristics.h`](include/ADS/intractable/Heuristics.h) | `ch19.sec19.4.*` |
| | 19.5 | the fifteen puzzle | [`intractable/FifteenPuzzle.h`](include/ADS/intractable/FifteenPuzzle.h) | `ch19.sec19.5.fifteen` |
| | 19.6 | exercises 19.1–19.19 | [`test/ch19.cpp`](test/ch19.cpp) | `ch19.ex19.*` |
| Conclusione | Tab. 19.1 | easy twins of NP-hard problems, interval graphs | [`complexity/EasyCases.h`](include/ADS/complexity/EasyCases.h) | `conclusion.*` |

## Beyond the book

Several structures and algorithms that the book does not describe, but
that a course on the subject usually covers (e.g., the realizations with
cursors, the parenthesized lists of trees, the general schemes of the
techniques), are written on the same classes and tested in
[`test/beyond.cpp`](test/beyond.cpp).

| What | Header | Test |
|---|---|---|
| binary tree in a positional vector | [`tree/ArrayBinaryTree.h`](include/ADS/tree/ArrayBinaryTree.h) | `beyond.array-binary-tree` |
| binary and ordered trees with cursors | [`tree/CursorBinaryTree.h`](include/ADS/tree/CursorBinaryTree.h), [`tree/CursorTree.h`](include/ADS/tree/CursorTree.h) | `beyond.cursor-*` |
| ordered tree in a vector in preorder | [`tree/PreorderArrayTree.h`](include/ADS/tree/PreorderArrayTree.h) | `beyond.preorder-array-tree` |
| heap on a binary tree with pointers | [`priority/LinkedHeapPriorityQueue.h`](include/ADS/priority/LinkedHeapPriorityQueue.h) | `beyond.linked-heap` |
| graph of node objects with pointers | [`graph/PointerGraph.h`](include/ADS/graph/PointerGraph.h) | `beyond.pointer-graph` |
| k stacks in one vector | [`stack/MultiStack.h`](include/ADS/stack/MultiStack.h) | `beyond.multistack` |
| search, order, bubble and natural merge sort on sequences | [`sequence/SequenceAlgorithms.h`](include/ADS/sequence/SequenceAlgorithms.h) | `beyond.sequence-*` |
| palindromes and sorting with a stack | [`stack/StackAlgorithms.h`](include/ADS/stack/StackAlgorithms.h) | `beyond.stack-algorithms` |
| naive and Knuth-Morris-Pratt string matching | [`string/StringMatching.h`](include/ADS/string/StringMatching.h) | `beyond.string-matching` |
| exercises on lists, strings, trees and arrays | [`test/beyond.cpp`](test/beyond.cpp) | `beyond.*-exercises`, `beyond.even-odd-strings` |
| list with cursors in a shared space with a free list | [`sequence/CursorList.h`](include/ADS/sequence/CursorList.h) | `beyond.cursor-list`, `ch04.spec.CursorList` |
| range of C++ over any sequence | [`sequence/SequenceRange.h`](include/ADS/sequence/SequenceRange.h) | `beyond.sequence-range` |
| search with the place of insertion and sublists of a sorted sequence | [`sequence/SequenceAlgorithms.h`](include/ADS/sequence/SequenceAlgorithms.h) | `beyond.sorted-list` |
| postfix expressions, infix to postfix (shunting-yard) | [`stack/Expressions.h`](include/ADS/stack/Expressions.h) | `beyond.postfix` |
| queue with no duplicates, positive items of a queue | [`queue/UniqueQueue.h`](include/ADS/queue/UniqueQueue.h), [`queue/QueueAlgorithms.h`](include/ADS/queue/QueueAlgorithms.h) | `beyond.queue-algorithms` |
| dictionary on a sorted vector, pseudo-random inspection | [`dictionary/SortedArrayDictionary.h`](include/ADS/dictionary/SortedArrayDictionary.h), [`dictionary/OpenHashTable.h`](include/ADS/dictionary/OpenHashTable.h) | `beyond.sorted-array-dictionary`, `beyond.hashing` |
| decision tree of the binary search | [`search/BinarySearch.h`](include/ADS/search/BinarySearch.h) | `beyond.decision-tree` |
| trees as parenthesized lists, tree of an expression, tree with lists of children | [`tree/BinaryTree.h`](include/ADS/tree/BinaryTree.h), [`tree/ExpressionTree.h`](include/ADS/tree/ExpressionTree.h), [`tree/ChildListTree.h`](include/ADS/tree/ChildListTree.h) | `beyond.list-forms`, `beyond.expression-tree`, `beyond.child-list-tree` |
| union by size in the MFSET | [`mfset/MFSet.h`](include/ADS/mfset/MFSet.h) | `contract_mfset.mfset` |
| general visit of a graph, unilateral connection | [`graph/Visit.h`](include/ADS/graph/Visit.h) | `beyond.generic-visit`, `beyond.unilateral-incidence` |
| general schemes of the techniques (enumeration, backtracking, greedy, divide et impera) | [`algorithm/Paradigms.h`](include/ADS/algorithm/Paradigms.h) | `beyond.paradigms` |
| Knuth's strong failure function, Hibbard's gaps, sentinels, minimum and maximum together | [`string/StringMatching.h`](include/ADS/string/StringMatching.h), [`sort/`](include/ADS/sort), [`search/Minimum.h`](include/ADS/search/Minimum.h) | `beyond.kmp-strong`, `beyond.slide-sorts` |
| greedy and backtracking knapsack, eight puzzle by breadth and depth | [`intractable/BranchAndBound.h`](include/ADS/intractable/BranchAndBound.h), [`intractable/FifteenPuzzle.h`](include/ADS/intractable/FifteenPuzzle.h) | `beyond.knapsack-heuristics`, `beyond.eight-puzzle` |

## From The Art of Computer Programming

The structures and the algorithms of Knuth's *The Art of Computer
Programming* that the book leaves out are in ADS too, from volumes 1, 2, 3,
4A and 4B, with a few relatives from the later literature; they are tested
in [`test/taocp.cpp`](test/taocp.cpp) against the standard library, against
a slower method that is obviously correct (the definition of the
transform, the enumeration of the subsets, the backtrack of chapter 16)
and against the counts and the bounds of their analysis.

| What | Header | Test |
|---|---|---|
| Fibonacci search and uniform binary search (6.2.1) | [`search/KnuthSearches.h`](include/ADS/search/KnuthSearches.h) | `taocp.knuth-searches` |
| AVL tree (6.2.3) and B-tree with removal (6.2.4) | [`tree/AVLTree.h`](include/ADS/tree/AVLTree.h), [`tree/BTree.h`](include/ADS/tree/BTree.h) | `taocp.balanced-trees` |
| skip list, treap, splay tree | [`dictionary/SkipList.h`](include/ADS/dictionary/SkipList.h), [`tree/Treap.h`](include/ADS/tree/Treap.h), [`tree/SplayTree.h`](include/ADS/tree/SplayTree.h) | `taocp.balanced-trees` |
| trie and compact trie (Patricia) (6.3) | [`dictionary/Trie.h`](include/ADS/dictionary/Trie.h) | `taocp.tries` |
| coalesced hashing with a cellar and Brent's double hashing (6.4) | [`dictionary/KnuthHashing.h`](include/ADS/dictionary/KnuthHashing.h) | `taocp.knuth-hashing` |
| Bloom filter | [`set/BloomFilter.h`](include/ADS/set/BloomFilter.h) | `taocp.bloom-filter` |
| binary and list insertion, address calculation, radix exchange, tournament sort, Ford-Johnson merge insertion (5.2, 5.3.1) | [`sort/KnuthSorts.h`](include/ADS/sort/KnuthSorts.h) | `taocp.knuth-sorts` |
| replacement selection and polyphase merge (5.4) | [`sort/ExternalSort.h`](include/ADS/sort/ExternalSort.h) | `taocp.external-sort` |
| threaded binary tree (2.3.1) | [`tree/ThreadedTree.h`](include/ADS/tree/ThreadedTree.h) | `taocp.threaded-tree` |
| boundary-tag allocator with first, best and next fit, buddy system (2.5) | [`memory/Allocators.h`](include/ADS/memory/Allocators.h) | `taocp.allocators` |
| heap of cells with the marking of Deutsch, Schorr and Waite (2.3.5) | [`memory/CellHeap.h`](include/ADS/memory/CellHeap.h) | `taocp.cell-heap` |
| sparse matrix on orthogonal lists, pivot step (2.2.6) | [`numeric/SparseMatrix.h`](include/ADS/numeric/SparseMatrix.h) | `taocp.sparse-matrix` |
| broadword tricks: sideways sum, ruler, Gray code, Gosper, Morton code (7.1.3) | [`combinatorial/Bitwise.h`](include/ADS/combinatorial/Bitwise.h) | `taocp.bitwise` |
| tuples, permutations, combinations, partitions, nested parentheses, with the Gray orders (7.2.1) | [`combinatorial/Generation.h`](include/ADS/combinatorial/Generation.h) | `taocp.generation` |
| linear congruential generators, chi-square, Kolmogorov-Smirnov, serial and spectral tests, shuffling and sampling (3.2-3.4) | [`numeric/RandomNumbers.h`](include/ADS/numeric/RandomNumbers.h) | `taocp.random-numbers` |
| fast Fourier transform (4.3.3, 4.6.4) | [`numeric/FFT.h`](include/ADS/numeric/FFT.h) | `taocp.fft` |
| integers of any length: Karatsuba and the NTT, Algorithm D and the recursive division, the conversions by halves, the gcd of Lehmer (4.3, 4.4, 4.5.2) | [`numeric/BigInteger.h`](include/ADS/numeric/BigInteger.h) | `taocp.big-integer*` |
| exact rationals (4.5.1) | [`numeric/Rational.h`](include/ADS/numeric/Rational.h) | `taocp.rational` |
| exact cover by dancing links, Algorithm X (7.2.2.1) | [`backtrack/DancingLinks.h`](include/ADS/backtrack/DancingLinks.h) | `taocp.dancing-links` |
| SAT by DPLL and by conflict-driven clause learning (7.2.2.2) | [`backtrack/SatSolvers.h`](include/ADS/backtrack/SatSolvers.h) | `taocp.sat-solvers` |

## From Ferragina's algorithm engineering

The algorithms of P. Ferragina's *Pearls of Algorithm Engineering*, from
the maximum sub-array sum to the compressed data structures, are in ADS as
well. Each header compares the algorithms of its chapter with the variants
of the later literature (e.g., SA-IS beside the skew algorithm, or the
sampled select of the practical libraries beside Clark's scheme). The tests
in `test/pearls_*.cpp` check them against brute force, against the standard
library and against the examples of the notes, with their numbers. A class
simulates the two-level memory of chapters 3 to 6 and counts the reads and
the writes of blocks; hence, the tests can also check the bounds on the
I/Os that those chapters prove.

| What | Headers | Tests |
|---|---|---|
| maximum sub-array sum, with lengths in a range and maximum density (2) | [`algorithm/MaximumSegments.h`](include/ADS/algorithm/MaximumSegments.h) | `pearls_io.ch2.*` |
| random sampling, on disk and on streams, Vitter's skips (3) | [`numeric/RandomSampling.h`](include/ADS/numeric/RandomSampling.h) | `pearls_io.ch3.*` |
| list ranking by pointer jumping and independent sets, in memory and on disk (4) | [`external/ListRanking.h`](include/ADS/external/ListRanking.h) | `pearls_io.ch4.*` |
| two-level memory, binary and multiway mergesort, distribution sort, loser tree, I/O lower bounds (5) | [`external/`](include/ADS/external), [`priority/LoserTree.h`](include/ADS/priority/LoserTree.h) | `pearls_io.ch5.*` |
| sorted set intersection: merge, binary search, mutual partition, doubling, two-level (6) | [`set/SortedIntersection.h`](include/ADS/set/SortedIntersection.h) | `pearls_io.ch6.*` |
| universal hashing, d-left, FKS, minimal ordered perfect hashing, cuckoo hashing, Bloom variants (8) | [`dictionary/`](include/ADS/dictionary), [`set/BloomVariants.h`](include/ADS/set/BloomVariants.h) | `pearls_hashing.*` |
| prefix search, front coding, Patricia trie, two-level dictionary (9) | [`string/PrefixSearch.h`](include/ADS/string/PrefixSearch.h), [`string/FrontCoding.h`](include/ADS/string/FrontCoding.h), [`dictionary/PatriciaTrie.h`](include/ADS/dictionary/PatriciaTrie.h) | `pearls_hashing.*` |
| suffix arrays (six constructions), LCP, range minimum queries, suffix trees and their applications (10) | [`string/SuffixArray.h`](include/ADS/string/SuffixArray.h), [`string/SuffixTree.h`](include/ADS/string/SuffixTree.h), [`search/RangeMinimum.h`](include/ADS/search/RangeMinimum.h) | `pearls_suffix.*` |
| integer codes: gamma, delta, Rice, Golomb, PForDelta, variable-byte, (s,c)-dense, interpolative, Elias-Fano (11) | [`compression/IntegerCodes.h`](include/ADS/compression/IntegerCodes.h), [`succinct/EliasFano.h`](include/ADS/succinct/EliasFano.h) | `pearls_succinct.*` |
| canonical Huffman, arithmetic coding, PPM (12), LZ77, LZSS, LZ78, LZW (13), Burrows-Wheeler, bzip, FM-index (14) | [`compression/`](include/ADS/compression) | `pearls_compression.*` |
| bit vectors with rank and select, succinct trees (Jacobson, LOUDS, balanced parentheses) and graphs (k2-tree, Boldi-Vigna) (15) | [`succinct/`](include/ADS/succinct) | `pearls_succinct.*` |

## The exercises and the exams of the course

Beyond the exercises of the book, the tests cover the material of the
course on which the book is based: the chapter collections, the thematic
collections, more than 80 exams from 2011 to 2026 with their official
solutions, the exercise sessions and the laboratory projects. Each problem
is implemented after its official solution and checked on the examples of
its statement. It is also checked against an independent oracle (brute
force, a simpler algorithm, the standard library) on random instances,
which usually exposes the slips of the official solutions; there, the test
implements the correct version and shows the counterexample. The tests are
in `test/exam_*.cpp`, one program per theme (e.g., backtracking, dynamic
programming, flows) or per source, and the name of a test gives the problem
and the date of its exam.

## Engineered structures

Beside the classes of the book, which follow its pseudocode, a few
structures are written for speed, for the programs that need it, and
tested in [`test/engineered.cpp`](test/engineered.cpp), the heaps in
[`test/heaps.cpp`](test/heaps.cpp), the vectors in
[`test/sequences.cpp`](test/sequences.cpp) and, the hashing, in
[`test/hashing.cpp`](test/hashing.cpp), the concurrent ones also on
real threads under ThreadSanitizer. `FlatHashTable` is the hash
table that [`bench/dictionaries.cpp`](bench/dictionaries.cpp) compares with
`boost::unordered_flat_map`, while `OpenHashTable` stays the table of
section 7.2 of the book; [`bench/hashing.cpp`](bench/hashing.cpp) compares
the hashing with Abseil, unordered_dense, robin-map, libcuckoo, XXH3,
wyhash, rapidhash, fastfilter_cpp, xor_singleheader, PTHash, BBHash and
RecSplit; [`bench/priority.cpp`](bench/priority.cpp) compares the heaps
with Boost.Heap, tlx and radix-heap, also in Dijkstra's algorithm, and
[`bench/memory.cpp`](bench/memory.cpp) the arena and the pool with
`std::pmr`, Boost.Pool, mimalloc and jemalloc.

| What | Header | Test |
|---|---|---|
| two-level bucket queue for integer keys (Dial; Burns et al.) | [`priority/BucketQueue.h`](include/ADS/priority/BucketQueue.h) | `engineered.bucket-queue` |
| d-ary heap of plain items, no handles | [`priority/DaryHeap.h`](include/ADS/priority/DaryHeap.h) | `engineered.dary-heap` |
| radix heap, monotone integer keys in buckets by digits of B bits (Ahuja et al.) | [`priority/RadixHeap.h`](include/ADS/priority/RadixHeap.h) | `heaps.radix-heap` |
| monotone bucket queue on a circular window, overflow to a radix heap (Dial) | [`priority/DialQueue.h`](include/ADS/priority/DialQueue.h) | `heaps.dial-queue` |
| sequence heap, sorted sequences merged in batches by trees of losers (Sanders) | [`priority/SequenceHeap.h`](include/ADS/priority/SequenceHeap.h) | `heaps.sequence-heap` |
| pairing heap with decrease-key, two-pass or auxiliary two-pass (Fredman et al.; Stasko and Vitter) | [`priority/PairingHeap.h`](include/ADS/priority/PairingHeap.h) | `heaps.pairing-heap` |
| vector with room for N items inside itself | [`sequence/SmallVector.h`](include/ADS/sequence/SmallVector.h) | `sequences.small-vector` |
| hash table with control bytes in groups of 15 matched by SSE2, NEON or SWAR, the hash mixed on 128 bits, at most 7/8 full (Swiss tables; Boost's `unordered_flat_map`) | [`dictionary/FlatHashTable.h`](include/ADS/dictionary/FlatHashTable.h) | `engineered.flat-hash-table` |
| hash table of pointers, linear probing, Fibonacci hashing | [`dictionary/PointerTable.h`](include/ADS/dictionary/PointerTable.h) | `engineered.pointer-table` |
| arena of objects in blocks, stable addresses | [`memory/Arena.h`](include/ADS/memory/Arena.h) | `engineered.arena` |
| pool of objects in doubling blocks, the slot freed last made first | [`memory/ObjectPool.h`](include/ADS/memory/ObjectPool.h) | `engineered.pool` |
| allocator of the large blocks on transparent huge pages | [`memory/HugePageAllocator.h`](include/ADS/memory/HugePageAllocator.h) | `engineered.huge-page-allocator` |
| SplitMix64, Zobrist and simple tabulation hashing | [`dictionary/RandomHashing.h`](include/ADS/dictionary/RandomHashing.h) | `engineered.random-hashing` |
| hashes of words and strings by folded products of 128 bits, stripes in AVX2 or AVX-512 past 256 bytes (wyhash, XXH3) | [`dictionary/FastHash.h`](include/ADS/dictionary/FastHash.h) | `hashing.fast-hash` |
| filter of Bloom in blocks of 256 bits, one bit in each of 8 words (Putze et al.; Parquet) | [`set/BlockedBloomFilter.h`](include/ADS/set/BlockedBloomFilter.h) | `hashing.blocked-bloom` |
| binary fuse filter, fingerprints of 8 or 16 bits (Graf and Lemire) | [`set/BinaryFuseFilter.h`](include/ADS/set/BinaryFuseFilter.h) | `hashing.binary-fuse` |
| minimal perfect hash function by pilots, about 3 bits per key (PTHash) | [`dictionary/MinimalPerfectHash.h`](include/ADS/dictionary/MinimalPerfectHash.h) | `hashing.minimal-perfect-hash` |
| cuckoo table with buckets of 4 places, tags of 8 bits, search in breadth (Li et al.; libcuckoo) | [`dictionary/BucketCuckooTable.h`](include/ADS/dictionary/BucketCuckooTable.h) | `hashing.bucket-cuckoo` |
| lock-free queue from one producer to one consumer | [`concurrent/SPSCQueue.h`](include/ADS/concurrent/SPSCQueue.h) | `engineered.spsc-queue` |
| lock-free inbox of many producers | [`concurrent/MPSCInbox.h`](include/ADS/concurrent/MPSCInbox.h) | `engineered.mpsc-inbox` |
| wavelet matrix: access, rank, select, range quantiles (Claude, Navarro and Ordonez) | [`succinct/WaveletMatrix.h`](include/ADS/succinct/WaveletMatrix.h) | `engineered_succinct.wavelet-matrix` |
| code of Huffman of bytes, lengths of at most 11 bits by package-merge, four streams, up to three symbols a lookup | [`compression/HuffmanCoder.h`](include/ADS/compression/HuffmanCoder.h) | `engineered_succinct.huffman-coder` |
| rANS of order 0 with four interleaved states (Duda; Giesen) | [`compression/Rans.h`](include/ADS/compression/Rans.h) | `engineered_succinct.rans-coder` |
| Stream VByte, the lengths apart, one shuffle per 4 integers (Lemire, Kurz and Rupp) | [`compression/StreamVByte.h`](include/ADS/compression/StreamVByte.h) | `engineered_succinct.stream-vbyte` |
| rank and select in about 3.9% of the bits, one 128-bit entry per 4096 bits (Kurpicz; Zhou, Andersen and Kaminsky) | [`succinct/CompactRankSelect.h`](include/ADS/succinct/CompactRankSelect.h) | `engineered_succinct.compact-rank-select` |
| compressed vector of bits in m H0 + 0.175 m bits with rank and select (Raman, Raman and Rao; Claude and Navarro) | [`succinct/RRRBitVector.h`](include/ADS/succinct/RRRBitVector.h) | `engineered_succinct.rrr-bit-vector` |
| tANS of order 0 with 2^11 states, two halves of four states each (Duda) | [`compression/Tans.h`](include/ADS/compression/Tans.h) | `engineered_succinct.tans-coder` |
| directly addressable codes with the optimal widths (Brisaboa, Ladra and Navarro) | [`compression/DirectAccessCodes.h`](include/ADS/compression/DirectAccessCodes.h) | `engineered_succinct.direct-access-codes` |
| parse of LZ77 with no window in O(n) time (Karkkainen, Kempa and Puglisi) | [`compression/LZ77Linear.h`](include/ADS/compression/LZ77Linear.h) | `engineered_succinct.lz77-linear` |

## Sorting

Beside the sorts of the book, ADS has the classical ones that the book
leaves out and the engineered ones of the current libraries. They are
tested in [`test/sorting.cpp`](test/sorting.cpp) on all the permutations of
up to 7 items and on the patterns that usually defeat a sort (sorted,
reversed, sawtooth, organ pipe, few distinct keys), and measured by
[`bench/sort.cpp`](bench/sort.cpp) against `std::sort`, `std::stable_sort`
and Boost.Sort (`make bench`, or CMake with `-DADS_BENCHMARKS=ON`).
`ADS::sort()` and `ADS::stableSort()`
([`sort/Sort.h`](include/ADS/sort/Sort.h)) choose among them by the type of
the items and by the order, i.e., the radix sort for numbers, the MSD
radix sort for strings, and pdqsort and Powersort for the rest, with
thresholds chosen on the measures of [`results/`](results/README.md).

| What | Header |
|---|---|
| bubble, cocktail shaker, odd-even transposition, comb and gnome sort | [`sort/ExchangeSorts.h`](include/ADS/sort/ExchangeSorts.h) |
| cycle sort (fewest writes) and pancake sort (prefix reversals) | [`sort/CycleSort.h`](include/ADS/sort/CycleSort.h) |
| tree sort, patience sort, smoothsort, strand sort | [`sort/TreeSort.h`](include/ADS/sort/TreeSort.h), [`sort/PatienceSort.h`](include/ADS/sort/PatienceSort.h), [`sort/Smoothsort.h`](include/ADS/sort/Smoothsort.h), [`sort/StrandSort.h`](include/ADS/sort/StrandSort.h) |
| sorting networks of Batcher (merge exchange, bitonic) | [`sort/SortingNetworks.h`](include/ADS/sort/SortingNetworks.h) |
| stooge sort, slow sort, bogo sort | [`sort/DidacticSorts.h`](include/ADS/sort/DidacticSorts.h) |
| introsort, pdqsort, dual-pivot and three-way quicksort | [`sort/EngineeredQuickSorts.h`](include/ADS/sort/EngineeredQuickSorts.h) |
| Timsort and Powersort | [`sort/RunSorts.h`](include/ADS/sort/RunSorts.h) |
| LSD radix sort of numbers and by a key, American flag sort, MSD radix sort of strings, bucket sort | [`sort/RadixSort.h`](include/ADS/sort/RadixSort.h) |
| multikey quicksort of strings | [`sort/MultikeyQuickSort.h`](include/ADS/sort/MultikeyQuickSort.h) |

## Design by contract

ADS follows the design by contract of B. Meyer: each operation checks its
specification as a contract ([`Contract.h`](include/ADS/Contract.h)) of
one of four kinds, i.e.,
`ADS_PRE` for the preconditions of the book, `ADS_POST` for the
postconditions, `ADS_INVARIANT` for the invariant of a structure, and
`ADS_AUDIT` for a condition that costs more than the operation (e.g., that
a vector is sorted before a binary search, or that a write left the other
items as they were). The macro `ADS_CONTRACTS` chooses which of them are
evaluated (0 none, 1 the preconditions, which is the default, 2 also the
postconditions, 3 also the invariants and the audits); with `NDEBUG`
defined, as in a release build, the default is 0, as for `assert`. A violated contract
throws `ADS::contract_violation`, a `std::logic_error` naming the
condition, the function and the place. Tests are built at the level 3;
furthermore, the axioms of each abstract data type are checked on all its
realizations (`contract_*.*-axioms`), e.g., `top( push( S , v ) ) = v` and
`pop( push( S , v ) ) = S` for a stack.

## The standard interface

Beside the operations of the book, the containers have the interface of
the standard library: `begin()` and `end()`, the usual member types, and
iterators of the strongest category that the structure allows. Both ends
have the same type, as the algorithms of C++17 and of Boost require.
Hence, a range-for, the algorithms of `<algorithm>` and of `std::ranges`,
and those of Boost.Range and Boost.Sort run on the containers of ADS as on
those of the standard library. Where the items can change in place (the
sequences, the deque, the arena) the iterators are mutable. A heap, an
ordered set or a dictionary keeps an invariant on its items, and its
iterators are constant, since the operations of the book are then the
only safe way to change it. Several containers share the iterators of
[`Iterators.h`](include/ADS/Iterators.h), and the tests in
[`test/interop.cpp`](test/interop.cpp) check the category of each iterator
at compile time and run the algorithms of std (and of Boost, when it is
installed) on each container against its operations.

| Category | Containers |
|---|---|
| contiguous | `ArraySequence` (mutable), `ArrayStack`, `DaryHeap`, `SortedArrayDictionary`, the adjacent nodes of `ArrayGraph` |
| random access | `ArrayQueue` (on the circular vector), `HeapPriorityQueue`, `Arena` (mutable), the values of `PreorderArrayTree`, the nodes of `ArrayGraph` |
| bidirectional | `List` and `Dequeue` (mutable), `UniqueQueue`, `ListPriorityQueue`, `TreePriorityQueue`, the sets on a list, `LinkedHashSet`, `BoolSet`, the open-addressing tables, `DirectTable`, `PointerTable`, the search trees, `AVLTree`, `Treap`, `SplayTree`, `ThreadedTree`, `BPlusTree`, `LinkedHeapPriorityQueue`, the nodes and the adjacent nodes of `ListGraph` and `MatrixGraph`, the nodes of the cursor trees |
| forward | `SinglyList` and `CursorList` (mutable), `LinkedQueue`, `SequenceStack`, the stacks of `MultiStack`, `BucketQueue`, `ChainedHashTable` and the sets on it, `HashMultiset`, `SkipList`, `Trie`, `RadixTree`, `BTree`, `FibonacciHeapPriorityQueue`, the children and the preorder of `Tree`, the preorder and the symmetric order of `BinaryTree`, the children of `CursorTree`, the rows and the columns of `SparseMatrix` |

A dictionary gives the pairs `( key , value )` and a priority queue the
pairs `( priority , item )`, in the order of the structure (i.e., the order
of the keys in a search tree or a trie, of the places in a hash table, of
the heap in a heap). In some containers the item is built on the fly (a
pair of references, a key rebuilt along a trie, the number of a node). The
iterator then models the concept of C++20, but it declares the category
of an input iterator to the older algorithms, as the views of the standard
library do; with these iterators one should use `std::ranges::prev` and
the algorithms of `std::ranges`, not `std::prev` and those of
`<algorithm>`.

## Further reading

Besides the book, the headers cite the textbooks and the papers from which
the structures and the algorithms beyond the book come; textbooks that
cover much of the same ground are

- T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, *Introduction to
  Algorithms*, 3rd ed., MIT Press, 2009;
- C. Demetrescu, I. Finocchi, G. F. Italiano, *Algoritmi e strutture
  dati*, 2nd ed., McGraw-Hill, 2008;
- R. Sedgewick, *Algorithms in C++, Parts 1-4*, 3rd ed., Addison-Wesley,
  1998;
- A. V. Aho, J. E. Hopcroft, J. D. Ullman, *Data Structures and
  Algorithms*, Addison-Wesley, 1983;
- D. E. Knuth, *The Art of Computer Programming*, vol. 1 (3rd ed., 1997),
  vol. 2 (3rd ed., 1998), vol. 3 (2nd ed., 1998), vol. 4A (2011) and
  vol. 4B (2022), Addison-Wesley;
- P. Ferragina, *Pearls of Algorithm Engineering*, Cambridge University
  Press, 2023;
- B. Meyer, *Object-Oriented Software Construction*, 2nd ed., Prentice
  Hall, 1997, for the design by contract;
- M. Cadoli, M. Lenzerini, P. Naggar, A. Schaerf, *Fondamenti della
  progettazione dei programmi: principi, tecniche e loro applicazioni in
  C++*, CittàStudi, 1997, for the specification of the abstract data types
  and their realization in C++.

## Build and test

With CMake,

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

or with the makefile, which builds and runs the same tests,

```sh
make test
```

A single exercise runs with `ctest -R ch04.ex4.4`, and a test program also
runs one test by name (`build/test/ch04 ex4.4`) and lists its tests
(`-l`).

The documentation is at [dmeoli.github.io/ADS](https://dmeoli.github.io/ADS/):
a site of Sphinx with the theme of Read the Docs, whose pages come from this
README and whose API reference Exhale and Breathe build from the comments of
the headers, extracted by Doxygen. Locally, after `pip install -r
docs/requirements.txt`, `python3 docs/pages.py && sphinx-build -b html docs
build-docs/html` writes it to `build-docs/html`.

## Use

```sh
cmake --install build --prefix <dir>     # or: make install PREFIX=<dir>
```

then, in another CMake project,

```cmake
find_package(ADS REQUIRED)
target_link_libraries(<target> PRIVATE ADS::ADS)
```

or, with pkg-config, `pkg-config --cflags ads`. The packages of vcpkg
(`ads`), conda-forge (`cpp-ads`), Homebrew, Debian and Ubuntu
(`libads-dev`), Conan, Spack, Nix, the AUR, MSYS2 and xmake install the same
files, from the recipes of [packaging](packaging/README.md). Without
installing anything, a CMake project can also take ADS at configure time,
with FetchContent (CMake 3.24 or later),

```cmake
include(FetchContent)
FetchContent_Declare(ADS
    URL https://github.com/dmeoli/ADS/releases/download/v0.1.0/ADS-0.1.0.tar.gz
    FIND_PACKAGE_ARGS 0.1)               # an installed ADS, if there is one
FetchContent_MakeAvailable(ADS)
target_link_libraries(<target> PRIVATE ADS::ADS)
```

or with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake),

```cmake
CPMAddPackage("gh:dmeoli/ADS@0.1.0")
target_link_libraries(<target> PRIVATE ADS::ADS)
```

and as a subproject ADS builds neither its tests nor its benchmarks. With
Bazel, `bazel_dep(name = "ads", version = "0.1.0")` in `MODULE.bazel` and
`deps = ["@ads"]`, from the Bazel Central Registry; with Meson, `meson wrap
install ads` and `dependency('ads')`, from WrapDB; in both, the targets
compile in C++20.

`BigInteger` (and through it `Rational`) can run on the backend of
[GMP](https://gmplib.org), which takes the products, the quotients, the
conversions and the greatest common divisor in the ranges of lengths where
GMP is the faster, and leaves the others to the code of ADS: with CMake,
`-DADS_WITH_GMP=ON` (off by default, and an error at configure time if GMP
is not found) defines `ADS_WITH_GMP` on `ADS::ADS` and links GMP, also for
the projects that find the installed ADS; without CMake, `-DADS_WITH_GMP`
and `-lgmp`, as `make test ADS_WITH_GMP=1 [GMP=<prefix>]` does. GMP is
under the LGPL v3 (or the GPL v2), compatible with the LGPL v3 of ADS;
without the macro nothing of GMP is included.

```cpp
#include "ADS/sequence/List.h"

ADS::List< int > L{ 3 , 1 , 2 };
for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
 std::cout << L.read( p );               // the positions of the book
std::ranges::reverse( L );               // the algorithms of std
for( int x : L ) std::cout << x;
```

## License

This software is released under the [GNU Lesser General Public License
version 3.0](https://opensource.org/licenses/lgpl-3.0.html) (see the file
[LICENSE](LICENSE)). The book is by Alberto Montresor, under CC BY-NC 4.0.

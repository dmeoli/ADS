# ADS — Algoritmi e Strutture Dati [![CI](https://github.com/dmeoli/ADS/actions/workflows/ci.yml/badge.svg)](https://github.com/dmeoli/ADS/actions/workflows/ci.yml)

ADS is a header-only C++20 library of the algorithms and the data
structures of the book

> A. Montresor, *Algoritmi e Strutture Dati*, 2025,
> <https://algoritmicamente.it> (CC BY-NC 4.0)

(a revision of A. A. Bertossi and A. Montresor, *Algoritmi e Strutture di
Dati*, 3rd edition, Città Studi, 2014), in which each exercise of the book
is a test. Each abstract data type is a C++20 concept, and its
realizations follow the pseudocode and the names of the book (`isEmpty()`,
`head()`, `next()`, `read()`, `push()`, `enqueue()`, and so on), so that the
code can be read side by side with the text. Where the book leaves a choice
open, or its pseudocode has a defect (a few figures, too, disagree with
the formulae they illustrate), the header documents the choice made and
the reason for it.

## Layout

The headers are grouped by abstract data type, one header per
realization: `include/ADS/ADT.h` has the concepts of the book, while
`sequence/`, `stack/`, `queue/`, `tree/`, `dictionary/`, `set/`,
`priority/`, `mfset/` and `graph/` have the realizations of each type
together with the algorithms on it (e.g., the operations on sets, the
visits and the shortest paths of a graph). The other folders hold the
algorithms grouped by technique or by problem: `search/` and `sort/` the
searches and the sorts, `numeric/` the products of integers, polynomials
and matrices, `string/` the algorithms on strings, `optimization/` the
problems of optimization (knapsack, intervals), `geometry/` the convex
hull, `backtrack/` the enumerations by backtrack, `complexity/` the
problems of NP and their reductions, `intractable/` the techniques for the
intractable problems, and `algorithm/` the rest. A header is included by
its path (e.g., `#include "ADS/graph/ListGraph.h"`) and includes what it
needs.

## Where each thing comes from

Each header and each test cites the section, the example or the exercise
of the book that it implements, and the tests are listed with their source
in [`test/tests.txt`](test/tests.txt); the solutions of the exercises are
in the tests, one function per exercise. Each solution is checked on the
example of the book and on its edge cases and, where an enumeration is
possible, on random instances against it. The text of the book is not
reproduced, since its license does not allow it in a GPL-licensed
repository; the code cites it instead.

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
| | 4.6 | exercises 4.1–4.11 | [`test/ch04.cpp`](test/ch04.cpp) | `ch04.ex4.*` |
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

A few structures and algorithms that the book does not describe, but that
a course on the subject usually covers, are written on the same classes
and tested in [`test/beyond.cpp`](test/beyond.cpp):

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

## Build and test

With CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

or with the makefile, which builds and runs the same tests:

```sh
make test
```

`ctest -R ch04.ex4.4` runs a single exercise; each test program also runs
one test by name (`build/test/ch04 ex4.4`) and lists them (`-l`).

## Use

```sh
cmake --install build --prefix <dir>     # or: make install PREFIX=<dir>
```

then, in another CMake project,

```cmake
find_package(ADS REQUIRED)
target_link_libraries(<target> PRIVATE ADS::ADS)
```

```cpp
#include "ADS/sequence/List.h"

ADS::List< int > L{ 1 , 2 , 3 };
for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
 std::cout << L.read( p );
```

## License

GNU GPL v3, see [LICENSE](LICENSE). The book is by Alberto Montresor,
under CC BY-NC 4.0.

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

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
- a CI job that runs the tests under AddressSanitizer and UBSan
- install and export for find_package(ADS), and the CI on GitHub Actions

/*--------------------------------------------------------------------------*/
/*----------------------------- File ADS.cppm ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Interface of the C++20 module ads, which exports every name that the
 * headers of include/ADS declare in the namespace ADS and in those nested in
 * it, but for the namespaces detail; a translation unit can then write
 *
 *     import ads;
 *
 * in place of the #include of the headers. The macros (e.g., ADS_PRE of
 * Contract.h) are not exported by a module, so a file that uses them still
 * includes the header that defines them, and those that configure the headers
 * (e.g., ADS_CONTRACTS, ADS_WITH_GMP) take the value they have where the
 * module is compiled. The names of the executors of OpenMP, oneTBB and
 * FastFlow and of the sort on MPI are exported only if the module is compiled
 * with their library, as their headers are.
 *
 * This file is written by cmake/ads_module_exports.py, which reads the names
 * from the AST of the headers: it is not to be edited by hand.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------- GLOBAL MODULE FRAGMENT -------------------------*/
/*--------------------------------------------------------------------------*/

module;

#include "ADS/ADT.h"
#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/Portable.h"
#include "ADS/algorithm/AkraBazzi.h"
#include "ADS/algorithm/BinaryCounter.h"
#include "ADS/algorithm/Hanoi.h"
#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/algorithm/MaximumSegments.h"
#include "ADS/algorithm/MonteCarlo.h"
#include "ADS/algorithm/Paradigms.h"
#include "ADS/algorithm/SequenceQueries.h"
#include "ADS/algorithm/Subsequences.h"
#include "ADS/backtrack/Anagrams.h"
#include "ADS/backtrack/DancingLinks.h"
#include "ADS/backtrack/Enumeration.h"
#include "ADS/backtrack/FastExactCover.h"
#include "ADS/backtrack/KnightTour.h"
#include "ADS/backtrack/NQueens.h"
#include "ADS/backtrack/SatSolvers.h"
#include "ADS/backtrack/Sudoku.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/combinatorial/FastBitwise.h"
#include "ADS/combinatorial/FastGeneration.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/complexity/Domino.h"
#include "ADS/complexity/EasyCases.h"
#include "ADS/complexity/Nondeterminism.h"
#include "ADS/complexity/Problems.h"
#include "ADS/complexity/Reductions.h"
#include "ADS/compression/ArithmeticCoding.h"
#include "ADS/compression/BinaryPacking.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/compression/CanonicalHuffman.h"
#include "ADS/compression/DirectAccessCodes.h"
#include "ADS/compression/Entropy.h"
#include "ADS/compression/FMIndex.h"
#include "ADS/compression/HuffmanCoder.h"
#include "ADS/compression/HuffmanFMIndex.h"
#include "ADS/compression/IntegerCodes.h"
#include "ADS/compression/LZ77Linear.h"
#include "ADS/compression/LempelZiv.h"
#include "ADS/compression/MoveRIndex.h"
#include "ADS/compression/PPM.h"
#include "ADS/compression/PackedBits.h"
#include "ADS/compression/RIndex.h"
#include "ADS/compression/Rans.h"
#include "ADS/compression/StreamVByte.h"
#include "ADS/compression/Tans.h"
#include "ADS/concurrent/ConcurrentCuckooTable.h"
#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/concurrent/EventCount.h"
#include "ADS/concurrent/MPMCQueue.h"
#include "ADS/concurrent/MPSCInbox.h"
#include "ADS/concurrent/MPSCQueue.h"
#include "ADS/concurrent/MultiQueue.h"
#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/concurrent/WorkStealingDeque.h"
#include "ADS/dictionary/AdaptiveRadixTree.h"
#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/CuckooHashing.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/DirectTable.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FingerprintPerfectHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/HashFunctions.h"
#include "ADS/dictionary/KnuthHashing.h"
#include "ADS/dictionary/MinimalPerfectHash.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/PatriciaTrie.h"
#include "ADS/dictionary/PerfectHashing.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/PositionIndex.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/dictionary/RowFingerprint.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/dictionary/SuccinctPatriciaTrie.h"
#include "ADS/dictionary/TableSize.h"
#include "ADS/dictionary/Trie.h"
#include "ADS/dictionary/TwoLevelDictionary.h"
#include "ADS/dictionary/UniversalHashing.h"
#if defined( ADS_WITH_MPI )
 #include "ADS/distributed/DistributedSort.h"
#endif
#include "ADS/external/ExternalSorting.h"
#include "ADS/external/ListRanking.h"
#include "ADS/external/TwoLevelMemory.h"
#include "ADS/geometry/ClosestPair.h"
#include "ADS/geometry/ConvexHull.h"
#include "ADS/geometry/IntervalTree.h"
#include "ADS/geometry/RangeTree.h"
#include "ADS/graph/AllPairsShortestPaths.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/Assignment.h"
#include "ADS/graph/EulerianCycle.h"
#include "ADS/graph/FlowApplications.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/GraphBuilder.h"
#include "ADS/graph/Grid.h"
#include "ADS/graph/GridGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/Paths.h"
#include "ADS/graph/PointerGraph.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/Visit.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/intractable/BranchAndBound.h"
#include "ADS/intractable/FifteenPuzzle.h"
#include "ADS/intractable/Heuristics.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/memory/Allocators.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/CellHeap.h"
#include "ADS/memory/ConcurrentPool.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/memory/PageHeap.h"
#include "ADS/memory/PoolAllocator.h"
#include "ADS/memory/SizeClassAllocator.h"
#include "ADS/mfset/ListMFSet.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/mfset/RemUnionFind.h"
#include "ADS/mfset/RollbackUnionFind.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/BinaryInteger.h"
#include "ADS/numeric/Catalan.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"
#include "ADS/numeric/DynamicCsr.h"
#include "ADS/numeric/FFT.h"
#include "ADS/numeric/Matrix.h"
#include "ADS/numeric/MatrixChain.h"
#include "ADS/numeric/NumberTheory.h"
#include "ADS/numeric/Polynomial.h"
#include "ADS/numeric/RSA.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/RandomSampling.h"
#include "ADS/numeric/Rational.h"
#include "ADS/numeric/SellMatrix.h"
#include "ADS/numeric/SparseAssembly.h"
#include "ADS/numeric/SparseMatrix.h"
#include "ADS/optimization/CoinChange.h"
#include "ADS/optimization/CoinGame.h"
#include "ADS/optimization/FractionalKnapsack.h"
#include "ADS/optimization/GridPaths.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/IntervalCover.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/Knapsack.h"
#include "ADS/optimization/KnapsackVariants.h"
#include "ADS/optimization/LinearPartition.h"
#include "ADS/optimization/OptimalSearchTree.h"
#include "ADS/optimization/SequenceProblems.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/optimization/StockTrading.h"
#include "ADS/optimization/WeightedIntervals.h"
#include "ADS/parallel/Executor.h"
#if defined( ADS_WITH_FASTFLOW )
 #include "ADS/parallel/FastFlowExecutor.h"
#endif
#if defined( _OPENMP )
 #include "ADS/parallel/OpenMPExecutor.h"
#endif
#include "ADS/parallel/ParallelFFT.h"
#include "ADS/parallel/ParallelGraph.h"
#include "ADS/parallel/ParallelGreedy.h"
#include "ADS/parallel/ParallelMerge.h"
#include "ADS/parallel/ParallelNumeric.h"
#include "ADS/parallel/ParallelShortestPaths.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/ParallelText.h"
#include "ADS/parallel/Scratch.h"
#include "ADS/parallel/SpeculativeFor.h"
#if defined( ADS_WITH_TBB )
 #include "ADS/parallel/TBBExecutor.h"
#endif
#include "ADS/parallel/ThreadPool.h"
#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/DialQueue.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/LoserTree.h"
#include "ADS/priority/MinMaxHeap.h"
#include "ADS/priority/MultiwayMerge.h"
#include "ADS/priority/OrderKey.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/priority/PriorityQueue.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/priority/SequenceHeap.h"
#include "ADS/priority/TreePriorityQueue.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/queue/LinkedQueue.h"
#include "ADS/queue/QueueAlgorithms.h"
#include "ADS/queue/RingDeque.h"
#include "ADS/queue/UniqueQueue.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/search/CompressedPGMIndex.h"
#include "ADS/search/CountLess.h"
#include "ADS/search/DynamicPGMIndex.h"
#include "ADS/search/InterpolationSearch.h"
#include "ADS/search/KnuthSearches.h"
#include "ADS/search/LocalOptima.h"
#include "ADS/search/Minimum.h"
#include "ADS/search/MonotoneSearch.h"
#include "ADS/search/NthElement.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/search/Selection.h"
#include "ADS/search/SortedSearches.h"
#include "ADS/search/StaticSearch.h"
#include "ADS/search/WordRangeMinimum.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/ListSort.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sequence/SequenceAlgorithms.h"
#include "ADS/sequence/SequenceRange.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sequence/SmallVector.h"
#include "ADS/sequence/TieredVector.h"
#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/BloomFilter.h"
#include "ADS/set/BloomVariants.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/DictionarySet.h"
#include "ADS/set/GrafiteFilter.h"
#include "ADS/set/HashMultiset.h"
#include "ADS/set/LinkedHashSet.h"
#include "ADS/set/ListSet.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/set/SetOperations.h"
#include "ADS/set/SortedIntersection.h"
#include "ADS/set/SortedListSet.h"
#include "ADS/set/VectorIntersection.h"
#include "ADS/sort/CountingSort.h"
#include "ADS/sort/CycleSort.h"
#include "ADS/sort/DidacticSorts.h"
#include "ADS/sort/EngineeredQuickSorts.h"
#include "ADS/sort/ExchangeSorts.h"
#include "ADS/sort/ExternalSort.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/KnuthSorts.h"
#include "ADS/sort/LearnedSort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/MultikeyQuickSort.h"
#include "ADS/sort/PatienceSort.h"
#include "ADS/sort/PigeonholeSort.h"
#include "ADS/sort/QuickSort.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/SelectionSort.h"
#include "ADS/sort/ShellSort.h"
#include "ADS/sort/Smoothsort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/Sorted.h"
#include "ADS/sort/SortingNetworks.h"
#include "ADS/sort/StrandSort.h"
#include "ADS/sort/StringSort.h"
#include "ADS/sort/TreeSort.h"
#include "ADS/sort/VectorQuickSort.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/Expressions.h"
#include "ADS/stack/MultiStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/stack/StackAlgorithms.h"
#include "ADS/string/AhoCorasick.h"
#include "ADS/string/Alignment.h"
#include "ADS/string/Anagrams.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/EditDistance.h"
#include "ADS/string/FrontCoding.h"
#include "ADS/string/Huffman.h"
#include "ADS/string/LongestCommonExtension.h"
#include "ADS/string/LongestCommonSubsequence.h"
#include "ADS/string/Lyndon.h"
#include "ADS/string/MultiSearch.h"
#include "ADS/string/Palindromes.h"
#include "ADS/string/Parentheses.h"
#include "ADS/string/PrefixSearch.h"
#include "ADS/string/RearCoding.h"
#include "ADS/string/Segmentations.h"
#include "ADS/string/StringBreaking.h"
#include "ADS/string/StringMatching.h"
#include "ADS/string/StringSearch.h"
#include "ADS/string/SubsequenceProblems.h"
#include "ADS/string/SuffixApplications.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixAutomaton.h"
#include "ADS/string/SuffixSorting.h"
#include "ADS/string/SuffixTree.h"
#include "ADS/string/WordBreak.h"
#include "ADS/succinct/BPTree.h"
#include "ADS/succinct/BVGraph.h"
#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/BitStream.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/EliasFanoGraph.h"
#include "ADS/succinct/HuffmanWaveletTree.h"
#include "ADS/succinct/InterleavedRankSelect.h"
#include "ADS/succinct/JacobsonTree.h"
#include "ADS/succinct/K2Tree.h"
#include "ADS/succinct/LAVector.h"
#include "ADS/succinct/LoudsTree.h"
#include "ADS/succinct/MoveStructure.h"
#include "ADS/succinct/PartitionedEliasFano.h"
#include "ADS/succinct/RRRBitVector.h"
#include "ADS/succinct/RankSelect.h"
#include "ADS/succinct/WaveletMatrix.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/ArrayBinaryTree.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/BTreeMap.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/ChildListTree.h"
#include "ADS/tree/CursorBinaryTree.h"
#include "ADS/tree/CursorTree.h"
#include "ADS/tree/ExpressionTree.h"
#include "ADS/tree/HeapByInsertion.h"
#include "ADS/tree/JoinAlgorithms.h"
#include "ADS/tree/JoinTree.h"
#include "ADS/tree/LinkCutTree.h"
#include "ADS/tree/ParentTree.h"
#include "ADS/tree/PreorderArrayTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/ThreadedTree.h"
#include "ADS/tree/Treap.h"
#include "ADS/tree/Tree.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- MODULE PURVIEW -----------------------------*/
/*--------------------------------------------------------------------------*/

export module ads;

export namespace ADS {

 // ADS/ADT.h
 using ADS::Dictionary;
 using ADS::Queue;
 using ADS::Sequence;
 using ADS::Set;
 using ADS::Stack;

 // ADS/Contract.h
 using ADS::contract_violation;

 // ADS/Iterators.h
 using ADS::CircularIterator;
 using ADS::IndexIterator;
 using ADS::IndirectIterator;
 using ADS::KeyIterator;
 using ADS::LinkedIterator;
 using ADS::PathIterator;
 using ADS::SlotIterator;
 using ADS::WalkIterator;

 // ADS/algorithm/AkraBazzi.h
 using ADS::akraBazzi;
 using ADS::akraBazziExponent;

 // ADS/algorithm/BinaryCounter.h
 using ADS::add;

 // ADS/algorithm/Hanoi.h
 using ADS::hanoi_iterative;
 using ADS::hanoi_iterative_labels;
 using ADS::hanoi_recursive;

 // ADS/algorithm/MasterTheorem.h
 using ADS::Growth;
 using ADS::linear;
 using ADS::master;

 // ADS/algorithm/MaximumSegments.h
 using ADS::Segment;
 using ADS::gcIndicator;
 using ADS::longestSegmentAtLeast;
 using ADS::longestSegmentAverageAtLeast;
 using ADS::maxDensitySegment;
 using ADS::maxSumSegment;
 using ADS::maxSumSegmentCubic;
 using ADS::maxSumSegmentLength;
 using ADS::maxSumSegmentPrefix;
 using ADS::maxSumSegmentQuadratic;

 // ADS/algorithm/MonteCarlo.h
 using ADS::freivalds;
 using ADS::halfSatisfiable;
 using ADS::zeroPolynomial;

 // ADS/algorithm/Paradigms.h
 using ADS::backtrack;
 using ADS::divideEtImpera;
 using ADS::enumerateOptimize;
 using ADS::enumerateSearch;
 using ADS::greedy;
 using ADS::maximal;

 // ADS/algorithm/SequenceQueries.h
 using ADS::largestGap;
 using ADS::lineAssignment;
 using ADS::longestKDistinctRun;
 using ADS::smallerToTheRight;

 // ADS/algorithm/Subsequences.h
 using ADS::lis;
 using ADS::lisLength;
 using ADS::maxSubarraySum;

 // ADS/backtrack/Anagrams.h
 using ADS::anagrams;

 // ADS/backtrack/DancingLinks.h
 using ADS::ExactCover;

 // ADS/backtrack/Enumeration.h
 using ADS::derangements;
 using ADS::permutations;
 using ADS::subsetSum;
 using ADS::subsets;

 // ADS/backtrack/FastExactCover.h
 using ADS::FastExactCover;

 // ADS/backtrack/KnightTour.h
 using ADS::knightTour;

 // ADS/backtrack/NQueens.h
 using ADS::admissible;
 using ADS::board;
 using ADS::nQueens;
 using ADS::nQueensIterative;

 // ADS/backtrack/SatSolvers.h
 using ADS::BasicCdcl;
 using ADS::Cdcl;
 using ADS::dpll;

 // ADS/backtrack/Sudoku.h
 using ADS::Grid;
 using ADS::check;
 using ADS::sudoku;

 // ADS/combinatorial/Bitwise.h
 using ADS::Word;
 using ADS::deinterleave;
 using ADS::gray;
 using ADS::grayInverse;
 using ADS::hasZeroByte;
 using ADS::interleave;
 using ADS::lambda;
 using ADS::nextSameWeight;
 using ADS::reverseBits;
 using ADS::rightmostOne;
 using ADS::ruler;
 using ADS::sidewaysSum;
 using ADS::smearRight;

 // ADS/combinatorial/FastBitwise.h
 using ADS::mortonDecode;
 using ADS::mortonEncode;
 using ADS::pdep;
 using ADS::pext;
 using ADS::popcount;
 using ADS::popcountAnd;
 using ADS::select;

 // ADS/combinatorial/FastGeneration.h
 using ADS::forEachCombination;
 using ADS::forEachCombinationMask;
 using ADS::forEachPermutation;
 using ADS::forEachPermutationLex;
 using ADS::nextCombination;

 // ADS/combinatorial/Generation.h
 using ADS::Tuple;
 using ADS::combinationsLex;
 using ADS::grayTuples;
 using ADS::heapPermutations;
 using ADS::integerPartitions;
 using ADS::mixedRadix;
 using ADS::nestedParentheses;
 using ADS::permutationsLex;
 using ADS::plainChanges;
 using ADS::revolvingDoor;
 using ADS::setPartitions;

 // ADS/complexity/Domino.h
 using ADS::Tile;
 using ADS::Tiling;
 using ADS::legalTiling;
 using ADS::linearTiling;
 using ADS::squareTiling;

 // ADS/complexity/EasyCases.h
 using ADS::bipartiteMatching;
 using ADS::intervalClique;
 using ADS::intervalGraph;
 using ADS::literalNode;
 using ADS::twoColoring;
 using ADS::twoSat;

 // ADS/complexity/Nondeterminism.h
 using ADS::Chooser;

 // ADS/complexity/Problems.h
 using ADS::Adjacency;
 using ADS::Clause;
 using ADS::Cnf;
 using ADS::Nodes;
 using ADS::bisection;
 using ADS::boundedSpanningTree;
 using ADS::bribes;
 using ADS::clique;
 using ADS::cliqueCertificate;
 using ADS::coloring;
 using ADS::exactCover3;
 using ADS::hamiltonianChain;
 using ADS::hamiltonianCircuit;
 using ADS::hamiltonianCycle;
 using ADS::hamiltonianPath;
 using ADS::independentSet;
 using ADS::knapsackDecision;
 using ADS::longestPath;
 using ADS::longestPathDag;
 using ADS::lp01;
 using ADS::multiprocessorScheduling;
 using ADS::partition;
 using ADS::satCertificate;
 using ADS::satisfiable;
 using ADS::subsetSumExists;
 using ADS::threeDimensionalMatching;
 using ADS::tsp;
 using ADS::tspCertificate;
 using ADS::vertexCover;

 // ADS/complexity/Reductions.h
 using ADS::complement;
 using ADS::cycleToChain;
 using ADS::cycleToPath;
 using ADS::dominoToClique;
 using ADS::dominoToLp01;
 using ADS::dominoToSat;
 using ADS::halfSum;
 using ADS::hamiltonToTsp;
 using ADS::sortToHull;
 using ADS::tileVar;

 // ADS/compression/ArithmeticCoding.h
 using ADS::RangeDecoder;
 using ADS::RangeEncoder;
 using ADS::adaptiveArithmeticDecode;
 using ADS::adaptiveArithmeticEncode;
 using ADS::arithmeticCodeExact;
 using ADS::arithmeticDecode;
 using ADS::arithmeticDecodeExact;
 using ADS::arithmeticEncode;
 using ADS::arithmeticInterval;

 // ADS/compression/BinaryPacking.h
 using ADS::binaryPack;
 using ADS::binaryUnpack;
 using ADS::patchedPack;
 using ADS::patchedUnpack;

 // ADS/compression/BurrowsWheeler.h
 using ADS::BWT;
 using ADS::bwt;
 using ADS::bzipCompress;
 using ADS::bzipDecompress;
 using ADS::inverseBwt;
 using ADS::inverseMoveToFront;
 using ADS::lfMapping;
 using ADS::moveToFront;
 using ADS::runLengthDecode;
 using ADS::runLengthEncode;
 using ADS::zeroRunDecode;
 using ADS::zeroRunEncode;

 // ADS/compression/CanonicalHuffman.h
 using ADS::CanonicalHuffman;
 using ADS::huffmanLengths;
 using ADS::limitedLengths;

 // ADS/compression/DirectAccessCodes.h
 using ADS::DirectAccessCodes;

 // ADS/compression/Entropy.h
 using ADS::entropy;

 // ADS/compression/FMIndex.h
 using ADS::FMIndex;

 // ADS/compression/HuffmanCoder.h
 using ADS::huffmanCompress;
 using ADS::huffmanDecompress;
 using ADS::limitedHuffmanLengths;

 // ADS/compression/HuffmanFMIndex.h
 using ADS::HuffmanFMIndex;

 // ADS/compression/IntegerCodes.h
 using ADS::PForDeltaBlock;
 using ADS::binaryLength;
 using ADS::dGaps;
 using ADS::decodeBounded;
 using ADS::decodeDelta;
 using ADS::decodeDense;
 using ADS::decodeGamma;
 using ADS::decodeGolomb;
 using ADS::decodeInterpolative;
 using ADS::decodePForDelta;
 using ADS::decodeRice;
 using ADS::decodeUnary;
 using ADS::decodeVByte;
 using ADS::deltaLength;
 using ADS::denseLength;
 using ADS::denseOptimalS;
 using ADS::encodeBounded;
 using ADS::encodeDelta;
 using ADS::encodeDense;
 using ADS::encodeGamma;
 using ADS::encodeGolomb;
 using ADS::encodeInterpolative;
 using ADS::encodePForDelta;
 using ADS::encodeRice;
 using ADS::encodeUnary;
 using ADS::encodeVByte;
 using ADS::gammaLength;
 using ADS::golombLength;
 using ADS::naturalToSigned;
 using ADS::pforDeltaWidth;
 using ADS::prefixSums;
 using ADS::riceLength;
 using ADS::signedToNatural;
 using ADS::unaryLength;
 using ADS::vbyteLength;

 // ADS/compression/LZ77Linear.h
 using ADS::lz77ParseLinear;

 // ADS/compression/LempelZiv.h
 using ADS::LZ77Phrase;
 using ADS::LZ78Phrase;
 using ADS::LZSSPhrase;
 using ADS::lz77Decode;
 using ADS::lz77Parse;
 using ADS::lz78Decode;
 using ADS::lz78Parse;
 using ADS::lzssDecode;
 using ADS::lzssParse;
 using ADS::lzssParseHashed;
 using ADS::lzwDecode;
 using ADS::lzwEncode;

 // ADS/compression/MoveRIndex.h
 using ADS::MoveRIndex;

 // ADS/compression/PPM.h
 using ADS::PPMModel;
 using ADS::ppmDecode;
 using ADS::ppmEncode;

 // ADS/compression/PackedBits.h
 using ADS::PackedBitReader;
 using ADS::PackedBitWriter;

 // ADS/compression/RIndex.h
 using ADS::RIndex;

 // ADS/compression/Rans.h
 using ADS::ransCompress;
 using ADS::ransDecompress;
 using ADS::scaleFrequencies;

 // ADS/compression/StreamVByte.h
 using ADS::streamVByteDecode;
 using ADS::streamVByteEncode;

 // ADS/compression/Tans.h
 using ADS::tansCompress;
 using ADS::tansDecompress;

 // ADS/concurrent/ConcurrentCuckooTable.h
 using ADS::ConcurrentCuckooTable;

 // ADS/concurrent/ConcurrentFlatTable.h
 using ADS::ConcurrentFlatTable;

 // ADS/concurrent/ConcurrentUnionFind.h
 using ADS::ConcurrentUnionFind;

 // ADS/concurrent/EventCount.h
 using ADS::EventCount;

 // ADS/concurrent/MPMCQueue.h
 using ADS::MPMCQueue;

 // ADS/concurrent/MPSCInbox.h
 using ADS::MPSCInbox;

 // ADS/concurrent/MPSCQueue.h
 using ADS::MPSCQueue;

 // ADS/concurrent/MultiQueue.h
 using ADS::MultiQueue;

 // ADS/concurrent/SPSCQueue.h
 using ADS::SPSCQueue;

 // ADS/concurrent/WorkStealingDeque.h
 using ADS::WorkStealingDeque;

 // ADS/dictionary/AdaptiveRadixTree.h
 using ADS::AdaptiveRadixTree;

 // ADS/dictionary/BucketCuckooTable.h
 using ADS::BucketCuckooTable;

 // ADS/dictionary/ChainedHashTable.h
 using ADS::Chain;
 using ADS::ChainedHashTable;

 // ADS/dictionary/CuckooHashing.h
 using ADS::CuckooHashTable;

 // ADS/dictionary/DenseHashMap.h
 using ADS::DenseHashMap;

 // ADS/dictionary/DirectTable.h
 using ADS::DirectTable;

 // ADS/dictionary/FastHash.h
 using ADS::DefaultHash;
 using ADS::Product128;
 using ADS::foldedProduct;
 using ADS::hashBytes;
 using ADS::hashWord;
 using ADS::product128;

 // ADS/dictionary/FingerprintPerfectHash.h
 using ADS::FingerprintPerfectHash;

 // ADS/dictionary/FlatHashTable.h
 using ADS::FlatHashTable;

 // ADS/dictionary/HashFunctions.h
 using ADS::bin_bit;
 using ADS::hash_division;
 using ADS::hash_extraction;
 using ADS::hash_fibonacci;
 using ADS::hash_midsquare;
 using ADS::hash_multiplication;
 using ADS::hash_xor;
 using ADS::int_bin;

 // ADS/dictionary/KnuthHashing.h
 using ADS::BrentHashTable;
 using ADS::CoalescedHashTable;

 // ADS/dictionary/MinimalPerfectHash.h
 using ADS::MinimalPerfectHash;

 // ADS/dictionary/OpenHashTable.h
 using ADS::OpenHashTable;
 using ADS::Probing;

 // ADS/dictionary/PatriciaTrie.h
 using ADS::PatriciaTrie;

 // ADS/dictionary/PerfectHashing.h
 using ADS::OrderedMinimalPerfectHash;
 using ADS::PerfectHashTable;

 // ADS/dictionary/PointerTable.h
 using ADS::PointerTable;
 using ADS::StateEqual;

 // ADS/dictionary/PositionIndex.h
 using ADS::PositionIndex;

 // ADS/dictionary/RandomHashing.h
 using ADS::SplitMix64;
 using ADS::TabulationHash;
 using ADS::ZobristTable;
 using ADS::fibonacciPlace;
 using ADS::splitmix64;

 // ADS/dictionary/RecSplit.h
 using ADS::RecSplit;

 // ADS/dictionary/RowFingerprint.h
 using ADS::canonicalWord;
 using ADS::rowFingerprint;
 using ADS::sameRow;

 // ADS/dictionary/SkipList.h
 using ADS::SkipList;

 // ADS/dictionary/SortedArrayDictionary.h
 using ADS::SortedArrayDictionary;

 // ADS/dictionary/SuccinctPatriciaTrie.h
 using ADS::SuccinctPatriciaTrie;

 // ADS/dictionary/TableSize.h
 using ADS::is_pow2;
 using ADS::is_prime;
 using ADS::next_prime;
 using ADS::resized;

 // ADS/dictionary/Trie.h
 using ADS::RadixTree;
 using ADS::Trie;

 // ADS/dictionary/TwoLevelDictionary.h
 using ADS::TwoLevelDictionary;

 // ADS/dictionary/UniversalHashing.h
 using ADS::CarterWegmanHash;
 using ADS::DLeftHashTable;
 using ADS::DotProductHash;
 using ADS::MultiplyShiftHash;
 using ADS::StringHash;

 // ADS/external/ExternalSorting.h
 using ADS::binaryMergeSort;
 using ADS::distributionSort;
 using ADS::mergeSortIOs;
 using ADS::multiwayMergeSort;
 using ADS::permuteBySorting;
 using ADS::permuteDirect;
 using ADS::permutingLowerBound;
 using ADS::sortingLowerBound;

 // ADS/external/ListRanking.h
 using ADS::deterministicCoinTossing;
 using ADS::externalListRank;
 using ADS::externalPointerJumping;
 using ADS::independentSetOfColors;
 using ADS::isList;
 using ADS::listRank;
 using ADS::listRankIndependentSet;
 using ADS::listRankPointerJumping;
 using ADS::parallelStep;
 using ADS::randomIndependentSet;

 // ADS/external/TwoLevelMemory.h
 using ADS::DiskFile;
 using ADS::DiskReader;
 using ADS::DiskWriter;
 using ADS::MemoryLease;
 using ADS::TwoLevelMemory;

 // ADS/geometry/ClosestPair.h
 using ADS::closestPair;
 using ADS::closestPairBrute;
 using ADS::closestPairSweep;
 using ADS::squaredDistance;

 // ADS/geometry/ConvexHull.h
 using ADS::Point;
 using ADS::convexHull;
 using ADS::convexHullBrute;
 using ADS::cross;
 using ADS::graham;
 using ADS::pseudoAngle;
 using ADS::sameSide;

 // ADS/geometry/IntervalTree.h
 using ADS::IntervalTree;

 // ADS/geometry/RangeTree.h
 using ADS::RangeTree;

 // ADS/graph/AllPairsShortestPaths.h
 using ADS::AllPairs;
 using ADS::addEdge;
 using ADS::floydWarshall;

 // ADS/graph/ArrayGraph.h
 using ADS::ArrayGraph;

 // ADS/graph/Assignment.h
 using ADS::Assignment;

 // ADS/graph/EulerianCycle.h
 using ADS::eulerianCycle;
 using ADS::isEulerianCycle;

 // ADS/graph/FlowApplications.h
 using ADS::CapacityDecrease;
 using ADS::decreaseCapacity;
 using ADS::edgeDisjointPaths;

 // ADS/graph/Graph.h
 using ADS::DynamicGraph;
 using ADS::Graph;
 using ADS::GraphNodes;
 using ADS::adjFromIncidence;
 using ADS::incidence;

 // ADS/graph/GraphBuilder.h
 using ADS::GraphBuilder;

 // ADS/graph/Grid.h
 using ADS::EIGHT_MOVES;
 using ADS::FOUR_MOVES;
 using ADS::FourMoves;
 using ADS::GridCell;
 using ADS::KNIGHT_MOVES;
 using ADS::floodFill;
 using ADS::gridDistance;
 using ADS::gridDistances;
 using ADS::insideGrid;
 using ADS::knightDistance;

 // ADS/graph/GridGraph.h
 using ADS::Corners;
 using ADS::GridGraph;
 using ADS::GridMoves;
 using ADS::chebyshev;
 using ADS::gridHeuristic;
 using ADS::manhattan;
 using ADS::octile;
 using ADS::readGridMap;

 // ADS/graph/ListGraph.h
 using ADS::ListGraph;

 // ADS/graph/Matching.h
 using ADS::bipartiteBMatching;
 using ADS::maxWeightMatching;

 // ADS/graph/MatrixGraph.h
 using ADS::MatrixGraph;

 // ADS/graph/MaxFlow.h
 using ADS::Augment;
 using ADS::BoundedEdge;
 using ADS::FlowNetwork;
 using ADS::feasibleCirculation;

 // ADS/graph/MinimumSpanningTree.h
 using ADS::MergeCounts;
 using ADS::Mergeable;
 using ADS::WeightedEdge;
 using ADS::edges;
 using ADS::isMinimumSpanningForest;
 using ADS::kruskal;
 using ADS::prim;

 // ADS/graph/Observable.h
 using ADS::EdgeDeleted;
 using ADS::EdgeInserted;
 using ADS::GraphEvent;
 using ADS::GraphEvents;
 using ADS::GraphObserver;
 using ADS::NodeDeleted;
 using ADS::NodeInserted;
 using ADS::Observable;
 using ADS::WeightChanged;
 using ADS::coalesce;
 using ADS::endsOf;
 using ADS::kindOf;

 // ADS/graph/Paths.h
 using ADS::PathLength;
 using ADS::countShortestPaths;
 using ADS::dagLongestPaths;
 using ADS::longestPathLayering;
 using ADS::multiSourceDistance;
 using ADS::refuelingStops;
 using ADS::setDistance;
 using ADS::simpleCycles;

 // ADS/graph/PointerGraph.h
 using ADS::PointerGraph;

 // ADS/graph/Properties.h
 using ADS::countTriangles;
 using ADS::diameter;
 using ADS::isTree;
 using ADS::treeDiameter;
 using ADS::treeVertexCover;
 using ADS::uniqueTopologicalOrder;
 using ADS::universalSink;

 // ADS/graph/Reoptimization.h
 using ADS::AssignmentObserver;
 using ADS::EventQueue;
 using ADS::MaxFlowObserver;
 using ADS::Reopt;
 using ADS::Reoptimizable;
 using ADS::ShortestPathsObserver;
 using ADS::SpanningForestObserver;

 // ADS/graph/ShortestPaths.h
 using ADS::DequeFrontier;
 using ADS::Frontier;
 using ADS::FrontierCounts;
 using ADS::PriorityFrontier;
 using ADS::PrototypeFrontier;
 using ADS::QueueFrontier;
 using ADS::ShortestPathTree;
 using ADS::StackFrontier;
 using ADS::bellmanFordMoore;
 using ADS::dagShortestPaths;
 using ADS::dijkstra;
 using ADS::fredmanTarjan;
 using ADS::infinity;
 using ADS::isShortestPathTree;
 using ADS::johnson;
 using ADS::papeDEsopo;
 using ADS::reachesNegativeCycle;
 using ADS::shortestPaths;

 // ADS/graph/StaticGraph.h
 using ADS::StaticGraph;
 using ADS::WeightedArc;
 using ADS::bfsOrder;

 // ADS/graph/Visit.h
 using ADS::Choice;
 using ADS::DfsSchema;
 using ADS::EdgeKind;
 using ADS::INF;
 using ADS::NIL;
 using ADS::VisitCounts;
 using ADS::bfs;
 using ADS::cc;
 using ADS::dfs;
 using ADS::distance;
 using ADS::genericVisit;
 using ADS::hasCycle;
 using ADS::isTopologicalOrder;
 using ADS::path;
 using ADS::scc;
 using ADS::topsort;
 using ADS::topsortKahn;
 using ADS::transpose;
 using ADS::unilaterallyConnected;

 // ADS/intractable/Approximation.h
 using ADS::apSubsetSum;
 using ADS::deltaTsp;
 using ADS::firstFit;
 using ADS::knapsackApprox;
 using ADS::tourCost;
 using ADS::tspGap;
 using ADS::vertexCoverApprox;

 // ADS/intractable/BranchAndBound.h
 using ADS::TspBound;
 using ADS::bbTsp;
 using ADS::knapsackBB;
 using ADS::knapsackBacktrack;
 using ADS::knapsackGreedy;
 using ADS::littleTsp;
 using ADS::maxWeightClique;
 using ADS::oneTree;
 using ADS::spanningTreeWeight;
 using ADS::tspLowerBound;

 // ADS/intractable/FifteenPuzzle.h
 using ADS::Board;
 using ADS::puzzleBFS;
 using ADS::puzzleDFS;
 using ADS::puzzleGoal;
 using ADS::puzzleMoves;
 using ADS::puzzleSpiralGoal;
 using ADS::solvable;
 using ADS::solvePuzzle;

 // ADS/intractable/Heuristics.h
 using ADS::Buckets;
 using ADS::cutSize;
 using ADS::dsatur;
 using ADS::greedySuperstring;
 using ADS::greedyTsp;
 using ADS::kOptNeighbours;
 using ADS::minMaxGreedy;
 using ADS::overlap;
 using ADS::partitionLocalSearch;
 using ADS::starAlignment;
 using ADS::twoOpt;
 using ADS::twoOptMove;

 // ADS/intractable/Pseudopolynomial.h
 using ADS::Piece;
 using ADS::bottino;
 using ADS::mcNaughton;
 using ADS::partitionDP;
 using ADS::realTimeScheduling;
 using ADS::schedulingDP;
 using ADS::subsetSumDP;
 using ADS::subsetSumSet;

 // ADS/memory/Allocators.h
 using ADS::BoundaryTagAllocator;
 using ADS::BuddyAllocator;

 // ADS/memory/Arena.h
 using ADS::Arena;

 // ADS/memory/CellHeap.h
 using ADS::CellHeap;

 // ADS/memory/ConcurrentPool.h
 using ADS::ConcurrentPoolResource;

 // ADS/memory/HugePageAllocator.h
 using ADS::HugePageAllocator;

 // ADS/memory/ObjectPool.h
 using ADS::ObjectPool;

 // ADS/memory/PoolAllocator.h
 using ADS::PoolAllocator;
 using ADS::PoolResource;

 // ADS/memory/SizeClassAllocator.h
 using ADS::SizeClassAllocator;

 // ADS/mfset/ListMFSet.h
 using ADS::ListMFSet;

 // ADS/mfset/MFSet.h
 using ADS::MFSet;
 using ADS::components;

 // ADS/mfset/RemUnionFind.h
 using ADS::RemUnionFind;

 // ADS/mfset/RollbackUnionFind.h
 using ADS::ConnectivityOp;
 using ADS::RollbackUnionFind;
 using ADS::offlineConnectivity;

 // ADS/mfset/UnionFind.h
 using ADS::Compression;
 using ADS::DisjointSets;
 using ADS::Union;
 using ADS::UnionFind;

 // ADS/numeric/BigInteger.h
 using ADS::BigInteger;
 using ADS::operator%;
 using ADS::operator*;
 using ADS::operator+;
 using ADS::operator-;
 using ADS::operator/;
 using ADS::operator<=>;

 // ADS/numeric/BinaryInteger.h
 using ADS::Bits;
 using ADS::ahmes;
 using ADS::binaryAdd;
 using ADS::carry;
 using ADS::fromBits;
 using ADS::multiplyBinaryFour;
 using ADS::multiplyBinaryKaratsuba;
 using ADS::multiplyBinarySchool;
 using ADS::toBits;
 using ADS::widen;

 // ADS/numeric/Catalan.h
 using ADS::catalan;

 // ADS/numeric/Combinatorics.h
 using ADS::FibonacciCache;
 using ADS::binomial;
 using ADS::binomialByFactorials;
 using ADS::factorial;
 using ADS::factorialRec;
 using ADS::fibonacci;
 using ADS::fibonacciTable;
 using ADS::tartaglia;

 // ADS/numeric/CsrMatrix.h
 using ADS::CsrMatrix;

 // ADS/numeric/DenseMatrix.h
 using ADS::DenseMatrix;
 using ADS::gemm;
 using ADS::multiplyStrassen;

 // ADS/numeric/DynamicCsr.h
 using ADS::DynamicCsc;
 using ADS::DynamicCsr;

 // ADS/numeric/FFT.h
 using ADS::Complex;
 using ADS::convolve;
 using ADS::dct;
 using ADS::dft;
 using ADS::dst;
 using ADS::fft;
 using ADS::fft2;
 using ADS::fftMany;
 using ADS::fftn;
 using ADS::irfft;
 using ADS::rfft;

 // ADS/numeric/Matrix.h
 using ADS::Matrix;
 using ADS::multiply;
 using ADS::scale;
 using ADS::strassen;

 // ADS/numeric/MatrixChain.h
 using ADS::matrixChain;
 using ADS::parenthesization;

 // ADS/numeric/NumberTheory.h
 using ADS::Montgomery64;
 using ADS::U64;
 using ADS::binaryGcd;
 using ADS::extendedEuclid;
 using ADS::factorize;
 using ADS::gcd;
 using ADS::isPrime;
 using ADS::isPrimeTrial;
 using ADS::kMersenne61;
 using ADS::millerRabin;
 using ADS::modInverse;
 using ADS::mulMod;
 using ADS::mulMod61;
 using ADS::powMod;
 using ADS::sieve;
 using ADS::witness;

 // ADS/numeric/Polynomial.h
 using ADS::multiplyFour;
 using ADS::multiplyKaratsuba;
 using ADS::multiplyNaive;

 // ADS/numeric/RSA.h
 using ADS::RsaKey;
 using ADS::randomPrime;
 using ADS::rsa;
 using ADS::rsaKeys;

 // ADS/numeric/RandomGenerators.h
 using ADS::AliasTable;
 using ADS::FullWordGenerator;
 using ADS::Philox4x32;
 using ADS::Philox4x64;
 using ADS::PhiloxT;
 using ADS::WyRand;
 using ADS::Xoshiro256;
 using ADS::Xoshiro256StarStar;
 using ADS::Xoshiro256T;
 using ADS::exponentialZiggurat;
 using ADS::normalZiggurat;
 using ADS::shuffleFast;
 using ADS::uniformBelow;
 using ADS::uniformReal;

 // ADS/numeric/RandomNumbers.h
 using ADS::LinearCongruential;
 using ADS::LinearCongruentialT;
 using ADS::MinStd;
 using ADS::Mmix;
 using ADS::chiSquare;
 using ADS::kolmogorovSmirnov;
 using ADS::reservoirSampling;
 using ADS::selectionSampling;
 using ADS::serialTest;
 using ADS::shuffle;
 using ADS::spectralTest2;

 // ADS/numeric/RandomSampling.h
 using ADS::heapSampling;
 using ADS::reservoirSampleL;
 using ADS::sampleByDictionary;
 using ADS::sampleBySkips;
 using ADS::sampleBySorting;
 using ADS::sampleBySwaps;

 // ADS/numeric/Rational.h
 using ADS::Rational;

 // ADS/numeric/SellMatrix.h
 using ADS::SellMatrix;

 // ADS/numeric/SparseAssembly.h
 using ADS::AssemblyWorkspace;
 using ADS::assembleCsc;
 using ADS::assembleCsr;
 using ADS::compressTriplets;

 // ADS/numeric/SparseMatrix.h
 using ADS::SparseMatrix;

 // ADS/optimization/CoinChange.h
 using ADS::greedyChange;
 using ADS::optimalChange;

 // ADS/optimization/CoinGame.h
 using ADS::coinsInALine;

 // ADS/optimization/FractionalKnapsack.h
 using ADS::fractionalKnapsack;

 // ADS/optimization/GridPaths.h
 using ADS::GridSquare;
 using ADS::countLatticePaths;
 using ADS::largestSquare;

 // ADS/optimization/HouseProblems.h
 using ADS::maxNonAdjacentSum;
 using ADS::nonAdjacentSelection;
 using ADS::paintHouses;

 // ADS/optimization/IntervalCover.h
 using ADS::minimumIntervalCover;

 // ADS/optimization/Intervals.h
 using ADS::intervalColoring;
 using ADS::towers;

 // ADS/optimization/Knapsack.h
 using ADS::knapsack;
 using ADS::zaino;

 // ADS/optimization/KnapsackVariants.h
 using ADS::coinChangeWays;
 using ADS::minValueExactWeight;
 using ADS::minimumSquares;
 using ADS::rodCutting;
 using ADS::subsetSumCount;
 using ADS::unboundedKnapsack;

 // ADS/optimization/LinearPartition.h
 using ADS::linearPartition;
 using ADS::painterPartition;

 // ADS/optimization/OptimalSearchTree.h
 using ADS::OptimalSearchTree;
 using ADS::optimalSearchTree;

 // ADS/optimization/SequenceProblems.h
 using ADS::largestDivisibleSubset;
 using ADS::longestChainSubsequence;
 using ADS::longestDistinctSegment;
 using ADS::maxSumIncreasingSubsequence;
 using ADS::trappingRainWater;

 // ADS/optimization/ShortestJobFirst.h
 using ADS::shortestJobFirst;
 using ADS::smithOrder;
 using ADS::totalCompletion;
 using ADS::weightedCompletion;

 // ADS/optimization/StockTrading.h
 using ADS::maxProfit;
 using ADS::maxProfitExactly;

 // ADS/optimization/WeightedIntervals.h
 using ADS::WeightedInterval;
 using ADS::maxinterval;

 // ADS/parallel/Executor.h
 using ADS::Executor;
 using ADS::Sequential;
 using ADS::blocksOf;
 using ADS::kBlocks;
 using ADS::kGrain;
 using ADS::parallelExclusiveScan;
 using ADS::parallelFor;
 using ADS::parallelForBlocks;
 using ADS::parallelInclusiveScan;
 using ADS::parallelInvoke;
 using ADS::parallelReduce;

 // ADS/parallel/FastFlowExecutor.h
#if defined( ADS_WITH_FASTFLOW )
 using ADS::FastFlowExecutor;
#endif

 // ADS/parallel/OpenMPExecutor.h
#if defined( _OPENMP )
 using ADS::OpenMPExecutor;
#endif

 // ADS/parallel/ParallelFFT.h
 using ADS::kParallelFFT;

 // ADS/parallel/ParallelGraph.h
 using ADS::bfsDistances;
 using ADS::connectedComponents;

 // ADS/parallel/ParallelGreedy.h
 using ADS::filterKruskal;
 using ADS::maximalIndependentSet;
 using ADS::maximalMatching;
 using ADS::parallelKruskal;
 using ADS::randomOrder;
 using ADS::spanningForest;

 // ADS/parallel/ParallelMerge.h
 using ADS::multisequenceSplit;

 // ADS/parallel/ParallelShortestPaths.h
 using ADS::deltaStepping;
 using ADS::rhoStepping;

 // ADS/parallel/ParallelSort.h
 using ADS::kParallelSort;
 using ADS::kParallelSortThreads;
 using ADS::learnedStableSort;

 // ADS/parallel/SpeculativeFor.h
 using ADS::Reservation;
 using ADS::SpeculativeCounts;
 using ADS::SpeculativeStep;
 using ADS::speculativeFor;

 // ADS/parallel/TBBExecutor.h
#if defined( ADS_WITH_TBB )
 using ADS::TBBExecutor;
#endif

 // ADS/parallel/ThreadPool.h
 using ADS::ThreadPool;

 // ADS/priority/BucketQueue.h
 using ADS::BucketQueue;

 // ADS/priority/DaryHeap.h
 using ADS::DaryHeap;

 // ADS/priority/DialQueue.h
 using ADS::DialQueue;

 // ADS/priority/FibonacciHeapPriorityQueue.h
 using ADS::FibonacciHeapPriorityQueue;

 // ADS/priority/HeapPriorityQueue.h
 using ADS::HeapPriorityQueue;

 // ADS/priority/IndexedDaryHeap.h
 using ADS::IndexedDaryHeap;

 // ADS/priority/LinkedHeapPriorityQueue.h
 using ADS::LinkedHeapPriorityQueue;

 // ADS/priority/ListPriorityQueue.h
 using ADS::ListPriorityQueue;

 // ADS/priority/LoserTree.h
 using ADS::LoserTree;
 using ADS::kWayMerge;

 // ADS/priority/MinMaxHeap.h
 using ADS::IntervalHeap;
 using ADS::MinMaxHeap;

 // ADS/priority/MultiwayMerge.h
 using ADS::FlatLoserTree;
 using ADS::multiwayMerge;

 // ADS/priority/PairingHeap.h
 using ADS::PairingHeap;

 // ADS/priority/PriorityQueue.h
 using ADS::PriorityQueue;

 // ADS/priority/RadixHeap.h
 using ADS::RadixHeap;

 // ADS/priority/SequenceHeap.h
 using ADS::SequenceHeap;

 // ADS/priority/TreePriorityQueue.h
 using ADS::TreePriorityQueue;

 // ADS/queue/ArrayQueue.h
 using ADS::ArrayQueue;

 // ADS/queue/Dequeue.h
 using ADS::Dequeue;

 // ADS/queue/LinkedQueue.h
 using ADS::LinkedQueue;

 // ADS/queue/QueueAlgorithms.h
 using ADS::copyPositive;
 using ADS::extractPositive;

 // ADS/queue/RingDeque.h
 using ADS::RingDeque;

 // ADS/queue/UniqueQueue.h
 using ADS::Duplicate;
 using ADS::UniqueQueue;

 // ADS/search/BinarySearch.h
 using ADS::binarySearch;
 using ADS::decisionTree;
 using ADS::gallopSearch;
 using ADS::lowerBound;
 using ADS::upperBound;

 // ADS/search/CompressedPGMIndex.h
 using ADS::CompressedPGMIndex;

 // ADS/search/DynamicPGMIndex.h
 using ADS::DynamicPGMIndex;

 // ADS/search/InterpolationSearch.h
 using ADS::interpolationSearch;

 // ADS/search/KnuthSearches.h
 using ADS::fibonacciSearch;
 using ADS::uniformBinarySearch;

 // ADS/search/LocalOptima.h
 using ADS::findPeak;
 using ADS::localMinimum;

 // ADS/search/Minimum.h
 using ADS::argmin;
 using ADS::min;
 using ADS::minMax;
 using ADS::minMaxDivide;

 // ADS/search/MonotoneSearch.h
 using ADS::exponentialSearch;
 using ADS::integerSqrt;
 using ADS::isPerfectSquare;
 using ADS::unboundedSearch;

 // ADS/search/NthElement.h
 using ADS::nthElement;

 // ADS/search/PGMIndex.h
 using ADS::PGMIndex;

 // ADS/search/RangeMinimum.h
 using ADS::PlusMinusOneRangeMinimum;
 using ADS::RangeMinimum;
 using ADS::SparseTable;
 using ADS::cartesianTree;

 // ADS/search/Selection.h
 using ADS::medianOfMedians;
 using ADS::selection;

 // ADS/search/SortedSearches.h
 using ADS::closestValue;
 using ADS::kthOfTwoSorted;
 using ADS::pairWithSum;
 using ADS::rotatedMinimum;

 // ADS/search/StaticSearch.h
 using ADS::SPlusTree;
 using ADS::branchlessLowerBound;

 // ADS/search/WordRangeMinimum.h
 using ADS::WordRangeMinimum;

 // ADS/sequence/ArraySequence.h
 using ADS::ArraySequence;

 // ADS/sequence/CursorList.h
 using ADS::CursorList;
 using ADS::CursorSpace;

 // ADS/sequence/List.h
 using ADS::List;

 // ADS/sequence/ListSort.h
 using ADS::listSort;

 // ADS/sequence/Rearrangements.h
 using ADS::dutchNationalFlag;
 using ADS::minimumSwapSort;
 using ADS::sortKSorted;
 using ADS::wiggleSort;

 // ADS/sequence/SequenceAlgorithms.h
 using ADS::Order;
 using ADS::bubbleSort;
 using ADS::mergeSorted;
 using ADS::naturalMergeSort;
 using ADS::order;
 using ADS::removeDuplicates;
 using ADS::search;
 using ADS::searchOrdered;
 using ADS::sublist;

 // ADS/sequence/SequenceRange.h
 using ADS::SequenceRange;
 using ADS::range;

 // ADS/sequence/SinglyList.h
 using ADS::SinglyList;

 // ADS/sequence/SmallVector.h
 using ADS::SmallVector;

 // ADS/sequence/TieredVector.h
 using ADS::TieredVector;

 // ADS/set/BinaryFuseFilter.h
 using ADS::BinaryFuseFilter;

 // ADS/set/BlockedBloomFilter.h
 using ADS::BlockedBloomFilter;

 // ADS/set/BloomFilter.h
 using ADS::BloomFilter;

 // ADS/set/BloomVariants.h
 using ADS::PartitionedBloomFilter;
 using ADS::SpectralBloomFilter;
 using ADS::bloomBestHashes;
 using ADS::bloomBits;
 using ADS::bloomFalsePositive;
 using ADS::bloomIntersection;
 using ADS::bloomLowerBound;
 using ADS::compressedBloom;

 // ADS/set/BoolSet.h
 using ADS::BoolSet;
 using ADS::setDifference;
 using ADS::setIntersection;
 using ADS::setSymmetricDifference;
 using ADS::setUnion;

 // ADS/set/CuckooFilter.h
 using ADS::CuckooFilter;

 // ADS/set/DictionarySet.h
 using ADS::ChainedHashSet;
 using ADS::DictionarySet;
 using ADS::OpenHashSet;
 using ADS::TreeSet;

 // ADS/set/GrafiteFilter.h
 using ADS::BucketingFilter;
 using ADS::GrafiteFilter;

 // ADS/set/HashMultiset.h
 using ADS::HashMultiset;

 // ADS/set/LinkedHashSet.h
 using ADS::LinkedHashSet;

 // ADS/set/ListSet.h
 using ADS::ListSet;

 // ADS/set/PrefixFilter.h
 using ADS::PrefixFilter;

 // ADS/set/RibbonFilter.h
 using ADS::RibbonFilter;

 // ADS/set/RoaringBitmap.h
 using ADS::RoaringBitmap;

 // ADS/set/SetOperations.h
 using ADS::IterableSet;

 // ADS/set/SortedIntersection.h
 using ADS::PermutedBuckets;
 using ADS::intersectBinarySearch;
 using ADS::intersectDoubling;
 using ADS::intersectMerge;
 using ADS::intersectMergeExternal;
 using ADS::intersectMutualPartition;
 using ADS::intersectTwoLevel;
 using ADS::intersectTwoLevelExternal;
 using ADS::twoLevelHeaders;

 // ADS/set/SortedListSet.h
 using ADS::SortedListSet;
 using ADS::merge;

 // ADS/set/VectorIntersection.h
 using ADS::intersect;

 // ADS/sort/CountingSort.h
 using ADS::countingSort;

 // ADS/sort/CycleSort.h
 using ADS::cycleSort;
 using ADS::pancakeSort;

 // ADS/sort/DidacticSorts.h
 using ADS::bogoSort;
 using ADS::slowSort;
 using ADS::stoogeSort;

 // ADS/sort/EngineeredQuickSorts.h
 using ADS::dualPivotQuickSort;
 using ADS::introSort;
 using ADS::pdqSort;
 using ADS::threeWayQuickSort;

 // ADS/sort/ExchangeSorts.h
 using ADS::cocktailShakerSort;
 using ADS::combSort;
 using ADS::gnomeSort;
 using ADS::oddEvenSort;

 // ADS/sort/ExternalSort.h
 using ADS::externalSort;
 using ADS::polyphaseMerge;
 using ADS::replacementSelection;

 // ADS/sort/Heapsort.h
 using ADS::heapBuild;
 using ADS::heapsort;
 using ADS::isMaxHeap;
 using ADS::maxHeapRestore;

 // ADS/sort/InsertionSort.h
 using ADS::insertionSort;
 using ADS::insertionSortSentinel;

 // ADS/sort/KnuthSorts.h
 using ADS::addressCalculationSort;
 using ADS::binaryInsertionSort;
 using ADS::fordJohnsonBound;
 using ADS::listInsertionSort;
 using ADS::mergeInsertionSort;
 using ADS::radixExchangeSort;
 using ADS::tournamentSort;

 // ADS/sort/LearnedSort.h
 using ADS::learnedSort;

 // ADS/sort/MergeSort.h
 using ADS::countInversions;
 using ADS::mergeParts;
 using ADS::mergeSort;
 using ADS::mergeSortBottomUp;
 using ADS::mergeSortRec;

 // ADS/sort/MultikeyQuickSort.h
 using ADS::multikeyQuickSort;

 // ADS/sort/PatienceSort.h
 using ADS::patienceSort;

 // ADS/sort/PigeonholeSort.h
 using ADS::pigeonholeSort;

 // ADS/sort/QuickSort.h
 using ADS::partitionByValue;
 using ADS::quickSort;
 using ADS::quickSortTuned;

 // ADS/sort/RadixSort.h
 using ADS::RadixKey;
 using ADS::americanFlagSort;
 using ADS::bucketSort;
 using ADS::msdRadixSort;
 using ADS::radixKey;
 using ADS::radixSort;
 using ADS::radixSortBy;

 // ADS/sort/RunSorts.h
 using ADS::powerSort;
 using ADS::timSort;

 // ADS/sort/SampleSort.h
 using ADS::sampleSort;

 // ADS/sort/SelectionSort.h
 using ADS::selectionSort;

 // ADS/sort/ShellSort.h
 using ADS::ciuraGaps;
 using ADS::hibbardGaps;
 using ADS::knuthGaps;
 using ADS::prattGaps;
 using ADS::shellGaps;
 using ADS::shellSort;

 // ADS/sort/Smoothsort.h
 using ADS::smoothsort;

 // ADS/sort/Sort.h
 using ADS::kRadix;
 using ADS::kRuns;
 using ADS::kStrings;
 using ADS::sort;
 using ADS::stableSort;

 // ADS/sort/Sorted.h
 using ADS::checkingContract;

 // ADS/sort/SortingNetworks.h
 using ADS::Comparator;
 using ADS::applyNetwork;
 using ADS::bitonicNetwork;
 using ADS::mergeExchangeNetwork;

 // ADS/sort/StrandSort.h
 using ADS::strandSort;

 // ADS/sort/StringSort.h
 using ADS::prefixKeySort;

 // ADS/sort/TreeSort.h
 using ADS::treeSort;

 // ADS/sort/VectorQuickSort.h
 using ADS::VectorKey;
 using ADS::kVectorSort;
 using ADS::vectorQuickSort;

 // ADS/stack/ArrayStack.h
 using ADS::ArrayStack;

 // ADS/stack/Expressions.h
 using ADS::evalPostfix;
 using ADS::toPostfix;
 using ADS::toPostfixParenthesized;

 // ADS/stack/MultiStack.h
 using ADS::MultiStack;

 // ADS/stack/SequenceStack.h
 using ADS::SequenceStack;

 // ADS/stack/StackAlgorithms.h
 using ADS::forEachBottomUp;
 using ADS::isPalindrome;
 using ADS::removeGreater;
 using ADS::sortStack;

 // ADS/string/AhoCorasick.h
 using ADS::AhoCorasick;

 // ADS/string/Alignment.h
 using ADS::Alignment;
 using ADS::alignmentScore;
 using ADS::globalAlignment;

 // ADS/string/ApproximateMatching.h
 using ADS::Table;
 using ADS::editDistance;
 using ADS::matchingTable;
 using ADS::occurrence;
 using ADS::stringMatching;
 using ADS::stringMatchingSingleRow;

 // ADS/string/EditDistance.h
 using ADS::approximateMatches;
 using ADS::lcsLength;
 using ADS::levenshtein;

 // ADS/string/FrontCoding.h
 using ADS::FrontCodedDictionary;
 using ADS::FrontPair;
 using ADS::decodeLength;
 using ADS::encodeLength;
 using ADS::frontCode;
 using ADS::frontDecode;

 // ADS/string/Huffman.h
 using ADS::decode;
 using ADS::encode;
 using ADS::huffman;

 // ADS/string/LongestCommonExtension.h
 using ADS::LceIndex;

 // ADS/string/LongestCommonSubsequence.h
 using ADS::diff;
 using ADS::lcs;
 using ADS::lcsMemo;
 using ADS::lcsTable;
 using ADS::longestCommonSubstring;

 // ADS/string/Lyndon.h
 using ADS::isLyndon;
 using ADS::leastRotation;
 using ADS::lyndonFactorization;

 // ADS/string/MultiSearch.h
 using ADS::MultiSearcher;

 // ADS/string/Palindromes.h
 using ADS::Eertree;
 using ADS::longestPalindrome;
 using ADS::longestPalindromicSubsequence;
 using ADS::longestPalindromicSubstring;
 using ADS::palindromicFactorization;
 using ADS::palindromicPartition;

 // ADS/string/Parentheses.h
 using ADS::longestBalancedSubsequence;
 using ADS::longestBalancedSubstring;

 // ADS/string/PrefixSearch.h
 using ADS::BinnedSearch;
 using ADS::BinnedStringSearch;
 using ADS::commonPrefixLength;
 using ADS::lexicographicPosition;
 using ADS::prefixEnd;
 using ADS::prefixRange;

 // ADS/string/RearCoding.h
 using ADS::RearCodedDictionary;
 using ADS::RearPair;
 using ADS::rearCode;
 using ADS::rearDecode;

 // ADS/string/Segmentations.h
 using ADS::countDecodings;
 using ADS::countSegmentations;

 // ADS/string/StringBreaking.h
 using ADS::stringBreakingCost;
 using ADS::stringBreakingOrder;

 // ADS/string/StringMatching.h
 using ADS::boyerMoore;
 using ADS::failure;
 using ADS::horspool;
 using ADS::kmp;
 using ADS::kmpStrong;
 using ADS::naiveMatching;
 using ADS::shiftOr;
 using ADS::strongFailure;
 using ADS::twoWay;
 using ADS::zFunction;
 using ADS::zMatching;

 // ADS/string/StringSearch.h
 using ADS::StringSearcher;
 using ADS::findAll;
 using ADS::findFirst;

 // ADS/string/SubsequenceProblems.h
 using ADS::isInterleaving;
 using ADS::lcs3;
 using ADS::scsLength;
 using ADS::shortestCommonSupersequence;
 using ADS::subsequenceOccurrences;

 // ADS/string/SuffixApplications.h
 using ADS::CopyPhrase;
 using ADS::decodeCopyPhrases;
 using ADS::distinctSubstrings;
 using ADS::kMismatches;
 using ADS::longestRepeatedSubstring;
 using ADS::lz77ByTree;
 using ADS::maximalRepeats;
 using ADS::proximity;
 using ADS::repeatedSubstring;

 // ADS/string/SuffixArray.h
 using ADS::SuffixArray;
 using ADS::Symbols;
 using ADS::inversePermutation;
 using ADS::lcpArray;
 using ADS::suffixArrayDoubling;
 using ADS::suffixArrayLarssonSadakane;
 using ADS::suffixArraySais;
 using ADS::suffixArrayScan;
 using ADS::suffixArraySkew;
 using ADS::suffixArraySorting;
 using ADS::toSymbols;

 // ADS/string/SuffixAutomaton.h
 using ADS::SuffixAutomaton;

 // ADS/string/SuffixSorting.h
 using ADS::lcpArrayPhi;
 using ADS::plcpArray;
 using ADS::suffixArray;

 // ADS/string/SuffixTree.h
 using ADS::SuffixTree;

 // ADS/string/WordBreak.h
 using ADS::wordBreak;

 // ADS/succinct/BPTree.h
 using ADS::BPTree;

 // ADS/succinct/BVGraph.h
 using ADS::BVGraph;

 // ADS/succinct/BalancedParentheses.h
 using ADS::BalancedParentheses;

 // ADS/succinct/BitStream.h
 using ADS::BitReader;
 using ADS::BitWriter;

 // ADS/succinct/BitVector.h
 using ADS::BitVector;
 using ADS::SubscriptIterator;
 using ADS::Words;
 using ADS::selectInWord;

 // ADS/succinct/CompactRankSelect.h
 using ADS::CompactRankSelect;

 // ADS/succinct/EliasFano.h
 using ADS::BasicEliasFano;
 using ADS::EliasFano;
 using ADS::LazyEliasFano;

 // ADS/succinct/EliasFanoGraph.h
 using ADS::EliasFanoGraph;

 // ADS/succinct/HuffmanWaveletTree.h
 using ADS::HuffmanWaveletTree;

 // ADS/succinct/InterleavedRankSelect.h
 using ADS::InterleavedRankSelect;

 // ADS/succinct/JacobsonTree.h
 using ADS::JacobsonTree;

 // ADS/succinct/K2Tree.h
 using ADS::K2Tree;

 // ADS/succinct/LAVector.h
 using ADS::LAVector;

 // ADS/succinct/LoudsTree.h
 using ADS::LoudsTree;

 // ADS/succinct/MoveStructure.h
 using ADS::MoveStructure;

 // ADS/succinct/PartitionedEliasFano.h
 using ADS::PartitionedEliasFano;

 // ADS/succinct/RRRBitVector.h
 using ADS::RRRBitVector;

 // ADS/succinct/RankSelect.h
 using ADS::BasicRankSelect;
 using ADS::LazyRankSelect;
 using ADS::RankSelect;
 using ADS::RankSelectOptions;

 // ADS/succinct/WaveletMatrix.h
 using ADS::WaveletMatrix;

 // ADS/tree/AVLTree.h
 using ADS::AVLTree;
 using ADS::AVLTreeNodes;

 // ADS/tree/ArrayBinaryTree.h
 using ADS::ArrayBinaryTree;

 // ADS/tree/BPlusTree.h
 using ADS::BPlusTree;

 // ADS/tree/BTree.h
 using ADS::BTree;

 // ADS/tree/BTreeMap.h
 using ADS::BTreeMap;

 // ADS/tree/BinaryTree.h
 using ADS::BinaryTree;
 using ADS::dary_child;
 using ADS::dary_parent;
 using ADS::from_list;
 using ADS::inorder;
 using ADS::join;
 using ADS::postorder;
 using ADS::to_binary;

 // ADS/tree/BinaryTreeAlgorithms.h
 using ADS::BinaryNode;
 using ADS::KeyedBinaryNode;
 using ADS::ParentBinaryNode;
 using ADS::cartesianBinaryTree;
 using ADS::equalTrees;
 using ADS::fromPreorderInorder;
 using ADS::isRedBlackTree;
 using ADS::isSearchTree;
 using ADS::lowestCommonAncestor;
 using ADS::minimumHeightSearchTree;
 using ADS::searchTreeRange;

 // ADS/tree/ChildListTree.h
 using ADS::ChildListTree;

 // ADS/tree/CursorBinaryTree.h
 using ADS::CursorBinaryTree;

 // ADS/tree/CursorTree.h
 using ADS::CursorTree;

 // ADS/tree/ExpressionTree.h
 using ADS::evaluate;
 using ADS::expressionTree;
 using ADS::infixForm;
 using ADS::prefixForm;

 // ADS/tree/HeapByInsertion.h
 using ADS::heapBuildByInsertion;
 using ADS::maxHeapSiftUp;

 // ADS/tree/JoinAlgorithms.h
 using ADS::AVLJoin;
 using ADS::JoinAdaptor;
 using ADS::JoinAlgorithms;
 using ADS::Joinable;
 using ADS::RedBlackJoin;
 using ADS::TakeFirst;
 using ADS::TakeSecond;
 using ADS::TreapJoin;
 using ADS::WeightBalancedJoin;
 using ADS::kJoinGrain;

 // ADS/tree/JoinTree.h
 using ADS::AVLBalance;
 using ADS::Augmentation;
 using ADS::AugmentedMap;
 using ADS::JoinAVLTree;
 using ADS::JoinRedBlackTree;
 using ADS::JoinTreap;
 using ADS::JoinTree;
 using ADS::JoinTreeNodes;
 using ADS::MaxAugmentation;
 using ADS::NoAugmentation;
 using ADS::RedBlackBalance;
 using ADS::SumAugmentation;
 using ADS::TreapBalance;
 using ADS::WeightBalance;
 using ADS::WeightBalancedTree;

 // ADS/tree/LinkCutTree.h
 using ADS::LinkCutTree;

 // ADS/tree/ParentTree.h
 using ADS::ParentTree;

 // ADS/tree/PreorderArrayTree.h
 using ADS::PreorderArrayTree;

 // ADS/tree/SearchTree.h
 using ADS::BinarySearchTree;
 using ADS::Color;
 using ADS::NotJoinable;
 using ADS::RedBlackTree;
 using ADS::SearchTree;
 using ADS::SearchTreeNodes;

 // ADS/tree/SplayTree.h
 using ADS::SplayTree;

 // ADS/tree/ThreadedTree.h
 using ADS::ThreadedTree;

 // ADS/tree/Treap.h
 using ADS::Treap;
 using ADS::TreapNodes;

 // ADS/tree/Tree.h
 using ADS::Tree;
 using ADS::children;
 using ADS::copy;
 using ADS::in_visit;
 using ADS::maxWeightLevel;
 using ADS::post_visit;
 using ADS::pre_visit;
 using ADS::preorder;
 using ADS::to_list;
 using ADS::tree_from_list;
}

export namespace ADS {
 namespace distributed {

  // ADS/distributed/DistributedSort.h
#if defined( ADS_WITH_MPI )
  using ADS::distributed::sort;
#endif
 }
}

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ADS.cppm ----------------------------*/
/*--------------------------------------------------------------------------*/

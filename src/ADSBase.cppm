/*--------------------------------------------------------------------------*/
/*--------------------------- File ADSBase.cppm ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Interface of the C++20 module ads.base, which exports the names of
 * ads.core, without importing it, and those that the headers of the base set
 * of ADS [see ADS/base.h] declare in the namespace ADS and in those nested in
 * it, but for the namespaces detail; a translation unit that writes import
 * ads.base; compiles neither the advanced headers but those that the base
 * ones use inside, whose names are not exported, nor the executors and the
 * headers of MPI.
 *
 * This file is written by cmake/ads_module_exports.py, which reads the names
 * from the AST of the headers and their sets from cmake/ads_sets.txt: it is
 * not to be edited by hand.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------- GLOBAL MODULE FRAGMENT -------------------------*/
/*--------------------------------------------------------------------------*/

module;

#include "ADS/base.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- MODULE PURVIEW -----------------------------*/
/*--------------------------------------------------------------------------*/

export module ads.base;

export namespace ADS
{

// ADT.h
using ADS::Dictionary;
using ADS::Queue;
using ADS::Sequence;
using ADS::Set;
using ADS::Stack;

// Capabilities.h
using ADS::addressable;
using ADS::augmented_by;
using ADS::batch_extract;
using ADS::batch_updatable;
using ADS::bulk_erasable;
using ADS::bulk_insertable;
using ADS::complexity;
using ADS::declares_monotone;
using ADS::decrease_key_kind;
using ADS::decrease_key_of;
using ADS::enable_stable_references;
using ADS::epoch_clearable;
using ADS::erasable;
using ADS::event_set;
using ADS::event_set_t;
using ADS::forwarding_query;
using ADS::forwarding_query_tag;
using ADS::in_arcs;
using ADS::insert_bulk;
using ADS::joinable;
using ADS::key_type_t;
using ADS::meetable_query;
using ADS::modifications_of;
using ADS::monotone_extract;
using ADS::monotone_of;
using ADS::nonmonotone_values;
using ADS::observes_values;
using ADS::prefetch;
using ADS::prefetchable;
using ADS::priority_t;
using ADS::query;
using ADS::query_meet_v;
using ADS::query_v;
using ADS::range_actionable;
using ADS::rank_complexity;
using ADS::rank_complexity_t;
using ADS::rankable;
using ADS::rekeyable;
using ADS::reservable;
using ADS::reserve;
using ADS::selectable;
using ADS::shiftable_keys;
using ADS::snapshotable;
using ADS::splittable;
using ADS::stable_edge_ids;
using ADS::stable_references;
using ADS::stable_references_t;
using ADS::thread_safe_reads;
using ADS::thread_safe_reads_t;
using ADS::undoable;

// Contract.h
using ADS::contract_violation;

// Iterators.h
using ADS::CircularIterator;
using ADS::IndexIterator;
using ADS::IndirectIterator;
using ADS::KeyIterator;
using ADS::LinkedIterator;
using ADS::PairRef;
using ADS::PathIterator;
using ADS::RankIterator;
using ADS::SlotIterator;
using ADS::WalkIterator;

// algorithm/AkraBazzi.h
using ADS::akraBazzi;
using ADS::akraBazziExponent;

// algorithm/BinaryCounter.h
using ADS::add;

// algorithm/Hanoi.h
using ADS::hanoi_iterative;
using ADS::hanoi_iterative_labels;
using ADS::hanoi_recursive;

// algorithm/MasterTheorem.h
using ADS::Growth;
using ADS::linear;
using ADS::master;

// algorithm/MaximumSegments.h
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

// algorithm/MonteCarlo.h
using ADS::freivalds;
using ADS::halfSatisfiable;
using ADS::zeroPolynomial;

// algorithm/Paradigms.h
using ADS::backtrack;
using ADS::divideEtImpera;
using ADS::enumerateOptimize;
using ADS::enumerateSearch;
using ADS::greedy;
using ADS::maximal;

// algorithm/SequenceQueries.h
using ADS::largestGap;
using ADS::lineAssignment;
using ADS::longestKDistinctRun;
using ADS::smallerToTheRight;

// algorithm/Subsequences.h
using ADS::lis;
using ADS::lisLength;
using ADS::maxSubarraySum;

// backtrack/Anagrams.h
using ADS::anagrams;

// backtrack/DancingLinks.h
using ADS::ExactCover;

// backtrack/Enumeration.h
using ADS::derangements;
using ADS::permutations;
using ADS::subsetSum;
using ADS::subsets;

// backtrack/KnightTour.h
using ADS::knightTour;

// backtrack/NQueens.h
using ADS::admissible;
using ADS::board;
using ADS::nQueens;
using ADS::nQueensIterative;

// backtrack/SatSolvers.h
using ADS::BasicCdcl;
using ADS::Cdcl;
using ADS::dpll;

// backtrack/Sudoku.h
using ADS::Grid;
using ADS::check;
using ADS::sudoku;

// combinatorial/Bitwise.h
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

// combinatorial/Generation.h
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

// complexity/Domino.h
using ADS::Tile;
using ADS::Tiling;
using ADS::legalTiling;
using ADS::linearTiling;
using ADS::squareTiling;

// complexity/EasyCases.h
using ADS::bipartiteMatching;
using ADS::intervalClique;
using ADS::intervalGraph;
using ADS::literalNode;
using ADS::twoColoring;
using ADS::twoSat;

// complexity/Nondeterminism.h
using ADS::Chooser;

// complexity/Problems.h
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

// complexity/Reductions.h
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

// compression/ArithmeticCoding.h
using ADS::RangeDecoder;
using ADS::RangeEncoder;
using ADS::adaptiveArithmeticDecode;
using ADS::adaptiveArithmeticEncode;
using ADS::arithmeticCodeExact;
using ADS::arithmeticDecode;
using ADS::arithmeticDecodeExact;
using ADS::arithmeticEncode;
using ADS::arithmeticInterval;

// compression/BurrowsWheeler.h
using ADS::BWT;
using ADS::bwt;
using ADS::bwtWideFrom;
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

// compression/CanonicalHuffman.h
using ADS::CanonicalHuffman;
using ADS::huffmanLengths;
using ADS::limitedLengths;

// compression/Entropy.h
using ADS::entropy;

// compression/FMIndex.h
using ADS::FMIndex;

// compression/IntegerCodes.h
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

// compression/LempelZiv.h
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

// compression/PPM.h
using ADS::PPMModel;
using ADS::ppmDecode;
using ADS::ppmEncode;

// compression/PackedBits.h
using ADS::PackedBitReader;
using ADS::PackedBitWriter;

// dictionary/BinaryTrie.h
using ADS::BinaryTrie;
using ADS::SimpleBinaryTrie;
using ADS::TrieWidth;

// dictionary/BitmapTrie.h
using ADS::BitmapTrie;

// dictionary/ChainedHashTable.h
using ADS::Chain;
using ADS::ChainedHashTable;

// dictionary/CuckooHashing.h
using ADS::CuckooHashTable;

// dictionary/DirectTable.h
using ADS::DirectTable;

// dictionary/HashFunctions.h
using ADS::bin_bit;
using ADS::hash_division;
using ADS::hash_extraction;
using ADS::hash_fibonacci;
using ADS::hash_midsquare;
using ADS::hash_multiplication;
using ADS::hash_xor;
using ADS::int_bin;

// dictionary/IndexedSkipList.h
using ADS::IndexedSkipList;

// dictionary/KnuthHashing.h
using ADS::BrentHashTable;
using ADS::CoalescedHashTable;

// dictionary/OpenHashTable.h
using ADS::OpenHashTable;
using ADS::Probing;

// dictionary/PatriciaTrie.h
using ADS::PatriciaTrie;

// dictionary/PerfectHashing.h
using ADS::OrderedMinimalPerfectHash;
using ADS::PerfectHashTable;

// dictionary/RandomHashing.h
using ADS::SplitMix64;
using ADS::TabulationHash;
using ADS::ZobristTable;
using ADS::fibonacciPlace;
using ADS::splitmix64;

// dictionary/SkipList.h
using ADS::SkipList;

// dictionary/SortedArrayDictionary.h
using ADS::SortedArrayDictionary;

// dictionary/TableSize.h
using ADS::PlaceHash;
using ADS::is_pow2;
using ADS::is_prime;
using ADS::next_prime;
using ADS::resized;

// dictionary/Trie.h
using ADS::RadixTree;
using ADS::Trie;

// dictionary/TwoLevelDictionary.h
using ADS::TwoLevelDictionary;

// dictionary/UniversalHashing.h
using ADS::CarterWegmanHash;
using ADS::CompoundHash;
using ADS::DLeftHashTable;
using ADS::DotProductHash;
using ADS::MultiplicativeHash;
using ADS::MultiplyShiftHash;
using ADS::SequenceHash;
using ADS::StringHash;

// dictionary/XFastTrie.h
using ADS::ListXFastTrie;
using ADS::XFastTrie;

// dictionary/YFastTrie.h
using ADS::YFastTrie;

// external/BlockBTree.h
using ADS::BTreePolicy;
using ADS::BlockBTree;

// external/BlockStore.h
using ADS::BlockStore;

// external/ExternalSorting.h
using ADS::binaryMergeSort;
using ADS::distributionSort;
using ADS::mergeSortIOs;
using ADS::multiwayMergeSort;
using ADS::permuteBySorting;
using ADS::permuteDirect;
using ADS::permutingLowerBound;
using ADS::sortingLowerBound;

// external/ListRanking.h
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

// external/MappedFile.h
using ADS::MappableAllocator;
using ADS::MappedFile;
using ADS::PersistError;

// external/Persistence.h
using ADS::ByteOrder;
using ADS::PersistReader;
using ADS::PersistWriter;
using ADS::Persistent;
using ADS::restore;
using ADS::restoreMapped;
using ADS::store;

// external/RecordFile.h
using ADS::RecordFile;
using ADS::RecordWriter;
using ADS::TempDirectory;

// external/TwoLevelMemory.h
using ADS::DiskFile;
using ADS::DiskReader;
using ADS::DiskWriter;
using ADS::MemoryLease;
using ADS::TwoLevelMemory;

// geometry/ClosestPair.h
using ADS::closestPair;
using ADS::closestPairBrute;
using ADS::closestPairSweep;
using ADS::squaredDistance;

// geometry/ConvexHull.h
using ADS::Point;
using ADS::convexHull;
using ADS::convexHullBrute;
using ADS::cross;
using ADS::graham;
using ADS::pseudoAngle;
using ADS::sameSide;

// graph/AllPairsShortestPaths.h
using ADS::AllPairs;
using ADS::addEdge;
using ADS::floydWarshall;

// graph/ArrayGraph.h
using ADS::ArrayGraph;

// graph/EulerianCycle.h
using ADS::eulerianCycle;
using ADS::eulerianTrail;
using ADS::isEulerianCycle;
using ADS::isEulerianTrail;

// graph/FlowApplications.h
using ADS::CapacityDecrease;
using ADS::decreaseCapacity;
using ADS::edgeDisjointPaths;

// graph/Graph.h
using ADS::DynamicGraph;
using ADS::Graph;
using ADS::GraphNodes;
using ADS::adjFromIncidence;
using ADS::incidence;

// graph/Grid.h
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

// graph/IncidenceGraph.h
using ADS::IncidenceGraph;

// graph/ListGraph.h
using ADS::ListGraph;

// graph/Matching.h
using ADS::bipartiteBMatching;
using ADS::branchAndBoundMatching;

// graph/MatrixGraph.h
using ADS::MatrixGraph;

// graph/MaxFlow.h
using ADS::Augment;
using ADS::BoundedEdge;
using ADS::FlowNetwork;
using ADS::feasibleCirculation;

// graph/MinimumSpanningTree.h
using ADS::MergeCounts;
using ADS::Mergeable;
using ADS::WeightedEdge;
using ADS::edges;
using ADS::isMinimumSpanningForest;
using ADS::kruskal;
using ADS::prim;

// graph/Paths.h
using ADS::PathLength;
using ADS::countShortestPaths;
using ADS::dagLongestPaths;
using ADS::longestPathLayering;
using ADS::multiSourceDistance;
using ADS::refuelingStops;
using ADS::setDistance;
using ADS::simpleCycles;

// graph/Properties.h
using ADS::countTriangles;
using ADS::diameter;
using ADS::isTree;
using ADS::treeDiameter;
using ADS::treeVertexCover;
using ADS::uniqueTopologicalOrder;
using ADS::universalSink;

// graph/ShortestPaths.h
using ADS::DequeFrontier;
using ADS::Frontier;
using ADS::FrontierCounts;
using ADS::FrontierQueue;
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

// graph/Visit.h
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

// intractable/Approximation.h
using ADS::apSubsetSum;
using ADS::deltaTsp;
using ADS::firstFit;
using ADS::knapsackApprox;
using ADS::tourCost;
using ADS::tspGap;
using ADS::vertexCoverApprox;

// intractable/BranchAndBound.h
using ADS::IncumbentChanged;
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

// intractable/FifteenPuzzle.h
using ADS::Board;
using ADS::manhattan;
using ADS::puzzleBFS;
using ADS::puzzleDFS;
using ADS::puzzleGoal;
using ADS::puzzleMoves;
using ADS::puzzleSpiralGoal;
using ADS::solvable;
using ADS::solvePuzzle;

// intractable/Heuristics.h
using ADS::Buckets;
using ADS::cutSize;
using ADS::dsatur;
using ADS::greedySuperstring;
using ADS::greedyTsp;
using ADS::kOptNeighbors;
using ADS::kOptNeighbours;
using ADS::minMaxGreedy;
using ADS::overlap;
using ADS::partitionLocalSearch;
using ADS::starAlignment;
using ADS::twoOpt;
using ADS::twoOptMove;

// intractable/Pseudopolynomial.h
using ADS::Piece;
using ADS::bottino;
using ADS::mcNaughton;
using ADS::partitionDP;
using ADS::realTimeScheduling;
using ADS::schedulingDP;
using ADS::subsetSumDP;
using ADS::subsetSumSet;

// memory/Allocators.h
using ADS::BoundaryTagAllocator;
using ADS::BuddyAllocator;

// memory/CellHeap.h
using ADS::CellHeap;

// memory/HugePageAllocator.h
using ADS::HugePageAllocator;
using ADS::ZeroPageAllocator;

// memory/ObjectPool.h
using ADS::ObjectPool;

// memory/Prefetch.h
using ADS::Locality;

// mfset/ListMFSet.h
using ADS::ListMFSet;

// mfset/MFSet.h
using ADS::MFSet;
using ADS::components;

// numeric/BigInteger.h
using ADS::BigInteger;
using ADS::operator%;
using ADS::operator*;
using ADS::operator+;
using ADS::operator-;
using ADS::operator/;
using ADS::operator<=>;

// numeric/BinaryInteger.h
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

// numeric/Catalan.h
using ADS::catalan;

// numeric/Combinatorics.h
using ADS::FibonacciCache;
using ADS::binomial;
using ADS::binomialByFactorials;
using ADS::factorial;
using ADS::factorialRec;
using ADS::fibonacci;
using ADS::fibonacciTable;
using ADS::tartaglia;

// numeric/Matrix.h
using ADS::Matrix;
using ADS::multiply;
using ADS::multiplyNaive;
using ADS::scale;
using ADS::strassen;

// numeric/MatrixChain.h
using ADS::matrixChain;
using ADS::parenthesization;

// numeric/NumberTheory.h
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

// numeric/Polynomial.h
using ADS::multiplyFour;
using ADS::multiplyKaratsuba;

// numeric/RSA.h
using ADS::RsaKey;
using ADS::randomPrime;
using ADS::rsa;
using ADS::rsaKeys;

// numeric/RandomNumbers.h
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

// numeric/RandomSampling.h
using ADS::heapSampling;
using ADS::reservoirSampleL;
using ADS::sampleByDictionary;
using ADS::sampleBySkips;
using ADS::sampleBySorting;
using ADS::sampleBySwaps;

// numeric/Rational.h
using ADS::Rational;

// numeric/SparseMatrix.h
using ADS::SparseMatrix;

// optimization/CoinChange.h
using ADS::greedyChange;
using ADS::optimalChange;

// optimization/CoinGame.h
using ADS::coinsInALine;

// optimization/FractionalKnapsack.h
using ADS::fractionalKnapsack;

// optimization/GridPaths.h
using ADS::GridSquare;
using ADS::countLatticePaths;
using ADS::largestSquare;

// optimization/HouseProblems.h
using ADS::maxNonAdjacentSum;
using ADS::nonAdjacentSelection;
using ADS::paintHouses;

// optimization/IntervalCover.h
using ADS::minimumIntervalCover;

// optimization/Intervals.h
using ADS::intervalColoring;
using ADS::towers;

// optimization/Knapsack.h
using ADS::knapsack;
using ADS::knapsackMemo;
using ADS::zaino;

// optimization/KnapsackVariants.h
using ADS::coinChangeWays;
using ADS::minValueExactWeight;
using ADS::minimumSquares;
using ADS::rodCutting;
using ADS::subsetSumCount;
using ADS::unboundedKnapsack;

// optimization/LinearPartition.h
using ADS::linearPartition;
using ADS::painterPartition;

// optimization/OptimalSearchTree.h
using ADS::OptimalSearchTree;
using ADS::optimalSearchTree;

// optimization/SequenceProblems.h
using ADS::largestDivisibleSubset;
using ADS::longestChainSubsequence;
using ADS::longestDistinctSegment;
using ADS::maxSumIncreasingSubsequence;
using ADS::trappingRainWater;

// optimization/ShortestJobFirst.h
using ADS::shortestJobFirst;
using ADS::smithOrder;
using ADS::totalCompletion;
using ADS::weightedCompletion;

// optimization/StockTrading.h
using ADS::maxProfit;
using ADS::maxProfitExactly;

// optimization/WeightedIntervals.h
using ADS::WeightedInterval;
using ADS::maxinterval;

// parallel/Executor.h
using ADS::Executor;
using ADS::Sequential;
using ADS::WorkerExecutor;
using ADS::blocksOf;
using ADS::isRunCaller;
using ADS::kBlocks;
using ADS::kGrain;
using ADS::parallelExclusiveScan;
using ADS::parallelFor;
using ADS::parallelForBlocks;
using ADS::parallelInclusiveScan;
using ADS::parallelInvoke;
using ADS::parallelReduce;

// priority/FibonacciHeapPriorityQueue.h
using ADS::FibonacciHeapPriorityQueue;

// priority/HeapPriorityQueue.h
using ADS::HeapPriorityQueue;

// priority/ListPriorityQueue.h
using ADS::ListPriorityQueue;

// priority/LoserTree.h
using ADS::LoserTree;
using ADS::kWayMerge;

// priority/MeldableHeap.h
using ADS::MeldableHeap;

// priority/PriorityQueue.h
using ADS::AddressablePriorityQueue;
using ADS::PriorityQueue;

// priority/TreePriorityQueue.h
using ADS::TreePriorityQueue;

// queue/ArrayQueue.h
using ADS::ArrayQueue;

// queue/Dequeue.h
using ADS::Dequeue;

// queue/LinkedQueue.h
using ADS::LinkedQueue;

// queue/MinDeque.h
using ADS::MinDeque;

// queue/MinQueue.h
using ADS::MinQueue;

// queue/QueueAlgorithms.h
using ADS::copyPositive;
using ADS::extractPositive;

// queue/RandomQueue.h
using ADS::RandomQueue;

// queue/RealTimeMinDeque.h
using ADS::RealTimeMinDeque;

// queue/RealTimeMinQueue.h
using ADS::RealTimeMinQueue;

// queue/UniqueQueue.h
using ADS::Duplicate;
using ADS::UniqueQueue;

// search/BinarySearch.h
using ADS::binarySearch;
using ADS::decisionTree;
using ADS::gallopSearch;
using ADS::lowerBound;
using ADS::upperBound;

// search/InterpolationSearch.h
using ADS::interpolationSearch;

// search/KnuthSearches.h
using ADS::fibonacciSearch;
using ADS::uniformBinarySearch;

// search/LocalOptima.h
using ADS::findPeak;
using ADS::localMinimum;

// search/Minimum.h
using ADS::argmin;
using ADS::min;
using ADS::minMax;
using ADS::minMaxDivide;

// search/MonotoneSearch.h
using ADS::exponentialSearch;
using ADS::integerSqrt;
using ADS::isPerfectSquare;
using ADS::unboundedSearch;

// search/PGMIndex.h
using ADS::PGMIndex;

// search/Selection.h
using ADS::medianOfMedians;
using ADS::selection;

// search/SortedSearches.h
using ADS::closestValue;
using ADS::kthOfTwoSorted;
using ADS::pairWithSum;
using ADS::rotatedMinimum;

// sequence/ArraySequence.h
using ADS::ArraySequence;

// sequence/ContiguousDeque.h
using ADS::ContiguousDeque;

// sequence/CubishArrayStack.h
using ADS::CubishArrayStack;

// sequence/CursorList.h
using ADS::CursorList;
using ADS::CursorSpace;

// sequence/DualArrayDeque.h
using ADS::DualArrayDeque;

// sequence/IndexedList.h
using ADS::IndexedDeque;
using ADS::IndexedList;
using ADS::rotate;
using ADS::rotateArray;

// sequence/List.h
using ADS::List;

// sequence/OrderMaintenanceList.h
using ADS::OrderMaintenanceList;

// sequence/Rearrangements.h
using ADS::dutchNationalFlag;
using ADS::minimumSwapSort;
using ADS::sortKSorted;
using ADS::wiggleSort;

// sequence/RootishArrayStack.h
using ADS::RootishArrayStack;

// sequence/SEList.h
using ADS::SEList;

// sequence/SequenceAlgorithms.h
using ADS::Order;
using ADS::bubbleSort;
using ADS::mergeSorted;
using ADS::naturalMergeSort;
using ADS::order;
using ADS::removeDuplicates;
using ADS::search;
using ADS::searchOrdered;
using ADS::sublist;

// sequence/SequenceRange.h
using ADS::SequenceRange;
using ADS::range;

// sequence/SinglyList.h
using ADS::SinglyList;

// sequence/SkipListSequence.h
using ADS::SkipListSequence;

// sequence/TreapList.h
using ADS::TreapList;

// sequence/Treque.h
using ADS::Treque;

// sequence/XorList.h
using ADS::XorList;
using ADS::xorSwap;

// set/Bag.h
using ADS::Bag;

// set/BloomFilter.h
using ADS::BloomFilter;

// set/BloomVariants.h
using ADS::PartitionedBloomFilter;
using ADS::SpectralBloomFilter;
using ADS::bloomBestHashes;
using ADS::bloomBits;
using ADS::bloomFalsePositive;
using ADS::bloomIntersection;
using ADS::bloomLowerBound;
using ADS::compressedBloom;

// set/BoolSet.h
using ADS::BoolSet;
using ADS::setDifference;
using ADS::setIntersection;
using ADS::setSymmetricDifference;
using ADS::setUnion;

// set/DictionarySet.h
using ADS::ChainedHashSet;
using ADS::DictionarySet;
using ADS::OpenHashSet;
using ADS::TreeSet;

// set/HashMultiset.h
using ADS::HashMultiset;

// set/LinkedHashSet.h
using ADS::LinkedHashSet;

// set/ListSet.h
using ADS::ListSet;

// set/SetInterfaces.h
using ADS::ArraySSet;
using ADS::ArrayUSet;
using ADS::KeyedDictionary;
using ADS::OrderedDictionary;
using ADS::SSet;
using ADS::USet;

// set/SetOperations.h
using ADS::IterableSet;

// set/SortedIntersection.h
using ADS::PermutedBuckets;
using ADS::intersectBinarySearch;
using ADS::intersectDoubling;
using ADS::intersectMerge;
using ADS::intersectMergeExternal;
using ADS::intersectMutualPartition;
using ADS::intersectTwoLevel;
using ADS::intersectTwoLevelExternal;
using ADS::twoLevelHeaders;

// set/SortedListSet.h
using ADS::SortedListSet;
using ADS::merge;

// set/SpaceEfficientSortedSet.h
using ADS::SpaceEfficientSortedSet;

// sort/CountingSort.h
using ADS::countingSort;

// sort/EngineeredQuickSorts.h
using ADS::dualPivotQuickSort;
using ADS::threeWayQuickSort;

// sort/ExchangeSorts.h
using ADS::cocktailShakerSort;
using ADS::combSort;
using ADS::gnomeSort;
using ADS::oddEvenSort;

// sort/ExternalSort.h
using ADS::externalSort;
using ADS::polyphaseMerge;
using ADS::replacementSelection;

// sort/Heapsort.h
using ADS::heapBuild;
using ADS::heapsort;
using ADS::isMaxHeap;
using ADS::maxHeapRestore;

// sort/InsertionSort.h
using ADS::insertionSort;
using ADS::insertionSortSentinel;

// sort/KnuthSorts.h
using ADS::addressCalculationSort;
using ADS::binaryInsertionSort;
using ADS::fordJohnsonBound;
using ADS::listInsertionSort;
using ADS::mergeInsertionSort;
using ADS::radixExchangeSort;
using ADS::tournamentSort;

// sort/MergeSort.h
using ADS::countInversions;
using ADS::mergeParts;
using ADS::mergeSort;
using ADS::mergeSortBottomUp;
using ADS::mergeSortRec;

// sort/MultikeyQuickSort.h
using ADS::multikeyQuickSort;

// sort/PigeonholeSort.h
using ADS::pigeonholeSort;

// sort/QuickSort.h
using ADS::partitionByValue;
using ADS::quickSort;
using ADS::quickSortTuned;

// sort/SelectionSort.h
using ADS::selectionSort;

// sort/ShellSort.h
using ADS::ciuraGaps;
using ADS::hibbardGaps;
using ADS::knuthGaps;
using ADS::prattGaps;
using ADS::shellGaps;
using ADS::shellSort;

// sort/Sorted.h
using ADS::RankStore;
using ADS::SortRange;
using ADS::SortStore;
using ADS::StringStore;
using ADS::checkingContract;

// sort/SortingNetworks.h
using ADS::Comparator;
using ADS::applyNetwork;
using ADS::bitonicNetwork;
using ADS::mergeExchangeNetwork;

// sort/StrandSort.h
using ADS::strandSort;

// sort/TreeSort.h
using ADS::treeSort;

// stack/ArrayStack.h
using ADS::ArrayStack;

// stack/Expressions.h
using ADS::evalPostfix;
using ADS::toPostfix;
using ADS::toPostfixParenthesized;

// stack/MinStack.h
using ADS::MinStack;

// stack/MultiStack.h
using ADS::MultiStack;

// stack/SequenceStack.h
using ADS::SequenceStack;

// stack/StackAlgorithms.h
using ADS::forEachBottomUp;
using ADS::isDyckWord;
using ADS::isMatched;
using ADS::isPalindrome;
using ADS::removeGreater;
using ADS::reverseWithQueue;
using ADS::sortStack;

// string/Alignment.h
using ADS::Alignment;
using ADS::alignmentScore;
using ADS::globalAlignment;

// string/ApproximateMatching.h
using ADS::Table;
using ADS::editDistance;
using ADS::matchingTable;
using ADS::occurrence;
using ADS::stringMatching;
using ADS::stringMatchingSingleRow;

// string/FrontCoding.h
using ADS::FrontCodedDictionary;
using ADS::FrontPair;
using ADS::decodeLength;
using ADS::encodeLength;
using ADS::frontCode;
using ADS::frontDecode;

// string/Huffman.h
using ADS::decode;
using ADS::encode;
using ADS::huffman;

// string/LongestCommonSubsequence.h
using ADS::diff;
using ADS::lcs;
using ADS::lcsMemo;
using ADS::lcsTable;
using ADS::longestCommonSubstring;

// string/Lyndon.h
using ADS::isLyndon;
using ADS::leastRotation;
using ADS::lyndonFactorization;

// string/Palindromes.h
using ADS::Eertree;
using ADS::longestPalindrome;
using ADS::longestPalindromicSubsequence;
using ADS::longestPalindromicSubstring;
using ADS::palindromicFactorization;
using ADS::palindromicPartition;

// string/Parentheses.h
using ADS::longestBalancedSubsequence;
using ADS::longestBalancedSubstring;

// string/PrefixSearch.h
using ADS::BinnedSearch;
using ADS::BinnedStringSearch;
using ADS::commonPrefixLength;
using ADS::lexicographicPosition;
using ADS::prefixEnd;
using ADS::prefixRange;

// string/RearCoding.h
using ADS::RearCodedDictionary;
using ADS::RearPair;
using ADS::rearCode;
using ADS::rearDecode;

// string/Segmentations.h
using ADS::countDecodings;
using ADS::countSegmentations;

// string/StringBreaking.h
using ADS::stringBreakingCost;
using ADS::stringBreakingOrder;

// string/StringMatching.h
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

// string/SubsequenceProblems.h
using ADS::isInterleaving;
using ADS::lcs3;
using ADS::scsLength;
using ADS::shortestCommonSupersequence;
using ADS::subsequenceOccurrences;

// string/SuffixApplications.h
using ADS::CopyPhrase;
using ADS::decodeCopyPhrases;
using ADS::distinctSubstrings;
using ADS::kMismatches;
using ADS::longestRepeatedSubstring;
using ADS::lz77ByTree;
using ADS::maximalRepeats;
using ADS::proximity;
using ADS::repeatedSubstring;

// string/SuffixArray.h
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

// string/SuffixTree.h
using ADS::SuffixTree;

// string/WordBreak.h
using ADS::wordBreak;

// succinct/BPTree.h
using ADS::BPTree;

// succinct/BVGraph.h
using ADS::BVGraph;

// succinct/BalancedParentheses.h
using ADS::BalancedParentheses;

// succinct/BitStream.h
using ADS::BitReader;
using ADS::BitWriter;

// succinct/BitVector.h
using ADS::BitVector;
using ADS::SubscriptIterator;
using ADS::Words;
using ADS::selectInWord;

// succinct/EliasFano.h
using ADS::BasicEliasFano;
using ADS::EliasFano;
using ADS::LazyEliasFano;

// succinct/EliasFanoGraph.h
using ADS::EliasFanoGraph;

// succinct/HuffmanWaveletTree.h
using ADS::HuffmanWaveletTree;

// succinct/JacobsonTree.h
using ADS::JacobsonTree;

// succinct/K2Tree.h
using ADS::K2Tree;

// succinct/LoudsTree.h
using ADS::LoudsTree;

// succinct/RankSelect.h
using ADS::BasicRankSelect;
using ADS::LazyRankSelect;
using ADS::RankSelect;
using ADS::RankSelectOptions;

// tree/AVLTree.h
using ADS::AVLTree;
using ADS::AVLTreeNodes;

// tree/ArrayBinaryTree.h
using ADS::ArrayBinaryTree;

// tree/BPlusTree.h
using ADS::BPlusTree;

// tree/BTree.h
using ADS::BTree;

// tree/BinaryTree.h
using ADS::BinaryTree;
using ADS::copy;
using ADS::dary_child;
using ADS::dary_parent;
using ADS::from_list;
using ADS::in_visit;
using ADS::inorder;
using ADS::join;
using ADS::post_visit;
using ADS::postorder;
using ADS::pre_visit;
using ADS::preorder;
using ADS::to_binary;
using ADS::to_list;

// tree/BinaryTreeAlgorithms.h
using ADS::BinaryNode;
using ADS::KeyedBinaryNode;
using ADS::ParentBinaryNode;
using ADS::SizedBinaryNode;
using ADS::cartesianBinaryTree;
using ADS::equalTrees;
using ADS::firstInorder;
using ADS::firstPostorder;
using ADS::fromPreorderInorder;
using ADS::fromShapeCode;
using ADS::isRedBlackTree;
using ADS::isSearchTree;
using ADS::isSizeBalanced;
using ADS::lessOrEqual;
using ADS::lowestCommonAncestor;
using ADS::minimumHeightSearchTree;
using ADS::nextInorder;
using ADS::nextPostorder;
using ADS::nextPreorder;
using ADS::orderNumbers;
using ADS::searchTreeFind;
using ADS::searchTreeFindKey;
using ADS::searchTreeLink;
using ADS::searchTreeLookup;
using ADS::searchTreeLowerBound;
using ADS::searchTreeLowerBoundEntry;
using ADS::searchTreeLowerBoundKey;
using ADS::searchTreeRange;
using ADS::searchTreeRank;
using ADS::searchTreeSelect;
using ADS::searchTreeSplice;
using ADS::shapeCode;
using ADS::subtreeHeight;
using ADS::subtreeSize;
using ADS::treeHeight;

// tree/ChildListTree.h
using ADS::ChildListTree;

// tree/CursorBinaryTree.h
using ADS::CursorBinaryTree;

// tree/CursorTree.h
using ADS::CursorTree;

// tree/ExpressionTree.h
using ADS::evaluate;
using ADS::expressionTree;
using ADS::infixForm;
using ADS::prefixForm;

// tree/FingerTreap.h
using ADS::FingerTreap;

// tree/HashedTreap.h
using ADS::HashedTreap;

// tree/HeapByInsertion.h
using ADS::heapBuildByInsertion;
using ADS::maxHeapSiftUp;

// tree/LeftLeaningRedBlackTree.h
using ADS::LeftLeaningRedBlackTree;

// tree/MeasuredSearchTree.h
using ADS::MeasuredSearchTree;

// tree/ParentTree.h
using ADS::ParentTree;

// tree/PartialRebuildingTree.h
using ADS::CountdownTree;
using ADS::DynamiteTree;
using ADS::PartialRebuildingTree;
using ADS::RebuildPolicy;
using ADS::WeightRebuildTree;

// tree/PreorderArrayTree.h
using ADS::PreorderArrayTree;

// tree/RandomizedSearchTree.h
using ADS::RandomizedSearchTree;

// tree/ScapegoatTree.h
using ADS::ScapegoatTree;

// tree/SearchTree.h
using ADS::BinarySearchTree;
using ADS::Color;
using ADS::NotJoinable;
using ADS::RedBlackTree;
using ADS::SearchTree;
using ADS::SearchTreeNodes;

// tree/ThreadedTree.h
using ADS::ThreadedTree;

// tree/Treap.h
using ADS::Treap;
using ADS::TreapNodes;

// tree/Tree.h
using ADS::Tree;
using ADS::children;
using ADS::maxWeightLevel;
using ADS::tree_from_list;
}

/*--------------------------------------------------------------------------*/
/*------------------------- End File ADSBase.cppm --------------------------*/
/*--------------------------------------------------------------------------*/

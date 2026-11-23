/*--------------------------------------------------------------------------*/
/*------------------------- File ADSAdvanced.cppm --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Interface of the C++20 module ads.advanced, which exports the names of
 * ads.core, without importing it, and those that the headers of the advanced
 * set of ADS [see ADS/advanced.h] declare in the namespace ADS and in those
 * nested in it, but for the namespaces detail; the names of the base headers
 * that these use are not exported. The names of the executors of OpenMP,
 * oneTBB and FastFlow and of the sort on MPI are exported only if the module
 * is compiled with their library, as their headers are.
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

#include "ADS/advanced.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- MODULE PURVIEW -----------------------------*/
/*--------------------------------------------------------------------------*/

export module ads.advanced;

export namespace ADS
{

// Algebra.h
using ADS::AbelianGroup;
using ADS::Action;
using ADS::CommutativeMonoid;
using ADS::Group;
using ADS::MaxOf;
using ADS::MinOf;
using ADS::Monoid;
using ADS::Selective;
using ADS::SumOf;
using ADS::exact_v;

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

// Modification.h
using ADS::Batch;
using ADS::Dynamic;
using ADS::Keep;
using ADS::NeumaierSum;
using ADS::NoObservers;
using ADS::Quiet;
using ADS::RatioPolicy;
using ADS::Refused;
using ADS::Reopt;
using ADS::ReoptPolicy;
using ADS::ReoptStats;
using ADS::Shadow;
using ADS::Static;
using ADS::UndoLog;
using ADS::batch;
using ADS::no_rebuild;
using ADS::no_rebuild_t;
using ADS::quiet;

// backtrack/FastExactCover.h
using ADS::FastExactCover;

// combinatorial/FastBitwise.h
using ADS::mortonDecode;
using ADS::mortonEncode;
using ADS::pdep;
using ADS::pext;
using ADS::popcount;
using ADS::popcountAnd;
using ADS::select;

// combinatorial/FastGeneration.h
using ADS::forEachCombination;
using ADS::forEachCombinationMask;
using ADS::forEachPermutation;
using ADS::forEachPermutationLex;
using ADS::nextCombination;

// compression/ALP.h
using ADS::ALPFloat;
using ADS::alpCompress;
using ADS::alpDecompress;

// compression/BinaryPacking.h
using ADS::binaryPack;
using ADS::binaryUnpack;
using ADS::patchedPack;
using ADS::patchedUnpack;

// compression/DirectAccessCodes.h
using ADS::DirectAccessCodes;

// compression/FSST.h
using ADS::FSSTColumn;
using ADS::FSSTTable;

// compression/FastLanes.h
using ADS::FastLanesScheme;
using ADS::FastLanesWord;
using ADS::fastLanesDecode;
using ADS::fastLanesEncode;
using ADS::fastLanesPack;
using ADS::fastLanesTransposed;
using ADS::fastLanesUnpack;
using ADS::fastLanesVector;

// compression/HuffmanCoder.h
using ADS::huffmanCompress;
using ADS::huffmanDecompress;
using ADS::limitedHuffmanLengths;

// compression/HuffmanFMIndex.h
using ADS::HuffmanFMIndex;

// compression/LZ77Linear.h
using ADS::lz77ParseLinear;

// compression/LZBicriteria.h
using ADS::LZBicriteriaGoal;
using ADS::LZBicriteriaModel;
using ADS::LZBicriteriaParse;
using ADS::LZBicriteriaPhrase;
using ADS::lzBicriteriaDecode;
using ADS::lzBicriteriaEncode;
using ADS::lzBicriteriaMaxLiterals;
using ADS::lzBicriteriaParse;
using ADS::lzBicriteriaSize;
using ADS::lzBicriteriaTime;

// compression/MoveRIndex.h
using ADS::MoveRIndex;

// compression/PackedBits.h
using ADS::PackedBitReader;
using ADS::PackedBitWriter;

// compression/RIndex.h
using ADS::RIndex;

// compression/Rans.h
using ADS::ransCompress;
using ADS::ransDecompress;
using ADS::scaleFrequencies;

// compression/StreamVByte.h
using ADS::streamVByteDecode;
using ADS::streamVByteEncode;

// compression/Tans.h
using ADS::tansCompress;
using ADS::tansDecompress;

// concurrent/Backoff.h
using ADS::Backoff;

// concurrent/ConcurrentBTreeMap.h
using ADS::ConcurrentBTreeMap;

// concurrent/ConcurrentCuckooTable.h
using ADS::ConcurrentCuckooTable;

// concurrent/ConcurrentFlatTable.h
using ADS::ConcurrentFlatTable;
using ADS::EpochClear;
using ADS::FlatClear;
using ADS::ZeroingClear;

// concurrent/ConcurrentUnionFind.h
using ADS::ConcurrentUnionFind;

// concurrent/EpochReclaim.h
using ADS::DebraEpochs;
using ADS::EpochDomain;
using ADS::FraserEpochs;

// concurrent/EventCount.h
using ADS::EventCount;

// concurrent/HazardPointer.h
using ADS::HazardDomain;
using ADS::HazardPointers;
using ADS::hazard_pointer;
using ADS::hazard_pointer_obj_base;
using ADS::make_hazard_pointer;
using ADS::swap;

// concurrent/LockFreePriorityQueue.h
using ADS::LockFreePriorityQueue;

// concurrent/LockFreeQueue.h
using ADS::LockFreeQueue;

// concurrent/LockFreeStack.h
using ADS::LockFreeStack;

// concurrent/MPMCQueue.h
using ADS::MPMCQueue;

// concurrent/MPSCInbox.h
using ADS::MPSCInbox;

// concurrent/MPSCQueue.h
using ADS::MPSCQueue;

// concurrent/MinBoard.h
using ADS::MinBoard;

// concurrent/MultiQueue.h
using ADS::MultiQueue;
using ADS::MultiQueueHeap;

// concurrent/Reclaim.h
using ADS::Retirable;
using ADS::reclamation_policy;

// concurrent/SPSCQueue.h
using ADS::SPSCQueue;

// concurrent/SingleWriterFlatTable.h
using ADS::SingleWriterFlatTable;

// concurrent/StealablePriorityQueue.h
using ADS::StealablePriorityQueue;
using ADS::StealingFrontier;

// concurrent/WorkStealingDeque.h
using ADS::WorkStealingDeque;

// dictionary/AdaptiveRadixTree.h
using ADS::AdaptiveRadixTree;

// dictionary/BucketCuckooTable.h
using ADS::BucketCuckooTable;

// dictionary/ConsensusRecSplit.h
using ADS::ConsensusRecSplit;

// dictionary/DenseHashMap.h
using ADS::DenseHashMap;

// dictionary/FastHash.h
using ADS::DefaultHash;
using ADS::FastHash;
using ADS::Product128;
using ADS::foldedProduct;
using ADS::hashBytes;
using ADS::hashWord;
using ADS::product128;

// dictionary/FingerprintPerfectHash.h
using ADS::FingerprintPerfectHash;

// dictionary/FlatHashTable.h
using ADS::FlatHashTable;

// dictionary/MinimalPerfectHash.h
using ADS::MinimalPerfectHash;

// dictionary/PointerTable.h
using ADS::IndexPlaces;
using ADS::PointerTable;
using ADS::StateEqual;
using ADS::WidePlaces;
using ADS::WithPlaces;

// dictionary/PositionIndex.h
using ADS::PositionIndex;

// dictionary/PtrHash.h
using ADS::PtrHash;
using ADS::PtrHashBucket;
using ADS::PtrHashRemap;

// dictionary/RandomHashing.h
using ADS::SplitMix64;
using ADS::TabulationHash;
using ADS::ZobristTable;
using ADS::fibonacciPlace;
using ADS::splitmix64;

// dictionary/RecSplit.h
using ADS::RecSplit;

// dictionary/RowFingerprint.h
using ADS::canonicalWord;
using ADS::rowFingerprint;
using ADS::sameRow;

// dictionary/SuccinctPatriciaTrie.h
using ADS::SuccinctPatriciaTrie;

// distributed/MPIExecutor.h
#if defined( ADS_WITH_MPI )
using ADS::DistributedMin;
using ADS::MPIEnvironment;
using ADS::MPIExecutor;
#endif

// distributed/MPIMailbox.h
#if defined( ADS_WITH_MPI )
using ADS::MPIByteMailbox;
using ADS::MPICodec;
using ADS::MPIMailbox;
#endif

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

// geometry/IntervalTree.h
using ADS::IntervalTree;

// geometry/RangeTree.h
using ADS::RangeTree;

// graph/Assignment.h
using ADS::Assignment;
using ADS::AssignmentEngine;

// graph/Biconnectivity.h
using ADS::BiconnectedComponents;
using ADS::BlockCutTree;
using ADS::biconnectedComponents;
using ADS::blockCutTree;
using ADS::twoEdgeConnectedComponents;

// graph/BlossomMatching.h
using ADS::maxCardinalityMatching;
using ADS::maxWeightMatching;

// graph/GraphBuilder.h
using ADS::GraphBuilder;

// graph/GridGraph.h
using ADS::Corners;
using ADS::GridGraph;
using ADS::GridMoves;
using ADS::MoveCost;
using ADS::chebyshev;
using ADS::gridDistances;
using ADS::gridHeuristic;
using ADS::manhattan;
using ADS::octile;
using ADS::readGridMap;

// graph/Observable.h
using ADS::EdgeDeleted;
using ADS::EdgeInserted;
using ADS::GraphEvent;
using ADS::GraphEvents;
using ADS::GraphModifications;
using ADS::GraphObserver;
using ADS::NodeDeleted;
using ADS::NodeInserted;
using ADS::Observable;
using ADS::ObservedGraph;
using ADS::WeightChanged;
using ADS::coalesce;
using ADS::endsOf;
using ADS::inverse;
using ADS::kindOf;

// graph/PackedGraph.h
using ADS::PackedGraph;

// graph/PerfectMatching.h
using ADS::BlossomVOptions;
using ADS::PerfectMatchingResult;
using ADS::maxWeightMatching;
using ADS::minCostPerfectMatching;

// graph/PointerGraph.h
using ADS::PointerGraph;

// graph/Reoptimization.h
using ADS::AssignmentObserver;
using ADS::AugmentEngine;
using ADS::EventQueue;
using ADS::FlowEngine;
using ADS::MaxFlowObserver;
using ADS::Reoptimizable;
using ADS::ShortestPathsObserver;
using ADS::SpanningForestObserver;

// graph/StaticGraph.h
using ADS::ArcLayout;
using ADS::CsrArrays;
using ADS::StaticGraph;
using ADS::WeightedArc;
using ADS::bfsOrder;

// graph/StrongComponents.h
using ADS::sccPearce;
using ADS::sccTarjan;

// memory/Arena.h
using ADS::Arena;
using ADS::ArenaResource;

// memory/ConcurrentPool.h
using ADS::ConcurrentPoolResource;

// memory/HugePageAllocator.h
using ADS::HugePageAllocator;
using ADS::ZeroPageAllocator;

// memory/ObjectPool.h
using ADS::ObjectPool;

// memory/PoolAllocator.h
using ADS::PoolAllocator;
using ADS::PoolResource;

// memory/Prefetch.h
using ADS::Locality;

// memory/SizeClassAllocator.h
using ADS::SizeClassAllocator;

// mfset/RemUnionFind.h
using ADS::RemUnionFind;

// mfset/RollbackUnionFind.h
using ADS::ConnectivityOp;
using ADS::RollbackUnionFind;
using ADS::offlineConnectivity;

// mfset/UnionFind.h
using ADS::Compression;
using ADS::DisjointSets;
using ADS::Union;
using ADS::UnionFind;

// numeric/CsrMatrix.h
using ADS::BasicCsrMatrix;
using ADS::CsrMatrix;

// numeric/DenseMatrix.h
using ADS::DenseMatrix;
using ADS::gemm;
using ADS::kGemmBlock;
using ADS::multiply;
using ADS::multiplyStrassen;
using ADS::transpose;

// numeric/DynamicCsr.h
using ADS::DynamicCsc;
using ADS::DynamicCsr;

// numeric/RandomGenerators.h
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

// numeric/SellMatrix.h
using ADS::SellMatrix;

// numeric/SparseAssembly.h
using ADS::AssemblyWorkspace;
using ADS::assembleCsc;
using ADS::assembleCsr;
using ADS::compressTriplets;

// parallel/Affinity.h
using ADS::affinityCpus;
using ADS::availableCpus;
using ADS::currentCpu;
using ADS::pinThread;

// parallel/Channels.h
using ADS::ChannelFamily;
using ADS::ChannelPort;
using ADS::ChannelsOf;
using ADS::DefaultChannels;
using ADS::InboxChannels;
using ADS::LaneChannels;
using ADS::LockedChannels;
using ADS::MeshChannels;

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

// parallel/Farm.h
using ADS::feedbackFarm;

// parallel/FastFlowExecutor.h
#if defined( ADS_WITH_FASTFLOW )
using ADS::FastFlowExecutor;
using ADS::FastFlowLock;
#endif

// parallel/Mailbox.h
using ADS::CoalesceBy;
using ADS::Mailbox;
using ADS::MailboxCoalescing;
using ADS::NoCoalescing;
using ADS::ownerOf;

// parallel/OpenMPExecutor.h
#if defined( _OPENMP )
using ADS::OpenMPExecutor;
using ADS::OpenMPLock;
#endif

// parallel/ParallelBiconnectivity.h
using ADS::biconnectedComponents;
using ADS::twoEdgeConnectedComponents;

// parallel/ParallelGraph.h
using ADS::bfsDistances;
using ADS::connectedComponents;

// parallel/ParallelGreedy.h
using ADS::filterKruskal;
using ADS::maximalIndependentSet;
using ADS::maximalMatching;
using ADS::parallelKruskal;
using ADS::randomOrder;
using ADS::spanningForest;

// parallel/ParallelMerge.h
using ADS::multisequenceSplit;
using ADS::multiwayMerge;

// parallel/ParallelPrimitives.h
using ADS::countingSort;
using ADS::filter;
using ADS::flatten;
using ADS::groupByKey;
using ADS::histogram;
using ADS::histogramByKey;
using ADS::integerSort;
using ADS::pack;
using ADS::packIndex;
using ADS::randomPermutation;
using ADS::randomShuffle;
using ADS::reduceOf;
using ADS::removeDuplicates;
using ADS::scanOf;
using ADS::semisort;
using ADS::split;
using ADS::tabulate;
using ADS::tokens;

// parallel/ParallelShortestPaths.h
using ADS::deltaStepping;
using ADS::rhoStepping;

// parallel/ParallelSketch.h
using ADS::MergeableSketch;
using ADS::parallelSketch;

// parallel/ParallelSort.h
using ADS::kParallelSort;
using ADS::kParallelSortThreads;
using ADS::learnedSort;
using ADS::learnedStableSort;
using ADS::sort;
using ADS::stableSort;

// parallel/ParallelText.h
using ADS::lcpArray;
using ADS::lcpArrayPhi;
using ADS::suffixArray;

// parallel/SpeculativeFor.h
using ADS::Reservation;
using ADS::SpeculativeCounts;
using ADS::SpeculativeStep;
using ADS::speculativeFor;

// parallel/TBBExecutor.h
#if defined( ADS_WITH_TBB )
using ADS::TBBExecutor;
#endif

// parallel/ThreadPool.h
using ADS::ThreadPool;

// priority/BucketQueue.h
using ADS::BucketHeap;
using ADS::BucketQueue;
using ADS::BucketTie;
using ADS::LargestSecondKey;
using ADS::LeastSecondKey;
using ADS::keyF;
using ADS::keyG;
using ADS::packFG;

// priority/DaryHeap.h
using ADS::DaryHeap;

// priority/DialQueue.h
using ADS::DialQueue;

// priority/IndexedDaryHeap.h
using ADS::IndexedDaryHeap;
using ADS::TableHeap;

// priority/IndexedHeapPair.h
using ADS::IndexedHeapPair;

// priority/LinkedHeapPriorityQueue.h
using ADS::LinkedHeapPriorityQueue;

// priority/MinMaxHeap.h
using ADS::IntervalHeap;
using ADS::MinMaxHeap;

// priority/MultiwayMerge.h
using ADS::FlatLoserTree;

// priority/PairingHeap.h
using ADS::IntrusivePairingHeap;
using ADS::PairingHeap;

// priority/QueueCustomization.h
using ADS::queue_decrease;
using ADS::queue_deleteMin;
using ADS::queue_handle_t;
using ADS::queue_insert;
using ADS::queue_item_t;
using ADS::queue_min;

// priority/RadixHeap.h
using ADS::RadixHeap;

// priority/SequenceHeap.h
using ADS::SequenceHeap;

// priority/WorkingSetHeap.h
using ADS::WorkingSetHeap;
using ADS::universalDijkstra;

// queue/RingDeque.h
using ADS::RingDeque;

// queue/SlidingAggregate.h
using ADS::Amortized;
using ADS::SlidingAggregate;
using ADS::WorstCase;
using ADS::window_kind;

// search/CompressedPGMIndex.h
using ADS::CompressedPGMIndex;

// search/DynamicPGMIndex.h
using ADS::DynamicPGMIndex;

// search/NthElement.h
using ADS::nthElement;

// search/PackedMemoryArray.h
using ADS::CompressedPackedMemoryArray;
using ADS::PackedMemoryArray;

// search/RangeMinimum.h
using ADS::PlusMinusOneRangeMinimum;
using ADS::RangeMinimum;
using ADS::SparseTable;
using ADS::cartesianTree;

// search/StaticSearch.h
using ADS::SPlusTree;
using ADS::branchlessLowerBound;

// search/WordRangeMinimum.h
using ADS::WordRangeMinimum;

// sequence/Fenwick.h
using ADS::Fenwick;
using ADS::FenwickObserver;

// sequence/ListSort.h
using ADS::listSort;

// sequence/ObservableArray.h
using ADS::ObservableArray;

// sequence/SmallVector.h
using ADS::SmallVector;

// sequence/TieredVector.h
using ADS::TieredVector;

// set/BinaryFuseFilter.h
using ADS::BinaryFuseFilter;

// set/BlockedBloomFilter.h
using ADS::BlockedBloomFilter;

// set/CountMinSketch.h
using ADS::CountMinSketch;
using ADS::CountMinUpdate;
using ADS::CountSketch;

// set/CountingQuotientFilter.h
using ADS::CountingQuotientFilter;

// set/CuckooFilter.h
using ADS::CuckooFilter;

// set/GrafiteFilter.h
using ADS::BucketingFilter;
using ADS::GrafiteFilter;

// set/KllSketch.h
using ADS::KllSketch;

// set/MementoFilter.h
using ADS::MementoFilter;

// set/MinHashSketch.h
using ADS::MinHashSketch;

// set/PrefixFilter.h
using ADS::PrefixFilter;

// set/RibbonFilter.h
using ADS::RibbonFilter;

// set/RoaringBitmap.h
using ADS::RoaringBitmap;
using ADS::setDifference;
using ADS::setIntersection;
using ADS::setSymmetricDifference;
using ADS::setUnion;

// set/UltraLogLog.h
using ADS::UltraLogLog;

// set/VectorIntersection.h
using ADS::intersect;

// set/VectorQuotientFilter.h
using ADS::VectorQuotientFilter;

// sort/PdqSort.h
using ADS::pdqSort;

// sort/QuicksortAdversary.h
using ADS::QuicksortAdversary;

// sort/RadixSort.h
using ADS::RadixKey;
using ADS::americanFlagSort;
using ADS::kFlagSmall;
using ADS::msdRadixSort;
using ADS::radixKey;
using ADS::radixSort;
using ADS::radixSortBy;

// sort/RunSorts.h
using ADS::powerSort;

// sort/SampleSort.h
using ADS::sampleSort;

// sort/Sort.h
using ADS::kRadix;
using ADS::kRuns;
using ADS::kStrings;

// sort/Sorted.h
using ADS::RankStore;
using ADS::SortRange;
using ADS::SortStore;
using ADS::StringStore;
using ADS::checkingContract;

// sort/StringSort.h
using ADS::prefixKeySort;

// sort/VectorQuickSort.h
using ADS::VectorKey;
using ADS::kVectorSort;
using ADS::vectorQuickSort;

// string/AhoCorasick.h
using ADS::AhoCorasick;

// string/EditDistance.h
using ADS::approximateMatches;
using ADS::lcsLength;
using ADS::levenshtein;

// string/LongestCommonExtension.h
using ADS::LceIndex;

// string/MultiSearch.h
using ADS::MultiSearcher;

// string/StringSearch.h
using ADS::StringSearcher;
using ADS::findAll;
using ADS::findFirst;

// string/SuffixAutomaton.h
using ADS::SuffixAutomaton;

// string/SuffixSorting.h
using ADS::plcpArray;

// succinct/CompactRankSelect.h
using ADS::CompactRankSelect;

// succinct/InterleavedRankSelect.h
using ADS::InterleavedRankSelect;

// succinct/LAVector.h
using ADS::LAVector;

// succinct/MoveStructure.h
using ADS::MoveStructure;

// succinct/PackedArray.h
using ADS::PackedArray;
using ADS::TritArray;

// succinct/PartitionedEliasFano.h
using ADS::PartitionedEliasFano;

// succinct/RRRBitVector.h
using ADS::RRRBitVector;

// succinct/TinyRankSelect.h
using ADS::BasicTinyRankSelect;
using ADS::TinyRankSelect;

// succinct/WaveletMatrix.h
using ADS::WaveletMatrix;

// tree/BTreeMap.h
using ADS::BTreeMap;

// tree/JoinAlgorithms.h
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

// tree/JoinTree.h
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

// tree/LinkCutTree.h
using ADS::LinkCutTree;

// tree/MaterializedAggregate.h
using ADS::MaterializedAggregate;

// tree/SplayTree.h
using ADS::SplayTree;
}

export namespace ADS
{
namespace distributed
{

// distributed/DistributedSort.h
#if defined( ADS_WITH_MPI )
using ADS::distributed::sort;
#endif
}
}

export namespace ADS
{
namespace mod
{

// Modification.h
using ADS::mod::Assigned;
using ADS::mod::BulkErased;
using ADS::mod::BulkInserted;
using ADS::mod::Changed;
using ADS::mod::Cleared;
using ADS::mod::Cost;
using ADS::mod::Counts;
using ADS::mod::Direct;
using ADS::mod::Erased;
using ADS::mod::Group;
using ADS::mod::Inserted;
using ADS::mod::Key;
using ADS::mod::KeyChanged;
using ADS::mod::Lower;
using ADS::mod::Parent;
using ADS::mod::Permuted;
using ADS::mod::Rebuilt;
using ADS::mod::Relocated;
using ADS::mod::Replaced;
using ADS::mod::Role;
using ADS::mod::Shifted;
using ADS::mod::Supply;
using ADS::mod::Upper;
using ADS::mod::Value;
using ADS::mod::Via;
using ADS::mod::auditable;
using ADS::mod::coalesce_traits;
using ADS::mod::contains_v;
using ADS::mod::counted;
using ADS::mod::effect;
using ADS::mod::effects_of;
using ADS::mod::groups;
using ADS::mod::index_v;
using ADS::mod::inverse;
using ADS::mod::invertible;
using ADS::mod::lazy;
using ADS::mod::list;
using ADS::mod::masked;
using ADS::mod::must_not_throw;
using ADS::mod::observer_of;
using ADS::mod::observes;
using ADS::mod::observes_span;
using ADS::mod::reduction_of;
using ADS::mod::refuses;
using ADS::mod::role_of;
using ADS::mod::variant_t;
using ADS::mod::with_policy;
}
}

export namespace ADS
{
namespace mod
{
namespace reduce
{

// Modification.h
using ADS::mod::reduce::last;
using ADS::mod::reduce::max;
using ADS::mod::reduce::min;
using ADS::mod::reduce::sum;
}
}
}

/*--------------------------------------------------------------------------*/
/*----------------------- End File ADSAdvanced.cppm ------------------------*/
/*--------------------------------------------------------------------------*/

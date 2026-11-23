/*--------------------------------------------------------------------------*/
/*---------------------------- File advanced.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The advanced set of ADS: the structures and the algorithms written for
 * speed beyond the textbooks, the concurrent and the parallel ones, the
 * executors, the distributed layer, the typed modifications and the
 * observers, with the headers of `core.h`. A header of this set extends those
 * of the base set by composition, and includes those it needs, without the
 * module ads.advanced exporting their names. The executors of OpenMP, oneTBB
 * and FastFlow and the headers of MPI are included only with their macros, as
 * in the module.
 *
 * This file is written by `cmake/ads_sets.py` from `cmake/ads_sets.txt`: it
 * is not to be edited by hand.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_advanced
 #define __ADS_advanced
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/core.h"

#include "ADS/Algebra.h"
#include "ADS/Modification.h"
#include "ADS/backtrack/FastExactCover.h"
#include "ADS/combinatorial/FastBitwise.h"
#include "ADS/combinatorial/FastGeneration.h"
#include "ADS/compression/BinaryPacking.h"
#include "ADS/compression/DirectAccessCodes.h"
#include "ADS/compression/HuffmanCoder.h"
#include "ADS/compression/HuffmanFMIndex.h"
#include "ADS/compression/LZ77Linear.h"
#include "ADS/compression/MoveRIndex.h"
#include "ADS/compression/RIndex.h"
#include "ADS/compression/Rans.h"
#include "ADS/compression/StreamVByte.h"
#include "ADS/compression/Tans.h"
#include "ADS/concurrent/Backoff.h"
#include "ADS/concurrent/ConcurrentCuckooTable.h"
#include "ADS/concurrent/ConcurrentFlatTable.h"
#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/concurrent/EventCount.h"
#include "ADS/concurrent/LockFreePriorityQueue.h"
#include "ADS/concurrent/MPMCQueue.h"
#include "ADS/concurrent/MPSCInbox.h"
#include "ADS/concurrent/MPSCQueue.h"
#include "ADS/concurrent/MinBoard.h"
#include "ADS/concurrent/MultiQueue.h"
#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/concurrent/SingleWriterFlatTable.h"
#include "ADS/concurrent/StealablePriorityQueue.h"
#include "ADS/concurrent/WorkStealingDeque.h"
#include "ADS/dictionary/AdaptiveRadixTree.h"
#include "ADS/dictionary/BucketCuckooTable.h"
#include "ADS/dictionary/DenseHashMap.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FingerprintPerfectHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/dictionary/MinimalPerfectHash.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/PositionIndex.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/dictionary/RowFingerprint.h"
#include "ADS/dictionary/SuccinctPatriciaTrie.h"
#if defined( ADS_WITH_MPI )
 #include "ADS/distributed/DistributedSort.h"
#endif
#if defined( ADS_WITH_MPI )
 #include "ADS/distributed/MPIExecutor.h"
#endif
#if defined( ADS_WITH_MPI )
 #include "ADS/distributed/MPIMailbox.h"
#endif
#include "ADS/geometry/IntervalTree.h"
#include "ADS/geometry/RangeTree.h"
#include "ADS/graph/Assignment.h"
#include "ADS/graph/BlossomMatching.h"
#include "ADS/graph/GraphBuilder.h"
#include "ADS/graph/GridGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/graph/PointerGraph.h"
#include "ADS/graph/PushRelabel.h"
#include "ADS/graph/Reoptimization.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/graph/StrongComponents.h"
#include "ADS/memory/Arena.h"
#include "ADS/memory/ConcurrentPool.h"
#include "ADS/memory/PageHeap.h"
#include "ADS/memory/PoolAllocator.h"
#include "ADS/memory/SizeClassAllocator.h"
#include "ADS/mfset/RemUnionFind.h"
#include "ADS/mfset/RollbackUnionFind.h"
#include "ADS/mfset/UnionFind.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/numeric/DenseMatrix.h"
#include "ADS/numeric/DynamicCsr.h"
#include "ADS/numeric/RandomGenerators.h"
#include "ADS/numeric/SellMatrix.h"
#include "ADS/numeric/SparseAssembly.h"
#include "ADS/parallel/Affinity.h"
#include "ADS/parallel/Channels.h"
#include "ADS/parallel/Farm.h"
#if defined( ADS_WITH_FASTFLOW )
 #include "ADS/parallel/FastFlowExecutor.h"
#endif
#include "ADS/parallel/Mailbox.h"
#if defined( _OPENMP )
 #include "ADS/parallel/OpenMPExecutor.h"
#endif
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
#include "ADS/priority/IndexedDaryHeap.h"
#include "ADS/priority/IndexedHeapPair.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/MinMaxHeap.h"
#include "ADS/priority/MultiwayMerge.h"
#include "ADS/priority/OrderKey.h"
#include "ADS/priority/PairingHeap.h"
#include "ADS/priority/QueueCustomization.h"
#include "ADS/priority/RadixHeap.h"
#include "ADS/priority/SequenceHeap.h"
#include "ADS/queue/RingDeque.h"
#include "ADS/queue/SlidingAggregate.h"
#include "ADS/search/CompressedPGMIndex.h"
#include "ADS/search/CountLess.h"
#include "ADS/search/DynamicPGMIndex.h"
#include "ADS/search/NthElement.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/search/StaticSearch.h"
#include "ADS/search/WordRangeMinimum.h"
#include "ADS/sequence/Fenwick.h"
#include "ADS/sequence/ListSort.h"
#include "ADS/sequence/ObservableArray.h"
#include "ADS/sequence/SmallVector.h"
#include "ADS/sequence/TieredVector.h"
#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/CountingQuotientFilter.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/GrafiteFilter.h"
#include "ADS/set/MementoFilter.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/set/VectorIntersection.h"
#include "ADS/set/VectorQuotientFilter.h"
#include "ADS/sort/LearnedSort.h"
#include "ADS/sort/PdqSort.h"
#include "ADS/sort/QuicksortAdversary.h"
#include "ADS/sort/RadixSort.h"
#include "ADS/sort/RunSorts.h"
#include "ADS/sort/SampleSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/sort/StringSort.h"
#include "ADS/sort/VectorQuickSort.h"
#include "ADS/string/AhoCorasick.h"
#include "ADS/string/EditDistance.h"
#include "ADS/string/LongestCommonExtension.h"
#include "ADS/string/MultiSearch.h"
#include "ADS/string/StringSearch.h"
#include "ADS/string/SuffixAutomaton.h"
#include "ADS/string/SuffixSorting.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/InterleavedRankSelect.h"
#include "ADS/succinct/LAVector.h"
#include "ADS/succinct/MoveStructure.h"
#include "ADS/succinct/PackedArray.h"
#include "ADS/succinct/PartitionedEliasFano.h"
#include "ADS/succinct/RRRBitVector.h"
#include "ADS/succinct/WaveletMatrix.h"
#include "ADS/tree/BTreeMap.h"
#include "ADS/tree/JoinAlgorithms.h"
#include "ADS/tree/JoinTree.h"
#include "ADS/tree/LinkCutTree.h"
#include "ADS/tree/MaterializedAggregate.h"
#include "ADS/tree/SplayTree.h"

/*--------------------------------------------------------------------------*/

#endif /* advanced.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File advanced.h ---------------------------*/
/*--------------------------------------------------------------------------*/

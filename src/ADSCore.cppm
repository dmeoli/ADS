/*--------------------------------------------------------------------------*/
/*--------------------------- File ADSCore.cppm ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Interface of the C++20 module ads.core, which exports the names that the
 * headers of the core of ADS [see ADS/core.h] declare in the namespace ADS
 * and in those nested in it, but for the namespaces detail; the modules
 * ads.base and ads.advanced export it in turn.
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

#include "ADS/core.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- MODULE PURVIEW -----------------------------*/
/*--------------------------------------------------------------------------*/

export module ads.core;

export namespace ADS
{

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

// compression/PackedBits.h
using ADS::PackedBitReader;
using ADS::PackedBitWriter;

// dictionary/RandomHashing.h
using ADS::SplitMix64;
using ADS::TabulationHash;
using ADS::ZobristTable;
using ADS::fibonacciPlace;
using ADS::splitmix64;

// external/RecordFile.h
using ADS::RecordFile;
using ADS::RecordWriter;
using ADS::TempDirectory;

// memory/HugePageAllocator.h
using ADS::HugePageAllocator;

// memory/ObjectPool.h
using ADS::ObjectPool;

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

// sort/Sorted.h
using ADS::RankStore;
using ADS::SortRange;
using ADS::SortStore;
using ADS::StringStore;
using ADS::checkingContract;
}

/*--------------------------------------------------------------------------*/
/*------------------------- End File ADSCore.cppm --------------------------*/
/*--------------------------------------------------------------------------*/

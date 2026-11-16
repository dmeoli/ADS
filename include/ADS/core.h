/*--------------------------------------------------------------------------*/
/*------------------------------ File core.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The core of ADS, included by the two sets of the library [see `base.h` and
 * `advanced.h`]: the infrastructure that their headers share, i.e., the
 * contracts, the iterators, the primitives of the processor, the
 * capabilities, the concept of the executors, the pool of the nodes and its
 * huge pages, the random hashing, the checks of the sorts, the bits of the
 * coders and the files of records. The module ads.core exports its names.
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

#ifndef __ADS_core
 #define __ADS_core
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Abi.h"
#include "ADS/Capabilities.h"
#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/Portable.h"
#include "ADS/compression/PackedBits.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/external/RecordFile.h"
#include "ADS/memory/AllocatorAware.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/memory/ObjectPool.h"
#include "ADS/parallel/Executor.h"
#include "ADS/sort/Sorted.h"

/*--------------------------------------------------------------------------*/

#endif /* core.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File core.h -----------------------------*/
/*--------------------------------------------------------------------------*/

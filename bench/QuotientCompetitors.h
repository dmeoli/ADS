/*--------------------------------------------------------------------------*/
/*---------------------- File QuotientCompetitors.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quotient filters of their authors, the competitors of the counting
 * and vector quotient filters and of the Memento filter of ADS in
 * bench/hashing.cpp: the counting quotient filter of splatlab/cqf, the
 * vector quotient filter of splatlab/vqf (fingerprints of 8 bits, the
 * width its header fixes) and the Memento filter of n3slami/Memento_Filter,
 * each compiled in a unit of its own (qf_cqf.c, qf_vqf.cpp, qf_memento.c)
 * from the sources in the directory of HASHING, since cqf and Memento
 * declare the same names and types. The units define the functions below,
 * with C linkage, that the benchmark calls; the build defines ADS_HAS_CQF,
 * ADS_HAS_VQF and ADS_HAS_MEMENTO where it compiles each.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#ifndef __ADS_QuotientCompetitors
 #define __ADS_QuotientCompetitors

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// a counting quotient filter of nslots slots and keys hashed to keyBits
/// bits, its remainders of keyBits - log2( nslots ) bits
void * cqfNew( uint64_t nslots , int keyBits );
int cqfInsert( void * f , uint64_t key );  ///< 1 if inserted
uint64_t cqfCount( const void * f , uint64_t key );
int cqfRemove( void * f , uint64_t key );  ///< 1 if removed
uint64_t cqfBytes( const void * f );
void cqfFree( void * f );

/// a vector quotient filter of nslots slots, the keys taken as their hash
void * vqfNew( uint64_t nslots );
int vqfInsert( void * f , uint64_t key );  ///< 1 if inserted
int vqfContains( void * f , uint64_t key );
int vqfRemove( void * f , uint64_t key );  ///< 1 if removed
uint64_t vqfBytes( const void * f );
void vqfFree( void * f );

/// a Memento filter of nslots slots, mementos of r bits and prefixes
/// hashed to keyBits bits
void * mementoNew( uint64_t nslots , int keyBits , int r );
int mementoInsert( void * f , uint64_t key );  ///< 1 if inserted
int mementoRange( void * f , uint64_t a , uint64_t b );
uint64_t mementoBytes( const void * f );
void mementoFree( void * f );

#ifdef __cplusplus
 }
#endif

#endif /* QuotientCompetitors.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File QuotientCompetitors.h ----------------------*/
/*--------------------------------------------------------------------------*/

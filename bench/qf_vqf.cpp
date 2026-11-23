/*--------------------------------------------------------------------------*/
/*---------------------------- File qf_vqf.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vector quotient filter of splatlab/vqf behind the functions of
 * QuotientCompetitors.h, its sources (C++, in files named .c) included from
 * the directory of HASHING (with -I<HASHING>/vqf/include
 * -I<HASHING>/vqf/src), the tables of its shuffles of AVX-512 too, which
 * its code reads when the unit is compiled for AVX-512.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#include "QuotientCompetitors.h"

#include "shuffle_matrix_512.c"
#include "shuffle_matrix_512_16.c"
#include "vqf_filter.c"

extern "C" {

void * vqfNew( uint64_t nslots ) {
 return( vqf_init( nslots ) );
 }

int vqfInsert( void * f , uint64_t key ) {
 return( vqf_insert( static_cast< vqf_filter * >( f ) , key ) );
 }

int vqfContains( void * f , uint64_t key ) {
 return( vqf_is_present( static_cast< vqf_filter * >( f ) , key ) );
 }

int vqfRemove( void * f , uint64_t key ) {
 return( vqf_remove( static_cast< vqf_filter * >( f ) , key ) );
 }

uint64_t vqfBytes( const void * f ) {
 return(
   static_cast< const vqf_filter * >( f )->metadata.total_size_in_bytes );
 }

void vqfFree( void * f ) {
 std::free( f );
 }

 }  // end( extern "C" )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File qf_vqf.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/

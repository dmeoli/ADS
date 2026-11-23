/*--------------------------------------------------------------------------*/
/*----------------------------- File qf_cqf.c ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The counting quotient filter of splatlab/cqf behind the functions of
 * QuotientCompetitors.h, its sources included from the directory of HASHING
 * (with -I<HASHING>/cqf/include -I<HASHING>/cqf/src), its table zeroed
 * when made.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#include "QuotientCompetitors.h"

#include "gqf.c"
#include "hashutil.c"
#include "partitioned_counter.c"

void * cqfNew( uint64_t nslots , int keyBits )
{
 // qf_malloc() with the buffer zeroed, which qf_init() expects and
 // qf_malloc() leaves to the memory it gets
 QF * f = ( QF * )malloc( sizeof( QF ) );
 const uint64_t bytes =
   qf_init( f , nslots , ( uint64_t )keyBits , 0 , QF_HASH_DEFAULT , 1 , NULL ,
	    0 );
 void * buffer = calloc( bytes , 1 );
 f->runtimedata = ( qfruntime * )calloc( sizeof( qfruntime ) , 1 );
 if( qf_init( f , nslots , ( uint64_t )keyBits , 0 , QF_HASH_DEFAULT , 1 ,
	      buffer ,
	      bytes ) != bytes ) {
  free( f );
  return( NULL );
  }
 return( f );
 }

int cqfInsert( void * f , uint64_t key )
{
 return( qf_insert( ( QF * )f , key , 0 , 1 , QF_NO_LOCK ) >= 0 );
 }

uint64_t cqfCount( const void * f , uint64_t key )
{
 return( qf_count_key_value( ( const QF * )f , key , 0 , 0 ) );
 }

int cqfRemove( void * f , uint64_t key )
{
 return( qf_remove( ( QF * )f , key , 0 , 1 , QF_NO_LOCK ) >= 0 );
 }

uint64_t cqfBytes( const void * f )
{
 return( qf_get_total_size_in_bytes( ( const QF * )f ) );
 }

void cqfFree( void * f )
{
 qf_free( ( QF * )f );
 free( f );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- End File qf_cqf.c ----------------------------*/
/*--------------------------------------------------------------------------*/

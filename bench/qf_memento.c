/*--------------------------------------------------------------------------*/
/*--------------------------- File qf_memento.c ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Memento filter of n3slami/Memento_Filter behind the functions of
 * QuotientCompetitors.h, its sources included from the directory of HASHING
 * (with -I<HASHING>/Memento_Filter/include -I<HASHING>/Memento_Filter/src)
 * and their external names prefixed by memento_, which are those of cqf.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#include "QuotientCompetitors.h"

#define qf_bulk_load memento_qf_bulk_load
#define qf_copy memento_qf_copy
#define qf_delete_single memento_qf_delete_single
#define qf_destroy memento_qf_destroy
#define qf_dump memento_qf_dump
#define qf_dump_block memento_qf_dump_block
#define qf_dump_metadata memento_qf_dump_metadata
#define qf_free memento_qf_free
#define qf_get_bits_per_slot memento_qf_get_bits_per_slot
#define qf_get_hashmode memento_qf_get_hashmode
#define qf_get_hash_range memento_qf_get_hash_range
#define qf_get_hash_seed memento_qf_get_hash_seed
#define qf_get_nslots memento_qf_get_nslots
#define qf_get_num_distinct_key_value_pairs                                   \
 memento_qf_get_num_distinct_key_value_pairs
#define qf_get_num_key_bits memento_qf_get_num_key_bits
#define qf_get_num_key_fingerprint_bits memento_qf_get_num_key_fingerprint_bits
#define qf_get_num_memento_bits memento_qf_get_num_memento_bits
#define qf_get_num_occupied_slots memento_qf_get_num_occupied_slots
#define qf_get_sum_of_counts memento_qf_get_sum_of_counts
#define qf_get_total_size_in_bytes memento_qf_get_total_size_in_bytes
#define qfi_end memento_qfi_end
#define qfi_get_hash memento_qfi_get_hash
#define qfi_get_key memento_qfi_get_key
#define qfi_next memento_qfi_next
#define qf_init memento_qf_init
#define qf_inner_product memento_qf_inner_product
#define qf_insert_mementos memento_qf_insert_mementos
#define qf_insert_single memento_qf_insert_single
#define qf_intersect memento_qf_intersect
#define qf_is_auto_resize_enabled memento_qf_is_auto_resize_enabled
#define qf_iterator_by_key memento_qf_iterator_by_key
#define qf_iterator_from_position memento_qf_iterator_from_position
#define qf_magnitude memento_qf_magnitude
#define qf_malloc memento_qf_malloc
#define qf_merge memento_qf_merge
#define qf_multi_merge memento_qf_multi_merge
#define qf_point_query memento_qf_point_query
#define qf_range_query memento_qf_range_query
#define qf_reset memento_qf_reset
#define qf_resize memento_qf_resize
#define qf_resize_malloc memento_qf_resize_malloc
#define qf_set_auto_resize memento_qf_set_auto_resize
#define qf_use memento_qf_use
#define hash_64 memento_hash_64
#define hash_64i memento_hash_64i
#define MurmurHash64A memento_MurmurHash64A
#define MurmurHash64B memento_MurmurHash64B
#define kSelectInByte memento_kSelectInByte

#include "memento.c"
#include "hashutil.c"

void * mementoNew( uint64_t nslots , int keyBits , int r )
{
 QF * f = ( QF * )malloc( sizeof( QF ) );
 if( ! qf_malloc( f , nslots , ( uint64_t )keyBits , ( uint64_t )r ,
		  QF_HASH_DEFAULT , 1 ) ) {
  free( f );
  return( NULL );
  }
 return( f );
 }

int mementoInsert( void * f , uint64_t key )
{
 const QF * q = ( const QF * )f;
 const uint64_t r = q->metadata->memento_bits;
 return( qf_insert_single( ( QF * )f , key >> r ,
			    key & ( ( ( uint64_t )1 << r ) - 1 ) ,
			    QF_NO_LOCK ) >= 0 );
 }

int mementoRange( void * f , uint64_t a , uint64_t b )
{
 const QF * q = ( const QF * )f;
 const uint64_t r = q->metadata->memento_bits , m = ( ( uint64_t )1 << r ) - 1;
 if( a == b )
  return( qf_point_query( q , a >> r , a & m , QF_NO_LOCK ) > 0 );
 return( qf_range_query( q , a >> r , a & m , b >> r , b & m ,
			 QF_NO_LOCK ) > 0 );
 }

uint64_t mementoBytes( const void * f )
{
 return( qf_get_total_size_in_bytes( ( const QF * )f ) );
 }

void mementoFree( void * f )
{
 qf_free( ( QF * )f );
 free( f );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File qf_memento.c --------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*-------------------------- File pma_authors.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The CPMA of wheatman/Packed-Memory-Array behind the functions of
 * PmaCompetitors.h, compiled alone with the headers of that repository
 * (include/, ParallelTools/, tlx/, parlaylib/include/ and
 * StructOfArrays/include/) and the flags of its makefile, twice: with
 * PARLAY=0 and PARLAY_SEQUENTIAL for bench/ordered.cpp, the sequential
 * code with which its authors time the point operations, and with
 * PARLAY=1 for the parallel batches of bench/joins.cpp; VQSORT=0 in both,
 * i.e., its sort of ParlayLib instead of the one of Highway, which the
 * benchmarks never call since they pass their batches sorted.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/

#include "PmaCompetitors.h"

#include "PMA/CPMA.hpp"

namespace pma_authors
{

using U = CPMA< spmae_settings< std::uint64_t > >;   ///< uncompressed
using Z = CPMA< scpmae_settings< std::uint64_t > >;  ///< compressed

/// f on the set s, of the type of its kind
template< class F >
decltype( auto ) on( void * s , bool compressed , F && f )
{
 if( compressed )
  return( f( *static_cast< Z * >( s ) ) );
 return( f( *static_cast< U * >( s ) ) );
 }

/// f on the set s, of the type of its kind
template< class F >
decltype( auto ) on( const void * s , bool compressed , F && f )
{
 if( compressed )
  return( f( *static_cast< const Z * >( s ) ) );
 return( f( *static_cast< const U * >( s ) ) );
 }

void * make( bool compressed )
{
 if( compressed )
  return( new Z() );
 return( new U() );
 }

void * build( bool compressed , std::uint64_t * x , std::size_t n )
{
 if( compressed )
  return( new Z( x , x + n ) );
 return( new U( x , x + n ) );
 }

void destroy( void * s , bool compressed )
{
 if( compressed )
  delete static_cast< Z * >( s );
 else
  delete static_cast< U * >( s );
 }

bool insert( void * s , bool compressed , std::uint64_t k )
{
 return(
   on( s , compressed , [ & ]( auto & p ) { return( p.insert( k ) ); } ) );
 }

bool remove( void * s , bool compressed , std::uint64_t k )
{
 return(
   on( s , compressed , [ & ]( auto & p ) { return( p.remove( k ) ); } ) );
 }

bool has( const void * s , bool compressed , std::uint64_t k )
{
 return(
   on( s , compressed , [ & ]( const auto & p ) { return( p.has( k ) ); } ) );
 }

std::uint64_t insertBatch( void * s , bool compressed , std::uint64_t * x ,
			   std::size_t n , bool sorted )
{
 return( on( s , compressed , [ & ]( auto & p ) {
  return( std::uint64_t( p.insert_batch( x , n , sorted ) ) );
  } ) );
 }

std::uint64_t removeBatch( void * s , bool compressed , std::uint64_t * x ,
			   std::size_t n , bool sorted )
{
 return( on( s , compressed , [ & ]( auto & p ) {
  return( std::uint64_t( p.remove_batch( x , n , sorted ) ) );
  } ) );
 }

std::uint64_t scan( const void * s , bool compressed , std::uint64_t k ,
		    std::size_t c )
{
 return( on( s , compressed , [ & ]( const auto & p ) {
  std::uint64_t t = 0;
  p.map_range_length( [ & ]( auto e ) { t += std::uint64_t( e ); } , k , c );
  return( t );
  } ) );
 }

std::uint64_t sum( const void * s , bool compressed )
{
 return( on( s , compressed , [ & ]( const auto & p ) {
  return( std::uint64_t( p.sum() ) + std::uint64_t( p.size() ) );
  } ) );
 }

std::size_t bytes( const void * s , bool compressed )
{
 return( on( s , compressed , [ & ]( const auto & p ) {
  return( std::size_t( p.get_size() ) );
  } ) );
 }

}  // end( namespace pma_authors )

/*--------------------------------------------------------------------------*/
/*------------------------ End File pma_authors.cpp ------------------------*/
/*--------------------------------------------------------------------------*/

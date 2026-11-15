/*--------------------------------------------------------------------------*/
/*--------------------------- File hashattack.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The experiment of exercise 5.11 of Morin's book: n integer keys chosen so
 * that a hash table puts c n of them in one place, which makes a search
 * take linear time, against n random keys.
 *
 * - std::unordered_map of libstdc++ hashes an integer to itself and takes
 *   it modulo its prime number of buckets P: after `reserve( n )`, the keys
 *   0, P, 2 P, ... all fall in the bucket 0;
 * - the `ChainedHashTable` and the `OpenHashTable` of ADS with
 *   `std::hash` take \f$ h( k ) \bmod m \f$ on \f$ m = 2^d \f$ places: the
 *   multiples of \f$ 2^{20} \f$ all fall in the place 0 while
 *   \f$ m \le 2^{20} \f$;
 * - the same tables with the multiplicative hashing and the tabulation
 *   hashing of Morin's book [see `MultiplicativeHash` and
 *   `TabulationHash`], whose random multiplier or tables, unknown to who
 *   chooses the keys, spread the same keys over the places.
 *
 * The cases are "find-attack", n searches of the keys of the attack, and
 * "find-random", n searches of n random keys, each in nanoseconds per
 * search, the median of r runs, with the contracts off. The attack makes
 * the searches of the first two kinds of tables take \f$ \Theta( n ) \f$
 * each, which the cells show growing linearly with n, and those of the
 * randomized ones \f$ O( 1 ) \f$.
 *
 *   hashattack [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/UniversalHashing.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

using K = std::uint64_t;

/// the median time of the searches of the keys Q in the table that
/// `make()` builds with the keys S
template< class M , class F >
void measure( const char * what , const std::string & who ,
	      const std::vector< K > & S , const std::vector< K > & Q , int r ,
	      M && make , F && has )
{
 if( ADS::bench::skip( who ) )
  return;
 const auto t = make( S );
 const double ns = ADS::bench::median( [ & ]() { return( 0 ); } ,
				       [ & ]( int ) {
					std::uint64_t c = 0;
					for( const K k : Q )
					 c += has( *t , k ) ? 1 : 0;
					ADS::bench::sink =
					  ADS::bench::sink + c;
				        } ,
				       r , Q.size() );
 ADS::bench::print( "hashattack" , what , S.size() , who , ns );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const ADS::bench::Options o( argc , argv , { 1000 , 4000 , 16000 } );
 for( const std::size_t n : o.sizes ) {
  std::unordered_map< K , K > probe;
  probe.reserve( n );
  const K P = K( probe.bucket_count() );
  std::vector< K > stdAttack( n ) , powAttack( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   stdAttack[ i ] = K( i ) * P;
   powAttack[ i ] = K( i ) << 20;
   }
  const std::vector< K > random = ADS::bench::keys( n , o.seed );
  for( int c = 0 ; c < 2 ; ++c ) {
   const char * what = c == 0 ? "find-attack" : "find-random";
   const auto & SS = c == 0 ? stdAttack : random;
   const auto & SP = c == 0 ? powAttack : random;
   measure(
     what , "std::unordered_map" , SS , SS , o.runs ,
     [ & ]( const std::vector< K > & S ) {
      auto m = std::make_unique< std::unordered_map< K , K > >();
      m->reserve( n );
      for( const K k : S )
       m->emplace( k , k );
      return( m );
      } ,
     []( const auto & m , K k ) { return( m.count( k ) > 0 ); } );
   measure(
     what , "ChainedHashTable/std::hash" , SP , SP , o.runs ,
     [ & ]( const std::vector< K > & S ) {
      auto t = std::make_unique< ADS::ChainedHashTable< K , K > >( 1024 , 1.0 ,
								  1.0 / 3 );
      for( const K k : S )
       t->insert( k , k );
      return( t );
      } ,
     []( const auto & t , K k ) { return( t.contains( k ) ); } );
   using MH = ADS::MultiplicativeHash< K >;
   measure(
     what , "ChainedHashTable/multiplicative" , SP , SP , o.runs ,
     [ & ]( const std::vector< K > & S ) {
      auto t = std::make_unique<
	ADS::ChainedHashTable< K , K , ADS::Chain::FRONT , MH > >(
	1024 , 1.0 , 1.0 / 3 , MH( o.seed ) );
      for( const K k : S )
       t->insert( k , k );
      return( t );
      } ,
     []( const auto & t , K k ) { return( t.contains( k ) ); } );
   measure(
     what , "OpenHashTable/std::hash" , SP , SP , o.runs ,
     [ & ]( const std::vector< K > & S ) {
      auto t = std::make_unique< ADS::OpenHashTable< K , K > >(
	1024 , ADS::Probing::linear( 1 ) , true );
      for( const K k : S )
       t->insert( k , k );
      return( t );
      } ,
     []( const auto & t , K k ) { return( t.contains( k ) ); } );
   measure(
     what , "OpenHashTable/tabulation" , SP , SP , o.runs ,
     [ & ]( const std::vector< K > & S ) {
      auto t =
	std::make_unique< ADS::OpenHashTable< K , K , ADS::TabulationHash > >(
	  1024 , ADS::Probing::linear( 1 ) , true ,
	  ADS::TabulationHash( o.seed ) );
      for( const K k : S )
       t->insert( k , k );
      return( t );
      } ,
     []( const auto & t , K k ) { return( t.contains( k ) ); } );
   }
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File hashattack.cpp -------------------------*/
/*--------------------------------------------------------------------------*/

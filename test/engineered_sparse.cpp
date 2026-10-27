/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_sparse.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered structures of the rows of a model, beyond the
 * book: the index of the positions of objects against std::unordered_map,
 * with its renumbering after the erasure of positions, under good and
 * adversarial hashes; the assembly of the compressed forms from triplets
 * against an assembly by std::map; the compressed rows that grow and
 * shrink against a vector of rows; and the fingerprints of the rows with
 * their zeros and NaNs, at the level 3 of the contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/dictionary/PositionIndex.h"
#include "ADS/dictionary/RowFingerprint.h"
#include "ADS/numeric/DynamicCsr.h"
#include "ADS/numeric/SparseAssembly.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// an object of a model
struct Row {
 int id = 0;
 };

/// the worst hash: every key at home 7
struct Constant {
 std::size_t operator()( const Row * ) const { return( 7 ); }
 };

/// a hash of few homes, the address over 64: long runs that wrap
struct Coarse {
 std::size_t operator()( const Row * p ) const {
  return( std::size_t( reinterpret_cast< std::uintptr_t >( p ) >> 6 ) );
  }
 };

/// the index of v rebuilt by hand
template< class H >
bool indexes( const PositionIndex< Row , H > & X ,
              const std::vector< Row * > & v )
{
 if( X.size() != v.size() )
  return( false );
 for( std::size_t i = 0 ; i < v.size() ; ++i )
  if( X.find( v[ i ] ) != i )
   return( false );
 std::size_t n = 0;
 bool ok = true;
 X.forEach( [ & ]( const Row * p , std::uint32_t i ) {
  ++n;
  ok = ok && ( i < v.size() ) && ( v[ i ] == p ); } );
 return( ok && ( n == v.size() ) );
 }

/// random erasures of positions from a vector of n objects and its index,
/// in rounds, the index renumbered and compared with the vector
template< class H >
void erasures( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< Row > rows( 2 * n );
 std::vector< Row * > v;
 for( std::size_t i = 0 ; i < n ; ++i )
  v.push_back( &rows[ i ] );
 PositionIndex< Row , H > X;
 X.assign( v );
 CHECK( indexes( X , v ) );
 std::size_t next = n;
 for( int round = 0 ; round < 12 && ! v.empty() ; ++round ) {
  std::vector< std::uint32_t > E;
  switch( round % 6 ) {
   case 0: E = { 0 }; break;                           // the first
   case 1: E = { std::uint32_t( v.size() - 1 ) }; break;  // the last
   case 2:                                             // a block
    for( std::size_t i = v.size() / 3 ; i < v.size() / 2 ; ++i )
     E.push_back( std::uint32_t( i ) );
    break;
   case 3: break;                                      // none
   default:                                            // a random half
    for( std::size_t i = 0 ; i < v.size() ; ++i )
     if( g() % 2 )
      E.push_back( std::uint32_t( i ) );
   }
  for( auto e = E.rbegin() ; e != E.rend() ; ++e )
   v.erase( v.begin() + *e );
  X.renumber( E );
  CHECK( indexes( X , v ) );
  for( std::size_t k = 0 ; k < n / 8 && next < rows.size() ; ++k ) {
   X.insert( &rows[ next ] , std::uint32_t( v.size() ) );   // appended
   v.push_back( &rows[ next++ ] );
   }
  CHECK( indexes( X , v ) );
  }
 std::vector< std::uint32_t > all( v.size() );       // everything erased
 for( std::size_t i = 0 ; i < all.size() ; ++i )
  all[ i ] = std::uint32_t( i );
 X.renumber( all );
 CHECK( X.empty() && ( X.find( &rows[ 0 ] ) == X.NONE ) );
 }

/// the compressed form of triplets by std::map, by major index
std::map< std::pair< long , long > , double > byMap(
 const std::vector< long > & major , const std::vector< long > & minor ,
 const std::vector< double > & value )
{
 std::map< std::pair< long , long > , double > M;
 for( std::size_t k = 0 ; k < major.size() ; ++k )
  M[ { major[ k ] , minor[ k ] } ] += value[ k ];
 return( M );
 }

/// true if beg, ind and val are the compressed form of M, the minor
/// indices increasing in each major one
template< class I >
bool compressed( const std::map< std::pair< long , long > , double > & M ,
                 std::size_t nmajor , const std::vector< I > & beg ,
                 const std::vector< I > & ind ,
                 const std::vector< double > & val , std::size_t z )
{
 if( ( beg.size() != nmajor + 1 ) || ( beg[ 0 ] != 0 ) ||
     ( std::size_t( beg[ nmajor ] ) != z ) || ( z != M.size() ) )
  return( false );
 auto it = M.begin();
 for( std::size_t i = 0 ; i < nmajor ; ++i )
  for( I k = beg[ i ] ; k < beg[ i + 1 ] ; ++k , ++it )
   if( ( it == M.end() ) || ( it->first.first != long( i ) ) ||
       ( it->first.second != long( ind[ std::size_t( k ) ] ) ) ||
       ( std::abs( it->second - val[ std::size_t( k ) ] ) > 1e-9 ) )
    return( false );
 return( it == M.end() );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// PositionIndex against std::unordered_map: random insertions, replaced
// positions, erasures and lookups of keys present and absent, across many
// doublings, with the hash of ADS, a constant hash and a coarse one; the
// empty index, clear(), reserve(), the copies and moves, the contracts
ADS_TEST( "position-index" )
{
 PositionIndex< Row > E;
 Row a;
 CHECK( E.empty() && E.capacity() == 0 && E.find( &a ) == E.NONE &&
        ! E.erase( &a ) && ! E.contains( &a ) );
 CHECK_THROWS( E.insert( nullptr , 0 ) , contract_violation );
 CHECK_THROWS( E.insert( &a , E.NONE ) , contract_violation );
 CHECK_THROWS( (void) E.find( nullptr ) , contract_violation );
 CHECK( E.insert( &a , 5 ) && ! E.insert( &a , 6 ) && E.find( &a ) == 6 &&
        E.size() == 1 && E.capacity() >= 1 );
 CHECK( E.erase( &a ) && E.empty() && ! E.erase( &a ) );
 E.reserve( 1000 );
 CHECK( E.capacity() >= 1000 && E.empty() );
 auto random = [ & ]< class H >( H , std::size_t keys , int steps ) {
  std::mt19937_64 g( keys );
  std::vector< Row > rows( keys );
  PositionIndex< Row , H > X;
  std::unordered_map< const Row * , std::uint32_t > U;
  for( int t = 0 ; t < steps ; ++t ) {
   const Row * p = &rows[ g() % keys ];
   const std::uint32_t i = std::uint32_t( g() % 1000000 );
   switch( g() % 4 ) {
    case 0: case 1:
     CHECK( X.insert( p , i ) == U.insert_or_assign( p , i ).second );
     break;
    case 2: CHECK( X.erase( p ) == ( U.erase( p ) == 1 ) ); break;
    default: {
     const auto it = U.find( p );
     CHECK( X.find( p ) == ( it == U.end() ? X.NONE : it->second ) );
     }
    }
   CHECK( X.size() == U.size() );
   }
  for( const auto & [ p , i ] : U )
   CHECK( X.find( p ) == i );
  PositionIndex< Row , H > Y( X ) , Z;
  Z = std::move( X );
  CHECK( X.empty() && Y.size() == U.size() && Z.size() == U.size() );
  for( const auto & [ p , i ] : U )
   CHECK( Y.find( p ) == i && Z.find( p ) == i );
  Z.clear();
  CHECK( Z.empty() && Z.capacity() == Y.capacity() &&
         Z.find( U.begin()->first ) == Z.NONE );
  };
 random( FastHash< const Row * >() , 50 , 20000 );
 random( FastHash< const Row * >() , 5000 , 200000 );
 random( Constant() , 300 , 20000 );
 random( Coarse() , 3000 , 100000 );
 }

// renumber(): the erasure of the first, the last, a block, nothing, a
// random half and everything, the appended keys between, on small and
// large indices, with the hash of ADS and the adversarial ones whose runs
// cover most of the table and wrap; the contracts of the positions
ADS_TEST( "position-index-renumber" )
{
 erasures< FastHash< const Row * > >( 1 , 1 );
 erasures< FastHash< const Row * > >( 100 , 2 );
 erasures< FastHash< const Row * > >( 30000 , 3 );
 erasures< Constant >( 200 , 4 );
 erasures< Coarse >( 5000 , 5 );
 Row r[ 3 ];
 PositionIndex< Row > X;
 X.insert( &r[ 0 ] , 10 );
 X.insert( &r[ 1 ] , 20 );
 X.insert( &r[ 2 ] , 3 );
 X.renumber( std::vector< std::uint32_t >{ 4 , 5 } );   // past the last
 CHECK( X.find( &r[ 0 ] ) == 8 && X.find( &r[ 1 ] ) == 18 &&
        X.find( &r[ 2 ] ) == 3 );
 X.renumber( std::vector< std::uint32_t >{} );
 CHECK( X.size() == 3 && X.find( &r[ 2 ] ) == 3 );
 CHECK_THROWS( X.renumber( std::vector< std::uint32_t >{ 2 , 1 } ) ,
               contract_violation );
 CHECK_THROWS( X.renumber( std::vector< std::uint32_t >{ 1 , 1 } ) ,
               contract_violation );
 PositionIndex< Row > E;                    // nothing to renumber
 E.renumber( std::vector< std::uint32_t >{ 0 , 7 } );
 CHECK( E.empty() );
 }

// the assembly of the triplets against std::map: random matrices with
// repetitions, by columns and by rows, with indices of 32 and 64 bits,
// signed and unsigned, a workspace kept across calls; no triplets, one,
// all on one place (summed to 0), a column of 1000 places each twice in
// the reverse order, empty rows and columns, a matrix of no rows, the
// contracts of the indices
ADS_TEST( "sparse-assembly" )
{
 std::mt19937_64 g( 11 );
 AssemblyWorkspace< int , double > W;
 auto check = [ & ]( std::size_t m , std::size_t n ,
                     const std::vector< long > & R ,
                     const std::vector< long > & C ,
                     const std::vector< double > & V ) {
  const std::size_t z = R.size();
  std::vector< int > beg( n + 1 ) , ind( z + 1 );
  std::vector< double > val( z + 1 );
  std::size_t k = assembleCsc( m , n , z , R.data() , C.data() , V.data() ,
                               beg.data() , ind.data() , val.data() , W );
  CHECK( compressed( byMap( C , R , V ) , n , beg , ind , val , k ) );
  std::vector< unsigned > beg2( m + 1 ) , ind2( z + 1 );
  AssemblyWorkspace< unsigned , double > W2;
  k = assembleCsr( m , n , z , R.data() , C.data() , V.data() ,
                   beg2.data() , ind2.data() , val.data() , W2 );
  CHECK( compressed( byMap( R , C , V ) , m , beg2 , ind2 , val , k ) );
  std::vector< std::int64_t > beg3( m + 1 ) , ind3( z + 1 );
  k = compressTriplets( m , n , z , R.data() , C.data() , V.data() ,
                        beg3.data() , ind3.data() , val.data() );
  CHECK( compressed( byMap( R , C , V ) , m , beg3 , ind3 , val , k ) );
  };
 for( int round = 0 ; round < 30 ; ++round ) {
  const std::size_t m = 1 + g() % 60 , n = 1 + g() % 60 ,
                    z = g() % 2000;
  std::vector< long > R( z ) , C( z );
  std::vector< double > V( z );
  for( std::size_t k = 0 ; k < z ; ++k ) {
   R[ k ] = long( g() % ( m / 2 + 1 ) );    // half the rows empty
   C[ k ] = long( g() % n );
   V[ k ] = double( long( g() % 21 ) - 10 );
   }
  check( m , n , R , C , V );
  }
 check( 5 , 7 , {} , {} , {} );
 check( 0 , 3 , {} , {} , {} );
 check( 3 , 0 , {} , {} , {} );
 check( 1 , 1 , { 0 } , { 0 } , { 2.5 } );
 check( 4 , 4 , { 3 , 3 , 3 , 3 } , { 2 , 2 , 2 , 2 } ,
        { 1 , -1 , 2 , -2 } );               // summed to 0, kept
 {                                          // one long column, reversed
  std::vector< long > R , C;
  std::vector< double > V;
  for( long i = 999 ; i >= 0 ; --i )
   for( int c = 0 ; c < 2 ; ++c ) {
    R.push_back( i );
    C.push_back( 0 );
    V.push_back( double( i + c ) );
    }
  check( 1000 , 1 , R , C , V );
  }
 std::vector< long > R( 100000 ) , C( 100000 );
 std::vector< double > V( 100000 , 1 );
 for( std::size_t k = 0 ; k < R.size() ; ++k ) {
  R[ k ] = long( g() % 3000 );
  C[ k ] = long( g() % 3000 );
  }
 check( 3000 , 3000 , R , C , V );
 int beg[ 3 ] , ind[ 2 ];
 double val[ 2 ];
 const long r[ 2 ] = { 0 , 2 } , c[ 2 ] = { 0 , 0 } , neg[ 2 ] = { 0 , -1 };
 const double v[ 2 ] = { 1 , 1 };
 CHECK_THROWS( assembleCsr( 2 , 2 , 2 , r , c , v , beg , ind , val , W ) ,
               contract_violation );
 CHECK_THROWS( assembleCsr( 2 , 2 , 2 , c , neg , v , beg , ind , val , W ) ,
               contract_violation );
 }

// DynamicCsr against a vector of rows: random appends of single rows and
// of batches, removals of random sets, of the first and the last row, of
// ranges and of everything, with rows of no entries; the arrays beg, ind
// and val always the compressed form; the contracts
ADS_TEST( "dynamic-csr" )
{
 using Entries = std::vector< std::pair< int , double > >;
 std::mt19937_64 g( 5 );
 DynamicCsr<> D;
 std::vector< Entries > V;
 CHECK( D.empty() && D.rows() == 0 && D.nonzeros() == 0 && D.beg()[ 0 ] == 0 );
 auto same = [ & ]( void ) {
  if( D.rows() != V.size() )
   return( false );
  std::size_t z = 0;
  for( std::size_t i = 0 ; i < V.size() ; ++i ) {
   if( std::size_t( D.beg()[ i ] ) != z )
    return( false );
   const auto x = D.indices( i );
   const auto y = D.values( i );
   if( x.size() != V[ i ].size() )
    return( false );
   for( std::size_t k = 0 ; k < x.size() ; ++k )
    if( ( x[ k ] != V[ i ][ k ].first ) || ( y[ k ] != V[ i ][ k ].second ) )
     return( false );
   z += x.size();
   }
  return( ( std::size_t( D.beg()[ V.size() ] ) == z ) &&
          ( D.nonzeros() == z ) );
  };
 auto row = [ & ]( void ) {
  Entries e;
  const std::size_t k = g() % 4 == 0 ? 0 : g() % 30;
  for( std::size_t j = 0 ; j < k ; ++j )
   e.emplace_back( int( g() % 1000 ) , double( g() % 100 ) );
  return( e );
  };
 for( int t = 0 ; t < 3000 ; ++t ) {
  const unsigned c = unsigned( g() % 6 );
  if( c <= 1 ) {                              // one row
   const Entries e = row();
   std::vector< int > x;
   std::vector< double > y;
   for( auto [ j , v ] : e ) { x.push_back( j ); y.push_back( v ); }
   CHECK( D.append( x , y ) == V.size() );
   V.push_back( e );
   }
  else if( c == 2 ) {                         // a batch, offset in its arrays
   const std::size_t r = g() % 5;
   std::vector< int > beg{ 3 } , x( 3 , -1 );
   std::vector< double > y( 3 , -1 );
   for( std::size_t k = 0 ; k < r ; ++k ) {
    const Entries e = row();
    for( auto [ j , v ] : e ) { x.push_back( j ); y.push_back( v ); }
    beg.push_back( int( x.size() ) );
    V.push_back( e );
    }
   D.append( r , beg.data() , x.data() , y.data() );
   }
  else if( c == 3 && ! V.empty() ) {          // a random set
   std::vector< std::uint32_t > R;
   for( std::size_t i = 0 ; i < V.size() ; ++i )
    if( g() % 3 == 0 )
     R.push_back( std::uint32_t( i ) );
   D.remove( R );
   for( auto i = R.rbegin() ; i != R.rend() ; ++i )
    V.erase( V.begin() + *i );
   }
  else if( c == 4 && ! V.empty() ) {          // the first or the last
   const std::size_t i = g() % 2 ? 0 : V.size() - 1;
   const std::vector< std::size_t > R{ i };
   D.remove( std::span< const std::size_t >( R ) );
   V.erase( V.begin() + i );
   }
  else if( ! V.empty() ) {                    // a range
   const std::size_t a = g() % V.size() , b = a + g() % ( V.size() - a + 1 );
   D.remove( a , b );
   V.erase( V.begin() + a , V.begin() + b );
   }
  CHECK( same() );
  }
 std::vector< int > all( V.size() );
 for( std::size_t i = 0 ; i < all.size() ; ++i )
  all[ i ] = int( i );
 D.remove( all );
 CHECK( D.empty() && D.nonzeros() == 0 );
 D.append( std::vector< int >{ 1 } , std::vector< double >{ 2 } );
 D.values( 0 )[ 0 ] = 3;
 CHECK( D.val()[ 0 ] == 3 );
 D.clear();
 CHECK( D.empty() && D.beg()[ 0 ] == 0 );
 CHECK_THROWS( D.append( std::vector< int >{ 1 } , std::vector< double >{} ) ,
               contract_violation );
 D.append( std::vector< int >{} , std::vector< double >{} );
 CHECK_THROWS( D.remove( std::span< const int >( std::vector< int >{ 1 } ) ) ,
               contract_violation );
 CHECK_THROWS( D.remove( 0 , 2 ) , contract_violation );
 CHECK_THROWS( (void) D.indices( 1 ) , contract_violation );
 }

// the fingerprints: +0 and -0, the NaNs of any sign and payload, equal;
// the lengths across the blocks of 64 words; the rows that differ in one
// value, the seeds, the dense and the sparse rows; no collision among
// 10^5 random rows; sameRow() on the same cases
ADS_TEST( "row-fingerprint" )
{
 const double nan1 = std::numeric_limits< double >::quiet_NaN();
 const double nan2 = -std::bit_cast< double >( 0x7FF0000000000123ULL );
 CHECK( std::isnan( nan2 ) && canonicalWord( nan1 ) == canonicalWord( nan2 )
        && canonicalWord( 0.0 ) == canonicalWord( -0.0 ) &&
        canonicalWord( 1.0 ) != canonicalWord( -1.0 ) );
 CHECK( rowFingerprint( std::vector< double >{} ) ==
        rowFingerprint( std::vector< double >{} ) );
 std::mt19937_64 g( 9 );
 for( std::size_t n : { 1 , 2 , 7 , 8 , 63 , 64 , 65 , 128 , 129 , 1000 } ) {
  std::vector< double > x( n );
  for( auto & v : x )
   v = double( g() % 1000 ) / 7 - 50;
  x[ 0 ] = 0.0;
  std::vector< double > y = x;
  y[ 0 ] = -0.0;
  x[ n - 1 ] = nan1;
  y[ n - 1 ] = nan2;
  CHECK( rowFingerprint( x ) == rowFingerprint( y ) && sameRow( x , y ) );
  CHECK( rowFingerprint( x , 1 ) != rowFingerprint( x , 2 ) );
  y[ ( n - 1 ) / 2 ] += n == 1 ? 0 : 1;   // not the NaN at the end
  if( n > 1 )
   CHECK( rowFingerprint( x ) != rowFingerprint( y ) && ! sameRow( x , y ) );
  std::vector< double > s( x.begin() , x.end() - 1 );
  CHECK( rowFingerprint( s ) != rowFingerprint( x ) && ! sameRow( s , x ) );
  std::vector< int > j( n );
  for( std::size_t k = 0 ; k < n ; ++k )
   j[ k ] = int( 3 * k );
  const auto f = rowFingerprint( n , j.data() , x.data() );
  y = x;
  if( x[ 0 ] == 0 )                         // not the NaN of n = 1
   y[ 0 ] = -0.0;
  CHECK( f == rowFingerprint( n , j.data() , y.data() ) &&
         sameRow( n , j.data() , x.data() , n , j.data() , y.data() ) );
  const std::vector< int > j0 = j;
  j[ n - 1 ] += 1;
  CHECK( f != rowFingerprint( n , j.data() , x.data() ) &&
         ! sameRow( n , j0.data() , x.data() , n , j.data() , x.data() ) );
  CHECK( f != rowFingerprint( x ) );        // dense and sparse apart
  }
 std::unordered_set< std::uint64_t > F;
 for( int r = 0 ; r < 100000 ; ++r ) {
  std::vector< double > x( 1 + g() % 20 );
  for( auto & v : x )
   v = double( g() % 3 );                   // many rows alike
  x[ 0 ] = double( r );
  F.insert( rowFingerprint( x ) );
  }
 CHECK( F.size() == 100000 );
 const int i1[ 2 ] = { 1 , 2 } , i2[ 2 ] = { 1 , 3 };
 const double v1[ 2 ] = { 1 , -0.0 } , v2[ 2 ] = { 1 , 0.0 };
 CHECK( sameRow( 2 , i1 , v1 , 2 , i1 , v2 ) &&
        ! sameRow( 2 , i1 , v1 , 2 , i2 , v1 ) &&
        ! sameRow( 2 , i1 , v1 , 1 , i1 , v1 ) &&
        sameRow< int >( 0 , nullptr , nullptr , 0 , nullptr , nullptr ) );
 CHECK_THROWS( (void) rowFingerprint< int >( 1 , nullptr , v1 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File engineered_sparse.cpp --------------------*/
/*--------------------------------------------------------------------------*/

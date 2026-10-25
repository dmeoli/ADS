/*--------------------------------------------------------------------------*/
/*---------------------------- File assembly.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the assembly of the sparse matrices for the solvers of
 * linear programs, in the arrays beg, ind and val by columns with indices
 * of type int and values of type double:
 *
 * - triplets: n triplets ( i , j , v ) of an m x m matrix, m = n / 10, in a
 *   random order (case random, few places repeated) or each place twice
 *   (case repeated), into the compressed sparse columns, the values of a
 *   place summed: ADS::assembleCsc() into arrays of the caller with a
 *   workspace kept across the runs, against setFromTriplets() of a
 *   SparseMatrix of Eigen by columns (when Eigen is found), which sums the
 *   repetitions too, and against a std::map of the places whose entries are
 *   then written in the arrays; ns per triplet;
 * - bundle: the cuts of a decomposition, n / 20 rows of 20 entries, then
 *   20 rounds that append n / 200 new rows, remove as many random ones and
 *   hand the matrix to a solver as beg, ind and val: ADS::DynamicCsr,
 *   whose arrays are the matrix, against a std::vector of rows, each a
 *   std::vector of pairs ( index , value ), copied into the three arrays at
 *   each round; ns per entry of the matrix and per round.
 *
 * Each cell is the median of r runs; the building of the triplets and of
 * the rows to append is not timed; the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   assembly [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

#include "ADS/numeric/DynamicCsr.h"
#include "ADS/numeric/SparseAssembly.h"

#if __has_include( <Eigen/Sparse> )
 #include <Eigen/Sparse>
 #define ADS_HAS_EIGEN 1
#endif

#include <map>
#include <set>
#include <utility>

using namespace ADS::bench;

namespace {

/// the triplets of a case
struct Triplets {
 std::size_t m = 0;
 std::vector< int > row , col;
 std::vector< double > val;
 };

/// n triplets of an m x m matrix, m = n / 10, each place twice if twice
Triplets triplets( std::size_t n , bool twice , unsigned seed )
{
 std::mt19937_64 g( seed );
 Triplets T;
 T.m = std::max< std::size_t >( 1 , n / 10 );
 const std::size_t k = twice ? n / 2 : n;
 for( std::size_t t = 0 ; t < k ; ++t ) {
  const int i = int( g() % T.m ) , j = int( g() % T.m );
  for( int c = 0 ; c < ( twice ? 2 : 1 ) ; ++c ) {
   T.row.push_back( i );
   T.col.push_back( j );
   T.val.push_back( double( g() % 1000 ) / 7 );
   }
  }
 std::vector< std::size_t > p( T.row.size() );
 for( std::size_t i = 0 ; i < p.size() ; ++i )
  p[ i ] = i;
 std::shuffle( p.begin() , p.end() , g );
 Triplets S = T;
 for( std::size_t i = 0 ; i < p.size() ; ++i ) {
  S.row[ i ] = T.row[ p[ i ] ];
  S.col[ i ] = T.col[ p[ i ] ];
  S.val[ i ] = T.val[ p[ i ] ];
  }
 return( S );
 }

/// the arrays of a solver
struct Arrays {
 std::vector< int > beg , ind;
 std::vector< double > val;
 };

/// the case triplets
void assembly( const char * what , const Triplets & T , std::size_t n ,
               int r )
{
 const std::size_t z = T.row.size() , m = T.m;
 Arrays A;
 A.beg.resize( m + 1 );
 A.ind.resize( z );
 A.val.resize( z );
 ADS::AssemblyWorkspace< int , double > W;
 auto ads = [ & ]( int ) {
  sink = sink + ADS::assembleCsc( m , m , z , T.row.data() , T.col.data() ,
                                  T.val.data() , A.beg.data() ,
                                  A.ind.data() , A.val.data() , W ); };
 for( const char * who : { "ADS::assembleCsc" , "ADS::assembleCsc A/A" } )
  if( ! skip( who ) )
   print( "assembly" , what , n , who , median( [](){ return( 0 ); } , ads ,
                                                r , z ) );
#ifdef ADS_HAS_EIGEN
 if( ! skip( "Eigen::setFromTriplets" ) ) {
  using E = Eigen::Triplet< double , int >;
  std::vector< E > ET;
  for( std::size_t k = 0 ; k < z ; ++k )
   ET.emplace_back( T.row[ k ] , T.col[ k ] , T.val[ k ] );
  print( "assembly" , what , n , "Eigen::setFromTriplets" ,
         median( [](){ return( 0 ); } , [ & ]( int ) {
          Eigen::SparseMatrix< double , Eigen::ColMajor , int > S{
           int( m ) , int( m ) };
          S.setFromTriplets( ET.begin() , ET.end() );
          sink = sink + std::uint64_t( S.nonZeros() ); } , r , z ) );
  }
#endif
 if( skip( "std::map" ) )
  return;
 print( "assembly" , what , n , "std::map" , median( [](){ return( 0 ); } ,
        [ & ]( int ) {
         std::map< std::pair< int , int > , double > M;
         for( std::size_t k = 0 ; k < z ; ++k )
          M[ { T.col[ k ] , T.row[ k ] } ] += T.val[ k ];
         std::fill( A.beg.begin() , A.beg.end() , 0 );
         std::size_t q = 0;
         for( const auto & [ p , v ] : M ) {
          ++A.beg[ std::size_t( p.first ) + 1 ];
          A.ind[ q ] = p.second;
          A.val[ q++ ] = v;
          }
         for( std::size_t j = 0 ; j < m ; ++j )
          A.beg[ j + 1 ] += A.beg[ j ];
         sink = sink + q; } , r , z ) );
 }

/// the rows of the case bundle
struct Bundle {
 std::vector< std::vector< int > > ind;            ///< the first rows
 std::vector< std::vector< double > > val;
 std::vector< std::vector< std::vector< int > > > nind;   ///< by round
 std::vector< std::vector< std::vector< double > > > nval;
 std::vector< std::vector< std::uint32_t > > gone;         ///< by round
 };

constexpr int ROUNDS = 20 , WIDTH = 20;

Bundle bundle( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 Bundle B;
 const std::size_t R = std::max< std::size_t >( 1 , n / WIDTH ) ,
                   k = std::max< std::size_t >( 1 , n / 200 );
 auto row = [ & ]( std::vector< int > & x , std::vector< double > & y ) {
  std::set< int > s;
  while( s.size() < WIDTH )
   s.insert( int( g() % ( 10 * R ) ) );
  x.assign( s.begin() , s.end() );
  y.resize( WIDTH );
  for( auto & v : y )
   v = double( g() % 1000 ) / 7;
  };
 B.ind.resize( R );
 B.val.resize( R );
 for( std::size_t i = 0 ; i < R ; ++i )
  row( B.ind[ i ] , B.val[ i ] );
 B.nind.resize( ROUNDS );
 B.nval.resize( ROUNDS );
 B.gone.resize( ROUNDS );
 for( int t = 0 ; t < ROUNDS ; ++t ) {
  B.nind[ t ].resize( k );
  B.nval[ t ].resize( k );
  for( std::size_t i = 0 ; i < k ; ++i )
   row( B.nind[ t ][ i ] , B.nval[ t ][ i ] );
  std::set< std::uint32_t > s;               // of the R + k rows then
  while( s.size() < k )
   s.insert( std::uint32_t( g() % ( R + k ) ) );
  B.gone[ t ].assign( s.begin() , s.end() );
  }
 return( B );
 }

/// the case bundle
void bundles( std::size_t n , int r , unsigned seed )
{
 const Bundle B = bundle( n , seed );
 const std::size_t ops = n * ROUNDS;
 auto ads = [ & ]( int ) {
  ADS::DynamicCsr<> D;
  for( std::size_t i = 0 ; i < B.ind.size() ; ++i )
   D.append( B.ind[ i ] , B.val[ i ] );
  std::uint64_t t = 0;
  for( int k = 0 ; k < ROUNDS ; ++k ) {
   for( std::size_t i = 0 ; i < B.nind[ k ].size() ; ++i )
    D.append( B.nind[ k ][ i ] , B.nval[ k ][ i ] );
   D.remove( B.gone[ k ] );
   t += std::uint64_t( D.beg()[ D.rows() ] ) + std::uint64_t( D.ind()[ 0 ] );
   }
  sink = sink + t; };
 for( const char * who : { "ADS::DynamicCsr" , "ADS::DynamicCsr A/A" } )
  if( ! skip( who ) )
   print( "assembly" , "bundle" , n , who ,
          median( [](){ return( 0 ); } , ads , r , ops ) );
 if( skip( "std::vector rows" ) )
  return;
 print( "assembly" , "bundle" , n , "std::vector rows" ,
        median( [](){ return( 0 ); } , [ & ]( int ) {
         using Entries = std::vector< std::pair< int , double > >;
         std::vector< Entries > V;
         auto add = [ & ]( const std::vector< int > & x ,
                           const std::vector< double > & y ) {
          Entries e( x.size() );
          for( std::size_t j = 0 ; j < x.size() ; ++j )
           e[ j ] = { x[ j ] , y[ j ] };
          V.push_back( std::move( e ) ); };
         for( std::size_t i = 0 ; i < B.ind.size() ; ++i )
          add( B.ind[ i ] , B.val[ i ] );
         Arrays A;
         std::uint64_t t = 0;
         for( int k = 0 ; k < ROUNDS ; ++k ) {
          for( std::size_t i = 0 ; i < B.nind[ k ].size() ; ++i )
           add( B.nind[ k ][ i ] , B.nval[ k ][ i ] );
          std::size_t o = 0 , q = 0;         // the rows removed, compacted
          for( std::size_t i = 0 ; i < V.size() ; ++i )
           if( ( q < B.gone[ k ].size() ) && ( B.gone[ k ][ q ] == i ) )
            ++q;
           else
            V[ o++ ] = std::move( V[ i ] );
          V.resize( o );
          A.beg.assign( 1 , 0 );             // handed to the solver
          A.ind.clear();
          A.val.clear();
          for( const auto & e : V ) {
           for( const auto & [ j , v ] : e ) {
            A.ind.push_back( j );
            A.val.push_back( v );
            }
           A.beg.push_back( int( A.ind.size() ) );
           }
          t += std::uint64_t( A.beg.back() ) + std::uint64_t( A.ind[ 0 ] );
          }
         sink = sink + t; } , r , ops ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 } );
 for( std::size_t n : o.sizes ) {
  assembly( "random" , triplets( n , false , o.seed ) , n , o.runs );
  assembly( "repeated" , triplets( n , true , o.seed ) , n , o.runs );
  bundles( n , o.runs , o.seed );
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File assembly.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

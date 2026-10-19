/*--------------------------------------------------------------------------*/
/*--------------------------- File succinct.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the succinct structures: the rank and select directory,
 * Elias-Fano, the balanced parentheses and the wavelet matrix of ADS
 * against, when they are found, sdsl-lite (rank_support_v, rank_support_v5,
 * select_support_mcl, sd_vector, bp_support_sada, wm_int, wt_huff), sux
 * (Rank9, Rank9Sel, SimpleSelect, SimpleSelectZero, EliasFano), pasta
 * (FlatRankSelect, WideRankSelect) and succinct (rs_bit_vector,
 * elias_fano, bp_vector):
 *
 * - bitvector: rank1, select1 and select0 on n random bits with 50%, 10%
 *   and 1% of 1s, and the building of the directory;
 * - eliasfano: access and nextGEQ (the rank of the competitors) on n
 *   random distinct integers in [ 0 , 128 n ), and the building, for n up
 *   to 10^7;
 * - parentheses: findClose, findOpen and enclose on the parentheses of a
 *   random tree of n nodes (a uniform Dyck word of 2n bits), for n up to
 *   10^7;
 * - wavelet: access, rank and select on n random symbols of 8 and 16
 *   bits, 10^5 queries a cell, and the building;
 * - space: the bits of each structure per bit (per item for Elias-Fano and
 *   the wavelet matrix), in the column of the time, so that the ratios of
 *   bench/summary.py read the same way (above 1 when ADS is smaller).
 *
 * Each cell of a query is the median of r runs of q = 10^6 random queries
 * on a structure built once, in nanoseconds per query; the queries are
 * independent, so that the misses of the cache overlap. Each cell of a
 * building is the median of r buildings from a copy of the input, the copy
 * excluded. The contracts are off (ADS_CONTRACTS=0).
 *
 *   succinct [ -r runs ] [ -s seed ] n1 n2 ...
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

#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/RankSelect.h"
#if __has_include( "ADS/succinct/WaveletMatrix.h" )
 #include "ADS/succinct/WaveletMatrix.h"
 #define ADS_HAS_WM 1
#endif

#if __has_include( <sdsl/bit_vectors.hpp> )
 #include <sdsl/bit_vectors.hpp>
 #include <sdsl/bp_support.hpp>
 #include <sdsl/rank_support.hpp>
 #include <sdsl/select_support.hpp>
 #include <sdsl/wavelet_trees.hpp>
 #define ADS_HAS_SDSL 1
#endif

#if __has_include( <sux/bits/Rank9Sel.hpp> )
 #include <sux/bits/EliasFano.hpp>
 #include <sux/bits/Rank9.hpp>
 #include <sux/bits/Rank9Sel.hpp>
 #include <sux/bits/SimpleSelect.hpp>
 #include <sux/bits/SimpleSelectZero.hpp>
 #define ADS_HAS_SUX 1
#endif

#if __has_include( <pasta/bit_vector/bit_vector.hpp> )
 #include <pasta/bit_vector/bit_vector.hpp>
 #include <pasta/bit_vector/support/flat_rank_select.hpp>
 #include <pasta/bit_vector/support/wide_rank_select.hpp>
 #define ADS_HAS_PASTA 1
#endif

#if __has_include( <succinct/rs_bit_vector.hpp> )
 #include <succinct/bp_vector.hpp>
 #include <succinct/elias_fano.hpp>
 #include <succinct/mapper.hpp>
 #include <succinct/rs_bit_vector.hpp>
 #define ADS_HAS_SUCCINCT 1
#endif

#include <memory>
#include <span>
#include <type_traits>

using namespace ADS::bench;

namespace {

constexpr std::size_t kQueries = 1000000;

/// the median time of q queries f( x ) for the x of X, summed into the sink
template< class F >
double queries( const std::vector< std::uint64_t > & X , int r , F && f )
{
 return( median( [](){ return( 0 ); } , [ & ]( int ) {
          std::uint64_t s = 0;
          for( const auto x : X ) s += std::uint64_t( f( x ) );
          sink = sink + s; } , r , X.size() ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BIT VECTORS ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the input of a cell of the bit vectors: the words, the bits as bools
/// and the queries
struct Bits {
 std::size_t n = 0 , ones = 0;
 std::vector< std::uint64_t > words;
 std::vector< std::uint64_t > R , S1 , S0;   ///< rank places, select ks
 };

Bits makeBits( std::size_t n , double d , unsigned seed )
{
 Bits b;
 b.n = n;
 b.words.assign( ( n + 63 ) / 64 , 0 );
 std::mt19937_64 g( seed );
 std::uniform_real_distribution< double > U( 0 , 1 );
 for( std::size_t i = 0 ; i < n ; ++i )
  if( U( g ) < d ) {
   b.words[ i / 64 ] |= std::uint64_t( 1 ) << ( i % 64 );
   ++b.ones;
   }
 const std::size_t zeros = n - b.ones;
 for( std::size_t q = 0 ; q < kQueries ; ++q ) {
  b.R.push_back( g() % ( n + 1 ) );
  b.S1.push_back( 1 + g() % b.ones );
  b.S0.push_back( 1 + g() % zeros );
  }
 return( b );
 }

/// a missing query of a structure
using None = std::nullptr_t;

/// the cells of a structure: its name, how to build it from the words,
/// its rank1, select1 and select0 (k counting from 1), any of them None if
/// missing, and its bits per bit of the vector, the vector included; the
/// queries are lambdas, inlined into the loop of the queries
template< class B , class R , class S1 , class S0 , class Z >
void bitvector( const std::string & dname , const Bits & b ,
                const std::string & who , B build , R rank , S1 sel1 ,
                S0 sel0 , Z bits , int r )
{
 struct { const std::string & who; B & build; R & rank; S1 & sel1; S0 & sel0;
          Z & bits; } c{ who , build , rank , sel1 , sel0 , bits };
 print( "bitvector" , "build " + dname , b.n , c.who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto s = c.build( b );
          sink = sink + std::uint64_t( s != nullptr ); } , r , b.n / 64 ) );
 const auto s = c.build( b );
 if constexpr( ! std::is_same_v< R , None > )
  print( "bitvector" , "rank1 " + dname , b.n , c.who , queries( b.R , r ,
          [ & ]( std::uint64_t x ) { return( c.rank( *s , x ) ); } ) );
 if constexpr( ! std::is_same_v< S1 , None > )
  print( "bitvector" , "select1 " + dname , b.n , c.who , queries( b.S1 ,
          r , [ & ]( std::uint64_t x ) { return( c.sel1( *s , x ) ); } ) );
 if constexpr( ! std::is_same_v< S0 , None > )
  print( "bitvector" , "select0 " + dname , b.n , c.who , queries( b.S0 ,
          r , [ & ]( std::uint64_t x ) { return( c.sel0( *s , x ) ); } ) );
 print( "space" , "bitvector " + dname , b.n , c.who , c.bits( *s ) );
 }

/// a bit vector of ADS from the words (a word at a time, in the versions
/// of ADS with no constructor from the words)
template< class BV = ADS::BitVector >
BV adsBits( const Bits & b )
{
 using Span = std::span< const std::uint64_t >;
 if constexpr( std::is_constructible_v< BV , Span , std::size_t > )
  return( BV( Span( b.words ) , b.n ) );
 else {
  BV v;
  v.reserve( b.n );
  for( std::size_t w = 0 ; w < b.words.size() ; ++w )
   v.append( b.words[ w ] , int( std::min< std::size_t >( 64 ,
                                                        b.n - 64 * w ) ) );
  return( v );
  }
 }

void bitvectors( std::size_t n , int r , unsigned seed )
{
 const std::pair< double , const char * > D[] = { { 0.5 , "50%" } ,
                                                  { 0.1 , "10%" } ,
                                                  { 0.01 , "1%" } };
 for( const auto & [ d , dname ] : D ) {
  const Bits b = makeBits( n , d , seed );
  const double nb = double( b.n );
  for( const char * who : { "ADS::RankSelect" , "ADS::RankSelect A/A" } )
   bitvector( dname , b ,
    who , []( const Bits & b ) {
     return( std::make_unique< ADS::RankSelect >( adsBits( b ) ) ); } ,
    []( const ADS::RankSelect & s , std::uint64_t i ) {
     return( s.rank1( i ) ); } ,
    []( const ADS::RankSelect & s , std::uint64_t k ) {
     return( s.select1( k ) ); } ,
    []( const ADS::RankSelect & s , std::uint64_t k ) {
     return( s.select0( k ) ); } ,
    [ nb ]( const ADS::RankSelect & s ) {
     return( double( s.size() + s.directoryBits() ) / nb ); } , r );
#ifdef ADS_HAS_SDSL
  struct SdslV {
   sdsl::bit_vector v;
   sdsl::rank_support_v< 1 > rk;
   sdsl::select_support_mcl< 1 > s1;
   sdsl::select_support_mcl< 0 > s0;
   };
  auto sdslBits = []( const Bits & b ) {
   sdsl::bit_vector v( b.n , 0 );
   std::copy( b.words.begin() , b.words.end() , v.data() );
   return( v );
   };
  bitvector( dname , b ,
   "sdsl::rank_support_v+mcl" , [ & ]( const Bits & b ) {
    auto s = std::make_unique< SdslV >();
    s->v = sdslBits( b );
    s->rk = sdsl::rank_support_v< 1 >( &s->v );
    s->s1 = sdsl::select_support_mcl< 1 >( &s->v );
    s->s0 = sdsl::select_support_mcl< 0 >( &s->v );
    return( s ); } ,
   []( const SdslV & s , std::uint64_t i ) { return( s.rk.rank( i ) ); } ,
   []( const SdslV & s , std::uint64_t k ) { return( s.s1.select( k ) ); } ,
   []( const SdslV & s , std::uint64_t k ) { return( s.s0.select( k ) ); } ,
   [ nb ]( const SdslV & s ) {
    return( 8 * double( sdsl::size_in_bytes( s.v ) +
                        sdsl::size_in_bytes( s.rk ) +
                        sdsl::size_in_bytes( s.s1 ) +
                        sdsl::size_in_bytes( s.s0 ) ) / nb ); } , r );
  struct SdslV5 {
   sdsl::bit_vector v;
   sdsl::rank_support_v5< 1 > rk;
   };
  bitvector( dname , b ,
   "sdsl::rank_support_v5" , [ & ]( const Bits & b ) {
    auto s = std::make_unique< SdslV5 >();
    s->v = sdslBits( b );
    s->rk = sdsl::rank_support_v5< 1 >( &s->v );
    return( s ); } ,
   []( const SdslV5 & s , std::uint64_t i ) { return( s.rk.rank( i ) ); } ,
   None() , None() , [ nb ]( const SdslV5 & s ) {
    return( 8 * double( sdsl::size_in_bytes( s.v ) +
                        sdsl::size_in_bytes( s.rk ) ) / nb ); } , r );
#endif
#ifdef ADS_HAS_SUX
  struct Sux {
   std::vector< std::uint64_t > v;
   std::unique_ptr< sux::bits::Rank9Sel<> > rs;
   std::unique_ptr< sux::bits::SimpleSelect<> > s1;
   std::unique_ptr< sux::bits::SimpleSelectZero<> > s0;
   };
  bitvector( dname , b ,
   "sux::Rank9Sel+SimpleSelect" , []( const Bits & b ) {
    auto s = std::make_unique< Sux >();
    s->v = b.words;
    s->rs = std::make_unique< sux::bits::Rank9Sel<> >( s->v.data() , b.n );
    s->s1 = std::make_unique< sux::bits::SimpleSelect<> >( s->v.data() ,
                                                           b.n , 3 );
    s->s0 = std::make_unique< sux::bits::SimpleSelectZero<> >( s->v.data() ,
                                                               b.n , 3 );
    return( s ); } ,
   []( const Sux & s , std::uint64_t i ) { return( s.rs->rank( i ) ); } ,
   []( const Sux & s , std::uint64_t k ) {
    return( s.s1->select( k - 1 ) ); } ,
   []( const Sux & s , std::uint64_t k ) {
    return( s.s0->selectZero( k - 1 ) ); } ,
   [ nb ]( const Sux & s ) {
    return( ( nb + double( s.rs->bitCount() + s.s1->bitCount() +
                           s.s0->bitCount() ) ) / nb ); } , r );
  struct SuxR9 {
   std::vector< std::uint64_t > v;
   std::unique_ptr< sux::bits::Rank9Sel<> > rs;
   };
  bitvector( dname , b ,
   "sux::Rank9Sel" , []( const Bits & b ) {
    auto s = std::make_unique< SuxR9 >();
    s->v = b.words;
    s->rs = std::make_unique< sux::bits::Rank9Sel<> >( s->v.data() , b.n );
    return( s ); } ,
   []( const SuxR9 & s , std::uint64_t i ) { return( s.rs->rank( i ) ); } ,
   []( const SuxR9 & s , std::uint64_t k ) {
    return( s.rs->select( k - 1 ) ); } , None() ,
   [ nb ]( const SuxR9 & s ) {
    return( ( nb + double( s.rs->bitCount() ) ) / nb ); } , r );
#endif
#ifdef ADS_HAS_PASTA
  struct Pasta {
   pasta::BitVector v;
   std::unique_ptr< pasta::FlatRankSelect<> > rs;
   };
  bitvector( dname , b ,
   "pasta::FlatRankSelect" , []( const Bits & b ) {
    auto s = std::make_unique< Pasta >();
    s->v = pasta::BitVector( b.n , false );
    std::copy( b.words.begin() , b.words.end() , s->v.data().begin() );
    s->rs = std::make_unique< pasta::FlatRankSelect<> >( s->v );
    return( s ); } ,
   []( const Pasta & s , std::uint64_t i ) { return( s.rs->rank1( i ) ); } ,
   []( const Pasta & s , std::uint64_t k ) { return( s.rs->select1( k ) ); } ,
   []( const Pasta & s , std::uint64_t k ) { return( s.rs->select0( k ) ); } ,
   [ nb ]( const Pasta & s ) {
    return( ( 8 * double( s.v.space_usage() + s.rs->space_usage() ) ) /
            nb ); } , r );
  struct PastaW {
   pasta::BitVector v;
   std::unique_ptr< pasta::WideRankSelect<> > rs;
   };
  bitvector( dname , b ,
   "pasta::WideRankSelect" , []( const Bits & b ) {
    auto s = std::make_unique< PastaW >();
    s->v = pasta::BitVector( b.n , false );
    std::copy( b.words.begin() , b.words.end() , s->v.data().begin() );
    s->rs = std::make_unique< pasta::WideRankSelect<> >( s->v );
    return( s ); } ,
   []( const PastaW & s , std::uint64_t i ) { return( s.rs->rank1( i ) ); } ,
   []( const PastaW & s , std::uint64_t k ) {
    return( s.rs->select1( k ) ); } ,
   []( const PastaW & s , std::uint64_t k ) {
    return( s.rs->select0( k ) ); } ,
   [ nb ]( const PastaW & s ) {
    return( ( 8 * double( s.v.space_usage() + s.rs->space_usage() ) ) /
            nb ); } , r );
#endif
#ifdef ADS_HAS_SUCCINCT
  bitvector( dname , b ,
   "succinct::rs_bit_vector" , []( const Bits & b ) {
    succinct::bit_vector_builder bb( b.n );
    for( std::size_t w = 0 ; w < b.words.size() ; ++w )
     bb.set_bits( 64 * w , b.words[ w ] ,
                  std::min< std::size_t >( 64 , b.n - 64 * w ) );
    return( std::make_unique< succinct::rs_bit_vector >( &bb , true ,
                                                         true ) ); } ,
   []( const succinct::rs_bit_vector & s , std::uint64_t i ) {
    return( s.rank( i ) ); } ,
   []( const succinct::rs_bit_vector & s , std::uint64_t k ) {
    return( s.select( k - 1 ) ); } ,
   []( const succinct::rs_bit_vector & s , std::uint64_t k ) {
    return( s.select0( k - 1 ) ); } ,
   [ nb ]( const succinct::rs_bit_vector & s ) {
    auto & t = const_cast< succinct::rs_bit_vector & >( s );
    return( 8 * double( succinct::mapper::size_of( t ) ) / nb ); } , r );
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- ELIAS-FANO -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cells of an Elias-Fano structure: access( i ) of the item i, from
/// 0, and nextGEQ( x ), the number of the items below x
template< class B , class A , class G , class Z >
void eliasFano( const std::vector< std::uint64_t > & X , std::uint64_t u ,
                const std::vector< std::uint64_t > & I ,
                const std::vector< std::uint64_t > & Q ,
                const std::string & who , B build , A access , G geq ,
                Z bits , int r )
{
 struct { const std::string & who; B & build; A & access; G & geq;
          Z & bits; } c{ who , build , access , geq , bits };
 const std::size_t n = X.size();
 print( "eliasfano" , "build" , n , c.who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto s = c.build( X , u );
          sink = sink + std::uint64_t( s != nullptr ); } , r , n ) );
 auto s = c.build( X , u );
 print( "eliasfano" , "access" , n , c.who , queries( I , r ,
         [ & ]( std::uint64_t i ) { return( c.access( *s , i ) ); } ) );
 print( "eliasfano" , "nextGEQ" , n , c.who , queries( Q , r ,
         [ & ]( std::uint64_t x ) { return( c.geq( *s , x ) ); } ) );
 print( "space" , "eliasfano" , n , c.who , c.bits( *s ) );
 }

void eliasFanos( std::size_t n , int r , unsigned seed )
{
 const std::uint64_t u = 128 * std::uint64_t( n );
 std::mt19937_64 g( seed );
 std::vector< std::uint64_t > X;
 {
 std::unordered_set< std::uint64_t > seen;
 while( X.size() < n )
  if( const std::uint64_t x = g() % u ; seen.insert( x ).second )
   X.push_back( x );
 std::sort( X.begin() , X.end() );
 }
 std::vector< std::uint64_t > I , Q;
 for( std::size_t q = 0 ; q < kQueries ; ++q ) {
  I.push_back( g() % n );
  Q.push_back( g() % u );
  }
 const double nn = double( n );
 using EF = ADS::EliasFano;
 for( const char * who : { "ADS::EliasFano" , "ADS::EliasFano A/A" } )
  eliasFano( X , u , I , Q ,
   who , []( const std::vector< std::uint64_t > & X , std::uint64_t u ) {
    return( std::make_unique< EF >( X , u ) ); } ,
   []( const EF & s , std::uint64_t i ) { return( s.access( i ) ); } ,
   []( const EF & s , std::uint64_t x ) { return( s.nextGEQ( x ) ); } ,
   [ nn ]( const EF & s ) { return( double( s.sizeInBits() ) / nn ); } ,
   r );
#ifdef ADS_HAS_SDSL
 struct SD {
  sdsl::sd_vector<> v;
  sdsl::sd_vector<>::rank_1_type rk;
  sdsl::sd_vector<>::select_1_type sl;
  };
 eliasFano( X , u , I , Q ,
  "sdsl::sd_vector" , []( const std::vector< std::uint64_t > & X ,
                          std::uint64_t u ) {
   auto s = std::make_unique< SD >();
   sdsl::sd_vector_builder b( u , X.size() );
   for( const auto x : X ) b.set( x );
   s->v = sdsl::sd_vector<>( b );
   s->rk = sdsl::sd_vector<>::rank_1_type( &s->v );
   s->sl = sdsl::sd_vector<>::select_1_type( &s->v );
   return( s ); } ,
  []( const SD & s , std::uint64_t i ) { return( s.sl.select( i + 1 ) ); } ,
  []( const SD & s , std::uint64_t x ) { return( s.rk.rank( x ) ); } ,
  [ nn ]( const SD & s ) {
   return( 8 * double( sdsl::size_in_bytes( s.v ) ) / nn ); } , r );
#endif
#ifdef ADS_HAS_SUX
 using SE = sux::bits::EliasFano<>;
 eliasFano( X , u , I , Q ,
  "sux::EliasFano" , []( const std::vector< std::uint64_t > & X ,
                         std::uint64_t u ) {
   return( std::make_unique< SE >( X , u ) ); } ,
  []( const SE & s , std::uint64_t i ) {
   return( const_cast< SE & >( s ).select( i ) ); } ,
  []( const SE & s , std::uint64_t x ) {
   return( const_cast< SE & >( s ).rank( x ) ); } ,
  [ nn ]( const SE & s ) {
   return( double( const_cast< SE & >( s ).bitCount() ) / nn ); } , r );
#endif
#ifdef ADS_HAS_SUCCINCT
 using SC = succinct::elias_fano;
 eliasFano( X , u , I , Q ,
  "succinct::elias_fano" , []( const std::vector< std::uint64_t > & X ,
                               std::uint64_t u ) {
   succinct::elias_fano::elias_fano_builder b( u , X.size() );
   for( const auto x : X ) b.push_back( x );
   return( std::make_unique< SC >( &b , true ) ); } ,
  []( const SC & s , std::uint64_t i ) { return( s.select( i ) ); } ,
  []( const SC & s , std::uint64_t x ) { return( s.rank( x ) ); } ,
  [ nn ]( const SC & s ) {
   return( 8 * double( succinct::mapper::size_of(
                        const_cast< SC & >( s ) ) ) / nn ); } , r );
#endif
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- BALANCED PARENTHESES -------------------------*/
/*--------------------------------------------------------------------------*/


/// the parentheses of a uniform random tree of n nodes: a random sequence
/// of n '(' and n + 1 ')' rotated to the place after its minimum (the
/// cycle lemma), less its last ')'
Bits makeTree( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::vector< char > s( 2 * n + 1 , 0 );
 std::fill( s.begin() , s.begin() + std::ptrdiff_t( n ) , 1 );
 std::shuffle( s.begin() , s.end() , g );
 long e = 0 , mn = 1;
 std::size_t at = 0;
 for( std::size_t i = 0 ; i < s.size() ; ++i ) {
  e += s[ i ] ? 1 : -1;
  if( e < mn ) { mn = e; at = i + 1; }
  }
 std::rotate( s.begin() , s.begin() + std::ptrdiff_t( at % s.size() ) ,
              s.end() );
 Bits b;
 b.n = 2 * n;
 b.words.assign( ( b.n + 63 ) / 64 , 0 );
 std::vector< std::uint64_t > opens , closes;
 for( std::size_t i = 0 ; i < b.n ; ++i )
  if( s[ i ] ) {
   b.words[ i / 64 ] |= std::uint64_t( 1 ) << ( i % 64 );
   opens.push_back( i );
   }
  else closes.push_back( i );
 b.ones = n;
 for( std::size_t q = 0 ; q < kQueries ; ++q ) {
  b.S1.push_back( opens[ g() % n ] );
  b.S0.push_back( closes[ g() % n ] );
  }
 return( b );
 }

/// the cells of a structure of balanced parentheses: its findClose,
/// findOpen and enclose, any of them None if missing
template< class B , class C , class O , class E , class Z >
void parentheses( const Bits & b , const std::string & who , B build ,
                  C close , O open , E enclose , Z bits , int r )
{
 struct { const std::string & who; B & build; C & close; O & open;
          E & enclose; Z & bits; } c{ who , build , close , open , enclose ,
                                      bits };
 const std::size_t n = b.ones;
 print( "parentheses" , "build" , n , c.who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          auto s = c.build( b );
          sink = sink + std::uint64_t( s != nullptr ); } , r , n ) );
 auto s = c.build( b );
 if constexpr( ! std::is_same_v< C , None > )
  print( "parentheses" , "findClose" , n , c.who , queries( b.S1 , r ,
          [ & ]( std::uint64_t i ) { return( c.close( *s , i ) ); } ) );
 if constexpr( ! std::is_same_v< O , None > )
  print( "parentheses" , "findOpen" , n , c.who , queries( b.S0 , r ,
          [ & ]( std::uint64_t i ) { return( c.open( *s , i ) ); } ) );
 if constexpr( ! std::is_same_v< E , None > )
  print( "parentheses" , "enclose" , n , c.who , queries( b.S1 , r ,
          [ & ]( std::uint64_t i ) { return( c.enclose( *s , i ) ); } ) );
 print( "space" , "parentheses" , n , c.who , c.bits( *s ) );
 }

void parenthesess( std::size_t n , int r , unsigned seed )
{
 const Bits b = makeTree( n , seed );
 const double nb = double( b.n );
 using BP = ADS::BalancedParentheses;
 for( const char * who : { "ADS::BalancedParentheses" ,
                           "ADS::BalancedParentheses A/A" } )
  parentheses( b ,
   who , []( const Bits & b ) {
    return( std::make_unique< BP >( adsBits( b ) ) ); } ,
   []( const BP & s , std::uint64_t i ) { return( s.findClose( i ) ); } ,
   []( const BP & s , std::uint64_t i ) { return( s.findOpen( i ) ); } ,
   []( const BP & s , std::uint64_t i ) { return( s.enclose( i ) ); } ,
   [ nb ]( const BP & s ) {
    return( double( s.size() + s.bits().directoryBits() + s.treeBits() ) /
            nb ); } , r );
#ifdef ADS_HAS_SDSL
 struct Sada {
  sdsl::bit_vector v;
  sdsl::bp_support_sada<> bp;
  };
 parentheses( b ,
  "sdsl::bp_support_sada" , []( const Bits & b ) {
   auto s = std::make_unique< Sada >();
   s->v = sdsl::bit_vector( b.n , 0 );
   std::copy( b.words.begin() , b.words.end() , s->v.data() );
   s->bp = sdsl::bp_support_sada<>( &s->v );
   return( s ); } ,
  []( const Sada & s , std::uint64_t i ) { return( s.bp.find_close( i ) ); } ,
  []( const Sada & s , std::uint64_t i ) { return( s.bp.find_open( i ) ); } ,
  []( const Sada & s , std::uint64_t i ) { return( s.bp.enclose( i ) ); } ,
  [ nb ]( const Sada & s ) {
   return( 8 * double( sdsl::size_in_bytes( s.v ) +
                       sdsl::size_in_bytes( s.bp ) ) / nb ); } , r );
#endif
#ifdef ADS_HAS_SUCCINCT
 using SB = succinct::bp_vector;
 parentheses( b ,
  "succinct::bp_vector" , []( const Bits & b ) {
   succinct::bit_vector_builder bb( b.n );
   for( std::size_t w = 0 ; w < b.words.size() ; ++w )
    bb.set_bits( 64 * w , b.words[ w ] ,
                 std::min< std::size_t >( 64 , b.n - 64 * w ) );
   return( std::make_unique< SB >( &bb , false , false ) ); } ,
  []( const SB & s , std::uint64_t i ) { return( s.find_close( i ) ); } ,
  []( const SB & s , std::uint64_t i ) { return( s.find_open( i ) ); } ,
  None() ,                                 // no enclose() but find_open()
  [ nb ]( const SB & s ) {
   return( 8 * double( succinct::mapper::size_of(
                        const_cast< SB & >( s ) ) ) / nb ); } , r );
#endif
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- WAVELET --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cells of a sequence of integers: access( i ), rank( c , i ) and
/// select( c , k ), a query of rank or select coded as c 2^40 + i (or k)
template< class B , class A , class K , class S , class Z >
void wavelet( const std::string & what ,
              const std::vector< std::uint64_t > & X , int w ,
              const std::vector< std::uint64_t > & I ,
              const std::vector< std::uint64_t > & R ,
              const std::vector< std::uint64_t > & L ,
              const std::string & who , B build , A access , K rank ,
              S select , Z bits , int r )
{
 struct { const std::string & who; B & build; A & access; K & rank;
          S & select; Z & bits; } c{ who , build , access , rank , select ,
                                     bits };
 const std::size_t n = X.size();
 print( "wavelet" , "build " + what , n , c.who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          auto s = c.build( X , w );
          sink = sink + std::uint64_t( s != nullptr ); } , r , n ) );
 auto s = c.build( X , w );
 print( "wavelet" , "access " + what , n , c.who , queries( I , r ,
         [ & ]( std::uint64_t i ) { return( c.access( *s , i ) ); } ) );
 print( "wavelet" , "rank " + what , n , c.who , queries( R , r ,
         [ & ]( std::uint64_t q ) { return( c.rank( *s , q ) ); } ) );
 print( "wavelet" , "select " + what , n , c.who , queries( L , r ,
         [ & ]( std::uint64_t q ) { return( c.select( *s , q ) ); } ) );
 print( "space" , "wavelet " + what , n , c.who , c.bits( *s ) );
 }

void wavelets( std::size_t n , int r , unsigned seed )
{
 constexpr std::uint64_t M = ( std::uint64_t( 1 ) << 40 ) - 1;
 for( const int w : { 8 , 16 } ) {
  const std::string what = std::to_string( w ) + " bits";
  std::mt19937_64 g( seed );
  std::vector< std::uint64_t > X( n ) , I , R , L;
  for( auto & x : X ) x = g() % ( std::uint64_t( 1 ) << w );
  // the occurrence of each item among those equal to it, for select
  std::vector< std::uint64_t > seen( std::size_t( 1 ) << w , 0 ) , occ( n );
  for( std::size_t i = 0 ; i < n ; ++i ) occ[ i ] = ++seen[ X[ i ] ];
  for( std::size_t q = 0 ; q < kQueries / 10 ; ++q ) {
   I.push_back( g() % n );
   const std::size_t p = g() % n;
   R.push_back( ( X[ p ] << 40 ) | ( g() % ( n + 1 ) ) );
   L.push_back( ( X[ p ] << 40 ) | occ[ p ] );
   }
  const double nn = double( n );
#ifdef ADS_HAS_WM
  using WM = ADS::WaveletMatrix;
  for( const char * who : { "ADS::WaveletMatrix" ,
                            "ADS::WaveletMatrix A/A" } )
   wavelet( what , X , w , I , R , L ,
    who , []( const std::vector< std::uint64_t > & X , int w ) {
     return( std::make_unique< WM >( X , w ) ); } ,
    []( const WM & s , std::uint64_t i ) { return( s.access( i ) ); } ,
    []( const WM & s , std::uint64_t q ) {
     return( s.rank( q >> 40 , q & M ) ); } ,
    []( const WM & s , std::uint64_t q ) {
     return( s.select( q >> 40 , q & M ) ); } ,
    [ nn ]( const WM & s ) { return( double( s.sizeInBits() ) / nn ); } ,
    r );
#endif
#ifdef ADS_HAS_SDSL
  auto sdslInts = []( const std::vector< std::uint64_t > & X , int w ) {
   sdsl::int_vector<> v( X.size() , 0 , std::uint8_t( w ) );
   for( std::size_t i = 0 ; i < X.size() ; ++i ) v[ i ] = X[ i ];
   return( v );
   };
  using SWM = sdsl::wm_int< sdsl::bit_vector , sdsl::rank_support_v< 1 > ,
                            sdsl::select_support_mcl< 1 > ,
                            sdsl::select_support_mcl< 0 > >;
  using SWT = sdsl::wt_int< sdsl::bit_vector , sdsl::rank_support_v< 1 > ,
                            sdsl::select_support_mcl< 1 > ,
                            sdsl::select_support_mcl< 0 > >;
  auto sdslCand = [ & ]< class T >( const char * who , T * ) {
   wavelet( what , X , w , I , R , L ,
    who , [ & ]( const std::vector< std::uint64_t > & X , int w ) {
     auto s = std::make_unique< T >();
     sdsl::construct_im( *s , sdslInts( X , w ) );
     return( s ); } ,
    []( const T & s , std::uint64_t i ) { return( s[ i ] ); } ,
    []( const T & s , std::uint64_t q ) {
     return( s.rank( q & M , q >> 40 ) ); } ,
    []( const T & s , std::uint64_t q ) {
     return( s.select( q & M , q >> 40 ) ); } ,
    [ nn ]( const T & s ) {
     return( 8 * double( sdsl::size_in_bytes( s ) ) / nn ); } , r );
   };
  sdslCand( "sdsl::wm_int" , ( SWM * )nullptr );
  sdslCand( "sdsl::wt_int" , ( SWT * )nullptr );
#endif
  }
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 , 10000000 , 100000000 } );
 for( std::size_t n : o.sizes )
  if( n <= 10000000 ) eliasFanos( n , o.runs , o.seed );
 for( std::size_t n : o.sizes )
  if( n <= 10000000 ) parenthesess( n , o.runs , o.seed );
 for( std::size_t n : o.sizes )
  if( n <= 10000000 ) wavelets( n , o.runs , o.seed );
 for( std::size_t n : o.sizes ) bitvectors( n , o.runs , o.seed );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File succinct.cpp --------------------------*/
/*--------------------------------------------------------------------------*/

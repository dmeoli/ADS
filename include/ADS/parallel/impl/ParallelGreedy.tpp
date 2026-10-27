/*--------------------------------------------------------------------------*/
/*------------------------ File ParallelGreedy.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel greedy algorithms of ParallelGreedy.h on speculativeFor(),
 * and their sequential versions, which the calls on one thread run and
 * the audits compare with.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/


#include <algorithm>
#include <atomic>
#include <cstdint>

namespace ADS {

/*--------------------------------------------------------------------------*/

inline std::vector< int > randomOrder( int n , unsigned long long seed )
{
 ADS_PRE( n >= 0 );
 std::vector< int > p( std::size_t( n < 0 ? 0 : n ) );
 for( int i = 0 ; i < n ; ++i )
  p[ i ] = i;
 std::uint64_t x = seed;
 auto next = [ & ]( void ) {             // SplitMix64
  std::uint64_t z = ( x += 0x9e3779b97f4a7c15ULL );
  z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
  z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebULL;
  return( z ^ ( z >> 31 ) );
  };
 for( int i = n - 1 ; i > 0 ; --i )
  std::swap( p[ i ] , p[ int( next() % std::uint64_t( i + 1 ) ) ] );
 return( p );
 }

namespace detail {

/// the union-find of the parallel greedy algorithms: the roots linked by
/// the commits, with no rank, and the paths halved by the finds, which run
/// only while no link does; the parents read and written as atomics
class GreedyUnionFind {
 public:

 explicit GreedyUnionFind( int n ) : f_p( std::size_t( n < 0 ? 0 : n ) ) {
  for( int x = 0 ; x < n ; ++x )
   f_p[ x ] = x;
  }

 int find( int x ) {
  for( ; ; ) {
   const int y = load( x );
   if( y == x )
    return( x );
   const int z = load( y );
   if( z != y )
    std::atomic_ref< int >( f_p[ x ] ).store( z ,
                                              std::memory_order_relaxed );
   x = z;
   }
  }

 /// the root r under p
 void link( int r , int p ) {
  std::atomic_ref< int >( f_p[ r ] ).store( p , std::memory_order_relaxed );
  }

 private:

 int load( int x ) {
  return( std::atomic_ref< int >( f_p[ x ] ).load(
                                  std::memory_order_relaxed ) );
  }

 std::vector< int > f_p;
 };

/// the edges A[ ids[ i ] ] in the order of i, by the greedy algorithm on
/// the union-find U: the ids taken appended to out
template< class X , class GetEnds >
void greedyForest( X & ex , const std::vector< int > & ids ,
                   GetEnds && ends , GreedyUnionFind & U ,
                   std::vector< int > & out )
{
 const std::size_t k = ids.size();
 if( ( ex.concurrency() <= 1 ) || ( k < 4096 ) ) {
  for( int id : ids ) {                   // the sequential greedy
   const auto [ u , v ] = ends( id );
   const int ru = U.find( u ) , rv = U.find( v );
   if( ru != rv ) {
    U.link( ru , rv );
    out.push_back( id );
    }
   }
  return;
  }
 int n = 0;
 for( int id : ids ) {
  const auto [ u , v ] = ends( id );
  n = std::max( { n , u + 1 , v + 1 } );
  }
 std::vector< Reservation > R( static_cast< std::size_t >( n ) );
 std::vector< int > ru( k ) , rv( k );
 std::vector< char > taken( k , 0 );
 struct Step {
  const std::vector< int > & ids;
  GetEnds & ends;
  GreedyUnionFind & U;
  std::vector< Reservation > & R;
  std::vector< int > & ru , & rv;
  std::vector< char > & taken;

  bool reserve( std::size_t i ) {
   const auto [ u , v ] = ends( ids[ i ] );
   ru[ i ] = U.find( u );
   rv[ i ] = U.find( v );
   if( ru[ i ] == rv[ i ] )
    return( false );                      // left out
   R[ ru[ i ] ].reserve( i );
   R[ rv[ i ] ].reserve( i );
   return( true );
   }

  bool commit( std::size_t i ) {
   if( R[ ru[ i ] ].check( i ) ) {
    R[ ru[ i ] ].reset();
    R[ rv[ i ] ].checkReset( i );
    U.link( ru[ i ] , rv[ i ] );
    taken[ i ] = 1;
    return( true );
    }
   if( R[ rv[ i ] ].check( i ) ) {
    R[ rv[ i ] ].reset();
    U.link( rv[ i ] , ru[ i ] );
    taken[ i ] = 1;
    return( true );
    }
   return( false );
   }
  } step{ ids , ends , U , R , ru , rv , taken };
 speculativeFor( ex , step , 0 , k );
 for( std::size_t i = 0 ; i < k ; ++i )
  if( taken[ i ] )
   out.push_back( ids[ i ] );
 }

/// the items x of ids with keep( x ), in their order, in parallel
template< class X , class P >
std::vector< int > parallelFilter( X & ex , const std::vector< int > & ids ,
                                   P && keep )
{
 const std::size_t k = ids.size();
 std::vector< char > flag( k );
 parallelFor( ex , 0 , k , [ & ]( std::size_t i ) {
  flag[ i ] = keep( ids[ i ] ) ? 1 : 0; } );
 std::vector< std::size_t > I( ids.begin() , ids.end() ) , out( k );
 const std::size_t c = packKept( ex , I , flag , k , out );
 return( std::vector< int >( out.begin() , out.begin() +
                                           std::ptrdiff_t( c ) ) );
 }

/// whether the edges taken are those of the sequential greedy algorithm
/// on the order ids
template< class GetEnds >
bool sameGreedy( int n , const std::vector< int > & ids , GetEnds && ends ,
                 const std::vector< int > & taken )
{
 GreedyUnionFind U( n );                // no audit of its own, unlike
 std::vector< int > seq;                // UnionFind's, n per merge
 for( int id : ids ) {
  const auto [ u , v ] = ends( id );
  const int a = U.find( u ) , b = U.find( v );
  if( a != b ) {
   U.link( a , b );
   seq.push_back( id );
   }
  }
 return( seq == taken );
 }

/// the ids of A sorted by ( w , id ), in parallel
template< class X , class W >
std::vector< int > byWeight( X & ex ,
                             const std::vector< WeightedEdge< W > > & A ,
                             std::vector< int > ids )
{
 ADS::sort( ex , ids , [ & ]( int a , int b ) {
  return( ( A[ a ].w < A[ b ].w ) ||
          ( ! ( A[ b ].w < A[ a ].w ) && ( a < b ) ) ); } );
 return( ids );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , class W >
std::vector< int > parallelKruskal( E & ex , int n ,
                                const std::vector< WeightedEdge< W > > & A )
{
 ADS_PRE( n >= 0 );
 std::vector< int > ids( A.size() );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  ADS_PRE( ( A[ i ].u >= 0 ) && ( A[ i ].u < n ) && ( A[ i ].v >= 0 ) &&
           ( A[ i ].v < n ) );
  ids[ i ] = int( i );
  }
 ids = detail::byWeight( ex , A , std::move( ids ) );
 auto ends = [ & ]( int id ) {
  return( std::pair< int , int >( A[ id ].u , A[ id ].v ) ); };
 detail::GreedyUnionFind U( n );
 std::vector< int > out;
 detail::greedyForest( ex , ids , ends , U , out );
 ADS_AUDIT( detail::sameGreedy( n , ids , ends , out ) );
 return( out );
 }

template< Executor E , class W >
std::vector< int > filterKruskal( E & ex , int n ,
                                const std::vector< WeightedEdge< W > > & A )
{
 ADS_PRE( n >= 0 );
 std::vector< int > ids( A.size() );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  ADS_PRE( ( A[ i ].u >= 0 ) && ( A[ i ].u < n ) && ( A[ i ].v >= 0 ) &&
           ( A[ i ].v < n ) );
  ids[ i ] = int( i );
  }
 auto ends = [ & ]( int id ) {
  return( std::pair< int , int >( A[ id ].u , A[ id ].v ) ); };
 auto less = [ & ]( int a , int b ) {
  return( ( A[ a ].w < A[ b ].w ) ||
          ( ! ( A[ b ].w < A[ a ].w ) && ( a < b ) ) ); };
 detail::GreedyUnionFind U( n );
 std::vector< int > out;
 const std::size_t base = std::max< std::size_t >( 4096 ,
                                                   2 * std::size_t( n ) );
 auto rec = [ & ]( auto && self , std::vector< int > I ) -> void {
  if( I.size() <= base ) {
   detail::greedyForest( ex , detail::byWeight( ex , A , std::move( I ) ) ,
                         ends , U , out );
   return;
   }
  // the pivot: the median of a sample of 1023 places, evenly spaced
  std::vector< int > S;
  for( std::size_t j = 0 ; j < 1023 ; ++j )
   S.push_back( I[ j * I.size() / 1023 ] );
  std::nth_element( S.begin() , S.begin() + 511 , S.end() , less );
  const int p = S[ 511 ];
  std::vector< int > light = detail::parallelFilter( ex , I ,
                             [ & ]( int x ) { return( less( x , p ) ); } );
  std::vector< int > heavy = detail::parallelFilter( ex , I ,
                             [ & ]( int x ) { return( ! less( x , p ) ); } );
  I.clear();
  I.shrink_to_fit();
  self( self , std::move( light ) );
  heavy = detail::parallelFilter( ex , heavy , [ & ]( int x ) {
   return( U.find( A[ x ].u ) != U.find( A[ x ].v ) ); } );
  self( self , std::move( heavy ) );
  };
 rec( rec , std::move( ids ) );
 ADS_AUDIT( ( [ & ]( void ) {
  std::vector< int > all( A.size() );
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   all[ i ] = int( i );
  std::sort( all.begin() , all.end() , less );
  return( detail::sameGreedy( n , all , ends , out ) );
  }() ) );
 return( out );
 }

template< Executor E >
std::vector< int > spanningForest( E & ex , int n ,
                          const std::vector< std::pair< int , int > > & A )
{
 ADS_PRE( n >= 0 );
 std::vector< int > ids( A.size() );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  ADS_PRE( ( A[ i ].first >= 0 ) && ( A[ i ].first < n ) &&
           ( A[ i ].second >= 0 ) && ( A[ i ].second < n ) );
  ids[ i ] = int( i );
  }
 auto ends = [ & ]( int id ) { return( A[ id ] ); };
 detail::GreedyUnionFind U( n );
 std::vector< int > out;
 detail::greedyForest( ex , ids , ends , U , out );
 ADS_AUDIT( detail::sameGreedy( n , ids , ends , out ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class W >
std::vector< int > maximalIndependentSet( E & ex ,
                                          const StaticGraph< W > & g ,
                                          const std::vector< int > & pi )
{
 using G = StaticGraph< W >;
 const int n = g.n();
 ADS_PRE( ! g.directed() && ( int( pi.size() ) == n ) );
 std::vector< int > rank( n , -1 );
 for( int i = 0 ; i < n ; ++i ) {
  ADS_PRE( ( pi[ i ] >= 0 ) && ( pi[ i ] < n ) && ( rank[ pi[ i ] ] < 0 ) );
  rank[ pi[ i ] ] = i;
  }
 constexpr char UNDECIDED = 0 , IN = 1 , OUT = 2;
 std::vector< char > state( n , UNDECIDED );
 auto sequential = [ & ]( void ) {
  std::vector< char > s( n , UNDECIDED );
  for( int i = 0 ; i < n ; ++i ) {
   const int v = pi[ i ];
   s[ v ] = IN;
   for( const auto & a : g.out( v ) )
    if( s[ G::head( a ) ] == IN ) {
     s[ v ] = OUT;
     break;
     }
   }
  return( s );
  };
 if( ( ex.concurrency() <= 1 ) || ( n < 4096 ) )
  state = sequential();
 else {
  std::vector< char > next( n , UNDECIDED );
  struct Step {
   const G & g;
   const std::vector< int > & pi , & rank;
   std::vector< char > & state , & next;

   bool reserve( std::size_t i ) {
    const int v = pi[ i ];
    char s = IN;
    for( const auto & a : g.out( v ) ) {
     const int u = G::head( a );
     if( rank[ u ] < int( i ) ) {
      if( state[ u ] == IN ) {
       s = OUT;
       break;
       }
      if( state[ u ] == UNDECIDED )
       s = UNDECIDED;
      }
     }
    next[ v ] = s;
    return( true );
    }

   bool commit( std::size_t i ) {
    const int v = pi[ i ];
    state[ v ] = next[ v ];
    return( state[ v ] != UNDECIDED );
    }
   } step{ g , pi , rank , state , next };
  speculativeFor( ex , step , 0 , std::size_t( n ) );
  }
 ADS_AUDIT( state == sequential() );
 std::vector< int > out;
 for( int v = 0 ; v < n ; ++v )
  if( state[ v ] == IN )
   out.push_back( v );
 return( out );
 }

template< Executor E >
std::vector< int > maximalMatching( E & ex , int n ,
                          const std::vector< std::pair< int , int > > & A ,
                          const std::vector< int > & pi )
{
 ADS_PRE( ( n >= 0 ) && ( pi.size() == A.size() ) );
 const std::size_t m = A.size();
 {
  std::vector< char > seen( m , 0 );
  for( std::size_t i = 0 ; i < m ; ++i ) {
   ADS_PRE( ( A[ i ].first >= 0 ) && ( A[ i ].first < n ) &&
            ( A[ i ].second >= 0 ) && ( A[ i ].second < n ) );
   ADS_PRE( ( pi[ i ] >= 0 ) && ( std::size_t( pi[ i ] ) < m ) &&
            ! seen[ pi[ i ] ] );
   seen[ pi[ i ] ] = 1;
   }
 }
 std::vector< char > taken( m , 0 );
 auto sequential = [ & ]( void ) {
  std::vector< char > busy( n , 0 ) , t( m , 0 );
  for( std::size_t i = 0 ; i < m ; ++i ) {
   const auto [ u , v ] = A[ pi[ i ] ];
   if( ( u != v ) && ! busy[ u ] && ! busy[ v ] ) {
    busy[ u ] = busy[ v ] = 1;
    t[ pi[ i ] ] = 1;
    }
   }
  return( t );
  };
 if( ( ex.concurrency() <= 1 ) || ( m < 4096 ) )
  taken = sequential();
 else {
  std::vector< char > busy( n , 0 );
  std::vector< Reservation > R( static_cast< std::size_t >( n ) );
  struct Step {
   const std::vector< std::pair< int , int > > & A;
   const std::vector< int > & pi;
   std::vector< char > & busy , & taken;
   std::vector< Reservation > & R;

   bool reserve( std::size_t i ) {
    const auto [ u , v ] = A[ pi[ i ] ];
    if( ( u == v ) || busy[ u ] || busy[ v ] )
     return( false );
    R[ u ].reserve( i );
    R[ v ].reserve( i );
    return( true );
    }

   bool commit( std::size_t i ) {
    const auto [ u , v ] = A[ pi[ i ] ];
    if( R[ v ].check( i ) ) {
     R[ v ].reset();
     if( R[ u ].check( i ) ) {
      R[ u ].reset();
      busy[ u ] = busy[ v ] = 1;
      taken[ pi[ i ] ] = 1;
      return( true );
      }
     }
    else
     R[ u ].checkReset( i );
    return( false );
    }
   } step{ A , pi , busy , taken , R };
  speculativeFor( ex , step , 0 , m );
  }
 ADS_AUDIT( taken == sequential() );
 std::vector< int > out;
 for( std::size_t i = 0 ; i < m ; ++i )
  if( taken[ i ] )
   out.push_back( int( i ) );
 return( out );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ParallelGreedy.tpp -----------------------*/
/*--------------------------------------------------------------------------*/

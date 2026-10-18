/*--------------------------------------------------------------------------*/
/*-------------------------- File SatSolvers.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The solvers of SatSolvers.h, with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the truth of the literal l under a ( -1 free, 0 false, 1 true ), as
/// -1 false, 0 free, 1 true
inline int truth( const std::vector< int > & a , int l )
{
 const int x = a[ std::abs( l ) - 1 ];
 if( x < 0 ) return( 0 );
 return( ( x == 1 ) == ( l > 0 ) ? 1 : -1 );
 }

/// DPLL on the partial assignment a
inline bool dpll( const Cnf & F , std::vector< int > & a )
{
 std::vector< int > forced;                 // to undo on the way back
 auto undo = [ & ]( void ) { for( int v : forced ) a[ v ] = -1; };
 for( bool changed = true ; changed ; ) {   // the unit propagation
  changed = false;
  for( const auto & c : F ) {
   int free = 0 , last = 0;
   bool sat = false;
   for( int l : c ) {
    const int t = truth( a , l );
    if( t > 0 ) { sat = true; break; }
    if( t == 0 ) { ++free; last = l; }
    }
   if( sat ) continue;
   if( free == 0 ) { undo(); return( false ); }   // a contradiction
   if( free == 1 ) {
    a[ std::abs( last ) - 1 ] = last > 0;
    forced.push_back( std::abs( last ) - 1 );
    changed = true;
    }
   }
  }
 int branch = 0;                            // a literal of a shortest
 std::size_t best = 0;                      // open clause
 for( const auto & c : F ) {
  std::size_t free = 0;
  int pick = 0;
  bool sat = false;
  for( int l : c ) {
   const int t = truth( a , l );
   if( t > 0 ) { sat = true; break; }
   if( t == 0 ) { ++free; if( ! pick ) pick = l; }
   }
  if( ! sat && ( ( branch == 0 ) || ( free < best ) ) ) {
   branch = pick;
   best = free;
   }
  }
 if( branch == 0 ) return( true );          // every clause satisfied
 const int v = std::abs( branch ) - 1;
 for( int val : { int( branch > 0 ) , int( branch < 0 ) } ) {
  a[ v ] = val;
  if( dpll( F , a ) ) return( true );
  }
 a[ v ] = -1;
 undo();
 return( false );
 }

}  // end( namespace detail )

inline std::optional< std::vector< bool > > dpll( const Cnf & F , int n )
{
 ADS_PRE( n >= 0 );
 for( const auto & c : F )
  for( int l : c ) ADS_PRE( ( l != 0 ) && ( std::abs( l ) <= n ) );
 std::vector< int > a( n , -1 );
 if( ! detail::dpll( F , a ) ) return( std::nullopt );
 std::vector< bool > x( n );
 for( int v = 0 ; v < n ; ++v ) x[ v ] = a[ v ] == 1;   // the free: false
 ADS_POST( satCertificate( F , x ) );
 return( x );
 }

inline Cdcl::Cdcl( const Cnf & F , int n )
 : f_F( F ) , f_n( n )
{
 ADS_PRE( n >= 0 );                         // before sizing by n
 for( const auto & c : F )
  for( int l : c ) ADS_PRE( ( l != 0 ) && ( std::abs( l ) <= n ) );
 f_watch.resize( 2 * std::size_t( n ) );
 f_assign.assign( n , -1 );
 f_level.assign( n , 0 );
 f_reason.assign( n , -1 );
 f_phase.assign( n , false );
 f_seen.assign( n , false );
 f_act.assign( n , 0 );
 for( int v = 0 ; v < n ; ++v ) f_heap.emplace( 0 , v );
 for( const auto & c : F ) {
  std::vector< int > k;
  for( int l : c ) k.push_back( code( l ) );
  std::sort( k.begin() , k.end() );
  k.erase( std::unique( k.begin() , k.end() ) , k.end() );
  bool taut = false;                        // x and not x: always true
  for( std::size_t i = 1 ; i < k.size() ; ++i )
   taut = taut || ( k[ i ] == ( k[ i - 1 ] ^ 1 ) );
  if( taut ) continue;
  if( k.empty() ) { f_empty = true; continue; }
  if( k.size() == 1 ) {                     // a unit at level 0
   const int t = value( k[ 0 ] );
   if( t < 0 ) f_empty = true;
   else if( t == 0 ) enqueue( k[ 0 ] , -1 );
   continue;
   }
  addClause( std::move( k ) );
  }
 }

inline int Cdcl::value( int c ) const
{
 const int x = f_assign[ c >> 1 ];
 if( x < 0 ) return( 0 );
 return( ( x == 1 ) == ( ( c & 1 ) == 0 ) ? 1 : -1 );
 }

inline void Cdcl::enqueue( int c , int reason )
{
 const int v = c >> 1;
 f_assign[ v ] = ( c & 1 ) == 0;
 f_level[ v ] = level();
 f_reason[ v ] = reason;
 f_trail.push_back( c );
 }

inline int Cdcl::addClause( std::vector< int > c )
{
 const int ci = int( f_C.size() );
 f_watch[ c[ 0 ] ].push_back( ci );         // the first two watched
 f_watch[ c[ 1 ] ].push_back( ci );
 f_C.push_back( std::move( c ) );
 return( ci );
 }

inline int Cdcl::propagate( void )
{
 while( f_qhead < f_trail.size() ) {
  const int fl = f_trail[ f_qhead++ ] ^ 1;  // the literal now false
  auto & ws = f_watch[ fl ];
  std::size_t i = 0 , j = 0;
  while( i < ws.size() ) {
   const int ci = ws[ i++ ];
   auto & c = f_C[ ci ];
   if( c[ 0 ] == fl ) std::swap( c[ 0 ] , c[ 1 ] );   // c[ 1 ] is false
   if( value( c[ 0 ] ) > 0 ) {              // already satisfied
    ws[ j++ ] = ci;
    continue;
    }
   bool moved = false;                      // another literal to watch
   for( std::size_t k = 2 ; k < c.size() ; ++k )
    if( value( c[ k ] ) >= 0 ) {
     std::swap( c[ 1 ] , c[ k ] );
     f_watch[ c[ 1 ] ].push_back( ci );
     moved = true;
     break;
     }
   if( moved ) continue;
   ws[ j++ ] = ci;
   if( value( c[ 0 ] ) < 0 ) {              // a conflict
    while( i < ws.size() ) ws[ j++ ] = ws[ i++ ];
    ws.resize( j );
    f_qhead = f_trail.size();
    return( ci );
    }
   enqueue( c[ 0 ] , ci );                  // a unit: c[ 0 ] forced
   }
  ws.resize( j );
  }
 return( -1 );
 }

inline void Cdcl::bump( int v )
{
 if( ( f_act[ v ] += f_inc ) > 1e100 ) {    // rescale all, rebuild
  for( auto & a : f_act ) a *= 1e-100;
  f_inc *= 1e-100;
  f_heap = {};
  for( int u = 0 ; u < f_n ; ++u ) f_heap.emplace( f_act[ u ] , u );
  }
 else
  f_heap.emplace( f_act[ v ] , v );         // the old entry goes stale
 }

inline void Cdcl::analyze( int confl , std::vector< int > & learnt ,
                           int & back )
{
 learnt.assign( 1 , -1 );                   // room for the UIP
 int pathC = 0 , p = -1;
 std::size_t idx = f_trail.size();
 do {
  const auto & c = f_C[ confl ];
  for( std::size_t j = ( p < 0 ) ? 0 : 1 ; j < c.size() ; ++j ) {
   const int q = c[ j ] , v = q >> 1;
   if( f_seen[ v ] || ( f_level[ v ] == 0 ) ) continue;
   f_seen[ v ] = true;
   bump( v );
   if( f_level[ v ] >= level() ) ++pathC;   // to resolve away
   else learnt.push_back( q );
   }
  while( ! f_seen[ f_trail[ --idx ] >> 1 ] ) {}   // the last one seen
  p = f_trail[ idx ];
  confl = f_reason[ p >> 1 ];
  f_seen[ p >> 1 ] = false;
  --pathC;
  } while( pathC > 0 );
 learnt[ 0 ] = p ^ 1;                       // the first UIP, negated
 back = 0;
 std::size_t at = 1;
 for( std::size_t i = 1 ; i < learnt.size() ; ++i ) {
  f_seen[ learnt[ i ] >> 1 ] = false;
  if( f_level[ learnt[ i ] >> 1 ] > back ) {
   back = f_level[ learnt[ i ] >> 1 ];
   at = i;
   }
  }
 if( learnt.size() > 1 ) std::swap( learnt[ 1 ] , learnt[ at ] );
 f_inc /= 0.95;                             // the decay of VSIDS
 }

inline void Cdcl::cancelUntil( int lev )
{
 if( level() <= lev ) return;
 const auto from = std::size_t( f_trailLim[ lev ] );
 for( std::size_t i = f_trail.size() ; i-- > from ; ) {
  const int v = f_trail[ i ] >> 1;
  f_phase[ v ] = f_assign[ v ] == 1;        // phase saving
  f_assign[ v ] = -1;
  f_reason[ v ] = -1;
  f_heap.emplace( f_act[ v ] , v );
  }
 f_trail.resize( f_trailLim[ lev ] );
 f_trailLim.resize( lev );
 f_qhead = f_trail.size();
 }

inline int Cdcl::pickBranch( void )
{
 while( ! f_heap.empty() ) {
  const auto [ a , v ] = f_heap.top();
  f_heap.pop();
  if( ( f_assign[ v ] < 0 ) && ( a == f_act[ v ] ) ) return( v );
  }
 for( int v = 0 ; v < f_n ; ++v )           // the stale entries gone
  if( f_assign[ v ] < 0 ) return( v );
 return( -1 );
 }

namespace detail {

/// the i-th term of the sequence of Luby, 1 1 2 1 1 2 4 ...
inline long luby( long i )
{
 long k = 1;
 while( ( ( 1L << k ) - 1 ) < i ) ++k;
 while( i != ( 1L << k ) - 1 ) {
  i -= ( 1L << ( k - 1 ) ) - 1;
  k = 1;
  while( ( ( 1L << k ) - 1 ) < i ) ++k;
  }
 return( 1L << ( k - 1 ) );
 }

}  // end( namespace detail )

inline Cdcl::Result Cdcl::solve( long maxConflicts )
{
 if( f_empty ) return( Result::UNSAT );
 std::vector< int > learnt;
 long restart = 1 , budget = 100 * detail::luby( 1 );
 for( ; ; ) {
  const int confl = propagate();
  if( confl >= 0 ) {
   ++f_conflicts;
   if( level() == 0 ) {
    f_empty = true;
    return( Result::UNSAT );
    }
   int back;
   analyze( confl , learnt , back );
   cancelUntil( back );
   if( learnt.size() == 1 )
    enqueue( learnt[ 0 ] , -1 );
   else
    enqueue( learnt[ 0 ] , addClause( learnt ) );
   if( ( maxConflicts >= 0 ) && ( f_conflicts >= maxConflicts ) )
    return( Result::UNKNOWN );
   if( --budget == 0 ) {                    // a restart
    cancelUntil( 0 );
    budget = 100 * detail::luby( ++restart );
    }
   continue;
   }
  const int v = pickBranch();
  if( v < 0 ) {                             // all assigned, no conflict
   f_model.assign( f_n , false );
   for( int u = 0 ; u < f_n ; ++u ) f_model[ u ] = f_assign[ u ] == 1;
   ADS_POST( satCertificate( f_F , f_model ) );
   return( Result::SAT );
   }
  ++f_decisions;
  f_trailLim.push_back( int( f_trail.size() ) );
  enqueue( 2 * v + ( f_phase[ v ] ? 0 : 1 ) , -1 );
  }
 }

inline const std::vector< bool > & Cdcl::model( void ) const
{
 return( f_model );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SatSolvers.tpp -------------------------*/
/*--------------------------------------------------------------------------*/

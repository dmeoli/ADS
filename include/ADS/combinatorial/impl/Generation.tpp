/*--------------------------------------------------------------------------*/
/*-------------------------- File Generation.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The generators of Generation.h, with their contracts.
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

namespace ADS {

namespace detail {

/// visit( x ), true if it asks to stop; never if it returns void
template< class F , class X >
inline bool visitStops( F & visit , X && x )
{
 if constexpr( std::is_void_v< std::invoke_result_t< F & , X && > > ) {
  visit( std::forward< X >( x ) );
  return( false );
  }
 else
  return( bool( visit( std::forward< X >( x ) ) ) );
 }

}  // end( namespace detail )

template< class F >
long mixedRadix( const std::vector< int > & m , F && visit )
{
 ADS_PRE( std::all_of( m.begin() , m.end() , []( int x ) {
  return( x >= 1 ); } ) );
 const int n = int( m.size() );
 Tuple a( n , 0 );
 long count = 0;
 for( ; ; ) {
  ++count;
  if( detail::visitStops( visit , a ) ) return( count );
  int j = n - 1;                            // M3-M4: add 1 with carries
  while( ( j >= 0 ) && ( a[ j ] == m[ j ] - 1 ) ) a[ j-- ] = 0;
  if( j < 0 ) return( count );
  ++a[ j ];
  }
 }

template< class F >
long grayTuples( const std::vector< int > & m , F && visit )
{
 ADS_PRE( std::all_of( m.begin() , m.end() , []( int x ) {
  return( x >= 2 ); } ) );
 const int n = int( m.size() );
 if( n == 0 ) {                             // the empty tuple alone
  (void) detail::visitStops( visit , Tuple() );
  return( 1 );
  }
 Tuple a( n , 0 ) , o( n , 1 );             // the digits and directions,
 std::vector< int > f( n + 1 );             // the focus pointers
 for( int j = 0 ; j <= n ; ++j ) f[ j ] = j;
 long count = 0;
 for( ; ; ) {                               // places from the right: the
  ++count;                                  // place j is a[ n - 1 - j ]
  if( detail::visitStops( visit , a ) ) return( count );
  const int j = f[ 0 ];                     // H3
  f[ 0 ] = 0;
  if( j == n ) return( count );
  int & d = a[ n - 1 - j ];
  d += o[ j ];                              // H4: change the place j
  if( ( d == 0 ) || ( d == m[ n - 1 - j ] - 1 ) ) {
   o[ j ] = -o[ j ];                        // H5: the end of its range
   f[ j ] = f[ j + 1 ];
   f[ j + 1 ] = j + 1;
   }
  }
 }

template< class F >
long permutationsLex( Tuple a , F && visit )
{
 std::sort( a.begin() , a.end() );
 long count = 0;
 const int n = int( a.size() );
 for( ; ; ) {
  ++count;
  if( detail::visitStops( visit , a ) ) return( count );
  int j = n - 2;                            // L2: the last ascent
  while( ( j >= 0 ) && ( a[ j ] >= a[ j + 1 ] ) ) --j;
  if( j < 0 ) return( count );
  int l = n - 1;                            // L3: the last larger item
  while( a[ j ] >= a[ l ] ) --l;
  std::swap( a[ j ] , a[ l ] );
  std::reverse( a.begin() + j + 1 , a.end() );   // L4
  }
 }

template< class F >
long plainChanges( int n , F && visit )
{
 ADS_PRE( n >= 0 );
 Tuple a( n + 1 ) , c( n + 1 , 0 ) , o( n + 1 , 1 );   // 1-based, as P
 for( int i = 1 ; i <= n ; ++i ) a[ i ] = i - 1;
 long count = 0;
 for( ; ; ) {
  ++count;                                  // P2
  if( detail::visitStops( visit , Tuple( a.begin() + 1 , a.end() ) ) )
   return( count );
  int j = n , s = 0;                        // P3
  for( ; ; ) {
   if( j < 1 ) return( count );
   const int q = c[ j ] + o[ j ];           // P4
   if( q < 0 ) {                            // P7
    o[ j ] = -o[ j ];
    --j;
    continue;
    }
   if( q == j ) {                           // P6
    if( j == 1 ) return( count );
    ++s;
    o[ j ] = -o[ j ];
    --j;
    continue;
    }
   std::swap( a[ j - c[ j ] + s ] , a[ j - q + s ] );   // P5
   c[ j ] = q;
   break;
   }
  }
 }

template< class F >
long heapPermutations( int n , F && visit )
{
 ADS_PRE( n >= 0 );
 Tuple a( n ) , c( n , 0 );
 for( int i = 0 ; i < n ; ++i ) a[ i ] = i;
 long count = 1;
 if( detail::visitStops( visit , a ) ) return( count );
 for( int i = 1 ; i < n ; ) {               // the iterative form of Heap
  if( c[ i ] < i ) {
   std::swap( a[ i % 2 ? c[ i ] : 0 ] , a[ i ] );
   ++count;
   if( detail::visitStops( visit , a ) ) return( count );
   ++c[ i ];
   i = 1;
   }
  else
   c[ i++ ] = 0;
  }
 return( count );
 }

template< class F >
long combinationsLex( int n , int t , F && visit )
{
 ADS_PRE( ( t >= 0 ) && ( t <= n ) );
 Tuple c( t );
 for( int j = 0 ; j < t ; ++j ) c[ j ] = j;
 long count = 0;
 for( ; ; ) {
  ++count;
  if( detail::visitStops( visit , c ) ) return( count );
  int j = t - 1;                            // the last that can grow
  while( ( j >= 0 ) && ( c[ j ] == n - t + j ) ) --j;
  if( j < 0 ) return( count );
  ++c[ j ];
  for( int k = j + 1 ; k < t ; ++k ) c[ k ] = c[ k - 1 ] + 1;
  }
 }

namespace detail {

/// the revolving door of the t-combinations of 0 , ... , n - 1, by the
/// recursion C( n , t ) = C( n - 1 , t ) , ( C( n - 1 , t - 1 ) reversed
/// ) + { n - 1 }; false if stopped
template< class F >
bool door( int n , int t , bool reversed , Tuple & c , long & count ,
           F & visit )
{
 if( t == 0 ) {
  ++count;
  return( ! visitStops( visit , c ) );
  }
 if( n == t ) {
  for( int j = 0 ; j < t ; ++j ) c[ j ] = j;
  ++count;
  return( ! visitStops( visit , c ) );
  }
 auto withLast = [ & ]( bool rev ) {        // the combinations with n - 1
  c[ t - 1 ] = n - 1;
  return( door( n - 1 , t - 1 , rev , c , count , visit ) );
  };
 if( ! reversed ) {
  if( ! door( n - 1 , t , false , c , count , visit ) ) return( false );
  return( withLast( true ) );
  }
 if( ! withLast( false ) ) return( false );
 return( door( n - 1 , t , true , c , count , visit ) );
 }

}  // end( namespace detail )

template< class F >
long revolvingDoor( int n , int t , F && visit )
{
 ADS_PRE( ( t >= 0 ) && ( t <= n ) );
 Tuple c( t );
 long count = 0;
 detail::door( n , t , false , c , count , visit );
 return( count );
 }

template< class F >
long integerPartitions( int n , F && visit )
{
 ADS_PRE( n >= 0 );
 long count = 0;
 if( n == 0 ) {
  ++count;
  (void) detail::visitStops( visit , Tuple() );
  return( count );
  }
 Tuple a = { n };                           // the parts, nonincreasing
 for( ; ; ) {
  ++count;
  if( detail::visitStops( visit , a ) ) return( count );
  int ones = 0;                             // the 1s at the end go away
  while( ! a.empty() && ( a.back() == 1 ) ) {
   a.pop_back();
   ++ones;
   }
  if( a.empty() ) return( count );
  const int x = --a.back();                 // the last part > 1 drops by 1
  int rest = ones + 1;                      // and the rest is split into
  while( rest > x ) {                       // parts of x
   a.push_back( x );
   rest -= x;
   }
  if( rest > 0 ) a.push_back( rest );
  }
 }

template< class F >
long setPartitions( int n , F && visit )
{
 ADS_PRE( n >= 0 );
 long count = 0;
 if( n == 0 ) {
  ++count;
  (void) detail::visitStops( visit , Tuple() );
  return( count );
  }
 Tuple a( n , 0 ) , b( n , 1 );             // b[ j ] = 1 + max( a[ 0 .. j ) )
 for( ; ; ) {
  ++count;
  if( detail::visitStops( visit , a ) ) return( count );
  int j = n - 1;                            // the last that can grow
  while( ( j > 0 ) && ( a[ j ] == b[ j ] ) ) --j;
  if( j == 0 ) return( count );
  ++a[ j ];
  for( int k = j + 1 ; k < n ; ++k ) {
   a[ k ] = 0;
   b[ k ] = std::max( b[ k - 1 ] , a[ k - 1 ] + 1 );
   }
  }
 }

template< class F >
long nestedParentheses( int n , F && visit )
{
 ADS_PRE( n >= 0 );
 std::string s( std::size_t( 2 * n ) , ' ' );
 long count = 0;
 auto rec = [ & ]( auto & self , int i , int open , int close ) -> bool {
  if( i == 2 * n ) {
   ++count;
   return( ! detail::visitStops( visit , s ) );
   }
  if( open < n ) {                          // '(' before ')' : lexicographic
   s[ i ] = '(';
   if( ! self( self , i + 1 , open + 1 , close ) ) return( false );
   }
  if( close < open ) {
   s[ i ] = ')';
   if( ! self( self , i + 1 , open , close + 1 ) ) return( false );
   }
  return( true );
  };
 rec( rec , 0 , 0 , 0 );
 return( count );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Generation.tpp -------------------------*/
/*--------------------------------------------------------------------------*/

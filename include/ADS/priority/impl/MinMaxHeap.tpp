/*--------------------------------------------------------------------------*/
/*--------------------------- File MinMaxHeap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MinMaxHeap and IntervalHeap [see MinMaxHeap.h], with
 * their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------------ MinMaxHeap --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Better >
void MinMaxHeap< T , Better >::run( void )
{
 if( f_sorted ||
     ( ++f_run < std::max< std::size_t >( 64 , f_A.size() / 8 ) ) )
  return;
 if constexpr( detail::kKeyed< T , Better > )
  radixSortBy( f_A , []( const T & x ) {
   return( detail::orderKeyBy< Better >( x ) ); } );
 else
  ADS::sort( f_A , f_better );
 f_sorted = true;
 f_lo = 0;
 f_hi = f_A.size();
 }

template< class T , class Better >
void MinMaxHeap< T , Better >::unsort( void )
{
 f_A.erase( f_A.begin() + std::ptrdiff_t( f_hi ) , f_A.end() );
 f_A.erase( f_A.begin() , f_A.begin() + std::ptrdiff_t( f_lo ) );
 f_sorted = false;
 f_run = 0;
 for( std::size_t i = f_A.size() / 2 + 1 ; i-- > 0 ; )   // bottom up
  if( minLevel( i ) )
   down< true >( i );
  else
   down< false >( i );
 }

template< class T , class Better >
template< bool Min >
inline void MinMaxHeap< T , Better >::up( std::size_t i )
{
 while( i >= 3 ) {
  const std::size_t g = ( ( i - 1 ) / 2 - 1 ) / 2;
  if( ! first< Min >( f_A[ i ] , f_A[ g ] ) )
   break;
  std::swap( f_A[ i ] , f_A[ g ] );
  i = g;
  }
 }

template< class T , class Better >
inline void MinMaxHeap< T , Better >::push( const T & x )
{
 if( f_sorted )
  unsort();
 f_run = 0;
 f_A.push_back( x );
 const std::size_t i = f_A.size() - 1;
 if( i > 0 ) {
  const std::size_t p = ( i - 1 ) / 2;
  if( minLevel( i ) ) {
   if( f_better( f_A[ p ] , f_A[ i ] ) ) {   // worse than its parent of a
    std::swap( f_A[ i ] , f_A[ p ] );        // level for the worst
    up< false >( p );
    }
   else
    up< true >( i );
   }
  else if( f_better( f_A[ i ] , f_A[ p ] ) ) {   // better than its parent
   std::swap( f_A[ i ] , f_A[ p ] );             // of a level for the best
   up< true >( p );
   }
  else
   up< false >( i );
  }
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better >
const T & MinMaxHeap< T , Better >::min( void ) const
{
 ADS_PRE( ! empty() );
 if( f_sorted )
  return( f_A[ f_lo ] );
 return( f_A[ 0 ] );
 }

template< class T , class Better >
std::size_t MinMaxHeap< T , Better >::maxAt( void ) const
{
 const std::size_t n = f_A.size();
 if( n <= 2 )
  return( n - 1 );
 return( f_better( f_A[ 1 ] , f_A[ 2 ] ) ? 2 : 1 );
 }

template< class T , class Better >
const T & MinMaxHeap< T , Better >::max( void ) const
{
 ADS_PRE( ! empty() );
 if( f_sorted )
  return( f_A[ f_hi - 1 ] );
 return( f_A[ maxAt() ] );
 }

template< class T , class Better >
template< bool Min >
inline void MinMaxHeap< T , Better >::down( std::size_t i )
{
 const std::size_t n = f_A.size();
 for( ; ; ) {
  const std::size_t c = 2 * i + 1;
  if( c >= n )
   break;
  std::size_t m = c;                        // the first of the children
  if( ( c + 1 < n ) && first< Min >( f_A[ c + 1 ] , f_A[ m ] ) )
   m = c + 1;                               // and the grandchildren
  const std::size_t g = 4 * i + 3 , ge = g + 4 < n ? g + 4 : n;
  for( std::size_t k = g ; k < ge ; ++k )
   if( first< Min >( f_A[ k ] , f_A[ m ] ) )
    m = k;
  if( ! first< Min >( f_A[ m ] , f_A[ i ] ) )
   break;
  std::swap( f_A[ m ] , f_A[ i ] );
  if( m < g )                               // a child: done
   break;
  const std::size_t p = ( m - 1 ) / 2;      // a grandchild, against its
  if( first< Min >( f_A[ p ] , f_A[ m ] ) ) // parent of the other kind
   std::swap( f_A[ p ] , f_A[ m ] );
  i = m;
  }
 }

template< class T , class Better >
T MinMaxHeap< T , Better >::popMin( void )
{
 ADS_PRE( ! empty() );
 run();
 if( f_sorted ) {                           // an end of the sorted items
  T x = std::move( f_A[ f_lo++ ] );
  if( f_lo == f_hi )
   clear();
  ADS_INVARIANT( ordered() );
  return( x );
  }
 T x = std::move( f_A[ 0 ] );
 T y = std::move( f_A.back() );
 f_A.pop_back();
 if( ! f_A.empty() ) {
  f_A[ 0 ] = std::move( y );
  down< true >( 0 );
  }
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better >
T MinMaxHeap< T , Better >::popMax( void )
{
 ADS_PRE( ! empty() );
 run();
 if( f_sorted ) {                           // an end of the sorted items
  T x = std::move( f_A[ --f_hi ] );
  if( f_lo == f_hi )
   clear();
  ADS_INVARIANT( ordered() );
  return( x );
  }
 const std::size_t k = maxAt();
 T x = std::move( f_A[ k ] );
 T y = std::move( f_A.back() );
 f_A.pop_back();
 if( k < f_A.size() ) {
  f_A[ k ] = std::move( y );
  down< false >( k );
  }
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better >
bool MinMaxHeap< T , Better >::ordered( void ) const
{
 if( f_sorted )
  return( std::is_sorted( f_A.begin() + std::ptrdiff_t( f_lo ) ,
                          f_A.begin() + std::ptrdiff_t( f_hi ) , f_better ) );
 if( f_A.size() > 1024 )
  return( true );
 for( std::size_t i = 1 ; i < f_A.size() ; ++i ) {
  const std::size_t p = ( i - 1 ) / 2;
  const bool m = minLevel( i );
  if( m ? f_better( f_A[ p ] , f_A[ i ] ) : f_better( f_A[ i ] , f_A[ p ] ) )
   return( false );
  if( i >= 3 ) {
   const std::size_t g = ( p - 1 ) / 2;
   if( m ? f_better( f_A[ i ] , f_A[ g ] )
         : f_better( f_A[ g ] , f_A[ i ] ) )
    return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- IntervalHeap -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Better >
void IntervalHeap< T , Better >::run( void )
{
 if( f_sorted ||
     ( ++f_run < std::max< std::size_t >( 64 , f_A.size() / 8 ) ) )
  return;
 if constexpr( detail::kKeyed< T , Better > )
  radixSortBy( f_A , []( const T & x ) {
   return( detail::orderKeyBy< Better >( x ) ); } );
 else
  ADS::sort( f_A , f_better );
 f_sorted = true;
 f_lo = 0;
 f_hi = f_A.size();
 }

template< class T , class Better >
void IntervalHeap< T , Better >::unsort( void )
{
 const std::size_t n = f_hi - f_lo;
 std::vector< T > B;                        // the node j: the j-th best
 B.reserve( n );                            // and the j-th worst
 for( std::size_t j = 0 ; j < n / 2 ; ++j ) {
  B.push_back( std::move( f_A[ f_lo + j ] ) );
  B.push_back( std::move( f_A[ f_hi - 1 - j ] ) );
  }
 if( n % 2 )
  B.push_back( std::move( f_A[ f_lo + n / 2 ] ) );
 f_A.swap( B );
 f_sorted = false;
 f_run = 0;
 }

template< class T , class Better >
inline void IntervalHeap< T , Better >::push( const T & x )
{
 if( f_sorted )
  unsort();
 f_run = 0;
 f_A.push_back( x );
 const std::size_t i = f_A.size() - 1 , j = i / 2;
 bool low;                                  // x is the lo of its node
 if( i % 2 == 1 ) {                         // the second item of the node
  low = f_better( f_A[ i ] , f_A[ i - 1 ] );
  if( low )
   std::swap( f_A[ i ] , f_A[ i - 1 ] );
  }
 else {                                     // alone in its node
  if( j == 0 ) {
   ADS_INVARIANT( ordered() );
   return;
   }
  const std::size_t q = ( j - 1 ) / 2;
  if( f_better( x , f_A[ 2 * q ] ) )
   low = true;
  else if( f_better( f_A[ 2 * q + 1 ] , x ) )
   low = false;
  else {
   ADS_INVARIANT( ordered() );
   return;
   }
  }
 if( low )                                  // up by the lo of the nodes
  for( std::size_t k = j ; k > 0 ; ) {
   const std::size_t q = ( k - 1 ) / 2;
   if( ! f_better( f_A[ 2 * k ] , f_A[ 2 * q ] ) )
    break;
   std::swap( f_A[ 2 * k ] , f_A[ 2 * q ] );
   k = q;
   }
 else                                       // up by the hi of the nodes
  for( std::size_t k = j , pos = i ; k > 0 ; ) {
   const std::size_t q = ( k - 1 ) / 2;
   if( ! f_better( f_A[ 2 * q + 1 ] , f_A[ pos ] ) )
    break;
   std::swap( f_A[ pos ] , f_A[ 2 * q + 1 ] );
   pos = 2 * q + 1;
   k = q;
   }
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better >
const T & IntervalHeap< T , Better >::min( void ) const
{
 ADS_PRE( ! empty() );
 if( f_sorted )
  return( f_A[ f_lo ] );
 return( f_A[ 0 ] );
 }

template< class T , class Better >
const T & IntervalHeap< T , Better >::max( void ) const
{
 ADS_PRE( ! empty() );
 if( f_sorted )
  return( f_A[ f_hi - 1 ] );
 return( f_A[ f_A.size() > 1 ? 1 : 0 ] );
 }

template< class T , class Better >
T IntervalHeap< T , Better >::popMin( void )
{
 ADS_PRE( ! empty() );
 run();
 if( f_sorted ) {                           // an end of the sorted items
  T x = std::move( f_A[ f_lo++ ] );
  if( f_lo == f_hi )
   clear();
  ADS_INVARIANT( ordered() );
  return( x );
  }
 T x = std::move( f_A[ 0 ] );
 if( f_A.size() <= 2 ) {
  if( f_A.size() == 2 )
   f_A[ 0 ] = std::move( f_A[ 1 ] );
  f_A.pop_back();
  ADS_INVARIANT( ordered() );
  return( x );
  }
 T y = std::move( f_A.back() );             // the last item into the hole
 f_A.pop_back();                            // of the lo of the root, down
 const std::size_t n = f_A.size();          // by the better lo of the
 std::size_t j = 0;                         // children
 for( ; ; ) {
  const std::size_t c1 = 2 * j + 1 , c2 = c1 + 1;
  if( 2 * c1 >= n )
   break;
  const std::size_t c = ( ( 2 * c2 < n ) &&
                          f_better( f_A[ 2 * c2 ] , f_A[ 2 * c1 ] ) ) ? c2
                                                                     : c1;
  if( ! f_better( f_A[ 2 * c ] , y ) )
   break;
  f_A[ 2 * j ] = std::move( f_A[ 2 * c ] );
  j = c;
  if( ( 2 * j + 1 < n ) && f_better( f_A[ 2 * j + 1 ] , y ) )
   std::swap( y , f_A[ 2 * j + 1 ] );       // y beyond the hi: swapped
  }
 f_A[ 2 * j ] = std::move( y );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better >
T IntervalHeap< T , Better >::popMax( void )
{
 ADS_PRE( ! empty() );
 run();
 if( f_sorted ) {                           // an end of the sorted items
  T x = std::move( f_A[ --f_hi ] );
  if( f_lo == f_hi )
   clear();
  ADS_INVARIANT( ordered() );
  return( x );
  }
 if( f_A.size() <= 2 ) {
  T x = std::move( f_A.back() );
  f_A.pop_back();
  ADS_INVARIANT( ordered() );
  return( x );
  }
 T x = std::move( f_A[ 1 ] );
 T y = std::move( f_A.back() );             // the last item into the hole
 f_A.pop_back();                            // of the hi of the root, down
 const std::size_t n = f_A.size();          // by the worse hi of the
 std::size_t j = 0 , hole = 1;              // children
 for( ; ; ) {
  const std::size_t c1 = 2 * j + 1 , c2 = c1 + 1;
  if( 2 * c1 >= n )
   break;
  std::size_t c = c1 , h = hi( c1 );
  if( ( 2 * c2 < n ) && f_better( f_A[ h ] , f_A[ hi( c2 ) ] ) ) {
   c = c2;
   h = hi( c2 );
   }
  if( ! f_better( y , f_A[ h ] ) )
   break;
  f_A[ hole ] = std::move( f_A[ h ] );
  hole = h;
  j = c;
  if( ( h == 2 * j + 1 ) && f_better( y , f_A[ 2 * j ] ) )
   std::swap( y , f_A[ 2 * j ] );           // y below the lo: swapped
  }
 f_A[ hole ] = std::move( y );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better >
bool IntervalHeap< T , Better >::ordered( void ) const
{
 if( f_sorted )
  return( std::is_sorted( f_A.begin() + std::ptrdiff_t( f_lo ) ,
                          f_A.begin() + std::ptrdiff_t( f_hi ) , f_better ) );
 const std::size_t n = f_A.size();
 if( n > 1024 )
  return( true );
 for( std::size_t j = 0 ; 2 * j < n ; ++j ) {
  if( ( 2 * j + 1 < n ) && f_better( f_A[ 2 * j + 1 ] , f_A[ 2 * j ] ) )
   return( false );
  if( j > 0 ) {
   const std::size_t q = ( j - 1 ) / 2;
   if( f_better( f_A[ 2 * j ] , f_A[ 2 * q ] ) ||
       f_better( f_A[ 2 * q + 1 ] , f_A[ hi( j ) ] ) )
    return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MinMaxHeap.tpp ------------------------*/
/*--------------------------------------------------------------------------*/

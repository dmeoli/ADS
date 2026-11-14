/*--------------------------------------------------------------------------*/
/*-------------------------- File IndexedList.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of IndexedList [see IndexedList.h], with their contracts.
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

template< class T >
void rotateArray( std::span< T > a , std::size_t r )
{
 const std::size_t n = a.size();
 if( n == 0 )
  return;
 r %= n;
 const auto old = detail::snapshot( std::vector< T >( a.begin() ,
                                                      a.end() ) );
 auto reverse = [ & ]( std::size_t i , std::size_t j ) {   // a[ i .. j )
  for( ; ( i + 1 < j ) ; ++i , --j )
   std::swap( a[ i ] , a[ j - 1 ] );
  };
 reverse( 0 , n );
 reverse( 0 , r );
 reverse( r , n );
 ADS_AUDIT( ( [ & ] {
  for( std::size_t i = 0 ; i < n ; ++i )
   if( ! detail::equal( ( *old )[ i ] , a[ ( i + r ) % n ] ) )
    return( false );
  return( true ); }() ) );
 }

template< IndexedList L >
void rotate( L & l , std::size_t r )
{
 const std::size_t n = l.size();
 if( n == 0 )
  return;
 r %= n;
 if constexpr( requires { l.rotate( r ); } ) {
  l.rotate( r );
  return;
  }
 else {
  using T = typename L::value_type;
  std::vector< T > old;
  if constexpr( ADS_CONTRACTS >= 3 )
   for( std::size_t i = 0 ; i < n ; ++i ) old.push_back( l[ i ] );
  const std::size_t c = std::gcd( n , r );
  for( std::size_t s = 0 ; ( r > 0 ) && ( s < c ) ; ++s ) {
   T x = std::move( l[ s ] );               // the cycle of s, backwards
   std::size_t j = s;
   for( ; ; ) {
    const std::size_t p = ( j + n - r ) % n;   // the item coming to j
    if( p == s ) break;
    l[ j ] = std::move( l[ p ] );
    j = p;
    }
   l[ j ] = std::move( x );
   }
  ADS_AUDIT( ( [ & ] {
   for( std::size_t i = 0 ; i < n ; ++i )
    if( ! detail::equal( old[ i ] , l[ ( i + r ) % n ] ) )
     return( false );
   return( true ); }() ) );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File IndexedList.tpp ------------------------*/
/*--------------------------------------------------------------------------*/

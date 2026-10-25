/*--------------------------------------------------------------------------*/
/*-------------------------- File StringSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of StringSort.h.
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

namespace detail::pks {

/// the groups sorted by insertion
inline constexpr std::size_t kSmall = 8;

/// a < b, for strings that agree on their characters before d (those
/// that both have)
inline bool lessFrom( const std::string & a , const std::string & b ,
                      std::size_t d )
{
 const std::size_t m = std::min( { d , a.size() , b.size() } );
 return( std::string_view( a ).substr( m ) <
         std::string_view( b ).substr( m ) );
 }

/// the characters d to d + 7 of s, the first the most significant, 0 past
/// its end
inline std::uint64_t window( const std::string & s , std::size_t d )
{
 const std::size_t len = s.size();
 if( d >= len )
  return( 0 );
 std::uint64_t w = 0;
 std::memcpy( &w , s.data() + d , std::min< std::size_t >( len - d , 8 ) );
 if constexpr( std::endian::native == std::endian::little )
  w = detail::byteSwap( w );
 return( w );
 }

/// the characters that all the strings of s[ 0 .. n ) share from d on
inline std::size_t sharedFrom( const std::string * s , std::size_t n ,
                               std::size_t d )
{
 const std::string & c = s[ 0 ];
 std::size_t l = c.size() > d ? c.size() - d : 0;
 for( std::size_t i = 1 ; ( i < n ) && ( l > 0 ) ; ++i ) {
  const std::size_t m = s[ i ].size() > d ? std::min( l ,
                                                      s[ i ].size() - d ) : 0;
  const char * const p = c.data() + d , * const q = s[ i ].data() + d;
  std::size_t j = 0;
  for( ; ( j + 8 <= m ) ; j += 8 ) {        // 8 characters at a time
   std::uint64_t x , y;
   std::memcpy( &x , p + j , 8 );
   std::memcpy( &y , q + j , 8 );
   if( x != y )
    break;
   }
  while( ( j < m ) && ( p[ j ] == q[ j ] ) ) ++j;
  l = j;
  }
 return( l );
 }

/// the sort of s[ 0 .. n ), strings that agree on their characters before
/// d, with the keys k and the room t of n strings, and budget steps left
/// that may leave a group of more than 3 / 4 of the strings
inline void group( std::string * s , std::uint64_t * k , std::string * t ,
                   std::size_t n , std::size_t d , int budget )
{
 auto stable = [ & ]() {
  std::stable_sort( s , s + n , [ d ]( const std::string & a ,
                                       const std::string & b ) {
   return( lessFrom( a , b , d ) ); } );
  };
 if( n <= kSmall ) {                        // by insertion, stable
  for( std::size_t i = 1 ; i < n ; ++i ) {
   std::size_t j = i;
   if( ! lessFrom( s[ j ] , s[ j - 1 ] , d ) )
    continue;
   std::string x = std::move( s[ i ] );
   for( ; ( j > 0 ) && lessFrom( x , s[ j - 1 ] , d ) ; --j )
    s[ j ] = std::move( s[ j - 1 ] );
   s[ j ] = std::move( x );
   }
  return;
  }
 const std::size_t l = sharedFrom( s , n , d );
 if( ( l == ( s[ 0 ].size() > d ? s[ 0 ].size() - d : 0 ) ) &&
     std::all_of( s + 1 , s + n , [ & ]( const std::string & x ) {
      return( x.size() == s[ 0 ].size() ); } ) )
  return;                                   // all equal
 d += l;
 const int b = int( std::bit_width( n - 1 ) );
 const std::uint64_t low = ( std::uint64_t( 1 ) << b ) - 1;
 for( std::size_t i = 0 ; i < n ; ++i )
  k[ i ] = ( window( s[ i ] , d ) & ~low ) | i;
 vectorQuickSort( k , n );
 for( std::size_t j = 0 ; j < n ; ++j )     // the strings to their places
  ::new( static_cast< void * >( t + j ) ) std::string(
   std::move( s[ k[ j ] & low ] ) );
 for( std::size_t j = 0 ; j < n ; ++j ) {
  s[ j ] = std::move( t[ j ] );
  t[ j ].~basic_string();
  }
 const std::size_t next = d + std::size_t( 64 - b ) / 8;
 for( std::size_t j = 0 ; j < n ; ) {       // the groups of equal keys
  std::size_t e = j + 1;
  while( ( e < n ) && ( ( k[ e ] & ~low ) == ( k[ j ] & ~low ) ) ) ++e;
  const std::size_t m = e - j;
  if( m == n ) {                            // no group split: the
   stable();                                // characters 0 past an end
   return;
   }
  if( m > 1 ) {
   if( 4 * m <= 3 * n )
    group( s + j , k + j , t + j , m , next , budget );
   else if( budget > 0 )
    group( s + j , k + j , t + j , m , next , budget - 1 );
   else
    std::stable_sort( s + j , s + e , [ next ]( const std::string & a ,
                                               const std::string & c ) {
     return( lessFrom( a , c , next ) ); } );
   }
  j = e;
  }
 }

}  // end( namespace detail::pks )

inline void prefixKeySort( std::vector< std::string > & A )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 if( n > 1 ) {
  const auto K = std::make_unique_for_overwrite< std::uint64_t[] >( n );
  std::allocator< std::string > alloc;
  std::string * const t = alloc.allocate( n );
  detail::pks::group( A.data() , K.get() , t , n , 0 ,
                      2 * int( std::bit_width( n ) ) );
  alloc.deallocate( t , n );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File StringSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/

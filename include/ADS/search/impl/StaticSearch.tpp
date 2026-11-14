/*--------------------------------------------------------------------------*/
/*------------------------- File StaticSearch.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The searches of StaticSearch.h, with their contracts. The searches are
 * inline, so that the loop of the caller expands them.
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

#include <cstring>

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- THE BRANCHLESS SEARCH --------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class C >
inline std::size_t branchlessLowerBound( const std::vector< T > & A ,
                                         const T & v , C less )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() , less ) );
 std::size_t n = A.size();
 if( n == 0 )
  return( 0 );
 const T * base = A.data();
 while( n > 1 ) {
  const std::size_t h = n / 2;
#if defined( __GNUC__ ) || defined( __clang__ )
  __builtin_prefetch( base + ( n - h ) / 2 );   // the two next middles
  __builtin_prefetch( base + h + ( n - h ) / 2 );
#endif
  base = less( base[ h ] , v ) ? base + h : base;
  n -= h;
  }
 const std::size_t r = std::size_t( base - A.data() ) +
                       ( less( *base , v ) ? 1 : 0 );
 ADS_POST( ( ( r == A.size() ) || ! less( A[ r ] , v ) ) &&
           ( ( r == 0 ) || less( A[ r - 1 ] , v ) ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- SPlusTree --------------------------------*/
/*--------------------------------------------------------------------------*/

#define ADS_SPT_T template< class T , class Less , std::size_t Bytes > \
 requires std::is_trivially_copyable_v< T >
#define ADS_SPT SPlusTree< T , Less , Bytes >

ADS_SPT_T
ADS_SPT::SPlusTree( const std::vector< T > & A , Less less )
 : f_less( std::move( less ) )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() , f_less ) );
 layout( A );
 }

ADS_SPT_T
ADS_SPT::SPlusTree( const SPlusTree & o )
 : f_size( o.f_size ) , f_off( o.f_off ) , f_items( o.f_items ) ,
   f_n( o.f_n ) , f_less( o.f_less )
{
 if( f_items ) {
  f_a = std::unique_ptr< T[] , Free >( allocate( f_items ) ,
                                       Free{ f_items } );
  std::memcpy( static_cast< void * >( f_a.get() ) , o.f_a.get() ,
               f_items * sizeof( T ) );
  }
 }

ADS_SPT_T
auto ADS_SPT::operator=( SPlusTree o ) noexcept
 -> SPlusTree &
{
 std::swap( f_a , o.f_a );
 std::swap( f_off , o.f_off );
 std::swap( f_size , o.f_size );
 std::swap( f_items , o.f_items );
 std::swap( f_n , o.f_n );
 std::swap( f_less , o.f_less );
 return( *this );
 }

ADS_SPT_T
T * ADS_SPT::allocate( std::size_t n )
{
 if( n * sizeof( T ) >= HUGE_BYTES )
  return( HugePageAllocator< T >().allocate( n ) );
 return( static_cast< T * >( ::operator new( n * sizeof( T ) ,
                                             std::align_val_t( 64 ) ) ) );
 }

ADS_SPT_T
void ADS_SPT::layout( const std::vector< T > & A )
{
 f_n = A.size();
 if( f_n == 0 )
  return;
 std::size_t s = ( f_n + B - 1 ) / B;      // the leaves, then up
 f_size.assign( 1 , s );
 while( s > 1 ) {
  s = ( s + B ) / ( B + 1 );
  f_size.push_back( s );
  }
 std::reverse( f_size.begin() , f_size.end() );
 const std::size_t H = f_size.size();
 f_off.assign( H + 1 , 0 );
 for( std::size_t h = 0 ; h < H ; ++h )
  f_off[ h + 1 ] = f_off[ h ] + f_size[ h ] * B;
 f_items = f_off[ H ];
 f_a = std::unique_ptr< T[] , Free >( allocate( f_items ) ,
                                      Free{ f_items } );
 T * a = f_a.get();
 const T blank = FAST ? pad() : T();
 std::uninitialized_fill( a , a + f_items , blank );
 std::copy( A.begin() , A.end() , a + f_off[ H - 1 ] );
 for( std::size_t h = 0 ; h + 1 < H ; ++h )
  for( std::size_t k = 0 ; k < f_size[ h ] ; ++k )
   for( std::size_t j = 0 ; j < B ; ++j ) {
    std::size_t c = k * ( B + 1 ) + j + 1;   // the child after the key
    if( c >= f_size[ h + 1 ] )
     continue;
    for( std::size_t l = h + 1 ; l + 1 < H ; ++l )
     c *= B + 1;                            // its leftmost leaf
    a[ f_off[ h ] + k * B + j ] = A[ c * B ];
    }
 }

ADS_SPT_T
template< bool Up >
inline std::size_t ADS_SPT::count( const T * a , const T & v ,
                                   std::size_t m ) const
{
 std::size_t r = 0;
 if constexpr( FAST ) {                     // the whole line, padded
  ( void ) m;
  r = detail::countLess< B , Up >( a , v );
  }
 else
  for( std::size_t i = 0 ; i < m ; ++i )
   if constexpr( Up )
    r += ! f_less( v , a[ i ] );
   else
    r += f_less( a[ i ] , v );
 return( r );
 }

ADS_SPT_T
template< bool Up >
inline std::size_t ADS_SPT::search( const T & v ) const
{
 const std::size_t H = f_size.size();
 const T * a = f_a.get();
 std::size_t k = 0;
 for( std::size_t h = 0 ; h + 1 < H ; ++h ) {
  std::size_t m = B;                        // the keys in use
  if constexpr( ! FAST ) {
   const std::size_t c = k * ( B + 1 ) + 1;
   m = f_size[ h + 1 ] > c ? std::min( B , f_size[ h + 1 ] - c ) : 0;
   }
  k = k * ( B + 1 ) + count< Up >( a + f_off[ h ] + k * B , v , m );
  }
 const std::size_t m = FAST ? B : std::min( B , f_n - k * B );
 return( std::min( f_n , k * B + count< Up >( a + f_off[ H - 1 ] + k * B ,
                                              v , m ) ) );
 }

ADS_SPT_T
inline std::size_t ADS_SPT::lowerBound( const T & v ) const
{
 if( f_n == 0 )
  return( 0 );
 const std::size_t r = search< false >( v );
 ADS_POST( ( ( r == f_n ) || ! f_less( ( *this )[ r ] , v ) ) &&
           ( ( r == 0 ) || f_less( ( *this )[ r - 1 ] , v ) ) );
 return( r );
 }

ADS_SPT_T
inline std::size_t ADS_SPT::upperBound( const T & v ) const
{
 if( f_n == 0 )
  return( 0 );
 std::size_t r;
 if constexpr( FAST ) {
  if( ! ( v < pad() ) )                     // the padding is not larger
   r = f_n;
  else
   r = search< true >( v );
  }
 else
  r = search< true >( v );
 ADS_POST( ( ( r == f_n ) || f_less( v , ( *this )[ r ] ) ) &&
           ( ( r == 0 ) || ! f_less( v , ( *this )[ r - 1 ] ) ) );
 return( r );
 }

ADS_SPT_T
inline bool ADS_SPT::contains( const T & v ) const
{
 const std::size_t r = lowerBound( v );
 return( ( r < f_n ) && ! f_less( v , ( *this )[ r ] ) );
 }

ADS_SPT_T
inline const T & ADS_SPT::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 return( f_a[ f_off[ f_size.size() - 1 ] + i ] );
 }

ADS_SPT_T
std::size_t ADS_SPT::size( void ) const
{
 return( f_n );
 }

ADS_SPT_T
bool ADS_SPT::empty( void ) const
{
 return( f_n == 0 );
 }

ADS_SPT_T
int ADS_SPT::height( void ) const
{
 return( int( f_size.size() ) );
 }

#undef ADS_SPT
#undef ADS_SPT_T

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File StaticSearch.tpp ------------------------*/
/*--------------------------------------------------------------------------*/

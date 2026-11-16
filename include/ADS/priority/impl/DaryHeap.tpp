/*--------------------------------------------------------------------------*/
/*--------------------------- File DaryHeap.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DaryHeap [see DaryHeap.h], with their contracts. The
 * hot ones are inline: a member of a template defined out of its class is
 * not, and the compiler would call it rather than expand it in the loop of
 * the caller, as it does with the members defined in the class.
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

namespace ADS
{

template< class T , class Better , unsigned D , bool BottomUp , class A >
DaryHeap< T , Better , D , BottomUp , A >::DaryHeap( Better better ,
						 const A & alloc )
    : f_A( alloc ) , f_better( std::move( better ) )
{ }

template< class T , class Better , unsigned D , bool BottomUp , class A >
inline void DaryHeap< T , Better , D , BottomUp , A >::push( const T & x )
{
 const std::size_t i = f_A.size();
 const std::size_t p = i > 0 ? ( i - 1 ) / D : 0;
 if( ( i == 0 ) || ! f_better( x , f_A[ p ] ) )
  f_A.push_back( x );           // x stays at the bottom
 else {                         // the parent opens the place
  T z = x;                      // x may live in f_A, which
  T y = std::move( f_A[ p ] );  // may move as it grows
  f_A.push_back( std::move( y ) );
  up( p , std::move( z ) );
  }
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
inline const T & DaryHeap< T , Better , D , BottomUp , A >::top( void ) const
{
 ADS_PRE( ! f_A.empty() );
 return( f_A.front() );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
inline T DaryHeap< T , Better , D , BottomUp , A >::pop( void )
{
 ADS_PRE( ! f_A.empty() );
 T x = std::move( f_A.front() );
 T y = std::move( f_A.back() );
 f_A.pop_back();
 if( ! f_A.empty() ) {
  if constexpr( BottomUp )
   bottomUp( std::move( y ) );
  else
   down( 0 , std::move( y ) );
  }
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
inline void DaryHeap< T , Better , D , BottomUp , A >::bottomUp( T x )
{
 const std::size_t n = f_A.size();
 std::size_t i = 0;
 for( ; ; ) {                        // the hole down to a leaf by
  const std::size_t c = D * i + 1;  // the best children, with no
  if( c >= n )                      // comparison with x
   break;
  std::size_t b = c;
  const std::size_t last = c + D < n ? c + D : n;
  for( std::size_t k = c + 1 ; k < last ; ++k )
   if( f_better( f_A[ k ] , f_A[ b ] ) )
    b = k;
  f_A[ i ] = std::move( f_A[ b ] );
  i = b;
  }
 up( i , std::move( x ) );  // and x up from there
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
T DaryHeap< T , Better , D , BottomUp , A >::erase( std::size_t i )
{
 ADS_PRE( i < f_A.size() );
 T r = std::move( f_A[ i ] );
 T x = std::move( f_A.back() );
 f_A.pop_back();
 if( i < f_A.size() ) {
  if( ( i > 0 ) && f_better( x , f_A[ parent( i ) ] ) )
   up( i , std::move( x ) );
  else
   down( i , std::move( x ) );
  }
 ADS_INVARIANT( ordered() );
 return( r );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
const T * DaryHeap< T , Better , D , BottomUp , A >::secondTop( void ) const
{
 const std::size_t n = f_A.size();
 if( n < 2 )
  return( nullptr );
 std::size_t b = 1;
 for( std::size_t k = 2 ; ( k <= D ) && ( k < n ) ; ++k )
  if( f_better( f_A[ k ] , f_A[ b ] ) )
   b = k;
 return( &f_A[ b ] );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
std::vector< T >
DaryHeap< T , Better , D , BottomUp , A >::smallest( std::size_t k ) const
{
 std::vector< T > S;
 const std::size_t n = f_A.size();
 if( ( n == 0 ) || ( k == 0 ) )
  return( S );
 S.reserve( k < n ? k : n );
 // a binary heap of places, the worst on top for the std:: heaps
 const auto worse = [ this ]( std::size_t a , std::size_t b ) {
  return( f_better( f_A[ b ] , f_A[ a ] ) );
  };
 std::vector< std::size_t > Q{ 0 };
 while( ( S.size() < k ) && ! Q.empty() ) {
  std::pop_heap( Q.begin() , Q.end() , worse );
  const std::size_t i = Q.back();
  Q.pop_back();
  S.push_back( f_A[ i ] );
  for( std::size_t c = D * i + 1 ; ( c <= D * i + D ) && ( c < n ) ; ++c ) {
   Q.push_back( c );
   std::push_heap( Q.begin() , Q.end() , worse );
   }
  }
 return( S );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
bool DaryHeap< T , Better , D , BottomUp , A >::empty( void ) const
{
 return( f_A.empty() );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
std::size_t DaryHeap< T , Better , D , BottomUp , A >::size( void ) const
{
 return( f_A.size() );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
void DaryHeap< T , Better , D , BottomUp , A >::reserve( std::size_t n )
{
 f_A.reserve( n );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
void DaryHeap< T , Better , D , BottomUp , A >::clear( void )
{
 f_A.clear();
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
template< std::input_iterator I , std::sentinel_for< I > S >
void DaryHeap< T , Better , D , BottomUp , A >::assign( I first , S last )
{
 detail::AllocVector< A , T > B( f_A.get_allocator() );
 for( ; first != last ; ++first )
  B.push_back( *first );
 f_A.swap( B );  // nothing thrown from here on
 const std::size_t n = f_A.size();
 if( n > 1 )
  for( std::size_t i = ( n - 2 ) / D + 1 ; i-- > 0 ; ) {
   T x = std::move( f_A[ i ] );
   down( i , std::move( x ) );
   }
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
inline void DaryHeap< T , Better , D , BottomUp , A >::up( std::size_t i ,
							   T x )
{
 while( i > 0 ) {
  const std::size_t p = ( i - 1 ) / D;
  if( ! f_better( x , f_A[ p ] ) )
   break;
  f_A[ i ] = std::move( f_A[ p ] );
  i = p;
  }
 f_A[ i ] = std::move( x );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
inline void DaryHeap< T , Better , D , BottomUp , A >::down( std::size_t i ,
							     T x )
{
 const std::size_t n = f_A.size();
 for( ; ; ) {
  const std::size_t c = D * i + 1;
  if( c >= n )
   break;
  std::size_t b = c;  // the best child
  const std::size_t last = c + D < n ? c + D : n;
  for( std::size_t k = c + 1 ; k < last ; ++k )
   if( f_better( f_A[ k ] , f_A[ b ] ) )
    b = k;
  if( ! f_better( f_A[ b ] , x ) )
   break;
  f_A[ i ] = std::move( f_A[ b ] );
  i = b;
  }
 f_A[ i ] = std::move( x );
 }

template< class T , class Better , unsigned D , bool BottomUp , class A >
bool DaryHeap< T , Better , D , BottomUp , A >::ordered( void ) const
{
 const std::size_t n = f_A.size();
 if( n > 1024 ) {  // the root and the last item
  for( std::size_t i = 1 ; ( i <= D ) && ( i < n ) ; ++i )
   if( f_better( f_A[ i ] , f_A[ 0 ] ) )
    return( false );
  return( ! f_better( f_A[ n - 1 ] , f_A[ ( n - 2 ) / D ] ) );
  }
 for( std::size_t i = 1 ; i < n ; ++i )
  if( f_better( f_A[ i ] , f_A[ ( i - 1 ) / D ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File DaryHeap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/

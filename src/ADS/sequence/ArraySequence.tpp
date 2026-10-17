/*--------------------------------------------------------------------------*/
/*------------------------- File ArraySequence.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ArraySequence [see ArraySequence.h], with their
 * contracts.
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
/*------------------------- CONSTRUCTING AND COPYING -----------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
ArraySequence< T >::ArraySequence( void )
 : f_A() , f_size( 0 ) , f_capacity( 0 )
{
 }

template< class T >
ArraySequence< T >::ArraySequence( std::initializer_list< T > items )
 : ArraySequence()
{
 for( const auto & v : items )
  insert( f_size , v );
 }

template< class T >
ArraySequence< T >::ArraySequence( const ArraySequence & other )
 : f_A( other.f_capacity ? new T[ other.f_capacity ] : nullptr ) ,
   f_size( other.f_size ) , f_capacity( other.f_capacity )
{
 for( Pos i = 0 ; i < f_size ; ++i )
  f_A[ i ] = other.f_A[ i ];
 }

template< class T >
ArraySequence< T >::ArraySequence( ArraySequence && other ) noexcept
 : f_A( std::move( other.f_A ) ) ,
   f_size( std::exchange( other.f_size , 0 ) ) ,
   f_capacity( std::exchange( other.f_capacity , 0 ) )
{
 }

template< class T >
ArraySequence< T > &
ArraySequence< T >::operator=( ArraySequence other ) noexcept
{
 f_A.swap( other.f_A );
 std::swap( f_size , other.f_size );
 std::swap( f_capacity , other.f_capacity );
 return( *this );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SEQUENCE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool ArraySequence< T >::isEmpty( void ) const
{
 return( f_size == 0 );
 }

template< class T >
bool ArraySequence< T >::finished( Pos p ) const
{
 return( ( p < 0 ) || ( p >= f_size ) );
 }

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::head( void ) const
{
 return( 0 );
 }

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::tail( void ) const
{
 return( f_size - 1 );
 }

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::next( Pos p ) const
{
 ADS_PRE( ( p >= -1 ) && ( p < f_size ) );
 return( p + 1 );
 }

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::prev( Pos p ) const
{
 ADS_PRE( ( p >= 0 ) && ( p <= f_size ) );
 return( p - 1 );
 }

template< class T >
typename ArraySequence< T >::Pos
ArraySequence< T >::insert( Pos p , const T & v )
{
 ADS_PRE( ( p >= 0 ) && ( p <= f_size ) );
 const Pos n = f_size;
 const auto old = detail::snapshot( *this );
 const T x = v;                 // v may be an item, which resize() moves
 if( f_size == f_capacity )     // full: twice the size
  resize( f_capacity ? 2 * f_capacity : 1 );
 for( Pos i = f_size ; i > p ; --i )
  f_A[ i ] = std::move( f_A[ i - 1 ] );
 f_A[ p ] = x;
 ++f_size;
 ADS_POST( ( f_size == n + 1 ) && detail::equal( f_A[ p ] , x ) );
 ADS_AUDIT( same( *old , 0 , 0 , p ) && same( *old , p , p + 1 , n - p ) );
 ADS_INVARIANT( fits() );
 return( p );
 }

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::remove( Pos p )
{
 ADS_PRE( ( p >= 0 ) && ( p < f_size ) );
 const Pos n = f_size;
 const auto old = detail::snapshot( *this );
 for( Pos i = p ; i < f_size - 1 ; ++i )
  f_A[ i ] = std::move( f_A[ i + 1 ] );
 --f_size;
 if( f_size <= f_capacity / 4 )   // a quarter full: half the size
  resize( f_capacity / 2 );
 ADS_POST( f_size == n - 1 );
 ADS_AUDIT( same( *old , 0 , 0 , p ) &&
            same( *old , p + 1 , p , n - p - 1 ) );
 ADS_INVARIANT( fits() );
 return( p );
 }

template< class T >
const T & ArraySequence< T >::read( Pos p ) const
{
 ADS_PRE( ( p >= 0 ) && ( p < f_size ) );
 return( f_A[ p ] );
 }

template< class T >
void ArraySequence< T >::write( Pos p , const T & v )
{
 ADS_PRE( ( p >= 0 ) && ( p < f_size ) );
 const auto old = detail::snapshot( *this );
 f_A[ p ] = v;
 ADS_POST( detail::equal( f_A[ p ] , v ) );
 ADS_AUDIT( same( *old , 0 , 0 , p ) &&
            same( *old , p + 1 , p + 1 , f_size - p - 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
typename ArraySequence< T >::Pos ArraySequence< T >::capacity( void ) const
{
 return( f_capacity );
 }

template< class T >
bool ArraySequence< T >::operator==( const ArraySequence & o ) const
{
 if( f_size != o.f_size ) return( false );
 for( Pos i = 0 ; i < f_size ; ++i )
  if( !( f_A[ i ] == o.f_A[ i ] ) ) return( false );
 return( true );
 }

template< class T >
bool ArraySequence< T >::fits( void ) const
{
 return( ( bool( f_A ) == ( f_capacity > 0 ) ) && ( f_size >= 0 ) &&
         ( f_size <= f_capacity ) );
 }

template< class T >
bool ArraySequence< T >::same( const ArraySequence & o , Pos i , Pos j ,
                               Pos k ) const
{
 for( Pos h = 0 ; h < k ; ++h )
  if( ! detail::equal( o.f_A[ i + h ] , f_A[ j + h ] ) )
   return( false );
 return( true );
 }

template< class T >
void ArraySequence< T >::resize( Pos c )
{
 std::unique_ptr< T[] > B( c ? new T[ c ] : nullptr );
 for( Pos i = 0 ; i < f_size ; ++i )
  B[ i ] = std::move( f_A[ i ] );
 f_A = std::move( B );
 f_capacity = c;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ArraySequence.tpp -----------------------*/
/*--------------------------------------------------------------------------*/

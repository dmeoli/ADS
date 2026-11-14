/*--------------------------------------------------------------------------*/
/*------------------------ File DualArrayDeque.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DualArrayDeque [see DualArrayDeque.h], with their
 * contracts.
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

template< class T , class S >
DualArrayDeque< T , S >::DualArrayDeque( std::initializer_list< T > l )
{
 for( const T & x : l ) f_B.push_back( x );
 balance();
 }

template< class T , class S >
T & DualArrayDeque< T , S >::operator[]( std::size_t i )
{
 ADS_PRE( i < size() );
 const std::size_t f = f_F.size();
 return( i < f ? f_F[ f - 1 - i ] : f_B[ i - f ] );
 }

template< class T , class S >
const T & DualArrayDeque< T , S >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < size() );
 const std::size_t f = f_F.size();
 return( i < f ? f_F[ f - 1 - i ] : f_B[ i - f ] );
 }

template< class T , class S >
void DualArrayDeque< T , S >::put( S & s , std::size_t j , T && x )
{
 if( j == s.size() )
  s.push_back( std::move( x ) );
 else if constexpr( requires { s.insert( j , x ); } )
  s.insert( j , std::move( x ) );
 else
  s.insert( s.begin() + std::ptrdiff_t( j ) , std::move( x ) );
 }

template< class T , class S >
void DualArrayDeque< T , S >::take( S & s , std::size_t j )
{
 if( j + 1 == s.size() )
  s.pop_back();
 else if constexpr( requires { s.erase( j ); } )
  s.erase( j );
 else
  s.erase( s.begin() + std::ptrdiff_t( j ) );
 }

template< class T , class S >
void DualArrayDeque< T , S >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= size() );
 [[maybe_unused]] const std::size_t n = size();
 T y = x;                                   // x may be an item
 const std::size_t f = f_F.size();
 //! [insert]
 if( i < f ) put( f_F , f - i , std::move( y ) );
 else put( f_B , i - f , std::move( y ) );
 balance();
 //! [insert]
 ADS_POST( size() == n + 1 );
 ADS_INVARIANT( balanced() );
 }

template< class T , class S >
void DualArrayDeque< T , S >::erase( std::size_t i )
{
 ADS_PRE( i < size() );
 [[maybe_unused]] const std::size_t n = size();
 const std::size_t f = f_F.size();
 //! [erase]
 if( i < f ) take( f_F , f - 1 - i );
 else take( f_B , i - f );
 balance();
 //! [erase]
 ADS_POST( size() == n - 1 );
 ADS_INVARIANT( balanced() );
 }

template< class T , class S >
void DualArrayDeque< T , S >::rotate( std::size_t r )
{
 const std::size_t n = size();
 if( n < 2 )
  return;
 r %= n;
 if( r <= n - r )
  for( std::size_t k = 0 ; k < r ; ++k ) {
   T x = std::move( ( *this )[ n - 1 ] );
   pop_back();
   insert( 0 , std::move( x ) );
   }
 else
  for( std::size_t k = 0 ; k < n - r ; ++k ) {
   T x = std::move( ( *this )[ 0 ] );
   pop_front();
   insert( n - 1 , std::move( x ) );
   }
 }

template< class T , class S >
void DualArrayDeque< T , S >::clear( void )
{
 f_F = S();
 f_B = S();
 }

template< class T , class S >
bool DualArrayDeque< T , S >::balanced( void ) const
{
 const std::size_t f = f_F.size() , b = f_B.size();
 return( ( f + b < 2 ) || ( ( 3 * f >= b ) && ( 3 * b >= f ) ) );
 }

template< class T , class S >
void DualArrayDeque< T , S >::deal( S & big , S & small , std::size_t t )
{
 //! [deal]
 const std::size_t b = small.size() , g = big.size();
 // small grows by t, its items t places up and big[ t - 1 ] , ... ,
 // big[ 0 ] below them, the places from the top down
 auto from = [ & ]( std::size_t j ) -> T & {
  return( j >= t ? small[ j - t ] : big[ t - 1 - j ] );
  };
 for( std::size_t j = b ; j < b + t ; ++j )
  small.push_back( std::move( from( j ) ) );
 for( std::size_t j = b ; j > 0 ; --j )
  small[ j - 1 ] = std::move( from( j - 1 ) );
 // big loses its t bottom items, the others t places down
 for( std::size_t k = 0 ; k + t < g ; ++k )
  big[ k ] = std::move( big[ k + t ] );
 for( std::size_t k = 0 ; k < t ; ++k )
  big.pop_back();
 //! [deal]
 }

template< class T , class S >
void DualArrayDeque< T , S >::balance( void )
{
 if( balanced() )
  return;
 //! [balance]
 const std::size_t m = size() / 2 , f = f_F.size();
 if( f > m ) deal( f_F , f_B , f - m );     // the bottom of F to B
 else deal( f_B , f_F , m - f );            // the bottom of B to F
 //! [balance]
 }

template< class T , class S >
bool DualArrayDeque< T , S >::operator==( const DualArrayDeque & o ) const
{
 if( size() != o.size() )
  return( false );
 for( std::size_t i = 0 ; i < size() ; ++i )
  if( !( ( *this )[ i ] == o[ i ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File DualArrayDeque.tpp -----------------------*/
/*--------------------------------------------------------------------------*/

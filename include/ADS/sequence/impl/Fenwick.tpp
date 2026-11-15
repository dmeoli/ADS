/*--------------------------------------------------------------------------*/
/*---------------------------- File Fenwick.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Fenwick and FenwickObserver [see `Fenwick.h`], with
 * their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Fenwick -----------------------------*/
/*--------------------------------------------------------------------------*/

template< Monoid G >
auto Fenwick< G >::prefix( std::size_t r ) const -> Value
{
 ADS_PRE( r <= f_a.size() );
 Value x = G::identity();
 for( std::size_t j = r ; j > 0 ; j -= low( j ) )
  x = G::combine( f_t[ j - 1 ] , x );
 return( x );
 }

template< Monoid G >
auto Fenwick< G >::range( std::size_t l , std::size_t r ) const -> Value
 requires Group< G >
{
 ADS_PRE( ( l <= r ) && ( r <= f_a.size() ) );
 return( G::combine( G::inverse( prefix( l ) ) , prefix( r ) ) );
 }

template< Monoid G >
void Fenwick< G >::set( std::size_t i , const Value & v )
{
 ADS_PRE( i < f_a.size() );
 const std::size_t n = f_a.size();
 if constexpr( AbelianGroup< G > ) {
  const Value d = G::combine( G::inverse( f_a[ i ] ) , v );
  f_a[ i ] = v;
  for( std::size_t j = i + 1 ; j <= n ; j += low( j ) )
   f_t[ j - 1 ] = G::combine( f_t[ j - 1 ] , d );
  counted();
  }
 else {
  f_a[ i ] = v;
  for( std::size_t j = i + 1 ; j <= n ; j += low( j ) )
   f_t[ j - 1 ] = node( j );
  }
 }

template< Monoid G >
void Fenwick< G >::add( std::size_t i , const Value & d )
 requires CommutativeMonoid< G >
{
 ADS_PRE( i < f_a.size() );
 const std::size_t n = f_a.size();
 f_a[ i ] = G::combine( f_a[ i ] , d );
 for( std::size_t j = i + 1 ; j <= n ; j += low( j ) )
  f_t[ j - 1 ] = G::combine( f_t[ j - 1 ] , d );
 if constexpr( Group< G > )
  counted();
 }

template< Monoid G >
void Fenwick< G >::push_back( const Value & v )
{
 f_a.push_back( v );
 f_t.push_back( G::identity() );
 f_t.back() = node( f_a.size() );
 }

template< Monoid G >
template< std::input_iterator I >
void Fenwick< G >::assign( I first , I last )
{
 f_a.assign( first , last );
 build();
 }

template< Monoid G >
auto Fenwick< G >::node( std::size_t j ) const -> Value
{
 Value x = G::identity();
 for( std::size_t k = low( j ) / 2 ; k >= 1 ; k /= 2 )
  x = G::combine( x , f_t[ j - k - 1 ] );
 return( G::combine( x , f_a[ j - 1 ] ) );
 }

template< Monoid G >
void Fenwick< G >::build( void )
{
 f_t.assign( f_a.size() , G::identity() );
 for( std::size_t j = 1 ; j <= f_a.size() ; ++j )
  f_t[ j - 1 ] = node( j );
 f_since = 0;
 }

template< Monoid G >
void Fenwick< G >::counted( void )
{
 if constexpr( ! exact_v< G > )
  if( ++f_since >= f_period )
   build();  // the drift dropped
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FenwickObserver -------------------------*/
/*--------------------------------------------------------------------------*/

template< Monoid G >
template< class A >
bool FenwickObserver< G >::audit( const A & a ) const
{
 flush();
 if( f_f.size() != a.size() )
  return( false );
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  if( ! ( f_f.get( i ) == a[ i ] ) )
   return( false );
 if constexpr( exact_v< G > ) {
  Value x = G::identity();
  for( std::size_t r = 0 ; r <= a.size() ; ++r ) {
   if( ! ( f_f.prefix( r ) == x ) )
    return( false );
   if( r < a.size() )
    x = G::combine( x , a[ r ] );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Fenwick.tpp --------------------------*/
/*--------------------------------------------------------------------------*/

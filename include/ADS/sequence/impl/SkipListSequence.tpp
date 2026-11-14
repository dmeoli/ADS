/*--------------------------------------------------------------------------*/
/*------------------------ File SkipListSequence.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SkipListSequence [see `SkipListSequence.h`], with their
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

template< class T >
SkipListSequence< T >::SkipListSequence( std::uint64_t seed ) : f_g( seed )
{
 clearHead();
 }

template< class T >
void SkipListSequence< T >::clearHead( void )
{
 for( Link & h : f_head ) h = Link{ nullptr , f_size + 1 };
 }

template< class T >
SkipListSequence< T >::SkipListSequence( SkipListSequence && other ) noexcept
 : f_levels( other.f_levels ) , f_size( other.f_size ) , f_g( other.f_g )
{
 std::copy( other.f_head , other.f_head + kMax , f_head );
 other.f_levels = 0;
 other.f_size = 0;
 other.clearHead();
 }

template< class T >
SkipListSequence< T >::~SkipListSequence()
{
 for( Node * x = f_head[ 0 ].next ; x ; ) {
  Node * n = tower( x )[ 0 ].next;
  dispose( x );
  x = n;
  }
 }

template< class T >
auto SkipListSequence< T >::make( const T & v , int L ) -> Node *
{
 const std::size_t bytes = kTower + std::size_t( L ) * sizeof( Link );
 void * p;
 if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  p = ::operator new( bytes , std::align_val_t( alignof( Node ) ) );
 else
  p = ::operator new( bytes );
 Node * x;
 try {
  x = ::new( p ) Node{ v , L };
  }
 catch( ... ) {
  if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
   ::operator delete( p , std::align_val_t( alignof( Node ) ) );
  else
   ::operator delete( p );
  throw;
  }
 std::uninitialized_fill_n( reinterpret_cast< Link * >(
                             static_cast< std::byte * >( p ) + kTower ) ,
                            L , Link{ nullptr , 0 } );
 return( x );
 }

template< class T >
void SkipListSequence< T >::dispose( Node * x ) noexcept
{
 x->~Node();
 if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  ::operator delete( static_cast< void * >( x ) ,
                     std::align_val_t( alignof( Node ) ) );
 else
  ::operator delete( static_cast< void * >( x ) );
 }

template< class T >
auto SkipListSequence< T >::nodeAt( std::size_t i ) const -> Node *
{
 // the indices are shifted by one, the head at 0 and nil at n + 1, so
 // that they stay unsigned
 Node * x = nullptr;
 std::size_t j = 0;
 for( int l = f_levels - 1 ; l >= 0 ; --l )
  for( const Link * t = towerOf( x ) ; j + t[ l ].w <= i + 1 ;
       t = towerOf( x ) ) {
   j += t[ l ].w;
   x = t[ l ].next;
   }
 return( x );
 }

template< class T >
const T & SkipListSequence< T >::get( std::size_t i ) const
{
 ADS_PRE( i < f_size );
 return( nodeAt( i )->item );
 }

template< class T >
void SkipListSequence< T >::set( std::size_t i , const T & v )
{
 ADS_PRE( i < f_size );
 nodeAt( i )->item = v;
 }

template< class T >
void SkipListSequence< T >::add( std::size_t i , const T & v )
{
 ADS_PRE( i <= f_size );
 const int L = std::min( kMax , 1 + std::countr_one( f_g() ) );
 Node * y = make( v , L );
 Link * ty = tower( y );
 for( ; f_levels < L ; ++f_levels )        // new levels: the head to nil
  f_head[ f_levels ] = Link{ nullptr , f_size + 1 };
 Node * x = nullptr;
 std::size_t j = 0;                         // the index of x, plus one
 for( int l = f_levels - 1 ; l >= 0 ; --l ) {
  Link * t = towerOf( x );
  while( j + t[ l ].w < i + 1 ) {
   j += t[ l ].w;
   x = t[ l ].next;
   t = towerOf( x );
   }
  if( l >= L )
   ++t[ l ].w;                              // it passes over y
  else {
   ty[ l ] = Link{ t[ l ].next , j + t[ l ].w - i };
   t[ l ] = Link{ y , i + 1 - j };
   }
  }
 for( int l = f_levels ; l < kMax ; ++l )   // the empty levels too
  ++f_head[ l ].w;
 ++f_size;
 ADS_INVARIANT( valid() );
 }

template< class T >
T SkipListSequence< T >::remove( std::size_t i )
{
 ADS_PRE( i < f_size );
 Node * x = nullptr , * y = nullptr;
 std::size_t j = 0;
 for( int l = f_levels - 1 ; l >= 0 ; --l ) {
  Link * t = towerOf( x );
  while( j + t[ l ].w < i + 1 ) {
   j += t[ l ].w;
   x = t[ l ].next;
   t = towerOf( x );
   }
  if( ( j + t[ l ].w == i + 1 ) && t[ l ].next ) {
   y = t[ l ].next;                         // unlinked on this level
   t[ l ] = Link{ tower( y )[ l ].next , t[ l ].w + tower( y )[ l ].w - 1 };
   }
  else
   --t[ l ].w;
  }
 for( int l = f_levels ; l < kMax ; ++l )
  --f_head[ l ].w;
 T v = std::move( y->item );
 dispose( y );
 --f_size;
 while( ( f_levels > 0 ) && ! f_head[ f_levels - 1 ].next ) --f_levels;
 ADS_INVARIANT( valid() );
 return( v );
 }

template< class T >
auto SkipListSequence< T >::truncate( std::size_t i ) -> SkipListSequence
{
 ADS_PRE( i <= f_size );
 SkipListSequence r( f_g() );
 Node * x = nullptr;
 std::size_t j = 0;
 for( int l = f_levels - 1 ; l >= 0 ; --l ) {
  Link * t = towerOf( x );
  while( j + t[ l ].w < i + 1 ) {
   j += t[ l ].w;
   x = t[ l ].next;
   t = towerOf( x );
   }
  r.f_head[ l ] = Link{ t[ l ].next , j + t[ l ].w - i };
  t[ l ] = Link{ nullptr , i + 1 - j };
  }
 r.f_size = f_size - i;
 for( int l = f_levels ; l < kMax ; ++l ) {
  r.f_head[ l ].w = r.f_size + 1;
  f_head[ l ].w = i + 1;
  }
 r.f_levels = f_levels;
 f_size = i;
 while( ( f_levels > 0 ) && ! f_head[ f_levels - 1 ].next ) --f_levels;
 while( ( r.f_levels > 0 ) && ! r.f_head[ r.f_levels - 1 ].next )
  --r.f_levels;
 ADS_INVARIANT( valid() && r.valid() );
 return( r );
 }

template< class T >
void SkipListSequence< T >::absorb( SkipListSequence & s )
{
 if( ( &s == this ) || s.empty() )
  return;
 const int L = std::max( f_levels , s.f_levels );
 Node * x = nullptr;
 std::size_t j = 0;
 for( int l = L - 1 ; l >= 0 ; --l ) {
  Link * t = towerOf( x );
  while( t[ l ].next ) {                    // to the end of the level
   j += t[ l ].w;
   x = t[ l ].next;
   t = towerOf( x );
   }
  t[ l ] = Link{ s.f_head[ l ].next , f_size + s.f_head[ l ].w - j };
  }
 f_size += s.f_size;
 for( int l = L ; l < kMax ; ++l )
  f_head[ l ].w = f_size + 1;
 f_levels = L;
 s.f_levels = 0;
 s.f_size = 0;
 s.clearHead();
 ADS_INVARIANT( valid() );
 }

template< class T >
bool SkipListSequence< T >::valid( void ) const
{
 for( int l = 0 ; l < kMax ; ++l ) {
  if( ( l >= f_levels ) && f_head[ l ].next ) return( false );
  std::size_t sum = 0;
  for( Node * x = nullptr ; ; ) {           // the widths sum to n + 1
   const Link * t = towerOf( x );
   sum += t[ l ].w;
   if( ! t[ l ].next ) break;
   x = t[ l ].next;
   if( x->height <= l ) return( false );
   }
  if( sum != f_size + 1 ) return( false );
  }
 std::size_t n = 0;
 for( Node * x = f_head[ 0 ].next ; x ; x = tower( x )[ 0 ].next ) ++n;
 return( n == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SkipListSequence.tpp ---------------------*/
/*--------------------------------------------------------------------------*/

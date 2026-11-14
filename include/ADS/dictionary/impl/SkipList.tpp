/*--------------------------------------------------------------------------*/
/*--------------------------- File SkipList.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SkipList [see SkipList.h], with their contracts.
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

template< std::totally_ordered K , class V >
SkipList< K , V >::SkipList( std::uint64_t seed ) : f_g( seed )
{
 }

template< std::totally_ordered K , class V >
SkipList< K , V >::SkipList( SkipList && other ) noexcept
 : f_levels( other.f_levels ) , f_size( other.f_size ) , f_g( other.f_g )
{
 std::copy( other.f_head , other.f_head + kMax , f_head );
 std::fill( other.f_head , other.f_head + kMax , nullptr );
 std::fill( other.f_finger , other.f_finger + kMax , nullptr );
 other.f_levels = 0;
 other.f_size = 0;
 }

template< std::totally_ordered K , class V >
SkipList< K , V >::~SkipList()
{
 for( Node * x = f_head[ 0 ] ; x ; ) {
  Node * n = tower( x )[ 0 ];
  dispose( x );
  x = n;
  }
 }

template< std::totally_ordered K , class V >
auto SkipList< K , V >::make( const K & k , const V & v , int L ) -> Node *
{
 const std::size_t bytes = kTower + std::size_t( L ) * sizeof( Node * );
 void * p;
 if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  p = ::operator new( bytes , std::align_val_t( alignof( Node ) ) );
 else
  p = ::operator new( bytes );
 Node * x;
 try {
  x = ::new( p ) Node{ k , v , L };
  }
 catch( ... ) {
  if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
   ::operator delete( p , std::align_val_t( alignof( Node ) ) );
  else
   ::operator delete( p );
  throw;
  }
 std::uninitialized_fill_n( reinterpret_cast< Node ** >(
                             static_cast< std::byte * >( p ) + kTower ) ,
                            L , nullptr );
 return( x );
 }

template< std::totally_ordered K , class V >
void SkipList< K , V >::dispose( Node * x ) noexcept
{
 x->~Node();
 if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  ::operator delete( static_cast< void * >( x ) ,
                     std::align_val_t( alignof( Node ) ) );
 else
  ::operator delete( static_cast< void * >( x ) );
 }

template< std::totally_ordered K , class V >
auto SkipList< K , V >::before( const K & k , Node * update[ kMax ] ) const
 -> Node *
{
//! [find]
 Node * x = nullptr;                        // nil is the head
 Node ** t = const_cast< Node ** >( f_head );
 const Node * same = nullptr;               // known not smaller than k
 for( int l = f_levels - 1 ; l >= 0 ; --l ) {
  for( Node * n ; ( ( n = t[ l ] ) != same ) && n && ( n->key < k ) ; )
   t = tower( x = n );
  same = t[ l ];                            // compared, not smaller than k
  update[ l ] = x;
  }
 Node * n = t[ 0 ];
 return( n && !( k < n->key ) ? n : nullptr );   // the node of k, if any
//! [find]
 }

template< std::totally_ordered K , class V >
std::optional< V > SkipList< K , V >::lookup( const K & k ) const
{
 Node * update[ kMax ];
 const Node * n = before( k , update );
 return( n ? std::optional< V >( n->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
auto SkipList< K , V >::lowerBound( const K & k ) const -> const_iterator
{
 Node * update[ kMax ];
 if( Node * n = before( k , update ) )
  return( const_iterator( n ) );
 return( const_iterator( f_levels ? towerOf( update[ 0 ] )[ 0 ]
                                  : nullptr ) );
 }

template< std::totally_ordered K , class V >
std::optional< V > SkipList< K , V >::fingerLookup( const K & k )
{
 if( f_levels == 0 )
  return( std::nullopt );
//! [finger]
 int l = 0;
 for( ; l < f_levels - 1 ; ++l ) {          // up to a node before k whose
  Node * x = f_finger[ l ];                 // successor is not
  if( x && !( x->key < k ) ) continue;
  Node * n = towerOf( x )[ l ];
  if( ! n || !( n->key < k ) ) break;
  }
 Node * x = f_finger[ l ];
 if( x && !( x->key < k ) )                 // past k on the top level
  x = nullptr;
 for( int j = l ; j >= 0 ; --j ) {
  for( Node * n ; ( n = towerOf( x )[ j ] ) && ( n->key < k ) ; ) x = n;
  f_finger[ j ] = x;
  }
//! [finger]
 Node * n = towerOf( x )[ 0 ];
 return( n && !( k < n->key ) ? std::optional< V >( n->value )
                              : std::nullopt );
 }

template< std::totally_ordered K , class V >
void SkipList< K , V >::insert( const K & k , const V & v )
{
 Node * update[ kMax ];
 if( Node * n = before( k , update ) )
  n->value = v;                             // the new value replaces it
 else {
//! [add]
  const int L = std::min( kMax , 1 + std::countr_one( f_g() ) );
  for( int l = f_levels ; l < L ; ++l )     // new levels start at the head
   update[ l ] = nullptr;
  Node * x = make( k , v , L );
  f_levels = std::max( f_levels , L );
  Node ** t = tower( x );
  for( int l = 0 ; l < L ; ++l ) {
   Node * & link = towerOf( update[ l ] )[ l ];
   t[ l ] = link;
   link = x;
   }
//! [add]
  ++f_size;
  std::fill( f_finger , f_finger + kMax , nullptr );
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered K , class V >
void SkipList< K , V >::remove( const K & k )
{
 Node * update[ kMax ];
 if( Node * n = before( k , update ) ) {
//! [remove]
  Node ** t = tower( n );
  for( int l = 0 ; l < n->height ; ++l )
   towerOf( update[ l ] )[ l ] = t[ l ];
  dispose( n );
  --f_size;
  while( ( f_levels > 0 ) && ! f_head[ f_levels - 1 ] ) --f_levels;
//! [remove]
  std::fill( f_finger , f_finger + kMax , nullptr );
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered K , class V >
bool SkipList< K , V >::contains( const K & k ) const
{
 Node * update[ kMax ];
 return( before( k , update ) != nullptr );
 }

template< std::totally_ordered K , class V >
std::size_t SkipList< K , V >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V >
int SkipList< K , V >::levels( void ) const
{
 return( f_levels );
 }

template< std::totally_ordered K , class V >
template< class F >
void SkipList< K , V >::forEach( F && f ) const
{
 for( Node * x = f_head[ 0 ] ; x ; x = tower( x )[ 0 ] )
  f( std::as_const( x->key ) , std::as_const( x->value ) );
 }

template< std::totally_ordered K , class V >
bool SkipList< K , V >::sorted( void ) const
{
 std::size_t n = 0;
 for( Node * x = f_head[ 0 ] ; x ; x = tower( x )[ 0 ] , ++n )
  if( tower( x )[ 0 ] && !( x->key < tower( x )[ 0 ]->key ) )
   return( false );
 if( n != f_size )
  return( false );
 for( int l = f_levels ; l < kMax ; ++l )   // no node above the levels
  if( f_head[ l ] ) return( false );
 for( int l = 1 ; l < f_levels ; ++l ) {    // a sublist of the level below
  Node * y = f_head[ l - 1 ];
  for( Node * x = f_head[ l ] ; x ; x = tower( x )[ l ] ) {
   if( x->height <= l ) return( false );
   while( y && ( y != x ) ) y = tower( y )[ l - 1 ];
   if( ! y ) return( false );
   }
  }
 return( true );
 }

template< std::totally_ordered K , class V >
const K * SkipList< K , V >::findKey( const K & k ) const
{
 Node * update[ kMax ];
 const Node * n = before( k , update );
 return( n ? &n->key : nullptr );
 }

template< std::totally_ordered K , class V >
const K * SkipList< K , V >::lowerBoundKey( const K & k ) const
{
 const const_iterator it = lowerBound( k );
 return( it == end() ? nullptr : &( *it ).first );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File SkipList.tpp --------------------------*/
/*--------------------------------------------------------------------------*/

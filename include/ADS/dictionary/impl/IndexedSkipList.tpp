/*--------------------------------------------------------------------------*/
/*------------------------ File IndexedSkipList.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of IndexedSkipList [see `IndexedSkipList.h`], with their
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

template< std::totally_ordered K , class V >
IndexedSkipList< K , V >::IndexedSkipList( std::uint64_t seed ) : f_g( seed )
{
 for( Link & h : f_head ) h = Link{ nullptr , 1 };
 }

template< std::totally_ordered K , class V >
IndexedSkipList< K , V >::IndexedSkipList( IndexedSkipList && other ) noexcept
 : f_levels( other.f_levels ) , f_size( other.f_size ) , f_g( other.f_g )
{
 std::copy( other.f_head , other.f_head + kMax , f_head );
 for( Link & h : other.f_head ) h = Link{ nullptr , 1 };
 other.f_levels = 0;
 other.f_size = 0;
 }

template< std::totally_ordered K , class V >
IndexedSkipList< K , V >::~IndexedSkipList()
{
 for( Node * x = f_head[ 0 ].next ; x ; ) {
  Node * n = tower( x )[ 0 ].next;
  dispose( x );
  x = n;
  }
 }

template< std::totally_ordered K , class V >
auto IndexedSkipList< K , V >::make( const K & k , const V & v , int L )
 -> Node *
{
 const std::size_t bytes = kTower + std::size_t( L ) * sizeof( Link );
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
 std::uninitialized_fill_n( reinterpret_cast< Link * >(
                             static_cast< std::byte * >( p ) + kTower ) ,
                            L , Link{ nullptr , 0 } );
 return( x );
 }

template< std::totally_ordered K , class V >
void IndexedSkipList< K , V >::dispose( Node * x ) noexcept
{
 x->~Node();
 if constexpr( alignof( Node ) > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
  ::operator delete( static_cast< void * >( x ) ,
                     std::align_val_t( alignof( Node ) ) );
 else
  ::operator delete( static_cast< void * >( x ) );
 }

template< std::totally_ordered K , class V >
auto IndexedSkipList< K , V >::before( const K & k , Node * update[ kMax ] ,
                                       std::size_t rank[ kMax ] ) const
 -> Node *
{
 // the ranks are shifted by one, the head at 0 and nil at n + 1
 Node * x = nullptr;
 std::size_t j = 0;
 const Node * same = nullptr;               // known not smaller than k
 for( int l = f_levels - 1 ; l >= 0 ; --l ) {
  const Link * t = towerOf( x );
  for( Node * n ; ( ( n = t[ l ].next ) != same ) && n && ( n->key < k ) ;
       t = towerOf( x ) ) {
   j += t[ l ].w;
   x = n;
   }
  same = t[ l ].next;
  update[ l ] = x;
  rank[ l ] = j;
  }
 Node * n = towerOf( x )[ 0 ].next;
 return( n && !( k < n->key ) ? n : nullptr );
 }

template< std::totally_ordered K , class V >
std::optional< V > IndexedSkipList< K , V >::lookup( const K & k ) const
{
 Node * update[ kMax ];
 std::size_t rank[ kMax ];
 const Node * n = before( k , update , rank );
 return( n ? std::optional< V >( n->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
bool IndexedSkipList< K , V >::contains( const K & k ) const
{
 Node * update[ kMax ];
 std::size_t rank[ kMax ];
 return( before( k , update , rank ) != nullptr );
 }

template< std::totally_ordered K , class V >
std::size_t IndexedSkipList< K , V >::rank( const K & k ) const
{
 Node * update[ kMax ];
 std::size_t r[ kMax ];
 ( void ) before( k , update , r );
 return( f_levels ? r[ 0 ] : 0 );
 }

template< std::totally_ordered K , class V >
auto IndexedSkipList< K , V >::lowerBound( const K & k ) const
 -> const_iterator
{
 Node * update[ kMax ];
 std::size_t rank[ kMax ];
 if( Node * n = before( k , update , rank ) )
  return( const_iterator( n ) );
 return( const_iterator( f_levels ? towerOf( update[ 0 ] )[ 0 ].next
                                  : nullptr ) );
 }

template< std::totally_ordered K , class V >
auto IndexedSkipList< K , V >::select( std::size_t i ) const -> const_iterator
{
 ADS_PRE( i < f_size );
 Node * x = nullptr;
 std::size_t j = 0;
 for( int l = f_levels - 1 ; l >= 0 ; --l )
  for( const Link * t = towerOf( x ) ; j + t[ l ].w <= i + 1 ;
       t = towerOf( x ) ) {
   j += t[ l ].w;
   x = t[ l ].next;
   }
 return( const_iterator( x ) );
 }

template< std::totally_ordered K , class V >
void IndexedSkipList< K , V >::insert( const K & k , const V & v )
{
 Node * update[ kMax ];
 std::size_t rank[ kMax ];
 if( Node * n = before( k , update , rank ) )
  n->value = v;
 else {
  const int L = std::min( kMax , 1 + std::countr_one( f_g() ) );
  Node * y = make( k , v , L );
  Link * ty = tower( y );
  for( ; f_levels < L ; ++f_levels ) {      // new levels: from the head
   update[ f_levels ] = nullptr;
   rank[ f_levels ] = 0;
   }
  const std::size_t i = rank[ 0 ] + 1;      // the rank of y, shifted
  for( int l = 0 ; l < f_levels ; ++l ) {
   Link * t = towerOf( update[ l ] );
   if( l >= L )
    ++t[ l ].w;
   else {
    ty[ l ] = Link{ t[ l ].next , rank[ l ] + t[ l ].w + 1 - i };
    t[ l ] = Link{ y , i - rank[ l ] };
    }
   }
  for( int l = f_levels ; l < kMax ; ++l ) ++f_head[ l ].w;
  ++f_size;
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
void IndexedSkipList< K , V >::remove( const K & k )
{
 Node * update[ kMax ];
 std::size_t rank[ kMax ];
 if( Node * y = before( k , update , rank ) ) {
  for( int l = 0 ; l < f_levels ; ++l ) {
   Link * t = towerOf( update[ l ] );
   if( t[ l ].next == y )
    t[ l ] = Link{ tower( y )[ l ].next , t[ l ].w + tower( y )[ l ].w - 1 };
   else
    --t[ l ].w;
   }
  for( int l = f_levels ; l < kMax ; ++l ) --f_head[ l ].w;
  dispose( y );
  --f_size;
  while( ( f_levels > 0 ) && ! f_head[ f_levels - 1 ].next ) --f_levels;
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
template< class F >
void IndexedSkipList< K , V >::forEach( F && f ) const
{
 for( Node * x = f_head[ 0 ].next ; x ; x = tower( x )[ 0 ].next )
  f( std::as_const( x->key ) , std::as_const( x->value ) );
 }

template< std::totally_ordered K , class V >
bool IndexedSkipList< K , V >::valid( void ) const
{
 for( int l = 0 ; l < kMax ; ++l ) {
  if( ( l >= f_levels ) && f_head[ l ].next ) return( false );
  std::size_t sum = 0;
  for( Node * x = nullptr ; ; ) {           // the widths sum to n + 1
   const Link * t = towerOf( x );
   sum += t[ l ].w;
   if( ! t[ l ].next ) break;
   if( x && !( x->key < t[ l ].next->key ) ) return( false );
   x = t[ l ].next;
   if( x->height <= l ) return( false );
   }
  if( sum != f_size + 1 ) return( false );
  }
 return( true );
 }

template< std::totally_ordered K , class V >
const K * IndexedSkipList< K , V >::findKey( const K & k ) const
{
 Node * update[ kMax ];
 std::size_t rank[ kMax ];
 const Node * n = before( k , update , rank );
 return( n ? &n->key : nullptr );
 }

template< std::totally_ordered K , class V >
const K * IndexedSkipList< K , V >::lowerBoundKey( const K & k ) const
{
 const const_iterator it = lowerBound( k );
 return( it == end() ? nullptr : &( *it ).first );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File IndexedSkipList.tpp ----------------------*/
/*--------------------------------------------------------------------------*/

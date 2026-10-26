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
SkipList< K , V >::SkipList( std::uint64_t seed )
 : f_head( kMax , nullptr ) , f_g( seed )
{
 }

template< std::totally_ordered K , class V >
SkipList< K , V >::SkipList( SkipList && other ) noexcept
 : f_head( kMax , nullptr ) , f_levels( other.f_levels ) ,
   f_size( other.f_size ) , f_g( other.f_g )
{
 f_head.swap( other.f_head );               // other keeps empty levels
 other.f_levels = 0;
 other.f_size = 0;
 }

template< std::totally_ordered K , class V >
SkipList< K , V >::~SkipList()
{
 for( Node * x = f_head[ 0 ] ; x ; ) {
  Node * n = x->next[ 0 ];
  delete x;
  x = n;
  }
 }

template< std::totally_ordered K , class V >
auto SkipList< K , V >::before( const K & k , Node * update[ kMax ] ) const
 -> Node *
{
 Node * x = nullptr;                        // nil is the head
 for( int l = f_levels - 1 ; l >= 0 ; --l ) {
  for( ; ; ) {
   Node * n = x ? x->next[ l ] : f_head[ l ];
   if( ! n || !( n->key < k ) ) break;
   x = n;
   }
  update[ l ] = x;
  }
 Node * n = x ? x->next[ 0 ] : f_head[ 0 ];
 return( n && !( k < n->key ) ? n : nullptr );   // the node of k, if any
 }

template< std::totally_ordered K , class V >
std::optional< V > SkipList< K , V >::lookup( const K & k ) const
{
 Node * update[ kMax ];
 const Node * n = before( k , update );
 return( n ? std::optional< V >( n->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
void SkipList< K , V >::insert( const K & k , const V & v )
{
 Node * update[ kMax ];
 if( Node * n = before( k , update ) )
  n->value = v;                             // the new value replaces it
 else {
  const int L = std::min( kMax , 1 + std::countr_one( f_g() ) );
  for( int l = f_levels ; l < L ; ++l )     // new levels start at the head
   update[ l ] = nullptr;
  f_levels = std::max( f_levels , L );
  Node * x = new Node{ k , v , std::vector< Node * >( L , nullptr ) };
  for( int l = 0 ; l < L ; ++l ) {
   Node * & link = update[ l ] ? update[ l ]->next[ l ] : f_head[ l ];
   x->next[ l ] = link;
   link = x;
   }
  ++f_size;
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered K , class V >
void SkipList< K , V >::remove( const K & k )
{
 Node * update[ kMax ];
 if( Node * n = before( k , update ) ) {
  for( int l = 0 ; l < int( n->next.size() ) ; ++l ) {
   Node * & link = update[ l ] ? update[ l ]->next[ l ] : f_head[ l ];
   link = n->next[ l ];
   }
  delete n;
  --f_size;
  while( ( f_levels > 0 ) && ! f_head[ f_levels - 1 ] ) --f_levels;
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered K , class V >
bool SkipList< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
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
 for( const Node * x = f_head[ 0 ] ; x ; x = x->next[ 0 ] )
  f( x->key , x->value );
 }

template< std::totally_ordered K , class V >
bool SkipList< K , V >::sorted( void ) const
{
 std::size_t n = 0;
 for( const Node * x = f_head[ 0 ] ; x ; x = x->next[ 0 ] , ++n )
  if( x->next[ 0 ] && !( x->key < x->next[ 0 ]->key ) )
   return( false );
 if( n != f_size )
  return( false );
 for( int l = 1 ; l < f_levels ; ++l ) {    // a sublist of the level below
  const Node * y = f_head[ l - 1 ];
  for( const Node * x = f_head[ l ] ; x ; x = x->next[ l ] ) {
   while( y && ( y != x ) ) y = y->next[ l - 1 ];
   if( ! y ) return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File SkipList.tpp --------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- File BitmapTrie.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BitmapTrie [see BitmapTrie.h], with their contracts.
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

template< class V >
BitmapTrie< V >::BitmapTrie( void )
 : f_root( new Node ) , f_dummy( new Cell )
{
 f_dummy->prev = f_dummy->next = f_dummy;
 }

template< class V >
BitmapTrie< V >::BitmapTrie( BitmapTrie && o ) : BitmapTrie()
{
 *this = std::move( o );
 }

template< class V >
BitmapTrie< V > & BitmapTrie< V >::operator=( BitmapTrie && o ) noexcept
{
 std::swap( f_root , o.f_root );
 std::swap( f_dummy , o.f_dummy );
 std::swap( f_n , o.f_n );
 std::swap( f_nodes , o.f_nodes );
 return( *this );
 }

template< class V >
BitmapTrie< V >::~BitmapTrie()
{
 destroy( f_root );
 for( Cell * l = f_dummy->next ; l != f_dummy ; ) {
  Cell * n = l->next;
  delete l;
  l = n;
  }
 delete f_dummy;
 }

template< class V >
void BitmapTrie< V >::destroy( Node * u )
{
 std::vector< Node * > S{ u };   // a stack: the strings may be long
 while( ! S.empty() ) {
  Node * x = S.back();
  S.pop_back();
  for( Node * c : x->kids )
   S.push_back( c );
  delete x;
  }
 }

template< class V >
bool BitmapTrie< V >::has( const Node * u , unsigned c )
{
 return( ( u->bits[ c >> 6 ] >> ( c & 63 ) ) & 1u );
 }

template< class V >
std::size_t BitmapTrie< V >::rank( const Node * u , unsigned c )
{
 std::size_t r = 0;
 for( unsigned i = 0 ; i < ( c >> 6 ) ; ++i )
  r += std::size_t( std::popcount( u->bits[ i ] ) );
 const std::uint64_t below = ( std::uint64_t( 1 ) << ( c & 63 ) ) - 1;
 return( r + std::size_t( std::popcount( u->bits[ c >> 6 ] & below ) ) );
 }

template< class V >
auto BitmapTrie< V >::child( const Node * u , unsigned c ) -> Node *
{
 return( has( u , c ) ? u->kids[ rank( u , c ) ] : nullptr );
 }

template< class V >
auto BitmapTrie< V >::after( const Node * u , unsigned c ) -> Node *
{
 // the children are sorted by byte: the first one after c follows those
 // up to c
 const std::size_t r = rank( u , c ) + ( has( u , c ) ? 1 : 0 );
 return( r < u->kids.size() ? u->kids[ r ] : nullptr );
 }

template< class V >
bool BitmapTrie< V >::before( const Node * u , unsigned c )
{
 return( rank( u , c ) > 0 );
 }

template< class V >
auto BitmapTrie< V >::node( std::string_view s ) const -> Node *
{
 Node * u = f_root;
 for( const char ch : s )
  if( ! ( u = child( u , static_cast< unsigned char >( ch ) ) ) )
   return( nullptr );
 return( u );
 }

template< class V >
auto BitmapTrie< V >::successor( std::string_view s ) const -> Cell *
{
//! [BitmapTrie-successor]
 const Node * u = f_root;
 for( const char ch : s ) {
  const unsigned c = static_cast< unsigned char >( ch );
  if( Node * v = child( u , c ) ) {
   u = v;
   continue;
   }
  if( const Node * a = after( u , c ) )
   return( a->min );
  return( u->max ? u->max->next : f_dummy );
  }
 return( u->min ? u->min : f_dummy );
//! [BitmapTrie-successor]
 }

template< class V >
const std::string * BitmapTrie< V >::lowerBound( std::string_view s ) const
{
 const Cell * l = successor( s );
 return( l == f_dummy ? nullptr : &l->key );
 }

template< class V >
const std::string * BitmapTrie< V >::find( std::string_view s ) const
{
 const Node * u = node( s );
 return( u && u->term ? &u->term->key : nullptr );
 }

template< class V >
auto BitmapTrie< V >::seek( std::string_view s ) const
 -> const_iterator
{
 return( const_iterator( successor( s ) ) );
 }

template< class V >
std::optional< V > BitmapTrie< V >::lookup( std::string_view s ) const
{
 const Node * u = node( s );
 if( ! u || ! u->term )
  return( std::nullopt );
 return( u->term->value );
 }

template< class V >
bool BitmapTrie< V >::contains( std::string_view s ) const
{
 const Node * u = node( s );
 return( u && u->term );
 }

template< class V >
bool BitmapTrie< V >::insert( std::string_view s , const V & v )
{
 if( Node * e = node( s ) ; e && e->term ) {
  e->term->value = v;
  return( false );
  }
//! [BitmapTrie-insert]
 Cell * succ = successor( s );
 Cell * it = new Cell{ std::string( s ) , v , succ->prev , succ };
 succ->prev->next = it;
 succ->prev = it;

 std::vector< Node * > P( s.size() + 1 );   // the path of s
 P[ 0 ] = f_root;
 for( std::size_t i = 0 ; i < s.size() ; ++i ) {
  const unsigned c = static_cast< unsigned char >( s[ i ] );
  Node * u = P[ i ];
  Node * v = child( u , c );
  if( ! v ) {
   v = new Node;
   u->kids.insert( u->kids.begin() + std::ptrdiff_t( rank( u , c ) ) , v );
   u->bits[ c >> 6 ] |= std::uint64_t( 1 ) << ( c & 63 );
   ++f_nodes;
   }
  P[ i + 1 ] = v;
  }
 Node * e = P[ s.size() ];
 e->term = it;
 ++f_n;

 // from the node of s up: s is the smallest string of a node while nothing
 // there precedes its path, the largest while nothing follows it
 bool isMin = true;
 bool isMax = e->kids.empty();
 e->min = it;
 if( isMax )
  e->max = it;
 for( std::size_t i = s.size() ; i-- > 0 ; ) {
  Node * u = P[ i ];
  const unsigned c = static_cast< unsigned char >( s[ i ] );
  isMin = isMin && ! u->term && ! before( u , c );
  isMax = isMax && ! after( u , c );
  if( ! isMin && ! isMax )
   break;
  if( isMin )
   u->min = it;
  if( isMax )
   u->max = it;
  }
//! [BitmapTrie-insert]
 ADS_POST( contains( s ) );
 ADS_INVARIANT( consistent() );
 return( true );
 }

template< class V >
bool BitmapTrie< V >::remove( std::string_view s )
{
 std::vector< Node * > P( s.size() + 1 );
 P[ 0 ] = f_root;
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  if( ! ( P[ i + 1 ] = child( P[ i ] ,
                              static_cast< unsigned char >( s[ i ] ) ) ) )
   return( false );
 Node * e = P[ s.size() ];
 Cell * it = e->term;
 if( ! it )
  return( false );
 e->term = nullptr;
//! [BitmapTrie-remove]
 Cell * prev = it->prev;
 Cell * next = it->next;
 prev->next = next;
 next->prev = prev;
 --f_n;

 // the nodes left with no string below them
 std::size_t d = s.size();
 while( ( d > 0 ) && P[ d ]->kids.empty() && ! P[ d ]->term ) {
  Node * u = P[ d - 1 ];
  const unsigned c = static_cast< unsigned char >( s[ d - 1 ] );
  u->kids.erase( u->kids.begin() + std::ptrdiff_t( rank( u , c ) ) );
  u->bits[ c >> 6 ] &= ~( std::uint64_t( 1 ) << ( c & 63 ) );
  delete P[ d ];
  --f_nodes;
  --d;
  }

 // the strings below a node are consecutive in the list
 for( std::size_t i = 0 ; i <= d ; ++i ) {
  Node * u = P[ i ];
  if( u->min == it )
   u->min = ( f_n == 0 ) ? nullptr : next;
  if( u->max == it )
   u->max = ( f_n == 0 ) ? nullptr : prev;
  }
 delete it;
//! [BitmapTrie-remove]
 ADS_POST( ! contains( s ) );
 ADS_INVARIANT( consistent() );
 return( true );
 }

template< class V >
bool BitmapTrie< V >::subtree( const Node * u , const Cell * & lo ,
                               const Cell * & hi , std::size_t & n ) const
{
 std::size_t k = 0;
 for( const auto b : u->bits )
  k += std::size_t( std::popcount( b ) );
 if( k != u->kids.size() )
  return( false );
 lo = hi = u->term;
 if( u->term )
  ++n;
 for( const Node * c : u->kids ) {
  const Cell * a , * b;
  if( ! subtree( c , a , b , n ) || ! a )   // a child is never empty
   return( false );
  if( ! lo )
   lo = a;
  else if( ! ( hi->key < a->key ) )
   return( false );
  hi = b;
  }
 return( ( u->min == lo ) && ( u->max == hi ) );
 }

template< class V >
bool BitmapTrie< V >::consistent( void ) const
{
 if( f_n > 256 )
  return( true );
 std::size_t k = 0;
 for( const Cell * l = f_dummy->next ; l != f_dummy ; l = l->next , ++k )
  if( ( l->next->prev != l ) ||
      ( ( l->next != f_dummy ) && ! ( l->key < l->next->key ) ) )
   return( false );
 const Cell * lo , * hi;
 std::size_t n = 0;
 return( ( k == f_n ) && subtree( f_root , lo , hi , n ) && ( n == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BitmapTrie.tpp -------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- File BPlusTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BPlusTree [see BPlusTree.h], with their contracts.
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

template< std::totally_ordered K , class V >
BPlusTree< K , V >::BPlusTree( int m )
 : f_m( m )
{
 ADS_PRE( m > 2 );
 f_root = new Node( true );
 }

template< std::totally_ordered K , class V >
BPlusTree< K , V >::~BPlusTree()
{
 destroy( f_root );
 }

template< std::totally_ordered K , class V >
std::optional< V > BPlusTree< K , V >::lookup( const K & x ) const
{
 const Node * f = leaf( x );
 const auto i = std::lower_bound( f->k.begin() , f->k.end() , x ) -
                f->k.begin();
 if( ( i < long( f->k.size() ) ) && ( f->k[ i ] == x ) )
  return( f->v[ i ] );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
void BPlusTree< K , V >::insert( const K & x , const V & v )
{
 Node * f = leaf( x );
 const auto i = std::lower_bound( f->k.begin() , f->k.end() , x ) -
                f->k.begin();
 if( ( i < long( f->k.size() ) ) && ( f->k[ i ] == x ) ) {
  f->v[ i ] = v;                         // the key is there
  ADS_POST( lookup( x ).has_value() );
  return;
  }
 f->k.insert( f->k.begin() + i , x );
 f->v.insert( f->v.begin() + i , v );
 ++f_size;
 if( int( f->k.size() ) <= f_m ) {       // the leaf was not full
  ADS_INVARIANT( shaped() );
  return;
  }

 // the leaf f splits: the first a keys to a new leaf g on its left
 const int a = ( f_m + 2 ) / 2;          // ceil( ( m + 1 ) / 2 )
 Node * g = new Node( true );
 g->k.assign( f->k.begin() , f->k.begin() + a );
 g->v.assign( f->v.begin() , f->v.begin() + a );
 f->k.erase( f->k.begin() , f->k.begin() + a );
 f->v.erase( f->v.begin() , f->v.begin() + a );
 g->prev = f->prev;
 g->next = f;
 if( f->prev ) f->prev->next = g;
 f->prev = g;
 up( f , g , g->k.back() );
 ADS_INVARIANT( shaped() );
 }

template< std::totally_ordered K , class V >
std::size_t BPlusTree< K , V >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V >
int BPlusTree< K , V >::order( void ) const
{
 return( f_m );
 }

template< std::totally_ordered K , class V >
std::vector< std::pair< K , V > >
BPlusTree< K , V >::range( const K & A , const K & B ) const
{
 std::vector< std::pair< K , V > > out;
 for( const Node * f = leaf( A ) ; f ; f = f->next )
  for( std::size_t i = 0 ; i < f->k.size() ; ++i ) {
   if( B < f->k[ i ] )
    return( out );
   if( ! ( f->k[ i ] < A ) )
    out.emplace_back( f->k[ i ] , f->v[ i ] );
   }
 return( out );
 }

template< std::totally_ordered K , class V >
std::vector< std::vector< std::vector< K > > >
BPlusTree< K , V >::levels( void ) const
{
 std::vector< std::vector< std::vector< K > > > out;
 std::vector< const Node * > L = { f_root };
 while( ! L.empty() ) {
  out.emplace_back();
  std::vector< const Node * > N;
  for( auto u : L ) {
   out.back().push_back( u->k );
   N.insert( N.end() , u->p.begin() , u->p.end() );
   }
  L = std::move( N );
  }
 return( out );
 }

template< std::totally_ordered K , class V >
void BPlusTree< K , V >::check( void ) const
{
 if( ! shaped() )
  detail::violated( "invariant" , "shaped()" , __func__ , __FILE__ ,
                    __LINE__ );
 }

template< std::totally_ordered K , class V >
bool BPlusTree< K , V >::shaped( void ) const
{
 int depth = -1;
 const Node * last = nullptr;
 return( shaped( f_root , nullptr , nullptr , 0 , depth , last ) &&
         ! ( last && last->next ) );
 }

template< std::totally_ordered K , class V >
auto BPlusTree< K , V >::leaf( const K & x ) const -> Node *
{
 Node * u = f_root;
 while( ! u->is_leaf ) {
  const auto i = std::lower_bound( u->k.begin() , u->k.end() , x ) -
                 u->k.begin();            // the first k[ i ] >= x
  u = u->p[ i ];
  }
 return( u );
 }

template< std::totally_ordered K , class V >
void BPlusTree< K , V >::up( Node * f , Node * g , const K & s )
{
 Node * b = f->parent;
 if( ! b ) {                              // f was the root
  b = f_root = new Node( false );
  b->k = { s };
  b->p = { g , f };
  f->parent = g->parent = b;
  return;
  }
 const auto i = std::find( b->p.begin() , b->p.end() , f ) - b->p.begin();
 b->p.insert( b->p.begin() + i , g );
 b->k.insert( b->k.begin() + i , s );
 g->parent = b;
 if( int( b->p.size() ) <= f_m )
  return;

 // b has m + 1 children: the first a go to a new node c on its left, the
 // key between the two groups goes up
 const int a = ( f_m + 2 ) / 2;
 Node * c = new Node( false );
 c->p.assign( b->p.begin() , b->p.begin() + a );
 c->k.assign( b->k.begin() , b->k.begin() + ( a - 1 ) );
 const K mid = b->k[ a - 1 ];
 b->p.erase( b->p.begin() , b->p.begin() + a );
 b->k.erase( b->k.begin() , b->k.begin() + a );
 for( auto u : c->p )
  u->parent = c;
 up( b , c , mid );
 }

template< std::totally_ordered K , class V >
bool BPlusTree< K , V >::shaped( const Node * u , const K * lo ,
                                 const K * hi , int d , int & depth ,
                                 const Node * & last ) const
{
 if( ! std::is_sorted( u->k.begin() , u->k.end() ) ||
     ( std::adjacent_find( u->k.begin() , u->k.end() ) != u->k.end() ) )
  return( false );                       // the keys of a node increase
 for( const K & x : u->k )
  if( ( lo && ! ( *lo < x ) ) || ( hi && ( *hi < x ) ) )
   return( false );                      // within their interval
 if( u->is_leaf ) {
  if( ( ( u != f_root ) && u->k.empty() ) ||
      ( int( u->k.size() ) > f_m ) || ( u->k.size() != u->v.size() ) )
   return( false );                      // 1 to m keys, one value each
  if( depth < 0 )
   depth = d;
  else if( depth != d )
   return( false );                      // the leaves on one level
  if( ( u->prev != last ) || ( last && ( last->next != u ) ) )
   return( false );                      // and linked in order
  last = u;
  return( true );
  }
 const int f = int( u->p.size() );
 if( ( f > f_m ) || ( f < ( u == f_root ? 2 : ( f_m + 1 ) / 2 ) ) ||
     ( int( u->k.size() ) != f - 1 ) )
  return( false );                       // the children of a node
 for( int i = 0 ; i < f ; ++i )
  if( ( u->p[ i ]->parent != u ) ||
      ! shaped( u->p[ i ] , i > 0 ? &u->k[ i - 1 ] : lo ,
                i < f - 1 ? &u->k[ i ] : hi , d + 1 , depth , last ) )
   return( false );
 return( true );
 }

template< std::totally_ordered K , class V >
void BPlusTree< K , V >::destroy( Node * u )
{
 for( auto c : u->p )
  destroy( c );
 delete u;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File BPlusTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/

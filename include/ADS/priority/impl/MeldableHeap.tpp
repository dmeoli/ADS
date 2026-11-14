/*--------------------------------------------------------------------------*/
/*-------------------------- File MeldableHeap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MeldableHeap [see MeldableHeap.h], with their
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

template< class T , class Less , class A >
MeldableHeap< T , Less , A >::MeldableHeap( Less less , std::uint64_t seed ,
                                            const A & alloc )
 : f_less( std::move( less ) ) , f_g( seed ) , f_alloc( alloc )
{
 }

template< class T , class Less , class A >
MeldableHeap< T , Less , A >::MeldableHeap( MeldableHeap && o ) noexcept
 : f_root( std::exchange( o.f_root , nullptr ) ) ,
   f_n( std::exchange( o.f_n , 0 ) ) , f_less( o.f_less ) , f_g( o.f_g ) ,
   f_bits( o.f_bits ) , f_left( o.f_left ) , f_alloc( o.f_alloc )
{
 }

template< class T , class Less , class A >
MeldableHeap< T , Less , A > & MeldableHeap< T , Less , A >::operator=(
                                                  MeldableHeap && o ) noexcept
{
 std::swap( f_root , o.f_root );
 std::swap( f_n , o.f_n );
 std::swap( f_less , o.f_less );
 std::swap( f_g , o.f_g );
 std::swap( f_bits , o.f_bits );
 std::swap( f_left , o.f_left );
 std::swap( f_alloc , o.f_alloc );
 return( *this );
 }

template< class T , class Less , class A >
MeldableHeap< T , Less , A >::~MeldableHeap()
{
 clear();
 }

template< class T , class Less , class A >
void MeldableHeap< T , Less , A >::clear( void )
{
 std::vector< Node * > S;   // a stack: the tree may be deep
 if( f_root )
  S.push_back( f_root );
 while( ! S.empty() ) {
  Node * u = S.back();
  S.pop_back();
  if( u->left )
   S.push_back( u->left );
  if( u->right )
   S.push_back( u->right );
  drop( u );
  }
 f_root = nullptr;
 f_n = 0;
 }

template< class T , class Less , class A >
inline bool MeldableHeap< T , Less , A >::coin( void )
{
 if( f_left == 0 ) {
  f_bits = f_g();
  f_left = 64;
  }
 const bool c = f_bits & 1u;
 f_bits >>= 1;
 --f_left;
 return( c );
 }

template< class T , class Less , class A >
auto MeldableHeap< T , Less , A >::make( T && x ) -> Node *
{
 Node * u = Traits::allocate( f_alloc , 1 );
 try {
  Traits::construct( f_alloc , u , std::move( x ) );
  }
 catch( ... ) {
  Traits::deallocate( f_alloc , u , 1 );
  throw;
  }
 return( u );
 }

template< class T , class Less , class A >
void MeldableHeap< T , Less , A >::drop( Node * u )
{
 Traits::destroy( f_alloc , u );
 Traits::deallocate( f_alloc , u , 1 );
 }

template< class T , class Less , class A >
auto MeldableHeap< T , Less , A >::merge( Node * a , Node * b ) -> Node *
{
//! [MeldableHeap-merge]
 if( ! a )
  return( b );
 if( ! b )
  return( a );
 if( f_less( b->x , a->x ) )
  std::swap( a , b );
 Node * r = a;
 for( ; ; ) {   // b goes below a, whose item is not larger than b's
  Node * & c = coin() ? a->left : a->right;
  if( ! c ) {
   c = b;
   b->parent = a;
   return( r );
   }
  if( f_less( b->x , c->x ) ) {   // b takes the place of c, which goes
   Node * d = c;                  // below b
   c = b;
   b->parent = a;
   a = b;
   b = d;
   }
  else
   a = c;
  }
//! [MeldableHeap-merge]
 }

template< class T , class Less , class A >
auto MeldableHeap< T , Less , A >::push( T x ) -> Handle
{
 Node * u = make( std::move( x ) );
 f_root = merge( f_root , u );
 f_root->parent = nullptr;
 ++f_n;
 ADS_INVARIANT( consistent() );
 return( u );
 }

template< class T , class Less , class A >
const T & MeldableHeap< T , Less , A >::top( void ) const
{
 ADS_PRE( f_n > 0 );
 return( f_root->x );
 }

template< class T , class Less , class A >
const T * MeldableHeap< T , Less , A >::secondTop( void ) const
{
 if( f_n < 2 )
  return( nullptr );
 const Node * l = f_root->left;
 const Node * r = f_root->right;
 if( ! l || ( r && f_less( r->x , l->x ) ) )
  return( &r->x );
 return( &l->x );
 }

template< class T , class Less , class A >
T MeldableHeap< T , Less , A >::pop( void )
{
 ADS_PRE( f_n > 0 );
 return( erase( f_root ) );
 }

template< class T , class Less , class A >
T MeldableHeap< T , Less , A >::erase( Handle h )
{
 ADS_PRE( ( h != nullptr ) && ( f_n > 0 ) );
//! [MeldableHeap-erase]
 Node * m = merge( h->left , h->right );
 Node * p = h->parent;
 if( m )
  m->parent = p;
 if( ! p ) {
  ADS_PRE( h == f_root );
  f_root = m;
  }
 else if( p->left == h )
  p->left = m;
 else
  p->right = m;
//! [MeldableHeap-erase]
 T x = std::move( h->x );
 drop( h );
 --f_n;
 ADS_INVARIANT( consistent() );
 return( x );
 }

template< class T , class Less , class A >
const T & MeldableHeap< T , Less , A >::get( Handle h )
{
 return( h->x );
 }

template< class T , class Less , class A >
void MeldableHeap< T , Less , A >::meld( MeldableHeap & o )
{
 ADS_PRE( f_alloc == o.f_alloc );
 if( &o == this )
  return;
 f_root = merge( f_root , std::exchange( o.f_root , nullptr ) );
 if( f_root )
  f_root->parent = nullptr;
 f_n += std::exchange( o.f_n , 0 );
 ADS_INVARIANT( consistent() );
 }

template< class T , class Less , class A >
std::vector< T > MeldableHeap< T , Less , A >::smallest( std::size_t k ) const
{
 std::vector< T > S;
 if( ! f_root || ( k == 0 ) )
  return( S );
 S.reserve( k < f_n ? k : f_n );
 const auto better = [ this ]( const Node * a , const Node * b ) {
  return( f_less( a->x , b->x ) );
  };
 std::vector< const Node * > Q{ f_root };   // a binary heap of nodes
 while( ( S.size() < k ) && ! Q.empty() ) {
  std::pop_heap( Q.begin() , Q.end() ,
                 [ & ]( auto a , auto b ) { return( better( b , a ) ); } );
  const Node * u = Q.back();
  Q.pop_back();
  S.push_back( u->x );
  for( const Node * c : { u->left , u->right } )
   if( c ) {
    Q.push_back( c );
    std::push_heap( Q.begin() , Q.end() ,
                    [ & ]( auto a , auto b ) { return( better( b , a ) ); } );
    }
  }
 return( S );
 }

template< class T , class Less , class A >
bool MeldableHeap< T , Less , A >::subtree( const Node * u ,
                                            std::size_t & n ) const
{
 std::vector< const Node * > S{ u };
 while( ! S.empty() ) {
  const Node * v = S.back();
  S.pop_back();
  ++n;
  for( const Node * c : { v->left , v->right } )
   if( c ) {
    if( ( c->parent != v ) || f_less( c->x , v->x ) )
     return( false );
    S.push_back( c );
    }
  }
 return( true );
 }

template< class T , class Less , class A >
bool MeldableHeap< T , Less , A >::consistent( void ) const
{
 if( ! f_root )
  return( f_n == 0 );
 if( f_root->parent )
  return( false );
 if( f_n > 256 )
  return( true );
 std::size_t n = 0;
 return( subtree( f_root , n ) && ( n == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File MeldableHeap.tpp ------------------------*/
/*--------------------------------------------------------------------------*/

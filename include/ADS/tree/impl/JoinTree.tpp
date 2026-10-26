/*--------------------------------------------------------------------------*/
/*--------------------------- File JoinTree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of JoinTree [see JoinTree.h], with their contracts, and
 * the adaptor of its nodes to the algorithms based on join.
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

/*--------------------------------------------------------------------------*/
/*---------------------------- JoinTreeNodes -------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V , class B , class G >
struct JoinTreeNodes {
 using T = JoinTree< K , V , B , G >;
 using Node = typename T::Node;
 using Key = K;
 using Value = V;
 using Rank = int;

 static constexpr bool kAVL = std::is_same_v< B , AVLBalance >;
 static constexpr bool kRB = std::is_same_v< B , RedBlackBalance >;
 static constexpr bool kWB = std::is_same_v< B , WeightBalance >;
 static constexpr bool kTreap = std::is_same_v< B , TreapBalance >;
 static constexpr bool kAug = ! std::is_same_v< G , NoAugmentation >;
 static_assert( kAVL || kRB || kWB || kTreap , "an unknown scheme" );

 static Node * nil( void ) { return( nullptr ); }
 static Node * left( const Node * t ) { return( t->left ); }
 static Node * right( const Node * t ) { return( t->right ); }
 static const K & key( const Node * t ) { return( t->key ); }
 static V & value( Node * t ) { return( t->value ); }
 static std::size_t size( const Node * t ) { return( t ? t->size : 0 ); }
 static typename G::Value aug( const Node * t ) {
  return( t ? t->aug : G::identity() );
  }

 /// AVL: the height, -1 for nil
 static int height( const Node * t ) { return( t ? int( t->meta ) - 1 : -1 ); }

 /// Red-Black: the color and the black height
 static bool red( const Node * t ) { return( t && ( t->meta & 1 ) ); }
 static int black( const Node * t ) { return( t ? int( t->meta >> 1 ) : 0 ); }
 static void paint( Node * t , bool r ) {
  t->meta = ( std::uint64_t( black( t->left ) + ( r ? 0 : 1 ) ) << 1 ) |
            std::uint64_t( r );
  }

 /// treap: the priority
 static std::uint64_t priority( const Node * t ) { return( t->meta ); }

 static void expose( Node * t , Node * & l , Node * & r ) {
  l = t->left;
  r = t->right;
  t->left = t->right = nullptr;
  }

 static Node * connect( Node * m , Node * l , Node * r ) {
  m->left = l;
  m->right = r;
  m->size = 1 + size( l ) + size( r );
  if constexpr( kAVL )
   m->meta = std::uint64_t( std::max( height( l ) , height( r ) ) + 2 );
  if constexpr( kRB )
   paint( m , red( m ) );
  if constexpr( kAug )
   m->aug = G::combine( G::combine( aug( l ) , G::from( m->key ,
                                                         m->value ) ) ,
                        aug( r ) );
  return( m );
  }

 static void dispose( Node * t ) {
  if( ! t ) return;
  dispose( t->left );
  dispose( t->right );
  delete t;
  }

 static Rank rank( const Node * t ) {
  if constexpr( kAVL ) return( height( t ) );
  else if constexpr( kRB ) return( black( t ) );
  else return( 0 );
  }

 static Rank childRank( const Node * , Rank , const Node * c ) {
  return( rank( c ) );
  }

 static std::pair< Node * , Rank > join( Node * l , Rank rl , Node * m ,
                                         Node * r , Rank rr ) {
  using Result = std::pair< Node * , Rank >;
  if constexpr( kAVL ) {
   Node * t = AVLJoin< JoinTreeNodes >::join( l , m , r );
   return( Result( t , height( t ) ) );
   }
  else if constexpr( kRB )
   return( RedBlackJoin< JoinTreeNodes >::join( l , rl , m , r , rr ) );
  else if constexpr( kWB )
   return( Result( WeightBalancedJoin< JoinTreeNodes >::join( l , m , r ) ,
                   0 ) );
  else
   return( Result( TreapJoin< JoinTreeNodes >::join( l , m , r ) , 0 ) );
  }

 /// the tree side, for Joinable
 static Node * take( T & t ) {
  Node * r = t.f_root;
  t.f_root = nullptr;
  return( r );
  }
 static void put( T & t , Node * r ) {
  if constexpr( kRB )
   if( red( r ) )
    paint( r , false );
  dispose( t.f_root );
  t.f_root = r;
  }
 static Node * peek( const T & t ) { return( t.f_root ); }
 static T fresh( T & t ) { return( T( t.f_g() ) ); }
 static bool valid( const T & t ) { return( t.valid() ); }
 static std::uint64_t salt( T & t ) {
  if constexpr( kTreap ) return( t.f_g() );
  else return( 0 );
  }
 static Node * make( const K & k , const V & v , std::uint64_t s ,
                     std::size_t i ) {
  Node * n = new Node{ nullptr , nullptr , 1 , 0 , G::identity() , k , v };
  if constexpr( kTreap )
   n->meta = splitmix64( s + 0x9e3779b97f4a7c15ULL * std::uint64_t( i ) );
  if constexpr( kRB )
   n->meta = 2;
  return( connect( n , nullptr , nullptr ) );
  }
 };

/*--------------------------------------------------------------------------*/
/*------------------------------ JoinTree ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
JoinTree< K , V , B , G >::JoinTree( std::uint64_t seed ) : f_g( seed )
{
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
JoinTree< K , V , B , G >::JoinTree( const JoinTree & o ) : f_g( o.f_g )
{
 auto copy = [ & ]( auto & self , const Node * u ) -> Node * {
  if( ! u ) return( nullptr );
  Node * c = new Node( *u );
  c->left = c->right = nullptr;
  c->left = self( self , u->left );
  c->right = self( self , u->right );
  return( c );
  };
 f_root = copy( copy , o.f_root );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
JoinTree< K , V , B , G >::JoinTree( JoinTree && o ) noexcept
 : f_root( o.f_root ) , f_g( o.f_g )
{
 o.f_root = nullptr;
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
auto JoinTree< K , V , B , G >::operator=( JoinTree o ) noexcept
 -> JoinTree &
{
 swap( o );
 return( *this );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
JoinTree< K , V , B , G >::~JoinTree()
{
 Nodes::dispose( f_root );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
void JoinTree< K , V , B , G >::swap( JoinTree & o ) noexcept
{
 std::swap( f_root , o.f_root );
 std::swap( f_g , o.f_g );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
std::optional< V > JoinTree< K , V , B , G >::lookup( const K & k ) const
{
 for( const Node * t = f_root ; t ; )
  if( k < t->key ) t = t->left;
  else if( t->key < k ) t = t->right;
  else return( t->value );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
void JoinTree< K , V , B , G >::insert( const K & k , const V & v )
{
 Node * n = Nodes::make( k , v , f_g() , 0 );
 using J = JoinAlgorithms< Nodes >;
 bool fresh = false;
 f_root = J::insert( J::whole( f_root ) , n , fresh ).t;
 Nodes::put( *this , Nodes::take( *this ) );
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
void JoinTree< K , V , B , G >::remove( const K & k )
{
 using J = JoinAlgorithms< Nodes >;
 bool gone = false;
 f_root = J::remove( J::whole( f_root ) , k , gone ).t;
 Nodes::put( *this , Nodes::take( *this ) );
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
bool JoinTree< K , V , B , G >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
std::size_t JoinTree< K , V , B , G >::size( void ) const
{
 return( Nodes::size( f_root ) );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
bool JoinTree< K , V , B , G >::empty( void ) const
{
 return( f_root == nullptr );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
void JoinTree< K , V , B , G >::clear( void )
{
 Nodes::dispose( f_root );
 f_root = nullptr;
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
int JoinTree< K , V , B , G >::height( void ) const
{
 auto h = [ & ]( auto & self , const Node * t ) -> int {
  return( t ? 1 + std::max( self( self , t->left ) ,
                            self( self , t->right ) ) : -1 );
  };
 return( h( h , f_root ) );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
template< class F >
void JoinTree< K , V , B , G >::forEach( F && f ) const
{
 auto visit = [ & ]( auto & self , const Node * t ) -> void {
  if( ! t ) return;
  self( self , t->left );
  f( t->key , t->value );
  self( self , t->right );
  };
 visit( visit , f_root );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
auto JoinTree< K , V , B , G >::augValue( void ) const -> AugValue
{
 return( Nodes::aug( f_root ) );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
template< class F >
void JoinTree< K , V , B , G >::augVisit( const K & lo , const K & hi ,
                                          F && f ) const
{
 if( hi < lo )
  return;
 const Node * t = f_root;            // the node where lo and hi part
 while( t && ( ( t->key < lo ) || ( hi < t->key ) ) )
  t = hi < t->key ? t->left : t->right;
 if( ! t )
  return;
 // the left path: the pieces >= lo, met from the right to the left, kept
 // to be given in increasing order
 std::vector< const Node * > P;
 for( const Node * u = t->left ; u ; )
  if( u->key < lo ) u = u->right;
  else {
   P.push_back( u );
   u = u->left;
   }
 for( auto it = P.rbegin() ; it != P.rend() ; ++it ) {
  f( G::from( ( *it )->key , ( *it )->value ) );
  if( ( *it )->right )
   f( ( *it )->right->aug );
  }
 f( G::from( t->key , t->value ) );
 for( const Node * u = t->right ; u ; )      // the right path
  if( hi < u->key ) u = u->left;
  else {
   if( u->left )
    f( u->left->aug );
   f( G::from( u->key , u->value ) );
   u = u->right;
   }
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
auto JoinTree< K , V , B , G >::augRange( const K & lo ,
                                          const K & hi ) const -> AugValue
{
 AugValue a = G::identity();
 augVisit( lo , hi , [ & ]( const AugValue & x ) {
  a = G::combine( a , x ); } );
 return( a );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
auto JoinTree< K , V , B , G >::augPrefix( const K & k ) const -> AugValue
{
 AugValue a = G::identity();
 for( const Node * u = f_root ; u ; )
  if( k < u->key ) u = u->left;
  else {
   a = G::combine( G::combine( a , Nodes::aug( u->left ) ) ,
                   G::from( u->key , u->value ) );
   u = u->right;
   }
 return( a );
 }

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
bool JoinTree< K , V , B , G >::valid( void ) const
{
 using W = WeightBalancedJoin< Nodes >;
 bool ok = true;
 // the black height of the subtree (Red-Black), 0 for the others
 auto rec = [ & ]( auto & self , const Node * t , const K * lo ,
                   const K * hi ) -> int {
  if( ( ! ok ) || ( ! t ) )
   return( 0 );
  if( ( lo && !( *lo < t->key ) ) || ( hi && !( t->key < *hi ) ) ||
      ( t->size != 1 + Nodes::size( t->left ) + Nodes::size( t->right ) ) )
   ok = false;
  const int bl = self( self , t->left , lo , &t->key ) ,
            br = self( self , t->right , &t->key , hi );
  if constexpr( Nodes::kAug && std::equality_comparable< AugValue > )
   if( !( t->aug == G::combine( G::combine( Nodes::aug( t->left ) ,
                                            G::from( t->key , t->value ) ) ,
                                Nodes::aug( t->right ) ) ) )
    ok = false;
  if constexpr( Nodes::kAVL ) {
   const int l = Nodes::height( t->left ) , r = Nodes::height( t->right );
   if( ( Nodes::height( t ) != 1 + std::max( l , r ) ) || ( l - r > 1 ) ||
       ( r - l > 1 ) )
    ok = false;
   }
  if constexpr( Nodes::kRB ) {
   const int b = bl + ( Nodes::red( t ) ? 0 : 1 );
   if( ( bl != br ) || ( Nodes::black( t ) != b ) ||
       ( Nodes::red( t ) && ( Nodes::red( t->left ) ||
                              Nodes::red( t->right ) ) ) )
    ok = false;
   return( b );
   }
  if constexpr( Nodes::kWB )
   if( ! W::like( W::weight( t->left ) , W::weight( t->right ) ) )
    ok = false;
  if constexpr( Nodes::kTreap )
   if( ( t->left && ( t->left->meta > t->meta ) ) ||
       ( t->right && ( t->right->meta > t->meta ) ) )
    ok = false;
  return( 0 );
  };
 rec( rec , f_root , nullptr , nullptr );
 if constexpr( Nodes::kRB )
  if( Nodes::red( f_root ) )
   ok = false;
 return( ok );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File JoinTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/

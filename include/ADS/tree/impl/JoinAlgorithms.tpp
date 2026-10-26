/*--------------------------------------------------------------------------*/
/*------------------------ File JoinAlgorithms.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The joins of the four balancing schemes and the algorithms built on them
 * [see JoinAlgorithms.h], with their contracts.
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
/*------------------------------- ROTATIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// the right child of t goes up, by expose() and connect()
template< class A >
typename A::Node * joinRotateLeft( typename A::Node * t )
{
 typename A::Node * a , * b , * c , * d;
 A::expose( t , a , b );
 A::expose( b , c , d );
 A::connect( t , a , c );
 return( A::connect( b , t , d ) );
 }

/// the left child of t goes up, by expose() and connect()
template< class A >
typename A::Node * joinRotateRight( typename A::Node * t )
{
 typename A::Node * a , * b , * c , * d;
 A::expose( t , a , b );
 A::expose( a , c , d );
 A::connect( t , d , b );
 return( A::connect( a , c , t ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- AVLJoin ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< class A >
auto AVLJoin< A >::joinRight( Node * l , Node * m , Node * r ) -> Node *
{
 Node * ll , * c;
 A::expose( l , ll , c );
 if( A::height( c ) <= A::height( r ) + 1 ) {
  Node * t = A::connect( m , c , r );
  if( A::height( t ) <= A::height( ll ) + 1 )
   return( A::connect( l , ll , t ) );
  return( detail::joinRotateLeft< A >(
           A::connect( l , ll , detail::joinRotateRight< A >( t ) ) ) );
  }
 Node * t = joinRight( c , m , r );
 Node * u = A::connect( l , ll , t );
 if( A::height( t ) <= A::height( ll ) + 1 )
  return( u );
 return( detail::joinRotateLeft< A >( u ) );
 }

template< class A >
auto AVLJoin< A >::joinLeft( Node * l , Node * m , Node * r ) -> Node *
{
 Node * c , * rr;
 A::expose( r , c , rr );
 if( A::height( c ) <= A::height( l ) + 1 ) {
  Node * t = A::connect( m , l , c );
  if( A::height( t ) <= A::height( rr ) + 1 )
   return( A::connect( r , t , rr ) );
  return( detail::joinRotateRight< A >(
           A::connect( r , detail::joinRotateLeft< A >( t ) , rr ) ) );
  }
 Node * t = joinLeft( l , m , c );
 Node * u = A::connect( r , t , rr );
 if( A::height( t ) <= A::height( rr ) + 1 )
  return( u );
 return( detail::joinRotateRight< A >( u ) );
 }

template< class A >
auto AVLJoin< A >::join( Node * l , Node * m , Node * r ) -> Node *
{
 if( A::height( l ) > A::height( r ) + 1 )
  return( joinRight( l , m , r ) );
 if( A::height( r ) > A::height( l ) + 1 )
  return( joinLeft( l , m , r ) );
 return( A::connect( m , l , r ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- RedBlackJoin --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class A >
auto RedBlackJoin< A >::joinRight( Node * l , int bl , Node * m , Node * r ,
                                   int br ) -> Node *
{
 if( ( ! A::red( l ) ) && ( bl == br ) ) {
  A::paint( m , true );
  return( A::connect( m , l , r ) );
  }
 const bool black = ! A::red( l );
 Node * ll , * c;
 A::expose( l , ll , c );
 Node * t = joinRight( c , bl - ( black ? 1 : 0 ) , m , r , br );
 Node * u = A::connect( l , ll , t );
 if( black && A::red( t ) && A::red( A::right( t ) ) ) {
  A::paint( A::right( t ) , false );
  return( detail::joinRotateLeft< A >( u ) );
  }
 return( u );
 }

template< class A >
auto RedBlackJoin< A >::joinLeft( Node * l , int bl , Node * m , Node * r ,
                                  int br ) -> Node *
{
 if( ( ! A::red( r ) ) && ( bl == br ) ) {
  A::paint( m , true );
  return( A::connect( m , l , r ) );
  }
 const bool black = ! A::red( r );
 Node * c , * rr;
 A::expose( r , c , rr );
 Node * t = joinLeft( l , bl , m , c , br - ( black ? 1 : 0 ) );
 Node * u = A::connect( r , t , rr );
 if( black && A::red( t ) && A::red( A::left( t ) ) ) {
  A::paint( A::left( t ) , false );
  return( detail::joinRotateRight< A >( u ) );
  }
 return( u );
 }

template< class A >
auto RedBlackJoin< A >::join( Node * l , int bl , Node * m , Node * r ,
                              int br ) -> std::pair< Node * , int >
{
 using Result = std::pair< Node * , int >;
 if( bl > br ) {
  Node * t = joinRight( l , bl , m , r , br );
  if( A::red( t ) && A::red( A::right( t ) ) ) {
   A::paint( t , false );
   return( Result( t , bl + 1 ) );
   }
  return( Result( t , bl ) );
  }
 if( br > bl ) {
  Node * t = joinLeft( l , bl , m , r , br );
  if( A::red( t ) && A::red( A::left( t ) ) ) {
   A::paint( t , false );
   return( Result( t , br + 1 ) );
   }
  return( Result( t , br ) );
  }
 if( ( ! A::red( l ) ) && ( ! A::red( r ) ) ) {
  A::paint( m , true );
  return( Result( A::connect( m , l , r ) , bl ) );
  }
 A::paint( m , false );
 return( Result( A::connect( m , l , r ) , bl + 1 ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- WeightBalancedJoin ----------------------------*/
/*--------------------------------------------------------------------------*/

template< class A >
bool WeightBalancedJoin< A >::like( std::size_t a , std::size_t b )
{
 const std::size_t s = a + b;
 return( ( kAlphaDen * a >= kAlphaNum * s ) &&
         ( kAlphaDen * b >= kAlphaNum * s ) );
 }

template< class A >
std::size_t WeightBalancedJoin< A >::weight( const Node * t )
{
 return( A::size( t ) + 1 );
 }

template< class A >
auto WeightBalancedJoin< A >::joinRight( Node * l , Node * m ,
                                         Node * r ) -> Node *
{
 if( like( weight( l ) , weight( r ) ) )
  return( A::connect( m , l , r ) );
 Node * ll , * c;
 A::expose( l , ll , c );
 Node * t = joinRight( c , m , r );
 if( like( weight( ll ) , weight( t ) ) )
  return( A::connect( l , ll , t ) );
 const std::size_t w1 = weight( A::left( t ) ) , w2 = weight( A::right( t ) );
 if( like( weight( ll ) , w1 ) && like( weight( ll ) + w1 , w2 ) )
  return( detail::joinRotateLeft< A >( A::connect( l , ll , t ) ) );
 return( detail::joinRotateLeft< A >(
          A::connect( l , ll , detail::joinRotateRight< A >( t ) ) ) );
 }

template< class A >
auto WeightBalancedJoin< A >::joinLeft( Node * l , Node * m ,
                                        Node * r ) -> Node *
{
 if( like( weight( l ) , weight( r ) ) )
  return( A::connect( m , l , r ) );
 Node * c , * rr;
 A::expose( r , c , rr );
 Node * t = joinLeft( l , m , c );
 if( like( weight( t ) , weight( rr ) ) )
  return( A::connect( r , t , rr ) );
 const std::size_t w1 = weight( A::left( t ) ) , w2 = weight( A::right( t ) );
 if( like( weight( rr ) , w2 ) && like( weight( rr ) + w2 , w1 ) )
  return( detail::joinRotateRight< A >( A::connect( r , t , rr ) ) );
 return( detail::joinRotateRight< A >(
          A::connect( r , detail::joinRotateLeft< A >( t ) , rr ) ) );
 }

template< class A >
auto WeightBalancedJoin< A >::join( Node * l , Node * m , Node * r ) -> Node *
{
 const std::size_t wl = weight( l ) , wr = weight( r );
 if( like( wl , wr ) )
  return( A::connect( m , l , r ) );
 return( wl > wr ? joinRight( l , m , r ) : joinLeft( l , m , r ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- TreapJoin --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class A >
auto TreapJoin< A >::join( Node * l , Node * m , Node * r ) -> Node *
{
 const Node * nil = A::nil();
 const auto p = A::priority( m );
 const bool tl = ( l != nil ) && ( A::priority( l ) > p ) &&
                 ( ( r == nil ) || ( A::priority( l ) >= A::priority( r ) ) );
 const bool tr = ( ! tl ) && ( r != nil ) && ( A::priority( r ) > p );
 if( tl ) {
  Node * ll , * lr;
  A::expose( l , ll , lr );
  return( A::connect( l , ll , join( lr , m , r ) ) );
  }
 if( tr ) {
  Node * rl , * rr;
  A::expose( r , rl , rr );
  return( A::connect( r , join( l , m , rl ) , rr ) );
  }
 return( A::connect( m , l , r ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- JoinAlgorithms ------------------------------*/
/*--------------------------------------------------------------------------*/

template< JoinAdaptor A >
auto JoinAlgorithms< A >::empty( void ) -> Tree
{
 return( Tree{ A::nil() , A::rank( A::nil() ) } );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::whole( Node * t ) -> Tree
{
 return( Tree{ t , A::rank( t ) } );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::expose( Tree x ) -> Exposed
{
 Node * l = A::left( x.t ) , * r = A::right( x.t );
 const Rank rl = A::childRank( x.t , x.r , l ) ,
            rr = A::childRank( x.t , x.r , r );
 A::expose( x.t , l , r );
 return( Exposed{ Tree{ l , rl } , x.t , Tree{ r , rr } } );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::join( Tree l , Node * m , Tree r ) -> Tree
{
 const auto [ t , rk ] = A::join( l.t , l.r , m , r.t , r.r );
 return( Tree{ t , rk } );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::split( Tree x , const Key & k , Tree & l ,
                                 Tree & r ) -> Node *
{
 if( x.t == A::nil() ) {
  l = r = empty();
  return( A::nil() );
  }
 const Exposed e = expose( x );
 if( k < A::key( e.m ) ) {
  Tree lr;
  Node * f = split( e.l , k , l , lr );
  r = join( lr , e.m , e.r );
  return( f );
  }
 if( A::key( e.m ) < k ) {
  Tree rl;
  Node * f = split( e.r , k , rl , r );
  l = join( e.l , e.m , rl );
  return( f );
  }
 l = e.l;
 r = e.r;
 return( e.m );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::splitLast( Tree x , Tree & rest ) -> Node *
{
 const Exposed e = expose( x );
 if( e.r.t == A::nil() ) {
  rest = e.l;
  return( e.m );
  }
 Tree t;
 Node * last = splitLast( e.r , t );
 rest = join( e.l , e.m , t );
 return( last );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::join2( Tree l , Tree r ) -> Tree
{
 if( l.t == A::nil() )
  return( r );
 if( r.t == A::nil() )
  return( l );
 Tree rest;
 Node * m = splitLast( l , rest );
 return( join( rest , m , r ) );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::insert( Tree x , Node * n , bool & fresh ) -> Tree
{
 if( x.t == A::nil() ) {
  fresh = true;
  return( join( empty() , n , empty() ) );
  }
 const Exposed e = expose( x );
 if( A::key( n ) < A::key( e.m ) )
  return( join( insert( e.l , n , fresh ) , e.m , e.r ) );
 if( A::key( e.m ) < A::key( n ) )
  return( join( e.l , e.m , insert( e.r , n , fresh ) ) );
 fresh = false;
 A::value( e.m ) = std::move( A::value( n ) );
 A::dispose( n );
 return( join( e.l , e.m , e.r ) );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::remove( Tree x , const Key & k , bool & gone )
 -> Tree
{
 if( x.t == A::nil() ) {
  gone = false;
  return( x );
  }
 const Exposed e = expose( x );
 if( k < A::key( e.m ) )
  return( join( remove( e.l , k , gone ) , e.m , e.r ) );
 if( A::key( e.m ) < k )
  return( join( e.l , e.m , remove( e.r , k , gone ) ) );
 gone = true;
 A::dispose( e.m );
 return( join2( e.l , e.r ) );
 }

template< JoinAdaptor A >
template< class G , class H >
void JoinAlgorithms< A >::fork( Sequential & , G && g , H && h )
{
 g();
 h();
 }

template< JoinAdaptor A >
template< Executor E , class G , class H >
void JoinAlgorithms< A >::fork( E & ex , G && g , H && h )
{
 parallelInvoke( ex , std::forward< G >( g ) , std::forward< H >( h ) );
 }

template< JoinAdaptor A >
template< Executor E >
bool JoinAlgorithms< A >::small( E & ex , std::size_t n )
{
 if constexpr( std::is_same_v< E , Sequential > )
  return( false );
 else
  return( ( n < kJoinGrain ) || ( ex.concurrency() < 2 ) );
 }

template< JoinAdaptor A >
template< Executor E , class F >
auto JoinAlgorithms< A >::unite( E & ex , Tree a , Tree b , const F & f )
 -> Tree
{
 if( a.t == A::nil() ) return( b );
 if( b.t == A::nil() ) return( a );
 const std::size_t na = A::size( a.t ) , nb = A::size( b.t );
 if( small( ex , na + nb ) ) {
  Sequential seq;
  return( unite( seq , a , b , f ) );
  }
 const bool ea = na < nb;              // the smaller one is exposed
 const Exposed e = expose( ea ? a : b );
 Tree sl , sr;
 Node * d = split( ea ? b : a , A::key( e.m ) , sl , sr );
 Tree tl , tr;
 fork( ex , [ & ]() { tl = ea ? unite( ex , e.l , sl , f )
                              : unite( ex , sl , e.l , f ); } ,
            [ & ]() { tr = ea ? unite( ex , e.r , sr , f )
                              : unite( ex , sr , e.r , f ); } );
 if( d != A::nil() ) {
  Value & v = A::value( e.m );
  v = ea ? f( std::as_const( v ) , std::as_const( A::value( d ) ) )
         : f( std::as_const( A::value( d ) ) , std::as_const( v ) );
  A::dispose( d );
  }
 return( join( tl , e.m , tr ) );
 }

template< JoinAdaptor A >
template< Executor E , class F >
auto JoinAlgorithms< A >::intersect( E & ex , Tree a , Tree b , const F & f )
 -> Tree
{
 if( ( a.t == A::nil() ) || ( b.t == A::nil() ) ) {
  A::dispose( a.t );
  A::dispose( b.t );
  return( empty() );
  }
 const std::size_t na = A::size( a.t ) , nb = A::size( b.t );
 if( small( ex , na + nb ) ) {
  Sequential seq;
  return( intersect( seq , a , b , f ) );
  }
 const bool ea = na < nb;
 const Exposed e = expose( ea ? a : b );
 Tree sl , sr;
 Node * d = split( ea ? b : a , A::key( e.m ) , sl , sr );
 Tree tl , tr;
 fork( ex , [ & ]() { tl = ea ? intersect( ex , e.l , sl , f )
                              : intersect( ex , sl , e.l , f ); } ,
            [ & ]() { tr = ea ? intersect( ex , e.r , sr , f )
                              : intersect( ex , sr , e.r , f ); } );
 if( d == A::nil() ) {
  A::dispose( e.m );
  return( join2( tl , tr ) );
  }
 Value & v = A::value( e.m );
 v = ea ? f( std::as_const( v ) , std::as_const( A::value( d ) ) )
        : f( std::as_const( A::value( d ) ) , std::as_const( v ) );
 A::dispose( d );
 return( join( tl , e.m , tr ) );
 }

template< JoinAdaptor A >
template< Executor E >
auto JoinAlgorithms< A >::subtract( E & ex , Tree a , Tree b ) -> Tree
{
 if( a.t == A::nil() ) {
  A::dispose( b.t );
  return( a );
  }
 if( b.t == A::nil() )
  return( a );
 const std::size_t na = A::size( a.t ) , nb = A::size( b.t );
 if( small( ex , na + nb ) ) {
  Sequential seq;
  return( subtract( seq , a , b ) );
  }
 Tree tl , tr;
 if( na < nb ) {                       // the keys of a split b
  const Exposed e = expose( a );
  Tree sl , sr;
  Node * d = split( b , A::key( e.m ) , sl , sr );
  fork( ex , [ & ]() { tl = subtract( ex , e.l , sl ); } ,
             [ & ]() { tr = subtract( ex , e.r , sr ); } );
  if( d == A::nil() )
   return( join( tl , e.m , tr ) );
  A::dispose( d );
  A::dispose( e.m );
  return( join2( tl , tr ) );
  }
 const Exposed e = expose( b );        // the keys of b split a
 Tree sl , sr;
 Node * d = split( a , A::key( e.m ) , sl , sr );
 fork( ex , [ & ]() { tl = subtract( ex , sl , e.l ); } ,
            [ & ]() { tr = subtract( ex , sr , e.r ); } );
 A::dispose( d );
 A::dispose( e.m );
 return( join2( tl , tr ) );
 }

template< JoinAdaptor A >
template< Executor E >
auto JoinAlgorithms< A >::build( E & ex , Node * const * N , std::size_t n )
 -> Tree
{
 if( n == 0 )
  return( empty() );
 if( small( ex , n ) ) {
  Sequential seq;
  return( build( seq , N , n ) );
  }
 const std::size_t mid = n / 2;
 Tree l , r;
 fork( ex , [ & ]() { l = build( ex , N , mid ); } ,
            [ & ]() { r = build( ex , N + mid + 1 , n - mid - 1 ); } );
 return( join( l , N[ mid ] , r ) );
 }

template< JoinAdaptor A >
template< Executor E , class F >
auto JoinAlgorithms< A >::multiInsert( E & ex , Tree x , Node * const * N ,
                                       std::size_t n , const F & f ) -> Tree
{
 if( n == 0 )
  return( x );
 if( x.t == A::nil() )
  return( build( ex , N , n ) );
 if( small( ex , A::size( x.t ) + n ) ) {
  Sequential seq;
  return( multiInsert( seq , x , N , n , f ) );
  }
 const Exposed e = expose( x );
 const Key & k = A::key( e.m );
 const std::size_t j = std::size_t( std::partition_point( N , N + n ,
                        [ & ]( const Node * u ) { return( A::key( u ) < k ); }
                        ) - N );
 const bool dup = ( j < n ) && !( k < A::key( N[ j ] ) );
 Tree l , r;
 fork( ex , [ & ]() { l = multiInsert( ex , e.l , N , j , f ); } ,
            [ & ]() { r = multiInsert( ex , e.r , N + j + dup ,
                                       n - j - dup , f ); } );
 if( dup ) {
  Value & v = A::value( e.m );
  v = f( std::as_const( v ) , std::as_const( A::value( N[ j ] ) ) );
  A::dispose( N[ j ] );
  }
 return( join( l , e.m , r ) );
 }

template< JoinAdaptor A >
template< Executor E >
auto JoinAlgorithms< A >::multiRemove( E & ex , Tree x , const Key * K ,
                                       std::size_t n , std::size_t & gone )
 -> Tree
{
 if( ( n == 0 ) || ( x.t == A::nil() ) ) {
  gone = 0;
  return( x );
  }
 if( small( ex , A::size( x.t ) + n ) ) {
  Sequential seq;
  return( multiRemove( seq , x , K , n , gone ) );
  }
 const Exposed e = expose( x );
 const Key & k = A::key( e.m );
 const std::size_t j = std::size_t( std::lower_bound( K , K + n , k ) - K );
 const bool hit = ( j < n ) && !( k < K[ j ] );
 Tree l , r;
 std::size_t gl = 0 , gr = 0;
 fork( ex , [ & ]() { l = multiRemove( ex , e.l , K , j , gl ); } ,
            [ & ]() { r = multiRemove( ex , e.r , K + j + hit ,
                                       n - j - hit , gr ); } );
 gone = gl + gr + hit;
 if( hit ) {
  A::dispose( e.m );
  return( join2( l , r ) );
  }
 return( join( l , e.m , r ) );
 }

template< JoinAdaptor A >
template< Executor E , class P >
auto JoinAlgorithms< A >::filter( E & ex , Tree x , const P & p ) -> Tree
{
 if( x.t == A::nil() )
  return( x );
 if( small( ex , A::size( x.t ) ) ) {
  Sequential seq;
  return( filter( seq , x , p ) );
  }
 const Exposed e = expose( x );
 Tree l , r;
 fork( ex , [ & ]() { l = filter( ex , e.l , p ); } ,
            [ & ]() { r = filter( ex , e.r , p ); } );
 if( p( A::key( e.m ) , std::as_const( A::value( e.m ) ) ) )
  return( join( l , e.m , r ) );
 A::dispose( e.m );
 return( join2( l , r ) );
 }

template< JoinAdaptor A >
template< Executor E , class M , class R , class U >
U JoinAlgorithms< A >::mapReduce( E & ex , Node * t , const M & map ,
                                  const R & reduce , const U & id )
{
 if( t == A::nil() )
  return( id );
 if( small( ex , A::size( t ) ) ) {
  Sequential seq;
  return( mapReduce( seq , t , map , reduce , id ) );
  }
 U l = id , r = id;
 fork( ex , [ & ]() { l = mapReduce( ex , A::left( t ) , map , reduce ,
                                     id ); } ,
            [ & ]() { r = mapReduce( ex , A::right( t ) , map , reduce ,
                                     id ); } );
 return( reduce( reduce( std::move( l ) ,
                         map( A::key( t ) ,
                              std::as_const( A::value( t ) ) ) ) ,
                 std::move( r ) ) );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::extract( Tree & x , const Key & lo ,
                                   const Key & hi ) -> Tree
{
 if( hi < lo )
  return( empty() );
 Tree a , b , c , d;
 Node * u = split( x , lo , a , b );
 if( u != A::nil() )
  b = join( empty() , u , b );
 Node * v = split( b , hi , c , d );
 if( v != A::nil() )
  c = join( c , v , empty() );
 x = join2( a , d );
 return( c );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- Joinable ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class A , class Key , class Value >
auto Joinable< T , A , Key , Value >::split( const Key & k ) &&
 -> std::tuple< T , std::optional< Value > , T >
{
 using J = JoinAlgorithms< A >;
 using Node = typename A::Node;
 T & self = static_cast< T & >( *this );
 T right = A::fresh( self );
 typename J::Tree l , r;
 Node * m = J::split( J::whole( A::take( self ) ) , k , l , r );
 std::optional< Value > v;
 if( m != A::nil() ) {
  v = std::move( A::value( m ) );
  A::dispose( m );
  }
 A::put( self , l.t );
 A::put( right , r.t );
 ADS_INVARIANT( A::valid( self ) && A::valid( right ) );
 return( std::tuple< T , std::optional< Value > , T >( std::move( self ) ,
          std::move( v ) , std::move( right ) ) );
 }

template< class T , class A , class Key , class Value >
T Joinable< T , A , Key , Value >::join( T && l , const Key & k ,
                                         const Value & v , T && r )
{
 using J = JoinAlgorithms< A >;
 using Node = typename A::Node;
 ADS_PRE( ( A::peek( l ) == A::nil() ) ||
          ( J::lastKey( A::peek( l ) ) < k ) );
 ADS_PRE( ( A::peek( r ) == A::nil() ) ||
          ( k < J::firstKey( A::peek( r ) ) ) );
 Node * m = A::make( k , v , A::salt( l ) , 0 );
 const typename J::Tree tl = J::whole( A::take( l ) ) ,
                        tr = J::whole( A::take( r ) );
 A::put( l , J::join( tl , m , tr ).t );
 ADS_INVARIANT( A::valid( l ) );
 return( std::move( l ) );
 }

template< class T , class A , class Key , class Value >
T Joinable< T , A , Key , Value >::join2( T && l , T && r )
{
 using J = JoinAlgorithms< A >;
 ADS_PRE( ( A::peek( l ) == A::nil() ) || ( A::peek( r ) == A::nil() ) ||
          ( J::lastKey( A::peek( l ) ) < J::firstKey( A::peek( r ) ) ) );
 const typename J::Tree tl = J::whole( A::take( l ) ) ,
                        tr = J::whole( A::take( r ) );
 A::put( l , J::join2( tl , tr ).t );
 ADS_INVARIANT( A::valid( l ) );
 return( std::move( l ) );
 }

template< class T , class A , class Key , class Value >
T Joinable< T , A , Key , Value >::extractRange( const Key & lo ,
                                                 const Key & hi )
{
 using J = JoinAlgorithms< A >;
 T & self = static_cast< T & >( *this );
 T out = A::fresh( self );
 typename J::Tree x = J::whole( A::take( self ) );
 const typename J::Tree c = J::extract( x , lo , hi );
 A::put( self , x.t );
 A::put( out , c.t );
 ADS_INVARIANT( A::valid( self ) && A::valid( out ) );
 return( out );
 }

template< class T , class A , class Key , class Value >
template< Executor E , class F >
void Joinable< T , A , Key , Value >::unionWith( E & ex , T && o , F f )
{
 using J = JoinAlgorithms< A >;
 T & self = static_cast< T & >( *this );
 const typename J::Tree a = J::whole( A::take( self ) ) ,
                        b = J::whole( A::take( o ) );
 A::put( self , J::unite( ex , a , b , f ).t );
 ADS_INVARIANT( A::valid( self ) );
 }

template< class T , class A , class Key , class Value >
template< Executor E , class F >
void Joinable< T , A , Key , Value >::intersectWith( E & ex , T && o , F f )
{
 using J = JoinAlgorithms< A >;
 T & self = static_cast< T & >( *this );
 const typename J::Tree a = J::whole( A::take( self ) ) ,
                        b = J::whole( A::take( o ) );
 A::put( self , J::intersect( ex , a , b , f ).t );
 ADS_INVARIANT( A::valid( self ) );
 }

template< class T , class A , class Key , class Value >
template< Executor E >
void Joinable< T , A , Key , Value >::differenceWith( E & ex , T && o )
{
 using J = JoinAlgorithms< A >;
 T & self = static_cast< T & >( *this );
 const typename J::Tree a = J::whole( A::take( self ) ) ,
                        b = J::whole( A::take( o ) );
 A::put( self , J::subtract( ex , a , b ).t );
 ADS_INVARIANT( A::valid( self ) );
 }

template< class T , class A , class Key , class Value >
template< Executor E , class F >
void Joinable< T , A , Key , Value >::multiInsert( E & ex ,
                         std::span< const std::pair< Key , Value > > B ,
                         F f )
{
 using J = JoinAlgorithms< A >;
 using Node = typename A::Node;
 ADS_AUDIT( std::adjacent_find( B.begin() , B.end() ,
             []( const auto & x , const auto & y ) {
              return( !( x.first < y.first ) ); } ) == B.end() );
 T & self = static_cast< T & >( *this );
 std::vector< Node * > N( B.size() );
 const std::uint64_t salt = A::salt( self );
 parallelFor( ex , 0 , B.size() , [ & ]( std::size_t i ) {
  N[ i ] = A::make( B[ i ].first , B[ i ].second , salt , i );
  } );
 const typename J::Tree x = J::whole( A::take( self ) );
 A::put( self , J::multiInsert( ex , x , N.data() , N.size() , f ).t );
 ADS_INVARIANT( A::valid( self ) );
 }

template< class T , class A , class Key , class Value >
template< Executor E >
void Joinable< T , A , Key , Value >::multiRemove( E & ex ,
                                                  std::span< const Key > B )
{
 using J = JoinAlgorithms< A >;
 ADS_AUDIT( std::adjacent_find( B.begin() , B.end() ,
             []( const Key & x , const Key & y ) {
              return( !( x < y ) ); } ) == B.end() );
 T & self = static_cast< T & >( *this );
 std::size_t gone = 0;
 const typename J::Tree x = J::whole( A::take( self ) );
 A::put( self , J::multiRemove( ex , x , B.data() , B.size() , gone ).t );
 ADS_INVARIANT( A::valid( self ) );
 }

template< class T , class A , class Key , class Value >
template< Executor E , class P >
void Joinable< T , A , Key , Value >::filter( E & ex , P p )
{
 using J = JoinAlgorithms< A >;
 T & self = static_cast< T & >( *this );
 const typename J::Tree x = J::whole( A::take( self ) );
 A::put( self , J::filter( ex , x , p ).t );
 ADS_INVARIANT( A::valid( self ) );
 }

template< class T , class A , class Key , class Value >
template< Executor E , class M , class R , class U >
U Joinable< T , A , Key , Value >::mapReduce( E & ex , M map , R reduce ,
                                              U id ) const
{
 using J = JoinAlgorithms< A >;
 const T & self = static_cast< const T & >( *this );
 return( J::mapReduce( ex , A::peek( self ) , map , reduce , id ) );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::firstKey( Node * t ) -> const Key &
{
 while( A::left( t ) != A::nil() )
  t = A::left( t );
 return( A::key( t ) );
 }

template< JoinAdaptor A >
auto JoinAlgorithms< A >::lastKey( Node * t ) -> const Key &
{
 while( A::right( t ) != A::nil() )
  t = A::right( t );
 return( A::key( t ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File JoinAlgorithms.tpp ----------------------*/
/*--------------------------------------------------------------------------*/

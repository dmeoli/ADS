/*--------------------------------------------------------------------------*/
/*--------------------------- File BTreeMap.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BTreeMap [see BTreeMap.h], with their contracts. The
 * hot ones are inline: a member of a template defined out of its class is
 * not, and the compiler would call it rather than expand it in the loop of
 * the caller.
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

#include <vector>

namespace ADS {

#define ADS_BTM_T template< class K , class V , class Less , std::size_t B , \
                           bool R > \
 requires std::default_initializable< K > && std::default_initializable< V >
#define ADS_BTM BTreeMap< K , V , Less , B , R >

/*--------------------------------------------------------------------------*/
/*------------------------------ THE NODES ---------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
template< class N >
inline N * ADS_BTM::make( void )
{
 N * x;
 if constexpr( std::is_same_v< N , Leaf > )
  x = f_leaves.make();
 else
  x = f_inners.make();
 if constexpr( FAST )
  std::fill( std::begin( x->k ) , std::end( x->k ) , pad() );
 return( x );
 }

ADS_BTM_T
inline void ADS_BTM::free( Leaf * x ) noexcept
{
 f_leaves.free( x );
 }

ADS_BTM_T
inline void ADS_BTM::free( Inner * x ) noexcept
{
 f_inners.free( x );
 }

ADS_BTM_T
inline void ADS_BTM::clean( K * a , std::size_t from , std::size_t to )
{
 for( std::size_t i = from ; i < to ; ++i )
  if constexpr( FAST )
   a[ i ] = pad();
  else
   a[ i ] = K();
 }

ADS_BTM_T
inline std::size_t ADS_BTM::total( const void * x , int h )
{
 if( h == 1 )
  return( static_cast< const Leaf * >( x )->n );
 const Inner * u = static_cast< const Inner * >( x );
 std::size_t t = 0;
 if constexpr( R )
  for( std::uint32_t j = 0 ; j <= u->n ; ++j )
   t += u->cnt[ j ];
 return( t );
 }

ADS_BTM_T
inline void ADS_BTM::recount( Inner * u , int h )
{
 if constexpr( R )
  for( std::uint32_t j = 0 ; j <= u->n ; ++j )
   u->cnt[ j ] = total( u->c[ j ] , h - 1 );
 else {
  ( void ) u;
  ( void ) h;
  }
 }

ADS_BTM_T
inline void ADS_BTM::fetch( const void * x , std::size_t bytes )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 const char * p = static_cast< const char * >( x );
 for( std::size_t o = 0 ; o < bytes ; o += 64 )
  __builtin_prefetch( p + o );
#else
 ( void ) x;
 ( void ) bytes;
#endif
 }

ADS_BTM_T
template< std::size_t C >
inline std::size_t ADS_BTM::place( const K * a , std::uint32_t n ,
                                   const K & x ) const
{
 if constexpr( FAST ) {                     // all the places, padded
  ( void ) n;
  return( detail::countLess< C >( a , x ) );
  }
 else
  return( std::size_t( std::lower_bound( a , a + n , x , f_less ) - a ) );
 }

ADS_BTM_T
inline bool ADS_BTM::equal( const K & a , const K & b ) const
{
 if constexpr( FAST )
  return( a == b );
 else
  return( ! f_less( b , a ) );              // a is not smaller than b
 }

ADS_BTM_T
template< bool Values >
inline auto ADS_BTM::descend( const K & x , const Inner * * up ,
                              std::uint32_t * at ) const -> const Leaf *
{
 const void * u = f_root;
 for( int h = f_height ; h > 1 ; --h ) {
  const Inner * w = static_cast< const Inner * >( u );
  const auto i = std::uint32_t( place< I >( w->k , w->n , x ) );
  u = w->c[ i ];
  fetch( u , h > 2 ? I * sizeof( K ) : Values ? sizeof( Leaf ) :
                                                L * sizeof( K ) );
  if( up && ( h == 2 ) ) {
   *up = w;
   *at = i;
   }
  }
 return( static_cast< const Leaf * >( u ) );
 }

ADS_BTM_T
auto ADS_BTM::edge( bool right ) const -> const Inner *
{
 if( f_height < 2 )
  return( nullptr );
 const Inner * u = static_cast< const Inner * >( f_root );
 for( int h = f_height ; h > 2 ; --h )
  u = static_cast< const Inner * >( u->c[ right ? u->n : 0 ] );
 return( u );
 }

ADS_BTM_T
inline auto ADS_BTM::descend( const K & x , Step * path ) const
 -> const Leaf *
{
 const void * u = f_root;
 for( int h = f_height , d = 0 ; h > 1 ; --h , ++d ) {
  Inner * w = static_cast< Inner * >( const_cast< void * >( u ) );
  const auto i = std::uint32_t( place< I >( w->k , w->n , x ) );
  path[ d ] = Step{ w , i };
  u = w->c[ i ];
  fetch( u , h > 2 ? I * sizeof( K ) : sizeof( Leaf ) );
  }
 return( static_cast< const Leaf * >( u ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CONSTRUCTORS ---------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
ADS_BTM::BTreeMap( Less less ) : f_less( std::move( less ) )
{
 }

ADS_BTM_T
ADS_BTM::BTreeMap( const BTreeMap & o ) : f_less( o.f_less )
{
 build( o.begin() , o.end() );
 }

ADS_BTM_T
ADS_BTM::BTreeMap( BTreeMap && o ) noexcept : f_less( o.f_less )
{
 swap( o );
 }

ADS_BTM_T
auto ADS_BTM::operator=( BTreeMap o ) noexcept -> BTreeMap &
{
 swap( o );
 return( *this );
 }

ADS_BTM_T
ADS_BTM::~BTreeMap()
{
 clear();
 }

ADS_BTM_T
void ADS_BTM::swap( BTreeMap & o ) noexcept
{
 std::swap( f_root , o.f_root );
 std::swap( f_first , o.f_first );
 std::swap( f_last , o.f_last );
 std::swap( f_size , o.f_size );
 std::swap( f_height , o.f_height );
 std::swap( f_less , o.f_less );
 f_leaves.swap( o.f_leaves );
 f_inners.swap( o.f_inners );
 }

ADS_BTM_T
void ADS_BTM::destroy( void * x , int h ) noexcept
{
 if( h == 1 ) {
  free( static_cast< Leaf * >( x ) );
  return;
  }
 Inner * u = static_cast< Inner * >( x );
 for( std::uint32_t i = 0 ; i <= u->n ; ++i )
  destroy( u->c[ i ] , h - 1 );
 free( u );
 }

ADS_BTM_T
void ADS_BTM::clear( void )
{
 if( f_root ) {
  if constexpr( std::is_trivially_destructible_v< K > &&
                std::is_trivially_destructible_v< V > ) {
   ObjectPool< Leaf >().swap( f_leaves );   // the blocks, all at once
   ObjectPool< Inner >().swap( f_inners );
   }
  else
   destroy( f_root , f_height );
  }
 f_root = nullptr;
 f_first = f_last = nullptr;
 f_size = 0;
 f_height = 0;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ SEARCHES ----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
inline const V * ADS_BTM::find( const K & k ) const
{
 if( ! f_root )
  return( nullptr );
 const Leaf * x = descend< true >( k );
 const std::size_t p = place< L >( x->k , x->n , k );
 if( ( p < x->n ) && equal( x->k[ p ] , k ) )
  return( &x->v[ p ] );
 return( nullptr );
 }

ADS_BTM_T
inline V * ADS_BTM::find( const K & k )
{
 return( const_cast< V * >( std::as_const( *this ).find( k ) ) );
 }

ADS_BTM_T
inline std::optional< V > ADS_BTM::lookup( const K & k ) const
{
 const V * v = find( k );
 return( v ? std::optional< V >( *v ) : std::nullopt );
 }

ADS_BTM_T
inline bool ADS_BTM::contains( const K & k ) const
{
 return( find( k ) != nullptr );
 }

ADS_BTM_T
std::size_t ADS_BTM::size( void ) const
{
 return( f_size );
 }

ADS_BTM_T
bool ADS_BTM::empty( void ) const
{
 return( f_size == 0 );
 }

ADS_BTM_T
int ADS_BTM::height( void ) const
{
 return( f_height );
 }

ADS_BTM_T
auto ADS_BTM::begin( void ) const -> const_iterator
{
 return( const_iterator( this , f_first , 0 , edge( false ) , 0 ) );
 }

ADS_BTM_T
auto ADS_BTM::end( void ) const -> const_iterator
{
 return( const_iterator( this , nullptr , 0 , nullptr , 0 ) );
 }

ADS_BTM_T
inline auto ADS_BTM::lowerBound( const K & k ) const -> const_iterator
{
 if( ! f_root )
  return( end() );
 const Inner * u = nullptr;
 std::uint32_t c = 0;
 const Leaf * x = descend< false >( k , &u , &c );
 return( const_iterator( this , x ,
                         std::uint32_t( place< L >( x->k , x->n , k ) ) ,
                         u , c ) );
 }

ADS_BTM_T
inline auto ADS_BTM::upperBound( const K & k ) const -> const_iterator
{
 if( ! f_root )
  return( end() );
 const Inner * u = nullptr;
 std::uint32_t c = 0;
 const Leaf * x = descend< false >( k , &u , &c );   // where k would be
 std::size_t p;
 if constexpr( FAST ) {
  p = detail::countLess< L , true >( x->k , k );
  p = std::min< std::size_t >( p , x->n );  // k may be the padding
  }
 else
  p = std::size_t( std::upper_bound( x->k , x->k + x->n , k , f_less ) -
                   x->k );
 return( const_iterator( this , x , std::uint32_t( p ) , u , c ) );
 }

ADS_BTM_T
template< class F >
void ADS_BTM::forEach( F && f ) const
{
 for( const_iterator it = begin() ; it.f_l ; ++it )
  f( it.f_l->k[ it.f_i ] , it.f_l->v[ it.f_i ] );
 }

ADS_BTM_T
template< class F >
void ADS_BTM::forRange( const K & lo , const K & hi , F && f ) const
{
 for( const_iterator it = lowerBound( lo ) ; it.f_l ; ++it ) {
  if( ! f_less( it.f_l->k[ it.f_i ] , hi ) )
   return;
  f( it.f_l->k[ it.f_i ] , it.f_l->v[ it.f_i ] );
  }
 }

ADS_BTM_T
std::size_t ADS_BTM::rank( const K & k ) const requires R
{
 std::size_t r = 0;
 if( ! f_root )
  return( 0 );
 const void * x = f_root;
 for( int h = f_height ; h > 1 ; --h ) {    // the children before the way
  const Inner * u = static_cast< const Inner * >( x );
  const auto i = std::uint32_t( place< I >( u->k , u->n , k ) );
  for( std::uint32_t j = 0 ; j < i ; ++j )
   r += u->cnt[ j ];
  x = u->c[ i ];
  }
 const Leaf * l = static_cast< const Leaf * >( x );
 r += place< L >( l->k , l->n , k );
 ADS_POST( r <= f_size );
 return( r );
 }

ADS_BTM_T
auto ADS_BTM::select( std::size_t i ) const -> const_iterator requires R
{
 if( i >= f_size )
  return( end() );
 const void * x = f_root;
 const Inner * up = nullptr;
 std::uint32_t at = 0;
 for( int h = f_height ; h > 1 ; --h ) {    // the child that holds rank i
  const Inner * u = static_cast< const Inner * >( x );
  std::uint32_t j = 0;
  while( i >= u->cnt[ j ] ) {
   i -= u->cnt[ j ];
   ++j;
   }
  if( h == 2 ) {
   up = u;
   at = j;
   }
  x = u->c[ j ];
  }
 return( const_iterator( this , static_cast< const Leaf * >( x ) ,
                         std::uint32_t( i ) , up , at ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- INSERTION ----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
bool ADS_BTM::insert( const K & k , const V & v )
{
 if( ! f_root ) {
  Leaf * x = make< Leaf >();
  x->k[ 0 ] = k;
  x->v[ 0 ] = v;
  x->n = 1;
  f_root = f_first = f_last = x;
  f_height = 1;
  f_size = 1;
  ADS_POST( contains( k ) );
  return( true );
  }
 if constexpr( ! R )                        // past the largest key, in a
  if( ( f_last->n < L ) &&                  // leaf with room: no descent,
      f_less( f_last->k[ f_last->n - 1 ] , k ) ) {   // no bound to change
   f_last->k[ f_last->n ] = k;
   f_last->v[ f_last->n ] = v;
   ++f_last->n;
   ++f_size;
   ADS_POST( contains( k ) );
   ADS_INVARIANT( ( f_size > 4096 ) || valid() );
   return( true );
   }
 Step path[ DEPTH ];
 Leaf * x = const_cast< Leaf * >( descend( k , path ) );
 const auto p = std::uint32_t( place< L >( x->k , x->n , k ) );
 if( ( p < x->n ) && equal( x->k[ p ] , k ) ) {
  x->v[ p ] = v;
  return( false );
  }
 if constexpr( R )                          // one pair more below the way
  for( int d = 0 ; d + 1 < f_height ; ++d )
   ++path[ d ].u->cnt[ path[ d ].i ];
 if( x->n < L ) [[likely]] {
  if constexpr( std::is_trivially_copyable_v< V > ) {
   const V w = v;                           // v may live in x
   std::move_backward( x->k + p , x->k + x->n , x->k + x->n + 1 );
   std::move_backward( x->v + p , x->v + x->n , x->v + x->n + 1 );
   x->k[ p ] = k;
   x->v[ p ] = w;
   }
  else {
   const bool in = ( &v >= x->v ) && ( &v < x->v + L );
   V w;
   if( in )
    w = v;
   std::move_backward( x->k + p , x->k + x->n , x->k + x->n + 1 );
   std::move_backward( x->v + p , x->v + x->n , x->v + x->n + 1 );
   x->k[ p ] = k;
   x->v[ p ] = in ? std::move( w ) : v;
   }
  ++x->n;
  }
 else
  split( x , p , k , v , path , f_height - 2 );
 ++f_size;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( ( f_size > 4096 ) || valid() );
 return( true );
 }

ADS_BTM_T
void ADS_BTM::split( Leaf * x , std::uint32_t p , const K & k , const V & v ,
                     Step * path , int d )
{
 V w = v;                                   // v may live in x
 if( ( p == L ) && ! x->next ) {            // past the largest key
  Leaf * r = make< Leaf >();
  r->k[ 0 ] = k;
  r->v[ 0 ] = std::move( w );
  r->n = 1;
  r->prev = x;
  x->next = r;
  f_last = r;
  climb( x->k[ L - 1 ] , r , path , d , 1 );
  return;
  }
 if( ( p == 0 ) && ! x->prev ) {            // before the smallest key
  Leaf * l = make< Leaf >();
  l->k[ 0 ] = k;
  l->v[ 0 ] = std::move( w );
  l->n = 1;
  l->next = x;
  x->prev = l;
  f_first = l;
  climb( k , l , path , d , -1 );
  return;
  }
 Leaf * r = make< Leaf >();                 // L + 1 pairs, m of them in x
 constexpr std::uint32_t m = ( L + 1 ) / 2;
 if( p < m ) {
  std::move( x->k + m - 1 , x->k + L , r->k );
  std::move( x->v + m - 1 , x->v + L , r->v );
  r->n = L - m + 1;
  std::move_backward( x->k + p , x->k + m - 1 , x->k + m );
  std::move_backward( x->v + p , x->v + m - 1 , x->v + m );
  x->k[ p ] = k;
  x->v[ p ] = std::move( w );
  }
 else {
  std::move( x->k + m , x->k + L , r->k );
  std::move( x->v + m , x->v + L , r->v );
  const std::uint32_t q = p - m;
  std::move_backward( r->k + q , r->k + L - m , r->k + L - m + 1 );
  std::move_backward( r->v + q , r->v + L - m , r->v + L - m + 1 );
  r->k[ q ] = k;
  r->v[ q ] = std::move( w );
  r->n = L - m + 1;
  }
 x->n = m;
 clean( x->k , m , L );
 r->next = x->next;
 r->prev = x;
 if( x->next )
  x->next->prev = r;
 else
  f_last = r;
 x->next = r;
 climb( x->k[ m - 1 ] , r , path , d , 0 );
 }

ADS_BTM_T
void ADS_BTM::climb( K s , void * r , Step * path , int d , int edge )
{
 // the child c[ i ] of path[ d ] split, s is the bound of the left part
 // and r the new node, on the right of c[ i ] or, on the left edge of the
 // tree, on its left
 bool before = ( edge < 0 );
 for( ; ; --d ) {
  if( d < 0 ) {                             // a new root
   Inner * u = make< Inner >();
   u->k[ 0 ] = std::move( s );
   u->c[ 0 ] = before ? r : f_root;
   u->c[ 1 ] = before ? f_root : r;
   u->n = 1;
   recount( u , f_height + 1 );
   f_root = u;
   ++f_height;
   return;
   }
  Inner * u = path[ d ].u;
  const std::uint32_t i = path[ d ].i;
  if( u->n < I ) [[likely]] {
   std::move_backward( u->k + i , u->k + u->n , u->k + u->n + 1 );
   std::move_backward( u->c + i + 1 , u->c + u->n + 1 , u->c + u->n + 2 );
   u->k[ i ] = std::move( s );
   u->c[ i + 1 ] = r;
   if( before )
    std::swap( u->c[ i ] , u->c[ i + 1 ] );
   ++u->n;
   recount( u , f_height - d );
   return;
   }
  K t[ I + 1 ];                             // I + 1 keys, I + 2 children
  void * c[ I + 2 ];
  std::move( u->k , u->k + i , t );
  t[ i ] = std::move( s );
  std::move( u->k + i , u->k + I , t + i + 1 );
  std::copy( u->c , u->c + i + 1 , c );
  c[ i + 1 ] = r;
  std::copy( u->c + i + 1 , u->c + I + 1 , c + i + 2 );
  if( before )
   std::swap( c[ i ] , c[ i + 1 ] );
  // u keeps m keys, t[ m ] goes up, the new node on its right takes the
  // I - m others; on the edges of the tree u stays almost full, or almost
  // empty, so that the keys inserted in order fill the nodes
  const std::uint32_t m = edge > 0 ? std::uint32_t( I - 1 ) :
                          edge < 0 ? 1 : std::uint32_t( ( I + 1 ) / 2 );
  Inner * w = make< Inner >();
  w->prev = u;                              // w after u on its level
  w->next = u->next;
  if( u->next )
   u->next->prev = w;
  u->next = w;
  std::move( t + m + 1 , t + I + 1 , w->k );
  std::copy( c + m + 1 , c + I + 2 , w->c );
  w->n = std::uint32_t( I - m );
  std::move( t , t + m , u->k );
  std::copy( c , c + m + 1 , u->c );
  u->n = m;
  clean( u->k , m , I );
  recount( u , f_height - d );
  recount( w , f_height - d );
  s = std::move( t[ m ] );
  r = w;
  before = false;
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ REMOVAL -----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
bool ADS_BTM::remove( const K & k )
{
 if( ! f_root )
  return( false );
 Step path[ DEPTH ];
 Leaf * x = const_cast< Leaf * >( descend( k , path ) );
 const auto p = std::uint32_t( place< L >( x->k , x->n , k ) );
 if( ( p >= x->n ) || ! equal( x->k[ p ] , k ) )
  return( false );
 if constexpr( R )                          // one pair less below the way
  for( int d = 0 ; d + 1 < f_height ; ++d )
   --path[ d ].u->cnt[ path[ d ].i ];
 std::move( x->k + p + 1 , x->k + x->n , x->k + p );
 std::move( x->v + p + 1 , x->v + x->n , x->v + p );
 --x->n;
 clean( x->k , x->n , x->n + 1 );
 x->v[ x->n ] = V();
 --f_size;
 if( f_height == 1 ) {
  if( x->n == 0 ) {
   free( x );
   f_root = nullptr;
   f_first = f_last = nullptr;
   f_height = 0;
   }
  }
 else if( x->n < L / 2 )
  shrink( x , path , f_height - 2 );
 ADS_POST( f_root == nullptr || ! contains( k ) );
 ADS_INVARIANT( ( f_size > 4096 ) || valid() );
 return( true );
 }

ADS_BTM_T
void ADS_BTM::shrink( Leaf * x , Step * path , int d )
{
 ( void ) x;
 Inner * u = path[ d ].u;                   // at least 1 key
 const std::uint32_t i = path[ d ].i;
 const std::uint32_t a = i < u->n ? i : i - 1;   // the left of the two
 Leaf * l = static_cast< Leaf * >( u->c[ a ] );
 Leaf * r = static_cast< Leaf * >( u->c[ a + 1 ] );
 if( l->n + r->n <= L ) {                   // r into l
  std::move( r->k , r->k + r->n , l->k + l->n );
  std::move( r->v , r->v + r->n , l->v + l->n );
  l->n += r->n;
  l->next = r->next;
  if( r->next )
   r->next->prev = l;
  else
   f_last = l;
  free( r );
  std::move( u->k + a + 1 , u->k + u->n , u->k + a );
  std::move( u->c + a + 2 , u->c + u->n + 1 , u->c + a + 1 );
  --u->n;
  clean( u->k , u->n , u->n + 1 );
  recount( u , f_height - d );
  if( d == 0 ) {
   if( u->n == 0 ) {                        // the root gives way
    f_root = u->c[ 0 ];
    --f_height;
    free( u );
    }
   }
  else if( u->n < I / 2 )
   shrink( u , path , d - 1 );
  return;
  }
 const std::uint32_t h = ( l->n + r->n ) / 2;    // l keeps h
 if( l->n < h ) {                           // from r to l
  const std::uint32_t t = h - l->n;
  std::move( r->k , r->k + t , l->k + l->n );
  std::move( r->v , r->v + t , l->v + l->n );
  std::move( r->k + t , r->k + r->n , r->k );
  std::move( r->v + t , r->v + r->n , r->v );
  clean( r->k , r->n - t , r->n );
  for( std::uint32_t j = r->n - t ; j < r->n ; ++j )
   r->v[ j ] = V();
  l->n += t;
  r->n -= t;
  }
 else {                                     // from l to r
  const std::uint32_t t = l->n - h;
  std::move_backward( r->k , r->k + r->n , r->k + r->n + t );
  std::move_backward( r->v , r->v + r->n , r->v + r->n + t );
  std::move( l->k + h , l->k + l->n , r->k );
  std::move( l->v + h , l->v + l->n , r->v );
  clean( l->k , h , l->n );
  for( std::uint32_t j = h ; j < l->n ; ++j )
   l->v[ j ] = V();
  l->n = h;
  r->n += t;
  }
 u->k[ a ] = l->k[ l->n - 1 ];
 recount( u , f_height - d );
 }

ADS_BTM_T
void ADS_BTM::shrink( Inner * x , Step * path , int d )
{
 ( void ) x;
 Inner * u = path[ d ].u;                   // at least 1 key
 const std::uint32_t i = path[ d ].i;
 const std::uint32_t a = i < u->n ? i : i - 1;
 Inner * l = static_cast< Inner * >( u->c[ a ] );
 Inner * r = static_cast< Inner * >( u->c[ a + 1 ] );
 if( l->n + 1 + r->n <= I ) {               // r and the bound into l
  l->k[ l->n ] = std::move( u->k[ a ] );
  std::move( r->k , r->k + r->n , l->k + l->n + 1 );
  std::copy( r->c , r->c + r->n + 1 , l->c + l->n + 1 );
  l->n += 1 + r->n;
  l->next = r->next;
  if( r->next )
   r->next->prev = l;
  free( r );
  recount( l , f_height - d - 1 );
  std::move( u->k + a + 1 , u->k + u->n , u->k + a );
  std::move( u->c + a + 2 , u->c + u->n + 1 , u->c + a + 1 );
  --u->n;
  clean( u->k , u->n , u->n + 1 );
  recount( u , f_height - d );
  if( d == 0 ) {
   if( u->n == 0 ) {
    f_root = u->c[ 0 ];
    --f_height;
    free( u );
    }
   }
  else if( u->n < I / 2 )
   shrink( u , path , d - 1 );
  return;
  }
 const std::uint32_t h = ( l->n + r->n ) / 2;    // l keeps h keys
 if( l->n < h ) {                           // through the bound, from r
  const std::uint32_t t = h - l->n;
  l->k[ l->n ] = std::move( u->k[ a ] );
  std::move( r->k , r->k + t - 1 , l->k + l->n + 1 );
  std::copy( r->c , r->c + t , l->c + l->n + 1 );
  u->k[ a ] = std::move( r->k[ t - 1 ] );
  std::move( r->k + t , r->k + r->n , r->k );
  std::copy( r->c + t , r->c + r->n + 1 , r->c );
  clean( r->k , r->n - t , r->n );
  l->n += t;
  r->n -= t;
  }
 else {                                     // through the bound, from l
  const std::uint32_t t = l->n - h;
  std::move_backward( r->k , r->k + r->n , r->k + r->n + t );
  std::copy_backward( r->c , r->c + r->n + 1 , r->c + r->n + 1 + t );
  r->k[ t - 1 ] = std::move( u->k[ a ] );
  std::move( l->k + h + 1 , l->k + l->n , r->k );
  std::copy( l->c + h + 1 , l->c + l->n + 1 , r->c );
  u->k[ a ] = std::move( l->k[ h ] );
  clean( l->k , h , l->n );
  l->n = h;
  r->n += t;
  }
 recount( l , f_height - d - 1 );
 recount( r , f_height - d - 1 );
 recount( u , f_height - d );
 }

ADS_BTM_T
auto ADS_BTM::erase( const_iterator it ) -> const_iterator
{
 const K k = ( *it ).first;
 remove( k );
 return( lowerBound( k ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BUILD ------------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
template< std::input_iterator It >
void ADS_BTM::build( It first , It last )
{
 clear();
 std::vector< std::pair< K , V > > P;       // a single pass on the input
 for( ; first != last ; ++first ) {
  auto && [ k , v ] = *first;
  P.emplace_back( k , v );
  }
 ADS_AUDIT( std::adjacent_find( P.begin() , P.end() ,
                                [ & ]( const auto & a , const auto & b ) {
                                 return( ! f_less( a.first , b.first ) );
                                 } ) == P.end() );
 const std::size_t n = P.size();
 if( n == 0 )
  return;
 // the leaves, as full and as even as they can be
 std::vector< void * > level;
 std::vector< K > top;                      // the largest key below each
 std::size_t g = ( n + L - 1 ) / L;
 Leaf * prev = nullptr;
 for( std::size_t j = 0 , s = 0 ; j < g ; ++j ) {
  const std::size_t c = n / g + ( j < n % g ? 1 : 0 );
  Leaf * x = make< Leaf >();
  for( std::size_t q = 0 ; q < c ; ++q ) {
   x->k[ q ] = std::move( P[ s + q ].first );
   x->v[ q ] = std::move( P[ s + q ].second );
   }
  x->n = std::uint32_t( c );
  s += c;
  x->prev = prev;
  if( prev )
   prev->next = x;
  else
   f_first = x;
  prev = x;
  level.push_back( x );
  top.push_back( x->k[ c - 1 ] );
  }
 f_last = prev;
 f_height = 1;
 while( level.size() > 1 ) {                // a level of internal nodes
  const std::size_t m = level.size();
  g = ( m + I ) / ( I + 1 );
  std::vector< void * > up;
  std::vector< K > ut;
  for( std::size_t j = 0 , s = 0 ; j < g ; ++j ) {
   const std::size_t c = m / g + ( j < m % g ? 1 : 0 );   // children
   Inner * u = make< Inner >();
   for( std::size_t q = 0 ; q < c ; ++q ) {
    u->c[ q ] = level[ s + q ];
    if( q + 1 < c )
     u->k[ q ] = top[ s + q ];
    }
   u->n = std::uint32_t( c - 1 );
   recount( u , f_height + 1 );
   if( ! up.empty() ) {
    u->prev = static_cast< Inner * >( up.back() );
    u->prev->next = u;
    }
   up.push_back( u );
   ut.push_back( top[ s + c - 1 ] );
   s += c;
   }
  level.swap( up );
  top.swap( ut );
  ++f_height;
  }
 f_root = level[ 0 ];
 f_size = n;
 ADS_INVARIANT( ( f_size > 4096 ) || valid() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ INVARIANT ---------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_BTM_T
bool ADS_BTM::valid( void ) const
{
 if( ! f_root )
  return( ( f_height == 0 ) && ( f_size == 0 ) && ! f_first && ! f_last );
 const Leaf * last = nullptr;
 std::vector< const Inner * > left( std::size_t( f_height ) + 1 , nullptr );
 std::size_t count = 0;
 if( ! valid( f_root , f_height , nullptr , nullptr , last , left ,
              count ) )
  return( false );
 for( const Inner * u : left )
  if( u && u->next )
   return( false );
 return( ( count == f_size ) && ( last == f_last ) && ! last->next &&
         ( f_first->prev == nullptr ) );
 }

ADS_BTM_T
bool ADS_BTM::valid( const void * x , int h , const K * lo , const K * hi ,
                     const Leaf * & last ,
                     std::vector< const Inner * > & left ,
                     std::size_t & count ) const
{
 auto inside = [ & ]( const K & k ) {       // in ( lo , hi ]
  return( ( ! lo || f_less( *lo , k ) ) && ( ! hi || ! f_less( *hi , k ) ) );
  };
 if( h == 1 ) {
  const Leaf * l = static_cast< const Leaf * >( x );
  if( ( l->n > L ) || ( l->n == 0 ) )
   return( false );
  for( std::uint32_t i = 0 ; i < l->n ; ++i )
   if( ! inside( l->k[ i ] ) ||
       ( ( i > 0 ) && ! f_less( l->k[ i - 1 ] , l->k[ i ] ) ) )
    return( false );
  if constexpr( FAST )
   for( std::uint32_t i = l->n ; i < L ; ++i )
    if( !( l->k[ i ] == pad() ) )
     return( false );
  if( ( l->prev != last ) || ( last ? last->next != l : f_first != l ) )
   return( false );
  last = l;
  count += l->n;
  return( true );
  }
 const Inner * u = static_cast< const Inner * >( x );
 if( ( u->n > I ) || ( u->n == 0 ) || ( u->prev != left[ h ] ) ||
     ( left[ h ] && ( left[ h ]->next != u ) ) )
  return( false );
 left[ h ] = u;
 for( std::uint32_t i = 0 ; i < u->n ; ++i )
  if( ! inside( u->k[ i ] ) ||
      ( ( i > 0 ) && ! f_less( u->k[ i - 1 ] , u->k[ i ] ) ) )
   return( false );
 if constexpr( FAST )
  for( std::uint32_t i = u->n ; i < I ; ++i )
   if( !( u->k[ i ] == pad() ) )
    return( false );
 for( std::uint32_t i = 0 ; i <= u->n ; ++i ) {
  const std::size_t before = count;
  if( ! valid( u->c[ i ] , h - 1 , i > 0 ? &u->k[ i - 1 ] : lo ,
               i < u->n ? &u->k[ i ] : hi , last , left , count ) )
   return( false );
  if constexpr( R )                         // the counts of the children
   if( u->cnt[ i ] != count - before )
    return( false );
  }
 return( true );
 }

#undef ADS_BTM
#undef ADS_BTM_T

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File BTreeMap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/

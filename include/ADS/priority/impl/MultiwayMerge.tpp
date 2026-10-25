/*--------------------------------------------------------------------------*/
/*------------------------ File MultiwayMerge.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of MultiwayMerge.h, with their contracts.
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

template< class T , class C >
FlatLoserTree< T , C >::FlatLoserTree( std::size_t k , C less )
 : f_k( k ) , f_K( 1 ) , f_less( less )
{
 ADS_PRE( ( k >= 1 ) && ( k < kSup ) );
 while( f_K < k ) f_K *= 2;
 f_leaf.resize( f_K );
 for( std::size_t s = 0 ; s < f_K ; ++s ) {
  if constexpr( kByValue ) f_leaf[ s ].key = supKey();
  else f_leaf[ s ].key = nullptr;
  f_leaf[ s ].src = kSup | std::uint32_t( s );
  }
 f_node.assign( f_K , f_leaf[ 0 ] );
 build();
 }

template< class T , class C >
std::size_t FlatLoserTree< T , C >::sources( void ) const
{
 return( f_k );
 }

template< class T , class C >
const T & FlatLoserTree< T , C >::keyOf( const Node & a )
{
 if constexpr( kByValue ) return( a.key );
 else return( *a.key );
 }

template< class T , class C >
T FlatLoserTree< T , C >::supKey( void )
{
 if constexpr( kNatural ) {
  using L = std::numeric_limits< T >;
  if constexpr( L::has_infinity ) return( kUp ? L::infinity() :
                                                -L::infinity() );
  else return( kUp ? L::max() : L::lowest() );
  }
 else return( T() );
 }

template< class T , class C >
bool FlatLoserTree< T , C >::beats( const Node & a , const Node & b ) const
{
#if defined( __SIZEOF_INT128__ )
 if constexpr( kInteger ) {                 // ( key , source ) on 128 bits
  using W = std::conditional_t< std::is_signed_v< T > , __int128 ,
                                unsigned __int128 >;
  const W ka = W( a.key ) , kb = W( b.key );
  if constexpr( kUp )
   return( ( ( ka << 64 ) | W( a.src ) ) < ( ( kb << 64 ) | W( b.src ) ) );
  else
   return( ( ( kb << 64 ) | W( a.src ) ) < ( ( ka << 64 ) | W( b.src ) ) );
  }
 else
#endif
 if constexpr( kNatural ) {                 // the exhausted ones lose by key
  const bool lt = f_less( a.key , b.key ) , gt = f_less( b.key , a.key );
  return( lt | ( ( ! gt ) & ( a.src < b.src ) ) );
  }
 else {
  if( ( a.src | b.src ) & kSup ) [[unlikely]] return( a.src < b.src );
  const bool lt = f_less( keyOf( a ) , keyOf( b ) ) ,
             gt = f_less( keyOf( b ) , keyOf( a ) );
  return( lt | ( ( ! gt ) & ( a.src < b.src ) ) );
  }
 }

template< class T , class C >
void FlatLoserTree< T , C >::start( std::size_t s , const T & x )
{
 ADS_PRE( s < f_k );
 if constexpr( kByValue ) f_leaf[ s ].key = x;
 else f_leaf[ s ].key = &x;
 f_leaf[ s ].src = std::uint32_t( s );
 }

template< class T , class C >
void FlatLoserTree< T , C >::build( void )
{
 if( f_K == 1 ) {
  f_node[ 0 ] = f_leaf[ 0 ];
  return;
  }
 std::vector< Node > win( f_K );            // the winner below each node
 for( std::size_t v = f_K - 1 ; v >= 1 ; --v ) {
  const Node & a = 2 * v >= f_K ? f_leaf[ 2 * v - f_K ] : win[ 2 * v ];
  const Node & b = 2 * v + 1 >= f_K ? f_leaf[ 2 * v + 1 - f_K ] :
                                       win[ 2 * v + 1 ];
  if( beats( b , a ) ) {
   win[ v ] = b;
   f_node[ v ] = a;
   }
  else {
   win[ v ] = a;
   f_node[ v ] = b;
   }
  }
 f_node[ 0 ] = win[ 1 ];
 ADS_INVARIANT( detail::contract( [ & ]( void ) {
  return( winnerIsBest() ); } ) );
 }

template< class T , class C >
bool FlatLoserTree< T , C >::empty( void ) const
{
 return( f_node[ 0 ].src & kSup );
 }

template< class T , class C >
std::size_t FlatLoserTree< T , C >::winner( void ) const
{
 ADS_PRE( ! empty() );
 return( f_node[ 0 ].src );
 }

template< class T , class C >
const T & FlatLoserTree< T , C >::top( void ) const
{
 ADS_PRE( ! empty() );
 return( keyOf( f_node[ 0 ] ) );
 }

template< class T , class C >
void FlatLoserTree< T , C >::replay( Node cur )
{
 for( std::size_t v = ( f_K + ( cur.src & ~kSup ) ) >> 1 ; v > 0 ;
      v >>= 1 ) {
  Node & nd = f_node[ v ];
  if constexpr( std::is_trivially_copyable_v< Key > &&
                ( sizeof( Key ) <= 16 ) ) {  // masks, no jump
   const std::uint64_t m = std::uint64_t( 0 ) -
                           std::uint64_t( beats( nd , cur ) );
   std::uint64_t kn[ 2 ] = { 0 , 0 } , kc[ 2 ] = { 0 , 0 };
   std::memcpy( kn , &nd.key , sizeof( Key ) );
   std::memcpy( kc , &cur.key , sizeof( Key ) );
   for( int w = 0 ; w < ( sizeof( Key ) > 8 ? 2 : 1 ) ; ++w ) {
    const std::uint64_t d = ( kn[ w ] ^ kc[ w ] ) & m;
    kn[ w ] ^= d;
    kc[ w ] ^= d;
    }
   std::memcpy( &nd.key , kn , sizeof( Key ) );
   std::memcpy( &cur.key , kc , sizeof( Key ) );
   const std::uint32_t ds = ( nd.src ^ cur.src ) & std::uint32_t( m );
   nd.src ^= ds;
   cur.src ^= ds;
   }
  else if( beats( nd , cur ) ) std::swap( nd , cur );
  }
 f_node[ 0 ] = cur;
 ADS_INVARIANT( detail::contract( [ & ]( void ) {
  return( winnerIsBest() ); } ) );
 }

template< class T , class C >
void FlatLoserTree< T , C >::replaceTop( const T & x )
{
 ADS_PRE( ! empty() );
 Node cur;
 if constexpr( kByValue ) cur.key = x;
 else cur.key = &x;
 cur.src = f_node[ 0 ].src;
 replay( cur );
 }

template< class T , class C >
void FlatLoserTree< T , C >::exhaustTop( void )
{
 ADS_PRE( ! empty() );
 Node cur;
 if constexpr( kByValue ) cur.key = supKey();
 else cur.key = nullptr;
 cur.src = f_node[ 0 ].src | kSup;
 replay( cur );
 }

template< class T , class C >
bool FlatLoserTree< T , C >::winnerIsBest( void ) const
{
 for( std::size_t v = 1 ; v < f_K ; ++v )
  if( beats( f_node[ v ] , f_node[ 0 ] ) ) return( false );
 return( true );
 }

namespace detail {

/// the bytes of the buffer of a merger of the funnel
inline constexpr std::size_t kFunnelBuffer = 4096;

/// b if c else a, by a mask, which a compiler does not turn into a jump
template< class P >
inline P * pick( bool c , P * a , P * b )
{
 const auto x = reinterpret_cast< std::uintptr_t >( a ) ,
            y = reinterpret_cast< std::uintptr_t >( b );
 return( reinterpret_cast< P * >( x ^ ( ( x ^ y ) &
                                         ( std::uintptr_t( 0 ) -
                                           std::uintptr_t( c ) ) ) ) );
 }

/// the two sorted ranges merged into out, the first one first on ties
template< class I , class O , class C >
O mergeTwo( I a , I ae , I b , I be , O out , C less )
{
 if constexpr( kNaturalOrder< std::iter_value_t< I > , C > &&
               std::random_access_iterator< I > ) {
  while( ( a != ae ) && ( b != be ) ) {     // no jump on the order
   const bool tb = less( *b , *a );
   *out = *pick( tb , std::to_address( a ) , std::to_address( b ) );
   ++out;
   b += tb;
   a += ! tb;
   }
  out = std::copy( a , ae , out );
  return( std::copy( b , be , out ) );
  }
 else return( std::merge( a , ae , b , be , out , less ) );
 }

/// the 3 or 4 ranges [ a[ i ] , e[ i ) ) merged into o, the lower range
/// first on ties: at each step the least head of each pair and the least
/// of the two, all computed with no jump, over stretches as long as the
/// shortest range, so that no step checks an end; a range that runs out
/// is dropped and the others merged on
template< class T , class C >
T * mergeFew( const T ** a , const T ** e , std::size_t k , T * o ,
              C less )
{
 while( k > 2 ) {
  std::size_t m = std::size_t( e[ 0 ] - a[ 0 ] );
  for( std::size_t i = 1 ; i < k ; ++i )
   m = std::min( m , std::size_t( e[ i ] - a[ i ] ) );
  const T * p0 = a[ 0 ] , * p1 = a[ 1 ] , * p2 = a[ 2 ] ,
          * p3 = k > 3 ? a[ 3 ] : nullptr;
  if( k == 4 )
   for( std::size_t i = 0 ; i < m ; ++i ) {
    const T * u = pick( less( *p1 , *p0 ) , p0 , p1 );
    const T * v = pick( less( *p3 , *p2 ) , p2 , p3 );
    const T * w = pick( less( *v , *u ) , u , v );
    *o++ = *w;
    p0 += w == p0;
    p1 += w == p1;
    p2 += w == p2;
    p3 += w == p3;
    }
  else
   for( std::size_t i = 0 ; i < m ; ++i ) {
    const T * u = pick( less( *p1 , *p0 ) , p0 , p1 );
    const T * w = pick( less( *p2 , *u ) , u , p2 );
    *o++ = *w;
    p0 += w == p0;
    p1 += w == p1;
    p2 += w == p2;
    }
  a[ 0 ] = p0;
  a[ 1 ] = p1;
  a[ 2 ] = p2;
  if( k > 3 ) a[ 3 ] = p3;
  std::size_t j = 0;                        // the ranges left, in order
  for( std::size_t i = 0 ; i < k ; ++i )
   if( a[ i ] != e[ i ] ) {
    a[ j ] = a[ i ];
    e[ j ] = e[ i ];
    ++j;
    }
  k = j;
  }
 if( k == 2 ) return( mergeTwo( a[ 0 ] , e[ 0 ] , a[ 1 ] , e[ 1 ] , o ,
                                less ) );
 if( k == 1 ) return( std::copy( a[ 0 ] , e[ 0 ] , o ) );
 return( o );
 }

/// a tree of binary mergers with a buffer at each internal node, filled
/// lazily: a node merges its two children into its buffer when the buffer
/// runs out, a child being refilled when it runs out in turn; a stretch of
/// a merge is as long as the least of the room left in its three ranges,
/// so that its steps check no end. The buffers hold the items, or their
/// addresses if kIndirect, the root writing the items themselves, so that
/// an item is copied once and only addresses climb the tree.
template< class T , class C , bool kIndirect >
class MergeFunnel {

 public:

 /// the items in the buffers: the items or their addresses
 using E = std::conditional_t< kIndirect , const T * , T >;

 /// the sources [ first , last ) of R, which must be contiguous; buffers
 /// of b items
 template< class I >
 MergeFunnel( const std::vector< std::pair< I , I > > & R , std::size_t b ,
              C less )
  : f_b( b ) , f_less( less ) {
  f_node.reserve( 2 * R.size() );
  build( R , 0 , R.size() );
  std::size_t bufs = 0;
  for( const auto & v : f_node )
   bufs += ( v.left >= 0 ) || kIndirect ? 1 : 0;
  f_space.resize( bufs * b );
  std::size_t used = 0;
  for( auto & v : f_node )
   if( ( v.left >= 0 ) || kIndirect ) {
    v.buf = f_space.data() + used;
    if( v.left >= 0 ) v.cur = v.end = v.buf;
    else v.end = v.cur = v.buf;             // the addresses not made yet
    used += b;
    }
  }

 /// the n items of the sources, written to o; the end of the output
 T * run( T * o , std::size_t n ) {
  return( o + fill( f_node.back() , o , n ) );
  }

 private:

 /// a node: a source (left < 0) or a merger of two children
 struct Node {
  const E * cur = nullptr;                  // its items not yet taken
  const E * end = nullptr;
  E * buf = nullptr;                        // its buffer
  const T * src = nullptr;                  // the items of a source
  const T * srcEnd = nullptr;
  std::ptrdiff_t left = -1 , right = -1;    // the children of a merger
  bool done = false;                        // no item left below
  };

 /// the node of the sources lo , ... , hi - 1, the last one made
 template< class I >
 std::ptrdiff_t build( const std::vector< std::pair< I , I > > & R ,
                       std::size_t lo , std::size_t hi ) {
  Node v;
  if( hi - lo == 1 ) {
   v.src = std::to_address( R[ lo ].first );
   v.srcEnd = v.src + ( R[ lo ].second - R[ lo ].first );
   if constexpr( ! kIndirect ) {
    v.cur = v.src;
    v.end = v.srcEnd;
    }
   }
  else {
   const std::size_t mid = lo + ( hi - lo ) / 2;
   v.left = build( R , lo , mid );
   v.right = build( R , mid , hi );
   }
  f_node.push_back( v );
  return( std::ptrdiff_t( f_node.size() - 1 ) );
  }

 /// the node c has items, refilled if needed; false if exhausted
 bool ready( Node & c ) {
  if( c.cur != c.end ) return( true );
  if( c.done ) return( false );
  std::size_t got = 0;
  if( c.left >= 0 ) got = fill( c , c.buf , f_b );
  else if constexpr( kIndirect ) {          // the addresses of a source
   got = std::min( f_b , std::size_t( c.srcEnd - c.src ) );
   for( std::size_t i = 0 ; i < got ; ++i ) c.buf[ i ] = c.src + i;
   c.src += got;
   }
  c.cur = c.buf;
  c.end = c.buf + got;
  if( got == 0 ) c.done = true;
  return( got > 0 );
  }

 /// the item of a buffer
 static const T & item( const E & e ) {
  if constexpr( kIndirect ) return( *e );
  else return( e );
  }

 /// what a merger writes of the item e: e, or the item if o is the output
 template< class O >
 static const O & put( const E & e ) {
  if constexpr( std::is_same_v< O , E > ) return( e );
  else return( *e );
  }

 /// up to cap items of the children of v merged into o
 template< class O >
 std::size_t fill( Node & v , O * o , std::size_t cap ) {
  Node & x = f_node[ std::size_t( v.left ) ];
  Node & y = f_node[ std::size_t( v.right ) ];
  O * const o0 = o , * const oe = o + cap;
  while( o != oe ) {
   Node * one = nullptr;                    // the only child left
   if( ! ready( x ) ) one = &y;
   else if( ! ready( y ) ) one = &x;
   if( one ) {
    while( ( o != oe ) && ready( *one ) ) {
     const std::size_t m = std::min( std::size_t( one->end - one->cur ) ,
                                     std::size_t( oe - o ) );
     for( std::size_t i = 0 ; i < m ; ++i )
      o[ i ] = put< O >( one->cur[ i ] );
     o += m;
     one->cur += m;
     }
    break;
    }
   const E * a = x.cur , * b = y.cur;
   const std::size_t m = std::min( { std::size_t( x.end - a ) ,
                                     std::size_t( y.end - b ) ,
                                     std::size_t( oe - o ) } );
   for( std::size_t i = 0 ; i < m ; ++i ) { // no end checked, no jump
    const bool tb = f_less( item( *b ) , item( *a ) );
    *o++ = put< O >( *pick( tb , a , b ) );
    b += tb;
    a += ! tb;
    }
   x.cur = a;
   y.cur = b;
   }
  return( std::size_t( o - o0 ) );
  }

 std::size_t f_b;
 C f_less;
 std::vector< Node > f_node;
 std::vector< E > f_space;

 };  // end( class MergeFunnel )

/// the items of R stably merged, the reference of the audit
template< class I , class C >
std::vector< std::iter_value_t< I > > mergeReference(
                        const std::vector< std::pair< I , I > > & R ,
                        C less )
{
 std::vector< std::iter_value_t< I > > all;
 for( const auto & r : R ) all.insert( all.end() , r.first , r.second );
 std::stable_sort( all.begin() , all.end() , less );
 return( all );
 }

}  // end( namespace detail )

template< class I , class O , class C >
O multiwayMerge( const std::vector< std::pair< I , I > > & R , O out ,
                 C less )
{
 using T = std::iter_value_t< I >;
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  for( const auto & r : R )
   if( ! std::is_sorted( r.first , r.second , less ) ) return( false );
  return( true ); } ) );
 [[maybe_unused]] const O start = out;
 std::size_t n = 0 , live = 0 , one = 0 , two = 0;
 for( std::size_t s = 0 ; s < R.size() ; ++s ) {
  const auto len = std::size_t( std::distance( R[ s ].first ,
                                               R[ s ].second ) );
  n += len;
  if( len > 0 ) {
   ( live == 0 ? one : two ) = s;
   ++live;
   }
  }
 if( live == 1 ) out = std::copy( R[ one ].first , R[ one ].second , out );
 else if( live == 2 )
  out = detail::mergeTwo( R[ one ].first , R[ one ].second ,
                          R[ two ].first , R[ two ].second , out , less );
 else if constexpr( std::contiguous_iterator< I > &&
                    std::contiguous_iterator< O > &&
                    std::is_trivially_copyable_v< T > &&
                    std::is_default_constructible_v< T > ) {
  if( ( live > 2 ) && ( live <= 4 ) && ( sizeof( T ) > 16 ) ) {  // heads
   const T * a[ 4 ] , * e[ 4 ];
   std::size_t j = 0;
   for( const auto & r : R )
    if( r.first != r.second ) {
     a[ j ] = std::to_address( r.first );
     e[ j ] = a[ j ] + ( r.second - r.first );
     ++j;
     }
   T * o = std::to_address( out );
   out += detail::mergeFew( a , e , j , o , less ) - o;
   }
  else if( live > 2 ) {                    // the funnel
   constexpr bool ind = false;
   using F = detail::MergeFunnel< T , C , ind >;
   F f( R , detail::kFunnelBuffer / sizeof( typename F::E ) , less );
   T * o = std::to_address( out );
   out += f.run( o , n ) - o;
   }
  }
 else if( live > 2 ) {
  FlatLoserTree< T , C > L( R.size() , less );
  std::vector< I > p( R.size() );
  for( std::size_t s = 0 ; s < R.size() ; ++s ) {
   p[ s ] = R[ s ].first;
   if( p[ s ] != R[ s ].second ) L.start( s , *p[ s ] );
   }
  L.build();
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::size_t s = L.winner();
   *out = L.top();
   ++out;
   if( ++p[ s ] != R[ s ].second ) L.replaceTop( *p[ s ] );
   else L.exhaustTop();
   }
  }
 if constexpr( std::forward_iterator< O > ) {
  ADS_AUDIT( detail::contract( [ & ]( void ) {
   const auto ref = detail::mergeReference( R , less );
   std::size_t i = 0;
   for( O q = start ; q != out ; ++q , ++i )
    if( ( i >= ref.size() ) || less( *q , ref[ i ] ) ||
        less( ref[ i ] , *q ) ) return( false );
   return( i == ref.size() ); } ) );
  }
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File MultiwayMerge.tpp ------------------------*/
/*--------------------------------------------------------------------------*/

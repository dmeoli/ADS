/*--------------------------------------------------------------------------*/
/*----------------------- File AdaptiveRadixTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of AdaptiveRadixTree [see AdaptiveRadixTree.h], with
 * their contracts.
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

#include <algorithm>
#include <bit>
#include <new>

#if defined( __SSE2__ ) || defined( _M_X64 )
 #include <emmintrin.h>
 #define ADS_ART_SSE2 1
#endif

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- THE ALLOCATION ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
void * AdaptiveRadixTree< V >::Slab::take( std::size_t bytes )
{
 const std::size_t c = ( bytes + STEP - 1 ) / STEP;
 if( c > CLASSES )
  return( ::operator new( bytes ) );
 if( Free * f = f_free[ c ] ) {             // a slot of the size, freed
  f_free[ c ] = f->next;
  return( f );
  }
 const std::size_t size = c * STEP;
 if( std::size_t( f_end - f_cur ) < size ) {   // a new block
  const std::size_t b = f_blocks.empty() ? std::size_t( 4096 ) :
   std::min< std::size_t >( 2 * f_blocks.back().second ,
                            std::size_t( 1 ) << 21 );
  char * p = HugePageAllocator< char >().allocate( b );
  f_blocks.emplace_back( p , b );
  f_cur = p;
  f_end = p + b;
  }
 void * p = f_cur;
 f_cur += size;
 return( p );
 }

template< class V >
void AdaptiveRadixTree< V >::Slab::give( void * p , std::size_t bytes )
 noexcept
{
 const std::size_t c = ( bytes + STEP - 1 ) / STEP;
 if( c > CLASSES ) {
  ::operator delete( p );
  return;
  }
 Free * f = static_cast< Free * >( p );
 f->next = f_free[ c ];
 f_free[ c ] = f;
 }

template< class V >
void AdaptiveRadixTree< V >::Slab::release( void ) noexcept
{
 for( const auto & [ p , b ] : f_blocks )
  HugePageAllocator< char >().deallocate( p , b );
 f_blocks.clear();
 f_cur = f_end = nullptr;
 for( auto & f : f_free ) f = nullptr;
 }

template< class V >
void AdaptiveRadixTree< V >::Slab::swap( Slab & o ) noexcept
{
 std::swap( f_blocks , o.f_blocks );
 std::swap( f_cur , o.f_cur );
 std::swap( f_end , o.f_end );
 for( std::size_t c = 0 ; c <= CLASSES ; ++c )
  std::swap( f_free[ c ] , o.f_free[ c ] );
 }

template< class V >
auto AdaptiveRadixTree< V >::newLeaf( std::string_view k , const V & v )
 -> Leaf *
{
 static_assert( alignof( Leaf ) <= 16 , "a leaf on a slot of 16 bytes" );
 void * p = f_slab.take( sizeof( Leaf ) + k.size() );
 Leaf * l = ::new( p ) Leaf{ v , std::uint32_t( k.size() ) };
 if( ! k.empty() )
  std::memcpy( reinterpret_cast< char * >( l + 1 ) , k.data() , k.size() );
 return( l );
 }

template< class V >
void AdaptiveRadixTree< V >::freeLeaf( Leaf * l ) noexcept
{
 const std::size_t bytes = sizeof( Leaf ) + l->len;
 l->~Leaf();
 f_slab.give( l , bytes );
 }

template< class V >
template< class N >
N * AdaptiveRadixTree< V >::make( void )
{
 if constexpr( std::is_same_v< N , Node4 > )
  return( f_n4.make() );
 else if constexpr( std::is_same_v< N , Node16 > )
  return( f_n16.make() );
 else if constexpr( std::is_same_v< N , Node48 > )
  return( f_n48.make() );
 else
  return( f_n256.make() );
 }

template< class V >
void AdaptiveRadixTree< V >::freeNode( Node * n ) noexcept
{
 switch( n->type ) {
  case( N4 ): f_n4.free( static_cast< Node4 * >( n ) ); break;
  case( N16 ): f_n16.free( static_cast< Node16 * >( n ) ); break;
  case( N48 ): f_n48.free( static_cast< Node48 * >( n ) ); break;
  default: f_n256.free( static_cast< Node256 * >( n ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE CHILDREN -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
inline void * const * AdaptiveRadixTree< V >::child( const Node * n ,
                                                      unsigned char c )
{
 switch( n->type ) {
  case( N4 ): {
   const Node4 * m = static_cast< const Node4 * >( n );
   for( unsigned i = 0 ; i < m->count ; ++i )
    if( m->key[ i ] == c )
     return( &m->child[ i ] );
   return( nullptr );
   }
  case( N16 ): {
   const Node16 * m = static_cast< const Node16 * >( n );
#ifdef ADS_ART_SSE2
   const __m128i e = _mm_cmpeq_epi8( _mm_set1_epi8( char( c ) ) ,
                      _mm_load_si128( reinterpret_cast< const __m128i * >(
                                       m->key ) ) );
   const unsigned b = unsigned( _mm_movemask_epi8( e ) ) &
                      ( ( 1u << m->count ) - 1 );
   return( b ? &m->child[ std::countr_zero( b ) ] : nullptr );
#else
   for( unsigned i = 0 ; i < m->count ; ++i )
    if( m->key[ i ] == c )
     return( &m->child[ i ] );
   return( nullptr );
#endif
   }
  case( N48 ): {
   const Node48 * m = static_cast< const Node48 * >( n );
   return( m->index[ c ] ? &m->child[ m->index[ c ] - 1 ] : nullptr );
   }
  default: {
   const Node256 * m = static_cast< const Node256 * >( n );
   return( m->child[ c ] ? &m->child[ c ] : nullptr );
   }
  }
 }

template< class V >
inline void * * AdaptiveRadixTree< V >::child( Node * n , unsigned char c )
{
 return( const_cast< void * * >( child( static_cast< const Node * >( n ) ,
                                        c ) ) );
 }

template< class V >
auto AdaptiveRadixTree< V >::minimum( const void * p ) -> const Leaf *
{
 while( ! isLeaf( p ) ) {
  const Node * n = static_cast< const Node * >( p );
  if( n->term )
   return( n->term );
  switch( n->type ) {
   case( N4 ): p = static_cast< const Node4 * >( n )->child[ 0 ]; break;
   case( N16 ): p = static_cast< const Node16 * >( n )->child[ 0 ]; break;
   case( N48 ): {
    const Node48 * m = static_cast< const Node48 * >( n );
    unsigned c = 0;
    while( ! m->index[ c ] ) ++c;
    p = m->child[ m->index[ c ] - 1 ];
    break;
    }
   default: {
    const Node256 * m = static_cast< const Node256 * >( n );
    unsigned c = 0;
    while( ! m->child[ c ] ) ++c;
    p = m->child[ c ];
    }
   }
  }
 return( asLeaf( p ) );
 }

template< class V >
std::uint32_t AdaptiveRadixTree< V >::mismatch( const Node * n ,
                                                std::string_view k ,
                                                std::size_t d )
{
 const std::uint32_t m = std::min< std::uint32_t >( n->plen , MAXP );
 std::uint32_t i = 0;
 for( ; i < m ; ++i )
  if( ( d + i >= k.size() ) || ( n->prefix[ i ] != (unsigned char) k[ d + i ] ) )
   return( i );
 if( n->plen > MAXP ) {                     // the rest from a leaf below
  const char * full = minimum( n )->key();
  for( ; i < n->plen ; ++i )
   if( ( d + i >= k.size() ) || ( full[ d + i ] != k[ d + i ] ) )
    return( i );
  }
 return( i );
 }

template< class V >
void AdaptiveRadixTree< V >::addChild( void * * ref , Node * n ,
                                       unsigned char c , void * x )
{
 auto header = []( Node * to , const Node * from ) {
  to->count = from->count;
  to->plen = from->plen;
  std::memcpy( to->prefix , from->prefix , MAXP );
  to->term = from->term;
  };
 switch( n->type ) {
  case( N4 ): {
   Node4 * m = static_cast< Node4 * >( n );
   if( m->count < 4 ) {
    unsigned i = 0;
    while( ( i < m->count ) && ( m->key[ i ] < c ) ) ++i;
    std::memmove( m->key + i + 1 , m->key + i , m->count - i );
    std::memmove( m->child + i + 1 , m->child + i ,
                  ( m->count - i ) * sizeof( void * ) );
    m->key[ i ] = c;
    m->child[ i ] = x;
    ++m->count;
    return;
    }
   Node16 * g = make< Node16 >();           // grow
   header( g , m );
   std::memcpy( g->key , m->key , 4 );
   std::memcpy( g->child , m->child , 4 * sizeof( void * ) );
   *ref = g;
   f_n4.free( m );
   addChild( ref , g , c , x );
   return;
   }
  case( N16 ): {
   Node16 * m = static_cast< Node16 * >( n );
   if( m->count < 16 ) {
    unsigned i = 0;
    while( ( i < m->count ) && ( m->key[ i ] < c ) ) ++i;
    std::memmove( m->key + i + 1 , m->key + i , m->count - i );
    std::memmove( m->child + i + 1 , m->child + i ,
                  ( m->count - i ) * sizeof( void * ) );
    m->key[ i ] = c;
    m->child[ i ] = x;
    ++m->count;
    return;
    }
   Node48 * g = make< Node48 >();
   header( g , m );
   for( unsigned i = 0 ; i < 16 ; ++i ) {
    g->child[ i ] = m->child[ i ];
    g->index[ m->key[ i ] ] = std::uint8_t( i + 1 );
    }
   for( unsigned i = 16 ; i < 48 ; ++i ) g->child[ i ] = nullptr;
   *ref = g;
   f_n16.free( m );
   addChild( ref , g , c , x );
   return;
   }
  case( N48 ): {
   Node48 * m = static_cast< Node48 * >( n );
   if( m->count < 48 ) {
    unsigned i = 0;
    while( m->child[ i ] ) ++i;             // a free place
    m->child[ i ] = x;
    m->index[ c ] = std::uint8_t( i + 1 );
    ++m->count;
    return;
    }
   Node256 * g = make< Node256 >();
   header( g , m );
   for( unsigned b = 0 ; b < 256 ; ++b )
    if( m->index[ b ] )
     g->child[ b ] = m->child[ m->index[ b ] - 1 ];
   *ref = g;
   f_n48.free( m );
   addChild( ref , g , c , x );
   return;
   }
  default: {
   Node256 * m = static_cast< Node256 * >( n );
   m->child[ c ] = x;
   ++m->count;
   }
  }
 }

template< class V >
void AdaptiveRadixTree< V >::removeChild( void * * ref , Node * n ,
                                          unsigned char c , void * * slot )
{
 switch( n->type ) {
  case( N4 ): case( N16 ): {
   unsigned char * key;
   void * * ch;
   if( n->type == N4 ) {
    key = static_cast< Node4 * >( n )->key;
    ch = static_cast< Node4 * >( n )->child;
    }
   else {
    key = static_cast< Node16 * >( n )->key;
    ch = static_cast< Node16 * >( n )->child;
    }
   const unsigned i = unsigned( slot - ch );
   std::memmove( key + i , key + i + 1 , n->count - i - 1 );
   std::memmove( ch + i , ch + i + 1 , ( n->count - i - 1 ) *
                                        sizeof( void * ) );
   break;
   }
  case( N48 ): {
   Node48 * m = static_cast< Node48 * >( n );
   m->child[ m->index[ c ] - 1 ] = nullptr;
   m->index[ c ] = 0;
   break;
   }
  default:
   static_cast< Node256 * >( n )->child[ c ] = nullptr;
  }
 --n->count;
 shrink( ref , n );
 }

template< class V >
void AdaptiveRadixTree< V >::shrink( void * * ref , Node * n )
{
 auto header = []( Node * to , const Node * from ) {
  to->count = from->count;
  to->plen = from->plen;
  std::memcpy( to->prefix , from->prefix , MAXP );
  to->term = from->term;
  };
 if( n->count == 0 ) {                      // the term alone, or nothing
  *ref = n->term ? tag( n->term ) : nullptr;
  freeNode( n );
  return;
  }
 if( ( n->count == 1 ) && ! n->term && ( n->type == N4 ) ) {
  Node4 * m = static_cast< Node4 * >( n );  // into its only child
  void * x = m->child[ 0 ];
  if( ! isLeaf( x ) ) {                     // the prefixes joined
   Node * y = static_cast< Node * >( x );
   unsigned char buf[ MAXP ];
   std::uint32_t k = std::min< std::uint32_t >( m->plen , MAXP );
   std::memcpy( buf , m->prefix , k );
   if( k < MAXP )
    buf[ k++ ] = m->key[ 0 ];
   std::memcpy( buf + k , y->prefix ,
                std::min< std::uint32_t >( y->plen , MAXP - k ) );
   y->plen += m->plen + 1;
   std::memcpy( y->prefix , buf , MAXP );
   }
  *ref = x;
  f_n4.free( m );
  return;
  }
 if( ( n->type == N16 ) && ( n->count <= 3 ) ) {
  Node16 * m = static_cast< Node16 * >( n );
  Node4 * s = make< Node4 >();
  header( s , m );
  std::memcpy( s->key , m->key , m->count );
  std::memcpy( s->child , m->child , m->count * sizeof( void * ) );
  *ref = s;
  f_n16.free( m );
  }
 else if( ( n->type == N48 ) && ( n->count <= 12 ) ) {
  Node48 * m = static_cast< Node48 * >( n );
  Node16 * s = make< Node16 >();
  header( s , m );
  unsigned j = 0;
  for( unsigned b = 0 ; b < 256 ; ++b )
   if( m->index[ b ] ) {
    s->key[ j ] = std::uint8_t( b );
    s->child[ j++ ] = m->child[ m->index[ b ] - 1 ];
    }
  *ref = s;
  f_n48.free( m );
  }
 else if( ( n->type == N256 ) && ( n->count <= 37 ) ) {
  Node256 * m = static_cast< Node256 * >( n );
  Node48 * s = make< Node48 >();
  header( s , m );
  unsigned j = 0;
  for( unsigned b = 0 ; b < 256 ; ++b )
   if( m->child[ b ] ) {
    s->child[ j ] = m->child[ b ];
    s->index[ b ] = std::uint8_t( ++j );
    }
  for( ; j < 48 ; ++j ) s->child[ j ] = nullptr;
  *ref = s;
  f_n256.free( m );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ OPERATIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
inline void AdaptiveRadixTree< V >::ahead( const void * p , unsigned char c )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Winvalid-offsetof"
 constexpr std::size_t i48 = offsetof( Node48 , index );
 constexpr std::size_t c256 = offsetof( Node256 , child );
 #pragma GCC diagnostic pop
 const char * b = static_cast< const char * >( p );
 if( isLeaf( p ) ) {
  __builtin_prefetch( b - 1 );
  return;
  }
 __builtin_prefetch( b );                   // the header, a Node4, the
 __builtin_prefetch( b + 64 );              // keys and the children of a
 __builtin_prefetch( b + 128 );             // Node16
 __builtin_prefetch( b + i48 + c );         // the index of a Node48
 __builtin_prefetch( b + c256 + 8 * c );    // the child of a Node256
#else
 ( void ) p;
 ( void ) c;
#endif
 }

template< class V >
inline auto AdaptiveRadixTree< V >::search( std::string_view k ) const
 -> const Leaf *
{
 const void * p = f_root;
 std::size_t d = 0;
 while( p ) {
  if( isLeaf( p ) ) {
   const Leaf * l = asLeaf( p );
   return( ( l->len == k.size() ) &&
           ( std::memcmp( l->key() , k.data() , k.size() ) == 0 ) ? l :
                                                                    nullptr );
   }
  const Node * n = static_cast< const Node * >( p );
  if( n->plen ) {                           // the first MAXP bytes only
   if( d + n->plen > k.size() )
    return( nullptr );
   if( std::memcmp( n->prefix , k.data() + d ,
                    std::min< std::uint32_t >( n->plen , MAXP ) ) != 0 )
    return( nullptr );
   d += n->plen;
   }
  if( d == k.size() ) {
   const Leaf * l = n->term;
   return( l && ( l->len == k.size() ) &&
           ( std::memcmp( l->key() , k.data() , k.size() ) == 0 ) ? l :
                                                                    nullptr );
   }
  void * const * s = child( n , (unsigned char) k[ d ] );
  if( ! s )
   return( nullptr );
  p = *s;
  ++d;
  if( d < k.size() )                        // the byte of p, if no prefix
   ahead( p , (unsigned char) k[ d ] );
  }
 return( nullptr );
 }

template< class V >
inline const V * AdaptiveRadixTree< V >::find( std::string_view k ) const
{
 const Leaf * l = search( k );
 return( l ? &l->value : nullptr );
 }

template< class V >
inline V * AdaptiveRadixTree< V >::find( std::string_view k )
{
 return( const_cast< V * >( std::as_const( *this ).find( k ) ) );
 }

template< class V >
inline std::optional< V > AdaptiveRadixTree< V >::lookup(
                                               std::string_view k ) const
{
 const V * v = find( k );
 return( v ? std::optional< V >( *v ) : std::nullopt );
 }

template< class V >
inline bool AdaptiveRadixTree< V >::contains( std::string_view k ) const
{
 return( search( k ) != nullptr );
 }

template< class V >
std::size_t AdaptiveRadixTree< V >::size( void ) const
{
 return( f_size );
 }

template< class V >
bool AdaptiveRadixTree< V >::empty( void ) const
{
 return( f_size == 0 );
 }

template< class V >
void AdaptiveRadixTree< V >::split( void * * ref , std::size_t d ,
                                    std::string_view k , const V & v )
{
 void * p = *ref;
 Leaf * o = asLeaf( p );
 const std::string_view ok = o->view();
 std::size_t i = d;                         // the bytes shared from d on
 const std::size_t m = std::min( ok.size() , k.size() );
 while( ( i < m ) && ( ok[ i ] == k[ i ] ) ) ++i;
 Node4 * n = make< Node4 >();
 n->plen = std::uint32_t( i - d );
 std::memcpy( n->prefix , k.data() + d ,
              std::min< std::uint32_t >( n->plen , MAXP ) );
 Leaf * w = newLeaf( k , v );
 if( i == ok.size() ) n->term = o;
 else addChild( ref , n , (unsigned char) ok[ i ] , p );
 if( i == k.size() ) n->term = w;
 else addChild( ref , n , (unsigned char) k[ i ] , tag( w ) );
 *ref = n;
 }

template< class V >
bool AdaptiveRadixTree< V >::insert( std::string_view k , const V & v )
{
 if( ! f_root ) {
  f_root = tag( newLeaf( k , v ) );
  ++f_size;
  ADS_POST( contains( k ) );
  return( true );
  }
 // the first pass goes down as a search, comparing only the bytes of the
 // prefixes kept in the nodes; if no prefix on the way was longer than
 // those bytes, the way is the one of k, and k goes where the search stops.
 // Otherwise it stops at a leaf l below the last node that k reaches: the
 // bytes of k shared with l are c, and every node on the way that begins
 // before c has l below it, so that its whole prefix is that of l, and the
 // second pass goes down again to the node or the leaf that splits at c
 const Leaf * l = nullptr;
 {
  void * * ref = &f_root;
  std::size_t d = 0;
  bool skipped = false;                     // a prefix longer than MAXP
  for( ; ; ) {
   void * p = *ref;
   if( isLeaf( p ) ) {
    l = asLeaf( p );
    if( skipped || ( l->view() == k ) )
     break;
    split( ref , d , k , v );               // the way is that of k
    ++f_size;
    ADS_POST( contains( k ) );
    ADS_INVARIANT( ( f_size > 2048 ) || valid() );
    return( true );
    }
   Node * n = static_cast< Node * >( p );
   if( ( d + n->plen > k.size() ) ||
       ( std::memcmp( n->prefix , k.data() + d ,
                      std::min< std::uint32_t >( n->plen , MAXP ) ) ) ) {
    l = minimum( n );
    break;
    }
   skipped |= n->plen > MAXP;
   d += n->plen;
   if( d == k.size() ) {                    // k ends at n
    if( skipped ) {
     l = minimum( n );
     break;
     }
    const bool added = ! n->term;
    if( added )
     n->term = newLeaf( k , v );
    else
     n->term->value = v;
    f_size += added ? 1 : 0;
    ADS_POST( contains( k ) );
    ADS_INVARIANT( ( f_size > 2048 ) || valid() );
    return( added );
    }
   void * * s = child( n , (unsigned char) k[ d ] );
   if( ! s ) {
    if( skipped ) {
     l = minimum( n );
     break;
     }
    addChild( ref , n , (unsigned char) k[ d ] , tag( newLeaf( k , v ) ) );
    ++f_size;
    ADS_POST( contains( k ) );
    ADS_INVARIANT( ( f_size > 2048 ) || valid() );
    return( true );
    }
   ref = s;
   ++d;
#ifdef ARTAHEAD
   if( d < k.size() )
    ahead( *ref , (unsigned char) k[ d ] );
#endif
   }
 }
 const std::string_view lk = l->view();
 std::size_t c = 0;                         // the bytes shared with l
 const std::size_t lim = std::min( lk.size() , k.size() );
 while( ( c < lim ) && ( lk[ c ] == k[ c ] ) ) ++c;
 if( ( c == lk.size() ) && ( c == k.size() ) ) {
  const_cast< Leaf * >( l )->value = v;
  return( false );
  }
 void * * ref = &f_root;
 std::size_t d = 0;
 for( ; ; ) {
  void * p = *ref;
  if( isLeaf( p ) ) {                       // the leaf splits at c
   split( ref , d , k , v );
   break;
   }
  Node * n = static_cast< Node * >( p );
  if( d + n->plen > c ) {                   // the prefix splits at c - d
   const std::uint32_t m = std::uint32_t( c - d );
   Node4 * top = make< Node4 >();
   top->plen = m;
   std::memcpy( top->prefix , n->prefix ,
                std::min< std::uint32_t >( m , MAXP ) );
   const unsigned char e = (unsigned char) lk[ c ];
   n->plen -= m + 1;
   std::memcpy( n->prefix , lk.data() + c + 1 ,
                std::min< std::uint32_t >( n->plen , MAXP ) );
   void * up = top;
   addChild( &up , top , e , n );
   Leaf * w = newLeaf( k , v );
   if( c == k.size() ) top->term = w;
   else addChild( &up , top , (unsigned char) k[ c ] , tag( w ) );
   *ref = top;
   break;
   }
  d += n->plen;
  if( d == k.size() ) {                     // k ends at n, not there yet
   n->term = newLeaf( k , v );
   break;
   }
  void * * s = child( n , (unsigned char) k[ d ] );
  if( ! s ) {
   addChild( ref , n , (unsigned char) k[ d ] , tag( newLeaf( k , v ) ) );
   break;
   }
  ref = s;
  ++d;
  }
 ++f_size;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( ( f_size > 2048 ) || valid() );
 return( true );
 }

template< class V >
bool AdaptiveRadixTree< V >::remove( std::string_view k )
{
 void * * ref = &f_root , * * pref = nullptr;
 Node * parent = nullptr;
 unsigned char pc = 0;
 std::size_t d = 0;
 bool gone = false;
 for( ; ; ) {
  void * p = *ref;
  if( ! p )
   break;
  if( isLeaf( p ) ) {
   Leaf * l = asLeaf( p );
   if( l->view() != k )
    break;
   if( parent )
    removeChild( pref , parent , pc , ref );
   else
    f_root = nullptr;
   freeLeaf( l );
   gone = true;
   break;
   }
  Node * n = static_cast< Node * >( p );
  if( n->plen ) {
   if( ( d + n->plen > k.size() ) ||
       ( std::memcmp( n->prefix , k.data() + d ,
                      std::min< std::uint32_t >( n->plen , MAXP ) ) != 0 ) )
    break;
   d += n->plen;
   }
  if( d == k.size() ) {
   if( n->term && ( n->term->view() == k ) ) {
    freeLeaf( n->term );
    n->term = nullptr;
    shrink( ref , n );
    gone = true;
    }
   break;
   }
  void * * s = child( n , (unsigned char) k[ d ] );
  if( ! s )
   break;
  pref = ref;
  parent = n;
  pc = (unsigned char) k[ d ];
  ref = s;
  ++d;
  }
 f_size -= gone ? 1 : 0;
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( ( f_size > 2048 ) || valid() );
 return( gone );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE VISITS -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
inline void AdaptiveRadixTree< V >::children( const Node * n )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 void * const * c;
 if( n->type == N4 )
  c = static_cast< const Node4 * >( n )->child;
 else if( n->type == N16 )
  c = static_cast< const Node16 * >( n )->child;
 else
  return;
 for( unsigned i = 0 ; i < n->count ; ++i )
  __builtin_prefetch( static_cast< const char * >( c[ i ] ) -
                      ( isLeaf( c[ i ] ) ? 1 : 0 ) );
#else
 ( void ) n;
#endif
 }

template< class V >
template< class F >
void AdaptiveRadixTree< V >::each( const Node * n , F && f )
{
 switch( n->type ) {
  case( N4 ): {
   const Node4 * m = static_cast< const Node4 * >( n );
   for( unsigned i = 0 ; i < m->count ; ++i ) f( m->child[ i ] );
   break;
   }
  case( N16 ): {
   const Node16 * m = static_cast< const Node16 * >( n );
   for( unsigned i = 0 ; i < m->count ; ++i ) f( m->child[ i ] );
   break;
   }
  case( N48 ): {
   const Node48 * m = static_cast< const Node48 * >( n );
   for( unsigned b = 0 ; b < 256 ; ++b )
    if( m->index[ b ] ) f( m->child[ m->index[ b ] - 1 ] );
   break;
   }
  default: {
   const Node256 * m = static_cast< const Node256 * >( n );
   for( unsigned b = 0 ; b < 256 ; ++b )
    if( m->child[ b ] ) f( m->child[ b ] );
   }
  }
 }

template< class V >
template< class F >
void AdaptiveRadixTree< V >::visit( const void * p , F & f )
{
 if( isLeaf( p ) ) {
  const Leaf * l = asLeaf( p );
  f( l->view() , l->value );
  return;
  }
 const Node * n = static_cast< const Node * >( p );
 children( n );
 if( n->term )
  f( n->term->view() , n->term->value );
 each( n , [ & ]( const void * c ) { visit( c , f ); } );
 }

template< class V >
template< class F >
void AdaptiveRadixTree< V >::forEach( F && f ) const
{
 if( f_root )
  visit( f_root , f );
 }

template< class V >
template< class F >
void AdaptiveRadixTree< V >::forPrefix( std::string_view p , F && f ) const
{
 const void * x = f_root;
 std::size_t d = 0;
 while( x ) {
  if( isLeaf( x ) ) {
   const Leaf * l = asLeaf( x );
   if( l->view().substr( 0 , p.size() ) == p )
    f( l->view() , l->value );
   return;
   }
  const Node * n = static_cast< const Node * >( x );
  const std::string_view m = minimum( n )->view();   // all share its path
  if( d + n->plen >= p.size() ) {           // the subtree, if it fits
   if( m.substr( 0 , p.size() ) == p )
    visit( x , f );
   return;
   }
  if( m.substr( d , n->plen ) != p.substr( d , n->plen ) )
   return;
  d += n->plen;
  void * const * s = child( n , (unsigned char) p[ d ] );
  if( ! s )
   return;
  x = *s;
  ++d;
  }
 }

template< class V >
std::vector< std::string > AdaptiveRadixTree< V >::keysWithPrefix(
                                                std::string_view p ) const
{
 std::vector< std::string > K;
 forPrefix( p , [ & ]( std::string_view k , const V & ) {
             K.emplace_back( k ); } );
 return( K );
 }

template< class V >
auto AdaptiveRadixTree< V >::const_iterator::step( std::vector< Frame > & S )
 -> const Leaf *
{
 while( ! S.empty() ) {
  Frame & f = S.back();
  const Node * n = f.node;
  if( f.next < 0 ) {                        // the term first
   f.next = 0;
   if( n->term )
    return( n->term );
   continue;
   }
  const void * c = nullptr;
  switch( n->type ) {
   case( N4 ): {
    const Node4 * m = static_cast< const Node4 * >( n );
    if( f.next < m->count ) c = m->child[ f.next++ ];
    break;
    }
   case( N16 ): {
    const Node16 * m = static_cast< const Node16 * >( n );
    if( f.next < m->count ) c = m->child[ f.next++ ];
    break;
    }
   case( N48 ): {
    const Node48 * m = static_cast< const Node48 * >( n );
    while( ( f.next < 256 ) && ! c ) {
     const unsigned i = m->index[ f.next++ ];
     if( i ) c = m->child[ i - 1 ];
     }
    break;
    }
   default: {
    const Node256 * m = static_cast< const Node256 * >( n );
    while( ( f.next < 256 ) && ! c ) c = m->child[ f.next++ ];
    }
   }
  if( ! c ) {
   S.pop_back();
   continue;
   }
  if( isLeaf( c ) )
   return( asLeaf( c ) );
  // the children of the next sibling too, if a node of a few children,
  // so that its leaves are on their way while this subtree is visited
  if( ( n->type <= N16 ) && ( f.next < n->count ) ) {
   const void * s = n->type == N4 ?
    static_cast< const Node4 * >( n )->child[ f.next ] :
    static_cast< const Node16 * >( n )->child[ f.next ];
   if( ! isLeaf( s ) )
    children( static_cast< const Node * >( s ) );
   }
  S.push_back( Frame{ static_cast< const Node * >( c ) , -1 } );
  children( S.back().node );
  }
 return( nullptr );
 }

template< class V >
auto AdaptiveRadixTree< V >::begin( void ) const -> const_iterator
{
 return( const_iterator( f_root ) );
 }

template< class V >
auto AdaptiveRadixTree< V >::end( void ) const -> const_iterator
{
 return( const_iterator() );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE LIFETIME --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
AdaptiveRadixTree< V >::AdaptiveRadixTree( const AdaptiveRadixTree & o )
{
 o.forEach( [ & ]( std::string_view k , const V & v ) { insert( k , v ); } );
 }

template< class V >
AdaptiveRadixTree< V >::AdaptiveRadixTree( AdaptiveRadixTree && o ) noexcept
{
 swap( o );
 }

template< class V >
auto AdaptiveRadixTree< V >::operator=( AdaptiveRadixTree o ) noexcept
 -> AdaptiveRadixTree &
{
 swap( o );
 return( *this );
 }

template< class V >
AdaptiveRadixTree< V >::~AdaptiveRadixTree()
{
 clear();
 }

template< class V >
void AdaptiveRadixTree< V >::swap( AdaptiveRadixTree & o ) noexcept
{
 std::swap( f_root , o.f_root );
 std::swap( f_size , o.f_size );
 f_n4.swap( o.f_n4 );
 f_n16.swap( o.f_n16 );
 f_n48.swap( o.f_n48 );
 f_n256.swap( o.f_n256 );
 f_slab.swap( o.f_slab );
 }

template< class V >
void AdaptiveRadixTree< V >::destroy( void * p ) noexcept
{
 if( isLeaf( p ) ) {
  freeLeaf( asLeaf( p ) );
  return;
  }
 Node * n = static_cast< Node * >( p );
 if( n->term )
  freeLeaf( n->term );
 each( n , [ & ]( void * c ) { destroy( c ); } );
 freeNode( n );
 }

template< class V >
void AdaptiveRadixTree< V >::clear( void )
{
 if( f_root )
  destroy( f_root );
 f_root = nullptr;
 f_size = 0;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ INVARIANT ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
bool AdaptiveRadixTree< V >::valid( void ) const
{
 std::size_t leaves = 0;
 std::string path;
 return( ( ! f_root || valid( f_root , path , leaves , true ) ) &&
         ( leaves == f_size ) );
 }

template< class V >
bool AdaptiveRadixTree< V >::valid( const void * p , std::string & path ,
                                    std::size_t & leaves , bool root ) const
{
 if( isLeaf( p ) ) {
  ++leaves;
  const std::string_view k = asLeaf( p )->view();
  return( ( k.size() >= path.size() ) &&
          ( k.substr( 0 , path.size() ) == path ) );
  }
 const Node * n = static_cast< const Node * >( p );
 const std::string_view m = minimum( n )->view();
 if( ( m.size() < path.size() + n->plen ) ||
     ( m.substr( 0 , path.size() ) != path ) )
  return( false );
 const std::size_t d = path.size();
 path.append( m.substr( d , n->plen ) );     // the whole prefix
 if( std::memcmp( n->prefix , path.data() + d ,
                  std::min< std::uint32_t >( n->plen , MAXP ) ) != 0 )
  return( false );
 if( ! root && ( n->count + ( n->term ? 1 : 0 ) < 2 ) )
  return( false );
 if( n->term ) {
  ++leaves;
  if( n->term->view() != path )
   return( false );
  }
 const unsigned cap = n->type == N4 ? 4 : n->type == N16 ? 16 :
                      n->type == N48 ? 48 : 256;
 const unsigned low = n->type == N4 ? 0 : n->type == N16 ? 3 :
                      n->type == N48 ? 12 : 37;   // the shrinking counts
 if( ( n->count > cap ) || ( ( n->type != N4 ) && ( n->count <= low ) ) )
  return( false );
 bool ok = true;
 unsigned seen = 0;
 int last = -1;
 for( unsigned b = 0 ; ok && ( b < 256 ) ; ++b ) {
  void * const * s = child( n , (unsigned char) b );
  if( ! s )
   continue;
  ++seen;
  last = int( b );
  path.push_back( char( b ) );
  ok = valid( *s , path , leaves , false );
  path.pop_back();
  }
 ( void ) last;
 if( n->type == N4 || n->type == N16 ) {     // the bytes sorted
  const unsigned char * key = n->type == N4 ?
   static_cast< const Node4 * >( n )->key :
   static_cast< const Node16 * >( n )->key;
  for( unsigned i = 1 ; i < n->count ; ++i )
   if( key[ i - 1 ] >= key[ i ] )
    return( false );
  }
 path.resize( d );
 return( ok && ( seen == n->count ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File AdaptiveRadixTree.tpp --------------------*/
/*--------------------------------------------------------------------------*/
